/////////////////////////////////////////////////////////////////////////////
// Name:        ProcessExecute.h
// Author:      Alex Thuering
// Created:     27.09.2014 (refactored)
// RCS-ID:      $Id: ProcessExecute.h,v 1.10 2016/08/05 07:16:27 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef DS_PROCESS_EXECUTE_H
#define DS_PROCESS_EXECUTE_H

#include "Process.h"
#include <wxVillaLib/ProgramProcess.h>
#include <wxVillaLib/PipeExecute.h>
#include <wx/regex.h>
class wxProgressDialog;

class ProcessExecute: public wxPipeExecute {
public:
	ProcessExecute(ProgressDlg* progressDlg): m_progressDlg(progressDlg) {}
	virtual ~ProcessExecute() {};

	bool Execute(wxString command, wxString inputFile = wxEmptyString, wxString outputFile = wxEmptyString) {
		m_progressDlg->AddDetailMsg(_("Executing command: ") + command);
		return wxPipeExecute::Execute(command, inputFile, outputFile);
	}
	
	virtual void ProcessOutput(wxString line) {
		m_progressDlg->AddDetailText(line + _T("\n"));
	}

	virtual bool IsCanceled() {
		return m_progressDlg->WasCanceled();
	}

protected:
	ProgressDlg* m_progressDlg;
};

class ProgressExecute: public ProcessExecute {
public:
	ProgressExecute(ProgressDlg* process, wxString filter): ProcessExecute(process), m_percent(0) {
		m_percentPattern.Compile(wxT("(([0-9]+[\\.,][0-9]+)|([0-9]+))%"),wxRE_ICASE);
		m_blockPattern.Compile(wxT("([0-9]+)[[:space:]]+of[[:space:]]+([0-9]+)"),wxRE_ICASE);
		m_filterPattern.Compile(filter,wxRE_ICASE);
		m_initSubStep = m_progressDlg->GetSubStep();
	}
	virtual ~ProgressExecute() {};

	virtual void ProcessOutput(wxString line) {
		// get last output if program is using \b (remove \b at begin/end, then get text after last \b)
		while (line.at(0) == wxT('\b'))
			line.Remove(0, 1);
		while (line.Last() == wxT('\b'))
			line.RemoveLast(1);
		line = line.AfterLast(wxT('\b'));
		if (m_filterPattern.Matches(line)) {
			if (m_blockPattern.Matches(line)) {
				long blocks = 0;
				long totalBlocks = 0;
				long percent = 0;
				if (m_blockPattern.GetMatch(line, 1).ToLong(&blocks)
						&& m_blockPattern.GetMatch(line, 2).ToLong(&totalBlocks)) {
					percent = (totalBlocks > 0) ? (blocks * 100) / totalBlocks : 0;
					m_progressDlg->SetSubStep(m_initSubStep + (int) m_percent);
					if (percent >= m_percent) {
						m_percent += 5;
					} else {
						return;
					}
				}
			} else if (m_percentPattern.Matches(line)) {
				long percent = 0;
				wxString percentStr = m_percentPattern.GetMatch(line, 1);
				percentStr = percentStr.BeforeFirst(wxT('.')).BeforeFirst(wxT(','));
				if (percentStr.ToLong(&percent)) {
					m_progressDlg->SetSubStep(m_initSubStep + (int) percent);
					if (percent >= m_percent) {
						m_percent += 5;
					} else if (percent < m_percent - 5) {
						m_initSubStep += 100;
						m_percent = 5;
					} else {
						return;
					}
				}
			}
		}
		m_progressDlg->AddDetailText(line + _T("\n"));
	}

protected:
    wxRegEx m_percentPattern;
    wxRegEx m_blockPattern;
    wxRegEx m_filterPattern;
	int     m_initSubStep;
	int     m_percent;
};


class AVConvExecute: public ProcessExecute {
public:
	AVConvExecute(ProgressDlg* progressDlg, long totalFrames): ProcessExecute(progressDlg), m_percent(0),
			m_pattern(wxT("frame=[[:space:]]*([0-9]+).*")) {
		m_initSubStep = m_progressDlg->GetSubStep();
		m_totalFrames = totalFrames;
	}
	virtual ~AVConvExecute() {}

	virtual void ProcessOutput(wxString line) {
		if (line.Find(wxT("buffer underflow")) >= 0
				|| line.Find(wxT("packet too large, ignoring buffer limits")) >= 0
				|| line.Find(wxT("Last message repeated")) >= 0 // Last message repeated X times
				|| line.Find(wxT("Past duration")) >= 0
				|| line.Find(wxT("configuration:")) >= 0)
			return;
		if (m_totalFrames > 0 && m_pattern.Matches(line)) {
			long frame = 0;
			m_pattern.GetMatch(line, 1).ToLong(&frame);
			m_percent = (frame * 100) / m_totalFrames;
			m_progressDlg->SetSubStep(m_initSubStep + (int) m_percent);
		}
		
		if (line.StartsWith("frame=") && m_progressDlg->GetLastDetailText().StartsWith("frame=")) {
			m_progressDlg->ReplaceLastDetailText(line + _T("\n"));
		} else {
			m_progressDlg->AddDetailText(line + _T("\n"));
		}
	}

protected:
	long m_totalFrames;
	int m_initSubStep;
	int m_percent;
	wxRegEx m_pattern;
};

class AVConvTimeExecute: public ProcessExecute {
public:
	AVConvTimeExecute(ProgressDlg* progressDlg): ProcessExecute(progressDlg),
			m_pattern(wxT(".*time=([0-9:\\.]+).*")), m_trackGain(0), m_maxVolume(0), m_ok(false) {
	}
	virtual ~AVConvTimeExecute() {}

	virtual void ProcessOutput(wxString line) {
		if (line.Contains(wxT("track_gain ="))) {
			int n1 = line.Index(wxT("track_gain ="));
			int n2 = line.Index(wxT(" dB"));
			if (n1 > 0 && n2 > n1) {
				n1 += 13;
				if (line.Mid(n1, n2 - n1).ToDouble(&m_trackGain))
					m_ok = true;
				else
					wxLogError(wxT("Error parsing '%s'"), line.substr(n1, n2 - n1).c_str());
			}
		} else if (line.Contains(wxT("max_volume:"))) {
			int n1 = line.Index(wxT("max_volume:"));
			int n2 = line.Index(wxT(" dB"));
			if (n1 > 0 && n2 > n1) {
				n1 += 12;
				if (line.Mid(n1, n2 - n1).ToDouble(&m_maxVolume))
					m_ok = true;
				else
					wxLogError(wxT("Error parsing '%s'"), line.substr(n1, n2 - n1).c_str());
			}
		}
		m_progressDlg->AddDetailText(line + _T("\n"));
	}
	
	bool IsOk() {
		return m_ok;
	}
	
	double GetTrackGain() {
		return m_trackGain;
	}
	
	double GetMaxVolume() {
		return m_maxVolume;
	}

protected:
	wxRegEx m_pattern;
	double m_trackGain;
	double m_maxVolume;
	bool m_ok;
};

class AVConvTimeProgExecute: public ProgramProcess {
public:
	AVConvTimeProgExecute(wxProgressDialog* progressDlg): ProgramProcess(progressDlg),
			m_pattern(wxT(".*time=([0-9:\\.]+).*")), m_trackGain(0), m_maxVolume(0), m_ok(false) {
	}
	virtual ~AVConvTimeProgExecute() {}

	/** Processes output line */
	virtual void ProcessOutput(const wxString& line, bool errorStream) {
		if (m_pattern.Matches(line)) {
			wxString time = m_pattern.GetMatch(line, 1);
			progDlg->Pulse(_("Analysis of audio") + wxString(wxT(": ")) + time);
		} else if (line.Contains(wxT("track_gain ="))) {
			int n1 = line.Index(wxT("track_gain ="));
			int n2 = line.Index(wxT(" dB"));
			if (n1 > 0 && n2 > n1) {
				n1 += 13;
				if (line.Mid(n1, n2 - n1).ToDouble(&m_trackGain)) {
					m_ok = true;
				} else
					wxLogError(wxT("Error parsing '%s'"), line.substr(n1, n2 - n1).c_str());
			}
		} else if (line.Contains(wxT("max_volume:"))) {
			int n1 = line.Index(wxT("max_volume:"));
			int n2 = line.Index(wxT(" dB"));
			if (n1 > 0 && n2 > n1) {
				n1 += 12;
				if (line.Mid(n1, n2 - n1).ToDouble(&m_maxVolume)) {
					m_ok = true;
				} else
					wxLogError(wxT("Error parsing '%s'"), line.substr(n1, n2 - n1).c_str());
			}
		} else
			progDlg->Pulse();
	}
	
	bool IsOk() {
		return m_ok;
	}
	
	double GetTrackGain() {
		return m_trackGain;
	}
	
	double GetMaxVolume() {
		return m_maxVolume;
	}

protected:
	wxRegEx m_pattern;
	double m_trackGain;
	double m_maxVolume;
	bool m_ok;
};

class SpumuxExecute: public wxPipeExecute {
public:
	SpumuxExecute(ProgressDlg* progressDlg, double fileSize): m_progressDlg(progressDlg) {
		m_fileSize = fileSize;
		m_error = false;
		m_progressRegEx.Compile(wxT("^INFO: ([0-9]+) bytes"));
		m_initSubStep = m_progressDlg->GetSubStep();
		m_percent = 0;
	}
	virtual ~SpumuxExecute() {};

	bool Execute(wxString command, wxString inputFile = wxEmptyString, wxString outputFile = wxEmptyString) {
		m_progressDlg->AddDetailMsg(_("Executing command: ") + command);
		bool result = wxPipeExecute::Execute(command, inputFile, outputFile);
		m_progressDlg->SetSubStep(m_initSubStep + 50);
		return result && !m_error;
	}
	
	virtual void ProcessOutput(wxString line) {
		if (line.StartsWith(wxT("ERR:"))) {
			m_progressDlg->AddDetailMsg(line, *wxRED);
			m_error = m_error || line.StartsWith(wxT("ERR:  Cannot pick button masks"));
		} else if (m_progressRegEx.Matches(line)) {
			if (m_fileSize == -1)
				return;
			double bytes = 0;
			m_progressRegEx.GetMatch(line, 1).ToDouble(&bytes);
			int percent = (bytes * 100) / m_fileSize;
			m_progressDlg->SetSubStep(m_initSubStep + (int) percent/2);
			if (percent >= m_percent) {
				m_percent += 20;
				m_progressDlg->AddDetailText(line + _T("\n"));
			}
		} else if (!line.StartsWith(wxT("STAT:")))
			m_progressDlg->AddDetailText(line + _T("\n"));
	}

	virtual bool IsCanceled() {
		return m_progressDlg->WasCanceled();
	}

protected:
	ProgressDlg* m_progressDlg;
	double m_fileSize;
	bool m_error;
	wxRegEx m_progressRegEx;
	int m_initSubStep;
	int m_percent;
};

#endif // DS_PROCESS_EXECUTE_H
