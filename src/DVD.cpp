/////////////////////////////////////////////////////////////////////////////
// Name:        DVD.cpp
// Purpose:     The class to store a DVD Structure (Titles/Menus)
// Author:      Alex Thuering
// Created:     29.01.2003
// RCS-ID:      $Id: DVD.cpp,v 1.138 2016/03/12 17:48:16 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "DVD.h"
#include "Menu.h"
#include "Cache.h"
#include "Config.h"
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/sstream.h>
#include <wxSVGXML/svgxmlhelpr.h>
#include <wxVillaLib/utils.h>
#include <wxVillaLib/PropDlg.h>
#include "SysUtils.h"

#define TEMPLATES_DIR wxFindDataDirectory(_T("templates"))

DVD::DVD() {
	m_label = wxT("DVD");
	m_dvdResolution = dvdSD;
	m_capacity = dcDVD5;
	m_videoBitrateAuto = true;
	m_videoBitrate = 4500;
	m_audioBitrate = s_config.GetAudioBitrate();
	m_jumppad = false;
	m_emptyMenu = true;
	m_playAllRegister = -1;
	m_rememberLastButtonRegister = -1;
	m_lastMenuRegister = -1;
	m_videoFormat = vfPAL;
	m_audioFormat = afMP2;
	m_aspectRatio = ar4_3;
	m_defPostCommand = cmdCALL_LAST_MENU;
	titleMenuTemplate = NULL;
}

DVD::~DVD() {
	WX_CLEAR_ARRAY(m_vmgm);
	WX_CLEAR_ARRAY(m_titlesets);
	if (titleMenuTemplate)
		delete titleMenuTemplate;
}

/** Returns disc capacity in KB */
long DVD::GetCapacityValue() {
	if (m_capacity == dcCD)
		return 720000;
	else if (m_capacity == dcDVD1)
		return 1426063;
	else if (m_capacity == dcDVD2)
		return 2589982;
	else if (m_capacity == dcDVD5)
		return 4590208;
	else if (m_capacity == dcDVD9)
		return 8343424;
	return 0;
}

/** Calculates video bitrate if it set to auto */
void DVD::CalculateVideoBitrate(long& encSize, long& encDuration, long& fixSize, long& fixDuration, long& fixAvgBitrate) {
	fixSize = s_config.GetDvdReservedSpace();
	fixDuration = 0;
	if (m_videoBitrateAuto)
		SetVideoBitrate(500);
	// menus
	for (int tsi = -1; tsi < (int) GetTitlesets().Count(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, true);
		for (int pgci = 0; pgci < (int) pgcs.Count(); pgci++) {
			Pgc* pgc = pgcs[pgci];
			for (int vobi = 0; vobi < (int) pgc->GetVobs().Count(); vobi++) {
				Vob* vob = pgc->GetVobs()[vobi];
				fixSize += vob->GetSize(this);
			}
		}
	}
	// titles
	encSize = 0;
	encDuration = 0;
	long titlesFixSize = 0;
	long titlesFixDuration = 0;
	for (int tsi = 0; tsi < (int) GetTitlesets().Count(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, false);
		for (int pgci = 0; pgci < (int) pgcs.Count(); pgci++) {
			Pgc* pgc = pgcs[pgci];
			for (int vobi = 0; vobi < (int) pgc->GetVobs().Count(); vobi++) {
				Vob* vob = pgc->GetVobs()[vobi];
				VideoFormat videoFormat = vfCOPY;
				if (!vob->GetSlideshow() && !vob->GetDoNotTranscode()) {
					for (int stIdx = 0; stIdx < (int) vob->GetStreams().size(); stIdx++) {
						Stream* stream = vob->GetStreams()[stIdx];
						if (stream->GetType() == stVIDEO) {
							videoFormat = stream->GetVideoFormat();
							break;
						}
					}
				}
				if (videoFormat != vfCOPY) {
					encSize += vob->GetSize(this);
					encDuration += lround(vob->GetResultDuration());
				} else {
					long size = vob->GetSize(this);
					long dur = lround(vob->GetResultDuration());
					fixSize += size;
					fixDuration += dur;
					titlesFixSize += vob->GetSlideshow() ?
							vob->GetSlideshow()->GetResultDuration()*s_config.GetSlideshowVideoBitrate()/8 : size;
					titlesFixDuration += dur;
				}
			}
		}
	}
	
	long bitrate = GetVideoBitrate();
	if (m_videoBitrateAuto) {
		if (GetCapacity() != dcUNLIMITED) {
			// size - size of video by bitrate 500 + size of audio streams
			long s = GetCapacityValue() - encSize - fixSize;
			bitrate = 500 + (s > 0 && encDuration > 0 ? s/encDuration*8 : (encDuration > 0 ? 0 : 7500));
			if (bitrate > 8000)
				bitrate = 8000;
		} else
			bitrate = 4500;
		encSize = bitrate*encDuration/8;
		SetVideoBitrate(bitrate);
	}
	fixAvgBitrate = titlesFixDuration > 0 ? titlesFixSize/titlesFixDuration*8 : 0;
}

unsigned int DVD::GetAudioStreamCount() {
	unsigned int count = 1; // at least one audio stream
	for (TitlesetArray::iterator it = GetTitlesets().begin(); it != GetTitlesets().end(); it++)
		if (count < (*it)->GetTitles().GetAudioStreamCount())
			count = (*it)->GetTitles().GetAudioStreamCount();
	return count;
}

unsigned int DVD::GetSubtitleStreamsCount() {
	unsigned int count = 0;
	for (TitlesetArray::iterator it = GetTitlesets().begin(); it != GetTitlesets().end(); it++)
		if (count < (*it)->GetTitles().GetSubtitleStreamsCount())
			count = (*it)->GetTitles().GetSubtitleStreamsCount();
	return count;
}

/**
 * Adds a new Titleset
 * @return Index of new titleset (tsi)
 **/
int DVD::AddTitleset() {
	Titleset* titleset = new Titleset;
	titleset->GetMenus().SetAspectRatio(m_aspectRatio);
	titleset->GetMenus().SetVideoFormat(m_videoFormat);
	GetTitlesets().Add(titleset);
	return GetTitlesets().Count();
}

/**
 * Adds a new PGC to given titleset or VMG
 * @return Index of new PGC (pgci)
 **/
int DVD::AddMenu(int tsi) {
	return AddMenu(new Menu(this, tsi, GetPgcArray(tsi, true).GetCount(), m_videoFormat, m_aspectRatio), tsi);
}

/**
 * Adds a new PGC to given titleset or VMG
 * @return Index of new PGC (pgci)
 **/
int DVD::AddMenu(Menu* menu, int tsi) {
	Vob* vob = new Vob(menu);
	vob->SetPause(-1);
	Pgc* pgc = new Pgc;
	pgc->GetVobs().Add(vob);
	PgcArray& pgcs = GetPgcArray(tsi, true);
	if (pgcs.GetCount() == 0 || pgcs.GetVideo().GetFormat() <= vfCOPY)
		pgcs.GetVideo().SetFormat(menu->GetVideoFormat());
	pgcs.Add(pgc);
	return pgcs.Count()-1;
}

/**
 * Adds a new PGC to given titleset or VMG
 * @return Index of new PGC (pgci)
 **/
int DVD::AddPgc(int tsi, bool menu, Pgc* pgc) {
	if (!pgc)
		pgc = new Pgc;
	PgcArray& pgcs = GetPgcArray(tsi, menu);
	pgcs.Add(pgc);
	return pgcs.Count() - 1;
}

/** Returns PGCs array */
PgcArray& DVD::GetPgcArray(int tsi, bool menus) {
	if (tsi >= 0 && tsi < (int) GetTitlesets().size()) {
		Titleset* ts = GetTitlesets()[tsi];
		if (menus)
			return ts->GetMenus();
		else
			return ts->GetTitles();
	}
	return GetVmgm();
}
/** Returns PGC from given titleset */
Pgc* DVD::GetPgc(int tsi, bool isMenu, int pgci) {
	if (tsi >= (int) GetTitlesets().size())
		return NULL;
	PgcArray& pgcs = GetPgcArray(tsi, isMenu);
	return pgci >= 0 && pgci < (int) pgcs.Count() ? pgcs[pgci] : NULL;
}

/** Returns vob from given titleset, pgc */
Vob* DVD::GetVob(int tsi, bool isMenu, int pgci, int vobi) {
	Pgc* pgc = GetPgc(tsi, isMenu, pgci);
	return pgc && vobi >= 0 && vobi < (int) pgc->GetVobs().size() ? pgc->GetVobs()[vobi] : NULL;
}

Vob* DVD::GetMenuVob(int tsi, int pgci) {
	PgcArray& pgcs = GetPgcArray(tsi, true);
	if (pgci < 0 || pgci >= (int) pgcs.Count())
		return NULL;
	// find vob with menu
	for (int vobi = 0; vobi < (int) pgcs[pgci]->GetVobs().GetCount(); vobi++) {
		if (pgcs[pgci]->GetVobs()[vobi]->GetMenu() != NULL)
			return pgcs[pgci]->GetVobs()[vobi];
	}
	return NULL;
}

/** Returns menu from titleset tsi und pgc pgci */
Menu* DVD::GetMenu(int tsi, int pgci) {
	Vob* vob = GetMenuVob(tsi, pgci);
	if (vob)
		return vob->GetMenu();
	return NULL;
}

/** Returns true if at least one menu exist  */
bool DVD::HasMenus() {
	if (GetVmgm().size())
		return true;
	for (unsigned int i = 0; i < GetTitlesets().size(); i++)
		if (GetTitlesets()[i]->GetMenus().size())
			return true;
	return false;
}

/** Returns true if all menus are ok */
bool DVD::CheckMenus() {
	for (int tsi = -1; tsi < (int) GetTitlesets().Count(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, true);
		bool hasRootMenu = false;
		bool hasTitleMenu = false;
		for (unsigned int pgci = 0; pgci < pgcs.GetCount(); pgci++) {
			hasRootMenu = hasRootMenu || pgcs[pgci]->GetEntries().find(wxT("root")) != pgcs[pgci]->GetEntries().end();
			hasTitleMenu = hasTitleMenu || pgcs[pgci]->GetEntries().find(wxT("title")) != pgcs[pgci]->GetEntries().end();
			Menu* menu = pgcs[pgci]->GetMenu();
			if (menu) {
				wxString src = DVDAction::GetSourceStr(this, tsi, pgci, true, wxT("")) + wxString(wxT(": "));
				// check aspect ration and number of buttons
				if (menu->GetAspectRatio() == ar16_9) {
					if (pgcs.GetVideo().GetWidescreen() == wtAUTO && menu->GetButtonsCount() > 12) {
						wxLogError(src + _("Wide screen DVD menu can contain maximal 12 buttons"));
						return false;
					} else if (menu->GetButtonsCount() > 18) {
						wxLogError(src + _("Wide screen DVD menu (nopanscan/noletterbox) can contain maximal 18 buttons"));
						return false;
					}
				} else if (menu->GetButtonsCount() > 34) {
					wxLogError(src + _("DVD menu can contain maximal 34 buttons"));
					return false;
				}
				// check if buttons overlapping
				if (menu->isButtonsOverlapping(src)) {
					return false;
				}
			}
		}
		if (!hasRootMenu && tsi >= 0 && pgcs.GetCount() > 0) {
			pgcs[0]->GetEntries().insert(wxT("root"));
		}
		if (!hasTitleMenu && tsi == -1 && pgcs.GetCount() > 0) {
			pgcs[0]->GetEntries().insert(wxT("title"));
		}
	}
	if (!IsEmptyMenuEnabled() && HasCallLastMenu())
		EnableEmptyMenu();
	return true;
}

/** Returns true if all actions are valid */
bool DVD::CheckActions(bool skipInvalidTarget) {
	bool valid = true;
	for (int tsi = -1; tsi < (int) GetTitlesets().Count(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, true);
		for (unsigned int pgci = 0; pgci < pgcs.GetCount(); pgci++) {
			Menu* menu = pgcs[pgci]->GetMenu();
			if (menu) {
				for (unsigned int obji = 0; obji < menu->GetObjectsCount(); obji++) {
					MenuObject* obj = menu->GetObject(obji);
					if (obj->IsButton() && !obj->GetAction().IsValid(
							this, tsi, pgci, true, obj->GetId(true), true, true, skipInvalidTarget))
						valid = false;
				}
			}
			// check commands
			if (pgcs[pgci]->GetPreCommands().length()) {
				DVDAction action;
				action.SetCustom(pgcs[pgci]->GetPreCommands());
				if (!action.IsValid(this, tsi, pgci, true, wxT(""), true))
					valid = false;
			}
			if (pgcs[pgci]->GetPostCommands().length()) {
				DVDAction action;
				action.SetCustom(pgcs[pgci]->GetPostCommands());
				if (!action.IsValid(this, tsi, pgci, true, wxT(""), true))
					valid = false;
			}
		}
		if (tsi >= 0) {
			PgcArray& pgcs = GetPgcArray(tsi, false);
			for (unsigned int pgci = 0; pgci < pgcs.GetCount(); pgci++) {
				Pgc* pgc = pgcs[pgci];
				// check commands
				if (pgc->GetPreCommands().length()) {
					DVDAction action;
					action.SetCustom(pgc->GetPreCommands());
					if (!action.IsValid(this, tsi, pgci, false, wxT(""), true))
						valid = false;
				}
				if (pgc->GetPostCommands().length()) {
					DVDAction action;
					action.SetCustom(pgc->GetPostCommands());
					if (!action.IsValid(this, tsi, pgci, false, wxT(""), true))
						valid = false;
				}
				for (int vobi = 0; vobi<(int)pgc->GetVobs().Count(); vobi++) {
					Vob* vob = pgc->GetVobs()[vobi];
					for (vector<Cell*>::iterator it = vob->GetCells().begin(); it != vob->GetCells().end(); it++) {
						if ((*it)->GetCommands().length()) {
							DVDAction action;
							action.SetCustom((*it)->GetCommands());
							if (!action.IsValid(this, tsi, pgci, false, wxT(""), true))
								valid = false;
						}
					}
				}
			}
		}
	}
	if (!valid) {
		wxLogError(_("Some actions are invalid. Please click 'Details' for more information."));
	}
	return valid;
}

/** Updates menus locations. Call it after moving of menus. */
void DVD::UpdateMenusLocations() {
	for (int tsi = -1; tsi < (int) GetTitlesets().GetCount(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, true);
		for (unsigned int pgci = 0; pgci < pgcs.GetCount(); pgci++) {
			Menu* menu = pgcs[pgci]->GetMenu();
			if (menu)
				menu->UpdateMenuLocation(tsi, pgci);
		}
	}
}

/** Updates image in buttons with given jump action */
bool DVD::UpdateButtonImageFor(int actionTsi, int actionPgci) {
	bool result = false;
	for (int tsi = -1; tsi < (int) GetTitlesets().size(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, true);
		for (unsigned int pgci = 0; pgci < pgcs.size(); pgci++) {
			Menu* menu = pgcs[pgci]->GetMenu();
			if (menu)
				result |= menu->UpdateButtonImageFor(actionTsi != tsi ? actionTsi : -2, actionPgci, this);
		}
	}
	return result;
}


/** Fix coordinates of buttons if they are out of range */
void DVD::FixButtonCoordinates() {
	for (int tsi = -1; tsi < (int) GetTitlesets().size(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, true);
		for (unsigned int pgci = 0; pgci < pgcs.size(); pgci++) {
			Menu* menu = pgcs[pgci]->GetMenu();
			if (menu)
				menu->FixButtonCoordinates();
		}
	}
}


/** Returns true if button with given jump action exits */
bool DVD::HasButtonWithJumpAction(int actionTsi, int actionPgci, bool isMenu) {
	for (int tsi = -1; tsi < (int) GetTitlesets().size(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, true);
		for (unsigned int pgci = 0; pgci < pgcs.size(); pgci++) {
			Menu* menu = pgcs[pgci]->GetMenu();
			if (menu && menu->GetButtonByJumpAction(actionTsi != tsi ? actionTsi : -2, actionPgci, isMenu) != NULL)
				return true;
		}
	}
	return false;
}

/** Sets video format of DVD */
void DVD::SetVideoFormat(VideoFormat videoFormat) {
	m_videoFormat = videoFormat;
	for (int tsi=-1; tsi< (int) GetTitlesets().GetCount(); tsi++)
		GetPgcArray(tsi, true).SetVideoFormat(videoFormat);
}

/** Sets audio format of DVD */
void DVD::SetAudioFormat(AudioFormat format) {
	m_audioFormat = format;
}

/** Sets aspect ratio of DVD */
void DVD::SetAspectRatio(AspectRatio aspectRatio) {
	m_aspectRatio = aspectRatio;
	m_vmgm.SetAspectRatio(m_aspectRatio);
	for (int tsi=-1; tsi< (int) GetTitlesets().GetCount(); tsi++)
		GetPgcArray(tsi, true).SetAspectRatio(aspectRatio);
}

/** Returns default title post command */
wxString DVD::GetDefPostCommandStr() {
	if (m_defPostCommand == cmdCALL_LAST_MENU)
		return wxT("call last menu;");
	else if (m_defPostCommand == cmdCALL_ROOT_MENU)
		return GetVmgm().size() > 0 ? wxT("call vmgm menu 1;") : wxT("call menu entry root;");
	else if (m_defPostCommand == cmdJUMP_NEXT_TITLE)
		return wxT("jump next title;");
	return HasMenus() ? wxT("call menu;") : wxT("");
}

/** Sets default title post command */
void DVD::SetDefPostCommand(DefaultPostCommand defPostCommand, bool updateTitles) {
	m_defPostCommand = defPostCommand;
	if (updateTitles) {
		for (int tsi = 0; tsi< (int) GetTitlesets().GetCount(); tsi++)
			GetPgcArray(tsi, false).SetPostCommands(GetDefPostCommandStr());
	}
}

wxSvgXmlNode* findTitleSelectionMenu(DVD* dvd) {
	// find title selection menu
	int tsi = dvd->GetVmgm().size() > 0 ? -1 : 0;
	int pgci = -1;
	PgcArray& menus = dvd->GetPgcArray(tsi, true);
	for (unsigned int menuIdx = 0; menuIdx < menus.GetCount(); menuIdx++) {
		Menu* menu = menus[menuIdx]->GetMenu();
		if (!menu)
			continue;
		if ((menu->GetButtonByJumpAction(-2, 0) && menu->GetButtonByJumpAction(-2, 1) && menu->GetButtonByJumpAction(-2, 2))
				|| (menu->GetButtonByJumpAction(0, 0) && menu->GetButtonByJumpAction(0, 1) && menu->GetButtonByJumpAction(0, 2))
				|| (menu->GetButtonByJumpAction(0, 0) && menu->GetButtonByJumpAction(1, 0) && menu->GetButtonByJumpAction(2, 0))) {
			pgci = menuIdx;
			break;
		}
	}
	if (pgci >= 0) {
		Vob* vob = dvd->GetVob(tsi, true, pgci, 0);
		if (vob && vob->GetMenu()) {
			return vob->GetMenu()->GetXML(DVDSTYLER_XML);
		}
	}
	return NULL;
}

/** Loads a project file */
bool DVD::Open(wxString fname, wxProgressDialog* progressDlg) {
	wxSvgXmlDocument xml;
	if (!xml.Load(fname)) {
		wxLogError(_("Cannot open file '%s'."), fname.c_str());
		return false;
	}

	wxSvgXmlNode* root = xml.GetRoot();
	if (root == NULL || root->GetName() != wxT("dvdstyler")) {
		wxLogError(_("'%s' is not a DVDStyler project file"), fname.c_str());
		return false;
	}

	if (fname.EndsWith(wxT(".dvdt")))
		m_templateFile = fname;
	else
		m_filename = fname;
	if (!PutXML(xml, progressDlg))
		return false;


	if (m_templateFile == fname) {
		titleMenuTemplate = findTitleSelectionMenu(this);
	} else if (m_templateFile.length()) {
		wxLogNull log;
		DVD* dvd = new DVD();
		if (dvd->Open(m_templateFile))
			titleMenuTemplate = findTitleSelectionMenu(this);
		delete dvd;
	}

	return true;
}

/** Initializes object with XML data */
bool DVD::PutXML(const wxSvgXmlDocument& xml, wxProgressDialog* progressDlg) {
	WX_CLEAR_ARRAY(GetTitlesets());

	wxSvgXmlNode* root = xml.GetRoot();
	if (root == NULL || root->GetName() != wxT("dvdstyler")) {
		wxLogError(wxT("Invalid project XML data"));
		return false;
	}
	root->GetPropVal(wxT("name"), &m_label);
	m_label = ClipDiscLabel(m_label);

	wxString val;
	long lval;

	int format = 0;
	if (root->GetPropVal(wxT("format"), &val) && val.ToLong(&lval))
		format = int(lval);
	if (root->GetPropVal(wxT("template"), &val)) {
		if (!wxFileName(val).IsAbsolute()) {
			wxString templateDir = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH + wxT("templates")
					+ wxFILE_SEP_PATH;
			if (wxFileExists(TEMPLATES_DIR + val))
				m_templateFile = TEMPLATES_DIR + val;
			else if (wxFileExists(templateDir + val))
				m_templateFile = templateDir + val;
			else if (wxFileExists(TEMPLATES_DIR + wxT("Basic") + wxFILE_SEP_PATH + val))
				m_templateFile = TEMPLATES_DIR + wxT("Basic") + wxFILE_SEP_PATH + val;
			else
				m_templateFile = TEMPLATES_DIR + val;
		} else
			m_templateFile = val;
	}
	if (root->GetPropVal(wxT("outputDir"), &val))
		m_outputDir = val;
	if (root->GetPropVal(wxT("isoFile"), &val))
		m_isoFile = val;
	if (root->GetPropVal(wxT("resolution"), &val) && val.ToLong(&lval))
		m_dvdResolution = DvdResolution(lval);
	if (root->GetPropVal(wxT("capacity"), &val) && val.ToLong(&lval))
		m_capacity = DiscCapacity(lval + 3);
	if (root->GetPropVal(wxT("jumppad"), &val) && val.ToLong(&lval))
		m_jumppad = int(lval);
	if (root->GetPropVal(wxT("emptyMenu"), &val) && val.ToLong(&lval))
		m_emptyMenu = int(lval);
	if (root->GetPropVal(wxT("videoFormat"), &val) && val.ToLong(&lval))
		m_videoFormat = VideoFormat(lval + (format == 2 ? 2 : (format == 3 ? 1 : 0)));
	if (root->GetPropVal(wxT("audioFormat"), &val) && val.ToLong(&lval))
		m_audioFormat = AudioFormat(lval + (format == 2 ? 2 : (format == 3 ? 1 : 0)));
	if (root->GetPropVal(wxT("aspectRatio"), &val) && val.ToLong(&lval))
		m_aspectRatio = AspectRatio(lval);
	if (root->GetPropVal(wxT("videoBitrate"), &val) && val.ToLong(&lval)) {
		m_videoBitrateAuto = lval == -1;
		m_videoBitrate = lval == -1 ? 4500 : lval;
	}
	if (root->GetPropVal(wxT("audioBitrate"), &val) && val.ToLong(&lval))
		m_audioBitrate = lval;
	if (root->GetPropVal(wxT("defPostCommand"), &val) && val.ToLong(&lval))
		m_defPostCommand = DefaultPostCommand(lval);

	wxSvgXmlNode* child = root->GetChildren();
	while (child) {
		if (child->GetName() == wxT("colours")) {
			for (int i = 0; i < 16; i++) {
				wxString colourStr = child->GetAttribute(wxString::Format(wxT("colour%d"), i));
				if (colourStr.length() != 7 || colourStr[0] != wxT('#'))
					continue;
				long r = 0, g = 0, b = 0;
				if (colourStr.Mid(1,2).ToLong(&r, 16) && colourStr.Mid(3,2).ToLong(&g, 16)
						&& colourStr.Mid(5,2).ToLong(&b, 16))
					wxPropDlg::GetColourData().SetCustomColour(i, wxColour(r, g, b));
			}
		} else if (child->GetName() == wxT("vmgm")) {
			if (!GetVmgm().PutXML(child, this, progressDlg)) {
				wxLogError(_("Can't load vmgm menus"));
				return false;
			}
		} else if (child->GetName() == wxT("titleset")) {
			Titleset* titleset = new Titleset;
			if (titleset->PutXML(child, this, GetTitlesets().GetCount(), progressDlg))
				GetTitlesets().Add(titleset);
			else {
				delete titleset;
				wxLogError(_("Can't load titleset"));
				return false;
			}
		}
		child = child->GetNext();
	}
	return true;
}


/** Returns XML Document */
wxSvgXmlDocument* DVD::GetXml() {
	m_playAllRegister = -1;
	m_rememberLastButtonRegister = -1;
	m_lastMenuRegister = -1;
	wxSvgXmlDocument* xml = new wxSvgXmlDocument;
	wxSvgXmlNode* root = new wxSvgXmlNode(wxSVGXML_ELEMENT_NODE, wxT("dvdstyler"));
	root->AddProperty(wxT("format"), wxT("4"));
	if (m_templateFile.length() > 0) {
		wxString tplName = m_templateFile;
		wxString dir = TEMPLATES_DIR;
		wxString dir2 = wxStandardPaths::Get().GetUserDataDir() + wxFILE_SEP_PATH + wxT("templates") + wxFILE_SEP_PATH;
		if (tplName.StartsWith(dir))
			tplName = tplName.substr(dir.length());
		else if (tplName.StartsWith(dir2))
			tplName = tplName.substr(dir2.length());
		root->AddProperty(wxT("template"), tplName);
	}
	if (m_outputDir.length() > 0)
		root->AddProperty(wxT("outputDir"), m_outputDir);
	if (m_isoFile.length() > 0)
		root->AddProperty(wxT("isoFile"), m_isoFile);
	root->AddProperty(wxT("name"), m_label);
	if (m_dvdResolution != dvdSD)
		root->AddProperty(wxT("resolution"), wxString::Format(wxT("%d"), m_dvdResolution));
	if (m_capacity != dcDVD5)
		root->AddProperty(wxT("capacity"), wxString::Format(wxT("%d"), m_capacity - 3));
	if (m_defPostCommand != cmdCALL_LAST_MENU)
		root->AddProperty(wxT("defPostCommand"), wxString::Format(wxT("%d"), m_defPostCommand));
	if (m_jumppad)
		root->AddProperty(wxT("jumppad"), wxT("1"));
	if (!m_emptyMenu)
		root->AddProperty(wxT("emptyMenu"), wxT("0"));
	root->AddProperty(wxT("videoFormat"), wxString::Format(wxT("%d"), m_videoFormat));
	root->AddProperty(wxT("audioFormat"), wxString::Format(wxT("%d"), m_audioFormat));
	root->AddProperty(wxT("aspectRatio"), wxString::Format(wxT("%d"), m_aspectRatio));
	if (!m_videoBitrateAuto)
		root->AddProperty(wxT("videoBitrate"), wxString::Format(wxT("%d"), m_videoBitrate));
	if (m_audioBitrate != s_config.GetAudioBitrate())
		root->AddProperty(wxT("audioBitrate"), wxString::Format(wxT("%d"), m_audioBitrate));
	wxSvgXmlNode* coloursNode = NULL;
	for (int i = 0; i < 16; i++) {
		wxColour colour = wxPropDlg::GetColourData().GetCustomColour(i);
		if (!colour.Ok() || colour == *wxWHITE)
			continue;
		if (coloursNode == NULL) {
			coloursNode = new wxSvgXmlNode(wxSVGXML_ELEMENT_NODE, wxT("colours"));
			root->AddChild(coloursNode);
		}
		coloursNode->AddProperty(wxString::Format(wxT("colour%d"), i),
				wxString::Format(_T("#%02x%02x%02x"), colour.Red(), colour.Green(), colour.Blue()));
	}
	root->AddChild(new wxSvgXmlNode(wxSVGXML_COMMENT_NODE, wxT(""), wxT("************** VMGM **************")));
	root->AddChild(GetVmgm().GetXML(DVDSTYLER_XML, this));
	for (int i=0; i<(int)m_titlesets.GetCount(); i++) {
		if (m_titlesets[i]->GetMenus().Count() || m_titlesets[i]->GetTitles().Count()) {
			root->AddChild(new wxSvgXmlNode(wxSVGXML_COMMENT_NODE, wxT(""),
					wxString::Format(wxT("************** TITLESET %d **************"), i + 1)));
			root->AddChild(m_titlesets[i]->GetXML(DVDSTYLER_XML, this));
		}
	}
	xml->SetRoot(root);
	return xml;
}

bool DVD::Save(wxString fname) {
	if (fname.length() > 0)
		m_filename = fname;
	wxSvgXmlDocument* xml = GetXml();
	bool result = xml->Save(m_filename);
	delete xml;
	return result;
}

/** Stores object data to string */
wxString DVD::Serialize() {
	wxSvgXmlDocument* xml = GetXml();
	wxStringOutputStream stream;
	xml->Save(stream);
	delete xml;
	return stream.GetString();
}

/** Restores object from data */
void DVD::Deserialize(const wxString& data) {
	wxStringInputStream stream(data);
	wxSvgXmlDocument xml;
	xml.Load(stream);
	PutXML(xml, NULL);
}

bool DVD::SaveDVDAuthor(wxString fname) {
	// set entry root
	for (unsigned int i = 0; i < m_titlesets.GetCount(); i++) {
		Menus& menus = (Menus&) GetPgcArray(i, true);
		if (menus.size() > 0 && menus.GetPgciByEntry(wxT("root")) == -1)
			menus[0]->GetEntries().insert(wxT("root"));
	}
	// find free register for "play all" etc.
	m_playAllRegister = -1;
	m_rememberLastButtonRegister = -1;
	m_lastMenuRegister = -1;
	m_playAllRegister = HasPlayAllButton() ? FindFreeRegister() : -1;
	m_rememberLastButtonRegister = HasRememberLastButton(true) ? FindFreeRegister() : -1;
	m_lastMenuRegister = HasCallLastMenu() || HasRememberLastButton() ? FindFreeRegister() : -1;
	// save config for dvdauthor
	wxSvgXmlDocument xml;
	wxSvgXmlNode* root = new wxSvgXmlNode(wxSVGXML_ELEMENT_NODE, wxT("dvdauthor"));
	root->AddProperty(wxT("jumppad"), m_jumppad ? wxT("1") : wxT("0"));
	root->AddChild(GetVmgm().GetXML(DVDAUTHOR_XML, this)); // vmgm
	for (unsigned int i = 0; i < m_titlesets.GetCount(); i++) {
		int nextTitleset = i < m_titlesets.GetCount() - 1 && !m_titlesets[i + 1]->IsEmpty() ? i + 2 : -1;
		if (!m_titlesets[i]->IsEmpty())
			root->AddChild(m_titlesets[i]->GetXML(DVDAUTHOR_XML, this, nextTitleset)); // titleset
	}
	xml.SetRoot(root);
	return xml.Save(fname);
}

void DVD::RenderThumbnail(wxString fname) {
	int tsi = GetVmgm().size() > 0 ? -1 : 0;
	Vob* vob = GetVob(tsi, true, 0, 0);
	if (vob == NULL || vob->GetMenu() == NULL)
		return;
	Vob* vob2 = GetVob(tsi, true, 1, 0);
	if (vob2) {
		wxImage img1 = vob->GetMenu()->GetImage(102, 54);
		wxImage img2 = vob2->GetMenu()->GetImage(102, 54);
		wxBitmap bmp(img1.GetWidth()*2 + 1, img1.GetHeight());
		wxMemoryDC dc;
		dc.SelectObject(bmp);
		dc.SetBackground(*wxLIGHT_GREY_BRUSH);
		dc.Clear();
		dc.DrawBitmap(wxBitmap(img1), 0, 0);
		dc.DrawBitmap(wxBitmap(img2), img1.GetWidth() + 1, 0);
		bmp.SaveFile(fname, wxBITMAP_TYPE_PNG);
	} else
		vob->GetMenu()->GetImage(72, 54).SaveFile(fname);
}

wxString DVD::GetPath(bool withSep) {
	wxString path = wxPathOnly(m_filename);
	return withSep && path.length() > 0 ? path + wxFILE_SEP_PATH : path;
}

long DVD::GetSize(bool afterTranscoding) {
	int size = s_config.GetDvdReservedSpace();
	for (int tsi = -1; tsi<(int)GetTitlesets().Count(); tsi++) {
		for (int m=0; m<=1; m++) {
			bool menu = m == 0;
			if (tsi == -1 && !menu) // "titleset -1" contains only vmgm menus
				break;
			PgcArray& pgcs = GetPgcArray(tsi, menu);
			for (int pgci = 0; pgci<(int)pgcs.Count(); pgci++) {
				Pgc* pgc = pgcs[pgci];
				for (int vobi = 0; vobi<(int)pgc->GetVobs().Count(); vobi++) {
					Vob* vob = pgc->GetVobs()[vobi];
					size += afterTranscoding ? vob->GetTranscodedSize(this) : vob->GetSize(this);
				}
			}
		}
	}
	return size;
}

long DVD::GetRequiredSize(Cache* cache) {
	int size = 0;
	for (int tsi = -1; tsi<(int)GetTitlesets().Count(); tsi++) {
		for (int m=0; m<=1; m++) {
			bool menu = m == 0;
			if (tsi == -1 && !menu) // "titleset -1" contains only vmgm menus
				break;
			PgcArray& pgcs = GetPgcArray(tsi, menu);
			for (int pgci = 0; pgci<(int)pgcs.Count(); pgci++) {
				Pgc* pgc = pgcs[pgci];
				for (int vobi = 0; vobi<(int)pgc->GetVobs().Count(); vobi++)
					size += pgc->GetVobs()[vobi]->GetRequiredSize(this, cache);
			}
		}
	}
	return size;
}

/** Returns true if at least one button has checked flag "Play all" */
bool DVD::HasPlayAllButton() {
	for (int tsi = -1; tsi<(int)GetTitlesets().Count(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, true);
		for (int pgci = 0; pgci<(int)pgcs.Count(); pgci++) {
			Pgc* pgc = pgcs[pgci];
			for (int vobi = 0; vobi<(int)pgc->GetVobs().Count(); vobi++) {
				Vob* vob = pgc->GetVobs()[vobi];
				if (vob->GetMenu()) {
					Menu* menu = vob->GetMenu();
					for (unsigned int obji = 0; obji<menu->GetObjectsCount(); obji++)
						if (menu->GetObject(obji)->IsButton()
								&& menu->GetObject(obji)->GetAction().IsPlayAll())
							return true;
				}
			}
		}
	}
	return false;
}

/** Returns true if at least one button has checked flag "Play all titlesets" */
bool DVD::HasPlayAllTitlesetButton() {
	for (int tsi = -1; tsi<(int)GetTitlesets().Count(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, true);
		for (int pgci = 0; pgci<(int)pgcs.Count(); pgci++) {
			Pgc* pgc = pgcs[pgci];
			for (int vobi = 0; vobi<(int)pgc->GetVobs().Count(); vobi++) {
				Vob* vob = pgc->GetVobs()[vobi];
				if (vob->GetMenu()) {
					Menu* menu = vob->GetMenu();
					for (unsigned int obji = 0; obji<menu->GetObjectsCount(); obji++)
						if (menu->GetObject(obji)->IsButton()
								&& menu->GetObject(obji)->GetAction().IsPlayAll()
								&& menu->GetObject(obji)->GetAction().IsPlayAllTitlesets())
							return true;
				}
			}
		}
	}
	return false;
}

/** Returns true if at least one menu has checked flag "Remember last selected button" */
bool DVD::HasRememberLastButton(bool checkIfNeedRegister) {
	for (int tsi = -1; tsi<(int)GetTitlesets().Count(); tsi++) {
		PgcArray& pgcs = GetPgcArray(tsi, true);
		for (int pgci = 0; pgci<(int)pgcs.Count(); pgci++) {
			Pgc* pgc = pgcs[pgci];
			for (int vobi = 0; vobi<(int)pgc->GetVobs().Count(); vobi++) {
				Vob* vob = pgc->GetVobs()[vobi];
				if (vob->GetMenu() && vob->GetMenu()->GetRememberLastButton()) {
					if (!checkIfNeedRegister || vob->GetMenu()->GetRememberLastButtonRegister() == -1)
						return true;
				}
			}
		}
	}
	return false;
}

/** Returns true if at least one title use "call last menu" command */
bool DVD::HasCallLastMenu() {
	for (int tsi = -1; tsi<(int)GetTitlesets().Count(); tsi++) {
		if (GetPgcArray(tsi, false).HasCallLastMenu())
			return true;
	}
	return false;
}

int DVD::FindFreeRegister() {
	for (int g=0; g<=12; g++) {
		if (!IsRegisterUsed(g))
			return g;
	}
	return -1;
}

bool DVD::IsRegisterUsed(int g) {
	if (m_playAllRegister == g || m_rememberLastButtonRegister == g || m_lastMenuRegister == g)
		return true;
	wxString gstr = wxT("g") + wxString::Format(wxT("%d"),g);
	for (int tsi = -1; tsi<(int)GetTitlesets().Count(); tsi++) {
		for (int m=0; m<=1; m++) {
			bool menu = m == 0;
			if (tsi == -1 && !menu) // titleset -1 is contains only vmgm menus
				continue;
			PgcArray& pgcs = GetPgcArray(tsi, menu);
			for (int pgci = 0; pgci<(int)pgcs.Count(); pgci++) {
				Pgc* pgc = pgcs[pgci];
				if (pgc->GetPostCommands().Find(gstr) != -1
						|| pgc->GetPreCommands().Find(gstr) != -1)
					return true;
				for (int vobi = 0; vobi<(int)pgc->GetVobs().Count(); vobi++) {
					Vob* vob = pgc->GetVobs()[vobi];
					if (vob->GetMenu()) {
						Menu* menu = vob->GetMenu();
						if (menu->GetRememberLastButtonRegister() == g)
							return true;
						for (unsigned int obji = 0; obji<menu->GetObjectsCount(); obji++) {
							MenuObject* obj = menu->GetObject(obji);
							if (obj->IsButton() && obj->GetAction().IsCustom()
									&& obj->GetAction().GetCustom().Find(gstr) != -1)
								return true;
						}
						for (unsigned int aIdx = 0; aIdx<menu->GetActionsCount(); aIdx++) {
							DVDAction* action = menu->GetAction(aIdx);
							if (action->IsCustom() && action->GetCustom().Find(gstr) != -1)
								return true;
						}
					} else {
						for (vector<Cell*>::iterator it = vob->GetCells().begin(); it != vob->GetCells().end(); it++)
							if ((*it)->GetCommands().Find(gstr) != -1)
								return true;
					}
				}
			}
		}
	}
	return false;
}

wxArrayString DVD::GetVideoFormatLabels(bool copy, bool none, bool menu, bool hd) {
	wxArrayString formats;
	if (none)
		formats.Add(_("Omit"));
	if (copy)
		formats.Add(_("Copy"));
	if (hd) {
		formats.Add(wxT("PAL Half HD 1280x720"));
		formats.Add(wxT("NTSC Half HD 1280x720"));
		formats.Add(wxT("PAL HDV 1440x1080"));
		formats.Add(wxT("NTSC HDV 1440x1080"));
		formats.Add(wxT("PAL Full HD 1920x1080"));
		formats.Add(wxT("NTSC Full HD 1920x1080"));
	}
	formats.Add(wxT("PAL 720x576"));
	formats.Add(wxT("NTSC 720x480"));
	if (!menu) {
		formats.Add(wxT("PAL 704x576"));
		formats.Add(wxT("NTSC 704x480"));
		formats.Add(wxT("PAL 352x576"));
		formats.Add(wxT("NTSC 352x480"));
		formats.Add(wxT("PAL 352x288"));
		formats.Add(wxT("NTSC 352x240"));
	}
	return formats;
}

wxArrayString DVD::GetAudioFormatLabels(bool copy, bool none, bool pcm) {
	wxArrayString formats;
	if (none)
		formats.Add(_("Omit"));
	if (copy)
		formats.Add(_("Copy"));
	formats.Add(_("MP2 48 kHz"));
	formats.Add(_("AC3 48 kHz"));
	if (pcm)
		formats.Add(_("PCM 48 kHz"));
	return formats;
}

wxArrayString DVD::GetSubtitleFormatLabels(bool copy, bool none) {
	wxArrayString formats;
	if (none)
		formats.Add(_("Omit"));
	if (copy)
		formats.Add(_("Copy"));
	return formats;
}

wxArrayString DVD::GetAspectRatioLabels(bool autom) {
	wxArrayString formats;
	if (autom)
		formats.Add(_("Auto"));
	formats.Add(wxT("4:3"));
	formats.Add(wxT("16:9"));
	return formats;
}

wxArrayString DVD::GetDvdResolutionLabels() {
	wxArrayString labels;
	labels.Add(_("SD (standard)"));
	labels.Add(_("Half HD (1280 x 720)"));
	labels.Add(_("HDV (1440 x 1080)"));
	labels.Add(_("Full HD (1920 x 1080)"));
	return labels;
}

wxArrayString DVD::GetCapacityLabels() {
	wxArrayString labels;
	labels.Add(_("CD (700 MB)"));
	labels.Add(_("DVD-1 (1.4 GB)"));
	labels.Add(_("DVD-2 (2.6 GB)"));
	labels.Add(_("DVD-5 (4.7 GB)"));
	labels.Add(_("DVD-9 (8.5 GB)"));
	labels.Add(_("Unlimited"));
	return labels;
}

/** Default title post command */
wxArrayString DVD::GetDefPostCommandLabels() {
	wxArrayString labels;
	labels.push_back(_("Call last menu"));
	labels.push_back(_("Call root menu"));
	labels.push_back(_("Play next title"));
	return labels;
}

wxArrayString DVD::GetVideoBitrateLabels() {
	wxArrayString labels;
	labels.Add(_("Auto"));
	for (int i=8; i>=2; i--)
		labels.Add(wxString::Format(_("%d MBit/s"), i));
	labels.Add(_("Custom"));
	return labels;
}

wxArrayString DVD::GetVideoFormatNames() {
	wxArrayString formats;
	formats.Add(wxT("none"));
	formats.Add(wxT("copy"));
	formats.Add(wxT("pal"));
	formats.Add(wxT("ntsc"));
	return formats;
}

wxArrayString DVD::GetAudioFormatNames() {
	wxArrayString formats;
	formats.Add(wxT("none"));
	formats.Add(wxT("copy"));
	formats.Add(wxT("mp2"));
	formats.Add(wxT("ac3"));
	formats.Add(wxT("pcm"));
	return formats;
}

VideoFormat DVD::GetVideoFormatByName(wxString name) {
	int idx = GetVideoFormatNames().Index(name, false);
	return idx > 0 ? (VideoFormat) idx : vfNONE;
}

AudioFormat DVD::GetAudioFormatByName(wxString name) {
	int idx = GetAudioFormatNames().Index(name, false);
	return idx > 0 ? (AudioFormat) idx : afNONE;
}

wxString DVD::GetVideoFormatName(VideoFormat format) {
	return GetVideoFormatNames()[(int)format];
}

wxString DVD::GetAudioFormatName(AudioFormat format) {
	return GetAudioFormatNames()[(int)format];
}

const wxString audioLangCodesArray[] = { wxT("AA"), wxT("AB"), wxT("AF"), wxT("AM"), wxT("AR"), wxT("AS"),
		wxT("AY"), wxT("AZ"), wxT("BA"), wxT("BE"), wxT("BG"), wxT("BH"), wxT("BI"), wxT("BN"), wxT("BO"),
		wxT("BR"), wxT("CA"), wxT("CO"), wxT("CS"), wxT("CY"), wxT("DA"), wxT("DE"), wxT("DZ"), wxT("EL"),
		wxT("EN"), wxT("EO"), wxT("ES"), wxT("ET"), wxT("EU"), wxT("FA"), wxT("FI"), wxT("FJ"), wxT("FO"),
		wxT("FR"), wxT("FY"), wxT("GA"), wxT("GD"), wxT("GL"), wxT("GN"), wxT("GU"), wxT("HA"), wxT("HI"),
		wxT("HR"), wxT("HU"), wxT("HY"), wxT("IA"), wxT("IE"), wxT("IK"), wxT("IN"), wxT("IS"), wxT("IT"),
		wxT("IW"), wxT("JA"), wxT("JI"), wxT("JV"), wxT("KA"), wxT("KK"), wxT("KL"), wxT("KM"), wxT("KN"),
		wxT("KO"), wxT("KS"), wxT("KU"), wxT("KY"), wxT("LA"), wxT("LN"), wxT("LO"), wxT("LT"), wxT("LV"),
		wxT("MG"), wxT("MI"), wxT("MK"), wxT("ML"), wxT("MN"), wxT("MO"), wxT("MR"), wxT("MS"), wxT("MT"),
		wxT("MY"), wxT("NA"), wxT("NE"), wxT("NL"), wxT("NO"), wxT("OC"), wxT("OM"), wxT("OR"), wxT("PA"),
		wxT("PL"), wxT("PS"), wxT("PT"), wxT("QU"), wxT("RM"), wxT("RN"), wxT("RO"), wxT("RU"), wxT("RW"),
		wxT("SA"), wxT("SD"), wxT("SG"), wxT("SH"), wxT("SI"), wxT("SK"), wxT("SL"), wxT("SM"), wxT("SN"),
		wxT("SO"), wxT("SQ"), wxT("SR"), wxT("SS"), wxT("ST"), wxT("SU"), wxT("SV"), wxT("SW"), wxT("TA"),
		wxT("TE"), wxT("TG"), wxT("TH"), wxT("TI"), wxT("TK"), wxT("TL"), wxT("TN"), wxT("TO"), wxT("TR"),
		wxT("TS"), wxT("TT"), wxT("TW"), wxT("UK"), wxT("UR"), wxT("UZ"), wxT("VI"), wxT("VO"), wxT("WO"),
		wxT("XH"), wxT("YO"), wxT("ZH"), wxT("ZU") };
static wxArrayString s_audioLanguageCodes(136, audioLangCodesArray);

wxArrayString DVD::GetAudioLanguageCodes() {
	return s_audioLanguageCodes;
}

static map<wxString, wxString> s_languageMap;

map<wxString, wxString>& DVD::GetLanguageMap() {
	if (s_languageMap.size() == 0) {
		s_languageMap[wxT("Abkhazian")] = wxT("AB");
		s_languageMap[wxT("Afar")] = wxT("AA");
		s_languageMap[wxT("Afrikaans")] = wxT("AF");
		s_languageMap[wxT("Akan")] = wxT("AK");
		s_languageMap[wxT("Albanian")] = wxT("SQ");
		s_languageMap[wxT("Amharic")] = wxT("AM");
		s_languageMap[wxT("Arabic")] = wxT("AR");
		s_languageMap[wxT("Aragonese")] = wxT("AN");
		s_languageMap[wxT("Assamese")] = wxT("AS");
		s_languageMap[wxT("Armenian")] = wxT("HY");
		s_languageMap[wxT("Avaric")] = wxT("AV");
		s_languageMap[wxT("Avestan")] = wxT("AE");
		s_languageMap[wxT("Aymara")] = wxT("AY");
		s_languageMap[wxT("Azerbaijani")] = wxT("AZ");
		s_languageMap[wxT("Bashkir")] = wxT("BA");
		s_languageMap[wxT("Bambara")] = wxT("BM");
		s_languageMap[wxT("Basque")] = wxT("EU");
		s_languageMap[wxT("Belarusian")] = wxT("BE");
		s_languageMap[wxT("Bengali")] = wxT("BN");
		s_languageMap[wxT("Bihari")] = wxT("BH");
		s_languageMap[wxT("Bislama")] = wxT("BI");
		s_languageMap[wxT("Bosnian")] = wxT("BS");
		s_languageMap[wxT("Breton")] = wxT("BR");
		s_languageMap[wxT("Bulgarian")] = wxT("BG");
		s_languageMap[wxT("Burmese")] = wxT("MY");
		s_languageMap[wxT("Catalan; Valencian")] = wxT("CA");
		s_languageMap[wxT("Chamorro")] = wxT("CH");
		s_languageMap[wxT("Chechen")] = wxT("CE");
		s_languageMap[wxT("Chichewa; Chewa; Nyanja")] = wxT("NY");
		s_languageMap[wxT("Chinese")] = wxT("ZH");
		s_languageMap[wxT("Chuvash")] = wxT("CV");
		s_languageMap[wxT("Cornish")] = wxT("KW");
		s_languageMap[wxT("Corsican")] = wxT("CO");
		s_languageMap[wxT("Cree")] = wxT("CR");
		s_languageMap[wxT("Croatian")] = wxT("HR");
		s_languageMap[wxT("Czech")] = wxT("CS");
		s_languageMap[wxT("Danish")] = wxT("DA");
		s_languageMap[wxT("Divehi; Dhivehi; Maldivian;")] = wxT("DV");
		s_languageMap[wxT("Dzongkha")] = wxT("DZ");
		s_languageMap[wxT("English")] = wxT("EN");
		s_languageMap[wxT("Esperanto")] = wxT("EO");
		s_languageMap[wxT("Estonian")] = wxT("ET");
		s_languageMap[wxT("Ewe")] = wxT("EE");
		s_languageMap[wxT("Faroese")] = wxT("FO");
		s_languageMap[wxT("Fijian")] = wxT("FJ");
		s_languageMap[wxT("Finnish")] = wxT("FI");
		s_languageMap[wxT("French")] = wxT("FR");
		s_languageMap[wxT("Fula; Fulah; Pulaar; Pular")] = wxT("FF");
		s_languageMap[wxT("Galician")] = wxT("GL");
		s_languageMap[wxT("German")] = wxT("DE");
		s_languageMap[wxT("Greek, Modern")] = wxT("EL");
		s_languageMap[wxT("Guarani")] = wxT("GN");
		s_languageMap[wxT("Gujarati")] = wxT("GU");
		s_languageMap[wxT("Haitian; Haitian Creole")] = wxT("HT");
		s_languageMap[wxT("Hausa")] = wxT("HA");
		s_languageMap[wxT("Hebrew, Modern")] = wxT("HE");
		s_languageMap[wxT("Herero")] = wxT("HZ");
		s_languageMap[wxT("Hindi")] = wxT("HI");
		s_languageMap[wxT("Hiri Motu")] = wxT("HO");
		s_languageMap[wxT("Hungarian")] = wxT("HU");
		s_languageMap[wxT("Interlingua")] = wxT("IA");
		s_languageMap[wxT("Indonesian")] = wxT("ID");
		s_languageMap[wxT("Interlingue, Occidental")] = wxT("IE");
		s_languageMap[wxT("Irish")] = wxT("GA");
		s_languageMap[wxT("Igbo")] = wxT("IG");
		s_languageMap[wxT("Sichuan Yi, Nuosu")] = wxT("II");
		s_languageMap[wxT("Inupiaq")] = wxT("IK");
		s_languageMap[wxT("Ido")] = wxT("IO");
		s_languageMap[wxT("Icelandic")] = wxT("IS");
		s_languageMap[wxT("Italian")] = wxT("IT");
		s_languageMap[wxT("Inuktitut")] = wxT("IU");
		s_languageMap[wxT("Japanese")] = wxT("JA");
		s_languageMap[wxT("Javanese")] = wxT("JV");
		s_languageMap[wxT("Georgian")] = wxT("KA");
		s_languageMap[wxT("Kongo")] = wxT("KG");
		s_languageMap[wxT("Kikuyu, Gikuyu")] = wxT("KI");
		s_languageMap[wxT("Kwanyama, Kuanyama")] = wxT("KJ");
		s_languageMap[wxT("Kazakh")] = wxT("KK");
		s_languageMap[wxT("Kalaallisut, Greenlandic")] = wxT("KL");
		s_languageMap[wxT("Central Khmer")] = wxT("KM");
		s_languageMap[wxT("Kannada")] = wxT("KN");
		s_languageMap[wxT("Korean")] = wxT("KO");
		s_languageMap[wxT("Kanuri")] = wxT("KR");
		s_languageMap[wxT("Kashmiri")] = wxT("KS");
		s_languageMap[wxT("Kurdish")] = wxT("KU");
		s_languageMap[wxT("Komi")] = wxT("KV");
		s_languageMap[wxT("Kirghiz, Kyrgyz")] = wxT("KY");
		s_languageMap[wxT("Latin")] = wxT("LA");
		s_languageMap[wxT("Luxembourgish")] = wxT("LB");
		s_languageMap[wxT("Luganda")] = wxT("LG");
		s_languageMap[wxT("Limburgish")] = wxT("LI");
		s_languageMap[wxT("Lingala")] = wxT("LN");
		s_languageMap[wxT("Lao")] = wxT("LO");
		s_languageMap[wxT("Lithuanian")] = wxT("LT");
		s_languageMap[wxT("Luba-Katanga")] = wxT("LU");
		s_languageMap[wxT("Latvian")] = wxT("LV");
		s_languageMap[wxT("Malagasy")] = wxT("MG");
		s_languageMap[wxT("Marshallese")] = wxT("MH");
		s_languageMap[wxT("Manx")] = wxT("GV");
		s_languageMap[wxT("Maori")] = wxT("MI");
		s_languageMap[wxT("Macedonian")] = wxT("MK");
		s_languageMap[wxT("Malayalam")] = wxT("ML");
		s_languageMap[wxT("Mongolian")] = wxT("MN");
		s_languageMap[wxT("Marathi")] = wxT("MR");
		s_languageMap[wxT("Malay")] = wxT("MS");
		s_languageMap[wxT("Maltese")] = wxT("MT");
		s_languageMap[wxT("Nauru")] = wxT("NA");
		s_languageMap[wxT("Norwegian Bokmal")] = wxT("NB");
		s_languageMap[wxT("North Ndebele")] = wxT("ND");
		s_languageMap[wxT("Nepali")] = wxT("NE");
		s_languageMap[wxT("Ndonga")] = wxT("NG");
		s_languageMap[wxT("Dutch, Flemish")] = wxT("NL");
		s_languageMap[wxT("Norwegian Nynorsk")] = wxT("NN");
		s_languageMap[wxT("Norwegian")] = wxT("NO");
		s_languageMap[wxT("South Ndebele")] = wxT("NR");
		s_languageMap[wxT("Navajo, Navaho")] = wxT("NV");
		s_languageMap[wxT("Occitan (after 1500)")] = wxT("OC");
		s_languageMap[wxT("Ojibwa")] = wxT("OJ");
		s_languageMap[wxT("Church Slavic")] = wxT("CU");
		s_languageMap[wxT("Oromo")] = wxT("OM");
		s_languageMap[wxT("Oriya")] = wxT("OR");
		s_languageMap[wxT("Ossetian, Ossetic")] = wxT("OS");
		s_languageMap[wxT("Panjabi, Punjabi")] = wxT("PA");
		s_languageMap[wxT("Pali")] = wxT("PI");
		s_languageMap[wxT("Persian")] = wxT("FA");
		s_languageMap[wxT("Polish")] = wxT("PL");
		s_languageMap[wxT("Pashto, Pushto")] = wxT("PS");
		s_languageMap[wxT("Portuguese")] = wxT("PT");
		s_languageMap[wxT("Quechua")] = wxT("QU");
		s_languageMap[wxT("Romansh")] = wxT("RM");
		s_languageMap[wxT("Kirundi")] = wxT("RN");
		s_languageMap[wxT("Romanian, Moldavian")] = wxT("RO");
		s_languageMap[wxT("Russian")] = wxT("RU");
		s_languageMap[wxT("Kinyarwanda")] = wxT("RW");
		s_languageMap[wxT("Sanskrit")] = wxT("SA");
		s_languageMap[wxT("Sardinian")] = wxT("SC");
		s_languageMap[wxT("Sindhi")] = wxT("SD");
		s_languageMap[wxT("Northern Sami")] = wxT("SE");
		s_languageMap[wxT("Samoan")] = wxT("SM");
		s_languageMap[wxT("Sango")] = wxT("SG");
		s_languageMap[wxT("Serbian")] = wxT("SR");
		s_languageMap[wxT("Scottish Gaelic; Gaelic")] = wxT("GD");
		s_languageMap[wxT("Shona")] = wxT("SN");
		s_languageMap[wxT("Sinhala, Sinhalese")] = wxT("SI");
		s_languageMap[wxT("Slovak")] = wxT("SK");
		s_languageMap[wxT("Slovene")] = wxT("SL");
		s_languageMap[wxT("Somali")] = wxT("SO");
		s_languageMap[wxT("Southern Sotho")] = wxT("ST");
		s_languageMap[wxT("Spanish; Castilian")] = wxT("ES");
		s_languageMap[wxT("Sundanese")] = wxT("SU");
		s_languageMap[wxT("Swahili")] = wxT("SW");
		s_languageMap[wxT("Swati")] = wxT("SS");
		s_languageMap[wxT("Swedish")] = wxT("SV");
		s_languageMap[wxT("Tamil")] = wxT("TA");
		s_languageMap[wxT("Telugu")] = wxT("TE");
		s_languageMap[wxT("Tajik")] = wxT("TG");
		s_languageMap[wxT("Thai")] = wxT("TH");
		s_languageMap[wxT("Tigrinya")] = wxT("TI");
		s_languageMap[wxT("Tibetan")] = wxT("BO");
		s_languageMap[wxT("Turkmen")] = wxT("TK");
		s_languageMap[wxT("Tagalog")] = wxT("TL");
		s_languageMap[wxT("Tswana")] = wxT("TN");
		s_languageMap[wxT("Tonga (Tonga Islands)")] = wxT("TO");
		s_languageMap[wxT("Turkish")] = wxT("TR");
		s_languageMap[wxT("Tsonga")] = wxT("TS");
		s_languageMap[wxT("Tatar")] = wxT("TT");
		s_languageMap[wxT("Twi")] = wxT("TW");
		s_languageMap[wxT("Tahitian")] = wxT("TY");
		s_languageMap[wxT("Uighur, Uyghur")] = wxT("UG");
		s_languageMap[wxT("Ukrainian")] = wxT("UK");
		s_languageMap[wxT("Urdu")] = wxT("UR");
		s_languageMap[wxT("Uzbek")] = wxT("UZ");
		s_languageMap[wxT("Venda")] = wxT("VE");
		s_languageMap[wxT("Vietnamese")] = wxT("VI");
		s_languageMap[wxT("Volapuk")] = wxT("VO");
		s_languageMap[wxT("Walloon")] = wxT("WA");
		s_languageMap[wxT("Welsh")] = wxT("CY");
		s_languageMap[wxT("Wolof")] = wxT("WO");
		s_languageMap[wxT("Western Frisian")] = wxT("FY");
		s_languageMap[wxT("Xhosa")] = wxT("XH");
		s_languageMap[wxT("Yiddish")] = wxT("YI");
		s_languageMap[wxT("Yoruba")] = wxT("YO");
		s_languageMap[wxT("Zhuang, Chuang")] = wxT("ZA");
		s_languageMap[wxT("Zulu")] = wxT("ZU");
	}
	return s_languageMap;
}

static wxArrayString s_languageNames;

wxArrayString& DVD::GetLanguageNames() {
	if (s_languageNames.size() == 0) {
		map<wxString, wxString>& langMap = DVD::GetLanguageMap();
		for (map<wxString, wxString>::iterator it = langMap.begin(); it != langMap.end(); it++) {
			s_languageNames.Add(it->first);
		}
	}
	return s_languageNames;
}

const wxString charsetsArray[] = {
	wxT("ASCII"), wxT("BIG5"), wxT("BIG5-HKSCS"), wxT("BIG5-HKSCS:2001"), wxT("BIG5-HKSCS:1999"),
	wxT("CP850"), wxT("CP862"), wxT("CP866"), wxT("CP874"), wxT("CP932"), wxT("CP936"), wxT("CP949"), wxT("CP950"),
	wxT("CP1131"), wxT("CP1133"), wxT("CP1250"), wxT("CP1251"), wxT("CP1252"),wxT("CP1133"),
	wxT("CP1253"), wxT("CP1254"), wxT("CP1255"), wxT("CP1256"), wxT("CP1257"), wxT("CP1258"),
	wxT("EUC-CN"), wxT("EUC-JP"), wxT("EUC-KR"), wxT("EUC-TW"),
	wxT("GB18030"), wxT("GBK"), wxT("Georgian-Academy"), wxT("Georgian-PS"), wxT("HZ"),
	wxT("ISO-2022-CN"), wxT("ISO-2022-CN-EXT"), wxT("ISO-2022-JP"), wxT("ISO-2022-JP-2"), wxT("ISO-2022-JP-1"),
	wxT("ISO-2022-KR"), wxT("ISO-8859-1"), wxT("ISO-8859-2"), wxT("ISO-8859-3"), wxT("ISO-8859-4"), wxT("ISO-8859-5"),
	wxT("ISO-8859-6"), wxT("ISO-8859-7"), wxT("ISO-8859-8"), wxT("ISO-8859-9"), wxT("ISO-8859-10"), wxT("ISO-8859-11"),
	wxT("ISO-8859-13"), wxT("ISO-8859-14"), wxT("ISO-8859-15"), wxT("ISO-8859-16"),
	wxT("JOHAB"), wxT("KOI8-R"), wxT("KOI8-RU"), wxT("KOI8-T"), wxT("KOI8-U"),
	wxT("Macintosh"), wxT("MacArabic"), wxT("MacCentralEurope"), wxT("MacCroatian"), wxT("MacCyrillic"),
	wxT("MacGreek"), wxT("MacHebrew"), wxT("MacIceland"), wxT("MacRoman"), wxT("MacRomania"), wxT("MacThai"),
	wxT("MacTurkish"), wxT("MacUkraine"), wxT("MuleLao-1"),
	wxT("PT154"), wxT("RK1048"), wxT("SHIFT_JIS"), wxT("TCVN"), wxT("TIS-620"),
	wxT("UCS-2"), wxT("UCS-2BE"), wxT("UCS-2LE"), wxT("UCS-4"), wxT("UCS-4BE"), wxT("UCS-4LE"),
	wxT("UTF-7"), wxT("UTF-8"), wxT("UTF-16"), wxT("UTF-16BE"), wxT("UTF-16LE"),
	wxT("UTF-32"), wxT("UTF-32BE"), wxT("UTF-32LE"), wxT("VISCII")
};
static wxArrayString s_charsets(94, charsetsArray);

wxArrayString& DVD::GetCharsets() {
	return s_charsets;
}
