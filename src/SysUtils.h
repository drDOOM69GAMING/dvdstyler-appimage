/////////////////////////////////////////////////////////////////////////////
// Name:        Utils.h
// Purpose:     System utilities
// Author:      Alex Thuering
// Created:		05.10.2015
// RCS-ID:      $Id: SysUtils.h,v 1.1 2015/11/29 17:27:05 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef SYS_UTILS_H
#define SYS_UTILS_H

#include <wx/combobox.h>

#ifdef __WXMAC__
extern "C++" {
	void EnumDevices(wxArrayString& devices, wxComboBox* deviceChoice);
	wxString GetDeviceName(const wxString& ioRegistryEntryPath);
	wxString GetDeviceNode(const wxString& ioRegistryEntryPath);
	bool IsMediaPresent(const wxString& ioRegistryEntryPath);
	bool IsMediaErasable(const wxString& ioRegistryEntryPath);
	bool IsMediaBlank(const wxString& ioRegistryEntryPath);
	long GetMediaSpaceFree(const wxString& ioRegistryEntryPath);
}
#else
	void EnumDevices(wxArrayString& devices, wxComboBox* deviceChoice);
#endif

/** Maximum length of the disc label (volume identifier) accepted by the ISO 9660
  * mastering tools (mkisofs/growisofs -V). The UDF primary volume descriptor
  * (Blu-ray) can hold 32 characters as well, but needs one byte for the dstring
  * compression id, so DISC_LABEL_LEN_UDF is smaller. */
const size_t DISC_LABEL_LEN_ISO = 32;
const size_t DISC_LABEL_LEN_UDF = 30;

/** Returns the disc label cleaned up for use as a volume identifier and clipped
  * to maxLen characters, so that ISO creation cannot fail with
  * "mkisofs: Volume ID string too long". Characters that are not allowed in a
  * volume identifier are replaced, leading/trailing spaces and dots are removed
  * and "DVD" is returned if nothing usable is left. */
wxString ClipDiscLabel(const wxString& label, size_t maxLen = DISC_LABEL_LEN_ISO);

/** Maximum length of a title name shown in the title list and used to build the
  * disc label. Kept in sync with the volume identifier limit so that a title
  * taken from a long file name can never produce an over-long disc label. */
const size_t TITLE_NAME_LEN = DISC_LABEL_LEN_ISO;

/** Shortens a title name to maxLen characters, appending an ellipsis when the
  * name had to be cut, so that long file names neither overflow the title list
  * nor produce a disc label that the mastering tools reject. */
wxString ClipTitleName(const wxString& name, size_t maxLen = TITLE_NAME_LEN);

/** Checks whether the DRM render node (/dev/dri/renderD128) is there. Note that
  * wxFileExists() must not be used for it: the node is a character device and
  * wxFileExists() only accepts regular files, which made VA-API look
  * unavailable on every system. */
bool HasVaapiRenderNode();

/** Checks whether the VA-API H.264 encoder can actually be used: a render node
  * must exist, the driver must open it and ffmpeg must produce frames with the
  * same hardware upload path the Blu-ray/AVCHD pipeline uses. Compiled-in
  * encoder support alone is not enough, so this is a real (cached) probe.
  * Always false on platforms without VA-API. */
bool IsVaapiH264EncodeAvailable();

/** Reason why the VA-API encode probe failed (empty while it succeeds). */
const wxString& GetVaapiEncodeProbeError();

/** True when HD titles should be encoded with the VA-API hardware encoder:
  * either requested explicitly, or - the default - when the probe shows that
  * the driver can really encode. */
bool UseVaapiH264Encode();

/** System Manager interface */
class SystemManager {
public:
	/** Checks if user has a privileges to stop (ie. shutdown) the computer system. */
	static bool CanStop();

	/** Initiates a request to stop (ie. shutdown) the computer system. */
	static void Stop();
};

#endif // UTILS_H
