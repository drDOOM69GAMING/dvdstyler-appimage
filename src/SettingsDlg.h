/////////////////////////////////////////////////////////////////////////////
// Name:        SettingsDlg.h
// Purpose:     Settings dialog
// Author:      Alex Thuering
// Created:     27.03.2004
// RCS-ID:      $Id: SettingsDlg.h,v 1.11 2016/07/27 14:00:23 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////
 
#ifndef SETTINGS_DLG_H
#define SETTINGS_DLG_H

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wxVillaLib/PropDlg.h>
class Cache;

class SettingsDlg: public wxPropDlg {
public:
    SettingsDlg(wxWindow *parent, Cache* cache);
	virtual ~SettingsDlg() {}

protected:
	void CreatePropPanel(wxSizer* sizer);
	bool SetValues();
  
private:
	Cache* m_cache;
	wxArrayString m_buttons;
	wxArrayString m_transitions;
	wxTextCtrl* m_directoryEdit;
	wxButton* m_directorySelectButton;
	wxChoice* encoderCtrl;
	wxArrayString modes;
	wxChoice* modeCtrl;
	wxCheckBox* hqCtrl;
	wxCheckBox* xhqCtrl;
	// HD output controls (dependency-wired: mode enables/disables the rest)
	int m_hdModeGroupIdx;
	wxChoice* qualityCtrl;
	wxChoice* mediaSizeCtrl;
	wxCheckBox* autoBitrateCtrl;
	wxSpinCtrl* bdBitrateCtrl;
	wxSpinCtrl* avchdBitrateCtrl;
	wxSpinCtrl* audioBitrateCtrl;
	wxTextCtrl* tsmuxerCtrl;
	wxTextCtrl* bdIsoCtrl;
	void OnChangeFileBrowserDir(wxCommandEvent& evt);
	void OnResetDontShowFlags(wxCommandEvent& evt);
	void OnClearCache(wxCommandEvent& evt);
	void OnChangeEncoderMode(wxCommandEvent& evt);
	void OnChangeHdMode(wxCommandEvent& evt);
	void OnChangeAutoBitrate(wxCommandEvent& evt);
	void UpdateHdControls();
	void OnCheckHQ(wxCommandEvent& evt);
	void OnCheckXHQ(wxCommandEvent& evt);
	void AddSpacer(wxSizer* sizer, int size);
	void AddStretchSpacer(wxSizer* sizer, int prop);
	DECLARE_EVENT_TABLE()
};

#endif // TEXT_PROP_DLG_H
