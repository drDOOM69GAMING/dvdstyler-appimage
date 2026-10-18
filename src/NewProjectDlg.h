/////////////////////////////////////////////////////////////////////////////
// Name:        NewProjectDlg.h
// Purpose:     New project dialog
// Author:      Alex Thuering
// Created:     29.10.2006
// RCS-ID:      $Id: NewProjectDlg.h,v 1.9 2013/01/10 15:29:57 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef NEW_PROJECT_DLG_H
#define NEW_PROJECT_DLG_H

#include <wx/wx.h>
#include <wxVillaLib/PropDlg.h>
#include "DVD.h"

class NewProjectDlg: public wxPropDlg {
public:
	/**
	 * Constructor. 
	 */
	NewProjectDlg(wxWindow *parent, bool create = true);
	virtual ~NewProjectDlg() {}
	
	wxString GetLabel();
	DvdResolution GetDvdResolution();
	DiscCapacity GetCapacity();
	int GetVideoBitrate();
	int GetAudioBitrate();
	VideoFormat GetVideoFormat();
	AudioFormat GetAudioFormat();
	AspectRatio GetAspectRatio();
	DefaultPostCommand GetDefPostCommand();
	bool IsNoMenu();
	/** Selected HD output mode (BD_MODE_NONE, BD_MODE_BLURAY or BD_MODE_AVCHD). */
	int GetHdMode();
	/** Selected media-size preset for the current HD mode (HdProfile::MediaSize). */
	int GetMediaSize();
	
protected:
	int propIndex;
	virtual void CreatePropPanel(wxSizer* sizer);
	virtual void CreateDVDPropPanel(wxSizer* sizer, DVD* dvd);
	virtual bool SetValues();
	virtual bool Validate();
	
	void UpdateResolutionIcon(int sel);
	void OnChangeResolution(wxCommandEvent& evt);
	/** Repopulates the capacity choice with the presets of the selected mode
	  * (DVD sizes for DVD-Video/AVCHD, BD-R sizes for Blu-ray). */
	void UpdateCapacityChoices();
	void OnChangeHdMode(wxCommandEvent& evt);
	/** Reads the mode that is currently selected in the dialog. */
	int GetSelectedHdMode();
	/** Offset caused by the optional DVD resolution choice. */
	int GetHdOffset();
	/** Writes the selected HD mode and media size to the global settings. The
	  * authoring pipeline reads them at burn time, so no restart is needed. */
	void PersistHdSettings();
	
	int m_hdModeGroupIdx;	/**< index of the HD output radio group, -1 if not created */
	int m_encGroupIdx;		/**< index of the CPU/GPU radio group, -1 if not created */
	int m_capacityIdx;		/**< index of the capacity/media-size choice, -1 if not created */
	int m_resolutionIdx;	/**< index of the resolution choice, -1 if not created */
	int m_dvdPanelEndIdx;	/**< index of the last control of the project panel, -1 if unknown */
	DVD* m_propDvd;			/**< project being edited, NULL when creating a new one */
	DECLARE_EVENT_TABLE()
};

#endif // NEW_PROJECT_DLG_H
