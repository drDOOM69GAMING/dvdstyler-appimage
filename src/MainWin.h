/////////////////////////////////////////////////////////////////////////////
// Name:        MainWin.h
// Purpose:     Main window
// Author:      Alex Thuering
// Created:	10.10.2003
// RCS-ID:      $Id: MainWin.h,v 1.52 2017/11/25 14:39:34 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef MAINWIN_H
#define MAINWIN_H

#include <vector>
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/docview.h>
#include <wx/html/helpctrl.h>
#include <wx/notebook.h>
#include <wx/imaglist.h>
#include <wxVillaLib/Thumbnails.h>
#include "DirCtrl.h"
#include "MenuEditor.h"
#include "TitlesetManager.h"
#include "StatusBar.h"
#include "Cache.h"

using namespace std;

#ifdef __WXMSW__
	struct ITaskbarList3;
#endif

// begin wxGlade: dependencies
#include <wx/splitter.h>
// end wxGlade

struct Undo {
	wxString data;
	int selectedMenu;
	bool changed;
};

class MainWin: public wxFrame {
public:
    // begin wxGlade: MainWin::ids
    enum {
        MENU_BURN_ID = 2105,
        MENU_BURN_ISO_ID = 2106,
        MENU_DVD_ADD_FILE_ID = 2110,
        MENU_DVD_ADD_FILE_AS_CHAPTER_ID,
        MENU_DVD_ADD_TITLES_FROM_DVD_ID,
        MENU_DVD_ADD_MENU_ID,
        MENU_DVD_ADD_CHAPTER_MENU_ID,
        MENU_DVD_ADD_VMMENU_ID,
        MENU_DVD_ADD_TITLESET_ID,
        MENU_DVD_OPTIONS_ID = 2119,
        MENU_SETTINGS_ID = 2120,
        DIR_TREE_ID = 2000,
        THUMBNAILS_ID = 2001,
        MENU_EDITOR_ID = 2002,
        TITLESET_MANAGER_ID = 2003,
        SPLIITTER_TITLE_ID = 2010
    };
    // end wxGlade
    
    /** Constructor */
    MainWin();
    /** Destructor */
	~MainWin();
	/** Creates new project */
	void NewDVD(wxString templateFile, wxString discTitle, wxString discLabel,
			DvdResolution dvdResolution, DiscCapacity capacity, int videoBitrate,
			int audioBitrate, VideoFormat videoFormat, AudioFormat audioFormat, AspectRatio aspectRatio,
			DefaultPostCommand defPostCommand = cmdCALL_LAST_MENU, bool chapterMenu = false, bool noMenus = false);
    /** Loads a project file */
	bool Open(wxString filename);
	/** Shows open dialog */
	void ShowOpenDlg();
	/** Saves a project file */
	bool Save(wxString fname = wxEmptyString);
    /** Adds a title to dvd */
	void AddTitle(wxString filename) { m_titlesetManager->AddVideo(filename); }
	/** Starts a burn process */
    void Burn(bool start_close);
    /** Returns the file history */
    wxFileHistory& GetFileHistory() { return m_fileHistory; }
    /** Sets progress bar value */
    void SetProgressBarValue(unsigned int completed, unsigned int total);
    /** Sets progress bar state */
    void SetProgressBarState(bool error = false);
    /** Sets output directory */
    void SetOutputDir(const wxString& outputDir) { m_dvd->SetOutputDir(outputDir); }
    /** Sets ISO file name with path */
    void SetIsoFile(const wxString& isoFile) { m_dvd->SetIsoFile(isoFile); }
    
  private:
    // begin wxGlade: MainWin::attributes
    wxMenuBar* m_menubar;
    DirCtrl* m_dirTree;
    wxStaticText* m_thumbLabel;
    wxThumbnails* m_thumbnails;
    wxPanel* m_thumbPanel;
    MenuEditor* m_menuEditor;
    wxSplitterWindow* m_splitterThumb;
    wxSplitterWindow* m_splitterDir;
    TitlesetManager* m_titlesetManager;
    wxSplitterWindow* m_splitterTitle;
    // end wxGlade
    StatusBar* m_statusbar;
    wxHtmlHelpController m_helpController;
#ifdef __WXMAC__
	wxNotebook* m_dirBook;
	wxPanel* m_dirBookPanel;
	wxImageList* m_dirBookImages;
#else
	wxToolBar* m_dirBar;
#endif
	
    // begin wxGlade: MainWin::methods
    void set_properties();
    void do_layout();
    // end wxGlade
	
	DVD* m_dvd;
	wxString m_lastDir;
	bool m_changed;
	wxFileHistory m_fileHistory;
	Cache m_cache;
	vector<Undo> m_undoHistory;
	unsigned int m_undoPos;
	
	void AddFileDialog(bool asChapter);
	
    /** It must be called after m_dvd was changed */
	void UpdateDVD();
	
    /** sets project status to changed */
    void SetChanged(bool changed, bool doUndo = false);
    /** returns true if project is changed */
	bool GetChanged() { return m_changed; }
    void OnChangedMenu(wxCommandEvent& event);
	void OnChangedDVD(wxCommandEvent& event);
	void DoChangedDVD(bool doUndo = false);
	void AddUndo();
	void DoUndo(bool redo);
	
    /** checks if project is saved and asks for saving */
    bool TestSaved();
	
	wxToolBar* m_toolbar;
	int m_splitterDirSash;
	int m_splitterTitleSash;
	wxMenu* m_thumbFileMenu;
	wxMenu* m_thumbButtonsMenu;
	
    /** shows directory tree */
	void ShowDirTree(bool show);
	void OnDirTreeBt(wxCommandEvent& event);
    void OnBackgroundsBt(wxCommandEvent& event);
	void OnButtonsBt(wxCommandEvent& event);
	void OnPageChanged(wxNotebookEvent& event);
	bool IsButtonSelected(int btId);
	void SelectButton(int btId);
	
	void OnDirSelected(wxTreeEvent& event);
	void OnThumbDoubleClick(wxCommandEvent& event);
	void OnThumbInfo(wxCommandEvent& event);
	void OnTitleInfo(wxCommandEvent& event);
	void OnMenuObjectInfo(wxCommandEvent& event);
    void OnSelectMenu(wxCommandEvent& event);
    
    void OnSetBackground(wxCommandEvent& event);
	void OnSetBackgroundUI(wxUpdateUIEvent& event);
	void OnPlay(wxCommandEvent& event);
	void OnPlayUI(wxUpdateUIEvent& event);
	void OnDelete(wxCommandEvent& event);
	void OnDeleteUI(wxUpdateUIEvent& event);
	void OnEditButton(wxCommandEvent& event);
	void OnResetButton(wxCommandEvent& event);
	void OnResetButtonUI(wxUpdateUIEvent &event);
	void OnRefresh(wxCommandEvent& event);
	
	virtual void OnNew(wxCommandEvent& event); // wxGlade: <event_handler>
	virtual void OnOpen(wxCommandEvent& event); // wxGlade: <event_handler>
	virtual void OnSave(wxCommandEvent& event); // wxGlade: <event_handler>
	virtual void OnSaveAs(wxCommandEvent& event); // wxGlade: <event_handler>
	virtual void OnBurn(wxCommandEvent& event); // wxGlade: <event_handler>
	virtual void OnBurnIso(wxCommandEvent& event); // wxGlade: <event_handler>
	virtual void OnExit(wxCommandEvent& event); // wxGlade: <event_handler>
	virtual void OnUndo(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnUpdateUIUndo(wxUpdateUIEvent &event);
	virtual void OnRedo(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnUpdateUIRedo(wxUpdateUIEvent &event);
	virtual void OnAddFile(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnAddFileAsChapter(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnAddTitlesFromDVD(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnAddMenu(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnAddChapterMenu(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnAddChapterMenuUpdateUI(wxUpdateUIEvent &event);
	virtual void OnAddVmMenu(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnAddTitleset(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnAddTitlesetUpdateUI(wxUpdateUIEvent& event);
	virtual void OnDVDOptions(wxCommandEvent& event); // wxGlade: <event_handler>
	virtual void OnSettings(wxCommandEvent& event); // wxGlade: <event_handler>
	virtual void OnAbout(wxCommandEvent& event); // wxGlade: <event_handler>
	virtual void OnHelpContents(wxCommandEvent &event); // wxGlade: <event_handler>
	virtual void OnCreateButton(wxCommandEvent& event);
	
	void OnSplitterTitleSash(wxSplitterEvent &event);
	void OnResize(wxSizeEvent &event);
	void OnClose(wxCloseEvent &event);
	void KillChildProcesses();
	
	
#ifdef __WXMSW__
	ITaskbarList3 *m_taskbarInterface;
	WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);
#endif
	
    DECLARE_EVENT_TABLE()
}; // wxGlade: end class

#endif // MAINWIN_H
