/////////////////////////////////////////////////////////////////////////////
// Name:        About.h
// Purpose:     About dialog
// Author:      Alex Thuering
// Created:     6.11.2003
// RCS-ID:      $Id: About.h,v 1.29 2015/09/22 19:24:53 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/html/htmlwin.h>
#include "Version.h" //APP_VERSION and APP_COPYRIGHT_YEARS

const wxString APP_MAINT		= wxString(wxT("Alex Thüring"), wxConvUTF8);
const wxString APP_NAME			= _T("DVDStyler");
const wxString APP_LICENCE		= _T("GPL");
const wxString APP_COPYRIGHT	= wxT("(c) ") + wxString(wxT(APP_COPYRIGHT_YEARS)) + wxT(" ") + APP_MAINT;
const wxString APP_THANKS		= _T("drDOOM69GAMING");
const wxString APP_WEBSITE		= _T("http://www.dvdstyler.org");

class About: public wxDialog {
public:
	About (wxWindow *parent);
    
private:
	void OnLinkClicked(wxHtmlLinkEvent& event);
	void OnChangeLog(wxCommandEvent& event);
	DECLARE_EVENT_TABLE()
};
