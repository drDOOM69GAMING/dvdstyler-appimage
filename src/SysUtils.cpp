/////////////////////////////////////////////////////////////////////////////
// Name:        Utils.h
// Purpose:     Miscellaneous utilities
// Author:      Alex Thuering
// Created:		06.04.2008
// RCS-ID:      $Id: SysUtils.cpp,v 1.6 2016/07/04 21:10:43 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "SysUtils.h"
#include "Config.h"
#include <wx/dir.h>
#include <wx/progdlg.h>
#include <wx/log.h>

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#include <windows.h>
#include <reason.h>
#elif defined(HAVE_LIBDBUS)
#include <dbus/dbus.h>
#endif

#ifdef HAVE_LIBUDEV
extern "C" {
#include <libudev.h>
}
#elif defined(__WXMSW__)
#include <initguid.h>
#include <devguid.h>
#include <setupapi.h>
#ifdef _WIN64
	#include <ntdddisk.h>
	#include <ntddscsi.h>
	#include <cfgmgr32.h>
#else
	#include <ddk/ntdddisk.h>
	#include <ddk/ntddscsi.h>
	#include <ddk/cfgmgr32.h>
#endif
#include <wx/msw/registry.h>
#endif

/** Characters that are not allowed in an ISO 9660 volume identifier. */
static const wxChar VOLUME_ID_INVALID_CHARS[] = wxT("/\\*?\"<>|:");

wxString ClipDiscLabel(const wxString& label, size_t maxLen) {
	wxString result;
	for (size_t i = 0; i < label.length() && result.length() < maxLen; i++) {
		wxChar c = label[i];
		if (c < 0x20 || c == 0x7f) // control characters
			continue;
		if (wxStrchr(VOLUME_ID_INVALID_CHARS, c))
			c = wxT('_');
		result += c;
	}
	result.Trim(false);
	result.Trim(true);
	while (result.length() > 0 && result.Last() == wxT('.'))
		result.RemoveLast();
	return result.length() ? result : wxT("DVD");
}

wxString ClipTitleName(const wxString& name, size_t maxLen) {
	wxString result = name;
	result.Trim(false);
	result.Trim(true);
	if (result.length() <= maxLen)
		return result;
	if (maxLen <= 3)
		return result.Left(maxLen);
	return result.Left(maxLen - 3) + wxT("...");
}

/** Checks if user has a privileges to stop (ie. shutdown) the computer system. */
bool SystemManager::CanStop() {
#ifdef __WXMSW__
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	// Get a token for this process.
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		wxLogError(wxT("Failed to get access rights token."));
		return false;
	}

	// Get the LUID for the shutdown privilege.
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;// one privilege to set
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// Get the shutdown privilege for this process.
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	return GetLastError() == 0;
#elif defined(HAVE_LIBDBUS)
	// initialise the errors
	DBusError err;
	dbus_error_init(&err);

	// connect to the bus
	DBusConnection* conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "Connection Error (%s)\n", err.message);
		dbus_error_free(&err);
	}
	if (conn == NULL) {
		fprintf(stderr, "Failed to get dbus connection.\n");
		return false;
	}

	DBusMessage* msg = dbus_message_new_method_call("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager",
			"org.freedesktop.ConsoleKit.Manager", "CanStop");
	if (msg == NULL) {
		fprintf(stderr, "DBus Message Null\n");
		return false;
	}

	// send message and get a handle for a reply
	DBusPendingCall* pending;
	if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) { // -1 is default timeout
		fprintf(stderr, "DBus Out Of Memory!\n");
		return false;
	}
	if (pending == NULL) {
		fprintf(stderr, "DBus Pending Call is null\n");
		return false;
	}
	dbus_connection_flush(conn);

	// block until we receive a reply
	dbus_pending_call_block(pending);

	// get the reply message
	msg = dbus_pending_call_steal_reply(pending);
	if (msg == NULL) {
		fprintf(stderr, "DBus Reply is null\n");
		return false;
	}
	// free the pending message handle
	dbus_pending_call_unref(pending);

	// read result
	bool result = false;
	DBusMessageIter args;
	if (!dbus_message_iter_init(msg, &args)) {
		fprintf(stderr, "DBus reply message has no arguments!\n");
	} else if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_BOOLEAN) {
		dbus_message_iter_get_basic(&args, &result);
	} else if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_STRING) {
		const char* msg;
		dbus_message_iter_get_basic(&args, &msg);
		fprintf(stderr, "DBus reply: %s\n", msg);
	} else
		fprintf(stderr, "DBus reply argument is not boolean!\n");

	// free message
	dbus_message_unref(msg);

	return result;
#else
	return false;
#endif
}

/** Initiates a request to stop (ie. shutdown) the computer system. */
void SystemManager::Stop() {
#ifdef __WXMSW__
	if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM |
					SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED))
	return;
#elif defined(HAVE_LIBDBUS)
	// initialise the errors
	DBusError err;
	dbus_error_init(&err);

	// connect to the bus
	DBusConnection* conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "Connection Error (%s)\n", err.message);
		dbus_error_free(&err);
	}
	if (conn == NULL) {
		fprintf(stderr, "Failed to get dbus connection.\n");
		return;
	}

	DBusMessage* msg = dbus_message_new_method_call("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager",
			"org.freedesktop.ConsoleKit.Manager", "Stop");
	if (msg == NULL) {
		fprintf(stderr, "Message Null\n");
		return;
	}

	// send message and get a handle for a reply
	DBusPendingCall* pending;
	if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) { // -1 is default timeout
		fprintf(stderr, "Out Of Memory!\n");
		return;
	}
	if (pending == NULL) {
		fprintf(stderr, "Pending Call Null\n");
		return;
	}
	dbus_connection_flush(conn);

	// free message
	dbus_message_unref(msg);
#endif
}



#ifdef HAVE_LIBUDEV
void EnumDevices(wxArrayString& devices, wxComboBox* deviceChoice) {
	udev* udev = udev_new();
	if (!udev) {
		wxLogError(wxT("Can't create udev"));
	} else {
		udev_enumerate* enumerate = udev_enumerate_new(udev);
		udev_enumerate_add_match_property(enumerate, "ID_CDROM_DVD", "1");
		udev_enumerate_scan_devices(enumerate);
		udev_list_entry* devs = udev_enumerate_get_list_entry(enumerate);
		udev_list_entry* dev_list_entry;
		udev_list_entry_foreach(dev_list_entry, devs) {
			udev_device* dev = udev_device_new_from_syspath(udev, udev_list_entry_get_name(dev_list_entry));
			wxString devNode(udev_device_get_devnode(dev), *wxConvFileName);
			wxString devModel(udev_device_get_property_value(dev, "ID_MODEL"), wxConvISO8859_1);
			devices.Add(devNode);
			deviceChoice->Append(devModel);
		}
	}
}
#elif defined(__WXMSW__)
const int BUFFER_SIZE = 512;

wxString GetDriveString(WCHAR* deviceId) {
	wxString result;
	int nLength = wcslen(deviceId);
	deviceId[nLength] = '\\';
	deviceId[nLength + 1] = 0;
	// get volume mount point for the device path.
	WCHAR volume[BUFFER_SIZE];
	if (GetVolumeNameForVolumeMountPoint(deviceId, volume, BUFFER_SIZE)) {
		TCHAR bufd[BUFFER_SIZE];
		TCHAR drive[] = TEXT("C:\\");
		for (TCHAR c = TEXT('C'); c <= TEXT('Z');  c++) {
			drive[0] = c;
			if (GetVolumeNameForVolumeMountPoint(drive, bufd, BUFFER_SIZE) && !wcscmp(bufd, volume)) {
				result = wxString(drive, wxConvISO8859_1).Mid(0, 2);
				break;
			}
		}
	}
	deviceId[nLength] = 0;
	return result;
}

wxString GetProductId(HANDLE hDevice) {
	STORAGE_PROPERTY_QUERY query;
	query.PropertyId = StorageAdapterProperty;
	query.QueryType = PropertyStandardQuery;
	UCHAR outBuf[512];
	ULONG returnedLength = 0;
	bool status = DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(STORAGE_PROPERTY_QUERY),
			&outBuf, 512, &returnedLength, NULL);
	if (!status)
		return wxT("");
	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;
	status = DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(STORAGE_PROPERTY_QUERY),
			&outBuf, 512, &returnedLength, NULL);
	if (!status)
		return wxT("");
	PSTORAGE_DEVICE_DESCRIPTOR  devDesc = (PSTORAGE_DEVICE_DESCRIPTOR) outBuf;
	PUCHAR p = (PUCHAR) outBuf;
	if (devDesc->ProductIdOffset && p[devDesc->ProductIdOffset])
		return wxString((char*) (p + devDesc->ProductIdOffset), wxConvISO8859_1);
	return wxT("");
}

wxString GetProductId(const wxString& drive) {
	TCHAR volPath[MAX_PATH];
	if (GetVolumePathName(drive, volPath, sizeof(volPath))) {
		for (size_t cchVolPath = _tcslen(volPath);
				cchVolPath > 0 && volPath[cchVolPath - 1] == TEXT('\\');
				cchVolPath--) {
			volPath[cchVolPath - 1] = TEXT('\0');
		}

		TCHAR volPathWithLeadingPrefix[sizeof(volPath)];
		if (_sntprintf(volPathWithLeadingPrefix,
				sizeof(volPathWithLeadingPrefix),
				volPath[0] == _T('\\') ? _T("%s") : _T("\\\\.\\%s"),
				volPath) < (int) sizeof(volPathWithLeadingPrefix)) {
		    HANDLE hDevice = CreateFile(volPathWithLeadingPrefix, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
		    		NULL, OPEN_EXISTING, 0, NULL);
		    if (hDevice != INVALID_HANDLE_VALUE)
		    	return GetProductId(hDevice);
		}
	}
	return wxT("");
}

void EnumDevices(wxArrayString& devices, wxComboBox* deviceChoice) {
	wxString drive = wxT("C:");
	for (char c = 'C'; c <= 'Z';  c++) {
		drive[0] = wxChar(c);
		if (GetDriveType(drive) == DRIVE_CDROM) {
			devices.Add(drive);
			deviceChoice->Append(drive + wxT(" ") + GetProductId(drive));
		}
	}
}
#elif defined(__WXMAC__)
extern "C++" void EnumDevices(wxArrayString& devices, wxComboBox* deviceChoice);
#else
void EnumDevices(wxArrayString& devices, wxComboBox* deviceChoice) {
	wxArrayString allDevices;
	wxDir d(wxT("/dev"));
	wxString fname;
	if (d.GetFirst(&fname, wxT("dvd*"), wxDIR_FILES))
		do { allDevices.Add(wxT("/dev/") + fname); } while (d.GetNext(&fname));
	if (d.GetFirst(&fname, wxT("cdrom*"), wxDIR_FILES))
		do { allDevices.Add(wxT("/dev/") + fname); } while (d.GetNext(&fname));
	if (d.GetFirst(&fname, wxT("sg*"), wxDIR_FILES))
		do { allDevices.Add(wxT("/dev/") + fname); } while (d.GetNext(&fname));
	allDevices.Sort();
	// get device name
	wxString scanCmd = s_config.GetBurnScanCmd();
	if (scanCmd.length()) {
		wxProgressDialog* pdlg = new wxProgressDialog(_("Scan devices"), _("Please wait..."),
				allDevices.GetCount() - 1, deviceChoice->GetParent(), wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_CAN_ABORT);
		pdlg->Show();
		pdlg->SetFocus();
		for (unsigned int i = 0; i < allDevices.GetCount(); i++) {
			wxString cmd = scanCmd;
			cmd.Replace(wxT("$DEVICE"), allDevices[i]);
			wxArrayString output;
			wxExecute(cmd, output, wxEXEC_SYNC | wxEXEC_NODISABLE);
			if (output.Count() > 0 && output[0].length() > 7 && output[0].SubString(0, 7) == wxT("INQUIRY:")) {
				wxString dev = output[0].AfterFirst(wxT('[')).BeforeLast(wxT(']'));
				dev = allDevices[i] + wxT(" ") + dev.BeforeFirst(wxT(']')).Trim() + wxT(" ")
						+ dev.AfterFirst(wxT('[')).BeforeFirst(wxT(']')).Trim();
				devices.Add(allDevices[i]);
				deviceChoice->Append(dev);
			}
			if (!pdlg->Update(i))
				break;
		}
		pdlg->Hide();
	}
	if (devices.Count() == 0) {
		for (unsigned int i=0; i<allDevices.GetCount(); i++) {
			devices.Add(allDevices[i]);
			deviceChoice->Append(allDevices[i]);
		}
	}
}
#endif //!HAVE_LIBUDEV && !__WXMSW__
