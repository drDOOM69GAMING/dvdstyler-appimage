/////////////////////////////////////////////////////////////////////////////
// Name:        Config.h
// Purpose:     Configuration
// Author:      Alex Thuering
// Created:     27.03.2003
// RCS-ID:      $Id: Config.h,v 1.110 2017/11/25 14:39:34 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef DS_CONFIG_H
#define DS_CONFIG_H

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/thread.h>
#include <wx/filename.h>
#include "Utils.h"

const wxString DEF_VERSION		= wxT("");
const int DEF_LANGUAGE			= wxLANGUAGE_UNKNOWN;
const wxString DEF_LANGUAGE_CODE= wxT("");
const wxString DEF_DISC_LABEL	= wxT("DVD");
const int DEF_DVD_RESOLUTION	= 0;
const int DEF_DISC_CAPACITY		= 3;
const int DEF_POST_COMMAND		= 0;
const int DEF_VIDEO_FORMAT		= 2;
const int DEF_AUDIO_FORMAT		= 3;
const wxString DEF_AUDIO_LANG	= wxT("");
const wxString DEF_SUBTITLE_LANG= wxT("");
const int DEF_ASPECT_RATIO		= 1;
const int DEF_CHAPTER_LENGTH	= 10;
const wxString DEF_BUTTON		= wxT("frame-v2.xml");
const wxString DEF_TEXT_BUTTON	= wxT("text-v3.xml");
const int DEF_SLIDE_DURATION	= 5;
const wxString DEF_TRANSITION	= wxT("fade.xml");
const double DEF_TRANSITION_DURATION = 1.0;
const wxString DEF_FILE_BROWSER_DIR = wxT("");
#define DEF_FILE_BROWSER_LAST_DIR wxGetHomeDir()
#define DEF_LAST_PROJECT_DIR wxGetHomeDir()
#define DEF_LAST_ADD_DIR wxGetHomeDir()
const int DEF_UNDO_HISTORY_SIZE = 100;
const int DEF_VIDEO_BITRATE = -1;
const int DEF_AUDIO_BITRATE = 192;
const int DEF_MENU_FRAME_COUNT = 12;
const int DEF_MENU_VIDEO_BITRATE = 8000;
const int DEF_SLIDESHOW_VIDEO_BITRATE = 8000;
const int DEF_THREAD_COUNT = wxThread::GetCPUCount() > 0 ? wxThread::GetCPUCount() : 1;
#ifdef __WXMSW__
	const bool DEF_USE_VAAPI = false;
#else
	#define DEF_USE_VAAPI (wxFileExists(wxT("/dev/dri/renderD128")))
#endif
const int DEF_DVD_RESERVED_SPACE = 0; // KB
#ifdef __WXMSW__
	#define DEF_GEN_TEMP_DIR wxFileName::GetTempDir()
#else
	#define DEF_GEN_TEMP_DIR (wxGetHomeDir() + wxFILE_SEP_PATH + wxT(".dvdstyler"))
#endif
#define DEF_GEN_OUTPUT_DIR wxGetHomeDir()
#ifdef __WXMSW__
	const bool DEF_PREVIEW_DO        = false;
	const wxString DEF_PREVIEW_CMD   = _T("wmplayer");
	const wxString DEF_BURN_DEVICE   = _T("");
	const wxString DEF_ADD_ECC_CMD   = _T("\"C:\\Program Files\\dvdisaster\\dvdisaster\" -mRS02 -n dvd -c -i \"$FILE\"");
#elif defined(__WXMAC__)
	const bool DEF_PREVIEW_DO        = true;
	const wxString DEF_PREVIEW_CMD   = _T("/Applications/VLC.app/Contents/MacOS/VLC");
	const wxString DEF_BURN_DEVICE   = _T("/dev/dvd");
	const wxString DEF_ADD_ECC_CMD   = _T("dvdisaster -mRS02 -n dvd -c -i \"$FILE\"");
#else
	const bool DEF_PREVIEW_DO        = true;
	const wxString DEF_PREVIEW_CMD   = _T("xine \"dvd:/$DIR\"");
	const wxString DEF_BURN_DEVICE   = _T("/dev/dvd");
	const wxString DEF_ADD_ECC_CMD   = _T("dvdisaster -mRS02 -n dvd -c -i \"$FILE\"");
#endif
const bool DEF_USE_MPLEX = false;
const bool DEF_USE_MPLEX_FOR_MENUS = true;
#ifdef USE_FFMPEG
const wxString DEF_AVCONV_CMD    = _T("ffmpeg");
const wxString DEF_PLAY_CMD    = _T("ffplay");
#else
const wxString DEF_AVCONV_CMD    = _T("avconv");
const wxString DEF_PLAY_CMD    = _T("avplay");
#endif
#ifdef __WXMSW__
	const wxString DEF_ENCODER   = _T("ffmpeg-vbr");
#else
	const wxString DEF_ENCODER   = _T("");
#endif
const wxString DEF_MPLEX_CMD     = _T("mplex -f 8 -S 0 -M -V $PARAMS -o \"$FILE_OUT\" \"$FILE_VIDEO\" \"$FILE_AUDIO\"");
const wxString DEF_SPUMUX_CMD    = _T("spumux -P -s $STREAM \"$FILE_CONF\"");
const wxString DEF_DVDAUTHOR_CMD = _T("dvdauthor -o \"$DIR\" -x \"$FILE_CONF\"");
const wxString DEF_ISO_CMD       = _T("mkisofs -V \"$VOL_ID\" -o \"$FILE\" -dvd-video \"$DIR\"");
const wxString DEF_ISO_SIZE_CMD  = _T("mkisofs -quiet -print-size \"$DIR\"");
const wxString DEF_BURN_SCAN_CMD = _T("dvd+rw-mediainfo $DEVICE");
const wxString DEF_BURN_CMD      = _T("growisofs -V \"$VOL_ID\" -dvd-compat -Z $DEV -dvd-video \"$DIR\" $SPEEDSTR");
#ifdef __WXMAC__
const wxString DEF_BURN_ISO_CMD  = _T("/usr/bin/hdiutil burn -device \"$DEV\" $SPEEDSTR \"$FILE\"");
const wxString DEF_FORMAT_CMD    = _T("/usr/bin/hdiutil burn -device \"$DEV\" -erase");
const wxString DEF_BURN_SPEED_OPT= _T("-speed $SPEED");
#else
const wxString DEF_BURN_ISO_CMD  = _T("growisofs -dvd-compat -Z $DEV=\"$FILE\" -use-the-force-luke=dao:$SIZE $SPEEDSTR");
const wxString DEF_FORMAT_CMD    = _T("dvd+rw-format -force $DEV");
const wxString DEF_BURN_SPEED_OPT= _T("-speed=$SPEED");
#endif
const bool DEF_GENERATE_DO       = false;
const bool DEF_ISO_DO            = false;
const bool DEF_BURN_DO           = true;
const bool DEF_FORMAT_DO         = false;
#define DEF_ISO_SAVETO (wxGetHomeDir() + wxFILE_SEP_PATH)
const int DEF_BURN_SPEED         = 0;
const bool DEF_ADD_ECC_DO        = false;

#define CONFIG_PROP(name, cfgName, defValue)\
  CONFIG_PROP_T(name, cfgName, defValue, wxString, wxString)

#define CONFIG_PROP_BOOL(name, cfgName, defValue)\
  CONFIG_PROP_T(name, cfgName, defValue, bool, long)

#define CONFIG_PROP_INT(name, cfgName, defValue)\
  CONFIG_PROP_T(name, cfgName, defValue, int, long)

//#define CONFIG_PROP_DOUBLE(name, cfgName, defValue)
//  CONFIG_PROP_T(name, cfgName, defValue, double, double)

#define CONFIG_PROP_T(name, cfgName, defValue, cfgType, cfgType2)\
  cfgType Get##name(bool def = false)\
  { return def ? (cfgType2) defValue : cfg->Read(cfgName, (cfgType2) defValue); }\
  void Set##name(cfgType value)\
  { wxLogNull log; if (value == defValue) cfg->DeleteEntry(cfgName); else cfg->Write(cfgName, value); }

#define CONFIG_PROP_DOUBLE(name, cfgName, defValue)\
  double Get##name(bool def = false)\
  { if (def) return defValue; wxString val = cfg->Read(cfgName, wxString::Format(wxT("%f"), defValue));\
  	  double dval; if (val.ToDouble(&dval)) return dval; return defValue; }\
  void Set##name(double value)\
  { wxLogNull log; if (value == defValue) cfg->DeleteEntry(cfgName); \
	else cfg->Write(cfgName, wxString::Format(wxT("%f"), value)); }

#define CONFIG_PROP_COLOUR(name, cfgName, defValue)\
  wxColour Get##name(bool def = false)\
  { if (def) return defValue; wxString val = cfg->Read(cfgName, Colour2String(defValue)); return String2Colour(val); }\
  void Set##name(const wxColour& value)\
  { wxLogNull log; if (value == defValue) cfg->DeleteEntry(cfgName); else cfg->Write(cfgName, Colour2String(value)); }

class Config {
public:
    void Init();
    bool Flush() { return cfg->Flush(); }
    bool DeleteAll() { return cfg->DeleteAll(); }
    bool DeleteGroup(wxString group) { return cfg->DeleteGroup(group); }
	
    bool IsMainWinMaximized();
    wxRect GetMainWinLocation();
    void SetMainWinLocation(wxRect rect, bool isMaximized);
    
    wxConfigBase* GetConfigBase() { return cfg; }
	
	CONFIG_PROP(Version,  _T("Version"), DEF_VERSION)
	CONFIG_PROP(LanguageCode, _T("Interface/LanguageCode"), DEF_LANGUAGE_CODE)
    CONFIG_PROP(DefDiscLabel, _T("Interface/DefDiscLabel"), DEF_DISC_LABEL)
	CONFIG_PROP_INT(DefDvdResolution, _T("Interface/DefDvdResolution"), DEF_DVD_RESOLUTION)
    CONFIG_PROP_INT(DefDiscCapacity, _T("Interface/DefDiscCapacity"), DEF_DISC_CAPACITY)
    CONFIG_PROP_INT(DefPostCommand, _T("Interface/DefPostCommand"), DEF_POST_COMMAND)
	
	CONFIG_PROP_INT(DefVideoFormat, _T("Interface/DefVideoFormat"), DEF_VIDEO_FORMAT)
	CONFIG_PROP_INT(DefAudioFormat, _T("Interface/DefAudioFormat"), DEF_AUDIO_FORMAT)
	CONFIG_PROP(DefAudioLanguage, _T("Interface/DefAudioLanguage"), DEF_AUDIO_LANG)
	CONFIG_PROP(DefSubtitleLanguage, _T("Interface/DefSubtitleLanguage"), DEF_SUBTITLE_LANG)
	CONFIG_PROP_INT(DefAspectRatio, _T("Interface/DefAspectRatio"), DEF_ASPECT_RATIO)
	CONFIG_PROP_BOOL(DefKeepAspectRatio, _T("Interface/DefKeepAspectRatio"), true)
	CONFIG_PROP_BOOL(DefRencodeNtscFilm, _T("Interface/DefRencodeNtscFilm"), false)
	CONFIG_PROP_BOOL(DefAudio51, _T("Interface/DefAudio51"), false)
	CONFIG_PROP_BOOL(DefAudioNormalize, _T("Interface/DefAudioNormalize"), false)
	CONFIG_PROP_INT(DefChapterLength, _T("Interface/DefChapterLength"), DEF_CHAPTER_LENGTH)
	CONFIG_PROP_BOOL(AddChapterAtTitleEnd, _T("Interface/AddChapterAtTitleEnd"), false)
	CONFIG_PROP(DefButton, _T("Interface/DefButton"), DEF_BUTTON)
	CONFIG_PROP(DefTextButton, _T("Interface/DefTextButton"), DEF_TEXT_BUTTON)
	CONFIG_PROP_BOOL(DefPlayAllTitlesets, _T("Interface/DefPlayAllTitlesets"), false)
	CONFIG_PROP_INT(DefSlideDuration, _T("Interface/DefSlideDuration"), DEF_SLIDE_DURATION)
	CONFIG_PROP(DefTransition, _T("Interface/DefTransition"), DEF_TRANSITION)
	CONFIG_PROP_DOUBLE(DefTransitionDuration, _T("Interface/DefTransitionDuration"), DEF_TRANSITION_DURATION)
	CONFIG_PROP_BOOL(AcceptInvalidActions, _T("Interface/AcceptInvalidActions"), false)
	CONFIG_PROP(FileBrowserDir, _T("Interface/FileBrowserDir"), DEF_FILE_BROWSER_DIR)
	CONFIG_PROP(FileBrowserLastDir, _T("Interface/FileBrowserLastDir"), DEF_FILE_BROWSER_LAST_DIR)
	CONFIG_PROP(LastProjectDir, _T("Interface/LastProjectDir"), DEF_LAST_PROJECT_DIR)
	CONFIG_PROP(LastAddDir, _T("Interface/LastAddDir"), DEF_LAST_ADD_DIR)
	CONFIG_PROP_INT(UndoHistorySize, _T("Interface/UndoHistorySize"), DEF_UNDO_HISTORY_SIZE)
	CONFIG_PROP_BOOL(ShowWelcomeDlg, _T("Interface/ShowWelcomeDlg"), true)
	CONFIG_PROP_BOOL(TitleDeletePrompt, _T("Interface/TitleDeletePrompt"), true)
	CONFIG_PROP_INT(CacheClearPrompt, _T("Interface/CacheClearPrompt"), 0)
	CONFIG_PROP_BOOL(OverwriteOutputDirPrompt, _T("Interface/OverwriteOutputDirPrompt"), true)
	CONFIG_PROP_BOOL(ClearThumbnailCache, _T("Interface/ClearThumbnailCache"), false)
	
	CONFIG_PROP(SubtitlesCharacterSet, _T("Subtitles/CharacterSet"), wxT("UTF-8"))
	CONFIG_PROP(SubtitlesFontFamily, _T("Subtitles/FontFamily"), wxT("Arial"))
	CONFIG_PROP(SubtitlesFontStyle, _T("Subtitles/FontStyle"), wxT("Regular"))
	CONFIG_PROP_DOUBLE(SubtitlesFontSize, _T("Subtitles/FontSize"), 28.0)
#if !wxCHECK_VERSION(2,9,0)
	CONFIG_PROP_COLOUR(SubtitlesFillColour, _T("Subtitles/FillColour"), wxColour(255, 255, 0))
#else
	CONFIG_PROP_COLOUR(SubtitlesFillColour, _T("Subtitles/FillColour"), *wxYELLOW)
#endif
	CONFIG_PROP_COLOUR(SubtitlesOutlineColour, _T("Subtitles/OutlineColour"), *wxBLACK)
	CONFIG_PROP_DOUBLE(SubtitlesOutlineThickness, _T("Subtitles/OutlineThickness"), 3.0)
	CONFIG_PROP_COLOUR(SubtitlesShadowColour, _T("Subtitles/ShadowColour"), *wxBLACK)
	CONFIG_PROP_INT(SubtitlesShadowOffsetX, _T("Subtitles/ShadowOffsetX"), 0)
	CONFIG_PROP_INT(SubtitlesShadowOffsetY, _T("Subtitles/ShadowOffsetY"), 0)
	CONFIG_PROP_INT(SubtitlesAlignment, _T("Subtitles/Alignment"), (wxALIGN_CENTER_HORIZONTAL | wxALIGN_BOTTOM))
	CONFIG_PROP_INT(SubtitlesLeftMargin, _T("Subtitles/LeftMargin"), 60)
	CONFIG_PROP_INT(SubtitlesRightMargin, _T("Subtitles/RightMargin"), 60)
	CONFIG_PROP_INT(SubtitlesTopMargin, _T("Subtitles/TopMargin"), 20)
	CONFIG_PROP_INT(SubtitlesBottomMargin, _T("Subtitles/BottomMargin"), 30)
	
	CONFIG_PROP(TempDir, _T("Generate/TempDir"), DEF_GEN_TEMP_DIR)
	CONFIG_PROP(OutputDir, _T("Generate/OutputDir"), DEF_GEN_OUTPUT_DIR)
	CONFIG_PROP_BOOL(RemoveTempFiles, _T("Generate/RemoveTempFiles"), true)
	CONFIG_PROP_INT(Vbr, _T("Generate/Vbr"), false)
	CONFIG_PROP(FfmpegOptions, _T("Generate/FfmpegOptions"), "")
	CONFIG_PROP_INT(VideoBitrate, _T("Generate/VideoBitrate"), DEF_VIDEO_BITRATE)
	CONFIG_PROP_INT(AudioBitrate, _T("Generate/AudioBitrate"), DEF_AUDIO_BITRATE)
	CONFIG_PROP_INT(MenuFrameCount,_T("Generate/MenuFrameCount"), DEF_MENU_FRAME_COUNT)
	CONFIG_PROP_INT(MenuVideoBitrate, _T("Generate/MenuVideoBitrate"), DEF_MENU_VIDEO_BITRATE)
	CONFIG_PROP_BOOL(MenuVideoCBR, _T("Generate/MenuVideoCBR"), false)
	CONFIG_PROP_BOOL(AllowHdMenues, _T("Generate/AllowHdMenues"), false)
	CONFIG_PROP_BOOL(AllowHdTitles, _T("Generate/AllowHdTitles"), false)
	CONFIG_PROP_BOOL(DrawButtonsOnBackground, _T("Generate/DrawButtonsOnBackground"), true)
	CONFIG_PROP_BOOL(ButtonsOffset2px, _T("Generate/ButtonsOffset2px"), false)
    CONFIG_PROP_INT(SlideshowVideoBitrate, _T("Generate/SlideshowVideoBitrate"), DEF_SLIDESHOW_VIDEO_BITRATE)
	CONFIG_PROP_BOOL(SlideshowVideoCBR, _T("Generate/SlideshowVideoCBR"), false)
    CONFIG_PROP_INT(ThreadCount, _T("Generate/ThreadCount"), DEF_THREAD_COUNT)
	CONFIG_PROP_BOOL(UseVAAPI, _T("Generate/UseVAAPI"), DEF_USE_VAAPI)
    CONFIG_PROP_INT(DvdReservedSpace, _T("Generate/DvdReservedSpace"), DEF_DVD_RESERVED_SPACE)
    CONFIG_PROP_BOOL(UseMplex, _T("Generate/UseMplex"), DEF_USE_MPLEX)
    CONFIG_PROP_BOOL(UseMplexForMenus, _T("Generate/UseMplexForMenus"), DEF_USE_MPLEX_FOR_MENUS)
    CONFIG_PROP(PlayCmd, _T("Generate/PlayCmd"), DEF_PLAY_CMD)
    CONFIG_PROP(AVConvCmd, _T("Generate/AVConvCmd"), DEF_AVCONV_CMD)
	CONFIG_PROP(Encoder, _T("Generate/Encoder"), DEF_ENCODER)
    CONFIG_PROP(EncoderMode, _T("Generate/EncoderMode"), wxT(""))
    CONFIG_PROP(MplexCmd, _T("Generate/MplexCmd"), DEF_MPLEX_CMD)
	CONFIG_PROP(SpumuxCmd, _T("Generate/SpumuxCmd"), DEF_SPUMUX_CMD)
	CONFIG_PROP(DvdauthorCmd, _T("Generate/DvdauthorCmd"), DEF_DVDAUTHOR_CMD)
	
	CONFIG_PROP_BOOL(PreviewDo, _T("Preview/Do"), DEF_PREVIEW_DO)
	CONFIG_PROP(PreviewCmd, _T("Preview/Cmd"), DEF_PREVIEW_CMD)
	
	CONFIG_PROP_BOOL(GenerateDo, _T("Generate/Do"), DEF_GENERATE_DO)
	
	CONFIG_PROP_BOOL(IsoDo, _T("Iso/Do"), DEF_ISO_DO)
	CONFIG_PROP(IsoCmd, _T("Iso/Cmd"), DEF_ISO_CMD)
	CONFIG_PROP(IsoSizeCmd, _T("Iso/SizeCmd"), DEF_ISO_SIZE_CMD)
	CONFIG_PROP(IsoSaveTo,  _T("Iso/SaveTo"), DEF_ISO_SAVETO)
	
	CONFIG_PROP_BOOL(BurnDo, _T("Burn/Do"), DEF_BURN_DO)
	CONFIG_PROP(BurnCmd, _T("Burn/Cmd"), DEF_BURN_CMD)
	CONFIG_PROP(BurnISOCmd, _T("Burn/ISOCmd"), DEF_BURN_ISO_CMD)
	CONFIG_PROP(BurnDevice, _T("Burn/Device"), DEF_BURN_DEVICE)
	CONFIG_PROP(BurnScanCmd, _T("Burn/ScanSmd"), DEF_BURN_SCAN_CMD)
	CONFIG_PROP(BurnSpeedOpt, _T("Burn/SpeedOpt"), DEF_BURN_SPEED_OPT)
	CONFIG_PROP_INT(BurnSpeed, _T("Burn/Speed"), DEF_BURN_SPEED)
	
	CONFIG_PROP_BOOL(FormatDo, _T("Burn/FormatDo"), DEF_FORMAT_DO)
	CONFIG_PROP(FormatCmd, _T("Burn/FormatCmd"), DEF_FORMAT_CMD)
	
	CONFIG_PROP_BOOL(AddECCDo, _T("AddECC/Do"), DEF_ADD_ECC_DO)
	CONFIG_PROP(AddECCCmd, _T("AddECC/Cmd"), DEF_ADD_ECC_CMD)
	
  protected:
    wxConfigBase* cfg;
};

extern Config s_config;

#endif //DS_CONFIG_H
