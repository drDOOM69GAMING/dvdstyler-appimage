/////////////////////////////////////////////////////////////////////////////
// Name:        ProcessBluray.h
// Purpose:     Blu-ray (HD) authoring process
// Author:      drDOOM69GAMING
// Licence:     GPL
//
// Encodes every title to Blu-ray compliant H.264 / AC-3 elementary streams,
// multiplexes them into a BDMV folder structure with tsMuxeR and optionally
// builds a UDF 2.50 ISO image (the filesystem required by Blu-ray players).
/////////////////////////////////////////////////////////////////////////////

#ifndef DS_PROCESS_BLURAY_H
#define DS_PROCESS_BLURAY_H

#include "Process.h"

class DVD;
class Vob;
class BurnDlg;

/**
 * Implements the Blu-ray authoring process
 */
class ProcessBluray: public Process {
public:
	/** Constructor */
	ProcessBluray(ProgressDlg* progressDlg, DVD* dvd, BurnDlg* burnDlg, const wxString& dvdOutDir,
			const wxString& tmpDir);

	/** Executes process */
	virtual bool Execute();

	/** Returns true, if process need be executed */
	virtual bool IsNeedExecute();

private:
	DVD* dvd;
	BurnDlg* burnDlg;
	wxString dvdOutDir;
	wxString tmpDir;

	/** Returns the Blu-ray compliant frame rate for the given source fps */
	static double GetBdFps(double sourceFps);

	/** Returns the exact ffmpeg frame rate string for the given fps */
	static wxString GetFpsStr(double fps);

	/** Returns the GOP size (in frames) for the given fps and max GOP seconds (1 = AVCHD, 2 = Blu-ray) */
	static int GetGopSize(double fps, int seconds);

	/** Logs an estimated output size and warns if the content cannot fit the media */
	void CheckCapacity(int vobCount);

	/** Encodes one title to H.264 elementary stream and AC-3 audio */
	bool EncodeTitle(Vob* vob, int titleIdx, const wxString& workDir, wxString& videoFile, wxString& audioFile,
			double& fps, wxString& chapterList);

	/** Generates a tsMuxeR meta file for one title */
	bool SaveTsMuxeRMeta(const wxString& metaFile, const wxString& videoFile, const wxString& audioFile, double fps,
			const wxString& chapterList);

	/** Formats a time in seconds as HH:MM:SS.mmm */
	static wxString FormatTime(double seconds);

	/** Counts the playlists (mpls) already in the BDMV structure */
	static int CountPlaylists(const wxString& bdmvRoot);

	/** Runs tsMuxeR to create the BDMV folder structure */
	bool AuthorBdmv(const wxString& metaFile, const wxString& bdmvRoot);
};

/**
 * Implements the Blu-ray UDF 2.50 ISO image creation.
 * Runs after the "what to do now" dialog, so it can honor decisions made
 * after the encoding finished.
 */
class ProcessBlurayIso: public Process {
public:
	/** Constructor */
	ProcessBlurayIso(ProgressDlg* progressDlg, DVD* dvd, BurnDlg* burnDlg, const wxString& dvdOutDir,
			const wxString& tmpDir);

	/** Executes process */
	virtual bool Execute();

	/** Returns true, if process need be executed */
	virtual bool IsNeedExecute();

	/** Returns true, if gauge need be updated */
	virtual bool IsUpdateGauge() { return false; }

private:
	DVD* dvd;
	BurnDlg* burnDlg;
	wxString dvdOutDir;
	wxString tmpDir;

	/** Builds a UDF 2.50 ISO image */
	bool BuildIso(const wxString& isoFile);
};

#endif // DS_PROCESS_BLURAY_H
