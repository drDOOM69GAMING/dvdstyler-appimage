/////////////////////////////////////////////////////////////////////////////
// Name:        ProcessCleanTemp.cpp
// Author:      Alex Thuering
// Created:     4.10.2014 (refactored)
// RCS-ID:      $Id: ProcessCleanTemp.cpp,v 1.3 2016/12/11 10:06:31 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "ProcessCleanTemp.h"
#include "ProcessIsoImage.h"
#include <wx/file.h>
#include <wx/dir.h>
#include <wx/log.h>
#include <wx/arrstr.h>

/** Constructor */
ProcessCleanTemp::ProcessCleanTemp(ProgressDlg* progressDlg, const wxString& tmpDir, const wxString& dvdTmpDir,
		const wxString& dvdOutDir): Process(progressDlg) {
	this->tmpDir = tmpDir;
	this->dvdTmpDir = dvdTmpDir;
	this->dvdOutDir = dvdOutDir;
}

/** Returns true, if process need be executed */
bool ProcessCleanTemp::IsNeedExecute() {
	return true;
}

/** Returns true, if gauge need be updated */
bool ProcessCleanTemp::IsUpdateGauge() {
	return false;
}

/** Executes process */
bool ProcessCleanTemp::Execute() {
	if (wxDir::Exists(tmpDir) && !DeleteTempFiles(true)) {
		progressDlg->Failed(wxT(""));
		return false;
	}
	
	// create temporary directories
	if (!wxDir::Exists(tmpDir) && !wxMkdir(tmpDir)) {
		progressDlg->Failed(wxString::Format(_("Can't create directory '%s'"), tmpDir.c_str()));
		return false;
	}
	if (!wxDir::Exists(dvdTmpDir) && !wxMkdir(dvdTmpDir)) {
		progressDlg->Failed(wxString::Format(_("Can't create directory '%s'"), dvdTmpDir.c_str()));
		return false;
	}
	if (!wxDir::Exists(dvdOutDir) && !wxMkdir(dvdOutDir)) {
		progressDlg->Failed(wxString::Format(_("Can't create directory '%s'"), dvdOutDir.c_str()));
		return false;
	}
	return true;
}

bool ProcessCleanTemp::DeleteDir(wxString dir) {
	if (dir.Last() != wxFILE_SEP_PATH)
		dir += wxFILE_SEP_PATH;
	wxDir d(dir);
	if (d.IsOpened()) {
		wxArrayString entries;
		wxString fname;
		while (d.GetFirst(&fname, wxEmptyString, wxDIR_FILES | wxDIR_DIRS | wxDIR_HIDDEN))
			entries.Add(dir + fname);
		d.Close();
		for (unsigned int i = 0; i < entries.Count(); i++) {
			if (wxDirExists(entries[i])) {
				if (!DeleteDir(entries[i]))
					return false;
			} else if (!DeleteFile(entries[i])) {
				return false;
			}
		}
		wxLogNull log;
		wxRmdir(dir);
	}
	return true;
}

bool ProcessCleanTemp::DeleteTempFiles(bool deleteOutDir) {
	progressDlg->AddDetailMsg(_("Cleaning temporary directory"));
	if (wxDirExists(dvdTmpDir) && !DeleteDir(dvdTmpDir)) {
		wxLogError(_("Cannot remove directory '%s'"), dvdTmpDir.c_str());
		return false;
	}
	if (deleteOutDir && wxDirExists(dvdOutDir)) {
		if (!DeleteDir(dvdOutDir + wxT("VIDEO_TS"))) {
			return false;
		}
		if (!DeleteDir(dvdOutDir + wxT("AUDIO_TS"))) {
			return false;
		}
		if (!DeleteDir(dvdOutDir + wxT("BDMV"))) {
			return false;
		}
		if (!DeleteDir(dvdOutDir + wxT("CERTIFICATE"))) {
			return false;
		}
		if (!wxRmdir(dvdOutDir)) {
			wxLogError(_("Cannot remove directory '%s'"), dvdOutDir.c_str());
			return false;
		}
	}
	if (wxFileExists(tmpDir + TMP_ISO) && !DeleteFile(tmpDir + TMP_ISO))
		return false;
	return true;
}

