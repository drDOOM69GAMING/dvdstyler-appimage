/////////////////////////////////////////////////////////////////////////////
// Name:        ProcessBluray.cpp
// Purpose:     Blu-ray (HD) authoring process
// Author:      drDOOM69GAMING
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "ProcessBluray.h"
#include "ProcessExecute.h"
#include "ProcessIsoImage.h"
#include "DVD.h"
#include "Titleset.h"
#include "BurnDlg.h"
#include "Config.h"
#include <wxVillaLib/utils.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/arrstr.h>
#include <wx/dynarray.h>
#include <wx/textfile.h>
#include <math.h>

/** Constructor */
ProcessBluray::ProcessBluray(ProgressDlg* progressDlg, DVD* dvd, BurnDlg* burnDlg, const wxString& dvdOutDir,
		const wxString& tmpDir): Process(progressDlg) {
	this->dvd = dvd;
	this->burnDlg = burnDlg;
	this->dvdOutDir = dvdOutDir;
	this->tmpDir = tmpDir;
}

/** Returns true, if process need be executed */
bool ProcessBluray::IsNeedExecute() {
	return s_config.Disc.GetMode() != BD_MODE_NONE;
}

/** Returns the Blu-ray compliant progressive frame rate for the given source fps */
double ProcessBluray::GetBdFps(double sourceFps) {
	if (sourceFps <= 0)
		return 24000.0 / 1001.0;
	// progressive 1080p on Blu-ray only allows up to 29.97 fps;
	// 50/60 fps sources are temporally downsampled to 25/29.97 fps
	if (sourceFps > 45.0)
		return sourceFps > 55.0 ? 30000.0 / 1001.0 : 25.0;
	const double bdRates[4] = { 24000.0 / 1001.0, 24.0, 25.0, 30000.0 / 1001.0 };
	double best = bdRates[0];
	double bestDiff = fabs(sourceFps - best);
	for (int i = 1; i < 4; i++) {
		double diff = fabs(sourceFps - bdRates[i]);
		if (diff < bestDiff) {
			best = bdRates[i];
			bestDiff = diff;
		}
	}
	return best;
}

/** Returns the exact ffmpeg frame rate string for the given fps */
wxString ProcessBluray::GetFpsStr(double fps) {
	const double rates[5] = { 24000.0 / 1001.0, 30000.0 / 1001.0, 60000.0 / 1001.0, 25.0, 50.0 };
	const wxChar* names[5] = { _T("24000/1001"), _T("30000/1001"), _T("60000/1001"), _T("25"), _T("50") };
	for (int i = 0; i < 5; i++)
		if (fabs(fps - rates[i]) < 0.001)
			return names[i];
	return wxString::Format(_T("%g"), fps);
}

/** Returns the GOP size (in frames) for the given fps */
int ProcessBluray::GetGopSize(double fps, int seconds) {
	// Blu-ray requires a maximum GOP of 2 seconds, AVCHD of 1 second
	int gop = (int) lround(fps * seconds);
	return gop < 24 ? 24 : gop;
}

/** Encodes one title to H.264 elementary stream and AC-3 audio */
bool ProcessBluray::EncodeTitle(Vob* vob, int titleIdx, const wxString& workDir, int videoBitrate, int audioBitrate,
		wxString& videoFile, wxString& audioFile, double& fps, wxString& chapterList) {
	if (progressDlg->WasCanceled())
		return false;
	progressDlg->AddDetailMsg(wxString::Format(_("Transcode video %d to Blu-ray H.264"), titleIdx + 1));

	wxString base = workDir + wxString::Format(wxT("bd%02d"), titleIdx);
	videoFile = base + wxT(".264");
	audioFile = base + wxT(".ac3");

	// frame rate
	bool avchd = s_config.Disc.GetMode() == BD_MODE_AVCHD;
	double srcFps = vob->GetVideoStream() ? vob->GetVideoStream()->GetSourceFps() : 0;
	fps = GetBdFps(srcFps);
	// AVCHD: keep 50/60 fps sources smooth as 720p50/60 (AVCHD 2.0)
	if (avchd && srcFps > 45.0)
		fps = srcFps > 55.0 ? 60000.0 / 1001.0 : 50.0;
	// settings model -> pipeline planner: resolve the coherent encoder profile
	HdProfile::Params profile = HdProfile::Resolve(avchd ? HdProfile::MODE_AVCHD : HdProfile::MODE_BLURAY,
			(HdProfile::Quality) s_config.Video.GetHdQuality(), videoBitrate);
	int gop = GetGopSize(fps, profile.gopSeconds);

	// build ffmpeg command
	wxString cmd = s_config.GetAVConvCmd();
	cmd += wxT(" -y");
	// VA-API: decode on the GPU always when enabled; encode on the GPU too when
	// the user opted in (h264_vaapi is faster but not strictly Blu-ray compliant)
	bool useVaapiEncode = s_config.GetUseVAAPI() && s_config.GetUseVAAPIEncode();
	if (useVaapiEncode) {
		cmd += wxT(" -init_hw_device vaapi=va:/dev/dri/renderD128 -hwaccel vaapi -hwaccel_device va -hwaccel_output_format vaapi");
	} else if (s_config.GetUseVAAPI()) {
		cmd += wxT(" -hwaccel vaapi -hwaccel_output_format yuv420p");
	}
	if (vob->GetStartTime() != 0)
		cmd += wxString::Format(wxT(" -ss %g"), vob->GetStartTime());
	cmd += wxT(" -i \"") + vob->GetFilename() + wxT("\"");

	// AVCHD is Blu-ray constrained to DVD media (High@4.0, max GOP 1 s,
	// max 18 Mbit/s). High-fps sources are encoded as 720p50/60.
	int width = 1920;
	int height = 1080;
	if (avchd && fps > 45.0) {
		width = 1280;
		height = 720;
	}

	// video: scale to the target resolution keeping aspect ratio, pad to fill
	// the frame. HD content must be BT.709; upscaled SD content is converted
	// from BT.601 so colors stay correct on standalone players.
	wxString vf;
	if (useVaapiEncode) {
		vf = wxString::Format(
				wxT("scale_vaapi=%d:%d:force_original_aspect_ratio=decrease,pad_vaapi=%d:%d:(ow-iw)/2:(oh-ih)/2"), width, height, width, height);
	} else {
		vf = wxString::Format(
				wxT("scale=%d:%d:force_original_aspect_ratio=decrease,pad=%d:%d:(ow-iw)/2:(oh-ih)/2"), width, height, width, height);
		Stream* videoStream = vob->GetVideoStream();
		if (videoStream && videoStream->GetSourceVideoSize().GetHeight() > 0
				&& videoStream->GetSourceVideoSize().GetHeight() <= 576)
			vf = wxT("colormatrix=bt601:bt709,") + vf;
		vf += wxT(",format=yuv420p");
	}
	cmd += wxT(" -vf \"") + vf + wxT("\"");
	cmd += wxT(" -r ") + GetFpsStr(fps);
	cmd += wxT(" -map 0:v:0");
	if (useVaapiEncode) {
		// GPU encode: no bluray_compat/slices support in h264_vaapi, so these
		// Blu-ray compliance options are intentionally omitted.
		cmd += wxT(" -c:v h264_vaapi -profile:v high -level ") + profile.GetLevelStr();
		cmd += wxT(" -colorspace bt709 -color_primaries bt709 -color_trc bt709");
		cmd += wxString::Format(wxT(" -b:v %dk -maxrate %dk -bufsize %dk"), videoBitrate, profile.maxrateKbps, profile.bufsizeKbps);
		cmd += wxString::Format(wxT(" -g %d -bf %d -refs %d"), gop, profile.bframes, profile.refs);
	} else {
		cmd += wxT(" -c:v libx264 -profile:v high -level ") + profile.GetLevelStr() + wxT(" -preset medium -pix_fmt yuv420p");
		cmd += wxT(" -colorspace bt709 -color_primaries bt709 -color_trc bt709");
		cmd += wxString::Format(wxT(" -b:v %dk -maxrate %dk -bufsize %dk"), videoBitrate, profile.maxrateKbps, profile.bufsizeKbps);
		cmd += wxString::Format(wxT(" -g %d -keyint_min %d -sc_threshold 0 -bf %d -refs %d"), gop, gop / 2, profile.bframes, profile.refs);
		cmd += wxString::Format(wxT(" -x264opts bluray_compat=%d:slices=%d"), profile.blurayCompat ? 1 : 0, profile.slices);
	}
	// chapters force key frames (offset to the trimmed timeline)
	chapterList.Clear();
	for (unsigned int i = 0; i < vob->GetCells().size(); i++) {
		double t = (double) vob->GetCells()[i]->GetStart() / 1000.0 - vob->GetStartTime();
		if (t <= 0.1)
			continue;
		if (chapterList.length())
			chapterList += wxT(",");
		chapterList += wxString::Format(wxT("%g"), t);
	}
	if (chapterList.length())
		cmd += wxT(" -force_key_frames ") + chapterList;
	if (vob->GetRecordingTime() > 0)
		cmd += wxString::Format(wxT(" -t %g"), vob->GetRecordingTime());
	cmd += wxT(" -an \"") + videoFile + wxT("\"");

	// audio: first audio track converted to AC-3 48 kHz
	if (vob->GetAudioStreamCount() > 0) {
		int channels = 2;
		Stream* stream = NULL;
		for (unsigned int i = 0; i < vob->GetStreams().size(); i++) {
			if (vob->GetStreams()[i]->GetType() == stAUDIO) {
				stream = vob->GetStreams()[i];
				break;
			}
		}
		if (stream && stream->GetSourceChannelNumber() >= 6)
			channels = 6;
		cmd += wxT(" -map 0:a:0 -c:a ac3");
		cmd += wxString::Format(wxT(" -b:a %dk -ar 48000 -ac %d"), audioBitrate, channels);
		// timestamp repair: keep A/V in sync on drifting sources
		cmd += wxT(" -af aresample=async=1000:first_pts=0");
		if (vob->GetRecordingTime() > 0)
			cmd += wxString::Format(wxT(" -t %g"), vob->GetRecordingTime());
		cmd += wxT(" -vn \"") + audioFile + wxT("\"");
	}

	// run ffmpeg (single pass, video + audio elementary streams)
	AVConvExecute exec(progressDlg, lround(vob->GetDuration() * fps));
	if (!exec.Execute(cmd)) {
		if (wxFileExists(videoFile))
			wxRemoveFile(videoFile);
		if (wxFileExists(audioFile))
			wxRemoveFile(audioFile);
		progressDlg->Failed(_("Error transcoding of ") + vob->GetFilename());
		return false;
	}
	return true;
}

/** Generates a tsMuxeR meta file for one title */
bool ProcessBluray::SaveTsMuxeRMeta(const wxString& metaFile, const wxString& videoFile, const wxString& audioFile,
		double fps, const wxString& chapterList) {
	wxTextFile file;
	if (!file.Create(metaFile))
		return false;
	wxString muxOpt = _T("MUXOPT --no-pcr-on-video-pid --new-audio-pes --blu-ray --vbr");
	if (chapterList.length()) {
		wxString chapters = chapterList;
		wxArrayString parts = wxSplit(chapters, wxT(','));
		wxString chapterStr;
		for (unsigned int i = 0; i < parts.Count(); i++) {
			double t = 0;
			parts[i].ToDouble(&t);
			if (chapterStr.length())
				chapterStr += wxT(",");
			chapterStr += FormatTime(t);
		}
		muxOpt += _T(" --chapter=\"") + chapterStr + _T("\"");
	}
	file.AddLine(muxOpt);
	file.AddLine(wxString::Format(_T("V_MPEG4/ISO/AVC, \"%s\", fps=%g, insertSEI, contSPS"), videoFile.c_str(), fps));
	if (audioFile.length())
		file.AddLine(wxString::Format(_T("A_AC3, \"%s\""), audioFile.c_str()));
	file.Write();
	return true;
}

/** Formats a time in seconds as HH:MM:SS.mmm */
wxString ProcessBluray::FormatTime(double seconds) {
	int msec = lround(seconds * 1000);
	int ms = msec % 1000;
	int sec = (msec / 1000) % 60;
	int min = (msec / 60000) % 60;
	int hour = msec / 3600000;
	return wxString::Format(_T("%02d:%02d:%02d.%03d"), hour, min, sec, ms);
}

/** Counts the playlists (mpls) already in the BDMV structure */
int ProcessBluray::CountPlaylists(const wxString& bdmvRoot) {
	wxString dir = bdmvRoot + wxFILE_SEP_PATH + wxT("BDMV") + wxFILE_SEP_PATH + wxT("PLAYLIST");
	if (!wxDir::Exists(dir))
		return 0;
	wxDir d(dir);
	wxString file;
	int count = 0;
	for (bool hasEntry = d.GetFirst(&file, wxEmptyString, wxDIR_FILES); hasEntry; hasEntry = d.GetNext(&file)) {
		if (file.Find(wxT(".mpls")) != wxNOT_FOUND)
			count++;
	}
	return count;
}

/** Runs tsMuxeR to create the BDMV folder structure */
bool ProcessBluray::AuthorBdmv(const wxString& metaFile, const wxString& bdmvRoot) {
	if (progressDlg->WasCanceled())
		return false;
	progressDlg->AddSummaryMsg(_("Authoring Blu-ray structure (BDMV)"));
	if (!wxDir::Exists(bdmvRoot))
		wxMkdir(bdmvRoot);
	int playlistsBefore = CountPlaylists(bdmvRoot);
	wxString cmd = s_config.Disc.GetBlurayTsMuxeRCmd();
	cmd += wxT(" \"") + metaFile + wxT("\" \"") + bdmvRoot + wxT("\"");
	ProcessExecute exec(progressDlg);
	if (!exec.Execute(cmd)) {
		progressDlg->Failed(_("tsMuxeR failed to author the Blu-ray structure."));
		return false;
	}
	if (!wxDir::Exists(bdmvRoot + wxFILE_SEP_PATH + wxT("BDMV"))) {
		progressDlg->Failed(_("tsMuxeR did not create a BDMV folder."));
		return false;
	}
	if (CountPlaylists(bdmvRoot) <= playlistsBefore) {
		progressDlg->Failed(_("tsMuxeR did not add a playlist for this title."));
		return false;
	}
	return true;
}

/** Logs an estimated output size and warns if the content cannot fit the media */
void ProcessBluray::CheckCapacity(int vobCount, int videoBitrate, int audioBitrate) {
	if (vobCount <= 0)
		return;
	HdProfile::Mode mode = (HdProfile::Mode) s_config.Disc.GetMode();
	HdProfile::MediaSize mediaSize = (HdProfile::MediaSize) s_config.Disc.GetMediaSize();
	double capacityGB = HdProfile::MediaSizeGB(mediaSize);
	double totalSeconds = GetTotalSeconds();
	double estBytes = HdProfile::EstimateBytes(totalSeconds, videoBitrate, audioBitrate);
	progressDlg->AddSummaryMsg(wxString::Format(
			_("Estimated output size: %.2f GB for %.0f minutes of video."), estBytes / 1.0e9, totalSeconds / 60.0));
	if (estBytes > HdProfile::MediaSizeBytes(mediaSize))
		progressDlg->AddSummaryMsg(
				wxString::Format(_("Warning: the estimated output size exceeds the %.1f GB media capacity. "
						"Reduce the video bitrate or the duration."), capacityGB), wxEmptyString, *wxRED);
}

/** Returns the total duration of all titles in seconds. */
double ProcessBluray::GetTotalSeconds() {
	double totalSeconds = 0;
	for (int tsi = 0; tsi < (int) dvd->GetTitlesets().Count(); tsi++) {
		Titleset* ts = dvd->GetTitlesets()[tsi];
		for (int pgci = 0; pgci < (int) ts->GetTitles().Count(); pgci++) {
			Pgc* pgc = ts->GetTitles()[pgci];
			for (int vobi = 0; vobi < (int) pgc->GetVobs().Count(); vobi++)
				totalSeconds += pgc->GetVobs()[vobi]->GetDuration();
		}
	}
	return totalSeconds;
}

/** Returns the video bitrate to use. When auto-fit is enabled, computes a
 *  bitrate that fills the selected media size (minus audio and a small muxing
 *  overhead), clamped to the mode's spec bounds. Otherwise returns the
 *  configured bitrate. */
int ProcessBluray::GetVideoBitrate(bool avchd, double totalSeconds) {
	HdProfile::Mode mode = avchd ? HdProfile::MODE_AVCHD : HdProfile::MODE_BLURAY;
	if (s_config.Disc.GetHdVideoBitrateAuto()) {
		int audioBitrate = s_config.Disc.GetAudioBitrate();
		double capacity = HdProfile::MediaSizeBytes((HdProfile::MediaSize) s_config.Disc.GetMediaSize());
		// leave ~5% for filesystem/muxing overhead and the audio stream
		double videoBudget = capacity * 0.95 - audioBitrate * 1000.0 / 8.0 * totalSeconds;
		int max = HdProfile::MaxVideoBitrateKbps(mode);
		if (totalSeconds > 0 && videoBudget > 0) {
			int bitrate = (int) (videoBudget * 8.0 / 1000.0 / totalSeconds);
			if (bitrate > max)
				bitrate = max;
			if (bitrate < 1000)
				bitrate = 1000;
			return bitrate;
		}
		// no valid duration: fall back to the configured bitrate
	}
	return avchd ? s_config.Disc.GetAvchdVideoBitrate() : s_config.Disc.GetBlurayVideoBitrate();
}

/** Executes process */
bool ProcessBluray::Execute() {
	if (progressDlg->WasCanceled())
		return false;

	wxString workDir = tmpDir;
	if (workDir.Last() != wxFILE_SEP_PATH)
		workDir += wxFILE_SEP_PATH;
	workDir += wxT("bd-tmp") + wxString(wxFILE_SEP_PATH);
	if (!wxDir::Exists(workDir))
		wxMkdir(workDir);

	wxString bdmvRoot = dvdOutDir;
	if (bdmvRoot.Last() == wxFILE_SEP_PATH)
		bdmvRoot.RemoveLast();

	// count titles
	int vobCount = 0;
	for (int tsi = 0; tsi < (int) dvd->GetTitlesets().Count(); tsi++) {
		Titleset* ts = dvd->GetTitlesets()[tsi];
		for (int pgci = 0; pgci < (int) ts->GetTitles().Count(); pgci++) {
			Pgc* pgc = ts->GetTitles()[pgci];
			for (int vobi = 0; vobi < (int) pgc->GetVobs().Count(); vobi++)
				vobCount++;
		}
	}
	progressDlg->SetSubSteps(vobCount * 200);
	// resolve the bitrates once: auto-fit uses the selected media size
	bool avchd = s_config.Disc.GetMode() == BD_MODE_AVCHD;
	int videoBitrate = GetVideoBitrate(avchd, GetTotalSeconds());
	int audioBitrate = s_config.Disc.GetAudioBitrate();
	CheckCapacity(vobCount, videoBitrate, audioBitrate);
	progressDlg->ReplaceSummaryMsg(avchd
			? _("AVCHD mode (DVD media): menus and subtitles are not authored; every title becomes its own BD title.")
			: _("Blu-ray mode: menus and subtitles are not authored; every title becomes its own BD title."));

	// encode and author every title
	int titleIdx = 0;
	for (int tsi = 0; tsi < (int) dvd->GetTitlesets().Count(); tsi++) {
		Titleset* ts = dvd->GetTitlesets()[tsi];
		for (int pgci = 0; pgci < (int) ts->GetTitles().Count(); pgci++) {
			Pgc* pgc = ts->GetTitles()[pgci];
			for (int vobi = 0; vobi < (int) pgc->GetVobs().Count(); vobi++) {
				Vob* vob = pgc->GetVobs()[vobi];
				wxString msg = wxString::Format(_("Transcode video %d of %d"), titleIdx + 1, vobCount);
				if (titleIdx == 0)
					progressDlg->AddSummaryMsg(msg);
				else
					progressDlg->ReplaceSummaryMsg(msg);
				wxString videoFile;
				wxString audioFile;
				wxString chapterList;
				double titleFps = 0;
				if (!EncodeTitle(vob, titleIdx, workDir, videoBitrate, audioBitrate, videoFile, audioFile, titleFps, chapterList))
					return false;
				wxString metaFile = workDir + wxString::Format(wxT("bd%02d.meta"), titleIdx);
				if (!SaveTsMuxeRMeta(metaFile, videoFile, audioFile, titleFps, chapterList)) {
					if (s_config.GetRemoveTempFiles()) {
						DeleteFile(videoFile);
						if (audioFile.length())
							DeleteFile(audioFile);
					}
					return false;
				}
				if (!AuthorBdmv(metaFile, bdmvRoot)) {
					if (s_config.GetRemoveTempFiles()) {
						DeleteFile(videoFile);
						if (audioFile.length())
							DeleteFile(audioFile);
						DeleteFile(metaFile);
					}
					return false;
				}
				// remove temporary elementary streams
				if (s_config.GetRemoveTempFiles()) {
					DeleteFile(videoFile);
					if (audioFile.length())
						DeleteFile(audioFile);
					DeleteFile(metaFile);
				}
				titleIdx++;
			}
		}
	}

	if (titleIdx == 0) {
		progressDlg->Failed(_("There are no titles to encode for Blu-ray."));
		return false;
	}

	progressDlg->IncStep();
	return true;
}

/** Constructor */
ProcessBlurayIso::ProcessBlurayIso(ProgressDlg* progressDlg, DVD* dvd, BurnDlg* burnDlg, const wxString& dvdOutDir,
		const wxString& tmpDir): Process(progressDlg) {
	this->dvd = dvd;
	this->burnDlg = burnDlg;
	this->dvdOutDir = dvdOutDir;
	this->tmpDir = tmpDir;
}

/** Returns true, if process need be executed */
bool ProcessBlurayIso::IsNeedExecute() {
	return burnDlg->DoCreateIso() || burnDlg->DoBurn();
}

/** Builds a UDF 2.50 ISO image */
bool ProcessBlurayIso::BuildIso(const wxString& isoFile) {
	if (progressDlg->WasCanceled())
		return false;
	progressDlg->AddSummaryMsg(_("Creating Blu-ray ISO image (UDF 2.50)"));
	wxString dir = dvdOutDir;
	if (dir.Last() == wxFILE_SEP_PATH)
		dir.RemoveLast();
	wxString cmd = s_config.Disc.GetBlurayIsoCmd();
	wxString label = dvd->GetLabel();
	label.Replace(wxT("\""), wxT("\\\""));
	cmd.Replace(_T("$VOL_ID"), label);
	cmd.Replace(_T("$DIR"), dir);
	cmd.Replace(_T("$FILE"), isoFile);
	ProcessExecute exec(progressDlg);
	if (!exec.Execute(cmd)) {
		progressDlg->Failed(_("Failed to create the Blu-ray ISO image."));
		return false;
	}
	if (!wxFileExists(isoFile)) {
		progressDlg->Failed(_("The Blu-ray ISO image was not created."));
		return false;
	}
	return true;
}

/** Executes process */
bool ProcessBlurayIso::Execute() {
	if (progressDlg->WasCanceled())
		return false;
	if (burnDlg->DoCreateIso()) {
		if (!BuildIso(burnDlg->GetIsoFile()))
			return false;
	} else if (burnDlg->DoBurn()) {
		wxString isoFile = tmpDir;
		if (isoFile.Last() != wxFILE_SEP_PATH)
			isoFile += wxFILE_SEP_PATH;
		isoFile += TMP_ISO;
		if (!BuildIso(isoFile))
			return false;
	}
	return true;
}
