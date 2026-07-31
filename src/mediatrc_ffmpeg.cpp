/////////////////////////////////////////////////////////////////////////////
// Name:        mediatrc_ffmpeg.cpp
// Purpose:     FFMPEG Media Transcoder
// Author:      Alex Thuering
// Created:     26.04.2008
// RCS-ID:      $Id: mediatrc_ffmpeg.cpp,v 1.80 2017/11/25 14:40:13 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "mediatrc_ffmpeg.h"
#include "Utils.h"
#include "Config.h"
#include <wxVillaLib/utils.h>
#include <wx/wx.h>

wxFfmpegMediaTranscoder::wxFfmpegMediaTranscoder() {
	Init();
}

wxFfmpegMediaTranscoder::~wxFfmpegMediaTranscoder() {
	// nothing to do
}

void wxFfmpegMediaTranscoder::Init() {
	m_cmd = wxT("");
	m_inputFileCount = 0;
	if (s_config.GetThreadCount() > 1)
		AddOption(wxT("threads"), wxString::Format(wxT("%d"), s_config.GetThreadCount()));
}

/** Returns transcoding command */
wxString wxFfmpegMediaTranscoder::GetCmd(bool forceInternalEncoder) const {
	wxString cmd = s_config.GetAVConvCmd();
	if (s_config.GetEncoder().length() && !forceInternalEncoder) {
		cmd = s_config.GetEncoder();
#ifdef __WXMSW__
		if (wxFileExists(wxGetAppPath() + cmd + wxT(".bat")))
			cmd += wxT(".bat");
#endif
		if (s_config.GetEncoderMode().length() > 1) {
			cmd += wxT(" -mode " + s_config.GetEncoderMode());
		}
	}
	if (s_config.GetFfmpegOptions().length() > 0)
		cmd += " " + s_config.GetFfmpegOptions();
	return cmd + m_cmd;
}

void wxFfmpegMediaTranscoder::AddOption(const wxString& name, const wxString& value) {
	m_cmd += wxString::Format(wxT(" -%s %s"), name.c_str(), value.c_str());
}

void wxFfmpegMediaTranscoder::AddAudioOption(const wxString& name, int streamIdx, const wxString& value) {
	m_cmd += wxString::Format(wxT(" -%s:a:%d %s"), name.c_str(), streamIdx, value.c_str());
}

void wxFfmpegMediaTranscoder::AddVideoOption(const wxString& name, int streamIdx, const wxString& value) {
	m_cmd += wxString::Format(wxT(" -%s:v:%d %s"), name.c_str(), streamIdx, value.c_str());
}

void wxFfmpegMediaTranscoder::AddSubtitleOption(const wxString& name, int streamIdx, const wxString& value) {
	m_cmd += wxString::Format(wxT(" -%s:s:%d %s"), name.c_str(), streamIdx, value.c_str());
}

bool wxFfmpegMediaTranscoder::AddInputFile(const wxString& fileName, const wxString& format, long tsOffset) {
	if (tsOffset > 0)
		AddOption("itsoffset", Time2String(tsOffset));
	if (fileName == "/dev/zero") {
		m_cmd += " -ac 2 -ar 48000 -f s16le -i /dev/zero";
	} else {
#ifndef __WXMSW__
		// use VA-API hardware decoding for the first (video) input file
		if (m_inputFileCount == 0 && s_config.GetUseVAAPI()) {
			AddOption(wxT("hwaccel"), wxT("vaapi"));
			AddOption(wxT("hwaccel_output_format"), wxT("yuv420p"));
		}
#endif
		if (format.length()) {
			if ((int) format.Index(':') > 0) {
				AddOption(wxT("f"), format.BeforeFirst(':'));
				AddOption(wxT("ac"), format.AfterFirst(':'));
			} else
				AddOption(wxT("f"), format);
		}
		m_cmd += wxString::Format(wxT(" -i \"%s\""), fileName.c_str());
	}
	
	m_inputFileCount++;
	return true;
}

bool wxFfmpegMediaTranscoder::SetOutputFile(const wxString& fileName, VideoFormat videoFormat, bool ntscFilm,
		AspectRatio aspectRatio, AudioFormat audioFormat, SubtitleFormat subtitleFormat, int videoBitrate,
		bool vbr, int audioBitrate, int mapFileIdx, int mapStreamIdx, StreamType mapStreamType) {
	wxArrayInt audioFormats;
	if (audioFormat != afNONE)
		audioFormats.Add(audioFormat);
	wxArrayInt subtitleFormats;
	if (subtitleFormat != sfNONE)
		subtitleFormats.Add(subtitleFormat);
	return SetOutputFile(fileName, videoFormat, ntscFilm, aspectRatio, audioFormats, subtitleFormats, videoBitrate,
			vbr, audioBitrate, mapFileIdx, mapStreamIdx, mapStreamType);
}

bool wxFfmpegMediaTranscoder::SetOutputFile(const wxString& fileName, VideoFormat videoFormat, bool ntscFilm,
		AspectRatio aspectRatio, wxArrayInt audioFormats, wxArrayInt subtitleFormats, int videoBitrate, bool vbr,
		int audioBitrate, int mapFileIdx, int mapStreamIdx, StreamType mapStreamType) {
	if (m_outputFormat.length() > 0) {
		AddOption(wxT("f"), m_outputFormat);
	} else if (audioFormats.size() == 0) {
		AddOption(wxT("f"), wxT("mpeg2video"));
	} else if (videoFormat == vfNONE) {
		AddOption(wxT("f"), audioFormats.size() && audioFormats[0] == afMP2 ? wxT("mp2")
				: audioFormats[0] == afAC3 ? wxT("ac3") : wxT("s16be"));
	} else {
		AddOption(wxT("f"), wxT("dvd"));
		m_cmd = wxT(" -fflags +genpts") + m_cmd;
	}
	
	if (videoFormat == vfCOPY) {
		AddVideoOption(wxT("c"), 0, wxT("copy"));
		AddVideoOption(wxT("bufsize"), 0, wxT("1835008")); // 224*1024*8;
	} else if (videoFormat != vfNONE) {
		AddVideoOption(wxT("c"), 0, wxT("mpeg2video"));
		if (aspectRatio != arAUTO) {
			AddOption(wxT("aspect"), aspectRatio == ar16_9 ? wxT("16:9") : wxT("4:3"));
		}
		
		wxSize frameSize = GetFrameSize(videoFormat);
		AddOption(wxT("s"), wxString::Format(wxT("%dx%d"), frameSize.x, frameSize.y));
		
		if (!isNTSC(videoFormat)) {
			AddOption(wxT("r"), wxT("25"));
			AddOption(wxT("g"), wxT("15"));
		} else {
			AddOption(wxT("r"), ntscFilm ? wxT("24000/1001") : wxT("30000/1001"));
			AddOption(wxT("g"), wxT("18"));
		}
		AddOption(wxT("pix_fmt"), wxT("yuv420p")); 

		AddVideoOption(wxT("b"), 0, wxString::Format(wxT("%d"), videoBitrate * 1000));
		AddVideoOption(wxT("maxrate"), 0, wxString::Format(wxT("%d"), vbr ? 9000000 : videoBitrate * 1000));
		AddVideoOption(wxT("minrate"), 0, wxString::Format(wxT("%d"), vbr ? 0 : videoBitrate * 1000));
		AddVideoOption(wxT("bufsize"), 0, wxT("1835008")); // 224*1024*8;

		AddOption(wxT("packetsize"), wxT("2048"));  // from www.mpucoder.com: DVD sectors contain 2048 bytes of data, this is also the size of one pack.
#ifdef USE_FFMPEG
		AddOption(wxT("muxrate"), wxT("10080000")); // from mplex project: data_rate = 1260000. mux_rate = data_rate * 50
#endif
		// force key frames at chapter marks
		if (m_chapterList.length())
			AddOption(wxT("force_key_frames"), m_chapterList);
		
		if (m_videoFilters.length())
			AddOption(wxT("vf"), m_videoFilters);
	}
	
	if (audioFormats.size() > 0) {
		AddOption(wxT("b:a"), wxString::Format(wxT("%d"), audioBitrate * 1000));
		AddOption(wxT("ar"), wxT("48000")); // audio sample rate
	}
	
	int outputStreamIdx = 0;
	for (unsigned int i = 0; i < audioFormats.size(); i++) {
		if (audioFormats[i] != afNONE) {
			AddAudioOption(wxT("c"), outputStreamIdx, audioFormats[i] == afCOPY ? wxT("copy") : audioFormats[i] == afMP2
						? wxT("mp2") : audioFormats[i] == afAC3 ? wxT("ac3") : wxT("pcm_s16be"));
			if (m_audioFilters.find(i) != m_audioFilters.end() && m_audioFilters[i].length() > 0) {
				AddAudioOption(wxT("filter"), outputStreamIdx, wxT('"') + m_audioFilters[i] + wxT('"'));
			}
			outputStreamIdx++;
		}
	}
	outputStreamIdx = 0;
	for (unsigned int i = 0; i < subtitleFormats.size(); i++) {
		if (subtitleFormats[i] != sfNONE) {
			AddSubtitleOption(wxT("c"), outputStreamIdx, wxT("copy"));
			outputStreamIdx++;
		}
	}
	
	if (mapFileIdx >= 0) {
		if (mapStreamIdx >= 0) {
			if (mapStreamType != stUNKNOWN) {
				wxChar stTypeChar[] = { wxT('v'), wxT('a'), wxT('s') };
				AddOption(wxT("map"), wxString::Format(wxT("%d:%c:%d"), mapFileIdx,
						stTypeChar[mapStreamType - 1], mapStreamIdx));
			} else {
				AddOption(wxT("map"), wxString::Format(wxT("%d:%d"), mapFileIdx, mapStreamIdx));
			}
		} else {
			AddOption(wxT("map"), wxString::Format(wxT("%d"), mapFileIdx));
		}
	} else {
		// map all input files into one
		for (unsigned int i = 0; i < m_inputFileCount; i++) {
			if (videoFormat != vfNONE && i == 0) // only first file can contain video stream
				AddOption(wxT("map"), wxString::Format(wxT("%d:V"), i));
			if (i > 0 || m_inputFileCount < 1 + audioFormats.size()) // first file can contain no audio streams
				AddOption(wxT("map"), wxString::Format(wxT("%d:a"), i));
		}
		// disable streams with format 'none'
		if (audioFormats.size() > 0) {
			for (unsigned int i = 0; i < audioFormats.size(); i++) {
				if (audioFormats[i] == afNONE)
					AddOption(wxT("map"), wxString::Format(wxT("-0:a:%d"), i));
			}
		} else {
			AddOption(wxT("map"), wxT("-0:a"));
		}
		if (subtitleFormats.size() > 0) {
			for (unsigned int i = 0; i < subtitleFormats.size(); i++) {
				if (subtitleFormats[i] == sfNONE)
					AddOption(wxT("map"), wxString::Format(wxT("-0:s:%d"), i));
				else if (subtitleFormats[i] == sfCOPY)
					AddOption(wxT("map"), wxString::Format(wxT("0:s:%d"), i));
			}
		} else {
			AddOption(wxT("map"), wxT("-0:s"));
		}
	}
	
	m_cmd += wxString::Format(wxT(" \"%s\""), fileName.c_str());
	return true;
}

/** Sets interlaced encoding flag */
void wxFfmpegMediaTranscoder::SetInterlaced(bool value) {
	if (value) {
		AddVideoOption(wxT("flags"), 0, wxT("+ilme+ildct"));
		AddVideoOption(wxT("alternate_scan"), 0, wxT("1"));
	}
}

/** Sets field first flag (Auto, TFF, BFF) */
void wxFfmpegMediaTranscoder::SetFirstField(FirstField firstField) {
	if (firstField != ffAUTO)
		AddVideoOption(wxT("top"), 0, wxString::Format(wxT("%d"), firstField));
}

/** Sets start time */
void wxFfmpegMediaTranscoder::SetStartTime(double startTime) {
	if (startTime != 0)
		AddOption(wxT("ss"), wxString::Format(wxT("%f"), startTime));
}

/** Sets recording time */
void wxFfmpegMediaTranscoder::SetRecordingTime(double recordingTime) {
	if (recordingTime != 0)
		AddOption(wxT("t"), wxString::Format(wxT("%f"), recordingTime));
}

/** Set filter to concat video segments */
void wxFfmpegMediaTranscoder::ConcatVideo(const wxString& resultFile, int videoBitrate, bool copyAudio,
		bool copySubtitle) {
	// -filter_complex "[0:v] [1:v] concat=n=2:v=1:a=0 [v]"
	AddOption(wxT("filter_complex"), wxT("\"[0:v] [1:v] concat=n=2 [v]\""));
	AddVideoOption(wxT("b"), 0, wxString::Format(wxT("%d"), videoBitrate * 1000));
	AddVideoOption(wxT("maxrate"), 0, wxString::Format(wxT("%d"), videoBitrate * 1000));
	AddVideoOption(wxT("minrate"), 0, wxString::Format(wxT("%d"), videoBitrate * 1000));
	AddVideoOption(wxT("bufsize"), 0, wxT("1835008")); // 224*1024*8;
	// -c:a copy -c:s copy
	if (copyAudio) {
		AddOption(wxT("c:a"), wxT("copy"));
	}
	if (copySubtitle) {
		AddOption(wxT("c:s"), wxT("copy"));
	}
	// -map "[v]" -map "2:a"
	AddOption(wxT("map"), wxT("[v]"));
	if (copyAudio) {
		AddOption(wxT("map"), wxT("2:a"));
	}
	if (copySubtitle) {
		AddOption(wxT("map"), wxT("2:s"));
	}
	// -f dvd result.mpg
	AddOption(wxT("f"), wxT("dvd"));
	m_cmd += wxString::Format(wxT(" \"%s\""), resultFile.c_str());
}

/** Set filter to concat audio segments */
void wxFfmpegMediaTranscoder::ConcatAudio(const wxString& resultFile, int segments) {
	//ffmpeg -i 1.mp2 -i 1.mp2 -filter_complex "[0:0] [1:0] concat=n=2:v=0:a=1 [a]" -map "[a]" output.mp2
	wxString filter;
	for (int i = 0; i < segments; i++) {
		filter += wxString::Format(wxT("[%d:0] "), i);
	}
	filter += wxString::Format(wxT("concat=n=%d:v=0:a=1 [a]"), segments);
	AddOption(wxT("filter_complex"), wxT('"') + filter + wxT('"'));
	AddOption(wxT("map"), wxT("[a]"));
	m_cmd += wxString::Format(wxT(" \"%s\""), resultFile.c_str());
}

/** Set filter and output to calculate replay gain values */
void wxFfmpegMediaTranscoder::ReplayGain(int audioStreamIdx) {
	AddOption(wxT("f"), wxT("null"));
	AddOption(wxT("map"), wxString::Format(wxT("0:a:%d"), audioStreamIdx));
	AddOption(wxT("map"), wxT("-0:v"));
	AddAudioOption(wxT("filter"), 0, wxT("\"replaygain\""));
#ifdef __WXMSW__
	m_cmd += wxT(" NUL");
#else
	m_cmd += wxT(" /dev/null");
#endif
}

/** Set filter and output to calculate volume peak value */
void wxFfmpegMediaTranscoder::VolumeDetect(int audioStreamIdx) {
	AddOption(wxT("f"), wxT("null"));
	AddOption(wxT("map"), wxString::Format(wxT("0:a:%d"), audioStreamIdx));
	AddOption(wxT("map"), wxT("-0:v"));
	AddAudioOption(wxT("filter"), 0, wxT("\"volumedetect\""));
#ifdef __WXMSW__
	m_cmd += wxT(" NUL");
#else
	m_cmd += wxT(" /dev/null");
#endif
}
