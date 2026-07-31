/////////////////////////////////////////////////////////////////////////////
// Name:        TemplateDlg.cpp
// Purpose:     Select DVD menu template dialog
// Author:      Alex Thuering
// Created:     20.10.2009
// RCS-ID:      $Id: TemplateDlg.cpp,v 1.20 2016/06/05 12:21:33 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "TemplateDlg.h"
#include "wxVillaLib/utils.h"
#include "wxVillaLib/rc/loading.png.h"
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/dir.h>

#define TEMPLATES_DIR wxFindDataDirectory(_T("templates"))

//(*InternalHeaders(TemplateDlg)
#include <wx/font.h>
#include <wx/intl.h>
#include <wx/button.h>
#include <wx/string.h>
//*)

//(*IdInit(TemplateDlg)
const long TemplateDlg::ID_TEXTCTRL1 = wxNewId();
const long TemplateDlg::ID_RADIOBUTTON1 = wxNewId();
const long TemplateDlg::ID_RADIOBUTTON2 = wxNewId();
const long TemplateDlg::CATEGORY_LIST_BOX_ID = wxNewId();
const long TemplateDlg::THUMBNAILS_ID = wxNewId();
//*)

BEGIN_EVENT_TABLE(TemplateDlg,wxDialog)
	//(*EventTable(TemplateDlg)
	//*)
	EVT_THUMBNAILS_DCLICK(THUMBNAILS_ID, TemplateDlg::OnThumbDoubleClick)
END_EVENT_TABLE()

/** Constructor */
TemplateDlg::TemplateDlg(wxWindow* parent, AspectRatio aspectRatio) {
	//(*Initialize(TemplateDlg)
	wxBoxSizer* menuTypeSizer;
	wxBoxSizer* captionSizer;
	wxStaticLine* staticLine1;
	wxStdDialogButtonSizer* stdDialogButtonSizer;
	wxStaticText* categoryLabel;
	wxBoxSizer* templateSizer;
	wxStaticText* captionLabel;
	wxBoxSizer* categorySizer;
	wxBoxSizer* boxSizerVertical;
	wxBoxSizer* mainSizer;

	Create(parent, wxID_ANY, _("Select template for DVD menus"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER, _T("wxID_ANY"));
	SetClientSize(wxSize(696,430));
	mainSizer = new wxBoxSizer(wxVERTICAL);
	boxSizerVertical = new wxBoxSizer(wxVERTICAL);
	captionSizer = new wxBoxSizer(wxHORIZONTAL);
	captionLabel = new wxStaticText(this, wxID_ANY, _("Caption:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	captionSizer->Add(captionLabel, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 4);
	m_titleText = new wxTextCtrl(this, ID_TEXTCTRL1, _("Disc Title"), wxDefaultPosition, wxSize(400,-1), 0, wxDefaultValidator, _T("ID_TEXTCTRL1"));
	wxFont m_titleTextFont(10,wxFONTFAMILY_SWISS,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,wxEmptyString,wxFONTENCODING_DEFAULT);
	m_titleText->SetFont(m_titleTextFont);
	captionSizer->Add(m_titleText, 1, wxEXPAND, 0);
	boxSizerVertical->Add(captionSizer, 0, wxEXPAND, 5);
	menuTypeSizer = new wxBoxSizer(wxHORIZONTAL);
	m_titleMenu = new wxRadioButton(this, ID_RADIOBUTTON1, _("Title selection"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP, wxDefaultValidator, _T("ID_RADIOBUTTON1"));
	menuTypeSizer->Add(m_titleMenu, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	m_chapterMenu = new wxRadioButton(this, ID_RADIOBUTTON2, _("Chapter selection"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_RADIOBUTTON2"));
	menuTypeSizer->Add(m_chapterMenu, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	boxSizerVertical->Add(menuTypeSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 4);
	mainSizer->Add(boxSizerVertical, 0, wxTOP|wxLEFT|wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 8);
	templateSizer = new wxBoxSizer(wxHORIZONTAL);
	categorySizer = new wxBoxSizer(wxVERTICAL);
	categoryLabel = new wxStaticText(this, wxID_ANY, _("Category"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	categorySizer->Add(categoryLabel, 0, wxALL|wxEXPAND, 2);
	m_categoryListBox = new wxListBox(this, CATEGORY_LIST_BOX_ID, wxDefaultPosition, wxSize(180,-1), 0, 0, wxLB_SINGLE|wxLB_NEEDED_SB, wxDefaultValidator, _T("CATEGORY_LIST_BOX_ID"));
	m_categoryListBox->SetSelection( m_categoryListBox->Append(_("All")) );
	wxFont m_categoryListBoxFont(10,wxFONTFAMILY_SWISS,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,wxEmptyString,wxFONTENCODING_DEFAULT);
	m_categoryListBox->SetFont(m_categoryListBoxFont);
	categorySizer->Add(m_categoryListBox, 1, wxEXPAND, 0);
	templateSizer->Add(categorySizer, 0, wxRIGHT|wxEXPAND, 4);
	m_thumbnails = new wxThumbnails(this,THUMBNAILS_ID, wxBORDER_DOUBLE);
	templateSizer->Add(m_thumbnails, 1, wxEXPAND, 4);
	mainSizer->Add(templateSizer, 1, wxALL|wxEXPAND, 4);
	staticLine1 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxSize(10,-1), wxLI_HORIZONTAL, _T("wxID_ANY"));
	mainSizer->Add(staticLine1, 0, wxEXPAND, 0);
	stdDialogButtonSizer = new wxStdDialogButtonSizer();
	stdDialogButtonSizer->AddButton(new wxButton(this, wxID_OK, wxEmptyString));
	stdDialogButtonSizer->AddButton(new wxButton(this, wxID_CANCEL, _("&No menu")));
	stdDialogButtonSizer->Realize();
	mainSizer->Add(stdDialogButtonSizer, 0, wxALL|wxEXPAND, 5);
	SetSizer(mainSizer);
	SetSizer(mainSizer);
	Layout();
	Center();

	Connect(CATEGORY_LIST_BOX_ID,wxEVT_COMMAND_LISTBOX_SELECTED,(wxObjectEventFunction)&TemplateDlg::OnCategorySelect);
	//*)
    m_aspectRatio = aspectRatio;
    if (m_aspectRatio == ar16_9)
    	SetSize(wxSize(890, -1));
    m_thumbnails->SetCaption(_("Templates"));
    m_thumbnails->SetThumbImageSize(m_aspectRatio == ar16_9 ? 205 : 145, 54);
    
    // load categories
	wxString dir = wxFindFirstFile(TEMPLATES_DIR + wxT("*"), wxDIR);
	while (!dir.IsEmpty()) {
		wxString subDir = dir.AfterLast(wxFILE_SEP_PATH);
		if (subDir != wxT("CVS") && !subDir.StartsWith(wxT("."))) {
			wxString label;
			if (subDir == wxT("Christmas"))
				label = _("Christmas & New Year's Eve");
			else
				label = wxGetTranslation(subDir.c_str());
			m_categoryListBox->Append(label);
			m_dirs.push_back(subDir);
		}
		dir = wxFindNextFile();
	}
	wxString tempateDir = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH + wxT("templates");
	if (wxDir::Exists(tempateDir)) {
		dir = wxFindFirstFile(tempateDir + wxFILE_SEP_PATH + wxT("*"), wxDIR);
		while (!dir.IsEmpty()) {
			wxString subDir = dir.AfterLast(wxFILE_SEP_PATH);
			if (!subDir.StartsWith(wxT(".")) && m_dirs.Index(subDir) == wxNOT_FOUND) {
				m_categoryListBox->Append(wxGetTranslation(subDir.c_str()));
				m_dirs.push_back(subDir);
			}
			dir = wxFindNextFile();
		}
	}
    wxCommandEvent evt;
    OnCategorySelect(evt);
	
	m_titleText->SetFocus();
	stdDialogButtonSizer->GetAffirmativeButton()->SetDefault();
}

TemplateDlg::~TemplateDlg() {
	//(*Destroy(TemplateDlg)
	//*)
}

/** Returns template file name */
wxString TemplateDlg::GetTemplate() {
	return m_thumbnails->GetSelectedItem() ? m_thumbnails->GetSelectedItem()->GetFilename() : wxT("");
}

/** Returns title */
wxString TemplateDlg::GetTitle() {
	return m_titleText->GetValue();
}

/** Returns true if chapter menu need to be created */
bool TemplateDlg::IsChapterMenu() {
	return m_chapterMenu->GetValue();
}

void TemplateDlg::OnThumbDoubleClick(wxCommandEvent& event) {
	EndModal(wxID_OK);
}

void TemplateDlg::LoadThumbnails(wxString dir) {
	if (!wxDir::Exists(dir))
		return;
	wxString fname = wxFindFirstFile(dir + wxFILE_SEP_PATH + _T("*.dvdt"));
	while (!fname.IsEmpty()) {
		if ((m_aspectRatio != ar16_9 && !fname.EndsWith(wxT("WS.dvdt")))
				|| (m_aspectRatio == ar16_9 && (fname.EndsWith(wxT("WS.dvdt"))
						|| !wxFile::Exists(fname.substr(0, fname.length() - 5) + wxT("WS.dvdt"))))) {
			wxLogNull log;
			wxImage img;
			wxString imgFile = fname.BeforeLast(wxT('.')) + wxT(".png");
			if (!wxFile::Exists(imgFile)) {
				DVD dvd;
				if (dvd.Open(fname))
					dvd.RenderThumbnail(imgFile);
			}
			img.LoadFile(imgFile);
			wxThumb* thumb = new wxThumb(img, wxT(""), fname);
			m_thumbnails->InsertItem(thumb);
		}
		fname = wxFindNextFile();
	}
}

void TemplateDlg::OnCategorySelect(wxCommandEvent &event) {
	m_thumbnails->Clear();
	wxString tempateDir = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH + wxT("templates");
	if (m_categoryListBox->GetSelection() == 0) {
		for (wxArrayString::const_iterator it = m_dirs.begin(); it != m_dirs.end(); it++) {
			LoadThumbnails(TEMPLATES_DIR + *it);
			LoadThumbnails(tempateDir + wxFILE_SEP_PATH + *it);
		}
	} else {
		wxString subDir = m_dirs[m_categoryListBox->GetSelection() - 1];
		LoadThumbnails(TEMPLATES_DIR + subDir);
		LoadThumbnails(tempateDir + wxFILE_SEP_PATH + subDir);
	}
	m_thumbnails->SortItems();
	
	if (m_thumbnails->GetItemCount() > 0)
		m_thumbnails->SetSelected(0);
	m_thumbnails->Update();
}
