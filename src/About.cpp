/////////////////////////////////////////////////////////////////////////////
// Name:        About.cpp
// Purpose:     About dialog
// Author:      Alex Thuering
// Created:     6.11.2003
// RCS-ID:      $Id: About.cpp,v 1.84 2016/05/08 17:06:49 ntalex Exp $
// Copyright:  (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "About.h"
#include "Authors.h"
#include <wxVillaLib/utils.h>
#include <wx/statline.h>
#include <wx/notebook.h>
#include <wx/hyperlink.h>
#include <wx/utils.h>
#include "rc/logo.png.h"

BEGIN_EVENT_TABLE(About, wxDialog)
	EVT_HTML_LINK_CLICKED(wxID_ANY, About::OnLinkClicked)
END_EVENT_TABLE()

wxString FixEmail(const wxString& str) {
	wxString result = str;
	result.Replace(wxT(" at "), wxT("@"));
	result.Replace(wxT("<"), wxT("&lt;"));
	result.Replace(wxT(">"), wxT("&gt;"));
	return result;
}

const wxString DOOMS_CHANGELOG = wxT(
	"DVDStyler ") wxT(APP_VERSION_STR) wxT(" AppImage - DOOMS Change-Log\n\n"
	"Changes since the original 3.3b4:\n\n"
	"1. Default temp directory is now ~/.dvdstyler instead of /tmp\n"
	"   (prevents failed encodes from filling up system space).\n\n"
	"2. New \"Create DVD without menus\" option added.\n\n"
	"3. New post-encode prompt: when encoding finishes you can choose\n"
	"   to Burn the DVD, Create an ISO image, Burn and create ISO,\n"
	"   or Nothing.\n\n"
	"4. VA-API hardware video decoding of source files is now used\n"
	"   when available (toggle in Settings > Generate:\n"
	"   \"Use hardware video decoding (VA-API)\").\n\n"
	"5. Dark-mode aware launcher: the app now follows the system\n"
	"   color scheme automatically.\n\n"
	"6. About dialog updated: copyright year 2003-2026 and a\n"
	"   special thanks line added.\n\n"
	"7. The \"No template\" button in the template dialog now reads\n"
	"   \"No menu\".\n\n"
	"8. Blu-ray (BD-R/BD-RE) and AVCHD (DVD-R/RW) authoring: a new\n"
	"   \"HD output\" mode in Settings > System core switches the pipeline\n"
	"   to full 1080p H.264 encodes, authored to BDMV with tsMuxeR and\n"
	"   packaged as a UDF 2.50 ISO (bundled pure-Python builder). AVCHD uses Blu-ray High@4.0\n"
	"   constraints with a 1s GOP and 18 Mbit/s max rate so it fits on\n"
	"   DVD media (high-fps sources become 720p50/60); Blu-ray uses\n"
	"   High@4.1 with a 2s GOP and 40 Mbit/s max rate.\n\n"
	"9. HD quality profiles (Standard/High/High+) trade encode speed for\n"
	"   quality by adjusting the B-frame and reference-frame counts.\n\n"
	"10. Settings refactored into a validated, layered model (app/video/\n"
	"    audio/subtitles/menu/disc/encode/output sections) with clamping\n"
	"    validation and dependency handling: switching the HD mode\n"
	"    re-clamps the applicable bitrates and enables only the controls\n"
	"    that apply.\n\n"
	"11. Version bumped from 3.3b4 to 3.3b6.\n\n"
	"12. VSO-style output folders: a \"CopyTo\" project property lets you\n"
	"    author straight into a folder layout ready for VSO (CopyTo\n"
	"    folder/VIDEO_TS for DVD, BDMV for Blu-ray), then burn with VSO.\n\n"
	"13. Media-size presets (4.7G/8.5G/25G/50G) with capacity checking,\n"
	"    so you can target DVD5/DVD9/BD25/BD50 output.\n\n"
	"14. All 14 common video formats added to the Format list; SD formats\n"
	"    keep the classic DVD pipeline and HD formats (720p/1080i/1080p)\n"
	"    use the Blu-ray pipeline.\n\n"
	"15. Auto-named projects: when a new project starts from a media file\n"
	"    the default disc label is applied, so you do not have to rename\n"
	"    the title by hand.\n\n"
	"16. Temp directory moved to Documents/DVDStyler/temp (was ~/.dvdstyler)\n"
	"    and folder permissions fixed (folders created as 755, not 001).\n\n"
	"17. VA-API hardware video encoding (h264_vaapi) added as an optional\n"
	"    toggle (Settings > Generate: \"Use hardware video encoding\").\n"
	"    Faster than the CPU x264 encode but not strictly Blu-ray compliant;\n"
	"    disabled by default.\n\n"
	"18. Encoding progress bar fixed: it now parses modern ffmpeg output\n"
	"    (frame=NNN with no space) so progress advances during encoding.\n\n"
	"19. Version bumped to 3.3b7.\n"
	"20. Genuine UDF 2.50 AVCHD/Blu-ray ISOs (udf250 builder); excludes the\n"
	"    tsMuxeR CERTIFICATE tree for AVCHD playback compatibility.\n"
	"21. Version bumped to 3.3b8.\n"
	"22. HD output always authored at 1920x1080 with the NTSC/PAL standards\n"
	"    frame rate (film 23.976, 29.97/59.94 recordings 29.97, 25/50 25);\n"
	"    removed the non-standard 720p50/60 AVCHD branch that PS4 and\n"
	"    standalone players rejected.\n"
	"23. Version bumped to 3.3b9.\n"
);

About::About(wxWindow* parent): wxDialog(parent, -1, wxEmptyString,
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    // sets the application icon
    SetTitle(_("About ..."));
	
	wxNotebook* notebook = new wxNotebook(this, -1);
	wxPanel* aboutPanel = new wxPanel(notebook, -1);
	notebook->AddPage(aboutPanel, _("About"));
	wxBoxSizer* aboutSizer = new wxBoxSizer(wxVERTICAL);
	aboutPanel->SetAutoLayout(true);
	aboutPanel->SetSizer(aboutSizer);
	
    // about info
    wxGridSizer* aboutinfo = new wxFlexGridSizer(2, 3, 3);
    aboutinfo->Add(new wxStaticText(aboutPanel, -1, _("Written by: ")), 0, wxALIGN_LEFT);
    aboutinfo->Add(new wxStaticText(aboutPanel, -1, APP_MAINT), 1, wxEXPAND | wxALIGN_LEFT);
    aboutinfo->Add(new wxStaticText(aboutPanel, -1, _("Version: ")), 0, wxALIGN_LEFT);
    aboutinfo->Add(new wxStaticText(aboutPanel, -1, APP_VERSION), 1, wxEXPAND | wxALIGN_LEFT);
    aboutinfo->Add(new wxStaticText(aboutPanel, -1, _("Licence type: ")), 0, wxALIGN_LEFT);
    aboutinfo->Add(new wxStaticText(aboutPanel, -1, APP_LICENCE), 1, wxEXPAND | wxALIGN_LEFT);
    aboutinfo->Add(new wxStaticText(aboutPanel, -1, _("Copyright: ")), 0, wxALIGN_LEFT);
    aboutinfo->Add(new wxStaticText(aboutPanel, -1, APP_COPYRIGHT), 1, wxEXPAND | wxALIGN_LEFT);
    aboutinfo->Add(new wxStaticText(aboutPanel, -1, _("Special thanks: ")), 0, wxALIGN_LEFT);
    aboutinfo->Add(new wxStaticText(aboutPanel, -1, APP_THANKS), 1, wxEXPAND | wxALIGN_LEFT);

    // about title/info
    wxBoxSizer* abouttext = new wxBoxSizer(wxVERTICAL);
    wxStaticText* appname = new wxStaticText(aboutPanel, -1, APP_NAME);
    appname->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    abouttext->Add(appname, 0, wxALIGN_LEFT);
    abouttext->Add(0, 10);
    abouttext->Add(aboutinfo, 1, wxEXPAND);

    // about icontitle//info
    wxBoxSizer* aboutpane = new wxBoxSizer(wxHORIZONTAL);
    wxBitmap bitmap = wxBITMAP_FROM_MEMORY(logo);
    aboutpane->Add(new wxStaticBitmap(aboutPanel, -1, bitmap), 0, wxALIGN_LEFT);
    aboutpane->Add(10, 0);
    aboutpane->Add(abouttext, 1, wxEXPAND);
	
    // about description
    aboutSizer->Add(aboutpane, 0, wxEXPAND | wxALL, 10);
    aboutSizer->Add(new wxStaticText(aboutPanel, -1,
	  _("DVDStyler is a crossplatform authoring system for Video DVD production.")),
	  0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 10);
    aboutSizer->Add(0, 6);
    wxHyperlinkCtrl* website = new wxHyperlinkCtrl(aboutPanel, wxID_ANY, APP_WEBSITE, APP_WEBSITE);
    wxString url = APP_WEBSITE;
    website->SetURL(url);
    aboutSizer->Add(website, 0, wxALIGN_CENTER);
    
    // support
    wxHtmlWindow* supportPanel = new wxHtmlWindow(notebook, -1,
    		wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHW_SCROLLBAR_AUTO);
    wxString page = wxT("<html><body>");
    page += wxT("<h5>") + wxString(_("Forum")) + wxT("</h5>");
    page += wxString::Format(_("Please use %sDVDStyler forum%s to get support, ask questions, or discuss this software."),
    		wxT("<a href=\"http://sourceforge.net/p/dvdstyler/discussion/318795/\" target=\"_blank\">"), wxT("</a>"));
    page += wxT("<h5>") + wxString(_("WIKI")) + wxT("</h5>");
    page += wxString::Format(_("Some documentation and %sFAQ%s you can find in %sDVDStyler WIKI%s or you can publish there your comments or small guides."),
			wxT("<a href=\"http://sourceforge.net/p/dvdstyler/wiki/Home/\" target=\"_blank\">"), wxT("</a>"),
			wxT("<a href=\"http://sourceforge.net/p/dvdstyler/wiki/Home/\" target=\"_blank\">"), wxT("</a>"));
    page += wxT("<h5>") + wxString(_("Bugs & RFE")) + wxT("</h5>");
    page += wxString::Format(_("Please use %sSourceforge Bugtracing system%s to report bug and %sSourceforge RFE system%s to submit a new feature request."),
    		wxT("<a href=\"http://sourceforge.net/p/dvdstyler/bugs/\" target=\"_blank\">"), wxT("</a>"),
    		wxT("<a href=\"http://sourceforge.net/p/dvdstyler/feature-requests/\" target=\"_blank\">"), wxT("</a>"));
    page += wxT("</body></html>");
    supportPanel->SetPage(page);
    notebook->AddPage(supportPanel, _("Support"));
	
	// authors
	wxHtmlWindow* authorsPanel = new wxHtmlWindow(notebook, -1,
	  wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHW_SCROLLBAR_AUTO);
	page = _T("<html><body>");
	page += _T("<table width='100%' cellspacing='2' cellpadding='2' border='0' valign='top'>");
	
	page += _T("<tr><td colspan='3'><b>") + wxString(_("Author and Maintainer")) + _T("</b></td></tr>");
	page += _T("<tr><td>&nbsp;</td><td colspan='2'>");
	page += FixEmail(s_author);
	page += _T("</td></tr>");
	
	page += _T("<tr><td colspan='3'><b>") + wxString(_("Doc Writer")) + _T("</b></td></tr>");
	page += _T("<tr><td>&nbsp;</td><td colspan='2'>");
	page += FixEmail(s_docWriter);
	page += _T("</td></tr>");
	
	page += _T("<tr><td colspan='3'><b>") + wxString(_("Packager (.deb)")) + _T("</b></td></tr>");
	page += _T("<tr><td>&nbsp;</td><td colspan='2'>");
	page += FixEmail(s_packager);
	page += _T("</td></tr>");
	
	page += _T("<tr><td colspan='3'><b>") + wxString(_("Translations")) + _T("</b></td></tr>");
	
	for (unsigned int i = 0; i < sizeof(s_translations)/sizeof(s_translations[0]); i++)
		page += wxT("<tr><td>&nbsp;</td><td>") + s_translations[i][0] + wxT("</td><td width='100%'>")
				+ FixEmail(s_translations[i][1]) + wxT("</td></tr>");
	
	page += _T("<tr><td colspan='3'><b>") + wxString(_("Libraries and Tools")) + _T("</b></td></tr>");
	page += _T("<tr><td>&nbsp;</td><td>wxWidgets</td><td>Julian Smart, Robert Roebling and other</td></tr>");
	page += _T("<tr><td>&nbsp;</td><td>dvdauthor</td><td>Scott Smith</td></tr>");
	page += _T("<tr><td>&nbsp;</td><td>cdrtools</td><td>Joerg Schilling</td></tr>");
	page += _T("<tr><td>&nbsp;</td><td>dvd+rw-tools</td><td>Andy Polyakov</td></tr>");
	
	page += _T("</table>");
	page += _T("</body></html>");
	authorsPanel->SetPage(page);
	notebook->AddPage(authorsPanel, _("Authors"));
	
	// licence
	wxHtmlWindow* licencePanel = new wxHtmlWindow(notebook, -1,
	  wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHW_SCROLLBAR_AUTO);
	page = _T("<html><body>");
	page += _T("<p>");
	page += _("DVDStyler is <a href='http://www.gnu.org/philosophy/free-sw.html'>free software</a> \
distributed under <a href='http://www.gnu.org/copyleft/gpl.html'>GNU General Public License (GPL)</a>. \
Please visit those sites for details of each agreement.");
	page += _T("</p>");
	page += _T("<p>");
	page += _("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, \
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS \
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, \
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR \
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.");
	page += _T("</p>");
	page += _T("</body></html>");
	licencePanel->SetPage(page);
	notebook->AddPage(licencePanel, _("Licence"));
	
	// buttons
	wxBoxSizer* totalpane = new wxBoxSizer(wxVERTICAL);
	totalpane->Add(notebook, 1, wxEXPAND|wxALL, 6);
    wxBoxSizer* buttonPane = new wxBoxSizer(wxHORIZONTAL);
    wxButton* changeLogButton = new wxButton(this, wxID_ANY, _("DOOMS Change-Log"));
    changeLogButton->Bind(wxEVT_BUTTON, &About::OnChangeLog, this);
    buttonPane->Add(changeLogButton, 0, wxALIGN_CENTER|wxRIGHT, 5);
    wxButton* okButton = new wxButton(this, wxID_OK, _("OK"));
    okButton->SetDefault();
    okButton->SetFocus();
    buttonPane->Add(okButton, 0, wxALIGN_CENTER);
    totalpane->Add(buttonPane, 0, wxALIGN_CENTER|wxALL, 10);
	
    SetSizerAndFit(totalpane);
	Center();

    ShowModal();
}

void About::OnLinkClicked(wxHtmlLinkEvent& event) {
	wxLaunchDefaultBrowser(event.GetLinkInfo().GetHref());
}

void About::OnChangeLog(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg(this, -1, _("DOOMS Change-Log"), wxDefaultPosition, wxSize(600, 420));
	wxBoxSizer* dlgSizer = new wxBoxSizer(wxVERTICAL);
	wxTextCtrl* text = new wxTextCtrl(&dlg, -1, DOOMS_CHANGELOG,
			wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	text->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	dlgSizer->Add(text, 1, wxEXPAND | wxALL, 8);
	wxButton* closeBtn = new wxButton(&dlg, wxID_OK, _("Close"));
	closeBtn->SetDefault();
	dlgSizer->Add(closeBtn, 0, wxALIGN_CENTER | wxBOTTOM, 10);
	dlg.SetSizer(dlgSizer);
	dlg.CenterOnParent();
	dlg.ShowModal();
}
