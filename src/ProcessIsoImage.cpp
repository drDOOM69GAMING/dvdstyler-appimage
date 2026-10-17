/////////////////////////////////////////////////////////////////////////////
// Name:        ProcessIsoImage.h
// Author:      Alex Thuering
// Created:     3.10.2014 (refactored)
// RCS-ID:      $Id: ProcessIsoImage.cpp,v 1.4 2016/12/11 10:34:21 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "ProcessIsoImage.h"
#include "ProcessExecute.h"
#include "BurnDlg.h"
#include "Cache.h"
#include "Config.h"
#include "SysUtils.h"
#include <wxVillaLib/utils.h>
#include <wx/filename.h>

/** Constructor */
ProcessIsoImage::ProcessIsoImage(ProgressDlg* progressDlg, BurnDlg* burnDlg, DVD* dvd, Cache* cache,
		const wxString& dvdOutDir, const wxString& tmpDir): Process(progressDlg) {
	this->burnDlg = burnDlg;
	this->dvd = dvd;
	this->cache = cache;
	this->dvdOutDir = dvdOutDir;
	this->tmpDir = tmpDir;
}

long GetFreeSpaceOn(wxString dir) {
	wxDiskspaceSize_t pFree;
	wxGetDiskSpace(dir, NULL, &pFree);
	return (long) (pFree.ToDouble() / 1024);
}

/** Executes process */
bool ProcessIsoImage::Execute() {
	if (progressDlg->WasCanceled())
		return false;
	wxString isoFile = burnDlg->DoCreateIso() ? burnDlg->GetIsoFile() : tmpDir + TMP_ISO;
	// check if there is enough space
	long size = 0;
	wxString cmd = s_config.GetIsoSizeCmd();
	cmd.Replace(_T("$DIR"), dvdOutDir.Mid(0, dvdOutDir.length() - 1));
#if defined(__WXMSW__) || defined(__WXMAC__)
		cmd = wxGetAppPath() + wxString(wxFILE_SEP_PATH) + cmd;
#endif
	wxArrayString output;
	wxExecute(cmd, output, wxEXEC_SYNC | wxEXEC_NODISABLE);
	if (output.Count() > 0 && output[0].length() > 0) {
		output[0].ToLong(&size);
		size = (size + 254)*2;
	}
	long freeSpace = GetFreeSpaceOn(wxFileName(isoFile).GetPath());
	if (size > freeSpace && freeSpace == GetFreeSpaceOn(cache->GetTempDir())) {
		progressDlg->AddDetailMsg(_("There is not enough space to store ISO: cache emptying."));
		cache->Clear();
	}
	progressDlg->AddSummaryMsg(_("Creating ISO image"));
	progressDlg->SetSubSteps(100);
	cmd = s_config.GetIsoCmd();
	wxString label = ClipDiscLabel(dvd->GetLabel());
	label.Replace(wxT("\""), wxT("\\\""));
	cmd.Replace(_T("$VOL_ID"), label);
	cmd.Replace(_T("$DIR"), dvdOutDir.Mid(0, dvdOutDir.length() - 1));
	cmd.Replace(_T("$FILE"), isoFile);
	ProgressExecute exec(progressDlg, wxT(".*"));
	if (!exec.Execute(cmd)) {
		progressDlg->Failed();
		return false;
	}
	progressDlg->IncStep();
	return true;
}

/** Returns true, if process need be executed */
bool ProcessIsoImage::IsNeedExecute() {
#ifdef __WXMAC__
	return burnDlg->DoCreateIso() || burnDlg->DoBurn();
#else
	return burnDlg->DoCreateIso() || (burnDlg->DoBurn() && burnDlg->DoAddECC());
#endif
}
