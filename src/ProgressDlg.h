/////////////////////////////////////////////////////////////////////////////
// Name:        ProgressDlg.h
// Author:      Alex Thuering
// Created:     14.08.2004
// RCS-ID:      $Id: ProgressDlg.h,v 1.49 2016/12/11 10:06:31 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef PROGRESSDLG_H
#define PROGRESSDLG_H

//(*Headers(ProgressDlg)
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/gauge.h>
//*)
#include "mediaenc.h"
#include <wx/ffile.h>

class DVD;
class Cache;
class MainWin;
class BurnDlg;

/**
 * Progress of generation/burning dialog
 */
class ProgressDlg: public wxDialog, public AbstractProgressDialog {
public:
	/** Constructor */
	ProgressDlg(MainWin* parent, Cache* cache, bool autoStart);
	/** Destructor */
	virtual ~ProgressDlg();
	
	/** starts the generating of DVD */
	bool Start(BurnDlg* burnDlg, DVD* dvd);
	/** burns the given ISO image directly with the default tool at default speed */
	bool BurnIso(const wxString& isoFile);
    /** Returns true, if process is canceled */
	bool WasCanceled();
    /** Cancel the process */
	void Cancel();
	/** Sets failed status */
	void Failed(const wxString& message = wxEmptyString);

	/** Adds the given message to summary textbox */
	void AddSummaryMsg(const wxString& message, const wxString& details = wxEmptyString,
			const wxColour& colour = wxNullColour);
	/** Replaces last line with the given text in summary textbox */
	void ReplaceSummaryMsg(const wxString& message);
	/** Add the given message to details textbox */
	void AddDetailMsg(const wxString& message, const wxColour& colour = wxNullColour);
	/** Replaces last message with the given message in details textbox */
	void ReplaceLastDetailMsg(const wxString& text);
	/** Add the given text to details textbox */
	void AddDetailText(const wxString& text);
	/** Replaces last line with the given text in details textbox */
	void ReplaceLastDetailText(const wxString& text);
	/** Returns last line from details textbox */
	wxString GetLastDetailText() { return m_lastDetailText; }

	/** Sets the step count of generating process */
	void SetSteps(int stepCount);
	/** Increments the step */
	void IncStep() {
		m_step ++;
		m_subStep = 0;
		UpdateGauge();
	}

	/** Sets the sub-step count of current step */
	void SetSubSteps(int value) {
		m_subStepCount = value;
		m_subStep = 0;
	}
	/** Sets the sub-step */
	void SetSubStep(int value) {
		m_subStep = value;
		UpdateGauge();
	}
	/** increments the sub-step */
	void IncSubStep(int value = 1) {
		SetSubStep(GetSubStep() + value);
	}
	/** Returns the sub-step */
	int GetSubStep() {
		return m_subStep;
	}
	
	/** Returns true, when PC need to be turned off when burning is finished */
	bool DoShutdown() {
		return m_shutdown->IsChecked();
	}

private:
	//(*Declarations(ProgressDlg)
	wxCheckBox* m_shutdown;
	wxButton* m_cancelBt;
	wxTextCtrl* m_summaryText;
	wxTextCtrl* m_detailsText;
	wxBoxSizer* m_panelSizer;
	wxButton* m_minimizeBt;
	wxStaticText* m_detailsLabel;
	wxButton* m_detailsBt;
	wxGauge* m_gauge;
	//*)
	//(*Identifiers(ProgressDlg)
	static const long ID_SUMMURY_CTRL;
	static const long ID_GAUGE1;
	static const long ID_DETAILS_CTRL;
	static const long ID_DETAILS_BT;
	static const long ID_CHECKBOX1;
	static const long ID_MINIMIZE_BT;
	//*)
	//(*Handlers(ProgressDlg)
	void OnHideDetails(wxCommandEvent& event);
	void OnMinimize(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnShutdownClick(wxCommandEvent& event);
	//*)

	wxWindowDisabler* m_winDisabler;
	wxString m_detailsBtLabel;

	bool m_autoStart;
	bool m_end;
	bool m_close;
	bool m_cancel;
	
	int m_step;
	int m_stepCount;
	int m_subStepCount;
	int m_subStep;
	
	Cache* m_cache;
	wxFFile m_logFile;
	wxString m_lastDetailText;
	wxString m_lastSummaryMsg;

	/** Initializes gauge */
	void InitGauge(int range);
	void UpdateGauge();
	
	void Run(BurnDlg* burnDlg, DVD* dvd);

	void End();

	DECLARE_EVENT_TABLE()
};

#endif
