/////////////////////////////////////////////////////////////////////////////
// Name:		BurnDlg.h
// Purpose:		Burn dialog
// Author:		Alex Thuering
// Created:		23.03.2003
// RCS-ID:		$Id: BurnDlg.h,v 1.21 2016/12/11 10:34:21 ntalex Exp $
// Copyright:	(c) Alex Thuering
// Licence:		GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef BURNDLG_H
#define BURNDLG_H

#include <wx/wx.h>
#include <wx/image.h>
#include "DVD.h"
class Cache;

//(*Headers(BurnDlg)
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/hyperlink.h>
#include <wx/choice.h>
#include <wx/statbmp.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/combobox.h>
//*)

class BurnDlg: public wxDialog {
public:
	/** Constructor */
	BurnDlg(wxWindow* parent, DVD* dvd, Cache* cache);
	virtual ~BurnDlg();
	
    wxString GetTempDir() { return m_tempDirText->GetValue(); }
    wxString GetOutputDir() { return m_outputDirText->GetValue(); }
	bool DoPreview() { return m_previewCheck->GetValue(); }
	bool DoGenerate() { return m_generateRadioBt->GetValue(); }
	bool DoCreateIso() { return m_isoRadioBt->GetValue(); }
	wxString GetIsoFile() { return m_isoText->GetValue(); }
	bool DoAddECC() { return m_addECCCheck->GetValue(); }
	bool DoBurn() { return m_burnRadioBt->GetValue(); }
	bool DoFormat() { return m_formatCheck->GetValue(); }
	void SetCreateIso(bool value) { m_isoRadioBt->SetValue(value); }
	void SetBurn(bool value) { m_burnRadioBt->SetValue(value); }
	wxString GetDevice();
	int GetSpeed();

	//(*Declarations(BurnDlg)
	wxStaticText* m_outputLabel;
	wxStaticText* m_isoLabel;
	wxStaticText* m_outputRequiredSpaceText;
	wxStaticText* m_requiredSpaceText;
	wxRadioButton* m_burnRadioBt;
	wxStaticText* m_speedLabel;
	wxRadioButton* m_generateRadioBt;
	wxStaticText* m_deviceLabel;
	wxStaticText* tempDirLabel;
	wxStaticText* m_outputRequiredSpaceTitle;
	wxStaticText* m_isoFreeSpaceTitle;
	wxStaticText* m_isoRequiredSpaceTitle;
	wxStaticText* m_outputFreeSpaceTitle;
	wxStaticBitmap* m_errorBitmap;
	wxTextCtrl* m_tempDirText;
	wxCheckBox* m_previewCheck;
	wxHyperlinkCtrl* m_downloadLink;
	wxStaticText* m_isoRequiredSpaceText;
	wxButton* m_previewBt;
	wxButton* m_isoBt;
	wxStaticBitmap* m_okBitmap;
	wxButton* m_outputDirBt;
	wxCheckBox* m_formatCheck;
	wxChoice* m_previewChoice;
	wxStaticText* m_freeSpaceText;
	wxRadioButton* m_isoRadioBt;
	wxTextCtrl* m_previewTextCtrl;
	wxStaticText* m_outputFreeSpaceText;
	wxComboBox* m_speedChoice;
	wxCheckBox* m_addECCCheck;
	wxButton* m_tempDirBt;
	wxStaticText* m_isoFreeSpaceText;
	wxComboBox* m_deviceChoice;
	wxTextCtrl* m_isoText;
	wxTextCtrl* m_outputDirText;
	//*)

protected:
	//(*Identifiers(BurnDlg)
	static const long ID_STATICTEXT1;
	static const long TEMP_DIR_FIELD_ID;
	static const long TEMP_DIR_BT_ID;
	static const long ID_STATICTEXT3;
	static const long ID_STATICTEXT4;
	static const long PREVIEW_CHECK_ID;
	static const long ID_PREVIEW_CHOICE;
	static const long ID_TEXTCTRL2;
	static const long ID_PREVIEW_BT;
	static const long ID_HYPERLINKCTRL1;
	static const long GENERATE_RADIO_BT_ID;
	static const long ID_TEXTCTRL1;
	static const long ID_OUTPUT_BT;
	static const long ID_STATICTEXT6;
	static const long ID_STATICTEXT7;
	static const long ISO_RADIO_BT_ID;
	static const long ISO_FILE_FIELD_ID;
	static const long ISO_BT_ID;
	static const long ID_STATICTEXT2;
	static const long ID_STATICTEXT5;
	static const long BURN_RADIO_BT_ID;
	static const long ID_CHECKBOX1;
	static const long ID_COMBOBOX1;
	static const long ID_COMBOBOX2;
	static const long ID_CHECKBOX2;
	//*)

private:
    DVD* m_dvd;
    Cache* m_cache;
	wxArrayString m_devices;
	bool m_eccCheckEnabled;
	void SetDevice(wxString device);
	void SetSpeed(int speed);
	void UpdateCtrls(bool def = false);
	bool UpdateSpaceLabels(bool showErrors = false);
	wxString GetVlcPath();
	wxString GetMpcHcPath();

	//(*Handlers(BurnDlg)
	void OnTempDirChange(wxCommandEvent& event);
	void OnChooseTempDir(wxCommandEvent& event);
	void OnRadioBt(wxCommandEvent& event);
	void OnChooseIsoFile(wxCommandEvent& event);
	void OnChooseOutputDir(wxCommandEvent& event);
	void OnPreviewCheck(wxCommandEvent& event);
	void OnChoosePreviewCmd(wxCommandEvent& event);
	void OnWindowFocus(wxFocusEvent& event);
	//*)
	void OnActivate(wxActivateEvent &event);
    virtual void OnOk(wxCommandEvent &event);
    virtual void OnReset(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
