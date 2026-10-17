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
	m_hdModeGroupIdx = -1;
	m_capacityIdx = -1;
	m_resolutionIdx = -1;
	m_propDvd = NULL;
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
	m_propDvd = dvd;
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
		m_resolutionIdx = GetLastControlIndex();
		if (s) {
			s->AddSpacer(4);
			AddIcon(s, _("Info"), _("Info"));
		}
		UpdateResolutionIcon(sel);
	}

	labels = DVD::GetCapacityLabels();
	AddChoiceProp(grid, _("Disc capacity:"), labels[dvd ? dvd->GetCapacity() : s_config.GetDefDiscCapacity()], labels);
	m_capacityIdx = GetLastControlIndex();

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

	// HD output mode. This is a global (not per-project) setting: the authoring
	// pipeline chooses Blu-ray/AVCHD or DVD-Video from it when the project is
	// built, so switching it here takes effect on the next burn without a restart.
	AddText(grid, _("HD output:"));
	wxSizer* hdSizer = BeginGroup(grid, wxT(""));
	labels.clear();
	labels.Add(_("none"));
	labels.Add(_("Blu-ray (BD-R/BD-RE)"));
	labels.Add(_("AVCHD (DVD-R/RW)"));
	AddRadioGroupProp(hdSizer, labels, s_config.Disc.GetMode());
	EndGroup();
	m_hdModeGroupIdx = GetLastControlIndex();
	if (sizer) {
		wxArrayPtrVoid* radioControls = (wxArrayPtrVoid*) m_controls[m_hdModeGroupIdx];
		for (int i = 0; i < (int) radioControls->GetCount(); i++)
			((wxRadioButton*) (*radioControls)[i])->Bind(wxEVT_RADIOBUTTON, &NewProjectDlg::OnChangeHdMode, this);
	}
	UpdateCapacityChoices();
	if (m_resolutionIdx >= 0)
		UpdateResolutionIcon(((wxChoice*) GetControl(m_resolutionIdx))->GetSelection());
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
	if (!Validate())
		return false;
	PersistHdSettings();
	return true;
}

void NewProjectDlg::UpdateResolutionIcon(int sel) {
	if (GetSelectedHdMode() != BD_MODE_NONE) {
		UpdateIcon(0, _("Info"),
				_("Blu-ray/AVCHD output ignores this setting: titles are always authored\nat 1920x1080 with the NTSC/PAL standards frame rate (film 23.976,\n29.97/59.94 sources 29.97, 25/50 sources 25)."),
				wxART_INFORMATION);
	} else {
		UpdateIcon(0, sel > 0 ? _("Warning") : _("Info"),
				sel > 0 ? _("DVD with HD resolution doesn't comply with the standard and\ncan be played only in some blue-ray or software players.")
						: _("DVD standard officially supports only video data\nwith resolution up to 720x576 pixels (SD)."),
				sel > 0 ? wxART_WARNING : wxART_INFORMATION);
	}
}

void NewProjectDlg::OnChangeResolution(wxCommandEvent& evt) {
	int sel = ((wxChoice*) evt.GetEventObject())->GetSelection();
	UpdateResolutionIcon(sel);
}

wxString NewProjectDlg::GetLabel() {
	return GetString(propIndex);
}

DvdResolution NewProjectDlg::GetDvdResolution() {
	if (m_resolutionIdx >= 0)
		return (DvdResolution) GetInt(m_resolutionIdx);
	return dvdSD;
}

DiscCapacity NewProjectDlg::GetCapacity() {
	// HD media sizes (BD-R 25/50/100, DVD-5/9 for AVCHD) are kept in the HD
	// media-size preset, not in the DVD capacity enum. A new project therefore
	// starts unlimited (the DVD size gate in MainWin::Burn is skipped in HD
	// mode); editing an existing project keeps its DVD capacity untouched.
	if (GetSelectedHdMode() != BD_MODE_NONE)
		return m_propDvd ? m_propDvd->GetCapacity() : dcUNLIMITED;
	int i2 = GetHdOffset();
	return (DiscCapacity) GetInt(propIndex + i2 + 1);
}

int NewProjectDlg::GetSelectedHdMode() {
	int mode = m_hdModeGroupIdx >= 0 ? GetInt(m_hdModeGroupIdx) : s_config.Disc.GetMode();
	if (mode < BD_MODE_NONE || mode > BD_MODE_AVCHD)
		mode = BD_MODE_NONE;
	return mode;
}

/** Number of controls between the label and the capacity choice. The DVD
  * resolution choice exists only if HD resolutions were enabled when the
  * dialog was built, so the index math must not depend on the current
  * setting: PersistHdSettings() may enable it while the dialog is accepted. */
int NewProjectDlg::GetHdOffset() {
	return m_resolutionIdx >= 0 ? 1 : 0;
}

int NewProjectDlg::GetHdMode() {
	return GetSelectedHdMode();
}

int NewProjectDlg::GetMediaSize() {
	int mode = GetSelectedHdMode();
	if (mode == BD_MODE_NONE || m_capacityIdx < 0)
		return s_config.Disc.GetMediaSize();
	wxChoice* choice = (wxChoice*) GetControl(m_capacityIdx);
	int sel = choice ? choice->GetSelection() : -1;
	if (sel >= 0 && choice->GetClientData(sel) != NULL)
		return (int) (wxUIntPtr) choice->GetClientData(sel);
	return HdProfile::DefaultMediaSize((HdProfile::Mode) mode);
}

/** Repopulates the capacity choice with the presets of the selected mode. */
void NewProjectDlg::UpdateCapacityChoices() {
	if (m_capacityIdx < 0)
		return;
	wxChoice* choice = (wxChoice*) GetControl(m_capacityIdx);
	if (choice == NULL)
		return;
	int mode = GetSelectedHdMode();
	choice->Clear();
	if (mode == BD_MODE_AVCHD) {
		choice->Append(_("DVD-5 (4.7 GB)"), (void*) (wxUIntPtr) HdProfile::MEDIA_DVD5);
		choice->Append(_("DVD-9 (8.5 GB)"), (void*) (wxUIntPtr) HdProfile::MEDIA_DVD9);
	} else if (mode == BD_MODE_BLURAY) {
		choice->Append(_("BD-R 25 GB"), (void*) (wxUIntPtr) HdProfile::MEDIA_BD25);
		choice->Append(_("BD-R 50 GB (BDXL)"), (void*) (wxUIntPtr) HdProfile::MEDIA_BD50);
		choice->Append(_("BD-R 100 GB (BDXL)"), (void*) (wxUIntPtr) HdProfile::MEDIA_BD100);
	} else {
		wxArrayString dvdLabels = DVD::GetCapacityLabels();
		for (unsigned int i = 0; i < dvdLabels.GetCount(); i++)
			choice->Append(dvdLabels[i]);
	}
	if (mode == BD_MODE_NONE) {
		int cap = m_propDvd ? m_propDvd->GetCapacity() : s_config.GetDefDiscCapacity();
		if (cap < 0 || cap >= (int) choice->GetCount())
			cap = dcDVD5;
		choice->SetSelection(cap);
	} else {
		int want = s_config.Disc.GetMediaSize();
		if (!HdProfile::MediaSizeValidForMode((HdProfile::MediaSize) want, (HdProfile::Mode) mode))
			want = HdProfile::DefaultMediaSize((HdProfile::Mode) mode);
		int sel = 0;
		for (int i = 0; i < (int) choice->GetCount(); i++)
			if ((int) (wxUIntPtr) choice->GetClientData(i) == want) {
				sel = i;
				break;
			}
		choice->SetSelection(sel);
	}
	Layout();
}

void NewProjectDlg::OnChangeHdMode(wxCommandEvent& evt) {
	UpdateCapacityChoices();
	// HD output is always authored at 1920x1080, so preselect Full HD to match
	// what the encoder actually produces (the choice is informational in HD mode)
	if (GetSelectedHdMode() != BD_MODE_NONE && m_resolutionIdx >= 0) {
		((wxChoice*) GetControl(m_resolutionIdx))->SetSelection(dvdFullHD);
		UpdateResolutionIcon(dvdFullHD);
	} else if (m_resolutionIdx >= 0) {
		UpdateResolutionIcon(((wxChoice*) GetControl(m_resolutionIdx))->GetSelection());
	}
	Layout();
}

void NewProjectDlg::PersistHdSettings() {
	int mode = GetSelectedHdMode();
	s_config.Disc.SetMode(mode);
	if (mode != BD_MODE_NONE) {
		s_config.Disc.SetMediaSize(GetMediaSize());
		// HD output implies HD titles; this makes the HD resolution choices
		// available the next time the dialog is opened
		if (!s_config.GetAllowHdTitles())
			s_config.SetAllowHdTitles(true);
	}
	s_config.Flush();
}

int NewProjectDlg::GetVideoBitrate() {
	int i2 = GetHdOffset();
	int q = GetInt(propIndex + i2 + 2);
	int b = GetInt(propIndex + i2 + 3);
	if (q == 0)
		return -1;
	return q < 8 ? (9-q)*1000 : b;
}

int NewProjectDlg::GetAudioBitrate() {
	int i2 = GetHdOffset();
	return GetInt(propIndex + i2 + 4);
}

VideoFormat NewProjectDlg::GetVideoFormat() {
	int i2 = GetHdOffset();
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
	int i2 = GetHdOffset();
	return (AspectRatio) (GetInt(propIndex + i2 + 6) + 1);
}

AudioFormat NewProjectDlg::GetAudioFormat() {
	int i2 = GetHdOffset();
	return (AudioFormat) (GetInt(propIndex + i2 + 7) + 2);
}

DefaultPostCommand NewProjectDlg::GetDefPostCommand() {
	int i2 = GetHdOffset();
	return (DefaultPostCommand) (GetInt(propIndex + i2 + 8));
}

bool NewProjectDlg::IsNoMenu() {
	int i2 = GetHdOffset();
	return GetBool(propIndex + i2 + 10);
}
