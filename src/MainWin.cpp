/////////////////////////////////////////////////////////////////////////////
// Name:        MainWin.cpp
// Purpose:     Main window
// Author:      Alex Thuering
// Created:     10.10.2003
// RCS-ID:      $Id: MainWin.cpp,v 1.194 2017/11/25 14:39:34 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include <src/ButtonEditDlg.h>
#include "MainWin.h"
#include "BurnDlg.h"
#include "ProgressDlg.h"
#include "About.h"
#include "NewProjectDlg.h"
#include "TemplateDlg.h"
#include "DVDPropDlg.h"
#include "SettingsDlg.h"
#include "Menu.h"
#include "Config.h"
#include "MPEG.h"
#include "MessageDlg.h"
#include <wxVillaLib/VerticalToolbar.h>
#include <wxVillaLib/SingleThumbnailDlg.h>
#include <wxVillaLib/utils.h>
#include <wx/imaglist.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/artprov.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/progdlg.h>
#include "math.h"

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#include "winutils.h"
#endif

#if defined(__WXMSW__) || defined(__WXMAC__)
#include "rc/new.png.h"
#include "rc/open.png.h"
#include "rc/save.png.h"
#define ICON_NEW wxBITMAP_FROM_MEMORY(new)
#define ICON_OPEN wxBITMAP_FROM_MEMORY(open)
#define ICON_SAVE wxBITMAP_FROM_MEMORY(save)
#else
#define ICON_NEW wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR)
#define ICON_OPEN wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR)
#define ICON_SAVE wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR)
#endif
#include "rc/dvdOptions.png.h"
#include "rc/settings.png.h"
#include "rc/run.png.h"
#include "rc/files.png.h"
#include "rc/addVideo.png.h"

#define BACKGROUNDS_DIR wxFindDataDirectory(_T("backgrounds"))
#define BUTTONS_DIR wxFindDataDirectory(_T("buttons"))
#define HELP_FILE(lang) wxFindDataFile(wxT("docs/help_") + lang + wxT(".zip"))

//////////////////////////// help functions //////////////////////////////////

wxBitmap SetDefaultMask(wxBitmap bitmap)
{
  wxImage tmp = bitmap.ConvertToImage();
  wxColour cmask(tmp.GetRed(0,0), tmp.GetGreen(0,0), tmp.GetBlue(0,0));
  bitmap.SetMask(new wxMask(bitmap, cmask));
  return bitmap;
}

//////////////////////////////// MainWin /////////////////////////////////////

enum {
	DIR_TREE_BT_ID = 2250,
	BACKGROUNS_BT_ID,
	BUTTONS_BT_ID,
	DIR_BOOK_ID,
	THUMBMENU_PLAY_ID,
	THUMBMENU_BACKGROUND_ID,
	THUMBMENU_REFRESH_ID,
	THUMBMENU_DELETE_ID,
	THUMBMENU_EDIT_BT_ID,
	THUMBMENU_RESET_BT_ID,
	MENU_CREATE_BUTTON_ID
};

BEGIN_EVENT_TABLE(MainWin, wxFrame)
    // begin wxGlade: MainWin::event_table
    EVT_MENU(wxID_NEW, MainWin::OnNew)
    EVT_MENU(wxID_OPEN, MainWin::OnOpen)
    EVT_MENU(wxID_SAVE, MainWin::OnSave)
    EVT_MENU(wxID_SAVEAS, MainWin::OnSaveAs)
    EVT_MENU(MENU_BURN_ID, MainWin::OnBurn)
    EVT_MENU(wxID_EXIT, MainWin::OnExit)
    EVT_MENU(wxID_UNDO, MainWin::OnUndo)
    EVT_MENU(wxID_REDO, MainWin::OnRedo)
    EVT_MENU(MENU_DVD_ADD_FILE_ID, MainWin::OnAddFile)
    EVT_MENU(MENU_DVD_ADD_FILE_AS_CHAPTER_ID, MainWin::OnAddFileAsChapter)
    EVT_MENU(MENU_DVD_ADD_TITLES_FROM_DVD_ID, MainWin::OnAddTitlesFromDVD)
    EVT_MENU(MENU_DVD_ADD_MENU_ID, MainWin::OnAddMenu)
    EVT_MENU(MENU_DVD_ADD_CHAPTER_MENU_ID, MainWin::OnAddChapterMenu)
    EVT_MENU(MENU_DVD_ADD_VMMENU_ID, MainWin::OnAddVmMenu)
    EVT_MENU(MENU_DVD_ADD_TITLESET_ID, MainWin::OnAddTitleset)
    EVT_MENU(MENU_DVD_OPTIONS_ID, MainWin::OnDVDOptions)
    EVT_MENU(MENU_SETTINGS_ID, MainWin::OnSettings)
    EVT_MENU(wxID_HELP_CONTENTS, MainWin::OnHelpContents)
    EVT_MENU(wxID_ABOUT, MainWin::OnAbout)
    // end wxGlade
	EVT_MENU(MENU_CREATE_BUTTON_ID, MainWin::OnCreateButton)
    EVT_UPDATE_UI(MENU_DVD_ADD_CHAPTER_MENU_ID, MainWin::OnAddChapterMenuUpdateUI)
    EVT_UPDATE_UI(MENU_DVD_ADD_TITLESET_ID, MainWin::OnAddTitlesetUpdateUI)
    EVT_UPDATE_UI(wxID_UNDO, MainWin::OnUpdateUIUndo)
	EVT_UPDATE_UI(wxID_REDO, MainWin::OnUpdateUIRedo)
    
	EVT_TREE_SEL_CHANGED(-1, MainWin::OnDirSelected)
	
	EVT_THUMBNAILS_DCLICK(THUMBNAILS_ID, MainWin::OnThumbDoubleClick)
	EVT_THUMBNAILS_POINTED(THUMBNAILS_ID, MainWin::OnThumbInfo)
	EVT_THUMBNAILS_POINTED(TITLESET_MANAGER_ID, MainWin::OnTitleInfo)
	EVT_MENUEDITOR_OBJECT_POINTED(MENU_EDITOR_ID, MainWin::OnMenuObjectInfo)
	
	EVT_MENU_SELECTED(TITLESET_MANAGER_ID, MainWin::OnSelectMenu)
	EVT_DVD_CHANGED(TITLESET_MANAGER_ID, MainWin::OnChangedDVD)
	EVT_MENUEDITOR_MENU_CHANGED(MENU_EDITOR_ID, MainWin::OnChangedMenu)
	
	EVT_TOOL(DIR_TREE_BT_ID, MainWin::OnDirTreeBt)
	EVT_TOOL(BACKGROUNS_BT_ID, MainWin::OnBackgroundsBt)
	EVT_TOOL(BUTTONS_BT_ID, MainWin::OnButtonsBt)
	EVT_NOTEBOOK_PAGE_CHANGED(DIR_BOOK_ID, MainWin::OnPageChanged)
	
	EVT_MENU(THUMBMENU_BACKGROUND_ID, MainWin::OnSetBackground)
	EVT_UPDATE_UI(THUMBMENU_BACKGROUND_ID, MainWin::OnSetBackgroundUI)
	EVT_MENU(THUMBMENU_PLAY_ID, MainWin::OnPlay)
	EVT_UPDATE_UI(THUMBMENU_PLAY_ID, MainWin::OnPlayUI)
	EVT_MENU(THUMBMENU_DELETE_ID, MainWin::OnDelete)
	EVT_UPDATE_UI(THUMBMENU_DELETE_ID, MainWin::OnDeleteUI)
	EVT_MENU(THUMBMENU_EDIT_BT_ID, MainWin::OnEditButton)
	EVT_MENU(THUMBMENU_RESET_BT_ID, MainWin::OnResetButton)
	EVT_UPDATE_UI(THUMBMENU_RESET_BT_ID, MainWin::OnResetButtonUI)
	EVT_MENU(THUMBMENU_REFRESH_ID, MainWin::OnRefresh)
	
	EVT_SIZE(MainWin::OnResize)
	EVT_SPLITTER_SASH_POS_CHANGED(SPLIITTER_TITLE_ID, MainWin::OnSplitterTitleSash)
	EVT_CLOSE(MainWin::OnClose)
END_EVENT_TABLE()

/** Constructor */
MainWin::MainWin(): wxFrame(NULL, -1, _T(""), wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_FRAME_STYLE), m_dvd(NULL), m_undoPos(0) {
#ifdef __WXMSW__
	m_taskbarInterface = NULL;
#endif
    // begin wxGlade: MainWin::MainWin
    m_splitterTitle = new wxSplitterWindow(this, SPLIITTER_TITLE_ID, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN);
#ifdef __WXMAC__
	m_dirBook = new wxNotebook(this, DIR_BOOK_ID, wxDefaultPosition, wxDefaultSize, wxNB_LEFT);
	m_dirBookPanel = new wxPanel(m_splitterTitle);
	m_splitterDir = new wxSplitterWindow(m_dirBookPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN);
#else
	m_dirBar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTB_VERTICAL|wxTB_FLAT);
	m_splitterDir = new wxSplitterWindow(m_splitterTitle, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN);
#endif
	m_splitterThumb = new wxSplitterWindow(m_splitterDir, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN);
    m_thumbPanel = new wxPanel(m_splitterThumb, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|wxCLIP_CHILDREN);
    m_menubar = new wxMenuBar();
    wxMenu* wxglade_tmp_menu_1 = new wxMenu();
    wxglade_tmp_menu_1->Append(wxID_NEW, _("&New\tCtrl-N"), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_1->Append(wxID_OPEN, _("&Open...\tCtrl-O"), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_1->Append(wxID_SAVE, _("&Save\tCtrl-S"), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_1->Append(wxID_SAVEAS, _("S&ave as...\tShift-Ctrl-S"), wxEmptyString, wxITEM_NORMAL);
    wxMenuItem* item = new wxMenuItem(wxglade_tmp_menu_1, MENU_BURN_ID, _("&Burn DVD...\tF9"), wxEmptyString, wxITEM_NORMAL);
    item->SetBitmap(wxBITMAP_FROM_MEMORY(run));
    wxglade_tmp_menu_1->Append(item);
    wxglade_tmp_menu_1->UpdateUI();
    wxglade_tmp_menu_1->AppendSeparator();
    wxglade_tmp_menu_1->Append(wxID_EXIT, _("&Exit\tAlt-X"), wxEmptyString, wxITEM_NORMAL);
    m_menubar->Append(wxglade_tmp_menu_1, _("&File"));
    wxMenu* wxglade_tmp_menu_2 = new wxMenu();
    item = new wxMenuItem(wxglade_tmp_menu_2, wxID_UNDO, _("&Undo\tCtrl-Z"), wxEmptyString, wxITEM_NORMAL);
    item->SetBitmap(wxArtProvider::GetBitmap(wxART_UNDO, wxART_MENU));
    wxglade_tmp_menu_2->Append(item);
    item = new wxMenuItem(wxglade_tmp_menu_2, wxID_REDO, _("&Redo\tCtrl-Y"), wxEmptyString, wxITEM_NORMAL);
    item->SetBitmap(wxArtProvider::GetBitmap(wxART_REDO, wxART_MENU));
    wxglade_tmp_menu_2->Append(item);
    m_menubar->Append(wxglade_tmp_menu_2, _("&Edit"));
    wxMenu* wxglade_tmp_menu_3 = new wxMenu();
    wxMenu* wxglade_tmp_menu_3_sub = new wxMenu();
    wxglade_tmp_menu_3_sub->Append(MENU_DVD_ADD_FILE_ID, _("&File..."), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_3_sub->Append(MENU_DVD_ADD_FILE_AS_CHAPTER_ID, _("File as &chapter..."), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_3_sub->Append(MENU_DVD_ADD_TITLES_FROM_DVD_ID, _("Titles from &DVD..."), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_3_sub->Append(MENU_DVD_ADD_MENU_ID, _("&Menu"), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_3_sub->Append(MENU_DVD_ADD_CHAPTER_MENU_ID, _("&Chapter menu"), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_3_sub->Append(MENU_DVD_ADD_VMMENU_ID, _("&VMGM menu"), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_3_sub->Append(MENU_DVD_ADD_TITLESET_ID, _("&Titleset"), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_3->Append(wxID_ANY, _("&Add"), wxglade_tmp_menu_3_sub, wxEmptyString);
    wxglade_tmp_menu_3->AppendSeparator();
    wxglade_tmp_menu_3->Append(MENU_DVD_OPTIONS_ID, _("&Options..."), wxEmptyString, wxITEM_NORMAL);
    m_menubar->Append(wxglade_tmp_menu_3, _("&DVD"));
    wxMenu* wxglade_tmp_menu_4 = new wxMenu();
    wxglade_tmp_menu_4->Append(MENU_SETTINGS_ID, _("&Settings..."), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_4->Append(MENU_CREATE_BUTTON_ID, _("&Create button from SVG..."), wxEmptyString, wxITEM_NORMAL);
    m_menubar->Append(wxglade_tmp_menu_4, _("&Configuration"));
    wxMenu* wxglade_tmp_menu_5 = new wxMenu();
    wxglade_tmp_menu_5->Append(wxID_HELP_CONTENTS, _("&Contents\tF1"), wxEmptyString, wxITEM_NORMAL);
    wxglade_tmp_menu_5->Append(wxID_ABOUT, _("&About..."), wxEmptyString, wxITEM_NORMAL);
    m_menubar->Append(wxglade_tmp_menu_5, _("&Help"));
    SetMenuBar(m_menubar);
    m_dirTree = new DirCtrl(m_splitterDir, DIR_TREE_ID);
    m_thumbLabel = new wxStaticText(m_thumbPanel, wxID_ANY, _("Directory"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
    m_thumbnails = new wxThumbnails(m_thumbPanel, THUMBNAILS_ID);
    m_menuEditor = new MenuEditor(m_splitterThumb, MENU_EDITOR_ID);
    m_titlesetManager = new TitlesetManager(m_splitterTitle, TITLESET_MANAGER_ID);

    set_properties();
    do_layout();
    // end wxGlade
    
    m_statusbar = new StatusBar(this);
    SetStatusBar(m_statusbar);
    PositionStatusBar();
    wxSizeEvent sizeEvent;
    m_statusbar->OnSize(sizeEvent);
    
    m_helpController.SetParentWindow(this);
    wxString lang = s_config.GetLanguageCode().length() > 0 ? s_config.GetLanguageCode().Lower() : wxT("en");
    if (!wxFileExists(HELP_FILE(lang))) {
    	if (lang.length() > 2 && wxFileExists(HELP_FILE(lang.substr(0, 2))))
    		lang = lang.substr(0, 2);
    	else
    		lang = wxT("en");
    }
    m_helpController.Initialize(HELP_FILE(lang));
    
    m_menuEditor->SetTitlesetManager(m_titlesetManager);
    
    // set filter
    m_thumbnails->SetFilter(wxTHUMB_FILTER_IMAGES|wxTHUMB_FILTER_VIDEOS|wxTHUMB_FILTER_AUDIOS|wxTHUMB_FILTER_SUBTITLES);
	
	// set file browser dir
	wxString fbDir = s_config.GetFileBrowserDir();
	if (fbDir.length() == 0)
		fbDir = s_config.GetFileBrowserLastDir();
	if (fbDir.length() == 0)
		fbDir = wxGetHomeDir();
	m_dirTree->SetPath(fbDir);
	m_lastDir = s_config.GetLastProjectDir();
	
	// show Backgrouns
	SelectButton(BACKGROUNS_BT_ID);
	wxCommandEvent evt;
	OnBackgroundsBt(evt);
	VideoFormat vf = (VideoFormat) s_config.GetDefVideoFormat();
	AudioFormat af = (AudioFormat) s_config.GetDefAudioFormat();
	AspectRatio ar = (AspectRatio) s_config.GetDefAspectRatio();
	NewDVD(wxT(""), wxT(""), s_config.GetDefDiscLabel(), (DvdResolution) s_config.GetDefDvdResolution(),
			(DiscCapacity) s_config.GetDefDiscCapacity(), s_config.GetVideoBitrate(), s_config.GetAudioBitrate(),
			vf <= vfCOPY ? vfPAL : vf, af <= afCOPY ? afMP2 : af, ar <= arAUTO ? ar4_3 : ar);

	// Restore frame size/position
	if (s_config.IsMainWinMaximized()) {
		Maximize(true);
	} else {
		wxRect rect = s_config.GetMainWinLocation();
		if (rect.width > 0 && rect.height > 0)
			SetSize(rect);
	}
	
    m_thumbFileMenu = new wxMenu;
    m_thumbFileMenu->Append(THUMBMENU_BACKGROUND_ID, _("&Assign to background"));
    m_thumbFileMenu->AppendSeparator();
    m_thumbFileMenu->Append(THUMBMENU_PLAY_ID, _("&Play"));
    m_thumbFileMenu->Append(THUMBMENU_DELETE_ID, _("&Delete"))->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE, wxART_MENU));
    m_thumbFileMenu->AppendSeparator();
    m_thumbFileMenu->Append(THUMBMENU_REFRESH_ID, _("&Refresh") + wxString(wxT("\tF5")));
	m_thumbnails->SetPopupMenu(m_thumbFileMenu);
	
	m_thumbButtonsMenu = new wxMenu;
	m_thumbButtonsMenu->Append(THUMBMENU_EDIT_BT_ID, _("&Edit button"));
	m_thumbButtonsMenu->Append(THUMBMENU_RESET_BT_ID, _("&Reset button"));
	m_thumbButtonsMenu->AppendSeparator();
	m_thumbButtonsMenu->Append(THUMBMENU_REFRESH_ID, _("&Refresh") + wxString(wxT("\tF5")));
	
    wxMenu* thumbGlobalMenu = new wxMenu;
    thumbGlobalMenu->Append(THUMBMENU_REFRESH_ID, _("&Refresh") + wxString(wxT("\tF5")));
    m_thumbnails->SetGlobalPopupMenu(thumbGlobalMenu);
    
    m_fileHistory.Load(*s_config.GetConfigBase());
//    m_fileHistory.UseMenu()
}

MainWin::~MainWin() {
#ifdef __WXMSW__
	if (m_taskbarInterface != NULL)
		m_taskbarInterface->Release();
#endif
}

void MainWin::set_properties() {
    m_toolbar = new wxToolBar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxTB_FLAT|wxTB_DOCKABLE);
    m_toolbar->SetToolBitmapSize(wxSize(22, 22));
	SetToolBar(m_toolbar);
	
    // begin wxGlade: MainWin::set_properties
    SetTitle(_("DVDStyler"));
    SetSize(wxSize(1000, 740));
    m_thumbLabel->SetBackgroundColour(wxColour(90, 16, 198));
    m_thumbLabel->SetForegroundColour(wxColour(255, 255, 255));
    m_titlesetManager->SetMinSize(wxSize(939, 671));
    // end wxGlade
	
    m_thumbLabel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
#ifndef __WXMSW__
    m_thumbLabel->SetForegroundColour(wxColour(0, 0, 0));
#endif
	
	// tool bar
	m_toolbar->AddTool(wxID_NEW,  wxT(""), ICON_NEW,  wxNullBitmap, wxITEM_NORMAL, _("New"), wxT(""));
	m_toolbar->AddTool(wxID_OPEN, wxT(""), ICON_OPEN, wxNullBitmap, wxITEM_NORMAL, _("Open..."), wxT(""));
	m_toolbar->AddTool(wxID_SAVE, wxT(""), ICON_SAVE, wxNullBitmap, wxITEM_NORMAL, _("Save"), wxT(""));
	m_toolbar->AddTool(MENU_DVD_OPTIONS_ID, wxT(""), wxBITMAP_FROM_MEMORY(dvdOptions), wxNullBitmap, wxITEM_NORMAL,
			_("DVD Options..."), wxT(""));
	m_toolbar->AddTool(MENU_SETTINGS_ID, wxT(""), wxBITMAP_FROM_MEMORY(settings), wxNullBitmap, wxITEM_NORMAL,
			_("DVD Options..."), wxT(""));
	m_toolbar->AddTool(MENU_BURN_ID,  wxT(""), wxBITMAP_FROM_MEMORY(run), wxNullBitmap, wxITEM_NORMAL,
			_("Burn..."), wxT(""));
	m_toolbar->AddSeparator();
#if wxCHECK_VERSION(2,9,0)
	m_toolbar->AddTool(MENU_DVD_ADD_FILE_ID,  wxT(""), wxBITMAP_FROM_MEMORY(addVideo), wxNullBitmap, wxITEM_DROPDOWN,
			_("Add File..."), wxT(""));
	wxMenu* addMenu = new wxMenu;
	addMenu->Append(MENU_DVD_ADD_FILE_ID, _("&File..."));
	addMenu->Append(MENU_DVD_ADD_FILE_AS_CHAPTER_ID, _("File as &chapter..."));
	addMenu->Append(MENU_DVD_ADD_TITLES_FROM_DVD_ID, _("Titles from &DVD..."));
	m_toolbar->SetDropdownMenu(MENU_DVD_ADD_FILE_ID, addMenu);
#else
	m_toolbar->AddTool(MENU_DVD_ADD_FILE_ID,  wxT(""), wxBITMAP_FROM_MEMORY(addVideo), wxNullBitmap, wxITEM_NORMAL,
			_("Add File..."), wxT(""));
#endif
	m_toolbar->Realize();
	
	// dir tree
	m_dirTree->SetWindowStyle(m_dirTree->GetWindowStyle() | wxCLIP_CHILDREN);
	
	// thumbnails
    m_thumbnails->SetLabelControl(m_thumbLabel);
	m_thumbnails->EnableDragging();
	
    // dir bar
#ifdef __WXMAC__
//	const wxSize imageSize(16, 16);
//	m_dirBookImages = new wxImageList(imageSize.x, imageSize.y);
//	m_dirBookImages->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_TOOLBAR, imageSize).ConvertToImage().Rotate90().Rotate90());
//	m_dirBook->SetImageList(m_dirBookImages);
	m_dirBook->AddPage(new wxPanel(m_dirBook), _("File browser"), false, 0);
	m_dirBook->AddPage(new wxPanel(m_dirBook), _("Backgrounds"));
	m_dirBook->AddPage(new wxPanel(m_dirBook), _("Buttons"));
	m_dirBook->SetMinSize(wxSize(m_dirBook->GetSize().x - 10, -1));
#else
	m_dirBar->SetSize(wxSize(40, 40));
    m_dirBar->SetMargins(4, 4);
    wxVerticalToolbar* vertToolBar = new wxVerticalToolbar(m_dirBar);
    vertToolBar->AddTool(DIR_TREE_BT_ID, _("File browser"),
    		wxArtProvider::GetBitmap(wxART_FOLDER, wxART_TOOLBAR), wxITEM_RADIO);
    vertToolBar->AddTool(BACKGROUNS_BT_ID, _("Backgrounds"), wxNullBitmap, wxITEM_RADIO);
	vertToolBar->AddTool(BUTTONS_BT_ID, _("Buttons"), wxNullBitmap, wxITEM_RADIO);
    vertToolBar->Update();
    m_dirBar->Realize();
#endif
}


void MainWin::do_layout() {
    // begin wxGlade: MainWin::do_layout
    wxBoxSizer* mainWinSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* thumbSizer = new wxBoxSizer(wxVERTICAL);
    thumbSizer->Add(m_thumbLabel, 0, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 0);
    thumbSizer->Add(m_thumbnails, 1, wxEXPAND, 0);
    m_thumbPanel->SetSizer(thumbSizer);
    m_splitterThumb->SplitVertically(m_thumbPanel, m_menuEditor, 10);
    m_splitterDir->SplitVertically(m_dirTree, m_splitterThumb, 10);
#ifdef __WXMAC__
	wxBoxSizer* dirBookSizer = new wxBoxSizer(wxHORIZONTAL);
	dirBookSizer->Add(m_dirBook, 0, wxEXPAND, 0);
	dirBookSizer->Add(m_splitterDir, 1, wxEXPAND, 0);
	m_dirBookPanel->SetSizer(dirBookSizer);
	m_splitterTitle->SplitHorizontally(m_dirBookPanel, m_titlesetManager, 10);
#else	
    m_splitterTitle->SplitHorizontally(m_splitterDir, m_titlesetManager, 10);
    mainWinSizer->Add(m_dirBar, 0, wxEXPAND, 0);
#endif
    mainWinSizer->Add(m_splitterTitle, 1, wxEXPAND, 0);
    SetSizer(mainWinSizer);
    Layout();
    Centre();
    // end wxGlade
	
	// splitters
	m_thumbPanel->SetSizeHints(
	  m_thumbnails->GetMinWidth(), m_thumbnails->GetMinHeight());
	m_splitterDirSash = 200;
	m_splitterDir->Unsplit(m_dirTree);
	m_splitterThumb->SetSashPosition(238);
	m_splitterDir->SetMinimumPaneSize(1);
    m_splitterThumb->SetMinimumPaneSize(1);
	m_splitterTitle->SetMinimumPaneSize(1);
	m_splitterTitleSash = 0;
}

void MainWin::ShowDirTree(bool show) {
#ifndef __WXMAC__
	m_dirBar->ToggleTool(DIR_TREE_BT_ID, show);
#endif
	if (show && !m_splitterDir->IsSplit()) {
		m_splitterDir->SplitVertically(m_dirTree, m_splitterThumb, m_splitterDirSash);
		m_dirTree->Show(true);
	} else if (!show && m_splitterDir->IsSplit()) {
		m_splitterDirSash = m_splitterDir->GetSashPosition();
#ifndef __WXMAC__
		// animate unsplitting
		m_dirTree->Freeze();
		wxStopWatch sw;
		float step = 0;
		int pos = m_splitterDirSash - 10;
		while (pos > 0) {
			m_splitterDir->SetSashPosition(pos);
			while (wxTheApp->Pending())
				wxTheApp->Dispatch();
			if (step == 0)
				step = sw.Time();
			pos -= int(step);
		}
#endif
		m_splitterDir->Unsplit(m_dirTree);
		m_dirTree->SetSize(0, 0);
		m_dirTree->Thaw();
	}
}

void MainWin::OnDirTreeBt(wxCommandEvent& event) {
	if (!m_splitterDir->IsSplit()) {
		ShowDirTree(true);
		wxTreeEvent evt;
		OnDirSelected(evt);
	} else
		ShowDirTree(false);
	m_thumbnails->SetPopupMenu(m_thumbFileMenu);
}

void MainWin::OnBackgroundsBt(wxCommandEvent& event) {
	ShowDirTree(false);
	m_thumbnails->ShowFileNames(false);
	m_thumbnails->Clear();
	wxString fname = wxFindFirstFile(BACKGROUNDS_DIR + wxT("*"));
	while (!fname.IsEmpty()) {
		if (wxImage::FindHandler(fname.AfterLast('.').Lower(), wxBITMAP_TYPE_ANY))
			m_thumbnails->InsertItem(new wxThumb(fname));
		fname = wxFindNextFile();
	}
	wxString bgDir = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH + wxT("backgrounds") + wxFILE_SEP_PATH;
	if (wxDirExists(bgDir)) {
		fname = wxFindFirstFile(bgDir + wxT("*"));
		while (!fname.IsEmpty()) {
			if (wxImage::FindHandler(fname.AfterLast('.').Lower(), wxBITMAP_TYPE_ANY))
				m_thumbnails->InsertItem(new wxThumb(fname));
			fname = wxFindNextFile();
		}
	}
	m_thumbnails->SetCaption(_("Backgrounds"));
	m_thumbnails->SortItems();
	m_thumbnails->SetPopupMenu(m_thumbFileMenu);
}

void MainWin::OnButtonsBt(wxCommandEvent& event) {
	wxArrayString buttons;
	wxTextFile file;
	if (file.Open(BUTTONS_DIR+ wxT("buttons.lst"))) {
		for (int i=0; i<(int)file.GetLineCount(); i++) {
			wxFileName fileName(file[i]);
			if (fileName.IsRelative())
				fileName.MakeAbsolute(BUTTONS_DIR);
			if (wxFile::Exists(fileName.GetFullPath())) {
				buttons.Add(fileName.GetFullPath());
			}
		}
	}
	wxString fname = wxFindFirstFile(BUTTONS_DIR + _T("*.xml"));
	while (!fname.IsEmpty()) {
		if (buttons.Index(fname) == -1)
			buttons.Add(fname);
		fname = wxFindNextFile();
	}
	
	set<wxString> canBeReseted;
	set<wxString> canBeDeleted;
	wxString btDir = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH + wxT("buttons") + wxFILE_SEP_PATH;
	if (wxDirExists(btDir)) {
		fname = wxFindFirstFile(btDir + wxT("*.xml"));
		while (!fname.IsEmpty()) {
			wxFileName fileName(fname);
			fileName = wxFileName(fileName.GetFullName());
			fileName.MakeAbsolute(BUTTONS_DIR);
			int idx = buttons.Index(fileName.GetFullPath());
			if (idx != wxNOT_FOUND) {
				buttons.RemoveAt(idx);
				buttons.Insert(fname, idx);
				canBeReseted.insert(fname);
			} else {
				buttons.Add(fname);
				canBeDeleted.insert(fname);
			}
			fname = wxFindNextFile();
		}
	}

	ShowDirTree(false);
	m_thumbnails->Clear();
	for (int i=0; i<(int)buttons.Count(); i++) {
		wxThumb* thumb = new wxThumb(buttons[i]);
		wxLogNull log;
		MenuObject bt(NULL, false, buttons[i], 0, 0, _("button"));
		thumb->SetImage(bt.GetImage(m_thumbnails->GetThumbImageWidth(), m_thumbnails->GetThumbImageHeight()));
		if (canBeReseted.find(buttons[i]) != canBeReseted.end())
			thumb->SetId(1);
		if (canBeDeleted.find(buttons[i]) != canBeDeleted.end())
			thumb->SetId(2);
		m_thumbnails->InsertItem(thumb);
	}
	m_thumbnails->SetCaption(_("Buttons"));
	m_thumbnails->SetPopupMenu(m_thumbButtonsMenu);
}

void MainWin::OnPageChanged(wxNotebookEvent& event) {
	wxCommandEvent evt;
	switch (event.GetSelection()) {
		case 0:
			OnDirTreeBt(evt);
			break;
		case 1:
			OnBackgroundsBt(evt);
			break;
		case 2:
			OnButtonsBt(evt);
			break;
		default:
			break;
	}
}

bool MainWin::IsButtonSelected(int btId) {
#ifdef __WXMAC__
	return (m_dirBook->GetSelection() == 0 && btId == DIR_TREE_BT_ID)
			|| (m_dirBook->GetSelection() == 1 && btId == BACKGROUNS_BT_ID)
			|| (m_dirBook->GetSelection() == 2 && btId == BUTTONS_BT_ID);
#else
	return m_dirBar->GetToolState(btId);
#endif
}

void MainWin::SelectButton(int btId) {
#ifdef __WXMAC__
	m_dirBook->SetSelection(btId == DIR_TREE_BT_ID ? 0 : (btId == BACKGROUNS_BT_ID ? 1 : 2));
#else
	m_dirBar->ToggleTool(btId, true);
#endif
}

void MainWin::OnDirSelected(wxTreeEvent& event) {
	if (!m_dvd)
		return;
	m_thumbnails->ShowFileNames();
	m_thumbnails->ShowDir(m_dirTree->GetPath());
	s_config.SetFileBrowserLastDir(m_dirTree->GetPath());
}

void MainWin::OnThumbInfo(wxCommandEvent& event) {
	wxString text;
	if (m_thumbnails->GetPointed() != -1) {
		if (IsButtonSelected(BACKGROUNS_BT_ID)) {
			text = _("Double click to set the background.");
		} else if (IsButtonSelected(BUTTONS_BT_ID)) {
			text = m_thumbnails->GetPointedItem()->GetFilename().AfterLast(wxFILE_SEP_PATH);
		} else {
			if (wxFile::Exists(m_thumbnails->GetPointedItem()->GetFilename())) {
				double size = wxFile(m_thumbnails->GetPointedItem()->GetFilename()).Length() / 1024;
				if (size < 1024)
					text.Printf(_("%s (%.1f KB)"),m_thumbnails->GetPointedItem()->GetFilename().c_str(), size);
				else if (size < 1024 * 1024)
					text.Printf(_("%s (%.1f MB)"),m_thumbnails->GetPointedItem()->GetFilename().c_str(),
							size / 1024);
				else
					text.Printf(_("%s (%.1f GB)"),m_thumbnails->GetPointedItem()->GetFilename().c_str(),
							size / 1024 / 1024);
			} else {
				m_thumbnails->UpdateItems();
			}
		}
	}
	m_statusbar->SetStatusText(text, 0);
}

void MainWin::OnTitleInfo(wxCommandEvent& event) {
	wxString text;
	if (m_titlesetManager->GetPointed() != -1) {
		int id = m_titlesetManager->GetPointedId();
		Vob* vob = m_dvd->GetVob(DVD::GetTsi(id), DVD::IsMenu(id),
				DVD::GetPgci(id), DVD::GetVobi(id));
		if (vob) {
			text = vob->GetFilenameDisplay();
			double duration = vob->GetResultDuration();
			if (vob->GetSlideshow()) {
				text = wxString::Format(_("%d slides"), vob->GetSlideshow()->GetCount());
				duration = vob->GetSlideshow()->GetResultDuration();
			}
			for (unsigned int i=0; i<vob->GetAudioFilenames().Count(); i++)
				text += (text.length() ? wxT(" + ") : wxT("Video + ")) + vob->GetAudioFilenames()[i];
			for (unsigned int i=0; i<vob->GetSubtitles().Count(); i++)
				text += wxT(" + ") + vob->GetSubtitles()[i]->GetFilename();
			if (text.length() && duration > 0) {
				wxString unit = _("sec");
				int dur = (int) duration;
				if (dur >= 60) {
					dur = lround(duration/60);
					unit = _("min");
				}
				text += wxString::Format(wxT(" (%d %s)"), dur, unit.c_str());
			}
		}
	}
	m_statusbar->SetStatusText(text, 0);
}

void MainWin::OnMenuObjectInfo(wxCommandEvent& event) {
	wxString text;
	wxString objId = m_menuEditor->GetPointed();
	if (objId.length()) {
		MenuObject* obj = m_menuEditor->GetMenu()->GetObject(objId);
		if (obj) {
			text = obj->GetId(true);
			if (obj->IsButton())
				text += wxT(": ") + obj->GetAction().AsString(m_dvd, true);
		} else
			text = objId;
	}
	m_statusbar->SetStatusText(text, 0);
}

void MainWin::OnThumbDoubleClick(wxCommandEvent& event) {
	if (m_thumbnails->GetSelected() == -1)
		return;
	wxString filename = m_thumbnails->GetSelectedItem()->GetFilename();
	if (wxImage::FindHandler(filename.AfterLast(wxT('.')).Lower(), wxBITMAP_TYPE_ANY))
		m_menuEditor->SetBackground(filename);
	else if (wxThumbnails::IsVideo(filename))
		wxExecute(s_config.GetPlayCmd() + " \"" + filename + "\"");
}

void MainWin::OnSetBackground(wxCommandEvent& event) {
	wxString filename = m_thumbnails->GetSelectedItem()->GetFilename();
	m_menuEditor->SetBackground(filename);
}

void MainWin::OnSetBackgroundUI(wxUpdateUIEvent& event) {
	if (m_thumbnails->GetSelected() >= 0) {
		wxString filename = m_thumbnails->GetSelectedItem()->GetFilename();
		event.Enable(wxImage::FindHandler(filename.AfterLast(wxT('.')).Lower(), wxBITMAP_TYPE_ANY)
				|| wxThumbnails::IsVideo(filename));
	} else
		event.Enable(false);
}

void MainWin::OnPlay(wxCommandEvent& event) {
	wxString filename = m_thumbnails->GetSelectedItem()->GetFilename();
	wxExecute(s_config.GetPlayCmd() + " \"" + filename + "\"");
}

void MainWin::OnPlayUI(wxUpdateUIEvent& event) {
	if (m_thumbnails->GetSelected() >= 0) {
		wxString filename = m_thumbnails->GetSelectedItem()->GetFilename();
		event.Enable(wxThumbnails::IsVideo(filename));
	} else
		event.Enable(false);
}

void MainWin::OnRefresh(wxCommandEvent& event) {
	if (IsButtonSelected(DIR_TREE_BT_ID))
		m_thumbnails->UpdateItems();
	else if (IsButtonSelected(BACKGROUNS_BT_ID))
		OnBackgroundsBt(event);
	else if (IsButtonSelected(BUTTONS_BT_ID))
		OnButtonsBt(event);
}

void MainWin::OnDelete(wxCommandEvent& event) {
	wxString filename = m_thumbnails->GetSelectedItem()->GetFilename();
	if (!wxFile::Exists(filename))
		return;
	wxMessageDialog msgDlg(this,
			wxString::Format(_("Are you sure you want to delete '%s' from the file system?"), filename.c_str()),
			wxT("DVDStyler"), wxYES_NO|wxICON_QUESTION);
	if (msgDlg.ShowModal() != wxID_YES)
		return;
	wxRemoveFile(filename);
	m_thumbnails->UpdateItems();
}

void MainWin::OnDeleteUI(wxUpdateUIEvent& event) {
	event.Enable(IsButtonSelected(DIR_TREE_BT_ID) && m_thumbnails->GetSelected() >= 0);
}

void MainWin::OnSplitterTitleSash(wxSplitterEvent &event) {
	int width, height;
	GetClientSize(&width, &height);
	m_splitterTitleSash = height - m_splitterTitle->GetSashPosition();
}

void MainWin::OnResize(wxSizeEvent &event) {
	wxFrame::OnSize(event);
	if (m_splitterTitle) {
		int width, height;
		GetClientSize(&width, &height);
		m_splitterTitle->SetSashPosition(height - m_splitterTitleSash);
	}
}

void MainWin::OnClose(wxCloseEvent &event) {
	if (event.CanVeto() && !TestSaved()) {
		event.Veto();
		return;
	}
	m_cache.ShowClearPrompt(this);
	if (!IsIconized())
		s_config.SetMainWinLocation(GetRect(), IsMaximized());
	Destroy();
}

void MainWin::NewDVD(wxString templateFile, wxString discTitle, wxString discLabel, DvdResolution dvdResolution,
		DiscCapacity capacity, int videoBitrate, int audioBitrate, VideoFormat videoFormat, AudioFormat audioFormat,
		AspectRatio aspectRatio, DefaultPostCommand defPostCommand, bool chapterMenu, bool noMenus) {
	if (aspectRatio == ar16_9 && templateFile.Length()
			&& wxFile::Exists(templateFile.substr(0, templateFile.length()-5) + wxT("WS.dvdt")))
		templateFile = templateFile.substr(0, templateFile.length()-5) + wxT("WS.dvdt");
	if (templateFile.Length() && Open(templateFile)) {
		m_dvd->SetLabel(discLabel);
		m_dvd->SetDvdResolution(dvdResolution);
		m_dvd->SetCapacity(capacity);
		m_dvd->SetVideoBitrateAuto(videoBitrate == -1);
		m_dvd->SetVideoBitrate(videoBitrate > 0 ? videoBitrate : 4500);
		m_dvd->SetAudioBitrate(audioBitrate);
		m_dvd->SetVideoFormat(videoFormat);
		m_dvd->SetAudioFormat(audioFormat);
		m_dvd->SetAspectRatio(aspectRatio);
		m_dvd->SetDefPostCommand(defPostCommand);
		if (m_dvd->GetTitlesets().size() == 0)
			m_dvd->AddTitleset();
		if (m_dvd->GetTitlesets()[0]->GetMenus().size()) {
			// move menus to VMG
			Menus& menus = m_dvd->GetTitlesets()[0]->GetMenus();
			Menus& vmMenus = m_dvd->GetVmgm();
			while (menus.size()) {
				vmMenus.push_back(menus.front());
				menus.erase(menus.begin());
			}
			m_dvd->UpdateMenusLocations();
			// fix button actions
			for (unsigned int mIdx = 0; mIdx < vmMenus.size(); mIdx++) {
				Menu* menu = vmMenus[mIdx]->GetMenu();
				if (!menu)
					continue;
				vmMenus[mIdx]->GetEntries().clear();
				if (mIdx == 0) {
					vmMenus[mIdx]->GetEntries().insert(wxT("title"));
				}
				for (unsigned int objIdx = 0; objIdx < menu->GetObjectsCount(); objIdx++) {
					MenuObject* obj = menu->GetObject(objIdx);
					DVDAction& action = obj->GetAction();
					if (!obj->IsButton() || action.IsCustom() || action.GetPgci() == -1 || action.GetTsi() >= 0
							|| action.IsMenu())
						continue;
					action.SetTsi(0);
				}
			}
		}
		if (chapterMenu && m_dvd->GetVmgm().size() == 2) {
			// remove title selection menu
			m_dvd->GetVmgm().erase(m_dvd->GetVmgm().begin() + 1);
			// add chapters menu
			m_titlesetManager->CreateChapterMenu(0, 1);
			// fix button action
			Menu* menu = m_dvd->GetMenu(-1, 0);
			if (menu) {
				MenuObject* menuButton = menu->GetButtonByJumpAction(-2, 1, true);
				if (menuButton) {
					menuButton->GetAction().SetTsi(0);
					menuButton->GetAction().SetPgci(0);
					MenuObjectParam* initParam = menuButton->GetInitParam();
					if (initParam && (initParam->type == wxT("text") || initParam->type == wxT("string"))) {
						wxString value = menuButton->GetParam(initParam->name);
						if (value == wxT("Select Title") || value == wxT("Select title"))
							value = _("Select chapter");
						menuButton->SetParam(initParam->name, value);
						menuButton->UpdateSize();
					}
				}
			}
		}
		for (int tsi = -1; tsi < (int) m_dvd->GetTitlesets().GetCount(); tsi++) {
			if (tsi >= 0) {
				PgcArray& titles = m_dvd->GetPgcArray(tsi, false);
				titles.GetVideo().SetFormat(videoFormat);
				titles.GetVideo().SetAspect(aspectRatio);
			}
			PgcArray& menus = m_dvd->GetPgcArray(tsi, true);
			menus.GetVideo().SetFormat(videoFormat);
			menus.GetVideo().SetAspect(aspectRatio);
			for (unsigned int mIdx = 0; mIdx < menus.GetCount(); mIdx++) {
				Menu* menu = menus[mIdx]->GetMenu();
				if (menu) {
					menu->SetVideoFormat(videoFormat);
					menu->SetAspectRatio(aspectRatio);
					for (unsigned int objIdx = 0; objIdx < menu->GetObjectsCount(); objIdx++) {
						MenuObject* obj = menu->GetObject(objIdx);
						MenuObjectParam* initParam = obj->GetInitParam();
						if (initParam && (initParam->type == wxT("text") || initParam->type == wxT("string"))) {
							wxString value = obj->GetParam(initParam->name);
							if (value == wxT("Play All") || value == wxT("Play all"))
								value = _("Play all");
							else if (value == wxT("Select Title") || value == wxT("Select title"))
								value = _("Select title");
							else if (value == wxT("Back"))
								value = _("Back");
							else if (value == wxT("Previous"))
								value = _("Previous");
							else if (value == wxT("Next"))
								value = _("Next");
							value.Replace(wxT("$DiscTitle"), discTitle);
							obj->SetParam(initParam->name, value);
							obj->UpdateSize();
						}
					}
				}
			}
		}
		if (noMenus) {
			// remove all menus
			for (int tsi = -1; tsi < (int) m_dvd->GetTitlesets().GetCount(); tsi++) {
				PgcArray& menus = m_dvd->GetPgcArray(tsi, true);
				for (unsigned int mIdx = 0; mIdx < menus.GetCount(); mIdx++)
					delete menus[mIdx];
				menus.Clear();
			}
			m_dvd->GetVmgm().SetFpc(wxT("jump title 1;"));
			m_dvd->SetDefPostCommand(cmdJUMP_NEXT_TITLE);
		}
	} else {
		if (m_dvd)
			delete m_dvd;
		m_dvd = new DVD;
		m_dvd->SetLabel(discLabel);
		m_dvd->SetDvdResolution(dvdResolution);
		m_dvd->SetCapacity(capacity);
		m_dvd->SetVideoBitrateAuto(videoBitrate == -1);
		m_dvd->SetVideoBitrate(videoBitrate > 0 ? videoBitrate : 4500);
		m_dvd->SetVideoFormat(videoFormat);
		m_dvd->SetAudioFormat(audioFormat);
		m_dvd->SetAspectRatio(aspectRatio);
		m_dvd->SetDefPostCommand(defPostCommand);
		m_dvd->AddTitleset();
		if (noMenus) {
			m_dvd->GetVmgm().SetFpc(wxT("jump title 1;"));
			m_dvd->SetDefPostCommand(cmdJUMP_NEXT_TITLE);
		} else
			m_dvd->AddMenu(-1);
	}
	UpdateDVD();
	SetChanged(false);
	if (s_config.GetCacheClearPrompt() == 1) {
		m_cache.Clear();
	}
}

bool MainWin::Open(wxString filename) {
	bool showProgressDlg = !filename.EndsWith(wxT(".dvdt"));
	wxProgressDialog* pDlg = NULL;
	if (showProgressDlg) {
		pDlg = new wxProgressDialog(wxT("DVDStyler"), _("Please wait..."), 99, this,
				wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH);
		pDlg->Show();
		pDlg->SetFocus();
		pDlg->Pulse();
	}
	
	DVD* dvd = new DVD;
	if (!dvd->Open(filename, pDlg)) {
		delete dvd;
		delete pDlg;
		wxLogError(_("Error opening '%s'"), filename.c_str());
		return false;
	}
	delete m_dvd;
	m_dvd = dvd;
	m_lastDir = m_dvd->GetPath(false);
	s_config.SetLastProjectDir(m_lastDir);
	UpdateDVD();
	SetChanged(false);
	if (s_config.GetCacheClearPrompt() == 1) {
		m_cache.Clear();
	}
	m_fileHistory.AddFileToHistory(m_dvd->GetFilename());
	m_fileHistory.Save(*s_config.GetConfigBase());

	delete pDlg;
	return true;
}

void MainWin::ShowOpenDlg() {
	wxFileDialog dlg(this, _("Open a DVDStyler project file"), m_lastDir, wxT(""),
			_("Project files") + wxString(wxT(" (*.dvds;*.xml)|*.dvds;*.xml")), wxFD_OPEN);
	dlg.Centre();
	if (dlg.ShowModal() == wxID_OK)
		Open(dlg.GetPath());
}

/** Saves a project file */
bool MainWin::Save(wxString fname) {
	if (!m_dvd->Save(fname))
		return false;
	SetChanged(false);
	m_fileHistory.AddFileToHistory(m_dvd->GetFilename());
	m_fileHistory.Save(*s_config.GetConfigBase());
	return true;
}

void MainWin::UpdateDVD() {
	m_menuEditor->Clear();
	m_titlesetManager->SetDVD(m_dvd);
	m_titlesetManager->UpdateItems();
	m_titlesetManager->SelectFirstMenu();
	DoChangedDVD();
}

void MainWin::SetChanged(bool changed, bool doUndo) {
	if (!doUndo)
		m_changed = changed;
	m_toolbar->EnableTool(wxID_SAVE, changed);
	m_menubar->GetMenu(0)->Enable(wxID_SAVE, changed);
	wxString fname = m_dvd->GetFilename();
	if (!fname.length())
		fname = _("Untitled");
	SetTitle(_T("DVDStyler - ") + fname + (m_changed || !m_dvd->GetFilename().length() ? _T("*") : _T("")));
	if (!doUndo) {
		if (!changed) {
			m_undoHistory.clear();
			m_undoPos = 0;
		}
		AddUndo();
	}
}

void MainWin::OnChangedMenu(wxCommandEvent& event) {
	SetChanged(true);
	bool updateAllItems = (bool) event.GetInt();
	if (updateAllItems) {
		m_titlesetManager->UpdateItems();
	} else if (m_titlesetManager->GetSelectedMenu() >= 0) {
		m_titlesetManager->GetItem(m_titlesetManager->GetSelectedMenu())->Update();
		m_titlesetManager->Refresh();
	}
}

void MainWin::OnSelectMenu(wxCommandEvent& event) {
	int tsi = 0;
	int pgci = 0;
	if (m_titlesetManager->GetSelectedMenu() >= 0) {
		int id = m_titlesetManager->GetSelectedMenuId();
		tsi = DVD::GetTsi(id);
		pgci = DVD::GetPgci(id);
	}
	Menu* menu = m_dvd->GetMenu(tsi, pgci);
	m_menuEditor->SetMenu(m_dvd, menu, tsi, pgci);
	m_menuEditor->Refresh();
}

void MainWin::OnChangedDVD(wxCommandEvent& event) {
	DoChangedDVD();
}

void MainWin::DoChangedDVD(bool doUndo) {
	SetChanged(true, doUndo);
	long encSize, encDuration, fixSize, fixDuration, fixAvgBitrate;
	m_dvd->CalculateVideoBitrate(encSize, encDuration, fixSize, fixDuration, fixAvgBitrate);
	long sizeTotal = m_dvd->GetCapacityValue();
	int videoBitrate = m_dvd->GetVideoBitrate();
	int audioBitrate = m_dvd->GetAudioBitrate();
	
	int bitrateForTotal = videoBitrate + audioBitrate;
	if (m_dvd->IsVideoBitrateAuto()) {
		if (encDuration == 0 && fixDuration > 0) {
			bitrateForTotal = fixAvgBitrate;
		}
		if (bitrateForTotal > 4500)
			bitrateForTotal = 4500;
	}
	
	int dur = fixDuration/60 + encDuration/60;
	int durTotal = sizeTotal > 0 ? fixDuration/60 + (sizeTotal-fixSize)*8/bitrateForTotal/60 : 0;
	
	// If video bitrate doesn't set to auto, display selected bitrate. 
	// Else if video must be encoded, display calculated bitrate that will be used for encoding.
	// Else if video will be copied without encoding, display average bitrate of all video files.
	// Else if no titles exits, display 8 MB/s
	m_statusbar->SetFillStatus(dur, durTotal, encDuration == 0 && fixDuration > 0 ? fixAvgBitrate : videoBitrate);
	m_menuEditor->ShowSafeTV();
	m_menuEditor->Refresh();
}

bool MainWin::TestSaved() {
	if (m_changed) {
		wxString fname = m_dvd->GetFilename();
		if (!fname.length())
			fname = _("Untitled");
		int save = wxMessageBox(wxString::Format(_("The file '%s' was modified, save changes?"),
				fname.c_str()), _("Confirm"), wxYES_NO|wxCANCEL | wxICON_QUESTION, this);
		if (save == wxYES) {
			wxCommandEvent event;
			OnSave(event);
			if (m_changed)
				return false;
		} else if (save == wxCANCEL)
			return false;
	}
	return true;
}

// wxGlade: add MainWin event handlers

void MainWin::OnNew(wxCommandEvent& event) {
	if (!TestSaved())
		return;
	NewProjectDlg dlg(this);
	if (dlg.ShowModal() == wxID_OK) {
		wxString templateFile = wxT("");
		wxString title = wxT("");
		bool chapterMenu = false;
		if (!dlg.IsNoMenu()) {
			TemplateDlg templateDlg(this, dlg.GetAspectRatio());
			if (templateDlg.ShowModal() == wxID_OK) {
				templateFile = templateDlg.GetTemplate();
				title = templateDlg.GetTitle();
				chapterMenu = templateDlg.IsChapterMenu();
			}
		}
		NewDVD(templateFile, title, dlg.GetLabel(), dlg.GetDvdResolution(), dlg.GetCapacity(),
				dlg.GetVideoBitrate(), dlg.GetAudioBitrate(), dlg.GetVideoFormat(), dlg.GetAudioFormat(),
				dlg.GetAspectRatio(), dlg.GetDefPostCommand(), chapterMenu, dlg.IsNoMenu());
	}
}

void MainWin::OnOpen(wxCommandEvent& event) {
	if (!TestSaved())
		return;
	ShowOpenDlg();
}

void MainWin::OnSave(wxCommandEvent& event) {
	if (m_dvd->GetFilename().length()) {
		Save();
	} else
		OnSaveAs(event);
}

void MainWin::OnSaveAs(wxCommandEvent& event) {
	wxString defName = m_dvd->GetLabel() + wxT(".dvds");
	wxFileDialog dlg(this, _("Save a DVDStyler project file"), m_lastDir, defName,
			_("Project files") + wxString(wxT(" (*.dvds;*.xml)|*.dvds;*.xml|"))
			+ _("All Files") + wxString(wxT(" (*.*)|*.*")), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	dlg.Centre();
	if (dlg.ShowModal() == wxID_OK && Save(dlg.GetPath())) {
		m_lastDir = m_dvd->GetPath(false);
		s_config.SetLastProjectDir(m_lastDir);
	}
}
/**
 * Starts compiling without closing
 */
void MainWin::OnBurn(wxCommandEvent& event) {
	Burn(false);
}
/**
 * Opens the burn dialog
 */
void MainWin::Burn(bool autoStart) {
	// check if titles fit on DVD
	m_titlesetManager->AskForReencoding(m_dvd->GetVideoFormat());
	if (m_dvd->GetCapacity() != dcUNLIMITED && m_dvd->GetSize() > m_dvd->GetCapacityValue()) {
		wxMessageBox(_("The selected video files don't fit on DVD. Please reduce\n\
video quality (bitrate) or remove some titles."), _("Burn"), wxOK|wxICON_ERROR, this);
		return;
	}
	// check if every titleset contains titles
	bool hasVobs = false;
	int emptyTsi = -1;
	for (int tsi = 0; tsi<(int)m_dvd->GetTitlesets().Count(); tsi++) {
		Titleset* ts = m_dvd->GetTitlesets()[tsi];
		bool tsHasVobs = false;
		for (int pgci = 0; pgci<(int)ts->GetTitles().Count(); pgci++) {
			if (ts->GetTitles()[pgci]->GetVobs().Count()>0) {
				hasVobs = true;
				tsHasVobs = true;
				break;
			}
		}
		if (!tsHasVobs)
			emptyTsi = tsi;
	}
	if (!hasVobs) {
		wxMessageBox(_("Project doesn't contain any video title. To add a title please\n\
drag a video file into the bottom section of the window."), _("Burn"), wxOK|wxICON_ERROR, this);
		return;
	}
	if (emptyTsi >= 0) {
		wxMessageBox(wxString::Format(_("Titleset %d doesn't contain any video title. To add a title\n\
please drag a video file into the bottom section of the window."), emptyTsi+1), _("Burn"), wxOK|wxICON_ERROR, this);
		return;
	}
	// check if all menus are ok
	if (!m_dvd->CheckMenus())
		return;
	// check if all actions are valid
	if (!m_dvd->CheckActions(m_dvd->HasTemplate()))
		return;
	// fix coordinates of buttons if they are out of range
	m_dvd->FixButtonCoordinates();
	// show burn dialog
	BurnDlg burnDlg(this, m_dvd, &m_cache);
	if (autoStart) {
		if (!m_cache.SetTempDir(s_config.GetTempDir()))
			return;
	} else if (burnDlg.ShowModal() != wxID_OK) {
		return;
	}
	// Progress dialog
	ProgressDlg progressDlg(this, &m_cache, autoStart);
	progressDlg.Start(&burnDlg, m_dvd);
	if (autoStart)
		Close(true);
}

void MainWin::OnExit(wxCommandEvent& event) {
	if (!TestSaved())
		return;
	if (!IsIconized())
		s_config.SetMainWinLocation(GetRect(), IsMaximized());
	Close(true);
}

void MainWin::AddUndo() {
	if (s_config.GetUndoHistorySize() == 0) {
		m_undoHistory.clear();
		m_undoPos = 0;
		return;
	}
	while ((int) m_undoPos < (int) m_undoHistory.size() - 1) {
		m_undoHistory.erase(m_undoHistory.end());
	}
	if (m_undoHistory.size() > (unsigned int) s_config.GetUndoHistorySize())
		m_undoHistory.erase(m_undoHistory.begin());
	int menuId = m_titlesetManager->GetSelectedMenuId();
	Undo undo;
	undo.data = m_dvd->Serialize();
	undo.selectedMenu = menuId;
	undo.changed = m_changed;
	m_undoHistory.push_back(undo);
	m_undoPos = m_undoHistory.size() - 1;
}

void MainWin::DoUndo(bool redo) {
	if ((m_undoPos == 0 && !redo) || (m_undoPos >= m_undoHistory.size() - 1 && redo))
		return;
	m_menuEditor->Clear();

	// restore
	m_undoPos += redo ? 1 : -1;
	m_dvd->Deserialize(m_undoHistory[m_undoPos].data);
	m_changed = m_undoHistory[m_undoPos].changed;
	
	// refresh titleset manager
	m_titlesetManager->UpdateItems();
	
	// refresh menu editor
	int menuId = m_undoHistory[m_undoPos].selectedMenu;
	if (!redo)
		menuId = m_undoHistory[m_undoPos+1].selectedMenu;
	if (menuId >= 0)
		m_titlesetManager->SelectMenu(DVD::GetTsi(menuId), DVD::GetPgci(menuId));
	else
		m_titlesetManager->SelectFirstMenu();
	
	// refresh status bar
	DoChangedDVD(true);
}

void MainWin::OnUndo(wxCommandEvent& event) {
	DoUndo(false);
}

void MainWin::OnUpdateUIUndo(wxUpdateUIEvent &event) {
	event.Enable(m_undoPos > 0);
}

void MainWin::OnRedo(wxCommandEvent& event) {
	DoUndo(true);
}

void MainWin::OnUpdateUIRedo(wxUpdateUIEvent &event) {
	event.Enable(m_undoPos < m_undoHistory.size() - 1);
}

void MainWin::OnAddFile(wxCommandEvent& event) {
	AddFileDialog(false);
}

void MainWin::OnAddFileAsChapter(wxCommandEvent& event) {
	AddFileDialog(true);
}

void MainWin::AddFileDialog(bool asChapter) {
	wxString video = wxThumbnails::GetVideoExtWildcard();
	wxString audio = wxThumbnails::GetAudioExtWildcard();
	wxString subtitle = wxThumbnails::GetSubtitleExtWildcard();
	wxString wildcard = _("Audio/Video/Subtitle Files")
			+ wxString::Format(wxT("|%s%s%s|"), video.c_str(), audio.c_str(), subtitle.c_str())
			+ _("Audio Files") + wxString::Format(wxT(" (%s)|%s|"), audio.c_str(), audio.c_str())
			+ _("Video Files") + wxString::Format(wxT(" (%s)|%s|"), video.c_str(), video.c_str())
			+ _("Subtitle Files") + wxString::Format(wxT(" (%s)|%s|"), subtitle.c_str(), subtitle.c_str())
			+ _("Image Files") + wxString::Format(wxT(" %s"), wxImage::GetImageExtWildcard().c_str())
			+ wxString(wxT("|")) + _("All Files") + wxString(wxT(" (*.*)|*.*"));
	if (asChapter)
		wildcard = _("Video Files") + wxString::Format(wxT(" (%s)|%s|"), video.c_str(), video.c_str())
				+ _("Image Files") + wxString::Format(wxT(" %s"), wxImage::GetImageExtWildcard().c_str())
				+ wxString(wxT("|")) + _("All Files") + wxString(wxT(" (*.*)|*.*"));
	wxFileDialog dialog(this, _("Choose a file"), wxT(""), wxT(""), wildcard, wxFD_OPEN | wxFD_MULTIPLE);
	// set file browser dir
	dialog.SetDirectory(s_config.GetLastAddDir() + wxFILE_SEP_PATH);
	if (dialog.ShowModal() == wxID_OK) {
		wxArrayString paths;
		dialog.GetPaths(paths);
		for (unsigned int i = 0; i < paths.GetCount(); i++) {
			wxString filename = paths[i];
			if (wxThumbnails::IsVideo(filename)) {
				m_titlesetManager->AddVideo(filename, !asChapter);
			} else if (wxThumbnails::IsAudio(filename)) {
				m_titlesetManager->AddAudio(filename);
			} else if (wxThumbnails::IsSubtitle(filename)) {
				m_titlesetManager->AddSubtitles(filename);
			} else if (wxImage::FindHandler(filename.AfterLast('.').Lower(), wxBITMAP_TYPE_ANY)) {
				m_titlesetManager->AddImage(filename, !asChapter && i == 0, i == paths.GetCount() - 1);
			} else {
				// try to load it as video
				m_titlesetManager->AddVideo(filename, !asChapter);
			}
		}
		s_config.SetLastAddDir(dialog.GetDirectory());
	}
}

void MainWin::OnAddTitlesFromDVD(wxCommandEvent& event) {
	wxDirDialog dialog(this, _("Choose DVD device or directory"));
	if (dialog.ShowModal() != wxID_OK)
		return;
	wxString dir = dialog.GetPath();
	if (wxDirExists(dir + wxFILE_SEP_PATH + wxString(wxT("VIDEO_TS"))))
		dir += wxFILE_SEP_PATH + wxString(wxT("VIDEO_TS"));

	// search for VOB files
	wxArrayString titlesets;
	wxArrayString tsLabels;
	while (true) {
		wxString fileName = dir + wxFILE_SEP_PATH + wxString::Format(wxT("VTS_%02d_1.VOB"), titlesets.size() + 1);
		if (!wxFileExists(fileName)) {
			if (titlesets.empty()) {
				wxLogError(wxT("%s: no such file or directory"), fileName.c_str());
				return;
			}
			break;
		}

		wxString vobs;
		for (int i = 1; i < 100; i++) {
			wxString fileName = dir + wxFILE_SEP_PATH + wxString::Format(wxT("VTS_%02d_%d.VOB"), titlesets.size() + 1, i);
			if (!wxFileExists(fileName)) {
				if (i == 1) {
					wxLogError(wxT("%s: no such file or directory"), fileName.c_str());
					return;
				}
				break;
			}
			if (vobs.length() > 0)
				vobs += wxT('|');
			vobs = vobs + fileName;
		}
		titlesets.push_back(wxT("concat:") + vobs);

		// determine titleset duration and load frame image
		long duration = -1;
		wxFfmpegMediaDecoder decoder;
		if (decoder.Load(titlesets[titlesets.size() - 1])) {
			duration = lround(decoder.GetDuration());
		}
		wxString label = wxString::Format(_T("Titleset %d"), titlesets.size());
		if (duration > 0)
			label += wxString::Format(wxT(" (%d:%02d:%02d)"), duration/3600, (duration/60) % 60, duration % 60);
		tsLabels.push_back(label);
	}

	if (titlesets.size() == 1) {
		m_titlesetManager->AddVideo(titlesets[0], true);
	} else {
		// select titleset
		SingleThumbnailDlg dlg(this, _("Choose a titleset to add:"), wxT("DVDStyler"), titlesets, tsLabels);
		dlg.SetSelection(0);
		if (dlg.ShowModal() == wxID_OK)
			m_titlesetManager->AddVideo(titlesets[dlg.GetSelection()], true);
	}
}

void MainWin::OnAddMenu(wxCommandEvent& event) {
	m_titlesetManager->SetSelected(-1);
	m_titlesetManager->AddMenu();
}

void MainWin::OnAddChapterMenu(wxCommandEvent& event) {
	int tsi = m_titlesetManager->GetSelected() >= 0 ? DVD::GetTsi(m_titlesetManager->GetSelectedId())
			: m_dvd->GetTitlesets().size() - 1;
	if (tsi < 0)
		tsi = 0;
	m_titlesetManager->CreateChapterMenu(tsi, tsi >= 0 && m_dvd->GetTitlesets()[tsi]->GetTitles().size() > 0
			? m_dvd->GetTitlesets()[tsi]->GetTitles()[0]->GetChapterCount() : 1);
	m_titlesetManager->UpdateItems();
	DoChangedDVD();
}

void MainWin::OnAddChapterMenuUpdateUI(wxUpdateUIEvent& event) {
	event.Enable(m_dvd->GetTitleMenuTemplate() != NULL && m_dvd->GetTitlesets().size() > 0);
}

void MainWin::OnAddVmMenu(wxCommandEvent& event) {
	m_titlesetManager->AddVmMenu();
}

void MainWin::OnAddTitleset(wxCommandEvent& event) {
	m_titlesetManager->AddTitleset();
}

void MainWin::OnAddTitlesetUpdateUI(wxUpdateUIEvent& event) {
	int cnt = m_dvd != NULL ? m_dvd->GetTitlesets().Count() : 0;
	event.Enable(cnt > 0 && m_dvd->GetTitlesets()[cnt - 1]->GetTitles().Count());
}

void MainWin::OnDVDOptions(wxCommandEvent& event) {
	DVDPropDlg dlg(this, m_dvd);
	if (dlg.ShowModal() == wxID_OK) {
		DoChangedDVD();
		m_menuEditor->Update();
		m_titlesetManager->UpdateItems();
	}
}

void MainWin::OnSettings(wxCommandEvent& event) {
	SettingsDlg dlg(this, &m_cache);
	dlg.ShowModal();
}


void MainWin::OnCreateButton(wxCommandEvent& event) {
	wxFileDialog fileDlg(this, _("Open a SVG file"), m_lastDir, "",
			_("SVG files") + wxString(" (*.svg)|*.svg"), wxFD_OPEN);
	fileDlg.Centre();
	if (fileDlg.ShowModal() != wxID_OK)
		return;
	ButtonEditDlg dlg(this);
	if (!dlg.LoadSVG(fileDlg.GetPath()))
		return;
	dlg.ShowModal();
}

void MainWin::OnEditButton(wxCommandEvent& event) {
	wxString filename = m_thumbnails->GetSelectedItem()->GetFilename();
	ButtonEditDlg dlg(this);
	if (!dlg.LoadButton(filename))
		return;
	if (dlg.ShowModal() == wxID_OK) {
		m_thumbnails->GetSelectedItem()->SetFilename(dlg.GetDestFileName());
		wxLogNull log;	
		MenuObject bt(NULL, false, dlg.GetDestFileName(), 0, 0, _("button"));
		m_thumbnails->GetSelectedItem()->SetImage(
				bt.GetImage(m_thumbnails->GetThumbImageWidth(), m_thumbnails->GetThumbImageHeight()));
		m_thumbnails->GetSelectedItem()->SetId(1);
		m_thumbnails->GetSelectedItem()->Update();
		m_thumbnails->Refresh();
	}
}

void MainWin::OnResetButton(wxCommandEvent& event) {
	if (m_thumbnails->GetSelectedItem()->GetId() == 1) {
		// Reset button
		if (wxMessageBox(_("Do you really want to reset this button?"),
				"DVDStyler", wxYES_NO|wxICON_QUESTION, this) != wxYES)
			return;
				
		wxRemoveFile(m_thumbnails->GetSelectedItem()->GetFilename());
		wxFileName fileName(m_thumbnails->GetSelectedItem()->GetFilename());
		fileName = wxFileName(fileName.GetFullName());
		fileName.MakeAbsolute(BUTTONS_DIR);
		m_thumbnails->GetSelectedItem()->SetFilename(fileName.GetFullPath());
		wxLogNull log;	
		MenuObject bt(NULL, false, fileName.GetFullPath(), 0, 0, _("button"));
		m_thumbnails->GetSelectedItem()->SetImage(
				bt.GetImage(m_thumbnails->GetThumbImageWidth(), m_thumbnails->GetThumbImageHeight()));
		m_thumbnails->GetSelectedItem()->SetId(1);
		m_thumbnails->GetSelectedItem()->Update();
		m_thumbnails->Refresh();
	} else if (m_thumbnails->GetSelectedItem()->GetId() == 2) {
		// Delete button
		if (wxMessageBox(_("Do you really want to delete this button?"),
				"DVDStyler", wxYES_NO|wxICON_QUESTION, this) != wxYES)
			return;
		wxRemoveFile(m_thumbnails->GetSelectedItem()->GetFilename());
		OnButtonsBt(event);
	}
}

void MainWin::OnResetButtonUI(wxUpdateUIEvent &event) {
	bool canBeReseted = m_thumbnails->GetSelectedItem()->GetId() == 1;
	bool canBeDeleted = m_thumbnails->GetSelectedItem()->GetId() == 2;
	event.SetText(canBeDeleted ? _("&Delete button") : _("&Reset button"));
	event.Enable(m_thumbnails->GetSelectedItem() && (canBeReseted || canBeDeleted));
}

void MainWin::OnHelpContents(wxCommandEvent& event) {
	m_helpController.DisplayContents();
}

void MainWin::OnAbout(wxCommandEvent& event) {
	About(this);
}

#ifdef __WXMSW__
static const UINT uTaskbarBtnCreatedMsg = RegisterWindowMessage ( _T("TaskbarButtonCreated") );
WXLRESULT MainWin::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	if (message == uTaskbarBtnCreatedMsg) {
		DWORD dwMajor = LOBYTE(LOWORD(GetVersion()));
		DWORD dwMinor = HIBYTE(LOWORD(GetVersion()));
		// Check that the Windows version is at least 6.1 (Win 7)
		if (dwMajor > 6 || (dwMajor == 6 && dwMinor > 0)) {
			HRESULT hr = 0;
			hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
					IID_ITaskbarList3, (void**) &m_taskbarInterface);
			if (FAILED(hr)) {
				m_taskbarInterface->Release();
				m_taskbarInterface = NULL;
			}
			if (m_taskbarInterface != NULL) {
				m_taskbarInterface->SetProgressState(GetHWND(), TBPF_NORMAL);
				m_taskbarInterface->SetProgressValue(GetHWND(), 0, 100);
			}

		}

		return TRUE;
	}
	return wxFrame::MSWWindowProc(message, wParam, lParam);
}
#endif

/** Sets progress bar value */
void MainWin::SetProgressBarValue(unsigned int completed, unsigned int total) {
#ifdef __WXMSW__
	if (m_taskbarInterface != NULL) {
		m_taskbarInterface->SetProgressValue(GetHWND(), completed, total);
	}
#endif
}

/** Sets progress bar state */
void MainWin::SetProgressBarState(bool error) {
#ifdef __WXMSW__
	if (m_taskbarInterface != NULL) {
		m_taskbarInterface->SetProgressState(GetHWND(), error ? TBPF_ERROR : TBPF_NORMAL);
	}
#endif
}
