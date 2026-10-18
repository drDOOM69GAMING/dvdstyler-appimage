/////////////////////////////////////////////////////////////////////////////
// Name:        DVDPropDlg.cpp
// Purpose:     DVD properties dialog
// Author:      Alex Thuering
// Created:     7.03.2005
// RCS-ID:      $Id: DVDPropDlg.cpp,v 1.39 2016/03/12 17:48:16 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "DVDPropDlg.h"
#include "Config.h"

DVDPropDlg::DVDPropDlg(wxWindow *parent, DVD* dvd): NewProjectDlg(parent, false), m_dvd(dvd) {
	Create(true);
	SetSize(400, -1);
}

void DVDPropDlg::CreatePropPanel(wxSizer* sizer) {
	// DVD properties
	CreateDVDPropPanel(sizer, m_dvd);
	
	wxFlexGridSizer* grid = sizer ? new wxFlexGridSizer(2, 1, 16) : NULL;
	if (grid) {
		grid->AddGrowableCol(1);
		sizer->Add(grid, 0, wxEXPAND|wxALL, 6);
	}
	wxString fpc = sizer ? m_dvd->GetVmgm().GetFpc() : wxT("");
	AddComboProp(grid, _("First Play Commands:"), fpc, GetFPCommands());

	AddCheckProp(sizer, _("Create jumppads"), sizer ? m_dvd->IsJumppadsEnabled() : false);
	AddCheckProp(sizer, _("Create empty VMGM/titleset menus if it doesn't exist"), sizer ? m_dvd->IsEmptyMenuEnabled() : true);
}

/**
 * Returns list of possible first play commands
 */
wxArrayString DVDPropDlg::GetFPCommands() {
	wxArrayString commands;
	for (int pgci=0; pgci<(int)m_dvd->GetVmgm().Count(); pgci++)
		commands.Add(wxString::Format(wxT("jump vmgm menu %d;"),pgci+1));
	int titleIdx = 0;
	for (int tsi=0; tsi<(int)m_dvd->GetTitlesets().Count(); tsi++) {
		Titleset* ts = m_dvd->GetTitlesets()[tsi];
		if (ts->GetMenus().Count()>0)
			commands.Add(wxString::Format(wxT("jump titleset %d menu;"), tsi+1));
		const StringSet& entrySet = ts->GetMenus().GetEntries();
		for (StringSet::const_iterator entry = entrySet.begin(); entry != entrySet.end(); entry++)
			commands.Add(wxString::Format(wxT("jump titleset %d menu entry %s;"), tsi+1, entry->c_str()));
		if (m_dvd->IsJumppadsEnabled()) {
			for (int pgci=1; pgci<(int)ts->GetMenus().Count(); pgci++)
				commands.Add(wxString::Format(wxT("jump titleset %d menu %d;"), tsi+1, pgci+1));
			for (int pgci=0; pgci<(int)ts->GetTitles().Count(); pgci++)
				commands.Add(wxString::Format(wxT("jump titleset %d title %d;"), tsi+1, pgci+1));
		} else {
			for (int pgci=0; pgci<(int)ts->GetTitles().Count(); pgci++) {
				commands.Add(wxString::Format(wxT("jump title %d;"), titleIdx+1));
				titleIdx++;
			}
		}
	}
	return commands;
}

bool DVDPropDlg::SetValues() {
	if (!Validate())
		return false;
	PersistHdSettings();
	int n = m_dvdPanelEndIdx >= 0 ? m_dvdPanelEndIdx + 1 : (s_config.GetAllowHdTitles() ? 12 : 11);
	if (GetString(n).length() > 0) {
		DVDAction action;
		action.SetCustom(GetString(n));
		if (!action.IsValid(m_dvd, -1, -1, true, wxT(""), true, false))
			return false;
	}
	m_dvd->SetLabel(GetLabel());
	m_dvd->SetDvdResolution(GetDvdResolution());
	m_dvd->SetCapacity(GetCapacity());
	m_dvd->SetVideoBitrateAuto(GetVideoBitrate() < 0);
	if (GetVideoBitrate() >= 0)
		m_dvd->SetVideoBitrate(GetVideoBitrate());
	m_dvd->SetAudioBitrate(GetAudioBitrate());
	if (m_dvd->GetVideoFormat() != GetVideoFormat())
		m_dvd->SetVideoFormat(GetVideoFormat());
	if (m_dvd->GetAspectRatio() != GetAspectRatio())
		m_dvd->SetAspectRatio(GetAspectRatio());
	if (m_dvd->GetDefPostCommand() != GetDefPostCommand())
		m_dvd->SetDefPostCommand(GetDefPostCommand(), true);
	m_dvd->SetAudioFormat(GetAudioFormat());
	// set FPC
	m_dvd->GetVmgm().SetFpc(GetString(n++));
	bool enableJumppad = GetBool(n++);
	if (m_dvd->IsJumppadsEnabled() != enableJumppad && !enableJumppad) {
		m_dvd->CheckActions(m_dvd->HasTemplate());
	}
	m_dvd->EnableJumppads(enableJumppad);
	m_dvd->EnableEmptyMenu(GetBool(n++));
	return true;
}
