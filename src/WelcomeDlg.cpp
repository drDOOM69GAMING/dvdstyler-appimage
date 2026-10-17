/////////////////////////////////////////////////////////////////////////////
// Name:        NewProjectDlg.cpp
// Purpose:     New project dialog
// Author:      Alex Thuering
// Created:     23.03.2009
// RCS-ID:      $Id: WelcomeDlg.cpp,v 1.6 2012/09/02 14:49:25 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "WelcomeDlg.h"
#include "Config.h"
#include <wxVillaLib/utils.h>
#include <wx/artprov.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/docview.h>

#ifdef __WXMSW__
#include "rc/new.png.h"
#include "rc/open.png.h"
#define ICON_NEW wxBITMAP_FROM_MEMORY(new)
#define ICON_OPEN wxBITMAP_FROM_MEMORY(open)
#else
#define ICON_NEW wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR)
#define ICON_OPEN wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR)
#endif

enum {
	NEW_PROJECT_BT_ID = 2000,
	OPEN_PROJECT_BT_ID
};

BEGIN_EVENT_TABLE(WelcomeDlg, NewProjectDlg)
	EVT_RADIOBUTTON(NEW_PROJECT_BT_ID, WelcomeDlg::OnChooseNew)
	EVT_RADIOBUTTON(OPEN_PROJECT_BT_ID, WelcomeDlg::OnChooseOpen)
    EVT_CHILD_FOCUS(WelcomeDlg::OnSetFocus)
    EVT_LISTBOX_DCLICK(wxID_ANY, WelcomeDlg::OnListBoxDClick)
END_EVENT_TABLE()

WelcomeDlg::WelcomeDlg(wxWindow *parent) : NewProjectDlg(parent, false) {
	m_newProjectBt = NULL;
	m_openProjectBt = NULL;
	m_listBox = NULL;
	Create(false, true);
	SetTitle(_("Welcome"));
	SetSize(500, -1);
}

void WelcomeDlg::CreatePropPanel(wxSizer* sizer) {
	// new project radio
	AddStaticLine(sizer, _("New project"));
	wxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(new wxStaticBitmap(this, wxID_ANY, ICON_NEW), 0, wxLEFT|wxRIGHT|wxTOP, 8);
	wxSizer* newPrjSizer = new wxBoxSizer(wxVERTICAL);
	newPrjSizer->Add(8, 8);
	AddRadioProp(newPrjSizer, _("Create a new project"), true, wxRB_GROUP, false, NEW_PROJECT_BT_ID);
	m_newProjectBt = (wxRadioButton*) GetLastControl();
	newPrjSizer->GetItem(newPrjSizer->GetChildren().GetCount()-1)->SetProportion(0);
	mainSizer->Add(newPrjSizer, 1, wxEXPAND);
	sizer->Add(mainSizer, 0, wxEXPAND);
	
	// open project radio
	AddStaticLine(sizer, _("Open project"));
	mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(new wxStaticBitmap(this, wxID_ANY, ICON_OPEN), 0, wxLEFT|wxRIGHT|wxTOP, 8);
	wxSizer* openPrjSizer = new wxBoxSizer(wxVERTICAL);
	openPrjSizer->Add(8, 8);
	AddRadioProp(openPrjSizer, _("Open an existing project file"), false, 0, false, OPEN_PROJECT_BT_ID);
	m_openProjectBt = (wxRadioButton*) GetLastControl();
	openPrjSizer->GetItem(openPrjSizer->GetChildren().GetCount()-1)->SetProportion(0);
	mainSizer->Add(openPrjSizer, 1, wxEXPAND);
	sizer->Add(mainSizer, 1, wxEXPAND);
	
	// new project content
	propIndex = GetLastControlIndex() + 1;
	CreateDVDPropPanel(newPrjSizer, NULL);
	
	// open project content
	m_listBox = new wxListBox(this,  wxID_ANY);
	m_listBox->SetMinSize(wxSize(-1, 200));
	m_listBox->Append(_("Browse files..."));
	m_listBox->SetSelection(-1);
	wxFileHistory history;
	history.Load(*s_config.GetConfigBase());
	for (int i=0; i<(int)history.GetCount(); i++)
		if (wxFileExists(history.GetHistoryFile(i)))
			m_listBox->Append(history.GetHistoryFile(i));
	openPrjSizer->Add(m_listBox, 1, wxEXPAND|wxTOP, 4);
	
	sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 8);
}

bool WelcomeDlg::SetValues() {
	if (!Validate())
		return false;
	PersistHdSettings();
	if (GetBool(GetLastControlIndex()))
		s_config.SetShowWelcomeDlg(false);
	return true;
}

void WelcomeDlg::OnChooseNew(wxCommandEvent& event) {
	m_listBox->SetSelection(-1);
}

void WelcomeDlg::OnChooseOpen(wxCommandEvent& event) {
	if (m_listBox->GetSelection() < 0)
		m_listBox->SetSelection(0);
}

void WelcomeDlg::OnSetFocus(wxChildFocusEvent& event) {
	if (event.GetWindow()->IsKindOf(CLASSINFO(wxListBox))) {
		m_openProjectBt->SetValue(true);
	} else if (event.GetWindow()->GetId() != OPEN_PROJECT_BT_ID
			&& !event.GetWindow()->IsKindOf(CLASSINFO(wxButton))
			&& !event.GetWindow()->IsKindOf(CLASSINFO(wxCheckBox))) {
		m_newProjectBt->SetValue(true);
		m_listBox->SetSelection(-1);
	}
}

void WelcomeDlg::OnListBoxDClick(wxCommandEvent& event) {
	EndModal(wxID_OK);
}

bool WelcomeDlg::IsNewProject() {
	return GetBool(0);
}

wxString WelcomeDlg::GetOpenFilename() {
	return m_listBox->GetSelection() > 0 ? m_listBox->GetStringSelection() : wxT("");
}
