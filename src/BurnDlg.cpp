/////////////////////////////////////////////////////////////////////////////
// Name:		BurnDlg.cpp
// Purpose:		Burn dialog
// Author:		Alex Thuering
// Created:		23.03.2003
// RCS-ID:		$Id: BurnDlg.cpp,v 1.57 2016/12/11 10:34:21 ntalex Exp $
// Copyright:	(c) Alex Thuering
// Licence:		GPL
/////////////////////////////////////////////////////////////////////////////

#include "BurnDlg.h"
#include "Cache.h"
#include "Config.h"
#include "MessageDlg.h"
#include "SysUtils.h"
#include <wx/utils.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/progdlg.h>
#include <wxVillaLib/utils.h>
#include "rc/ok.png.h"
#include "rc/error.png.h"

#if defined(__WXMSW__)
#define APP_EXT wxT(" (*.exe)|*.exe")
#elif defined(__WXMAC__)
#define APP_EXT wxT(" (*.app)|*.app")
#else
#define APP_EXT wxT(" |**")
#endif

//(*InternalHeaders(BurnDlg)
#include <wx/bitmap.h>
#include <wx/intl.h>
#include <wx/image.h>
#include <wx/string.h>
//*)

//(*IdInit(BurnDlg)
const long BurnDlg::ID_STATICTEXT1 = wxNewId();
const long BurnDlg::TEMP_DIR_FIELD_ID = wxNewId();
const long BurnDlg::TEMP_DIR_BT_ID = wxNewId();
const long BurnDlg::ID_STATICTEXT3 = wxNewId();
const long BurnDlg::ID_STATICTEXT4 = wxNewId();
const long BurnDlg::PREVIEW_CHECK_ID = wxNewId();
const long BurnDlg::ID_PREVIEW_CHOICE = wxNewId();
const long BurnDlg::ID_TEXTCTRL2 = wxNewId();
const long BurnDlg::ID_PREVIEW_BT = wxNewId();
const long BurnDlg::ID_HYPERLINKCTRL1 = wxNewId();
const long BurnDlg::GENERATE_RADIO_BT_ID = wxNewId();
const long BurnDlg::ID_TEXTCTRL1 = wxNewId();
const long BurnDlg::ID_OUTPUT_BT = wxNewId();
const long BurnDlg::ID_STATICTEXT6 = wxNewId();
const long BurnDlg::ID_STATICTEXT7 = wxNewId();
const long BurnDlg::ISO_RADIO_BT_ID = wxNewId();
const long BurnDlg::ISO_FILE_FIELD_ID = wxNewId();
const long BurnDlg::ISO_BT_ID = wxNewId();
const long BurnDlg::ID_STATICTEXT2 = wxNewId();
const long BurnDlg::ID_STATICTEXT5 = wxNewId();
const long BurnDlg::BURN_RADIO_BT_ID = wxNewId();
const long BurnDlg::ID_CHECKBOX1 = wxNewId();
const long BurnDlg::ID_COMBOBOX1 = wxNewId();
const long BurnDlg::ID_COMBOBOX2 = wxNewId();
const long BurnDlg::ID_CHECKBOX2 = wxNewId();
//*)

BEGIN_EVENT_TABLE(BurnDlg,wxDialog)
	//(*EventTable(BurnDlg)
	//*)
	EVT_ACTIVATE(BurnDlg::OnActivate)
    EVT_BUTTON(wxID_OK, BurnDlg::OnOk)
    EVT_BUTTON(wxID_APPLY, BurnDlg::OnReset)
END_EVENT_TABLE()

#if defined(HAVE_LIBUDEV) || defined(__WXMSW__) || defined(__WXMAC__)
    const int DEVICE_READONLY = wxCB_READONLY;
#else
	const int DEVICE_READONLY = 0;
#endif

BurnDlg::BurnDlg(wxWindow* parent,DVD* dvd, Cache* cache) {
	m_dvd = dvd;
	m_cache = cache;

	//(*Initialize(BurnDlg)
	wxBoxSizer* tempDirSizer;
	wxStaticText* requiredSpaceTitle;
	wxStaticText* freeSpaceTitle;
	wxBoxSizer* panelSizer;
	wxBoxSizer* deviceSizer;
	wxBoxSizer* outputFreeSpaceSizer;
	wxFlexGridSizer* outputGrid;
	wxFlexGridSizer* tempDirGrid;
	wxStdDialogButtonSizer* stdDialogButtonSizer;
	wxBoxSizer* freeSpaceSizer;
	wxBoxSizer* previewSizer;
	wxBoxSizer* isoSizer;
	wxFlexGridSizer* isoGrid;
	wxBoxSizer* outputSizer;
	wxFlexGridSizer* burnFlexGridSizer1;
	wxBoxSizer* isoFreeSpaceSizer;
	wxBoxSizer* mainSizer;

	Create(parent, wxID_ANY, _("Burn"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER, _T("wxID_ANY"));
	mainSizer = new wxBoxSizer(wxVERTICAL);
	panelSizer = new wxBoxSizer(wxVERTICAL);
	tempDirGrid = new wxFlexGridSizer(0, 2, 0, 0);
	tempDirGrid->AddGrowableCol(1);
	tempDirLabel = new wxStaticText(this, ID_STATICTEXT1, _("Temp directory:"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT1"));
	tempDirGrid->Add(tempDirLabel, 0, wxRIGHT|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 4);
	tempDirSizer = new wxBoxSizer(wxHORIZONTAL);
	m_tempDirText = new wxTextCtrl(this, TEMP_DIR_FIELD_ID, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("TEMP_DIR_FIELD_ID"));
	tempDirSizer->Add(m_tempDirText, 1, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 2);
	m_tempDirBt = new wxButton(this, TEMP_DIR_BT_ID, _("..."), wxDefaultPosition, wxSize(21,21), 0, wxDefaultValidator, _T("TEMP_DIR_BT_ID"));
	tempDirSizer->Add(m_tempDirBt, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	tempDirGrid->Add(tempDirSizer, 1, wxTOP|wxBOTTOM|wxEXPAND, 4);
	tempDirGrid->Add(16,16,0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	freeSpaceSizer = new wxBoxSizer(wxHORIZONTAL);
	freeSpaceTitle = new wxStaticText(this, wxID_ANY, _("Free:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	freeSpaceSizer->Add(freeSpaceTitle, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	m_freeSpaceText = new wxStaticText(this, ID_STATICTEXT3, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT3"));
	freeSpaceSizer->Add(m_freeSpaceText, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 16);
	requiredSpaceTitle = new wxStaticText(this, wxID_ANY, _("Required:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	freeSpaceSizer->Add(requiredSpaceTitle, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	m_requiredSpaceText = new wxStaticText(this, ID_STATICTEXT4, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT4"));
	freeSpaceSizer->Add(m_requiredSpaceText, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	tempDirGrid->Add(freeSpaceSizer, 0, wxEXPAND, 0);
	panelSizer->Add(tempDirGrid, 0, wxEXPAND, 0);
	m_previewCheck = new wxCheckBox(this, PREVIEW_CHECK_ID, _("preview"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("PREVIEW_CHECK_ID"));
	m_previewCheck->SetValue(false);
	panelSizer->Add(m_previewCheck, 0, wxTOP|wxBOTTOM|wxALIGN_LEFT, 4);
	previewSizer = new wxBoxSizer(wxHORIZONTAL);
	previewSizer->Add(16,16,0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	m_previewChoice = new wxChoice(this, ID_PREVIEW_CHOICE, wxDefaultPosition, wxDefaultSize, 0, 0, 0, wxDefaultValidator, _T("ID_PREVIEW_CHOICE"));
	previewSizer->Add(m_previewChoice, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 2);
	m_previewTextCtrl = new wxTextCtrl(this, ID_TEXTCTRL2, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_TEXTCTRL2"));
	previewSizer->Add(m_previewTextCtrl, 1, wxRIGHT|wxEXPAND, 2);
	m_previewBt = new wxButton(this, ID_PREVIEW_BT, _("..."), wxDefaultPosition, wxSize(21,21), 0, wxDefaultValidator, _T("ID_PREVIEW_BT"));
	previewSizer->Add(m_previewBt, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	m_okBitmap = new wxStaticBitmap(this, wxID_ANY, wxBITMAP_FROM_MEMORY(ok), wxDefaultPosition, wxDefaultSize, wxNO_BORDER, _T("wxID_ANY"));
	m_okBitmap->Hide();
	previewSizer->Add(m_okBitmap, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	m_errorBitmap = new wxStaticBitmap(this, wxID_ANY, wxBITMAP_FROM_MEMORY(error), wxDefaultPosition, wxDefaultSize, wxNO_BORDER, _T("wxID_ANY"));
	m_errorBitmap->Hide();
	previewSizer->Add(m_errorBitmap, 0, wxLEFT|wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	m_downloadLink = new wxHyperlinkCtrl(this, ID_HYPERLINKCTRL1, _("Download"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_CONTEXTMENU|wxHL_ALIGN_CENTRE|wxNO_BORDER, _T("ID_HYPERLINKCTRL1"));
	m_downloadLink->Hide();
	previewSizer->Add(m_downloadLink, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	panelSizer->Add(previewSizer, 0, wxTOP|wxBOTTOM|wxEXPAND, 4);
	m_generateRadioBt = new wxRadioButton(this, GENERATE_RADIO_BT_ID, _("just generate"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP, wxDefaultValidator, _T("GENERATE_RADIO_BT_ID"));
	panelSizer->Add(m_generateRadioBt, 0, wxTOP|wxBOTTOM|wxALIGN_LEFT, 4);
	outputGrid = new wxFlexGridSizer(0, 3, 0, 0);
	outputGrid->AddGrowableCol(2);
	outputGrid->Add(16,16,0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	m_outputLabel = new wxStaticText(this, wxID_ANY, _("Save to:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	outputGrid->Add(m_outputLabel, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	outputSizer = new wxBoxSizer(wxHORIZONTAL);
	m_outputDirText = new wxTextCtrl(this, ID_TEXTCTRL1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_TEXTCTRL1"));
	outputSizer->Add(m_outputDirText, 1, wxRIGHT|wxEXPAND, 2);
	m_outputDirBt = new wxButton(this, ID_OUTPUT_BT, _("..."), wxDefaultPosition, wxSize(21,21), 0, wxDefaultValidator, _T("ID_OUTPUT_BT"));
	outputSizer->Add(m_outputDirBt, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	outputGrid->Add(outputSizer, 1, wxTOP|wxBOTTOM|wxEXPAND, 4);
	outputGrid->Add(16,16,0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	outputGrid->Add(16,16,0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	outputFreeSpaceSizer = new wxBoxSizer(wxHORIZONTAL);
	m_outputFreeSpaceTitle = new wxStaticText(this, wxID_ANY, _("Free:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	outputFreeSpaceSizer->Add(m_outputFreeSpaceTitle, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	m_outputFreeSpaceText = new wxStaticText(this, ID_STATICTEXT6, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT6"));
	outputFreeSpaceSizer->Add(m_outputFreeSpaceText, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 16);
	m_outputRequiredSpaceTitle = new wxStaticText(this, wxID_ANY, _("Required:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	outputFreeSpaceSizer->Add(m_outputRequiredSpaceTitle, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	m_outputRequiredSpaceText = new wxStaticText(this, ID_STATICTEXT7, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT7"));
	outputFreeSpaceSizer->Add(m_outputRequiredSpaceText, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	outputGrid->Add(outputFreeSpaceSizer, 0, wxALL|wxEXPAND, 0);
	panelSizer->Add(outputGrid, 0, wxEXPAND, 0);
	m_isoRadioBt = new wxRadioButton(this, ISO_RADIO_BT_ID, _("create iso image"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ISO_RADIO_BT_ID"));
	panelSizer->Add(m_isoRadioBt, 0, wxTOP|wxBOTTOM|wxALIGN_LEFT, 4);
	isoGrid = new wxFlexGridSizer(0, 3, 0, 0);
	isoGrid->AddGrowableCol(2);
	isoGrid->Add(16,16,0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	m_isoLabel = new wxStaticText(this, wxID_ANY, _("Save to:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	isoGrid->Add(m_isoLabel, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	isoSizer = new wxBoxSizer(wxHORIZONTAL);
	m_isoText = new wxTextCtrl(this, ISO_FILE_FIELD_ID, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ISO_FILE_FIELD_ID"));
	isoSizer->Add(m_isoText, 1, wxRIGHT|wxEXPAND, 2);
	m_isoBt = new wxButton(this, ISO_BT_ID, _("..."), wxDefaultPosition, wxSize(21,21), 0, wxDefaultValidator, _T("ISO_BT_ID"));
	isoSizer->Add(m_isoBt, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	isoGrid->Add(isoSizer, 1, wxTOP|wxBOTTOM|wxEXPAND, 4);
	isoGrid->Add(16,16,0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	isoGrid->Add(16,16,0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	isoFreeSpaceSizer = new wxBoxSizer(wxHORIZONTAL);
	m_isoFreeSpaceTitle = new wxStaticText(this, wxID_ANY, _("Free:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	isoFreeSpaceSizer->Add(m_isoFreeSpaceTitle, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	m_isoFreeSpaceText = new wxStaticText(this, ID_STATICTEXT2, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT2"));
	isoFreeSpaceSizer->Add(m_isoFreeSpaceText, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 16);
	m_isoRequiredSpaceTitle = new wxStaticText(this, wxID_ANY, _("Required:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	isoFreeSpaceSizer->Add(m_isoRequiredSpaceTitle, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	m_isoRequiredSpaceText = new wxStaticText(this, ID_STATICTEXT5, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, _T("ID_STATICTEXT5"));
	isoFreeSpaceSizer->Add(m_isoRequiredSpaceText, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	isoGrid->Add(isoFreeSpaceSizer, 0, wxALL|wxEXPAND, 0);
	panelSizer->Add(isoGrid, 0, wxEXPAND, 6);
	m_burnRadioBt = new wxRadioButton(this, BURN_RADIO_BT_ID, _("burn"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("BURN_RADIO_BT_ID"));
	panelSizer->Add(m_burnRadioBt, 0, wxTOP|wxBOTTOM|wxALIGN_LEFT, 4);
	burnFlexGridSizer1 = new wxFlexGridSizer(0, 2, 0, 0);
	burnFlexGridSizer1->AddGrowableCol(1);
	burnFlexGridSizer1->Add(16,16,0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	m_formatCheck = new wxCheckBox(this, ID_CHECKBOX1, _("format DVD-RW"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX1"));
	m_formatCheck->SetValue(false);
	burnFlexGridSizer1->Add(m_formatCheck, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);
	burnFlexGridSizer1->Add(16,16,0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	deviceSizer = new wxBoxSizer(wxHORIZONTAL);
	m_deviceLabel = new wxStaticText(this, wxID_ANY, _("Device:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	deviceSizer->Add(m_deviceLabel, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 4);
	m_deviceChoice = new wxComboBox(this, ID_COMBOBOX1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, 0, wxCB_DROPDOWN, wxDefaultValidator, _T("ID_COMBOBOX1"));
	deviceSizer->Add(m_deviceChoice, 0, wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 12);
	m_speedLabel = new wxStaticText(this, wxID_ANY, _("Speed:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	deviceSizer->Add(m_speedLabel, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	m_speedChoice = new wxComboBox(this, ID_COMBOBOX2, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, 0, wxCB_DROPDOWN, wxDefaultValidator, _T("ID_COMBOBOX2"));
	deviceSizer->Add(m_speedChoice, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	burnFlexGridSizer1->Add(deviceSizer, 0, wxEXPAND, 5);
	panelSizer->Add(burnFlexGridSizer1, 0, wxALL|wxEXPAND, 0);
	m_addECCCheck = new wxCheckBox(this, ID_CHECKBOX2, _("add error correction data"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX2"));
	m_addECCCheck->SetValue(false);
	panelSizer->Add(m_addECCCheck, 0, wxTOP|wxALIGN_LEFT, 4);
	mainSizer->Add(panelSizer, 1, wxALL|wxEXPAND, 6);
	stdDialogButtonSizer = new wxStdDialogButtonSizer();
	stdDialogButtonSizer->AddButton(new wxButton(this, wxID_OK, _("Start")));
	stdDialogButtonSizer->AddButton(new wxButton(this, wxID_CANCEL, wxEmptyString));
	stdDialogButtonSizer->AddButton(new wxButton(this, wxID_APPLY, _("Reset")));
	stdDialogButtonSizer->Realize();
	mainSizer->Add(stdDialogButtonSizer, 0, wxALL|wxEXPAND, 6);
	SetSizer(mainSizer);
	mainSizer->Fit(this);
	mainSizer->SetSizeHints(this);
	Center();

	Connect(TEMP_DIR_FIELD_ID,wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&BurnDlg::OnTempDirChange);
	Connect(TEMP_DIR_BT_ID,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&BurnDlg::OnChooseTempDir);
	Connect(PREVIEW_CHECK_ID,wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&BurnDlg::OnPreviewCheck);
	Connect(ID_PREVIEW_CHOICE,wxEVT_COMMAND_CHOICE_SELECTED,(wxObjectEventFunction)&BurnDlg::OnPreviewCheck);
	Connect(ID_TEXTCTRL2,wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&BurnDlg::OnTempDirChange);
	Connect(ID_PREVIEW_BT,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&BurnDlg::OnChoosePreviewCmd);
	Connect(GENERATE_RADIO_BT_ID,wxEVT_COMMAND_RADIOBUTTON_SELECTED,(wxObjectEventFunction)&BurnDlg::OnRadioBt);
	Connect(ID_TEXTCTRL1,wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&BurnDlg::OnTempDirChange);
	Connect(ID_OUTPUT_BT,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&BurnDlg::OnChooseOutputDir);
	Connect(ISO_RADIO_BT_ID,wxEVT_COMMAND_RADIOBUTTON_SELECTED,(wxObjectEventFunction)&BurnDlg::OnRadioBt);
	Connect(ISO_FILE_FIELD_ID,wxEVT_COMMAND_TEXT_UPDATED,(wxObjectEventFunction)&BurnDlg::OnTempDirChange);
	Connect(ISO_BT_ID,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&BurnDlg::OnChooseIsoFile);
	Connect(BURN_RADIO_BT_ID,wxEVT_COMMAND_RADIOBUTTON_SELECTED,(wxObjectEventFunction)&BurnDlg::OnRadioBt);
	Connect(wxEVT_SET_FOCUS,(wxObjectEventFunction)&BurnDlg::OnWindowFocus);
	//*)
	stdDialogButtonSizer->GetAffirmativeButton()->SetDefault();

	m_tempDirText->SetFocus();

	int h = m_tempDirText->GetSize().GetHeight() > 21 ? m_tempDirText->GetSize().GetHeight() : 21;
	m_tempDirBt->SetSizeHints(h, h, h, h);
	m_previewBt->SetSizeHints(h, h, h, h);
	m_outputDirBt->SetSizeHints(h, h, h, h);
	m_isoBt->SetSizeHints(h, h, h, h);

	// set devices
	EnumDevices(m_devices, m_deviceChoice);

	m_speedChoice->Append(_T("auto"));
	for (int speed = 1; speed <= 8; speed = speed * 2)
		m_speedChoice->Append(wxString::Format(_T("%dx"), speed));

	// check if dvdisaster is installed
	wxString cmd = s_config.GetAddECCCmd();
	if (cmd.length() > 0)
		cmd = cmd[0] == wxT('"') ? cmd.Mid(0, cmd.Mid(1).Find(wxT('"')) + 2) : cmd.BeforeFirst(wxT(' '));
	m_eccCheckEnabled = false;
	if (cmd.length() > 0) {
		wxLogNull log;
		wxArrayString output;
		if (wxExecute(cmd + wxT(" --version"), output, wxEXEC_SYNC | wxEXEC_NODISABLE) == 0)
			m_eccCheckEnabled = true;
	}

	wxString tmpDir = s_config.GetTempDir();
	if (wxDirExists(tmpDir))
		m_cache->SetTempDir(tmpDir);

	UpdateCtrls();
}

BurnDlg::~BurnDlg() {
	//(*Destroy(BurnDlg)
	//*)
}

wxString BurnDlg::GetDevice() {
	if (m_devices.Count() && m_deviceChoice->GetSelection() >= 0)
		return m_devices[m_deviceChoice->GetSelection()];
	else
		return m_deviceChoice->GetValue();
}

void BurnDlg::SetDevice(wxString device) {
	if (m_devices.Count()) {
		int n = device.length() > 0 ? m_devices.Index(device) : 0;
		if (n >= 0) {
			m_deviceChoice->SetSelection(n);
		} else {
#ifdef HAVE_LIBUDEV
			m_deviceChoice->SetSelection(0);
#else
			m_deviceChoice->SetValue(device);
#endif
		}
	} else
		m_deviceChoice->SetValue(device);
}

int BurnDlg::GetSpeed() {
	long speed = 0;
	if (m_speedChoice->GetSelection() > 0)
		m_speedChoice->GetStringSelection().BeforeFirst(wxT('x')).ToLong(&speed);
	return speed;
}

void BurnDlg::SetSpeed(int speed) {
	if (speed == 0)
		m_speedChoice->SetSelection(0);
	else
		m_speedChoice->SetStringSelection(wxString::Format(_T("%dx"), speed));
}

void BurnDlg::UpdateCtrls(bool def) {
	m_previewChoice->Clear();
	bool win8 = false;
#ifdef __WXMSW__
	win8 = wxPlatformInfo::Get().CheckOSVersion(6, 2);
	if (!win8) {
		m_previewChoice->Append(wxT("Windows Media Player"));
	}
#elif !defined(__WXMAC__)
	m_previewChoice->Append(wxT("Xine Media Player"));
#endif
	m_previewChoice->Append(_("VLC media player"));
#ifdef __WXMSW__
	m_previewChoice->Append(_("MPC-HC player"));
#endif
	m_previewChoice->Append(_("Custom video player"));

	wxString vlcPath = GetVlcPath();
	wxString mpcHcPath = GetMpcHcPath();
	if (s_config.GetPreviewCmd(def) == s_config.GetPreviewCmd(true)
			|| (win8 && s_config.GetPreviewCmd() == wxT("wmplayer"))) {
		m_previewChoice->SetSelection(0);
	} else if (vlcPath.length() && vlcPath == s_config.GetPreviewCmd()) {
		m_previewChoice->SetSelection(win8 ? 0 : 1);
	} else if (mpcHcPath.length() && mpcHcPath == s_config.GetPreviewCmd()) {
		m_previewChoice->SetSelection(win8 ? 1 : 2);
	} else {
		m_previewChoice->SetSelection(m_previewChoice->GetCount() - 1);
		m_previewTextCtrl->SetValue(s_config.GetPreviewCmd());
	}

	wxString outName = m_dvd->GetLabel();
	if (outName == wxT("DVD")) // default label: use a generic name
		outName = wxT("dvd");
	if (m_dvd->GetFilename().length())
		wxFileName::SplitPath(m_dvd->GetFilename(), NULL, &outName, NULL);

	m_tempDirText->SetValue(s_config.GetTempDir(def));
	m_outputDirText->SetValue(m_dvd->GetOutputDir().length() > 0 ? m_dvd->GetOutputDir()
			: s_config.GetOutputDir(def) + wxFILE_SEP_PATH + outName);
	UpdateSpaceLabels();
	m_previewCheck->SetValue(s_config.GetPreviewDo(def));
	m_generateRadioBt->SetValue(s_config.GetGenerateDo(def));
	m_isoRadioBt->SetValue(s_config.GetIsoDo(def));
	m_isoText->SetValue(m_dvd->GetIsoFile().length() > 0 ? m_dvd->GetIsoFile()
			: wxPathOnly(s_config.GetIsoSaveTo(def)) + wxFILE_SEP_PATH + outName + wxT(".iso"));
	m_addECCCheck->SetValue(s_config.GetAddECCDo(def));
	m_burnRadioBt->SetValue(s_config.GetBurnDo(def));
	m_formatCheck->SetValue(s_config.GetFormatDo(def));
	SetDevice(s_config.GetBurnDevice(def));
	SetSpeed(s_config.GetBurnSpeed(def));
#if defined(__WXMSW__) || defined(__WXMAC__)
	bool hasDevices = m_deviceChoice->GetCount() > 0;
#else
	bool hasDevices = true;
#endif
	if (!hasDevices) {
 		if (m_burnRadioBt->GetValue())
 			m_isoRadioBt->SetValue(true);
 		m_burnRadioBt->Enable(false);
	}
	wxCommandEvent evt;
	OnPreviewCheck(evt);
	OnRadioBt(evt);
}

double CalcFreeSpace(wxString dir, wxStaticText* textCtrl = NULL, double requiredSpace = 0) {
	double freeSpace = 0;
	wxString text = _("N/A");
	if (wxDirExists(dir)) {
		wxDiskspaceSize_t pFree;
		wxGetDiskSpace(dir, NULL, &pFree);
		freeSpace = pFree.ToDouble() / 1024 / 1024 / 1024;
		text = wxString::Format(wxT("%.1f "), freeSpace) + _("GB");
	}
	if (textCtrl != NULL) {
		textCtrl->SetLabel(text);
		textCtrl->SetForegroundColour(freeSpace > requiredSpace ? wxColour(0, 128, 0) : *wxRED);
	}
	return freeSpace;
}

bool BurnDlg::UpdateSpaceLabels(bool showErrors) {
	wxString tempDir = m_tempDirText->GetValue();
	if (!wxDirExists(tempDir)) // get parent dir
		tempDir = wxFileName(tempDir).GetPath();
	wxString outputDir = wxFileName(m_outputDirText->GetValue()).GetPath();
	if (!wxDirExists(outputDir)) // get parent dir
		outputDir = wxFileName(outputDir).GetPath();
	wxString isoDir = wxFileName(m_isoText->GetValue()).GetPath();

	// required space
	double requiredSpace = ((double)m_dvd->GetRequiredSize(m_cache))/1024/1024;
	double outputRequiredSpace = ((double)m_dvd->GetSize())/1024/1024;
	if (DoGenerate()) {
		// check if output and temporary directory are on the same disk/partition
		double freeSpace = CalcFreeSpace(tempDir);
		double outputFreeSpace = CalcFreeSpace(outputDir);
		if (freeSpace != 0 && outputFreeSpace != 0 && freeSpace != outputFreeSpace) {
			requiredSpace -= outputRequiredSpace;
		}
	}
	if (requiredSpace < 0.1)
		requiredSpace = 0.1;
	if (outputRequiredSpace < 0.1)
		outputRequiredSpace = 0.1;

	// output required space and required space for ISO image
	m_requiredSpaceText->SetLabel(wxString::Format(wxT("%.1f "), requiredSpace) + _("GB"));
	m_outputRequiredSpaceText->SetLabel(wxString::Format(wxT("%.1f "), outputRequiredSpace) + _("GB"));
	m_isoRequiredSpaceText->SetLabel(wxString::Format(wxT("%.1f "), outputRequiredSpace) + _("GB"));

	// free space
	double freeSpace = CalcFreeSpace(tempDir, m_freeSpaceText, requiredSpace);
	//double outputFreeSpace =
	CalcFreeSpace(outputDir, m_outputFreeSpaceText, outputRequiredSpace);
	double isoFreeSpace = CalcFreeSpace(isoDir, m_isoFreeSpaceText, outputRequiredSpace);;

	Layout();

	bool hasFreeSpace = freeSpace > requiredSpace;
	if (DoCreateIso()) {
		// check if there is enough space for ISO
		if (freeSpace == isoFreeSpace) { // ISO will be saved on the same disk/partition
			// cache can be deleted after generation of dvd structure => max space = dvd * 2 or cache + dvd
			double totalRequired = requiredSpace > outputRequiredSpace * 2 ? requiredSpace : outputRequiredSpace * 2;
			m_freeSpaceText->SetForegroundColour(freeSpace > totalRequired ? wxColour(0, 128, 0) : *wxRED);
			m_isoFreeSpaceText->SetForegroundColour(freeSpace > totalRequired ? wxColour(0, 128, 0) : *wxRED);
			hasFreeSpace = freeSpace > totalRequired;
		} else { // ISO will be saved on another disk/partition
			if (showErrors && hasFreeSpace && isoFreeSpace <= outputRequiredSpace) {
				wxMessageBox(_("There is not enough space to store ISO file."), _("Burn"),
						wxOK|wxICON_ERROR, this);
				return false;
			}
		}
	}
	if (showErrors && !hasFreeSpace)
		wxMessageBox(_("There is not enough space on temporary directory."), _("Burn"),
				wxOK|wxICON_ERROR, this);
	return hasFreeSpace;
}

wxString BurnDlg::GetVlcPath() {
#ifdef __WXMSW__
	wxRegKey key(wxRegKey::HKLM, wxT("SOFTWARE\\Classes\\DVD\\shell\\PlayWithVLC\\command"));
	if (key.Exists()) {
		return key.QueryDefaultValue();
	}
	return wxT("");
#elif defined(__WXMAC__)
	return wxT("/Applications/VLC.app");
#else
	return wxT("vlc");
#endif
}

wxString BurnDlg::GetMpcHcPath() {
#ifdef __WXMSW__
	wxString val;
	wxRegKey key(wxRegKey::HKCU, wxT("SOFTWARE\\MPC-HC"));
	if (key.Exists() && key.HasValue(wxT("ExePath")) && key.QueryValue(wxT("ExePath"), val)) {
		return val;
	}
	wxRegKey key2(wxRegKey::HKCU, wxT("SOFTWARE\\MPC-HC\\MPC-HC"));
	if (key2.Exists() && key2.HasValue(wxT("ExePath")) && key2.QueryValue(wxT("ExePath"), val)) {
		return val;
	}
	return wxT("");
#else
	return wxT("");
#endif
}

void BurnDlg::OnTempDirChange(wxCommandEvent& event) {
	UpdateSpaceLabels();
}

void BurnDlg::OnChooseTempDir(wxCommandEvent& event) {
	wxDirDialog dlg(this, _("Choose a directory"), GetTempDir(), wxDD_NEW_DIR_BUTTON);
	if (dlg.ShowModal() != wxID_OK)
		return;
	wxString path = dlg.GetPath();
	if (!path.EndsWith(wxString(wxFILE_SEP_PATH)))
		path += wxFILE_SEP_PATH;
	m_tempDirText->SetValue(path);
}

void BurnDlg::OnChooseOutputDir(wxCommandEvent& event) {
	wxDirDialog dlg(this, _("Choose a directory"), GetOutputDir(), wxDD_NEW_DIR_BUTTON);
	if (dlg.ShowModal() != wxID_OK)
		return;
	wxString path = dlg.GetPath();
	if (!path.EndsWith(wxString(wxFILE_SEP_PATH)))
		path += wxFILE_SEP_PATH;
	if (wxDirExists(path)) {
		wxDir dir(path);
		if (dir.HasFiles() || (dir.HasSubDirs() && !wxDirExists(path + wxFILE_SEP_PATH + wxT("VIDEO_TS")))) {
			wxString outName = wxT("dvd");
			if (m_dvd->GetFilename().length())
				wxFileName::SplitPath(m_dvd->GetFilename(), NULL, &outName, NULL);
			path += outName + wxFILE_SEP_PATH;
		}
	}
	m_outputDirText->SetValue(path);
}

void BurnDlg::OnChoosePreviewCmd(wxCommandEvent& event) {
	wxFileDialog dialog(this, _("Choose a file"), m_previewTextCtrl->GetValue(), wxT(""),
			wxString(_("Programs")) + APP_EXT + wxT("|")
			+ wxString(_("All Files")) + wxT(" (*.*)|*.*"), wxFD_DEFAULT_STYLE);
	dialog.SetPath(m_previewTextCtrl->GetValue());
	if (dialog.ShowModal() == wxID_OK)
		m_previewTextCtrl->SetValue(dialog.GetPath());
}

void BurnDlg::OnRadioBt(wxCommandEvent& event) {
	UpdateSpaceLabels();
	m_outputLabel->Enable(DoGenerate());
	m_outputDirText->Enable(DoGenerate());
	m_outputDirBt->Enable(DoGenerate());
	m_outputFreeSpaceTitle->Enable(DoGenerate());
	m_outputFreeSpaceText->Enable(DoGenerate());
	m_outputRequiredSpaceTitle->Enable(DoGenerate());
	m_outputRequiredSpaceText->Enable(DoGenerate());
	m_isoLabel->Enable(DoCreateIso());
	m_isoText->Enable(DoCreateIso());
	m_isoBt->Enable(DoCreateIso());
	m_isoFreeSpaceTitle->Enable(DoCreateIso());
	m_isoFreeSpaceText->Enable(DoCreateIso());
	m_isoRequiredSpaceTitle->Enable(DoCreateIso());
	m_isoRequiredSpaceText->Enable(DoCreateIso());
	m_formatCheck->Enable(DoBurn());
	m_deviceLabel->Enable(DoBurn());
	m_deviceChoice->Enable(DoBurn());
	m_speedLabel->Enable(DoBurn());
	m_speedChoice->Enable(DoBurn());
	m_addECCCheck->Enable(m_eccCheckEnabled && (DoCreateIso() || DoBurn()));
}

void BurnDlg::OnChooseIsoFile(wxCommandEvent& event) {
	wxFileDialog dlg(this, _("Choose a file to save iso image"),
			wxPathOnly(GetIsoFile()), _T("dvd.iso"), _T("*.iso"), wxFD_SAVE);
	if (dlg.ShowModal() != wxID_OK)
		return;
	m_isoText->SetValue(dlg.GetPath());
}

void BurnDlg::OnActivate(wxActivateEvent &event) {
	UpdateSpaceLabels();
}

void BurnDlg::OnReset(wxCommandEvent& event) {
	UpdateCtrls(true);
}

void BurnDlg::OnOk(wxCommandEvent& event) {
	if (m_generateRadioBt->GetValue()) {
		wxString outputDir = m_outputDirText->GetValue().Trim().Trim(false);
		if (outputDir.length() == 0) {
			wxMessageBox(_("Please enter output directory"), _("Burn"), wxOK|wxICON_ERROR, this);
			return;
		}
		if (wxDirExists(outputDir)) {
			wxDir dir(outputDir);
			if (dir.HasFiles() || (dir.HasSubDirs() && !wxDirExists(outputDir + wxFILE_SEP_PATH + wxT("VIDEO_TS")))) {
				wxMessageBox(_("Output directory contains files. Please choose another output directory."),
						_("Burn"), wxOK|wxICON_ERROR, this);
				return;
			}
			if (s_config.GetOverwriteOutputDirPrompt()) {
				MessageDlg confirmDlg(this, wxString::Format(
						_("Directory '%s' already exist. Do you want to overwrite it?"), outputDir.c_str()),
						_("Burn"), wxYES_NO|wxICON_QUESTION);
				if (confirmDlg.ShowModal() == wxNO)
					return;
				s_config.SetOverwriteOutputDirPrompt(confirmDlg.IsShowAgain());
			}
		}
		if (!wxDirExists(outputDir) && !wxMkdir(outputDir)) {
			wxMessageBox(wxString::Format(_("Can't create directory '%s'"), outputDir.c_str()), _("Burn"),
					wxOK|wxICON_ERROR, this);
			return;
		}
	}
	if (m_previewCheck->GetValue()) {
		if (m_errorBitmap->IsShown()) {
			wxMessageBox(_("Please select a video player"), _("Burn"), wxOK|wxICON_ERROR, this);
			return;
		}
		if (m_previewChoice->GetSelection() == (int) m_previewChoice->GetCount() - 1
				&& m_previewTextCtrl->GetValue().length() == 0) {
			wxMessageBox(_("Please select path to video player"), _("Burn"), wxOK|wxICON_ERROR, this);
			return;
		}
	}
	if (m_isoRadioBt->GetValue()) {
		wxString isoFile = GetIsoFile().Trim().Trim(false);
		if (isoFile.length() == 0) {
			wxMessageBox(_("Please enter file name to save ISO"), _("Burn"), wxOK|wxICON_ERROR, this);
			return;
		}
		if (wxFileExists(isoFile) && wxMessageBox(wxString::Format(
				_("File '%s' already exist. Do you want to overwrite it?"), isoFile.c_str()),
				_("Burn"), wxYES_NO|wxICON_QUESTION, this) == wxNO)
			return;
	}
	if (m_burnRadioBt->GetValue()) {
		wxString device = GetDevice().Trim().Trim(false);
		if (device.length() == 0) {
			wxMessageBox(_("Please enter device name"), _("Burn"), wxOK|wxICON_ERROR, this);
			return;
		}
#ifdef __WXMSW__
		if (device.Find(wxT(':')) != 1 || device.length()> 3
				|| (device.length() == 3 && device.GetChar(2) != wxT('\\'))) {
			wxMessageBox(_("Invalid device name"), _("Burn"), wxOK|wxICON_ERROR, this);
			return;
		}
		SetDevice(device.SubString(0,1).MakeUpper());
#endif
	}
	if (!UpdateSpaceLabels(true)) {
		return;
	}
	wxString tmpDir = m_tempDirText->GetValue();
	if (m_cache->IsInitialized() && m_cache->GetTempDir() != tmpDir && m_cache->GetCount() > 0
			&& wxMessageBox(wxString::Format(
					_("There are %d transcoded file(s) in the cache. They\nwill be removed if you change temporary directory."), m_cache->GetCount()),
					_("Burn"), wxOK|wxCANCEL|wxICON_INFORMATION, this) == wxCANCEL)
		return;
	if (!wxDirExists(tmpDir) && !wxMkdir(tmpDir)) {
		wxMessageBox(wxString::Format(_("Can't create directory '%s'"), tmpDir.c_str()), _("Burn"),
				wxOK|wxICON_ERROR, this);
		return;
	}
	if (!m_cache->SetTempDir(tmpDir))
		return;
	s_config.SetTempDir(tmpDir);
	if (m_previewCheck->IsChecked()) {
		bool win8 = false;
#ifdef __WXMSW__
		win8 = wxPlatformInfo::Get().CheckOSVersion(6, 2);
#endif
		if (m_previewChoice->GetSelection() == 0 && !win8) {
			s_config.SetPreviewCmd(s_config.GetPreviewCmd(true));
		} else if (m_previewChoice->GetSelection() == (int) m_previewChoice->GetCount() - 1) {
			s_config.SetPreviewCmd(m_previewTextCtrl->GetValue());
#ifdef __WXMSW__
		} else if (m_previewChoice->GetSelection() == (int) m_previewChoice->GetCount() - 2) {
			s_config.SetPreviewCmd(GetMpcHcPath());
#endif
		} else {
			s_config.SetPreviewCmd(GetVlcPath());
		}
	}
	if (m_generateRadioBt->GetValue()) {
		wxString outputDir = m_outputDirText->GetValue().Trim().Trim(false);
		m_dvd->SetOutputDir(outputDir);
		if (outputDir.length() && outputDir.Last() == wxFILE_SEP_PATH)
			outputDir = outputDir.substr(0, outputDir.length() - 1);
		s_config.SetOutputDir(wxPathOnly(outputDir));
	}
	s_config.SetPreviewDo(m_previewCheck->GetValue());
	s_config.SetGenerateDo(m_generateRadioBt->GetValue());
	s_config.SetIsoDo(m_isoRadioBt->GetValue());
	if (m_isoRadioBt->GetValue()) {
		m_dvd->SetIsoFile(GetIsoFile().Trim().Trim(false));
		s_config.SetIsoSaveTo(wxPathOnly(GetIsoFile()) + wxFILE_SEP_PATH);
	}
	s_config.SetAddECCDo(m_addECCCheck->GetValue());
	s_config.SetBurnDo(m_burnRadioBt->GetValue());
	s_config.SetFormatDo(m_formatCheck->GetValue());
	s_config.SetBurnDevice(GetDevice());
	s_config.SetBurnSpeed(GetSpeed());
	s_config.Flush();
	EndModal(wxID_OK);
}

void BurnDlg::OnPreviewCheck(wxCommandEvent& event) {
	m_previewChoice->Enable(m_previewCheck->IsChecked());
	bool custom = m_previewCheck->IsChecked()
			&& m_previewChoice->GetSelection() == (int) m_previewChoice->GetCount() - 1;
	m_previewTextCtrl->Show(custom);
	m_previewBt->Show(custom);
	m_downloadLink->SetURL(wxT("https://www.videolan.org/"));
#if defined(__WXMSW__)
	bool playerSelected = false;
	bool playerInstalled = false;
	if (m_previewCheck->IsChecked()) {
		bool win8 = wxPlatformInfo::Get().CheckOSVersion(6, 2);
		if (m_previewChoice->GetSelection() == (win8 ? 0 : 1)) {
			playerSelected = true;
			playerInstalled = GetVlcPath().length() > 0;
		} else if (m_previewChoice->GetSelection() == (win8 ? 1 : 2)) {
			playerSelected = true;
			playerInstalled = GetMpcHcPath().length() > 0;
			m_downloadLink->SetURL(wxT("https://mpc-hc.org/"));
		}
	}
#elif defined(__WXMAC__)
	bool playerSelected = m_previewCheck->IsChecked() && m_previewChoice->GetSelection() == 0;
	bool playerInstalled = wxFileExists(GetVlcPath()) || wxDirExists(GetVlcPath());
#else
	bool playerSelected = m_previewCheck->IsChecked() && m_previewChoice->GetSelection() == 1;
	bool playerInstalled = playerSelected && wxExecute(GetVlcPath() + wxT(" --version"), wxEXEC_SYNC) == 0;
#endif
	m_okBitmap->Show(playerSelected && playerInstalled);
	m_errorBitmap->Show(playerSelected && !playerInstalled);
	m_downloadLink->Show(playerSelected && !playerInstalled);
	Layout();
}

void BurnDlg::OnWindowFocus(wxFocusEvent& event) {
    wxCommandEvent evt;
    OnPreviewCheck(evt);
}
