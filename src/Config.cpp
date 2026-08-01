/////////////////////////////////////////////////////////////////////////////
// Name:        Config.h
// Purpose:     Configuration
// Author:      Alex Thuering
// Created:     27.03.2003
// RCS-ID:      $Id: Config.cpp,v 1.8 2015/02/09 18:33:59 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "Config.h"
#include <wxVillaLib/utils.h>
#include <wx/fileconf.h>
#include <wx/stdpaths.h>
#include <wx/dir.h>

Config s_config;

void Config::InitSections() {
	App.SetCfg(cfg);
	Video.SetCfg(cfg);
	Audio.SetCfg(cfg);
	Subtitles.SetCfg(cfg);
	Menu.SetCfg(cfg);
	Disc.SetCfg(cfg);
	Encode.SetCfg(cfg);
	Output.SetCfg(cfg);
}

void Config::Init() {
#ifdef __WXMSW__
	// check if INI file exists
	wxString fileName = wxGetAppPath() + wxT("..") + wxFILE_SEP_PATH + wxT("DVDStyler.ini");
	if (wxFileExists(fileName)) {
		if (fileName.Lower().StartsWith(wxT("c:\\program files"))) {
			wxConfig::Set(new wxFileConfig(wxT(""), wxT(""),
				wxGetHomeDir() + wxFILE_SEP_PATH + wxT("DVDStyler.ini"), fileName));
		} else
			wxConfig::Set(new wxFileConfig(wxT(""), wxT(""), fileName));
	}
	cfg = wxConfig::Get();
#elif defined(__WXMAC__)
	cfg = wxConfig::Get();
#else
	// check if .dvdstyler exist and move it
	wxString dataDir = wxStandardPaths::Get().GetUserLocalDataDir();
	if (wxFileExists(dataDir)) {
		wxRenameFile(dataDir, dataDir + ".tmp");
		wxMkdir(dataDir);
		wxRenameFile(dataDir + ".tmp", dataDir + wxFILE_SEP_PATH + "dvdstyler");
	}
	if (!wxDir::Exists(dataDir)) {
		wxMkdir(dataDir);
	}

	cfg = new wxFileConfig("", "", dataDir + wxFILE_SEP_PATH + "dvdstyler");
	wxConfig::Set(cfg);
#endif
	InitSections();

	// Keep user content in sensible default folders (VSO-style): projects and
	// build output go to Documents/DVDStyler, ISO images to ~/Videos.
	wxString docsDir = DefaultDocumentsDir();
	wxFileName::Mkdir(docsDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxString isoDir = wxGetHomeDir() + wxFILE_SEP_PATH + wxT("Videos");
	wxFileName::Mkdir(isoDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	if (!cfg->Read(_T("Misc/DocsDirMigrated"), false)) {
		cfg->Write(_T("Generate/OutputDir"), docsDir);
		cfg->Write(_T("Iso/SaveTo"), isoDir + wxFILE_SEP_PATH);
		cfg->Write(_T("Interface/LastProjectDir"), docsDir);
		cfg->Write(_T("Misc/DocsDirMigrated"), true);
	}
	// point the burn temp dir at Documents as well (drop any stored value that
	// still references the old ~/.dvdstyler location)
	wxString legacyTmpDir = wxGetHomeDir() + wxFILE_SEP_PATH + wxT(".dvdstyler");
	if (cfg->Read(_T("Generate/TempDir"), legacyTmpDir) == legacyTmpDir)
		cfg->DeleteEntry(_T("Generate/TempDir"));
}

// ---------------------------------------------------------------------------
// DiscSettings: HD authoring mode and bitrates with validation + dependencies
// ---------------------------------------------------------------------------

/** Validates a value against a [min, max] range and stores it, following the
  * same convention as the config macros: a value equal to the default deletes
  * the stored entry. Out-of-range values are clamped before storing. */
static void SetClampedInt(wxConfigBase* cfg, const wxChar* cfgName, int value, int min, int max, int defValue) {
	int clamped = value < min ? min : (value > max ? max : value);
	wxLogNull log;
	if (clamped == defValue)
		cfg->DeleteEntry(cfgName);
	else
		cfg->Write(cfgName, (long) clamped);
}

int DiscSettings::GetMode(bool def) {
	return def ? HdProfile::MODE_NONE : (int) cfg->Read(_T("Generate/BlurayMode"), (long) HdProfile::MODE_NONE);
}

void DiscSettings::SetMode(int mode) {
	if (mode < HdProfile::MODE_NONE || mode > HdProfile::MODE_AVCHD)
		mode = HdProfile::MODE_NONE;
	{
		wxLogNull log;
		if (mode == HdProfile::MODE_NONE)
			cfg->DeleteEntry(_T("Generate/BlurayMode"));
		else
			cfg->Write(_T("Generate/BlurayMode"), (long) mode);
	}
	// dependency handling: the active mode constrains the allowed bitrates
	SetBlurayVideoBitrate(GetBlurayVideoBitrate());
	SetAvchdVideoBitrate(GetAvchdVideoBitrate());
}

int DiscSettings::GetBlurayVideoBitrate(bool def) {
	return def ? DEF_BLURAY_VIDEO_BITRATE : (int) cfg->Read(_T("Generate/BlurayVideoBitrate"),
			(long) DEF_BLURAY_VIDEO_BITRATE);
}

void DiscSettings::SetBlurayVideoBitrate(int kbps) {
	SetClampedInt(cfg, _T("Generate/BlurayVideoBitrate"), kbps, 1000,
			HdProfile::MaxVideoBitrateKbps(HdProfile::MODE_BLURAY), DEF_BLURAY_VIDEO_BITRATE);
}

int DiscSettings::GetAvchdVideoBitrate(bool def) {
	return def ? DEF_AVCHD_VIDEO_BITRATE : (int) cfg->Read(_T("Generate/AvchdVideoBitrate"),
			(long) DEF_AVCHD_VIDEO_BITRATE);
}

void DiscSettings::SetAvchdVideoBitrate(int kbps) {
	SetClampedInt(cfg, _T("Generate/AvchdVideoBitrate"), kbps, 1000,
			HdProfile::MaxVideoBitrateKbps(HdProfile::MODE_AVCHD), DEF_AVCHD_VIDEO_BITRATE);
}

int DiscSettings::GetAudioBitrate(bool def) {
	return def ? DEF_BLURAY_AUDIO_BITRATE : (int) cfg->Read(_T("Generate/BlurayAudioBitrate"),
			(long) DEF_BLURAY_AUDIO_BITRATE);
}

void DiscSettings::SetAudioBitrate(int kbps) {
	SetClampedInt(cfg, _T("Generate/BlurayAudioBitrate"), kbps, 128, 640, DEF_BLURAY_AUDIO_BITRATE);
}

int DiscSettings::GetMediaSize(bool def) {
	HdProfile::Mode mode = (HdProfile::Mode) GetMode();
	HdProfile::MediaSize stored = def ? HdProfile::DefaultMediaSize(mode) :
			(HdProfile::MediaSize) (int) cfg->Read(_T("Generate/MediaSize"),
					(long) HdProfile::DefaultMediaSize(mode));
	// a size left over from the other mode must not leak into this one
	if (!HdProfile::MediaSizeValidForMode(stored, mode))
		return HdProfile::DefaultMediaSize(mode);
	return stored;
}

void DiscSettings::SetMediaSize(int size) {
	HdProfile::Mode mode = (HdProfile::Mode) GetMode();
	if (!HdProfile::MediaSizeValidForMode((HdProfile::MediaSize) size, mode))
		size = HdProfile::DefaultMediaSize(mode);
	wxLogNull log;
	if (size == HdProfile::DefaultMediaSize(mode))
		cfg->DeleteEntry(_T("Generate/MediaSize"));
	else
		cfg->Write(_T("Generate/MediaSize"), (long) size);
}

bool Config::IsMainWinMaximized() {
	bool ret = false;
	cfg->Read(wxT("MainWin/maximized"), &ret);
	return ret;
}

wxRect Config::GetMainWinLocation() {
	wxRect rect;
	rect.x = cfg->Read(wxT("MainWin/x"), -1);
	rect.y = cfg->Read(wxT("MainWin/y"), -1);
	rect.width = cfg->Read(wxT("MainWin/width"), -1);
	rect.height = cfg->Read(wxT("MainWin/height"), -1);
	return rect;
}

void Config::SetMainWinLocation(wxRect rect, bool isMaximized) {
	cfg->Write(wxT("MainWin/maximized"), isMaximized);
	if (rect.width > 50 && rect.height > 50) {
		cfg->Write(wxT("MainWin/x"), rect.x);
		cfg->Write(wxT("MainWin/y"), rect.y);
		cfg->Write(wxT("MainWin/width"), rect.width);
		cfg->Write(wxT("MainWin/height"), rect.height);
	}
}
