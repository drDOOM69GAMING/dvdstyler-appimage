/////////////////////////////////////////////////////////////////////////////
// Name:        SettingsDlg.cpp
// Purpose:     Settings dialog
// Author:      Alex Thuering
// Created:     27.03.2004
// RCS-ID:      $Id: SettingsDlg.cpp,v 1.87 2016/10/05 19:51:30 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "SettingsDlg.h"
#include "Config.h"
#include "Languages.h"
#include "DVD.h"
#include "MenuObject.h"
#include "Cache.h"
#include <wxVillaLib/utils.h>
#include <wx/notebook.h>
#include <wx/filename.h>

#define BUTTONS_DIR wxFindDataDirectory(_T("buttons"))
#define TRANSITIONS_DIR wxFindDataDirectory(wxT("transitions"))

enum {
	FILE_BROWSER_CHOICE_ID = 7900,
	RESET_DONT_SHOW_FLAGS_BT_ID,
	CLEAR_CACHE_BT,
	MODE_CHOICE_ID,
	HQ_CHECK_ID,
	XHQ_CHECK_ID
};

BEGIN_EVENT_TABLE(SettingsDlg, wxPropDlg)
	EVT_CHOICE(FILE_BROWSER_CHOICE_ID, SettingsDlg::OnChangeFileBrowserDir)
	EVT_BUTTON(RESET_DONT_SHOW_FLAGS_BT_ID, SettingsDlg::OnResetDontShowFlags)
	EVT_BUTTON(CLEAR_CACHE_BT, SettingsDlg::OnClearCache)
	EVT_CHOICE(MODE_CHOICE_ID, SettingsDlg::OnChangeEncoderMode)
	EVT_CHECKBOX(HQ_CHECK_ID, SettingsDlg::OnCheckHQ)
	EVT_CHECKBOX(XHQ_CHECK_ID, SettingsDlg::OnCheckXHQ)
END_EVENT_TABLE()

SettingsDlg::SettingsDlg(wxWindow* parent, Cache* cache): wxPropDlg(parent), m_cache(cache), m_directoryEdit(NULL),
		m_directorySelectButton(NULL), encoderCtrl(NULL), modeCtrl(NULL), hqCtrl(NULL), xhqCtrl(NULL) {
	Create(true);
	SetTitle(_("Settings"));
	SetSize(600, -1);
	Center();
}

void SettingsDlg::CreatePropPanel(wxSizer* sizer) {
	bool def = sizer == NULL;
	wxNotebook* notebook = NULL;
	if (sizer) {
		notebook = new wxNotebook(this, -1);
		sizer->Add(notebook, 0, wxEXPAND);
	}

	// ----------- Interface Tab Sizers ------------- //
	wxBoxSizer* interfaceSizer = NULL;
	wxFlexGridSizer* interfaceGrid = NULL;
	if (sizer) {
		wxPanel* interfacePanel = new wxPanel(notebook, -1);
		notebook->AddPage(interfacePanel, _("Interface"));
		propWindow = interfacePanel;

		interfaceSizer = new wxBoxSizer(wxVERTICAL);
		interfacePanel->SetAutoLayout(true);
		interfacePanel->SetSizer(interfaceSizer);

		interfaceGrid = new wxFlexGridSizer(2, 4, 16);
		interfaceGrid->AddGrowableCol(1);
		interfaceSizer->Add(interfaceGrid, 1, wxEXPAND | wxALL, 10);
	}

	// -------------- Interface Tab ----------------- //
	wxString lang = GetLangName(s_config.GetLanguageCode());
	AddBitmapComboProp(interfaceGrid, _("Language:"), lang, GetLangNames(), GetLangBitmaps(), wxCB_READONLY);
	AddTextProp(interfaceGrid, _("Default disc label:"), s_config.GetDefDiscLabel(def));
	wxArrayString labels = DVD::GetCapacityLabels();
	AddChoiceProp(interfaceGrid, _("Default disc capacity:"), labels[s_config.GetDefDiscCapacity()], labels, 0);
	wxControl* discCapacityChoice = GetLastControl();
	
	// video format
	AddText(interfaceGrid, _("Default video format:"));
	wxSizer* vfSizer = NULL;
	if (interfaceGrid) {
		vfSizer = new wxBoxSizer(wxHORIZONTAL);
		interfaceGrid->Add(vfSizer);
	}
	int vf = s_config.GetDefVideoFormat() >= 2 ? s_config.GetDefVideoFormat() - 2 : 0;
	labels = DVD::GetVideoFormatLabels();
	AddChoiceProp(vfSizer, wxT(""), labels[vf], labels);
	// fix size
	wxControl* videoChoice = GetLastControl();
	discCapacityChoice->SetMinSize(videoChoice->GetBestSize());
	AddSpacer(vfSizer, 4);
	// aspect ratio
	labels = DVD::GetAspectRatioLabels();
	AddChoiceProp(vfSizer, wxT(""), labels[s_config.GetDefAspectRatio() - 1], labels);
	wxControl* aspectRatioChoice = GetLastControl();
	AddSpacer(vfSizer, 4);
	AddCheckProp(vfSizer, _("Keep aspect ratio"), s_config.GetDefKeepAspectRatio(def));
	AddStretchSpacer(vfSizer, 1);
	
	// audio format
	AddText(interfaceGrid, _("Default audio format:"));
	wxSizer* afSizer = NULL;
	if (interfaceGrid) {
		afSizer = new wxBoxSizer(wxHORIZONTAL);
		interfaceGrid->Add(afSizer);
	}
	int af = s_config.GetDefAudioFormat() >= 2 ? s_config.GetDefAudioFormat() - 2 : 0;
	labels = DVD::GetAudioFormatLabels();
	AddChoiceProp(afSizer, wxT(""), labels[af], labels);
	// fix size
	GetLastControl()->SetMinSize(videoChoice->GetBestSize());
	AddSpacer(afSizer, 4);
	// default audio language
	labels = DVD::GetAudioLanguageCodes();
	AddChoiceProp(afSizer, wxT(""), s_config.GetDefAudioLanguage(), labels);
	GetLastControl()->SetMinSize(aspectRatioChoice->GetBestSize());
	AddSpacer(afSizer, 4);
	AddCheckProp(afSizer, wxT("5.1"), s_config.GetDefAudio51(def));
	AddSpacer(afSizer, 4);
	AddCheckProp(afSizer, _("Normalize"), s_config.GetDefAudioNormalize(def));
	AddStretchSpacer(afSizer, 1);
	
	// default chapter length
	wxSizer* chpSizer = AddSpinProp(interfaceGrid, _("Default chapter length:"), s_config.GetDefChapterLength(def),
			0, 999, 60, _("min"), false);
	AddSpacer(chpSizer, 8);
	AddCheckProp(chpSizer, _("Add chapter at title end"), s_config.GetAddChapterAtTitleEnd(def));
	
	// default title post command
	labels = DVD::GetDefPostCommandLabels();
	AddChoiceProp(interfaceGrid, _("Default title post command:"), labels[s_config.GetDefPostCommand(def)], labels, 0);
	wxControl* postCommandChoice = GetLastControl();
	
	// default button
	int sel = 0;
	wxArrayString tmpArray;
	labels.Clear();
	m_buttons.Clear();
	wxString fname = wxFindFirstFile(BUTTONS_DIR + _T("*.xml"));
	while (!fname.IsEmpty()) {
		MenuObject obj(NULL, false, fname);
		tmpArray.Add(wxGetTranslation((const wxChar*)obj.GetTitle().GetData()) + wxString(wxT("|"))
				+ wxFileName(fname).GetFullName());
		fname = wxFindNextFile();
	}
	tmpArray.Sort();
	for (int i = 0; i<(int)tmpArray.GetCount(); i++) {
		labels.Add(tmpArray[i].BeforeLast('|'));
		wxString button = tmpArray[i].AfterLast('|');
		m_buttons.Add(button);
		if (button == s_config.GetDefButton(def))
			sel = m_buttons.GetCount() - 1;
	}
	wxSizer* dbtSizer = AddChoiceProp(interfaceGrid, _("Default button:"), labels[sel], labels, 0, false);
	wxControl* defButtonChoice = GetLastControl();
	if (postCommandChoice->GetBestSize().GetWidth() < defButtonChoice->GetBestSize().GetWidth())
		postCommandChoice->SetMinSize(defButtonChoice->GetBestSize());
	AddSpacer(dbtSizer, 4);
	AddCheckProp(dbtSizer, _("Accept invalid actions"), s_config.GetAcceptInvalidActions(def));
	
	// Slideshow
	AddSpinProp(interfaceGrid, _("Default slide duration:"), s_config.GetDefSlideDuration(def), 0, 999, 60, _("sec"));
	AddSpinDoubleProp(interfaceGrid, _("Default transition duration:"), s_config.GetDefTransitionDuration(def),
			0, 99, 60, _("sec"));
	Slideshow::GetTransitions(m_transitions, labels);
	sel = 0;
	for (unsigned int i = 0; i <= m_transitions.GetCount() - 1; i++)
		if (m_transitions[i] == s_config.GetDefTransition(def))
			sel = i;
	AddChoiceProp(interfaceGrid, _("Default slide transition:"), labels[sel], labels, 0, false);
	if (GetLastControl()->GetBestSize().GetWidth() < defButtonChoice->GetBestSize().GetWidth())
		GetLastControl()->SetMinSize(defButtonChoice->GetBestSize());

	// File Browser start directory
	wxString fbDir = s_config.GetFileBrowserDir();
	const wxString fbChoices[] = { _("Last directory"),_("Custom") };
	wxSizer* fbSizer = AddChoiceProp(interfaceGrid, _("File Browser start directory:"),
		fbChoices[fbDir.IsEmpty() ? 0 : 1], wxArrayString(2, fbChoices), 0, false, FILE_BROWSER_CHOICE_ID);
	if (GetLastControl()->GetBestSize().GetWidth() < defButtonChoice->GetBestSize().GetWidth())
		GetLastControl()->SetMinSize(defButtonChoice->GetBestSize());
	AddSpacer(fbSizer, 4);
	AddDirectoryProp(fbSizer, wxT(""), fbDir);
	if (fbSizer) {
		m_directoryEdit = (wxTextCtrl*) GetLastControl();
		m_directorySelectButton = (wxButton*) this->FindWindowByName(
				wxString::Format(wxT("SelectDirButton_%d"), GetLastControlIndex()));
	}
	m_directoryEdit->Enable(!fbDir.IsEmpty());
	m_directorySelectButton->Enable(!fbDir.IsEmpty());
#ifndef GNOME2
	AddText(interfaceGrid, wxEmptyString);
	AddCheckProp(interfaceGrid, _("Clear thumbnail cache after exit"), s_config.GetClearThumbnailCache(def));
#endif
	
	// undo history size
	AddSpinProp(interfaceGrid, _("Undo history size:"), s_config.GetUndoHistorySize(def), 0, 9999, 60);
	AddText(interfaceGrid, _("Clear transcoding cache:"));
	wxSizer* grpSizer = BeginGroup(interfaceGrid, wxT(""));
	wxArrayString options;
	options.Add(_("Prompt"));
	options.Add(_("Yes"));
	options.Add(_("No"));
	AddRadioGroupProp(grpSizer, options, s_config.GetCacheClearPrompt(def));
	EndGroup();
	if (grpSizer)
		grpSizer->Add(new wxButton(propWindow, CLEAR_CACHE_BT, _("Clear cache now")));
	AddStretchSpacer(grpSizer, 1);
	
	// don't show fags
	AddText(interfaceGrid, _("\"Don't show again\" flags:"));
	if (interfaceGrid)
		interfaceGrid->Add(new wxButton(propWindow, RESET_DONT_SHOW_FLAGS_BT_ID, _("Reset All")));
	

	// -------------- Core Tab Sizers ----------------- //
	wxBoxSizer* coreSizer = NULL;
	wxFlexGridSizer* coreGrid = NULL;
	wxBoxSizer* debugSizer = NULL;
	if (sizer) {
		wxPanel* interfacePanel = new wxPanel(notebook, -1);
		notebook->AddPage(interfacePanel, _("Core"));
		propWindow = interfacePanel;

		coreSizer = new wxBoxSizer(wxVERTICAL);
		interfacePanel->SetAutoLayout(true);
		interfacePanel->SetSizer(coreSizer);

		coreGrid = new wxFlexGridSizer(2, 4, 16);
		coreGrid->AddGrowableCol(1);
		coreSizer->Add(coreGrid, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);

		debugSizer = new wxBoxSizer(wxVERTICAL);
		coreSizer->Add(debugSizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 10);
	}

	// ----------------- Core Tab -------------------- //
	wxSizer* lineSizer = AddSpinProp(coreGrid, _("Frame count of menu:"), s_config.GetMenuFrameCount(def), 1, 9999, 80,
			wxT(""), false);
	AddSpacer(lineSizer, 4);
	AddCheckProp(lineSizer, _("Draw buttons on background"), s_config.GetDrawButtonsOnBackground(def));
	AddCheckProp(lineSizer, _("Allow HD menus"), s_config.GetAllowHdMenues(def));
	
	lineSizer = AddSpinProp(coreGrid, _("Menu video bitrate:"), s_config.GetMenuVideoBitrate(def), 1, 9000, 80,
			_("KBit/s"), false);
	AddSpacer(lineSizer, 4);
	AddCheckProp(lineSizer, wxT("CBR"), s_config.GetMenuVideoCBR(def));
		
	lineSizer = AddSpinProp(coreGrid, _("Slideshow video bitrate:"), s_config.GetSlideshowVideoBitrate(def), 1, 9000,
			80, _("KBit/s"), false);
	AddSpacer(lineSizer, 4);
	AddCheckProp(lineSizer, wxT("CBR"), s_config.GetSlideshowVideoCBR(def));
	
	AddSpinProp(coreGrid, _("Default audio bitrate:"), s_config.GetAudioBitrate(def), 1, 9999, 80, _("KBit/s"));
	AddSpinProp(coreGrid, _("Thread count:"), s_config.GetThreadCount(def), 1, 99, 80);
	AddCheckProp(coreGrid, _("Use hardware video decoding (VA-API)"), s_config.GetUseVAAPI(def));
	AddSpinProp(coreGrid, _("DVD reserved space:"), s_config.GetDvdReservedSpace(def), 0, 9999999, 80, _("KB"));
	
	// Encoder
	wxArrayString encoders;
	encoders.Add(_("internal"));
#ifdef __WXMSW__
	encoders.Add(wxT("ffmpeg-vbr"));
#endif
	int encoder = s_config.GetEncoder(def) == wxT("ffmpeg-vbr") ? 1 : 0;
	wxSizer* encoderSizer = AddChoiceCustomProp(coreGrid, _("Encoder:"), encoders[encoder], encoders, 1, 0, false);
	encoderCtrl = (wxChoice*) GetLastControl();
	AddSpacer(encoderSizer, 4);
	if (modes.size() == 0) {
		modes.Add(wxT("cbr"));
		modes.Add(wxT("vbr1pass"));
		modes.Add(wxT("vbr2pass"));
	}
	wxArrayString modeLabels;
	modeLabels.Add(_("auto"));
	modeLabels.Add(_("CBR"));
	modeLabels.Add(_("1-pass VBR"));
	modeLabels.Add(_("2-pass VBR"));
	int mode = modes.Index(s_config.GetEncoderMode(def).BeforeFirst(wxT('_')));
	AddChoiceProp(encoderSizer, wxT(""), modeLabels[mode >= 0 ? mode + 1 : 0], modeLabels, -2, false, MODE_CHOICE_ID);
	if (encoderSizer)
		SetLastControlCustom(GetLastControlIndex() - 1, s_config.GetEncoder(def) == s_config.GetEncoder(true));
	modeCtrl = (wxChoice*) GetLastControl();
	AddSpacer(encoderSizer, 4);
	AddCheckProp(encoderSizer, wxT("HQ"), s_config.GetEncoderMode(def).AfterFirst('_') == "hq", false, HQ_CHECK_ID);
	hqCtrl = (wxCheckBox*) GetLastControl();
	AddSpacer(encoderSizer, 4);
	AddCheckProp(encoderSizer, wxT("XHQ"), s_config.GetEncoderMode(def).AfterFirst('_') == "xhq", false, XHQ_CHECK_ID);
	xhqCtrl = (wxCheckBox*) GetLastControl();
	wxCommandEvent evt;
	OnChangeEncoderMode(evt);
	
	AddTextProp(coreGrid, _("FFmpeg options:"), s_config.GetFfmpegOptions(def));
	AddTextProp(coreGrid, _("Create ISO command:"), s_config.GetIsoCmd(def));
	AddTextProp(coreGrid, _("Burn command:"), s_config.GetBurnCmd(def));
	AddTextProp(coreGrid, _("Burn ISO command:"), s_config.GetBurnISOCmd(def));
	AddTextProp(coreGrid, _("Add ECC command:"), s_config.GetAddECCCmd(def));
	AddTextProp(coreGrid, _("Format command:"), s_config.GetFormatCmd(def));
	
	AddText(coreGrid, _("Use mplex:"));
	grpSizer = BeginGroup(coreGrid, wxT(""));
	labels.clear();
	labels.Add(_("Yes"));
	labels.Add(_("No"));
	labels.Add(_("For menus only"));
	int opt = s_config.GetUseMplex(def) ? 0 : (s_config.GetUseMplexForMenus(def) ? 2 : 1);
	AddRadioGroupProp(grpSizer, labels, opt);
	EndGroup();
	
	AddText(coreGrid, _("NTSC film:"));
	AddCheckProp(coreGrid, _("re-encode by default"), s_config.GetDefRencodeNtscFilm(def));
	AddText(coreGrid, _("HD video:"));
	AddCheckProp(coreGrid, _("allow (experimental)"), s_config.GetAllowHdTitles(def));
	AddText(coreGrid, _("HD output:"));
	grpSizer = BeginGroup(coreGrid, wxT(""));
	labels.clear();
	labels.Add(_("none"));
	labels.Add(_("Blu-ray (BD-R/BD-RE)"));
	labels.Add(_("AVCHD (DVD-R/RW)"));
	AddRadioGroupProp(grpSizer, labels, s_config.GetBlurayMode(def));
	EndGroup();
	AddSpinProp(coreGrid, _("Blu-ray video bitrate:"), s_config.GetBlurayVideoBitrate(def), 1000, 40000, 80, _("KBit/s"));
	AddSpinProp(coreGrid, _("AVCHD video bitrate:"), s_config.GetAvchdVideoBitrate(def), 1000, 18000, 80, _("KBit/s"));
	AddSpinProp(coreGrid, _("Audio bitrate:"), s_config.GetBlurayAudioBitrate(def), 128, 640, 80, _("KBit/s"));
	AddTextProp(coreGrid, _("tsMuxeR command:"), s_config.GetBlurayTsMuxeRCmd(def));
	AddTextProp(coreGrid, _("Blu-ray ISO command:"), s_config.GetBlurayIsoCmd(def));

	grpSizer = BeginGroup(debugSizer, _("Debug"));
	AddCheckProp(grpSizer, _("Don't remove temp files"), !s_config.GetRemoveTempFiles(def));
	
}

bool SettingsDlg::SetValues() {
	// check encoder
#ifdef __WXMSW__
	if (encoderCtrl->GetSelection() > 0) {
		if (!wxFileExists(wxGetAppPath() + encoderCtrl->GetStringSelection() + wxT(".exe"))
				&& !wxFileExists(wxGetAppPath() + encoderCtrl->GetStringSelection() + wxT(".bat"))) {
			wxMessageBox(wxT("Encoder ") + encoderCtrl->GetStringSelection() + wxT(" is not found. Please install."),
					GetTitle(), wxOK|wxICON_ERROR, this);
			return false;
		}
	}
#endif
	
	// interface settings
	int i = 0;
	if (GetLangCode(GetInt(i++)) != s_config.GetLanguageCode()) {
		s_config.SetLanguageCode(GetLangCode(GetInt(i - 1)));
		wxString langCode = s_config.GetLanguageCode().Upper().substr(0, 2);
		if (DVD::GetAudioLanguageCodes().Index(langCode) != wxNOT_FOUND)
			s_config.SetDefSubtitleLanguage(langCode);
		wxMessageBox(_("Language change will not take effect until DVDStyler is restarted"),
				GetTitle(), wxOK|wxICON_INFORMATION, this);
	}
	s_config.SetDefDiscLabel(GetString(i++));
	s_config.SetDefDiscCapacity(GetInt(i++));
	s_config.SetDefVideoFormat(GetInt(i++) + 2);
	s_config.SetDefAspectRatio(GetInt(i++) + 1);
	s_config.SetDefKeepAspectRatio(GetBool(i++));
	s_config.SetDefAudioFormat(GetInt(i++) + 2);
	s_config.SetDefAudioLanguage(GetString(i++));
	s_config.SetDefAudio51(GetBool(i++));
	s_config.SetDefAudioNormalize(GetBool(i++));
	s_config.SetDefChapterLength(GetInt(i++));
	s_config.SetAddChapterAtTitleEnd(GetBool(i++));
	s_config.SetDefPostCommand(GetInt(i++));
	s_config.SetDefButton(m_buttons[GetInt(i++)]);
	s_config.SetAcceptInvalidActions(GetBool(i++));
	s_config.SetDefSlideDuration(GetInt(i++));
	s_config.SetDefTransitionDuration(GetDouble(i++));
	s_config.SetDefTransition(m_transitions[GetInt(i++)]);
	
	i++;
	s_config.SetFileBrowserDir(GetString(i++));
#ifndef GNOME2
	s_config.SetClearThumbnailCache(GetBool(i++));
#endif
	s_config.SetUndoHistorySize(GetInt(i++));
	s_config.SetCacheClearPrompt(GetInt(i++));
	
	// system core settings
	s_config.SetMenuFrameCount(GetInt(i++));
	s_config.SetDrawButtonsOnBackground(GetBool(i++));
	s_config.SetAllowHdMenues(GetBool(i++));
	s_config.SetMenuVideoBitrate(GetInt(i++));
	s_config.SetMenuVideoCBR(GetBool(i++));
	s_config.SetSlideshowVideoBitrate(GetInt(i++));
	s_config.SetSlideshowVideoCBR(GetBool(i++));
	s_config.SetAudioBitrate(GetInt(i++));
	s_config.SetThreadCount(GetInt(i++));
	s_config.SetUseVAAPI(GetBool(i++));
	s_config.SetDvdReservedSpace(GetInt(i++));
	wxString encoder = GetInt(i++) > 0 ? GetString(i - 1) : wxT("");
	s_config.SetEncoder(encoder);
	wxString mode = encoder.length() == 0 || GetInt(i) == 0 ? wxT("") : modes[GetInt(i) - 1];
	if (GetBool(i+1) && mode.length())
		mode += "_hq";
	else if (GetBool(i+2) && mode.length())
		mode += "_xhq";
	s_config.SetEncoderMode(mode);
	i += 3;
	s_config.SetFfmpegOptions(GetString(i++)); //-hwaccel cuda -hwaccel_output_format cuda
	s_config.SetIsoCmd(GetString(i++));
	s_config.SetBurnCmd(GetString(i++));
	s_config.SetBurnISOCmd(GetString(i++));
	s_config.SetAddECCCmd(GetString(i++));
	s_config.SetFormatCmd(GetString(i++));
	int mplex = GetInt(i++);
	s_config.SetUseMplex(mplex == 0);
	s_config.SetUseMplexForMenus(mplex == 0 || mplex == 2);
	s_config.SetDefRencodeNtscFilm(GetBool(i++));
	s_config.SetAllowHdTitles(GetBool(i++));
	s_config.SetBlurayMode(GetInt(i++));
	s_config.SetBlurayVideoBitrate(GetInt(i++));
	s_config.SetAvchdVideoBitrate(GetInt(i++));
	s_config.SetBlurayAudioBitrate(GetInt(i++));
	s_config.SetBlurayTsMuxeRCmd(GetString(i++));
	s_config.SetBlurayIsoCmd(GetString(i++));
	s_config.SetRemoveTempFiles(!GetBool(i++));
	return true;
}

void SettingsDlg::OnChangeFileBrowserDir(wxCommandEvent& evt) {
	wxString dir = evt.GetSelection() == 0 ? wxT("") : wxGetHomeDir();
	m_directoryEdit->SetValue(dir);
	m_directoryEdit->Enable(evt.GetSelection() == 1);
	m_directorySelectButton->Enable(evt.GetSelection() == 1);
}

void SettingsDlg::OnResetDontShowFlags(wxCommandEvent& evt) {
	s_config.SetShowWelcomeDlg(true);
	s_config.SetTitleDeletePrompt(true);
	s_config.SetOverwriteOutputDirPrompt(true);
	s_config.SetCacheClearPrompt(0);
	wxMessageBox(_("All \"don't show again\" flags are reseted."), _("Settings"),
			wxOK | wxCENTRE | wxICON_INFORMATION);
}

void SettingsDlg::OnClearCache(wxCommandEvent& evt) {
	if (m_cache->Clear())
		wxMessageBox(_("The transcoding cache is successfully cleared."), _("Settings"),
				wxOK | wxCENTRE | wxICON_INFORMATION);
}

void SettingsDlg::OnChangeEncoderMode(wxCommandEvent& evt) {
	hqCtrl->Enable(modeCtrl->GetSelection() > 0);
	if (!hqCtrl->IsEnabled())
		hqCtrl->SetValue(false);
	xhqCtrl->Enable(modeCtrl->GetSelection() == 3);
	if (!xhqCtrl->IsEnabled())
		xhqCtrl->SetValue(false);
}

void SettingsDlg::OnCheckHQ(wxCommandEvent& evt) {
	if (evt.IsChecked())
		xhqCtrl->SetValue(false);
}

void SettingsDlg::OnCheckXHQ(wxCommandEvent& evt) {
	if (evt.IsChecked())
		hqCtrl->SetValue(false);
}

void SettingsDlg::AddSpacer(wxSizer* sizer, int size) {
	if (sizer)
		sizer->AddSpacer(size);
}

void SettingsDlg::AddStretchSpacer(wxSizer* sizer, int prop) {
	if (sizer)
		sizer->AddStretchSpacer(prop);
	
}
