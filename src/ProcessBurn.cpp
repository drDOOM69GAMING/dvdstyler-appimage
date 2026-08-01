/////////////////////////////////////////////////////////////////////////////
// Name:        ProcessBurn.cpp
// Author:      Alex Thuering
// Created:     3.10.2014 (refactored)
// RCS-ID:      $Id: ProcessBurn.cpp,v 1.4 2015/11/29 17:29:29 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "ProcessBurn.h"
#include "ProcessExecute.h"
#include "ProcessIsoImage.h"
#include "BurnDlg.h"
#include "ProgressDlg.h"
#include "SysUtils.h"
#include "Config.h"
#include <wxVillaLib/utils.h>
#include <wx/msgdlg.h>

class BurnExecute: public ProgressExecute {
public:
	BurnExecute(ProgressDlg* process, wxString filter): ProgressExecute(process, filter), m_burnOk(false) {}

	virtual void ProcessOutput(wxString line) {
		if (line.Find(wxT(": writing lead-out")) >= 0)
			m_burnOk = true;
		ProgressExecute::ProcessOutput(line);
	}

	bool Execute(wxString command, wxString inputFile = wxEmptyString, wxString outputFile = wxEmptyString) {
		m_burnOk = false;
		bool res = ProgressExecute::Execute(command, wxEmptyString, wxEmptyString);
		return res || m_burnOk;
	}

private:
	bool m_burnOk;
};

/** Constructor */
ProcessBurn::ProcessBurn(ProgressDlg* progressDlg, BurnDlg* burnDlg, DVD* dvd, const wxString& dvdOutDir,
		const wxString& tmpDir): Process(progressDlg) {
	this->burnDlg = burnDlg;
	this->dvd = dvd;
	this->dvdOutDir = dvdOutDir;
	this->tmpDir = tmpDir;
}

/** Returns true, if process need be executed */
bool ProcessBurn::IsNeedExecute() {
	return burnDlg->DoBurn();
}

/** Executes process */
bool ProcessBurn::Execute() {
	if (!burnDlg->DoBurn())
		return true;
	if (progressDlg->WasCanceled())
		return false;
	progressDlg->AddSummaryMsg(_("Burning"));
	progressDlg->SetSubSteps(100);
	// check disc
	wxString device = burnDlg->GetDevice();
	wxString cmd;
#ifdef __WXMAC__
	while (!IsMediaPresent(device)) {
		if (wxMessageBox(wxString::Format(_("Please insert a blank or rewritable DVD into the device %s."),
				GetDeviceName(device).c_str()), _("Burn"), wxOK|wxCANCEL, progressDlg) == wxCANCEL) {
			progressDlg->AddSummaryMsg(_("Aborted"), wxEmptyString, *wxRED);
		   return false;
		}
	}
	long discSize = GetMediaSpaceFree(device); // size in 2048 blocks
	progressDlg->AddDetailMsg(wxString::Format(wxT("Disc size: %ld MB"), discSize / 512));
#else
	cmd = s_config.GetBurnScanCmd();
	cmd.Replace(wxT("$DEVICE"), device);
#ifdef __WXMSW__
	cmd = wxGetAppPath() + wxString(wxFILE_SEP_PATH) + cmd;
#endif
	wxArrayString output;
	while (true) {
		if (wxExecute(cmd, output, wxEXEC_SYNC | wxEXEC_NODISABLE) == 0)
			break;
		if (wxMessageBox(wxString::Format(_("Please insert a blank or rewritable DVD into the device %s."), device.c_str()),
				_("Burn"), wxOK|wxCANCEL, progressDlg) == wxCANCEL) {
			progressDlg->AddSummaryMsg(_("Aborted"), wxEmptyString, *wxRED);
			return false;
		}
	}
	// get disc size
	long discSize = 0; // size in 2048 blocks
	for (unsigned int i = 0; i < output.Count(); i++) {
		if (output[i].length() > 12 && output[i].SubString(1, 12) == wxT("Free Blocks:")) {
			wxString discSizeStr = output[i].AfterFirst(wxT(':')).Trim(false).BeforeFirst(wxT('*'));
			discSizeStr.ToLong(&discSize);
			progressDlg->AddDetailMsg(wxString::Format(wxT("Disc size: %ld MB"), discSize / 512));
			break;
		}
	}
	if (discSize < 2290000)
		discSize = s_config.GetBlurayMode() == BD_MODE_BLURAY ? 12219392 : 2295104;
#endif
	// check size
	long size = 0;
#ifdef __WXMAC__
	bool burnIso = true;
#else
	bool burnIso = s_config.GetBlurayMode() != BD_MODE_NONE || burnDlg->DoAddECC();
#endif
	if (burnIso) {
		size = wxFile(tmpDir + TMP_ISO).Length() / 2048; // size in 2048 blocks
	} else {
		cmd = s_config.GetIsoSizeCmd();
		cmd.Replace(_T("$DIR"), dvdOutDir.Mid(0, dvdOutDir.length() - 1));
#if defined(__WXMSW__) || defined(__WXMAC__)
		cmd = wxGetAppPath() + wxString(wxFILE_SEP_PATH) + cmd;
#endif
		wxArrayString output;
		wxExecute(cmd, output, wxEXEC_SYNC | wxEXEC_NODISABLE);
		if (output.Count() > 0 && output[0].length() > 0) {
			output[0].ToLong(&size);
			size = size + 254;
		}
	}
	progressDlg->AddDetailMsg(wxString::Format(wxT("ISO Size: %ld MB"), size / 512));
	if (size > discSize && wxMessageBox(wxString::Format(_("Size of Disc Image > %.2f GB. Do you want to continue?"),
			(double) discSize / 512 / 1024), _("Burn"), wxYES_NO|wxICON_QUESTION, progressDlg) == wxNO) {
		progressDlg->AddSummaryMsg(_("Aborted"), wxEmptyString, *wxRED);
		return false;
	}
	// burn
	if (burnIso) {
		cmd = s_config.GetBurnISOCmd();
		cmd.Replace(_T("$FILE"), tmpDir + TMP_ISO);
		// get iso size in sectors
		long size = wxFile(tmpDir + TMP_ISO).Length() / 2048;
		cmd.Replace(_T("$SIZE"), wxString::Format(wxT("%ld"), size));
	} else {
		cmd = s_config.GetBurnCmd();
		cmd.Replace(_T("$DIR"), dvdOutDir.Mid(0, dvdOutDir.length() - 1));
	}
	wxString label = dvd->GetLabel();
	label.Replace(wxT("\""), wxT("\\\""));
	cmd.Replace(_T("$VOL_ID"), label);
	cmd.Replace(_T("$DEV"), device);
	wxString speedStr;
	if (burnDlg->GetSpeed() > 0) {
		speedStr = s_config.GetBurnSpeedOpt();
		speedStr.Replace(_T("$SPEED"), wxString::Format(_T("%d"), burnDlg->GetSpeed()));
	}
	cmd.Replace(_T("$SPEEDSTR"), speedStr);
	BurnExecute exec(progressDlg, wxT(".*"));
	if (!exec.Execute(cmd)) {
		progressDlg->Failed();
		return false;
	}
	progressDlg->IncStep();
	return true;
}
