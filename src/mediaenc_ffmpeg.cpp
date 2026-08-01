/////////////////////////////////////////////////////////////////////////////
// Name:        mediaenc_ffmpeg.cpp
// Purpose:     FFMPEG Media Encoder
// Author:      Alex Thuering
// Created:     04.08.2007
// RCS-ID:      $Id: mediaenc_ffmpeg.cpp,v 1.56 2016/10/23 19:03:35 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "mediaenc_ffmpeg.h"
#include <wx/wx.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef INT64_C
#define INT64_C(val) val##LL
#define UINT64_C(val) val##ULL
#endif
extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libavutil/avstring.h>
#include <libavutil/channel_layout.h>
#include <libavcodec/avcodec.h>
}

#define AUDIO_BUF_SIZE 524288
#define VIDEO_BUF_SIZE 1835008

wxFfmpegMediaEncoder::wxFfmpegMediaEncoder(int threadCount) {
	m_threadCount = threadCount;
	m_gopSize = -1;
	m_outputCtx = NULL;
	m_videoStm = NULL;
	m_audioStm = NULL;
	m_videoCodec = NULL;
	m_audioCodec = NULL;
    m_nextVideoPts = 0;
    m_nextAudioPts = 0;
	m_audioFrame = NULL;
	m_picture = NULL;
	m_imgConvertCtx = NULL;
	m_videoOutbuf = NULL;
	m_audioFile = NULL;
}

wxFfmpegMediaEncoder::~wxFfmpegMediaEncoder() {
	CloseEncoder();
}

wxString wxFfmpegMediaEncoder::GetBackendVersion() {
#ifdef LIBAVFORMAT_VERSION_INT
	return wxString::Format(wxT("libavformat %d.%d.%d, libavcodec %d.%d.%d, libavutil %d.%d.%d"),
			LIBAVFORMAT_VERSION_INT >> 16, LIBAVFORMAT_VERSION_INT >> 8 & 0xFF, LIBAVFORMAT_VERSION_INT & 0xFF,
			LIBAVCODEC_VERSION_INT >> 16, LIBAVCODEC_VERSION_INT >> 8 & 0xFF, LIBAVCODEC_VERSION_INT & 0xFF,
			LIBAVUTIL_VERSION_INT >> 16, LIBAVUTIL_VERSION_INT >> 8 & 0xFF, LIBAVUTIL_VERSION_INT & 0xFF);
#else
	return wxString::Format(wxT("libavformat %d, libavcodec %d, libavutil %d"),
			LIBAVCODEC_VERSION_MAJOR, LIBAVFORMAT_VERSION_MAJOR, LIBAVUTIL_VERSION_MAJOR);
#endif
}

void print_error(const char *filename, int err) {
	char errbuf[128];
	const char *errbuf_ptr = errbuf;
	if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
		errbuf_ptr = strerror(AVUNERROR(err));
	wxLogError(wxT("%s: %s\n"), filename, errbuf_ptr);
}

bool wxFfmpegMediaEncoder::BeginEncode(const wxString& fileName, VideoFormat videoFormat, AudioFormat audioFormat,
		AspectRatio aspectRatio, int videoBitrate, bool cbr) {
	EndEncode();
	if (videoFormat == vfNONE) {
		AVCodecID codecId = audioFormat == afAC3 ? AV_CODEC_ID_AC3 : AV_CODEC_ID_MP2;
		if (!addAudioStream(codecId))
			return false;
		
		m_audioFile = fopen((const char*) fileName.ToUTF8(), "wb");
		if (!m_audioFile) {
			wxLogError("Could not open '%s'", fileName.c_str());
			return false;
		}
		return true;
	}
#if LIBAVCODEC_VERSION_MAJOR > 58
	const
#endif
AVOutputFormat* outputFormat = NULL;
	if (videoFormat == vfNONE || audioFormat == afNONE)
		outputFormat = av_guess_format(NULL, (const char*) fileName.ToUTF8(), NULL);
	else
		outputFormat = av_guess_format("dvd", NULL, NULL);
	if (!outputFormat) {
		wxLogError(wxT("Cannot open output format"));
		return false;
	}

	AVCodecID video_codec = videoFormat == vfNONE ? AV_CODEC_ID_NONE : AV_CODEC_ID_MPEG2VIDEO;
	AVCodecID audio_codec;
	if (audioFormat == afNONE)
		audio_codec = AV_CODEC_ID_NONE;
	else if (audioFormat == afAC3)
		audio_codec = AV_CODEC_ID_AC3;
	else
		audio_codec = AV_CODEC_ID_MP2;
	
	m_outputCtx = NULL;
	avformat_alloc_output_context2(&m_outputCtx, outputFormat, NULL, (const char*) fileName.ToUTF8());
	if (!m_outputCtx) {
		wxLogError(wxT("memory allocation error"));
		return false;
	}
	
	m_outputCtx->packet_size = 2048;

	// add video and audio streams
	if (!addVideoStream(video_codec, videoFormat, aspectRatio, videoBitrate, cbr))
		return false;
	if (!addAudioStream(audio_codec))
		return false;

	// open the output file
	if (avio_open2(&m_outputCtx->pb, (const char*) fileName.ToUTF8(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
		wxLogError(wxT("Could not open '%s'"), fileName.c_str());
		return false;
	}
	
	// write the stream header
	AVDictionary *opts = NULL;
	av_dict_set(&opts, "packet_size", "2048", 0);
#ifdef USE_FFMPEG
	av_dict_set(&opts, "muxrate", "10080000", 0); // from mplex project: data_rate = 1260000. mux_rate = data_rate * 8
#endif
	char tmpstr[48];
	snprintf(tmpstr, sizeof(tmpstr), "%i", (int)(0.5 * AV_TIME_BASE));
	av_dict_set(&opts, "preload", tmpstr, 0);
	m_outputCtx->packet_size = 2048;
	m_outputCtx->max_delay = (int) (0.7 * AV_TIME_BASE);
	int ret = avformat_write_header(m_outputCtx, &opts);
    if (ret < 0) {
    	wxLogError(wxT("Error occurred when opening output file"));
        return false;
    } 
	av_dict_free(&opts);
	return true;
}

AVFrame* allocPicture(AVPixelFormat pix_fmt, int width, int height) {
	AVFrame* frame = av_frame_alloc();
	if (!frame)
		return NULL;
	frame->width = width;
	frame->height = height;
	frame->format = pix_fmt;
	if (av_frame_get_buffer(frame, 32) < 0) {
		av_free(frame);
		return NULL;
	}
	return frame;
}

bool wxFfmpegMediaEncoder::addVideoStream(int codecId, VideoFormat videoFormat, AspectRatio aspectRatio,
		int videoBitrate, bool cbr) {
	if (codecId == AV_CODEC_ID_NONE) {
		m_videoStm = NULL;
		return true;
	}
	m_videoStm = avformat_new_stream(m_outputCtx, NULL);
	if (!m_videoStm) {
		wxLogError(wxT("Could not alloc stream"));
		return false;
	}
	m_videoStm->id = 0;

	// find the video encoder and open it
	auto* encoder = avcodec_find_encoder((AVCodecID) codecId);
	if (!encoder) {
		wxLogError(wxT("Video codec not found"));
		return false;
	}

	m_videoCodec = avcodec_alloc_context3(encoder);

	AVCodecContext* c = m_videoCodec;
	c->thread_count = m_threadCount;
	c->codec_id = (AVCodecID) codecId;
	c->codec_type = AVMEDIA_TYPE_VIDEO;
	c->bit_rate = videoBitrate * 1000;
	wxSize frameSize = GetFrameSize(videoFormat);
	c->width = frameSize.GetWidth();
	c->height = frameSize.GetHeight();
	c->time_base.den = isNTSC(videoFormat) ? 30000 : 25;
	c->time_base.num = isNTSC(videoFormat) ? 1001 : 1;
	c->gop_size = m_gopSize > 0 ? m_gopSize : (isNTSC(videoFormat) ? 15 : 12);
	c->pix_fmt = AV_PIX_FMT_YUV420P;
	c->rc_buffer_size = VIDEO_BUF_SIZE;
	c->rc_max_rate = cbr ? videoBitrate * 1000 : 9000000;
	c->rc_min_rate = cbr ? videoBitrate * 1000 : 0;
	
	double ar = aspectRatio == ar16_9 ? (double) 16 / 9 : (double) 4 / 3;
	c->sample_aspect_ratio = av_d2q(ar * c->height / c->width, 255);
	m_videoStm->sample_aspect_ratio = c->sample_aspect_ratio;

	if (avcodec_open2(c, encoder, NULL) < 0) {
		wxLogError(wxT("Could not open video codec"));
		return false;
	}
	if (avcodec_parameters_from_context(m_videoStm->codecpar, c) < 0) {
		wxLogError("Failed to copy encoder parameters to output video stream");
		return false;
	}
	m_videoStm->time_base = c->time_base;

	m_videoOutbuf = (uint8_t*) av_malloc(VIDEO_BUF_SIZE);

	AVCPBProperties *props;
#if LIBAVCODEC_VERSION_MAJOR >= 61
	AVPacketSideData* sd = av_packet_side_data_new(&m_videoStm->codecpar->coded_side_data,
			&m_videoStm->codecpar->nb_coded_side_data, AV_PKT_DATA_CPB_PROPERTIES, sizeof(*props), 0);
	props = sd ? (AVCPBProperties*) sd->data : NULL;
#else
	props = (AVCPBProperties*) av_stream_new_side_data(m_videoStm, AV_PKT_DATA_CPB_PROPERTIES, sizeof(*props));
#endif
	if (props == NULL) {
		wxLogError(wxT("Could not set video buffer size"));
		return false;
	}
	props->buffer_size = VIDEO_BUF_SIZE;
	props->max_bitrate = 0;
	props->min_bitrate = 0;
	props->avg_bitrate = 0;
	props->vbv_delay = UINT64_MAX;

	// allocate the encoded raw picture
	m_picture = allocPicture(c->pix_fmt, c->width, c->height);
	if (!m_picture) {
		wxLogError(wxT("Could not allocate frame"));
		return false;
	}

	m_imgConvertCtx = sws_getContext(c->width, c->height, AV_PIX_FMT_RGB24, c->width, c->height, c->pix_fmt, SWS_BICUBIC,
			NULL, NULL, NULL);
	if (!m_imgConvertCtx) {
		wxLogError(wxT("Cannot initialize the conversion context"));
		return false;
	}

	return true;
}

bool hasSampleFmt(AVCodec* codec, AVSampleFormat sample_fmt) {
	if (codec != NULL && codec->sample_fmts != NULL) {
		int fIdx = 0;
		while (codec->sample_fmts[fIdx] >= 0) {
			if (codec->sample_fmts[fIdx] == sample_fmt)
				return true;
			fIdx++;
		}
	}
	return false;
}

bool wxFfmpegMediaEncoder::addAudioStream(int codecId) {
	if (codecId == AV_CODEC_ID_NONE) {
		m_audioStm = NULL;
		return true;
	}
	if (m_outputCtx != NULL) {
		m_audioStm = avformat_new_stream(m_outputCtx, NULL);
		if (!m_audioStm) {
			wxLogError(wxT("Could not alloc stream"));
			return false;
		}
		m_audioStm->id = 1;
	}

	// find the audio encoder and open it
	const AVCodec* encoder = NULL;
	AVSampleFormat sampleFmt = AV_SAMPLE_FMT_S16;
	if ((AVCodecID) codecId == AV_CODEC_ID_AC3) {
		sampleFmt = AV_SAMPLE_FMT_FLTP;
		encoder = avcodec_find_encoder((AVCodecID) codecId );
	} else {
		sampleFmt = AV_SAMPLE_FMT_S16;
 		encoder = avcodec_find_encoder((AVCodecID) codecId );
	}

	if (!encoder) {
		wxLogError(wxT("Audio codec not found"));
		return false;
	}

	m_audioCodec = avcodec_alloc_context3(encoder);

	AVCodecContext* c = m_audioCodec;
	c->thread_count = m_threadCount;
	c->bit_rate = 64000;
	c->sample_rate = 48000;
	c->sample_fmt = sampleFmt;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 37, 100)
	AVChannelLayout chLayoutStereo;
	av_channel_layout_default(&chLayoutStereo, 2);
	if (av_channel_layout_copy(&c->ch_layout, &chLayoutStereo)) {
		wxLogError("Failed to set 2 channels");
		return false;
	}
#else
	c->channel_layout = AV_CH_LAYOUT_STEREO;
	c->channels = 2;
#endif

	if (m_audioStm && avcodec_parameters_from_context(m_audioStm->codecpar, c) < 0) {
		wxLogError("Failed to copy encoder parameters to output audio stream");
		return false;
	}

	
	if (avcodec_open2(c, encoder, NULL) < 0) {
		wxLogError("Could not open audio codec");
		return false;
	}
	
	m_audioFrame = av_frame_alloc();
	if (!m_audioFrame) {
		wxLogError("Could not allocate audio frame");
		return false;
	}

	m_audioFrame->nb_samples = c->frame_size;
	m_audioFrame->format = c->sample_fmt;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 37, 100)
	if (av_channel_layout_copy(&m_audioFrame->ch_layout, &c->ch_layout) < 0) {
		wxLogError("Could not open copy channel layout");
		return false;
	}
#else
	m_audioFrame->channel_layout = c->channel_layout;
#endif
		
	int ret = av_frame_get_buffer(m_audioFrame, 0); // allocate the data buffers
	if (ret < 0) {
		wxLogError("Could not allocate audio data buffers");
		return false;
	}
	ret = av_frame_make_writable(m_audioFrame);
	if (ret < 0)
		return false;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 37, 100)
	for (int i = 0; i < c->ch_layout.nb_channels; i++) {
#else
	for (int i = 0; i < c->channels; i++) {
#endif
		uint16_t *samples = (uint16_t*)m_audioFrame->data[i];
		if (samples) {
		memset(samples, 0, c->frame_size * av_get_bytes_per_sample(c->sample_fmt));
		}
	}

	return true;
}

void wxFfmpegMediaEncoder::CloseAudioEncoder() {
	av_frame_free(&m_audioFrame);
	avcodec_free_context(&m_audioCodec);
	m_audioStm = NULL;
}

void wxFfmpegMediaEncoder::CloseVideoEncoder() {
	if (!m_videoStm)
		return;
	if (m_imgConvertCtx)
		sws_freeContext(m_imgConvertCtx);
	if (m_picture) {
		av_freep(&m_picture->data[0]);
		av_freep(&m_picture);
	}
	av_freep(&m_videoOutbuf);
	avcodec_free_context(&m_videoCodec);
	m_videoStm = NULL;
}

bool wxFfmpegMediaEncoder::EncodeImage(wxImage image, int frames, AbstractProgressDialog* progressDialog) {
	if (!image.Ok())
		return false;
	AVCodecContext *c = m_videoCodec;
	if (image.GetWidth() != c->width || image.GetHeight() != c->height) {
		wxLogError(wxT("Image size (%dx%d) doesn't match frame size (%dx%d)."),
				image.GetWidth(), image.GetHeight(), c->width, c->height);
		return false;
	}
	// convert wxImage to the codec pixel format
	uint8_t *rgbSrc[3] = { image.GetData(), NULL, NULL };
	int rgbStride[3] = { 3 * image.GetWidth(), 0, 0 };
	sws_scale(m_imgConvertCtx, rgbSrc, rgbStride, 0, c->height, m_picture->data, m_picture->linesize);
	// encode audio and video
	double duration = ((double) m_nextVideoPts + frames) * m_videoCodec->time_base.num / m_videoCodec->time_base.den;
	while (true) {
		double audioPts = m_audioStm ? ((double) m_nextAudioPts) * m_audioCodec->time_base.num
				/ m_audioCodec->time_base.den : 0.0;
		double videoPts = ((double) m_nextVideoPts) * m_videoCodec->time_base.num / m_videoCodec->time_base.den;
		if (progressDialog->WasCanceled())
			return false;

		if ((!m_audioStm || audioPts >= duration) && (!m_videoStm || videoPts >= duration))
			break;
		
		// write interleaved audio and video frames
		if (m_audioStm && audioPts < videoPts) {
			if (!writeAudioFrame())
				return false;
		} else {
			if (!writeVideoFrame())
				return false;
			if (frames == 1)
				return true;
		}
	}
	return true;
}

int encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output) {
    int ret;

    /* send the frame for encoding */
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        wxLogError("Error sending the frame to the encoder");
        return ret;
    }

    /* read all the available output packets (in general there may be any
     * number of them */
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 0;
        else if (ret < 0) {
        	wxLogError("Error encoding audio frame");
            return ret;
        }

        fwrite(pkt->data, 1, pkt->size, output);
        av_packet_unref(pkt);
    }
    
    return 0;
}

bool wxFfmpegMediaEncoder::EncodeAudio(double duration, AbstractProgressDialog* progressDialog) {
	if (m_outputCtx != NULL) {
		// encode audio stream
		while (true) {
			double audioPts = m_audioCodec ? ((double) m_nextAudioPts) * m_audioCodec->time_base.num
					/ m_audioCodec->time_base.den : 0.0;
			if (progressDialog->WasCanceled())
				return false;
	
			if (!m_audioCodec || audioPts >= duration)
				break;
			
			// write interleaved audio and video frames
			if (!writeAudioFrame())
				return false;
		}
	} else {
		// encode audio file
		AVCodecContext* c = m_audioCodec;
		
		AVPacket* pkt = av_packet_alloc();
		if (!pkt) {
			wxLogError("could not allocate the packet");
			return false;
		}
		
		int64_t pts = 0;
		while (true) {
			double audioPts = ((double) pts) * c->time_base.num /c->time_base.den;
			if (progressDialog->WasCanceled())
				return false;
			if (audioPts >= duration)
				break;
			
			pts += m_audioFrame->nb_samples;
			if (encode(c, m_audioFrame, pkt, m_audioFile) < 0)
				return false;
		}

		// flush the encoder
		encode(c, NULL, pkt, m_audioFile);
		
		av_packet_free(&pkt);
	}
	return true;
}

// return 0 on success, negative on error
typedef int (*process_packet_cb)(void *ctx, AVPacket *pkt);

int encode(AVCodecContext *avctx, AVPacket *pkt, AVFrame *frame, int *got_packet) {
	int ret;

	*got_packet = 0;

	ret = avcodec_send_frame(avctx, frame);
	if (ret < 0)
		return ret;

	ret = avcodec_receive_packet(avctx, pkt);
	if (!ret)
		*got_packet = 1;
	if (ret == AVERROR(EAGAIN))
		return 0;

	return ret;
}

bool wxFfmpegMediaEncoder::writeAudioFrame() {
	AVPacket* pkt = av_packet_alloc();
	AVCodecContext *c = m_audioCodec;
	
	m_audioFrame->pts = m_nextAudioPts;
	m_nextAudioPts += m_audioFrame->nb_samples;

	int got_packet = 0;
	encode(c, pkt, m_audioFrame, &got_packet);
	if (!got_packet) {
		av_packet_unref(pkt);
		return true;
	}

	pkt->stream_index = m_audioStm->index;
	
	// write the compressed frame in the media file
	int ret = av_interleaved_write_frame(m_outputCtx, pkt);
	if (ret < 0) {
		av_packet_unref(pkt);
		print_error("Error while writing audio frame", ret);
		return false;
	}
	av_packet_unref(pkt);
	return true;
}

bool wxFfmpegMediaEncoder::writeVideoFrame() {
	AVCodecContext *c = m_videoCodec;
	
	// encode the image
	m_picture->pts = m_nextVideoPts++;
	AVPacket* pkt = av_packet_alloc();
	pkt->data = m_videoOutbuf;
	pkt->size = VIDEO_BUF_SIZE;
	
	int got_packet = 0;
	int ret = encode(c, pkt, m_picture, &got_packet);
	if (ret < 0) {
		av_packet_unref(pkt);
		print_error("Error while writing video frame", ret);
		return false;
	}
	if (got_packet) {
		if (pkt->pts != (int64_t) AV_NOPTS_VALUE)
			pkt->pts = av_rescale_q(pkt->pts, c->time_base, m_videoStm->time_base);
		if (pkt->dts != (int64_t) AV_NOPTS_VALUE)
			pkt->dts = av_rescale_q(pkt->dts, c->time_base, m_videoStm->time_base);
		pkt->stream_index = m_videoStm->index;
		
		// write the compressed frame in the media file
		ret = av_interleaved_write_frame(m_outputCtx, pkt);
		if (ret < 0) {
			av_packet_unref(pkt);
			print_error("Error while writing video frame", ret);
			return false;
		}
	}
	av_packet_unref(pkt);
	return true;
}

void wxFfmpegMediaEncoder::EndEncode() {
	if (m_outputCtx) {
		// write the trailer
		if (m_outputCtx->nb_streams)
			av_write_trailer(m_outputCtx);
		
		CloseEncoder();
	} else if (m_audioFile != NULL) {
	    fclose(m_audioFile);
	    m_audioFile = NULL;
	    CloseAudioEncoder();
	}
}

void wxFfmpegMediaEncoder::CloseEncoder() {
	if (!m_outputCtx)
		return;

	CloseVideoEncoder();
	CloseAudioEncoder();
	
	// free the streams
	for (unsigned int i = 0; i < m_outputCtx->nb_streams; i++) {
		av_freep(&m_outputCtx->streams[i]);
	}

	// close the output file
	if (m_outputCtx->pb != NULL)
		avio_close(m_outputCtx->pb);
	
	// free the stream
	av_freep(&m_outputCtx);
}
