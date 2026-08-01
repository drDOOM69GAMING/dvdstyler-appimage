/////////////////////////////////////////////////////////////////////////////
// Name:        ProcessDvdFilesystem.h
// Author:      Alex Thuering
// Created:     26.09.2014 (refactored)
// RCS-ID:      $Id: ProcessDvdFilesystem.cpp,v 1.4 2016/12/11 10:06:31 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "ProcessDvdFilesystem.h"
#include "ProcessExecute.h"
#include "DVD.h"
#include "Config.h"
#include <wx/dir.h>

class DVDAuthorExecute: public ProgressExecute {
public:
	DVDAuthorExecute(ProgressDlg* progressDlg, int totalSize): ProgressExecute(progressDlg, wxT(".*")),
			m_totalSize(totalSize), m_warnings(0), m_warnStep(1), m_dvdauthorStep(0) {
		if (m_totalSize == 0)
			m_totalSize++;
	}
	virtual ~DVDAuthorExecute() {};

	virtual void ProcessOutput(wxString line) {
		if (m_dvdauthorStep) {
			ProgressExecute::ProcessOutput(line);
			return;
		}
		if (line.StartsWith(wxT("STAT: fixed"))) {
			m_dvdauthorStep++;
			m_initSubStep += 200;
			m_progressDlg->SetSubStep(m_initSubStep);
		} else if (line.StartsWith(wxT("STAT: VOBU"))) {
			long size = 0;
			wxString sizeStr = line.BeforeLast(wxT(',')).AfterLast(wxT(' '));
			if (sizeStr.Mid(sizeStr.Length() - 2) == _T("MB") && sizeStr.Remove(sizeStr.Length() - 2).ToLong(&size))
				m_progressDlg->SetSubStep(m_initSubStep + (int) size * 200
						/ m_totalSize);
		} else if (line.Mid(0, 5) == _T("WARN:")) {
			m_warnings++;
			if (m_warnings > m_warnStep * 10)
				m_warnStep = m_warnStep * 10;
			else if (m_warnings % m_warnStep != 0)
				return;
		}
		if (line.Mid(0, 4) == wxT("ERR:"))
			m_progressDlg->AddDetailMsg(line, *wxRED);
		else
			m_progressDlg->AddDetailText(line + _T("\n"));
	}

protected:
	int m_totalSize;
	int m_warnings;
	int m_warnStep;
	int m_dvdauthorStep;
};


ProcessDvdFilesystem::ProcessDvdFilesystem(ProgressDlg* progressDlg, DVD* dvd, const wxString& dvdTmpDir,
		const wxString& dvdOutDir): Process(progressDlg) {
	this->dvd = dvd;
	this->dvdTmpDir = dvdTmpDir;
	this->dvdOutDir = dvdOutDir;
}
	
/** Returns true, if process need be executed */
bool ProcessDvdFilesystem::IsNeedExecute() {
	return true;
}

/** Executes process */
bool ProcessDvdFilesystem::Execute() {
	if (progressDlg->WasCanceled())
		return false;
	PrintProjectTranscodedInfo();
	wxString dvdauthFile = dvdTmpDir + _T("dvdauthor.xml");
	dvd->SaveDVDAuthor(dvdauthFile);
	progressDlg->AddSummaryMsg(_("Generating DVD"));
	progressDlg->SetSubSteps(300);
	wxString cmd = s_config.GetDvdauthorCmd();
	cmd.Replace(_T("$FILE_CONF"), dvdauthFile);
	cmd.Replace(_T("$DIR"), dvdOutDir.Mid(0, dvdOutDir.length() - 1));
	DVDAuthorExecute dvdauthorExec(progressDlg, dvd->GetSize(true) / 1024);
	if (!dvdauthorExec.Execute(cmd)) {
		progressDlg->Failed();
		return false;
	}
	// remove temp files
	if (s_config.GetRemoveTempFiles()) {
		wxDir d(dvdTmpDir);
		wxString fname;
		for (bool hasEntry = d.GetFirst(&fname, wxEmptyString, wxDIR_FILES); hasEntry;
				hasEntry = d.GetNext(&fname))
			DeleteFile(dvdTmpDir + fname);
	}
	progressDlg->IncStep();
	return true;
	
}

void ProcessDvdFilesystem::PrintProjectTranscodedInfo() {
	progressDlg->AddDetailMsg(wxT("========================="));
	progressDlg->AddDetailMsg(wxT("            | Result size"));
	for (int tsi = -1; tsi < (int) dvd->GetTitlesets().Count(); tsi++) {
		PgcArray& pgcArray = dvd->GetPgcArray(tsi, true);
		for (int pgci = 0; pgci < (int) pgcArray.Count(); pgci++) {
			Pgc* pgc = pgcArray[pgci];
			for (int vobi = 0; vobi < (int) pgc->GetVobs().Count(); vobi++) {
				Vob* vob = pgc->GetVobs()[vobi];
				PrintInfoLine(wxString::Format(tsi == -1 ? wxT("VMGM menu %d") : wxT("Menu %d"), pgci + 1),
						vob->GetTranscodedSize(dvd));
			}
		}
		if (tsi == -1)
			continue;
		Titles& titles = dvd->GetTitlesets()[tsi]->GetTitles();
		for (int pgci = 0; pgci < (int) titles.Count(); pgci++) {
			Pgc* pgc = titles[pgci];
			for (int vobi = 0; vobi < (int) pgc->GetVobs().Count(); vobi++) {
				Vob* vob = pgc->GetVobs()[vobi];
				wxString titleStr = wxString::Format(wxT("Title %d"), pgci + 1);
				if (vobi > 0)
					titleStr += wxString::Format(wxT("-%d"), vobi + 1);
				PrintInfoLine(titleStr, vob->GetTranscodedSize(dvd));
			}
		}
	}
	PrintInfoLine(wxT("Total"), dvd->GetSize(true));
	progressDlg->AddDetailMsg(wxT("========================="));
}

void ProcessDvdFilesystem::PrintInfoLine(const wxString& info, int transcodedSize) {
	wxString text = info.substr(0, 12);
	text.Append(wxT(' '), 12 - info.length()).Append(wxT("| "));
	// transcoded size
	wxString str = wxString::Format(wxT("%0.1f"), ((float) transcodedSize) / 1024) + wxT(" MB");
	text.Append(str);
	progressDlg->AddDetailMsg(text);
}
