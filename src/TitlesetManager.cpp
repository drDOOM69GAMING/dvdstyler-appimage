/////////////////////////////////////////////////////////////////////////////
// Name:        TitlesetManager.h
// Purpose:     Titleset Manager
// Author:      Alex Thuering
// Created:	27.01.2003
// RCS-ID:      $Id: TitlesetManager.cpp,v 1.109 2017/11/25 15:56:34 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "TitlesetManager.h"
#include "MainWin.h"
#include "TitlePropDlg.h"
#include "MenuPropDlg.h"
#include "MPEG.h"
#include "Config.h"
#include "Utils.h"
#include "MessageDlg.h"
#include "ProcessTranscode.h"
#include <wxVillaLib/ThumbnailFactory.h>
#include <wxVillaLib/ImageProc.h>
#include <wxVillaLib/utils.h>
#include <wx/splitter.h>
#include <wx/dnd.h>
#include <wx/clipbrd.h>
#include <wx/utils.h>
#include <wx/artprov.h>
#include <wx/progdlg.h>

//////////////////////////////////////////////////////////////////////////////
///////////////////////////// TitleDnDFile ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class TitleDnDFile: public wxDropTarget {
public:
	TitleDnDFile(TitlesetManager* pOwner): m_pOwner(pOwner) {
		wxDataFormat actionDataFormat(DATAFORMAT_ACTION);
		wxDataObjectComposite* dataObject = new wxDataObjectComposite();
		dataObject->Add(m_fileDataObject = new wxFileDataObject);
		dataObject->Add(m_actionDataObject = new wxCustomDataObject(actionDataFormat));
		SetDataObject(dataObject);
		SetDefaultAction(wxDragMove);
	}

	virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def) {
		wxDropTarget::OnEnter(x, y, def);
		return def;
	}

	virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def) {
		wxDropTarget::OnDragOver(x, y, def);
		return def;
	}

	virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def) {
		if (!GetData())
			return wxDragError;
		
		wxDataObjectComposite* dobj = (wxDataObjectComposite*) m_dataObject;
		
		if (dobj->GetReceivedFormat() == wxDF_FILENAME) {
			return OnDropFiles(x, y, m_fileDataObject->GetFilenames(), def) ? def : wxDragError;
		} else if (dobj->GetReceivedFormat().GetId() == DATAFORMAT_ACTION) {
			int x1, y1;
			m_pOwner->CalcUnscrolledPosition(x, y, &x1, &y1);
			m_pOwner->MoveSelectedTo(m_pOwner->GetItemIndex(x1, y1));
			return def;
		}
		
		return wxDragError;
	}

	bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames, wxDragResult def) {
		bool res = false;
		for (unsigned int i = 0; i < filenames.size(); i++)
			res = AddFile(filenames[i], i == 0, i == filenames.size() - 1, def) || res;
		return res;
	}
	
	bool AddFile(wxString filename, bool first, bool last, wxDragResult def) {
		if (wxImage::FindHandler(filename.AfterLast('.').Lower(), wxBITMAP_TYPE_ANY)) {
			m_pOwner->AddImage(filename, def == wxDragMove && first, last);
			return true;
		} else if (wxThumbnails::IsVideo(filename)) {
			return m_pOwner->AddVideo(filename, def == wxDragMove);
		} else if (wxThumbnails::IsAudio(filename)) {
			m_pOwner->AddAudio(filename);
			return true;
		} else if (wxThumbnails::IsSubtitle(filename)) {
			m_pOwner->AddSubtitles(filename);
			return true;
		}
		
		// try to load it as video
		return m_pOwner->AddVideo(filename, def == wxDragMove);
	}
protected:
	TitlesetManager *m_pOwner;
	wxFileDataObject* m_fileDataObject;
	wxCustomDataObject* m_actionDataObject;
};

//////////////////////////////////////////////////////////////////////////////
/////////////////////////// TitlesetManager //////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

enum {
	MENU_TITLE_DEL_ID = 2650,
	MENU_TITLE_COPY_ID,
	MENU_TITLE_PASTE_ID,
	MENU_TITLE_PROP_ID,
	MENU_TITLE_ADD_ID,
	MENU_TITLE_MOVE_ID,
	MENU_MAIN_ADD_ID,
	MENU_MOVE_TITLE_LEFT_ID,
	MENU_MOVE_TITLE_RIGHT_ID
};

BEGIN_EVENT_TABLE(TitlesetManager, wxThumbnails)
	EVT_UPDATE_UI(MainWin::MENU_DVD_ADD_TITLESET_ID, TitlesetManager::OnAddTitlesetUpdateUI)
	EVT_MENU(MENU_MOVE_TITLE_LEFT_ID, TitlesetManager::OnMoveTitleLeft)
	EVT_MENU(MENU_MOVE_TITLE_RIGHT_ID, TitlesetManager::OnMoveTitleRight)
	EVT_UPDATE_UI(MENU_MOVE_TITLE_LEFT_ID, TitlesetManager::OnMoveTitleLeftUpdateUI)
	EVT_UPDATE_UI(MENU_MOVE_TITLE_RIGHT_ID, TitlesetManager::OnMoveTitleRightUpdateUI)
	EVT_MENU(MENU_TITLE_DEL_ID, TitlesetManager::OnDelete)
	EVT_MENU(MENU_TITLE_COPY_ID, TitlesetManager::OnCopy)
	EVT_MENU(MENU_TITLE_PASTE_ID, TitlesetManager::OnPaste)
	EVT_UPDATE_UI(MENU_TITLE_PASTE_ID, TitlesetManager::OnUpdateUIObjectPaste)
	EVT_MENU(MENU_TITLE_PROP_ID, TitlesetManager::OnProps)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(EVT_COMMAND_MENU_SELECTED)
DEFINE_EVENT_TYPE(EVT_COMMAND_DVD_CHANGED)

TitlesetManager::TitlesetManager(wxWindow* parent, int id): wxThumbnails(parent, id) {
	SetThumbSize(96, 76);
	SetOrientaion(wxTHUMB_HORIZONTAL);
	SetDropTarget(new TitleDnDFile(this));
	EnableDragging(false); // disable standard dragging of wxThumbnails
	m_dvd = NULL;
	m_selectedMenu = -1;
	m_reEncodingByDefault = false;
	m_progressDlg = NULL;

	wxMenu* addMenu = new wxMenu;
	addMenu->Append(MainWin::MENU_DVD_ADD_FILE_ID, _("&File..."));
	addMenu->Append(MainWin::MENU_DVD_ADD_FILE_AS_CHAPTER_ID, _("File as &chapter..."));
	addMenu->Append(MainWin::MENU_DVD_ADD_TITLES_FROM_DVD_ID, _("Titles from &DVD..."));
	addMenu->Append(MainWin::MENU_DVD_ADD_MENU_ID, _("&Menu"));
	addMenu->Append(MainWin::MENU_DVD_ADD_CHAPTER_MENU_ID, _("&Chapter menu"));
	addMenu->Append(MainWin::MENU_DVD_ADD_VMMENU_ID, _("&VMGM Menu"));
	addMenu->Append(MainWin::MENU_DVD_ADD_TITLESET_ID, _("&Titleset"));

	m_mainMenu = new wxMenu;
	m_mainMenu->Append(MENU_MAIN_ADD_ID, _("&Add"), addMenu);
	wxMenuItem* item = new wxMenuItem(m_mainMenu, MENU_TITLE_PASTE_ID, _("&Paste"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_PASTE, wxART_MENU));
	m_mainMenu->Append(item);
	SetGlobalPopupMenu(m_mainMenu);


	wxMenu* moveMenu = new wxMenu;
	moveMenu->Append(MENU_MOVE_TITLE_LEFT_ID, _("&Left"));
	moveMenu->Append(MENU_MOVE_TITLE_RIGHT_ID, _("&Right"));

	wxMenu* addMenu2 = new wxMenu;
	addMenu2->Append(MainWin::MENU_DVD_ADD_FILE_ID, _("&File..."));
	addMenu2->Append(MainWin::MENU_DVD_ADD_FILE_AS_CHAPTER_ID, _("File as &chapter..."));
	addMenu2->Append(MainWin::MENU_DVD_ADD_TITLES_FROM_DVD_ID, _("Titles from &DVD..."));
	addMenu2->Append(MainWin::MENU_DVD_ADD_MENU_ID, _("&Menu"));
	addMenu2->Append(MainWin::MENU_DVD_ADD_CHAPTER_MENU_ID, _("&Chapter menu"));
	addMenu2->Append(MainWin::MENU_DVD_ADD_VMMENU_ID, _("&VMGM Menu"));
	addMenu2->Append(MainWin::MENU_DVD_ADD_TITLESET_ID, _("&Titleset"));

	m_titleMenu = new wxMenu;
	m_titleMenu->Append(MENU_TITLE_ADD_ID, _("&Add"), addMenu2);
	m_titleMenu->Append(MENU_TITLE_MOVE_ID, _("&Move"), moveMenu);
	item = new wxMenuItem(m_titleMenu, MENU_TITLE_COPY_ID, _("&Copy"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_COPY, wxART_MENU));
	m_titleMenu->Append(item);
	item = new wxMenuItem(m_mainMenu, MENU_TITLE_PASTE_ID, _("&Paste"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_PASTE, wxART_MENU));
	m_titleMenu->Append(item);
	item = new wxMenuItem(m_titleMenu, MENU_TITLE_DEL_ID, _("&Delete"));
	item->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE, wxART_MENU));
	m_titleMenu->Append(item);
	m_titleMenu->Append(MENU_TITLE_PROP_ID, _("&Properties..."));
	SetPopupMenu(m_titleMenu);
}

TitlesetManager::~TitlesetManager() {
	// nothing to do
}

void TitlesetManager::SetDVD(DVD* dvd) {
	m_dvd = dvd;
	SetSelected(-1);
	m_selectedMenu = 0;
	UpdateItems();
}

wxBitmap TitlesetManager::GetThumbImage(wxThumb& thumb) {
	if (!m_dvd)
		return wxBitmap();
#ifndef __WXGTK__
	if (m_progressDlg)
		m_progressDlg->Pulse();
#endif
	int id = thumb.GetId();
	Vob* vob = m_dvd->GetVob(DVD::GetTsi(id), DVD::IsMenu(id), DVD::GetPgci(id), DVD::GetVobi(id));
	wxBitmap img = thumb.GetBitmap();
	if (!img.IsOk()) {
		int titleHeight = m_tTextHeight + 2;
		if (m_dvd->GetTitlesets().size() > 1 || m_dvd->GetVmgm().size() > 0)
			titleHeight *= 2;
		if (vob->GetMenu()) {
			img = wxBitmap(vob->GetMenu()->GetImage(m_tWidth-8, m_tHeight-titleHeight, m_progressDlg));
			thumb.SetBitmap(img);
		} else if (vob->GetSlideshow()) {
			img = wxBitmap(vob->GetSlideshow()->GetThumbImage(m_tWidth-8, m_tHeight-titleHeight));
			thumb.SetBitmap(img);
		} else
			img = thumb.GetBitmap(this, m_tWidth, m_tHeight-titleHeight);
	}
	return img;
}

void TitlesetManager::DrawThumbnail(wxBitmap& bmp, wxThumb& thumb, int index) {
	if (!m_dvd)
		return;
	int id = thumb.GetId();
	int tsi = DVD::GetTsi(id);
	int pgci = DVD::GetPgci(id);
	int vobi = DVD::GetVobi(id);
	bool isMenu = DVD::IsMenu(id);
	Pgc* pgc = m_dvd->GetPgc(tsi, isMenu, pgci);
	Vob* vob = pgc->GetVobs()[vobi];

	wxMemoryDC dc;
	dc.SelectObject(bmp);
	dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
	int x = m_tBorder/2;
	int y = m_tBorder/2;
	int y1 = y;
	int tw = bmp.GetWidth() - m_tBorder + 2;
	int th = bmp.GetHeight() - m_tBorder + 2;
	int th1 = th;
	int titleHeight1 = m_dvd->GetTitlesets().size() > 1 || m_dvd->GetVmgm().size() > 0 ? m_tTextHeight + 2 : 0;
	int titleHeight2 = m_tTextHeight + 2;
	int titleHeight = titleHeight1 + titleHeight2;
	// background
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(GetBackgroundColour(), wxBRUSHSTYLE_SOLID));
	dc.DrawRectangle(0, 0, bmp.GetWidth(), bmp.GetHeight());
	if (m_dvd->GetTitlesets().size() > 1 || m_dvd->GetVmgm().size() > 0) {
		// rounded rectangle
		dc.SetBrush(wxBrush(wxColour(128, 128, 128), wxBRUSHSTYLE_SOLID));
		dc.DrawRoundedRectangle(x, y, tw, th, 12);
		if (IsBeginOfTitleset(tsi, pgci, vobi, isMenu) && IsEndOfTitleset(tsi, pgci, vobi, isMenu)) {
			// nothing to do
		} else if (IsBeginOfTitleset(tsi, pgci, vobi, isMenu)) {
			dc.DrawRectangle(bmp.GetWidth()/2, y, bmp.GetWidth()/2, th);
		} else if (IsEndOfTitleset(tsi, pgci, vobi, isMenu)) {
			dc.DrawRectangle(0, y, bmp.GetWidth()/2, th);
		} else {
			dc.DrawRectangle(0, y, bmp.GetWidth(), th);
		}
		y1 += titleHeight1;
		th1 -= titleHeight1;
	}
	if (pgc->GetVobs().Count() > 1) {
		dc.SetBrush(wxBrush(wxColour(64, 64, 64), wxBRUSHSTYLE_SOLID));
		if (vobi == 0) {
			dc.DrawRectangle(bmp.GetWidth()/2, y1, bmp.GetWidth()/2, th1);
		} else if (vobi == (int) pgc->GetVobs().Count() - 1) {
			dc.DrawRectangle(0, y1, bmp.GetWidth()/2, th1);
		} else {
			dc.DrawRectangle(0, y1, bmp.GetWidth(), th1);
		}
	}
	// rounded rectangle
	wxColour rectColour = isMenu ? wxColour(64, 64, 64) : wxColour(0, 0, 0);
	dc.SetBrush(wxBrush(rectColour, wxBRUSHSTYLE_SOLID));
	dc.DrawRoundedRectangle(x, y1, tw, th1, 12);
	// image
	wxBitmap img = GetThumbImage(thumb);
	if (index == m_pointed) {
		wxImage imgTmp = img.ConvertToImage();
		wxAdjustBrightness(imgTmp, wxRect(0, 0, img.GetWidth(), img.GetHeight()),
				POINTED_BRIGHTNESS);
		img = wxBitmap(imgTmp);
	}
	wxRect imgRect(x + (m_tWidth-img.GetWidth())/2, y + titleHeight + (m_tHeight-titleHeight-img.GetHeight())/2,
			img.GetWidth(), img.GetHeight());
	dc.DrawBitmap(img, imgRect.x, imgRect.y);
	// audio files
	if (vob->GetAudioFilenames().GetCount() > 0) {
		wxString text = vob->GetAudioFilenames()[0].AfterLast(wxT('.')).Lower();
		for (int fi = 1; fi < (int)vob->GetAudioFilenames().GetCount(); fi++)
			text += wxT(",") + vob->GetAudioFilenames()[fi].AfterLast(wxT('.')).Lower();
		dc.SetTextForeground(*wxWHITE);
		wxFont font(8, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
		dc.SetFont(font);
		wxCoord w, h;
		dc.GetTextExtent(text, &w, &h);
		dc.DrawText(text, imgRect.x + img.GetWidth() - w - 4, imgRect.y + img.GetHeight() - h);
	}
	// outline
	if (index == m_selectedMenu) {
		dc.SetPen(wxPen(*wxBLUE, 0, wxPENSTYLE_SOLID));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(imgRect.x-1, imgRect.y-1, imgRect.width+1, imgRect.height+1);
	}
	// draw titleset caption
	if (titleHeight1 > 0 && IsBeginOfTitleset(tsi, pgci, vobi, isMenu)) {
		dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
		dc.SetTextForeground(*wxWHITE);
		wxString text = wxT("VMGM");
		if (tsi >= 0) {
			text = _("Titleset") + wxString::Format(_T(" %d"), tsi + 1);
		}
		int sw, sh;
		dc.GetTextExtent(text, &sw, &sh);
		int tx = x + (m_tWidth-sw)/2;
		int ty = y + (titleHeight1-sh)/2;
		dc.DrawText(text, tx, ty);
	}
	// draw title caption
	dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
	dc.SetTextForeground(*wxWHITE);
	wxString title = isMenu ? _("Menu") : _("Title");
	title += wxString::Format(_T(" %d"), pgci+1);
	if (vobi>0)
		title += wxString::Format(_T("-%d"), vobi+1);
	int sw, sh;
	dc.GetTextExtent(title, &sw, &sh);
	int tx = x + (m_tWidth-sw)/2;
	int ty = y + titleHeight1 + (titleHeight2-sh)/2;
	dc.DrawText(title, tx, ty);
	// draw caption
	for (int i=0; i<thumb.GetCaptionLinesCount(m_tWidth-m_tCaptionBorder); i++) {
		wxString caption = thumb.GetCaption(i);
		dc.GetTextExtent(caption, &sw, &sh);
		tx = x + (m_tWidth-sw)/2;
		ty = y + m_tHeight + i*m_tTextHeight + (m_tTextHeight-sh)/2;
		dc.DrawText(caption, tx, ty);
	}

	dc.SelectObject(wxNullBitmap);
}

void TitlesetManager::Paint(wxPaintDC& dc) {
	wxThumbnails::Paint(dc);
	if (m_dvd->GetTitlesets().size() == 1 && m_dvd->GetTitlesets()[0]->GetTitles().size() == 0) {
		dc.SetPen(*wxBLACK_PEN);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
		int col = m_items.GetCount() % m_cols;
		dc.DrawText(_("Drag your video files from the File Browser to here."),
				col * (m_tWidth + m_tBorder) + 2 * m_tBorder, m_tBorder + m_tHeight / 2);
	}
}

int TitlesetManager::GetRowHeight(unsigned int begRow, unsigned int count) {
	int capHeight = 0;
	for (unsigned int i=begRow; i<begRow+count; i++)
		if (i < m_tCaptionHeight.GetCount())
			capHeight += m_tCaptionHeight[i]>0 ? m_tCaptionHeight[i] : 1;
	return (m_tHeight + m_tBorder)*count + capHeight*m_tTextHeight;
}

void TitlesetManager::UpdateItems(wxProgressDialog* progressDlg) {
	m_progressDlg = progressDlg;
	if (m_progressDlg)
		m_progressDlg->Pulse();
	wxThumbnailFactory::ClearQueue(this);

	int selectedMenuId = -1; // to update m_selectedMenu
	if (m_selectedMenu>=0 && m_selectedMenu<(int)m_items.Count())
		selectedMenuId = m_items[m_selectedMenu]->GetId();
	WX_CLEAR_ARRAY(m_items);
	Menus* menus = &m_dvd->GetVmgm();
	for (int pgci = 0; pgci<(int)menus->Count(); pgci++) {
		Pgc* pgc = (*menus)[pgci];
		if (!pgc->GetVobs().Count())
			continue;
		wxThumb* thumb = new wxThumb(_T(""));
		thumb->SetId(DVD::MakeId(-1, pgci, 0, true));
		m_items.Add(thumb);
	}
	for (int tsi = 0; tsi<(int)m_dvd->GetTitlesets().Count(); tsi++) {
		Titleset* ts = m_dvd->GetTitlesets()[tsi];
		menus = &ts->GetMenus();
		for (int pgci = 0; pgci<(int)menus->Count(); pgci++) {
			Pgc* pgc = (*menus)[pgci];
			if (!pgc->GetVobs().Count())
				continue;
			wxThumb* thumb = new wxThumb(_T(""));
			thumb->SetId(DVD::MakeId(tsi, pgci, 0, true));
			m_items.Add(thumb);
		}
		for (int pgci = 0; pgci<(int)ts->GetTitles().Count(); pgci++) {
			Pgc* pgc = ts->GetTitles()[pgci];
			for (int vobi = 0; vobi<(int)pgc->GetVobs().Count(); vobi++) {
				Vob* vob = pgc->GetVobs()[vobi];
				wxString fname;
				wxString caption;
				if (vob->GetSlideshow()) {
					if (vob->GetSlideshow()->GetCount() > 1)
						caption = wxString::Format(_("Slideshow (%d images)"), vob->GetSlideshow()->GetCount());
					else
						caption = _("Slideshow (1 image)");
				} else {
					fname = vob->GetFilename();
					caption = vob->GetFilenameDisplay().AfterLast(wxFILE_SEP_PATH);
					if (caption.Find(wxT('.')) > 0)
						caption = caption.BeforeLast(wxT('.'));
				}
				wxThumb* thumb = new wxThumb(fname, caption);
				thumb->SetId(DVD::MakeId(tsi, pgci, vobi, false));
				m_items.Add(thumb);
			}
		}
	}
	if (selectedMenuId>=0) // update m_selectedMenu
		m_selectedMenu = GetIndex(selectedMenuId);
	SetThumbSize(96, 76 + (m_dvd->GetTitlesets().size() > 1 || m_dvd->GetVmgm().size() > 0 ? m_tTextHeight : 0));
	UpdateProp();
	Refresh();
	wxSplitterWindow* p = (wxSplitterWindow*) GetParent();
	p->SetSashPosition(p->GetSashPosition() + (m_rows > 1 ? 0 : 1000));
	m_progressDlg = NULL;
}

int TitlesetManager::GetIndex(int tsi, int pgci, int vobi, bool menu) {
	return GetIndex(DVD::MakeId(tsi, pgci, vobi, menu));
}

int TitlesetManager::GetIndex(int id) {
	for (int i=0; i<(int)m_items.Count(); i++) {
		if (m_items[i]->GetId() == id)
			return i;
	}
	return -1;
}

/** Checks if video fit on DVD without re-encoding */
bool TitlesetManager::AskForReencoding(VideoFormat videoFormat) {
	// check if "copy" is selected for video
	bool isCopy = false;
	for (int tsi = 0; tsi < (int) (m_dvd->GetTitlesets().Count()); tsi++) {
		Titleset* ts = m_dvd->GetTitlesets()[tsi];
		for (int pgci = 0; pgci < (int) (ts->GetTitles().Count()); pgci++) {
			Pgc* pgc = ts->GetTitles()[pgci];
			for (int vobi = 0; vobi < (int) (pgc->GetVobs().Count()); vobi++) {
				Stream* stream = pgc->GetVobs()[vobi]->GetVideoStream();
				if (stream != NULL && stream->GetVideoFormat() == vfCOPY) {
					isCopy = true;
					break;
				}
			}
		}
	}
	// Ask to enable re-encoding
	if (isCopy && m_dvd->GetCapacity() != dcUNLIMITED && m_dvd->GetSize() > m_dvd->GetCapacityValue()) {
		if (wxMessageBox(_("The selected video files don't fit on DVD without re-encoding.\n\
Do you want to enable re-encoding?"), _("Burn"), wxYES_NO | wxICON_QUESTION, this) == wxYES) {
			m_reEncodingByDefault = true;
			for (int tsi = 0; tsi < (int) (m_dvd->GetTitlesets().Count()); tsi++) {
				Titleset* ts = m_dvd->GetTitlesets()[tsi];
				for (int pgci = 0; pgci < (int) (ts->GetTitles().Count()); pgci++) {
					Pgc* pgc = ts->GetTitles()[pgci];
					for (int vobi = 0; vobi < (int) (pgc->GetVobs().Count()); vobi++) {
						Stream* stream = pgc->GetVobs()[vobi]->GetVideoStream();
						if (stream != NULL)
							stream->SetDestinationFormat(videoFormat);
					}
				}
			}
			SendDvdChangedEvent();
		}
		return true;
	}
	return false;
}

bool TitlesetManager::AddVideo(const wxString& fname, bool createTitle, int tsi, bool allowMenuDuplication) {
	wxProgressDialog pdlg(wxT("DVDStyler"), wxString::Format(_("Adding file %s.\nPlease wait..."),
			wxFileNameFromPath(fname).c_str()), 99, this->GetParent(), wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_SMOOTH);
	pdlg.Show();
	pdlg.SetFocus();
	pdlg.Pulse();
	Vob* vob = new Vob(_T(""));
	if (!vob->SetFilename(fname)) {
		delete vob;
		return false;
	}
	
	Stream* videoStream = vob->GetVideoStream();
	if (videoStream == NULL) {
		if (vob->GetAudioStreamCount() == 0) {
			delete vob;
			wxLogError(_("File '%s' doesn't contain any audio or video streams."), fname.c_str());
			return false;
		}
		delete vob;
		return AddAudio(fname);
	}
	// auto-name the project after the first video added (VSO-style)
	if (m_dvd->GetLabel() == s_config.GetDefDiscLabel()) {
		bool hasVideo = false;
		for (size_t tsi = 0; !hasVideo && tsi < m_dvd->GetTitlesets().Count(); tsi++)
			for (size_t pgci = 0; !hasVideo && pgci < m_dvd->GetTitlesets()[tsi]->GetTitles().Count(); pgci++)
				if (m_dvd->GetTitlesets()[tsi]->GetTitles()[pgci]->GetVobs().Count() > 0)
					hasVideo = true;
		if (!hasVideo) {
			wxFileName fn(fname);
			m_dvd->SetLabel(fn.GetName());
		}
	}
	
	// set chapters
	int chapterCount = vob->GetChapterCount();
	if (chapterCount == 0) {
		if (s_config.GetDefChapterLength() > 0) {
			if (vob->GetDuration() > 0) {
				int duration = (int) vob->GetDuration() / 60;
				chapterCount = duration / s_config.GetDefChapterLength();
				if (chapterCount > 0
						&& duration - s_config.GetDefChapterLength()*chapterCount < s_config.GetDefChapterLength()/5)
					chapterCount--;
				if (chapterCount > 100)
					chapterCount = 100;
			} else {
				chapterCount = 10;
			}
		}
		long t = 0;
		for (int i = 0; i <= chapterCount; i++) {
			vob->GetCells().push_back(new Cell(t * 60000));
			t += s_config.GetDefChapterLength();
		}
	}
	// add a chapter near the end - fix for the "next" button and post command for some players
	if (s_config.GetAddChapterAtTitleEnd() && vob->GetDuration() > 1) {
		int duration = (int) vob->GetDuration();
		vob->GetCells().push_back(new Cell((duration - 1) * 1000));
	}
		
	int videoBitrateOld = m_dvd->GetVideoBitrate();
	
	if (m_dvd->GetTitlesets().size() == 0) {
		m_dvd->GetTitlesets().Add(new Titleset);
	}

	// check if chapter menu need to be updated
	bool updateChapterMenu = false;
	if (m_dvd->GetTitleMenuTemplate() && m_dvd->GetVmgm().size() > 0 && tsi < 0) {
		Titleset* titleset = m_dvd->GetTitlesets().back();
		if (titleset->GetMenus().GetPgciByEntry(wxT("ptt")) >= 0) {
			if (titleset->GetTitles().size() == 0) {
				updateChapterMenu = true;
			} else if (titleset->GetTitles().size() == 1) {
				updateChapterMenu = true;
				if (chapterCount == 0)
					chapterCount++;
				chapterCount += titleset->GetTitles()[0]->GetChapterCount();
			}
		}
	}

	if (tsi < 0) {
		tsi = m_dvd->GetTitlesets().GetCount() - 1;
	}
	PgcArray& pgcs = m_dvd->GetTitlesets()[tsi]->GetTitles();
	
	if (pgcs.Count() == 0) {
		// first title => choose aspect ratio
		pgcs.GetVideo().SetAspect(videoStream->GetSourceAspectRatio() > 1.4 ? ar16_9 : ar4_3);
		// check DVD video format
		if (m_dvd->GetTitlesets().Count() == 1) {
			int fps = lround(videoStream->GetSourceFps());
			if (isNTSC(m_dvd->GetVideoFormat()) && fps == 25) {
				if (wxMessageBox(_("This file contains video in PAL format. Do you want to change DVD format to PAL too?"),
						wxT("DVDStyler"), wxYES_NO|wxICON_QUESTION, this) == wxYES)
					m_dvd->SetVideoFormat(vfPAL);
			} else if (!isNTSC(m_dvd->GetVideoFormat()) && (fps == 24 || fps == 30)) {
				if (wxMessageBox(_("This file contains video in NTSC format. Do you want to change DVD format to NTSC too?"),
						wxT("DVDStyler"), wxYES_NO|wxICON_QUESTION, this) == wxYES)
					m_dvd->SetVideoFormat(vfNTSC);
			}
		}
	} else if (createTitle && pgcs.Count() >= 99) {
		wxLogError(_("Maximal 99 titles are allowed for one titleset."));
		return false;
	}
	if (pgcs.GetVideo().GetFormat() <= vfCOPY)
		pgcs.GetVideo().SetFormat(m_dvd->GetVideoFormat());
	// check if reencoding is needed
	unsigned int audioStreamIdx = 0;
	unsigned int subtitleStreamIdx = 0;
	bool hasVideo = false;
	for (unsigned int i=0; i<vob->GetStreams().size(); i++) {
		Stream* stream = vob->GetStreams()[i];
		if (stream->GetType() == stVIDEO) {
			// disable png strema and mjpeg stream, if it not the first one
			if (stream->GetSourceCodecName() == "png" || (stream->GetSourceCodecName() == "mjpeg" && hasVideo)) {
				// ignore png and mjpeg stream
				stream->SetDestinationFormat(vfNONE);
			} else if (m_reEncodingByDefault
					|| vob->GetStreams().size() == 1 // only video is available
					|| !stream->IsDvdCompliant(m_dvd->IsHD())
					|| stream->GetSourceVideoFormat() != pgcs.GetVideo().GetFormat()
					|| (s_config.GetDefRencodeNtscFilm() && lround(stream->GetSourceFps()) == 24)
					|| fabs(stream->GetSourceAspectRatio() - GetFrameAspectRatio(pgcs.GetVideo().GetAspect())) >= 0.1) {
				// set destination format (default COPY) if codec is not mpeg2
				stream->SetDestinationFormat(pgcs.GetVideo().GetFormat());
				vob->SetKeepAspectRatio(s_config.GetDefKeepAspectRatio());
				hasVideo = true;
			} else if (vob->GetAudioFilenames().Count() == 0 && MPEG::HasNavPacks(vob->GetFilename())) {
				vob->SetDoNotTranscode(true);  // do not remultiplex/transcode if the video is already a VOB
				hasVideo = true;
			}
		} else if (stream->GetType() == stAUDIO) {
			// set destination format (default COPY) if codec is not mp2/ac3 or sample rate is not 48 kHz
			if (stream->GetSourceAudioFormat() != m_dvd->GetAudioFormat()
					|| stream->GetSourceSampleRate() != 48000) {
				stream->SetDestinationFormat(m_dvd->GetAudioFormat());
				vob->SetDoNotTranscode(false);
			}
			if (s_config.GetDefAudio51() && m_dvd->GetAudioFormat() >= afAC3
					&& stream->GetSourceChannelNumber() < 6) {
				stream->SetDestinationFormat(m_dvd->GetAudioFormat());
				stream->SetChannelNumber(6);
				vob->SetDoNotTranscode(false);
			}
			if (s_config.GetDefAudioNormalize() && m_dvd->GetAudioFormat() == afAC3) {
				stream->SetDestinationFormat(m_dvd->GetAudioFormat());
				vob->SetDoNotTranscode(false);
				stream->SetAudioAdjustType(aatNormalize);
			}
			// set language
			wxString langCode = stream->GetLanguageCode();
			if (langCode.length()) {
				while (pgcs.GetAudioLangCodes().size() < audioStreamIdx + 1)
					pgcs.GetAudioLangCodes().push_back(s_config.GetDefAudioLanguage());
				pgcs.GetAudioLangCodes()[audioStreamIdx] = langCode;
			}
			audioStreamIdx++;
		} else if (stream->GetType() == stSUBTITLE) {
			stream->SetDestinationFormat(stream->GetSourceCodecName() == wxT("dvdsub") ? sfCOPY : sfNONE);
			if (stream->GetSubtitleFormat() != sfNONE) {
				// set language
				wxString langCode = stream->GetLanguageCode();
				if (langCode.length()) {
					while (pgcs.GetSubPictures().size() < subtitleStreamIdx + 1)
						pgcs.GetSubPictures().push_back(new SubPicture(s_config.GetDefSubtitleLanguage()));
					pgcs.GetSubPictures()[subtitleStreamIdx]->SetLangCode(langCode);
				}
				subtitleStreamIdx++;
			}
		}
	}
	
	// add vob
	Pgc* pgc = pgcs.Count()>0 ? pgcs.Last() : NULL;
	if (createTitle || pgc == NULL) {
		pgc = new Pgc;
		pgc->SetPostCommands(m_dvd->GetDefPostCommandStr());
		pgc->GetVobs().Add(vob);
		pgcs.Add(pgc);
		m_dvd->UpdateButtonImageFor(tsi, pgcs.GetCount() - 1);
		if (allowMenuDuplication && !m_dvd->HasButtonWithJumpAction(tsi, pgcs.GetCount() - 1) && pgcs.GetCount() > 4) {
			// find menu to duplicate
			bool found = false;
			Menus& vmgmMenus = m_dvd->GetVmgm();
			if (vmgmMenus.size() > 0) {
				for (unsigned int menuIdx = 0; menuIdx < vmgmMenus.size(); menuIdx++) {
					Menu* menu = vmgmMenus[menuIdx]->GetMenu();
					if (menu && menu->GetButtonByJumpAction(tsi, pgcs.GetCount() - 2)
							&& menu->GetButtonByJumpAction(tsi, pgcs.GetCount() - 3)) {
						DuplicateMenu(-1, menuIdx, tsi);
						found = true;
						break;
					}
				}
			}
			if (!found) {
				PgcArray& menus = m_dvd->GetPgcArray(tsi, true);
				for (unsigned int menuIdx = 0; menuIdx < menus.size(); menuIdx++) {
					Menu* menu = menus[menuIdx]->GetMenu();
					if (menu && menu->GetButtonByJumpAction(-2, pgcs.GetCount() - 2)
							&& menu->GetButtonByJumpAction(-2, pgcs.GetCount() - 3)) {
						DuplicateMenu(tsi, menuIdx, -2);
						break;
					}
				}
			}
		}
	} else {
		pgc->GetVobs().Add(vob);
		m_dvd->UpdateButtonImageFor(tsi, pgcs.GetCount() - 1);
	}
	
	// set aspect ratio if it isn't set
	if (pgcs.GetVideo().GetAspect() == arAUTO) {
		float aspect = vob->GetVideoStream()->GetSourceAspectRatio();
		if (aspect > 0)
			pgcs.GetVideo().SetAspect(aspect < 1.4 ? ar4_3 : ar16_9);
		else
			pgcs.GetVideo().SetAspect(m_dvd->GetAspectRatio());
	}
	
	// create chapter menu
	if (updateChapterMenu) {
		CreateChapterMenu(m_dvd->GetTitlesets().size() - 1, chapterCount);
	}

	UpdateItems(&pdlg);
	SendDvdChangedEvent();
	
	if (!AskForReencoding(pgcs.GetVideo().GetFormat())
			&& videoBitrateOld >= 2000 && m_dvd->GetVideoBitrate() < 2000) {
		wxMessageBox(_("The video bitrate is reduced below 2 Mb/s to fit your video on DVD. \
This can produce DVD with bad video quality."), wxT("DVDStyler"), wxOK | wxICON_WARNING, this);
	}
	
	return true;
}

bool TitlesetManager::AddAudio(const wxString& fname) {
	if (m_dvd->GetTitlesets().Count() == 0)
		m_dvd->GetTitlesets().Add(new Titleset);

	PgcArray& pgcs = m_dvd->GetTitlesets().Last()->GetTitles();
	if (pgcs.GetCount() == 0 || pgcs.Last()->GetVobs().GetCount() == 0) {
		wxLogError(_("Please add a video track first."));
		return false;
	}
	Vob* vob = pgcs.Last()->GetVobs().Last();
	if (!vob->AddAudioFile(fname))
		return false;
	vob->SetDoNotTranscode(false);
	// check if reencoding is needed
	Stream* stream = vob->GetStreams()[vob->GetStreams().size()-1];
	if (vob->GetStreams().size() == 2 && stream->GetSourceAudioFormat() != m_dvd->GetAudioFormat())
		stream->SetDestinationFormat(m_dvd->GetAudioFormat());
	else if (stream->GetSourceAudioFormat() != afMP2 && stream->GetSourceAudioFormat() != afAC3)
		stream->SetDestinationFormat(m_dvd->GetAudioFormat());
	else if (stream->GetSourceSampleRate() != 48000)
		stream->SetDestinationFormat(stream->GetSourceAudioFormat());
	if (s_config.GetDefAudio51() && m_dvd->GetAudioFormat() >= afAC3
			&& stream->GetSourceChannelNumber() < 6) {
		stream->SetDestinationFormat(m_dvd->GetAudioFormat());
		stream->SetChannelNumber(6);
		vob->SetDoNotTranscode(false);
	}
	if (s_config.GetDefAudioNormalize() && m_dvd->GetAudioFormat() == afAC3) {
		stream->SetDestinationFormat(m_dvd->GetAudioFormat());
		vob->SetDoNotTranscode(false);
		stream->SetAudioAdjustType(aatNormalize);
	}
	UpdateItems();
	SendDvdChangedEvent();
	return true;
}

bool TitlesetManager::AddSubtitles(const wxString& fname) {
	if (m_dvd->GetTitlesets().Count() == 0)
		m_dvd->GetTitlesets().Add(new Titleset);

	PgcArray& pgcs = m_dvd->GetTitlesets().Last()->GetTitles();
	if (pgcs.Count()==0 || pgcs.Last()->GetVobs().Count()==0) {
		wxLogError(_("Please add a video track first."));
		return false;
	}
	Vob* vob = pgcs.Last()->GetVobs().Last();
	vob->AddSubtitlesFile(fname);
	UpdateItems();
	SendDvdChangedEvent();
	return true;
}

void TitlesetManager::AddImage(const wxString& fname, bool createTitle, bool update) {
	if (m_dvd->GetTitlesets().Count() == 0)
		m_dvd->GetTitlesets().Add(new Titleset);

	PgcArray& pgcs = m_dvd->GetTitlesets().Last()->GetTitles();
	Pgc* pgc = pgcs.Count()>0 ? pgcs.Last() : NULL;

	if (createTitle || pgc == NULL) {
		pgc = new Pgc;
		pgc->SetPostCommands(m_dvd->GetDefPostCommandStr());
		pgcs.Add(pgc);
	}

	Slideshow* slideshow = pgc->GetSlideshow();
	if (slideshow == NULL) {
		if (pgcs.GetVideo().GetAspect() == arAUTO)
			pgcs.GetVideo().SetAspect(m_dvd->GetAspectRatio());
		slideshow = new Slideshow(m_dvd->GetVideoFormat(), pgcs.GetVideo().GetAspect());
		Vob* vob = new Vob(slideshow);
		pgc->GetVobs().Add(vob);
	}
	slideshow->AddSlide(new Slide(fname));
	
	if (update) {
		UpdateItems();
		SendDvdChangedEvent();
	}
}

void TitlesetManager::SelectMenu(int tsi, int pgci) {
	m_selectedMenu = GetIndex(tsi, pgci, 0, true);
	SendMenuSelectedEvent();
}

void TitlesetManager::SelectFirstMenu() {
	m_selectedMenu = -1;
	for (int tsi=-1; tsi<(int)m_dvd->GetTitlesets().Count(); tsi++) {
		if (m_dvd->GetMenu(tsi, 0)) {
			m_selectedMenu = GetIndex(tsi, 0, 0, true);
			break;
		}
	}
	SendMenuSelectedEvent();
}

bool TitlesetManager::IsBeginOfTitleset(int tsi, int pgci, int vobi, bool isMenu) {
	return pgci == 0 && vobi == 0 && (isMenu || tsi < 0 || m_dvd->GetTitlesets()[tsi]->GetMenus().size() == 0);
}

bool TitlesetManager::IsEndOfTitleset(int tsi, int pgci, int vobi, bool isMenu) {
	PgcArray& pgcs = m_dvd->GetPgcArray(tsi, isMenu);
	return (!isMenu || tsi < 0 || m_dvd->GetTitlesets()[tsi]->GetTitles().size() == 0)
			&& pgci == (int)pgcs.size() - 1 && vobi == (int)pgcs[pgci]->GetVobs().Count()-1;
}

void TitlesetManager::SendMenuSelectedEvent() {
	wxCommandEvent evt(EVT_COMMAND_MENU_SELECTED, this->GetId());
	GetEventHandler()->ProcessEvent(evt);
}

void TitlesetManager::SendDvdChangedEvent() {
	wxCommandEvent evt(EVT_COMMAND_DVD_CHANGED, this->GetId());
	GetEventHandler()->ProcessEvent(evt);
}

void TitlesetManager::OnMouseUp(wxMouseEvent& event) {
	wxThumbnails::OnMouseUp(event);
	// Popup menu
	if (event.RightUp())
		return;
	// select menu
	if (GetSelected()<0)
		return;
	int id = GetSelectedId();
	if (DVD::IsMenu(id)) {
		SelectMenu(DVD::GetTsi(id), DVD::GetPgci(id));
		Refresh();
	}
}

void TitlesetManager::OnMouseDClick(wxMouseEvent& WXUNUSED(event)) {
	wxCommandEvent evt;
	OnProps(evt);
}

void TitlesetManager::OnMouseMove(wxMouseEvent &event) {
	wxThumbnails::OnMouseMove(event);
	// -- drag & drop --
	if (event.Dragging() && m_selected >= 0 && m_beginDrag) {
		DVDAction action;
		action.SetTsi(DVD::GetTsi(GetSelectedId()));
		action.SetPgci(DVD::GetPgci(GetSelectedId()));
		action.SetMenu(DVD::IsMenu(GetSelectedId()));
		int vobi = DVD::GetVobi(GetSelectedId());
		if (vobi > 0) {
			Pgc* pgc = m_dvd->GetPgc(action.GetTsi(), action.IsMenu(), action.GetPgci());
			Vob* vob = pgc->GetVobs()[vobi];
			action.SetChapter(pgc->GetChapterCount(vobi) + (vob->GetChapters().length() ? 0 : -1));
		}
		wxString actionStr = action.Serialize();
		wxDataFormat format(DATAFORMAT_ACTION);
		wxCustomDataObject dataObj(format);
		dataObj.SetData(actionStr.length(), actionStr.ToAscii());
		wxDropSource source(dataObj, this);
		source.DoDragDrop(wxDrag_DefaultMove);
		m_beginDrag = false;
	}
}

void TitlesetManager::AddMenu() {
	int tsi = m_dvd->GetTitlesets().Count()-1;
	if (GetSelected()>=0)
		tsi = DVD::GetTsi(GetSelectedId());
	PgcArray& menus = m_dvd->GetPgcArray(tsi, true);
	if (menus.GetCount() >= 99) {
		wxLogError(_("Maximal 99 menus are allowed for one titleset."));
		return;
	}
	int pgci = m_dvd->AddMenu(tsi);
	UpdateItems();
	SelectMenu(tsi, pgci);
	SendDvdChangedEvent();
}

void TitlesetManager::AddVmMenu() {
	int tsi = -1;
	int pgci = m_dvd->AddMenu(tsi);
	UpdateItems();
	SelectMenu(tsi, pgci);
	SendDvdChangedEvent();
}

void TitlesetManager::AddTitleset() {
	m_dvd->AddTitleset();
	SetSelected(-1);
	AddMenu();
}

bool TitlesetManager::DuplicateMenu(int tsi, int pgci, int targetTsi) {
	Vob* vob = m_dvd->GetVob(tsi, true, pgci, 0);
	if (!vob || !vob->GetMenu())
		return false;
	wxSvgXmlNode* menuXml = vob->GetMenu()->GetXML(DVDSTYLER_XML);
	// add new menu
	int pgci2 = m_dvd->AddMenu(tsi);
	Menu* menu = m_dvd->GetMenu(tsi, pgci2);
	menu->PutXML(menuXml);
	delete menuXml;
	// fix jump actions
	int actionPgci = m_dvd->GetPgcArray(targetTsi, false).size() - 1;
	for (unsigned int obji = 0; obji < menu->GetObjectsCount(); obji++) {
		MenuObject* obj = menu->GetObject(obji);
		if (obj->IsButton() && !obj->GetAction().IsCustom()) {
			if (obj->GetAction().GetTsi() == targetTsi && !obj->GetAction().IsMenu() && !obj->GetAction().IsPlayAll()) {
				obj->GetAction().SetPgci(actionPgci);
				menu->UpdateButtonImageFor(targetTsi, actionPgci, m_dvd);
				actionPgci++;
			} else if (obj->GetAction().GetTsi() == -2 && obj->GetAction().IsMenu()) {
				if (obj->GetAction().GetPgci() == pgci2) { // next button
					obj->GetAction().SetPgci(pgci2 + 1);
				} else if (obj->GetAction().GetPgci() == 100) { // previous button
					obj->GetAction().SetPgci(pgci2 - 1);
				} else if (pgci2 > 2 && obj->GetAction().GetPgci() == pgci2 - 2) { // previous button
					obj->GetAction().SetPgci(pgci2 - 1);
				}
			}
		}
	}
	return true;
}

bool TitlesetManager::CreateChapterMenu(int targetTsi, int chapterCount) {
	if (!m_dvd->GetTitleMenuTemplate() || targetTsi < 0)
		return false;

	int chapter = 0;

	// check if chapter menu already exists
	if (targetTsi == 0 && m_dvd->GetTitlesets().size() == 0) {
		m_dvd->AddTitleset();
	}
	Titleset* titleset = m_dvd->GetTitlesets()[targetTsi];
	int pttMenu = titleset->GetMenus().GetPgciByEntry(wxT("ptt"));
	if (pttMenu >= 0) {
		for (unsigned int menuIdx = pttMenu; menuIdx < titleset->GetMenus().size(); menuIdx++) {
			Menu* menu = titleset->GetMenus()[menuIdx]->GetMenu();
			for (unsigned int obji = 0; obji < menu->GetObjectsCount(); obji++) {
				MenuObject* obj = menu->GetObject(obji);
				DVDAction& action = obj->GetAction();
				if (obj->IsButton() && !action.IsCustom() && !action.IsMenu() && action.GetTsi() == -2
						&& action.GetPgci() == 0 && action.GetChapter() > chapter) {
					chapter = action.GetChapter();
				}
			}
		}
	}
	if (chapter > 0)
		chapter++;

	// create menus
	while (chapter < chapterCount) {
		// add new menu
		int pgci2 = m_dvd->AddMenu(targetTsi);
		Pgc* pgc = m_dvd->GetPgc(targetTsi, true, pgci2);
		pgc->GetEntries().clear();
		if (chapter == 0) {
			pgc->GetEntries().insert(wxT("ptt"));
		}
		Menu* menu = m_dvd->GetMenu(targetTsi, pgci2);
		menu->PutXML(m_dvd->GetTitleMenuTemplate());
		menu->SetAspectRatio(m_dvd->GetAspectRatio());
		menu->SetVideoFormat(m_dvd->GetVideoFormat());

		// fix jump actions
		for (unsigned int obji = 0; obji < menu->GetObjectsCount(); obji++) {
			MenuObject* obj = menu->GetObject(obji);
			DVDAction& action = obj->GetAction();
			if (obj->IsButton() && !action.IsCustom()) {
				if (!action.IsMenu() && !action.IsPlayAll()) {
					action.SetTsi(-2);
					action.SetPgci(0);
					action.SetChapter(chapter);
					chapter++;
				} else if (action.IsMenu() && action.GetTsi() == -2) {
					if (action.GetPgci() == 0) {
						action.SetTsi(-1); // home button -> vmgm menu 1
					} else if (action.GetPgci() == 2) {
						action.SetPgci(pgci2 + 1); // next button -> menu + 1
					} else if (pgci2 > 0 && action.GetPgci() == 100) { // previous button -> menu - 1
						obj->GetAction().SetPgci(pgci2 - 1);
					}
				}
			}
			MenuObjectParam* initParam = obj->GetInitParam();
			if (initParam && (initParam->type == wxT("text") || initParam->type == wxT("string"))
					&& obj->GetParam(initParam->name) == wxT("Back")) {
				obj->SetParam(initParam->name, _("Back"));
				obj->UpdateSize();
			}
		}
		menu->UpdateButtonImageFor(-2, 0, m_dvd);
		if (chapter == 0) {
			break;
		}
	}
	return true;
}

void TitlesetManager::FixVobChapters(Pgc* pgc, int vobi, int vobi2) {
	wxString chapters = pgc->GetVobs()[vobi]->GetChapters();
	wxString chapters2 = pgc->GetVobs()[vobi2]->GetChapters();
	if (!(chapters + wxT(",")).StartsWith(wxT("0:00,")) && (chapters2 + wxT(",")).StartsWith(wxT("0:00,"))) {
		chapters = (chapters.length() ? wxT("0:00,") : wxT("0:00")) + chapters;
		pgc->GetVobs()[vobi]->SetChapters(chapters, vobi == 0);
		chapters2 = chapters2.length() > 4 ? chapters2.substr(5) : wxT("");
		pgc->GetVobs()[vobi2]->SetChapters(chapters2, vobi2 == 0);
	}
}

void TitlesetManager::MoveSelectedTo(int idx) {
	if (GetSelected() < 0)
		return;
	int id = GetSelectedId();
	int tsi = DVD::GetTsi(id);
	int pgci = DVD::GetPgci(id);
	bool isMenu = DVD::IsMenu(id);
	int vobi = DVD::GetVobi(id);
	PgcArray& pgcs = m_dvd->GetPgcArray(tsi, isMenu);
	
	int id2 = -1;
	if (idx < 0) {
		int tsi2 = m_dvd->GetTitlesets().size() - 1;
		int pgci2 = m_dvd->GetPgcArray(tsi2, false).size() - 1;
		id2 = DVD::MakeId(tsi2, pgci2 >= 0 ? pgci2 : 0, 0, false);
	} else
		id2 = GetItem(idx)->GetId();
	if (id == id2 || id2 < 0)
		return;
	int tsi2 = DVD::GetTsi(id2);
	int pgci2 = DVD::GetPgci(id2);
	bool isMenu2 = DVD::IsMenu(id2);
	int vobi2 = DVD::GetVobi(id2);
	
	if (!isMenu && isMenu2) {
		pgci2 = 0;
		isMenu2 = false;
		if (tsi2 == -1)
			return;
	} else if (isMenu && !isMenu2) {
		PgcArray& pgcs2 = m_dvd->GetPgcArray(tsi2, true);
		pgci2 = pgcs2.size() > 0 ? pgcs2.size() - 1 : 0;
		isMenu2 = true;
	}
	if (tsi2 == tsi) {
		if (pgci2 > pgci) {
			int pgciTmp = pgci;
			while (pgciTmp != pgci2) {
				Pgc* pgc = pgcs[pgciTmp];
				pgcs[pgciTmp] = pgcs[pgciTmp + 1];
				pgcs[++pgciTmp] = pgc;
			}
			if (isMenu) {
				SelectMenu(tsi2, pgci2);
			} else {
				for (int i = pgci; i <= pgci2; i++)
					m_dvd->UpdateButtonImageFor(tsi, i);
			}
		} else if (pgci2 < pgci) {
			int pgciTmp = pgci;
			while (pgciTmp != pgci2) {
				Pgc* pgc = pgcs[pgciTmp];
				pgcs[pgciTmp] = pgcs[pgciTmp - 1];
				pgcs[--pgciTmp] = pgc;
			}
			if (isMenu) {
				SelectMenu(tsi2, pgci2);
			} else {
				for (int i = pgci2; i <= pgci; i++)
					m_dvd->UpdateButtonImageFor(tsi, i);
			}
		} else if (vobi2 > vobi) {
			Pgc* pgc = pgcs[pgci];
			if (vobi == 0)
				FixVobChapters(pgc, vobi, vobi2);
			int vobiTmp = vobi;
			while (vobiTmp != vobi2) {
				Vob* vob = pgc->GetVobs()[vobiTmp];
				pgc->GetVobs()[vobiTmp] = pgc->GetVobs()[vobiTmp + 1];
				pgc->GetVobs()[++vobiTmp] = vob;
			}
			if (!isMenu)
				m_dvd->UpdateButtonImageFor(tsi, pgci);
		} else if (vobi2 < vobi) {
			Pgc* pgc = pgcs[pgci];
			if (vobi2 == 0)
				FixVobChapters(pgc, vobi2, vobi);
			int vobiTmp = vobi;
			while (vobiTmp != vobi2) {
				Vob* vob = pgc->GetVobs()[vobiTmp];
				pgc->GetVobs()[vobiTmp] = pgc->GetVobs()[vobiTmp - 1];
				pgc->GetVobs()[--vobiTmp] = vob;
			}
			if (!isMenu)
				m_dvd->UpdateButtonImageFor(tsi, pgci);
		} else
			return;
	} else {
		Pgc* pgc = pgcs[pgci];
		pgcs.RemoveAt(pgci);
		PgcArray& pgcs2 = m_dvd->GetPgcArray(tsi2, isMenu2);
		pgcs2.Insert(pgc, pgci2);
		if (!isMenu) {
			for (unsigned int i = pgci; i < pgcs.size(); i++)
				m_dvd->UpdateButtonImageFor(tsi, i);
			for (unsigned int i = pgci2; i < pgcs2.size(); i++)
				m_dvd->UpdateButtonImageFor(tsi2, i);
		} else {
			SelectMenu(tsi2, pgci2);
		}
		if (tsi >= 0 && m_dvd->GetPgcArray(tsi, false).size() == 0
				&& m_dvd->GetPgcArray(tsi, true).size() == 0) {
			delete m_dvd->GetTitlesets()[tsi];
			m_dvd->GetTitlesets().RemoveAt(tsi);
		}
	}
	m_dvd->UpdateMenusLocations();
	UpdateItems();
	SendDvdChangedEvent();
}

void TitlesetManager::OnAddTitlesetUpdateUI(wxUpdateUIEvent& event) {
	int cnt = m_dvd->GetTitlesets().Count();
	event.Enable(cnt>0 && m_dvd->GetTitlesets()[cnt-1]->GetTitles().Count());
}

void TitlesetManager::OnMoveTitleLeft(wxCommandEvent& WXUNUSED(event)) {
	if (GetSelected()<0)
		return;
	int id = GetSelectedId();
	int tsi = DVD::GetTsi(id);
	int pgci = DVD::GetPgci(id);
	bool isMenu = DVD::IsMenu(id);
	PgcArray& pgcs = m_dvd->GetPgcArray(tsi, isMenu);
	if (pgci == 0)
		return;
	Pgc* pgc = pgcs[pgci];
	pgcs[pgci] = pgcs[pgci-1];
	pgcs[pgci-1] = pgc;
	if (isMenu) {
		SelectMenu(tsi, pgci-1);
	}
	m_dvd->UpdateMenusLocations();
	UpdateItems();
	SendDvdChangedEvent();
	m_dvd->UpdateButtonImageFor(tsi, pgci - 1);
	m_dvd->UpdateButtonImageFor(tsi, pgci);
}

void TitlesetManager::OnMoveTitleRight(wxCommandEvent& WXUNUSED(event)) {
	if (GetSelected()<0)
		return;
	int id = GetSelectedId();
	int tsi = DVD::GetTsi(id);
	int pgci = DVD::GetPgci(id);
	bool isMenu = DVD::IsMenu(id);
	PgcArray& pgcs = m_dvd->GetPgcArray(tsi, isMenu);
	if (pgci == (int)pgcs.Count()-1)
		return;
	Pgc* pgc = pgcs[pgci];
	pgcs[pgci] = pgcs[pgci+1];
	pgcs[pgci+1] = pgc;
	if (isMenu)
		SelectMenu(tsi, pgci+1);
	m_dvd->UpdateMenusLocations();
	UpdateItems();
	SendDvdChangedEvent();
	m_dvd->UpdateButtonImageFor(tsi, pgci);
	m_dvd->UpdateButtonImageFor(tsi, pgci + 1);
}

void TitlesetManager::OnMoveTitleLeftUpdateUI(wxUpdateUIEvent& event) {
	if (GetSelected()<0)
		return;
	int id = GetSelectedId();
	int pgci = DVD::GetPgci(id);
	event.Enable(pgci>0);
}

void TitlesetManager::OnMoveTitleRightUpdateUI(wxUpdateUIEvent& event) {
	if (GetSelected()<0)
		return;
	int id = GetSelectedId();
	int tsi = DVD::GetTsi(id);
	int pgci = DVD::GetPgci(id);
	bool isMenu = DVD::IsMenu(id);
	PgcArray& pgcs = m_dvd->GetPgcArray(tsi, isMenu);
	event.Enable(pgci<(int)pgcs.Count()-1);
}

void TitlesetManager::OnDelete(wxCommandEvent& WXUNUSED(event)) {
	if (GetSelected() < 0)
		return;
	int id = GetSelectedId();
	int tsi = DVD::GetTsi(id);
	int pgci = DVD::GetPgci(id);
	bool isMenu = DVD::IsMenu(id);
	int vobi = DVD::GetVobi(id);
	if (s_config.GetTitleDeletePrompt()) {
		wxString caption = isMenu ? _("Menu") : _("Title");
		caption += wxString::Format(wxT(" %d"), pgci+1);
		if (vobi>0)
			caption += wxString::Format(_T("-%d"), vobi+1);
		MessageDlg msgDlg(this, wxString::Format(_("Do you really want to delete %s?"), caption.c_str()),
				_("Titleset Manager"), wxYES_NO|wxICON_QUESTION);
		if (msgDlg.ShowModal() != wxYES)
			return;
		s_config.SetTitleDeletePrompt(msgDlg.IsShowAgain());
	}
	if (isMenu) {
		Titleset* ts= NULL;
		if (tsi>=0)
			ts = m_dvd->GetTitlesets()[tsi];
		Menus* menus = &m_dvd->GetVmgm();
		if (ts)
			menus = &ts->GetMenus();
		if (pgci>=(int)menus->GetCount())
			return;
		Pgc* pgc = (*menus)[pgci];
		if (vobi>=(int)pgc->GetVobs().Count())
			return;
		delete pgc->GetVobs()[vobi];
		pgc->GetVobs().RemoveAt(vobi);
		// remove pgc if need
		if (pgc->GetVobs().Count() == 0) {
			delete pgc;
			menus->RemoveAt(pgci);
		}
		// remove titleset if need
		if (ts && ts->GetTitles().Count() == 0
				&& ts->GetMenus().Count() == 0
				&& m_dvd->GetTitlesets().Count() > 1) {
			delete ts;
			m_dvd->GetTitlesets().RemoveAt(tsi);
		}
		// select next menu
		if (GetSelected() == m_selectedMenu) {
			if (pgci >= (int)menus->Count())
				pgci = menus->Count()-1;
			if (pgci>=0)
				SelectMenu(tsi, pgci);
			else
				SelectFirstMenu();
		} else if (m_selectedMenu>=0) {
			wxThumb* mThumb = m_items[m_selectedMenu];
			int mTsi = (mThumb->GetId()>>24)-1;
			int mPgci = ((mThumb->GetId()>>8) & 0xFFFF) / 2;
			if (mTsi == tsi && mPgci > pgci)
				m_selectedMenu--;
		}
	} else { // title
		if (tsi >= (int)m_dvd->GetTitlesets().Count())
			return;
		Titleset* ts = m_dvd->GetTitlesets()[tsi];

		//title
		if (pgci>=(int)ts->GetTitles().GetCount())
			return;
		Pgc* pgc = ts->GetTitles()[pgci];
		if (vobi>=(int)pgc->GetVobs().Count())
			return;
		// remove vob
		delete pgc->GetVobs()[vobi];
		pgc->GetVobs().RemoveAt(vobi);
		// remove title if need
		if (pgc->GetVobs().Count() == 0) {
			delete pgc;
			ts->GetTitles().RemoveAt(pgci);
			for (unsigned int pgci2 = pgci; pgci2 <= ts->GetTitles().GetCount(); pgci2++) {
				m_dvd->UpdateButtonImageFor(tsi, pgci2);
			}
		} else {
			m_dvd->UpdateButtonImageFor(tsi, pgci);
		}
		// remove titleset if need
		if (ts->GetTitles().Count() == 0
				&& ts->GetMenus().Count() == 0
				&& m_dvd->GetTitlesets().Count() > 1) {
			delete ts;
			m_dvd->GetTitlesets().RemoveAt(tsi);
		}
	}
	SetSelected(-1);
	UpdateItems();
	SendDvdChangedEvent();
}

void TitlesetManager::OnProps(wxCommandEvent& WXUNUSED(event)) {
	if (GetSelected()<0)
		return;
	int id = GetSelectedId();
	int tsi = DVD::GetTsi(id);
	int pgci = DVD::GetPgci(id);
	bool isMenu = DVD::IsMenu(id);
	int vobi = DVD::GetVobi(id);
	if (isMenu) { // menu
		MenuPropDlg propDlg(this->GetParent(), m_dvd, tsi, pgci);
		if (propDlg.ShowModal() == wxID_OK) {
			SendMenuSelectedEvent(); // update MenuEditor
			SendDvdChangedEvent();
			UpdateItems();
		}
	} else { //title
		TitlePropDlg propDlg(this->GetParent(), m_dvd, tsi, pgci, vobi);
		if (propDlg.ShowModal() == wxID_OK) {
			m_dvd->UpdateButtonImageFor(tsi, pgci);
			SendMenuSelectedEvent(); // update MenuEditor
			SendDvdChangedEvent();
			UpdateItems();
		}
	}
	m_pointed = -1;
	Refresh();
}

void TitlesetManager::OnCopy(wxCommandEvent& WXUNUSED(event)) {
	int id = GetSelectedId();
	int tsi = DVD::GetTsi(id);
	int pgci = DVD::GetPgci(id);
	int vobi = DVD::GetVobi(id);
	Vob* vob = m_dvd->GetVob(tsi, DVD::IsMenu(id), pgci, vobi);
	if (!vob || !vob->GetMenu())
		return;
	CopyXmlToClipboard(vob->GetMenu()->GetXML(DVDSTYLER_XML), DATAFORMAT_MENU);
}

void TitlesetManager::OnPaste(wxCommandEvent& WXUNUSED(event)) {
	int tsi = GetSelected() >= 0 ? DVD::GetTsi(GetSelectedId()) : m_dvd->GetTitlesets().Count() - 1;
	int pgci = m_dvd->AddMenu(tsi);
	Menu* menu = m_dvd->GetMenu(tsi, pgci);
	wxSvgXmlDocument doc;
	if (!GetXmlFromClipboard(DATAFORMAT_MENU, doc))
		return;
	menu->PutXML(doc.GetRoot());
	UpdateItems();
	SelectMenu(tsi, pgci);
	SendDvdChangedEvent();
}

void TitlesetManager::OnUpdateUIObjectPaste(wxUpdateUIEvent& event) {
	event.Enable(wxTheClipboard->IsSupported(wxDataFormat(DATAFORMAT_MENU)));
}

