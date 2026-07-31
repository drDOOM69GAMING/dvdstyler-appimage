/////////////////////////////////////////////////////////////////////////////
// Name:        ProcessAskOutput.cpp
// Purpose:     Ask after encoding whether to burn or create an ISO image
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "ProcessAskOutput.h"
#include "ProgressDlg.h"
#include "BurnDlg.h"
#include <wx/msgdlg.h>
#include <wx/choicdlg.h>

/** Constructor */
ProcessAskOutput::ProcessAskOutput(ProgressDlg* progressDlg, BurnDlg* burnDlg): Process(progressDlg) {
	this->burnDlg = burnDlg;
}

/** Returns true, if process need be executed */
bool ProcessAskOutput::IsNeedExecute() {
	return burnDlg->DoGenerate() && !burnDlg->DoCreateIso() && !burnDlg->DoBurn();
}

/** Returns true, if gauge need be updated */
bool ProcessAskOutput::IsUpdateGauge() {
	return false;
}

/** Executes process */
bool ProcessAskOutput::Execute() {
	if (progressDlg->WasCanceled())
		return false;
	progressDlg->AddSummaryMsg(_("Encoding finished."));
	wxArrayString choices;
	choices.Add(_("Burn to DVD"));
	choices.Add(_("Create ISO image"));
	choices.Add(_("Burn to DVD and create ISO image"));
	choices.Add(_("Nothing (just keep the generated files)"));
	wxSingleChoiceDialog dlg(progressDlg, _("Encoding is complete. What would you like to do now?"),
			_("DVD ready"), choices);
	if (dlg.ShowModal() != wxID_OK) {
		progressDlg->AddSummaryMsg(_("Aborted"), wxEmptyString, *wxRED);
		return false;
	}
	switch (dlg.GetSelection()) {
	case 0:
		burnDlg->SetBurn(true);
		break;
	case 1:
		burnDlg->SetCreateIso(true);
		break;
	case 2:
		burnDlg->SetBurn(true);
		burnDlg->SetCreateIso(true);
		break;
	default:
		break;
	}
	return true;
}
