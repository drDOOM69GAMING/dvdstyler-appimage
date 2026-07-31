/////////////////////////////////////////////////////////////////////////////
// Name:        NewProjectDlg.cpp
// Purpose:     New project dialog
// Author:      Alex Thuering
// Created:     29.10.2006
// RCS-ID:      $Id: NewProjectDlg.cpp,v 1.20 2014/01/28 18:03:15 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "NewProjectDlg.h"
#include "DVD.h"
#include "Config.h"

enum {
	DVD_RESOLUTION_CHOICE_ID = 7900,
};

BEGIN_EVENT_TABLE(NewProjectDlg, wxPropDlg)
	EVT_CHOICE(DVD_RESOLUTION_CHOICE_ID, NewProjectDlg::OnChangeResolution)
END_EVENT_TABLE()

NewProjectDlg::NewProjectDlg(wxWindow *parent, bool create): wxPropDlg(parent) {
	propIndex = 0;
	if (create) {
		Create();
		SetTitle(_("Create a new DVD"));
		SetSize(400, -1);
	}
}

void NewProjectDlg::CreatePropPanel(wxSizer* sizer) {
	CreateDVDPropPanel(sizer, NULL);
	AddCheckProp(sizer, _("Create DVD without menus"), false);
}

void NewProjectDlg::CreateDVDPropPanel(wxSizer* sizer, DVD* dvd) {
	wxFlexGridSizer* grid = sizer ? new wxFlexGridSizer(2, 1, 16) : NULL;
	if (grid) {
		grid->AddGrowableCol(1);
		sizer->Add(grid, 0, wxEXPAND|wxALL, 6);
	}
	AddTextProp(grid, _("Disc label:"), dvd ? dvd->GetLabel() : s_config.GetDefDiscLabel());

	wxArrayString labels;
	
	if (s_config.GetAllowHdTitles()) {
		labels = DVD::GetDvdResolutionLabels();
		int sel = dvd ? dvd->GetDvdResolution() : s_config.GetDefDvdResolution();
		wxSizer* s = AddChoiceProp(grid, _("DVD Resolution:"), labels[sel], labels, 220, false, DVD_RESOLUTION_CHOICE_ID);
		s->AddSpacer(4);
		AddIcon(s, _("Info"), _("Info"));
		UpdateResolutionIcon(sel);
	}

	labels = DVD::GetCapacityLabels();
	AddChoiceProp(grid, _("Disc capacity:"), labels[dvd ? dvd->GetCapacity() : s_config.GetDefDiscCapacity()], labels);

	// Video bitrate
	labels = DVD::GetVideoBitrateLabels();
	int b = dvd ? dvd->GetVideoBitrate() : s_config.GetVideoBitrate();
	int q = (dvd && dvd->IsVideoBitrateAuto()) || b == -1 ? 0 : (b%1000 == 0 && b>=2000 && b<=8000 ? 9-b/1000 : 8);
	wxSizer* vbSizer = AddChoiceCustomProp(grid, _("Video bitrate:"), labels[q], labels, 8, 0, false);
	if (vbSizer)
		vbSizer->AddSpacer(4);
	AddSpinProp(vbSizer, wxT(""), b == -1 ? 4500 : b, 500, 9000, 80, _("KBit/s"), false);
	if (vbSizer)
		SetLastControlCustom(GetLastControlIndex()-1, q == 8);
	
	// Audio bitrate
	b = dvd ? dvd->GetAudioBitrate() : s_config.GetAudioBitrate();
	AddSpinProp(grid, _("Audio bitrate:"), b, 1, 9999, 80, _("KBit/s"));

	// Video and audio format
	wxFlexGridSizer* fSizer = sizer ? new wxFlexGridSizer(3, 1, 16) : NULL;
	if (sizer) {
		fSizer->AddGrowableCol(0);
		fSizer->AddGrowableCol(1);
		fSizer->AddGrowableCol(2);
		sizer->Add(fSizer, 0, wxEXPAND|wxALL, 6);
	}
	// Video format
	wxSizer* grp = BeginGroup(fSizer, _("Video Format"));
	if (grp)
		grp->Add(4, 4);
	wxArrayString formats;
	formats.Add(wxT("PAL"));
	formats.Add(wxT("NTSC"));
	VideoFormat vf = dvd ? dvd->GetVideoFormat() : (VideoFormat) s_config.GetDefVideoFormat();
	AddRadioGroupProp(grp, formats, isNTSC(vf) ? 1 : 0);
	if (grp)
		grp->Add(4, 4);
	// Aspect Ratio
	formats = DVD::GetAspectRatioLabels();
	grp = BeginGroup(fSizer, _("Aspect Ratio"));
	if (grp)
		grp->Add(4, 4);
	AddRadioGroupProp(grp, formats, (dvd ? dvd->GetAspectRatio() : s_config.GetDefAspectRatio()) - 1);
	if (grp) {
		if (grp->GetMinSize().GetWidth() < 100)
			grp->SetMinSize(wxSize(100, -1));
		grp->Add(4, 4);
	}
	// Audio format
	grp = BeginGroup(fSizer, _("Audio Format"));
	if (grp)
		grp->Add(4, 4);
	formats = DVD::GetAudioFormatLabels(false, false, false);
	AddRadioGroupProp(grp, formats, (dvd ? dvd->GetAudioFormat() : s_config.GetDefAudioFormat()) - 2);
	if (grp)
		grp->Add(4, 4);

	labels = DVD::GetDefPostCommandLabels();
	AddChoiceProp(grid, _("Default title post command:"),
			labels[dvd ? dvd->GetDefPostCommand() : s_config.GetDefPostCommand()], labels);
}

bool NewProjectDlg::Validate() {
	if (GetLabel().length() == 0) {
		wxMessageBox(_("Please enter the volume name."), GetTitle(), wxOK|wxICON_ERROR, this);
		return false;
	}
	if (GetLabel().length() > 32) {
		wxMessageBox(_("Volume name can only be a maximum of 32 characters."), GetTitle(), wxOK|wxICON_ERROR, this);
		return false;
	}
	return true;
}

bool NewProjectDlg::SetValues() {
	return Validate();
}

void NewProjectDlg::UpdateResolutionIcon(int sel) {
	UpdateIcon(0, sel > 0 ? _("Warning") : _("Info"),
				sel > 0 ? _("DVD with HD resolution doesn't comply with the standard and\ncan be played only in some blue-ray or software players.")
						: _("DVD standard officially supports only video data\nwith resolution up to 720x576 pixels (SD)."),
				sel > 0 ? wxART_WARNING : wxART_INFORMATION);
}

void NewProjectDlg::OnChangeResolution(wxCommandEvent& evt) {
	int sel = ((wxChoice*) evt.GetEventObject())->GetSelection();
	UpdateResolutionIcon(sel);
}

wxString NewProjectDlg::GetLabel() {
	return GetString(propIndex);
}

DvdResolution NewProjectDlg::GetDvdResolution() {
	if (s_config.GetAllowHdTitles()) {
		return (DvdResolution) GetInt(propIndex + 1);
	}
	return dvdSD;
}

DiscCapacity NewProjectDlg::GetCapacity() {
	int i2 = s_config.GetAllowHdTitles() ? 1 : 0;
	return (DiscCapacity) GetInt(propIndex + i2 + 1);
}

int NewProjectDlg::GetVideoBitrate() {
	int i2 = s_config.GetAllowHdTitles() ? 1 : 0;
	int q = GetInt(propIndex + i2 + 2);
	int b = GetInt(propIndex + i2 + 3);
	if (q == 0)
		return -1;
	return q < 8 ? (9-q)*1000 : b;
}

int NewProjectDlg::GetAudioBitrate() {
	int i2 = s_config.GetAllowHdTitles() ? 1 : 0;
	return GetInt(propIndex + i2 + 4);
}

VideoFormat NewProjectDlg::GetVideoFormat() {
	int i2 = s_config.GetAllowHdTitles() ? 1 : 0;
	int vf = GetInt(propIndex + i2 + 5);
	if (GetDvdResolution() == dvdHalfHD)
		vf += vfPAL_HALF_HD;
	else if (GetDvdResolution() == dvdHDV)
		vf += vfPAL_HDV;
	else if (GetDvdResolution() == dvdFullHD)
		vf += vfPAL_FULL_HD;
	else 
		vf += vfPAL;
	return (VideoFormat) vf;
}

AspectRatio NewProjectDlg::GetAspectRatio() {
	int i2 = s_config.GetAllowHdTitles() ? 1 : 0;
	return (AspectRatio) (GetInt(propIndex + i2 + 6) + 1);
}

AudioFormat NewProjectDlg::GetAudioFormat() {
	int i2 = s_config.GetAllowHdTitles() ? 1 : 0;
	return (AudioFormat) (GetInt(propIndex + i2 + 7) + 2);
}

DefaultPostCommand NewProjectDlg::GetDefPostCommand() {
	int i2 = s_config.GetAllowHdTitles() ? 1 : 0;
	return (DefaultPostCommand) (GetInt(propIndex + i2 + 8));
}

bool NewProjectDlg::IsNoMenu() {
	int i2 = s_config.GetAllowHdTitles() ? 1 : 0;
	return GetBool(propIndex + i2 + 9);
}
