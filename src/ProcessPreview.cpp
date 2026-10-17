/////////////////////////////////////////////////////////////////////////////
// Name:        ProcessPreview.cpp
// Author:      Alex Thuering
// Created:     3.10.2014 (refactored)
// RCS-ID:      $Id: ProcessPreview.cpp,v 1.6 2016/08/21 13:36:55 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "ProcessPreview.h"
#include "Config.h"
#include "ProgressDlg.h"
#include "BurnDlg.h"
#include <wx/msgdlg.h>

/** Constructor */
ProcessPreview::ProcessPreview(ProgressDlg* progressDlg, BurnDlg* burnDlg, const wxString& dvdOutDir): Process(progressDlg) {
	this->burnDlg = burnDlg;
	this->dvdOutDir = dvdOutDir;
}
    
/** Returns true, if process need be executed */
bool ProcessPreview::IsNeedExecute() {
	return burnDlg->DoPreview() && !progressDlg->DoShutdown();
}

/** Returns true, if gauge need be updated */
bool ProcessPreview::IsUpdateGauge() {
	return false;
}
	
/** Executes process */
bool ProcessPreview::Execute() {
	if (progressDlg->WasCanceled())
		return false;
	progressDlg->AddSummaryMsg(_("Start preview"));
	wxString cmd = s_config.GetPreviewCmd();
	if (cmd == wxT("wmplayer")) {
		cmd = wxT("start wmplayer \"$DIR\\VIDEO_TS\\VIDEO_TS.VOB\"");
		cmd.Replace(wxT("$DIR"), dvdOutDir.Mid(0, dvdOutDir.length() - 1));
		if (!wxShell(cmd)) {
			wxString msg = _("Starting of selected DVD player is failed. \
Please open the following directory with your favorite DVD player: ");
			msg += dvdOutDir;
			wxMessageBox(msg, _("Burn"), wxOK|wxICON_ERROR, progressDlg);
		}
	} else if (cmd.StartsWith(wxT("/Applications/VLC.app/Contents/MacOS/VLC"))) {
		if (!wxFileExists(wxT("/Applications/VLC.app/Contents/MacOS/VLC"))) {
			wxString msg = _("VLC player is not found. \
Please install VLC player (http://www.videolan.org) in Applications directory \
or open the following directory with your favorite DVD player: ");
			msg += dvdOutDir;
			wxMessageBox(msg, _("Burn"), wxOK|wxICON_ERROR, progressDlg);
		} else {
			cmd += wxT(" \"dvd:///$DIR\"");
			cmd.Replace(wxT("$DIR"), dvdOutDir.Mid(0, dvdOutDir.length() - 1));
			if (wxExecute(cmd, wxEXEC_ASYNC) < 0) {
				wxString msg = _("Starting of selected DVD player is failed. \
Please open the following directory with your favorite DVD player: ");
				msg += dvdOutDir;
				wxMessageBox(msg, _("Burn"), wxOK|wxICON_ERROR, progressDlg);
			}
		}
	} else {
		if (cmd.Find(wxT("%1")) > 0) {
			cmd.Replace(wxT("%1"), wxT("$DIR"));
		} else if (cmd.Find(wxT("$DIR")) < 0) {
			if (cmd[0] != wxT('"'))
				cmd = wxT('"') + cmd + wxT('"');
			if (cmd.Find(wxT("wmplayer")) >= 0)
				cmd += wxT(" \"$DIR\\VIDEO_TS\\VIDEO_TS.VOB\"");
			else if (cmd.Find(wxT("mpc-hc")) >= 0)
				cmd += wxT(" /dvd \"$DIR\"");
			else
				cmd += wxT(" \"dvd:///$DIR\"");
		}
		cmd.Replace(wxT("$DIR"), dvdOutDir.Mid(0, dvdOutDir.length() - 1));
		if (!ExecAsync(cmd)) {
			wxString msg = _("Starting of selected DVD player is failed. \
Please open the following directory with your favorite DVD player: ");
			msg += dvdOutDir;
			wxMessageBox(msg, _("Burn"), wxOK|wxICON_ERROR, progressDlg);
		}
	}
	// In auto-start mode the action was taken from the saved defaults and was never
	// confirmed by the user, so ask once. When the user has just chosen the output
	// in the burn dialog there is nothing left to confirm.
	if (progressDlg->IsAutoStart() && (burnDlg->DoBurn() || burnDlg->DoCreateIso())) {
		wxString msg = burnDlg->DoBurn() ? _("Do you want to burn this video to DVD?") : _("Do you want to create an iso image of this video?");
		if (wxMessageBox(msg, _("Burn"), wxYES_NO|wxICON_QUESTION, progressDlg) == wxNO) {
			progressDlg->AddSummaryMsg(_("Aborted"), wxEmptyString, *wxRED);
			return false;
		}
	}
	return true;
}
