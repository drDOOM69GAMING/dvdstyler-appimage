/////////////////////////////////////////////////////////////////////////////
// Name:        ProcessAskOutput.h
// Purpose:     Ask after encoding whether to burn or create an ISO image
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef DS_PROCESS_ASK_OUTPUT_H
#define DS_PROCESS_ASK_OUTPUT_H

#include "Process.h"

class BurnDlg;

/**
 * Implements the ask-output process
 */
class ProcessAskOutput: public Process {
public:
	/** Constructor */
	ProcessAskOutput(ProgressDlg* progressDlg, BurnDlg* burnDlg);

	/** Returns true, if process need be executed */
    virtual bool IsNeedExecute();

    /** Returns true, if gauge need be updated */
    virtual bool IsUpdateGauge();

	/** Executes process */
	virtual bool Execute();

private:
    BurnDlg* burnDlg;
};

#endif // DS_PROCESS_ASK_OUTPUT_H
