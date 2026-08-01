/////////////////////////////////////////////////////////////////////////////
// Name:        ProgressDlg.cpp
// Author:      Alex Thuering
// Created:     14.08.2004
// RCS-ID:      $Id: ProgressDlg.cpp,v 1.182 2016/12/11 10:06:31 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "ProgressDlg.h"
#include "MainWin.h"
#include "BurnDlg.h"
#include "DVD.h"
#include "Cache.h"
#include "ProcessCleanTemp.h"
#include "ProcessProjectInfo.h"
#include "ProcessMenu.h"
#include "ProcessMenuTransitions.h"
#include "ProcessEncode.h"
#include "ProcessSlideshow.h"
#include "ProcessSubtitles.h"
#include "ProcessDvdFilesystem.h"
#include "ProcessBluray.h"
#include "ProcessAskOutput.h"
#include "ProcessPreview.h"
#include "ProcessIsoImage.h"
#include "ProcessEccData.h"
#include "ProcessFormatDvd.h"
#include "ProcessBurn.h"
#include "mediaenc_ffmpeg.h"
#include "Config.h"
#include "SysUtils.h"
#include "Version.h"
#include <wxVillaLib/utils.h>
#include <wx/filename.h>

//(*InternalHeaders(ProgressDlg)
#include <wx/intl.h>
#include <wx/string.h>
//*)

class ProcessLog: public wxLog {
public:
	/** Constructor */
	ProcessLog(ProgressDlg* progressDlg) {
		this->m_progressDlg = progressDlg;
	}
protected:
	/** Print the message into progress dialog details window. */
	void DoLogRecord(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) {
		m_progressDlg->AddDetailMsg(msg, level <= wxLOG_Error ? *wxRED : wxColour(64,64,64));
	}
private:
	ProgressDlg* m_progressDlg;
};

//(*IdInit(ProgressDlg)
const long ProgressDlg::ID_SUMMURY_CTRL = wxNewId();
const long ProgressDlg::ID_GAUGE1 = wxNewId();
const long ProgressDlg::ID_DETAILS_CTRL = wxNewId();
const long ProgressDlg::ID_DETAILS_BT = wxNewId();
const long ProgressDlg::ID_CHECKBOX1 = wxNewId();
const long ProgressDlg::ID_MINIMIZE_BT = wxNewId();
//*)

BEGIN_EVENT_TABLE(ProgressDlg,wxDialog)
	//(*EventTable(ProgressDlg)
	//*)
END_EVENT_TABLE()

ProgressDlg::ProgressDlg(MainWin* parent, Cache* cache, bool autoStart) {
	//(*Initialize(ProgressDlg)
	wxStaticText* summaryLabel;
	wxBoxSizer* btSizer;
	wxBoxSizer* mainSizer;

	Create(parent, wxID_ANY, _("Generate DVD"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER, _T("wxID_ANY"));
	SetClientSize(wxSize(600,600));
	mainSizer = new wxBoxSizer(wxVERTICAL);
	m_panelSizer = new wxBoxSizer(wxVERTICAL);
	summaryLabel = new wxStaticText(this, wxID_ANY, _("Summary:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	m_panelSizer->Add(summaryLabel, 0, wxBOTTOM|wxALIGN_LEFT, 2);
	m_summaryText = new wxTextCtrl(this, ID_SUMMURY_CTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH, wxDefaultValidator, _T("ID_SUMMURY_CTRL"));
	m_panelSizer->Add(m_summaryText, 1, wxBOTTOM|wxEXPAND, 8);
	m_gauge = new wxGauge(this, ID_GAUGE1, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxGA_SMOOTH, wxDefaultValidator, _T("ID_GAUGE1"));
	m_panelSizer->Add(m_gauge, 0, wxBOTTOM|wxEXPAND, 4);
	m_detailsLabel = new wxStaticText(this, wxID_ANY, _("Details:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	m_panelSizer->Add(m_detailsLabel, 0, wxBOTTOM|wxALIGN_LEFT, 2);
	m_detailsText = new wxTextCtrl(this, ID_DETAILS_CTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH, wxDefaultValidator, _T("ID_DETAILS_CTRL"));
	m_panelSizer->Add(m_detailsText, 1, wxEXPAND, 0);
	mainSizer->Add(m_panelSizer, 1, wxTOP|wxLEFT|wxRIGHT|wxEXPAND, 8);
	btSizer = new wxBoxSizer(wxHORIZONTAL);
	m_detailsBt = new wxButton(this, ID_DETAILS_BT, _("Hide details"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_DETAILS_BT"));
	btSizer->Add(m_detailsBt, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	m_shutdown = new wxCheckBox(this, ID_CHECKBOX1, _("Turn off PC when finished"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX1"));
	m_shutdown->SetValue(false);
	btSizer->Add(m_shutdown, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	btSizer->Add(-1,-1,1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	m_minimizeBt = new wxButton(this, ID_MINIMIZE_BT, _("Minimize"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_MINIMIZE_BT"));
	btSizer->Add(m_minimizeBt, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 8);
	m_cancelBt = new wxButton(this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("wxID_CANCEL"));
	btSizer->Add(m_cancelBt, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	mainSizer->Add(btSizer, 0, wxALL|wxEXPAND, 8);
	SetSizer(mainSizer);
	SetSizer(mainSizer);
	Layout();
	Center();

	Connect(ID_DETAILS_BT,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&ProgressDlg::OnHideDetails);
	Connect(ID_CHECKBOX1,wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&ProgressDlg::OnShutdownClick);
	Connect(ID_MINIMIZE_BT,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&ProgressDlg::OnMinimize);
	Connect(wxID_CANCEL,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&ProgressDlg::OnCancel);
	//*)

	m_detailsBtLabel = m_detailsBt->GetLabel();
	m_detailsBt->SetLabel(_T("<< ") + m_detailsBtLabel);
    m_detailsText->SetFont(wxFont(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	m_winDisabler = NULL;

    m_end = false;
    m_close = false;
    m_autoStart = autoStart;
    m_cancel = false;
    m_step = 0;
    m_stepCount = 0;
    m_subStep = 0;
    m_subStepCount = 0;
    m_cache = cache;
    
#if !defined(__WXMSW__) && !defined(HAVE_LIBDBUS)
    m_shutdown->Enable(false);
#endif
}

ProgressDlg::~ProgressDlg() {
	//(*Destroy(ProgressDlg)
	//*)
}

void ProgressDlg::OnHideDetails(wxCommandEvent& event) {
	if (m_detailsText->IsShown()) {
		m_detailsLabel->Hide();
		m_detailsText->Hide();
		m_detailsText->Freeze();
		m_panelSizer->Detach(m_detailsLabel);
		m_panelSizer->Detach(m_detailsText);
		int height = m_detailsLabel->GetSize().GetY() + m_detailsText->GetSize().GetY() + 2;
		SetSize(GetSize().GetX(), GetSize().GetY() - height);
		m_detailsBt->SetLabel(_("Show details") + wxString(_T(" >>")));
	} else {
		m_detailsLabel->Show();
		m_detailsText->Show();
		m_detailsText->Thaw();
		m_panelSizer->Insert(3, m_detailsLabel, 0, wxBOTTOM, 2);
		m_panelSizer->Insert(4, m_detailsText, 1, wxEXPAND, 0);
		int height = m_detailsLabel->GetSize().GetY() + m_detailsText->GetSize().GetY() + 2;
		SetSize(GetSize().GetX(), GetSize().GetY() + height);
		m_detailsBt->SetLabel(_T("<< ") + m_detailsBtLabel);
	}
}

void ProgressDlg::OnMinimize(wxCommandEvent& event) {
	wxPlatformInfo info;
	if ((info.GetOperatingSystemId() & wxOS_WINDOWS) && !info.CheckOSVersion(6,0))
		wxDELETE(m_winDisabler);
	((wxFrame*) GetParent())->Iconize();
}

void ProgressDlg::OnCancel(wxCommandEvent& event) {
	if (!WasCanceled() && !m_end) {
		Cancel();
	} else {
		m_close = true;
		((MainWin*) this->GetParent())->SetProgressBarValue(0, 100);
		((MainWin*) this->GetParent())->SetProgressBarState();
	}
}

/** Adds the given message to summary textbox */
void ProgressDlg::AddSummaryMsg(const wxString& message, const wxString& details, const wxColour& colour) {
	m_summaryText->SetDefaultStyle(wxTextAttr(colour.Ok() ? colour : *wxBLACK));
	m_summaryText->AppendText(message + _T("\n"));
	m_summaryText->ShowPosition(m_summaryText->GetLastPosition());
	AddDetailMsg(details.length() ? details : message, colour.Ok() ? colour : *wxBLACK);
	m_lastSummaryMsg = message;
}

/** Replaces last line with the given text in details textbox */
void ProgressDlg::ReplaceSummaryMsg(const wxString& message) {
	long lastPos = m_summaryText->GetLastPosition();
	m_summaryText->Replace(lastPos - m_lastSummaryMsg.length() - 1, lastPos + 1, message + wxT("\n"));
	m_summaryText->ShowPosition(m_summaryText->GetLastPosition());
	wxYieldIfNeeded();
	m_lastSummaryMsg = message;
}

/** Add the given message to details textbox */
void ProgressDlg::AddDetailMsg(const wxString& message, const wxColour& colour) {
	if (m_cancel)
		return;
	if (colour.Ok())
		m_detailsText->SetDefaultStyle(wxTextAttr(colour));
	AddDetailText(message + _T("\n"));
	m_detailsText->SetDefaultStyle(wxTextAttr(wxColour(64, 64, 64)));
}

/** Replaces last message with the given message in details textbox */
void ProgressDlg::ReplaceLastDetailMsg(const wxString& message) {
	ReplaceLastDetailText(message + _T("\n"));
}

/** Add the given text to details textbox */
void ProgressDlg::AddDetailText(const wxString& text) {
	if (wxLog::GetActiveTarget()->GetVerbose())
		fprintf(stderr, "%s", (const char*) text.mb_str());
	m_logFile.Write(text);
	m_logFile.Flush();
	m_detailsText->AppendText(text);
	m_detailsText->ShowPosition(m_detailsText->GetLastPosition());
	wxYieldIfNeeded();
	if (text.StartsWith(wxT("Encoding Mode:"))) {
		wxString mode = text.Mid(15);
		int len = mode.Find(wxT("HQ"));
		if (len > 0)
			mode = mode.Mid(0, len).Strip();
		else
			mode = mode.BeforeFirst(wxT(' '));
		if (mode.length() == 0)
			mode = _("Copy");
		int pos = m_lastSummaryMsg.Find(_("Encoding Mode"));
		if (pos > 0) {
			ReplaceSummaryMsg(m_lastSummaryMsg.Mid(0, pos + 15) + mode + wxT(")"));
		} else {
			ReplaceSummaryMsg(m_lastSummaryMsg + wxT(" (") + _("Encoding Mode") + wxT(": ") + mode + wxT(")"));
		}
	}
	m_lastDetailText = text;
}

/** Replaces last line with the given text in details textbox */
void ProgressDlg::ReplaceLastDetailText(const wxString& text) {
	if (wxLog::GetActiveTarget()->GetVerbose())
		fprintf(stderr, "%s", (const char*) text.mb_str());
	long lastPos = m_detailsText->GetLastPosition();
	m_detailsText->Replace(lastPos - m_lastDetailText.length(), lastPos + 1, text);
	m_detailsText->ShowPosition(m_detailsText->GetLastPosition());
	wxYieldIfNeeded();
	m_lastDetailText = text;
}

/** Sets the step count of generating process */
void ProgressDlg::SetSteps(int stepCount) {
	m_stepCount = stepCount;
	m_step = 0;
	InitGauge(stepCount*100);
}

void ProgressDlg::UpdateGauge() {
	int subStep = 0;
	if (m_subStepCount > 0 && m_subStep > m_subStepCount)
		m_subStep = m_subStepCount;
	if (m_subStepCount > 0)
		subStep = m_subStep * 100 / m_subStepCount;
	int step = m_step * 100 + subStep;
	m_gauge->SetValue(step);
	((MainWin*) this->GetParent())->SetProgressBarValue(step, m_stepCount * 100);
}

/** Initializes gauge */
void ProgressDlg::InitGauge(int range) {
	m_gauge->SetRange(range);
	((MainWin*) this->GetParent())->SetProgressBarState();
}

void ProgressDlg::Failed(const wxString& message) {
	AddSummaryMsg(_("Failed"), message, *wxRED);
	((MainWin*) this->GetParent())->SetProgressBarState(true);
}

void ProgressDlg::End() {
	if (!WasCanceled())
		wxBell();
	m_cancelBt->SetLabel(_("Close"));
}

bool ProgressDlg::WasCanceled() {
	wxYieldIfNeeded();
	return m_cancel;
}

/** Cancel the process */
void ProgressDlg::Cancel() {
	AddSummaryMsg(_("Aborted"), wxEmptyString, *wxRED);
	m_cancel = true;
}

bool ProgressDlg::Start(BurnDlg* burnDlg, DVD* dvd) {
	// disable parent window
	m_winDisabler = new wxWindowDisabler(this);
	// show dialog
	Show();
	// start
	wxLog* previousLog = wxLog::SetActiveTarget(new ProcessLog(this));
	// start
	Run(burnDlg, dvd);
	// end
	End();
	m_logFile.Close();
	// restore log
	delete wxLog::SetActiveTarget(previousLog);
	m_end = true;
	// close the Window or release the controls
	if (m_autoStart) {
		Close(true);
	} else {
    	while (!m_close) {
    		wxMilliSleep(100);
    		wxYield();
    	}
    }
	wxDELETE(m_winDisabler);
	return !m_cancel;
}

void ProgressDlg::Run(BurnDlg* burnDlg, DVD* dvd) {
	if (WasCanceled())
		return;

	// check if libav or ffmpeg is present
#ifdef __WXMSW__
	if ((!wxFileExists(wxGetAppPath() + s_config.GetAVConvCmd() + wxT(".exe"))
			&& !wxFileExists(wxGetAppPath() + s_config.GetAVConvCmd()))
			|| s_config.GetAVConvCmd() == wxT("ffmpeg-vbr")) {
		if (wxFileExists(wxGetAppPath() + wxT("avconv.exe")))
			s_config.SetAVConvCmd(wxT("avconv"));
		else if (wxFileExists(wxGetAppPath() + wxT("ffmpeg.exe")))
			s_config.SetAVConvCmd(wxT("ffmpeg"));
	}
#endif

	wxString tmpDir = burnDlg->GetTempDir();
	if (tmpDir.Last() != wxFILE_SEP_PATH)
		tmpDir += wxFILE_SEP_PATH;
	wxString dvdTmpDir = tmpDir + wxString(wxT("dvd-tmp")) + wxFILE_SEP_PATH;
	wxString dvdOutDir = tmpDir + wxString(wxT("dvd-out")) + wxFILE_SEP_PATH;
	if (burnDlg->DoGenerate()) {
		dvdOutDir = burnDlg->GetOutputDir();
		if (dvdOutDir.Last() != wxFILE_SEP_PATH)
			dvdOutDir += wxFILE_SEP_PATH;
	}

	// create log file
	m_logFile.Open(tmpDir + wxT("dvdstyler.log"), wxT("w"));

	// print version
	AddDetailMsg(wxT("DVDStyler v") + APP_VERSION);
	AddDetailMsg(wxGetOsDescription());
	AddDetailMsg((s_config.GetAVConvCmd() == wxT("ffmpeg") ? wxT("FFmpeg: ") : wxT("Libav: "))
			+ wxFfmpegMediaEncoder::GetBackendVersion());

	// prepare
	AddSummaryMsg(_("Prepare"));

	// clean temp dir
	ProcessCleanTemp cleanTemp(this, tmpDir, dvdTmpDir, dvdOutDir);
	if (!cleanTemp.Execute())
		return;

	// check cache and calculate steps
	AddDetailMsg(_("Search for transcoded files in cache"));
	m_cache->BeginClean();

	// processes
	vector<Process*> processes;
	processes.push_back(new ProcessProjectInfo(this, dvd)); // project info
	if (s_config.GetBlurayMode() != BD_MODE_NONE) {
		processes.push_back(new ProcessBluray(this, dvd, burnDlg, dvdOutDir, tmpDir)); // HD titles
	} else {
		processes.push_back(new ProcessMenu(this, dvd, dvdTmpDir)); // menus
		processes.push_back(new ProcessEncode(this, dvd, m_cache, dvdTmpDir)); // titles
		processes.push_back(new ProcessSlideshow(this, dvd, dvdTmpDir)); // slideshow
		processes.push_back(new ProcessSubtitles(this, dvd, m_cache, dvdTmpDir)); // subtitle
		processes.push_back(new ProcessMenuTransitions(this, dvd, dvdTmpDir)); // menu transitions
		processes.push_back(new ProcessDvdFilesystem(this, dvd, dvdTmpDir, dvdOutDir));
	}
	processes.push_back(new ProcessAskOutput(this, burnDlg));
	if (s_config.GetBlurayMode() != BD_MODE_NONE) {
		processes.push_back(new ProcessBlurayIso(this, dvd, burnDlg, dvdOutDir, tmpDir)); // UDF 2.50 ISO
	} else {
		processes.push_back(new ProcessPreview(this, burnDlg, dvdOutDir));
		processes.push_back(new ProcessIsoImage(this, burnDlg, dvd, m_cache, dvdOutDir, tmpDir));
		processes.push_back(new ProcessEccData(this, burnDlg, tmpDir));
		processes.push_back(new ProcessFormatDvd(this, burnDlg));
	}
	processes.push_back(new ProcessBurn(this, burnDlg, dvd, dvdOutDir, tmpDir));

	// remove unused files from cache
	m_cache->EndClean();

	// calculate step count
	int stepCount = 0;
	for (vector<Process*>::iterator it = processes.begin(); it != processes.end(); it++) {
		Process* process = *it;
		if (process->IsNeedExecute() && process->IsUpdateGauge())
			stepCount++;
	}
	SetSteps(stepCount);

	// Start generation
	for (vector<Process*>::iterator it = processes.begin(); it != processes.end(); it++) {
		Process* process = *it;
		if (process->IsNeedExecute() && !process->Execute())
			return;
	}

	if (WasCanceled())
		return;

	// clear temp directory
	if (s_config.GetRemoveTempFiles())
		cleanTemp.DeleteTempFiles(burnDlg->DoCreateIso() || burnDlg->DoBurn());

	if (burnDlg->DoBurn())
		AddSummaryMsg(_("Burning was successful."), wxEmptyString, wxColour(0, 128, 0));
	else
		AddSummaryMsg(_("Generating was successful."), wxEmptyString, wxColour(0, 128, 0));
	wxLog::FlushActive();
	
	if (DoShutdown()) {
		SystemManager::Stop();
	}
}


void ProgressDlg::OnShutdownClick(wxCommandEvent& event) {
	if (event.IsChecked() && !SystemManager::CanStop()) {
		wxMessageBox(wxT("You don't have the privileges to turn off the computer."),
				wxTheApp->GetAppName(), wxICON_ERROR);
		m_shutdown->SetValue(false);
	}
}
