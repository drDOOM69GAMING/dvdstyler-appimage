/////////////////////////////////////////////////////////////////////////////
// Name:        Cache.cpp
// Purpose:     Manage cache of transcoded files
// Author:      Alex Thuering
// Created:     29.08.2008
// RCS-ID:      $Id: Cache.cpp,v 1.27 2016/12/29 12:32:41 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "Cache.h"
#include "Config.h"
#include "MessageDlg.h"
#include <wx/log.h>
#include <wx/dir.h>
#include <wx/file.h>
#include <wx/tokenzr.h>
#include <algorithm>

const wxString CACHE_SUB_DIR = wxT("dvd-cache");

CacheEntry::CacheEntry(int idx): m_idx(idx) {
	m_used = false;
	m_videoBitrate = 0;
	m_audioBitrate = 0;
	m_startTime = 0;
	m_recordingTime = -1;
}

CacheEntry::CacheEntry(Vob* vob, DVD* dvd, int idx) {
	m_videoFile = vob->GetFilename();
	m_audioFiles = vob->GetAudioFilenames();
	for (unsigned int i = 0; i < vob->GetSubtitles().GetCount(); i++)
		m_subtitleFiles.Add(vob->GetSubtitles()[i]->AsString());
	for (unsigned int stIdx = 0; stIdx < vob->GetStreams().size(); stIdx++)
		m_formats.Add(vob->GetStreams()[stIdx]->GetDestinationFormat());
	m_videoBitrate = dvd->GetVideoBitrate();
	m_audioBitrate = dvd->GetAudioBitrate();
	m_startTime = vob->GetStartTime();
	m_recordingTime = vob->GetRecordingTime();
	m_videoFilters = vob->GetAllVideoFilters();
	for (unsigned int stIdx = 0; stIdx < vob->GetStreams().size(); stIdx++) {
		if (vob->GetStreams()[stIdx]->GetType() == stAUDIO) {
			m_audioFilters += wxT('|') + vob->GetStreams()[stIdx]->GetAllAudioFilters(true);
		}
	}
	m_idx = idx;
	m_used = false;
}

bool CacheEntry::operator==(const CacheEntry& entry) const {
	if (m_videoFile != entry.m_videoFile
			|| m_audioFiles != entry.m_audioFiles
			|| m_subtitleFiles != entry.m_subtitleFiles
			|| m_videoBitrate != entry.m_videoBitrate
			|| m_audioBitrate != entry.m_audioBitrate
			|| m_startTime != entry.m_startTime
			|| m_recordingTime != entry.m_recordingTime
			|| m_videoFilters != entry.m_videoFilters
			|| m_audioFilters != entry.m_audioFilters
			|| m_formats.Count() != entry.m_formats.Count())
		return false;
	for (unsigned int i = 0; i < m_formats.GetCount(); i++)
		if (m_formats[i] != entry.m_formats[i])
			return false;
	return true;
}

wxSvgXmlNode* CacheEntry::GetXML() {
	wxSvgXmlNode* entryNode = new wxSvgXmlNode(wxSVGXML_ELEMENT_NODE, wxT("entry"));
	entryNode->AddProperty(wxT("idx"), wxString::Format(wxT("%d"), m_idx));
	entryNode->AddProperty(wxT("videoFile"), m_videoFile);
	if (m_audioFiles.size()) {
		wxString audioFiles;
		for (unsigned int i = 0; i < m_audioFiles.size(); i++)
			audioFiles += wxT('|') + m_audioFiles[i];
		entryNode->AddProperty(wxT("audioFiles"), audioFiles.substr(1));
	}
	if (m_subtitleFiles.size()) {
		wxString subtitleFiles;
		for (unsigned int i = 0; i < m_subtitleFiles.size(); i++)
			subtitleFiles += wxT('|') + m_subtitleFiles[i];
		entryNode->AddProperty(wxT("subtitleFiles"), subtitleFiles.substr(1));
	}
	wxString formats;
	for (unsigned int i = 0; i < m_formats.size(); i++)
		formats += wxT('|') + wxString::Format(wxT("%d"), m_formats[i]);
	entryNode->AddProperty(wxT("formats"), formats.substr(1));
	entryNode->AddProperty(wxT("videoBitrate"), wxString::Format(wxT("%d"), m_videoBitrate));
	entryNode->AddProperty(wxT("audioBitrate"), wxString::Format(wxT("%d"), m_audioBitrate));
	if (m_startTime > 0)
		entryNode->AddProperty(wxT("startTime"), wxString::Format(wxT("%g"), m_startTime));
	if (m_recordingTime != -1)
		entryNode->AddProperty(wxT("recordingTime"), wxString::Format(wxT("%g"), m_recordingTime));
	if (m_videoFilters.length() > 0)
		entryNode->AddProperty(wxT("videoFilters"), m_videoFilters);
	if (m_audioFilters.length() > 0)
		entryNode->AddProperty(wxT("audioFilters"), m_audioFilters);
	return entryNode;
}

bool CacheEntry::PutXML(wxSvgXmlNode* node) {
	wxString val;
	long lval;
	if (node->GetPropVal(wxT("idx"), &val) && val.ToLong(&lval)) {
		m_idx = (int) lval;
	} else {
		return false;
	}
	node->GetPropVal(wxT("videoFile"), &m_videoFile);
	if (node->GetPropVal(wxT("audioFiles"), &val) && val.length()) {
		wxStringTokenizer tokenizer(val, wxT("|"));
		while (tokenizer.HasMoreTokens()) {
			wxString token = tokenizer.GetNextToken();
			m_audioFiles.Add(token);
		}
	}
	if (node->GetPropVal(wxT("subtitleFiles"), &val) && val.length()) {
		wxStringTokenizer tokenizer(val, wxT("|"));
		while (tokenizer.HasMoreTokens()) {
			wxString token = tokenizer.GetNextToken();
			m_subtitleFiles.Add(token);
		}
	}
	if (node->GetPropVal(wxT("formats"), &val) && val.length()) {
		wxStringTokenizer tokenizer(val, wxT("|"));
		while (tokenizer.HasMoreTokens()) {
			long lval;
			if (tokenizer.GetNextToken().ToLong(&lval)) {
				m_formats.Add((int) lval);
			}
		}
	}
	if (node->GetPropVal(wxT("videoBitrate"), &val) && val.ToLong(&lval)) {
		m_videoBitrate = (int) lval;
	}
	if (node->GetPropVal(wxT("audioBitrate"), &val) && val.ToLong(&lval)) {
		m_audioBitrate = (int) lval;
	}
	double dval;
	if (node->GetPropVal(wxT("startTime"), &val) && val.ToDouble(&dval)) {
		m_startTime = lval;
	}
	if (node->GetPropVal(wxT("recordingTime"), &val) && val.ToDouble(&dval)) {
		m_recordingTime = lval;
	}
	node->GetPropVal(wxT("videoFilters"), &m_videoFilters);
	node->GetPropVal(wxT("audioFilters"), &m_audioFilters);

	return true;
}

Cache::Cache() {
	// noting to do
}

Cache::~Cache() {
	// noting to do
}

/**
 * Returns true if the cache is already initialized
 */
bool Cache::IsInitialized() {
	return m_dir.length() > 0;
}

bool Cache::SetTempDir(wxString tempDir) {
	// check if temp dir exist 
	if (!wxDir::Exists(tempDir) && !wxMkdir(tempDir)) {
		wxLogError(_T("Can't create directory '%s'"), tempDir.c_str());
		return false;
	}
	// set new cache directory
	wxString cacheDir = tempDir + wxFILE_SEP_PATH + CACHE_SUB_DIR;
	if (!wxDir::Exists(cacheDir) && !wxMkdir(cacheDir)) {
		wxLogError(_T("Can't create directory '%s'"), cacheDir.c_str());
		return false;
	}
	// clear cache map if cache directory is changed
	if (m_dir.length() > 0 && m_dir != cacheDir) {
		Clear();
	}
	bool doOpen = m_dir != cacheDir;
	m_dir = cacheDir;
	if (doOpen) {
		Open();
	}
	return true;
}

/**
 * Returns temporally directory
 */
wxString Cache::GetTempDir() {
	if (!IsInitialized())
		return s_config.GetTempDir(); // default temporally directory
	return m_dir.BeforeLast(wxFILE_SEP_PATH); // temporally directory without CACHE_SUB_DIR
}

int Cache::GetCount() {
	return m_cacheSet.size();
}

/**
 * Return size of files in cache (in KB)
 */
long Cache::GetSize() {
	long size = 0;
	for (const CacheEntry& entry : m_cacheSet) {
		wxString filename = GetFileName(entry.GetIdx());
		if (wxFile::Exists(filename)) {
			size += wxFile(filename).Length()/1024;
		}
	}
	return size;
}

wxString Cache::Find(Vob* vob, DVD* dvd) {
	CacheSet::iterator it = std::find(m_cacheSet.begin(), m_cacheSet.end(), CacheEntry(vob, dvd, -1));
	if (it == m_cacheSet.end())
		return wxT(""); // no entry for given source file exists

	// check if cache entry is valid
	wxString filename = GetFileName(it->GetIdx());
	if (wxFile::Exists(filename) && wxFile(filename).Length() > 0) {
		it->SetUsed(true);
		return filename;
	}
	return wxT("");
}

wxString Cache::Add(Vob* vob, DVD* dvd) {
	int idx = GenerateIndex();
	m_cacheSet.push_back(CacheEntry(vob, dvd, idx));
	wxString filename = GetFileName(idx);
	if (wxFileExists(filename) && !wxRemoveFile(filename)) {
		wxLogError(wxString::Format(_("Can't remove file '%s'"), filename.c_str()));
	}
	return filename;
}

void Cache::Remove(Vob* vob, DVD* dvd) {
	if (wxFileExists(vob->GetTmpFilename()))
		wxRemoveFile(vob->GetTmpFilename());
	CacheSet::iterator entry = std::find(m_cacheSet.begin(), m_cacheSet.end(), CacheEntry(vob, dvd, -1));
	if (entry != m_cacheSet.end())
		m_cacheSet.erase(entry);
}

bool Cache::Clear() {
	bool successful = true;
	if (!IsInitialized())
		return successful;
	m_cacheSet.clear();
	// remove files
	if (!wxDirExists(m_dir))
		return successful;
	wxDir d(m_dir);
	wxString fname;
	for (bool hasEntry = d.GetFirst(&fname, wxEmptyString, wxDIR_FILES); hasEntry;
			hasEntry = d.GetNext(&fname)) {
		if (!wxRemoveFile(m_dir + wxFILE_SEP_PATH + fname)) {
			wxLogError(wxString::Format(_("Can't remove file '%s'"), fname.c_str()));
			successful = false;
		}
	}
	d.Close();
	wxRmdir(m_dir);
	return successful;
}

void Cache::BeginClean() {
	for (CacheSet::iterator it = m_cacheSet.begin(); it != m_cacheSet.end(); it++)
		it->SetUsed(false);
}

void Cache::EndClean() {
	CacheSet::iterator it = m_cacheSet.begin();
	while (it != m_cacheSet.end()) {
		if (!it->IsUsed()) {
			if (wxFileExists(GetFileName(it->GetIdx())))
				wxRemoveFile(GetFileName(it->GetIdx()));
			it = m_cacheSet.erase(it);
		} else {
			it++;
		}
	}
}

wxString Cache::GetFileName(int index) {
	return m_dir + wxFILE_SEP_PATH + wxString::Format(wxT("entry%03d.vob"), index);
}

int Cache::GenerateIndex() {
	int idx = 0;
	for (CacheSet::iterator it = m_cacheSet.begin(); it != m_cacheSet.end(); it++) {
		if (it->GetIdx() > idx)
			idx = it->GetIdx();
	}
	return idx + 1;
}

/**
 * Loads list of cache entries
 */
bool Cache::Open() {
	wxString cacheFile =  m_dir + wxFILE_SEP_PATH + wxT("cache.xml");
	if (!wxFileExists(cacheFile))
		return false;
	
	wxSvgXmlDocument xml;
	if (!xml.Load(cacheFile)) {
		return false;
	}

	wxSvgXmlNode* root = xml.GetRoot();
	if (root == NULL || root->GetName() != wxT("cache")) {
		return false;
	}
	
	wxSvgXmlNode* child = root->GetChildren();
	while (child) {
		if (child->GetName() == wxT("entry")) {
			CacheEntry entry(-1);
			if (entry.PutXML(child)) {
				m_cacheSet.push_back(entry);
			}
		}
		child = child->GetNext();
	}
	return true;
}

/**
 * Saves list of cache entries
 */
void Cache::Save() {
	wxString cacheFile =  m_dir + wxFILE_SEP_PATH + wxT("cache.xml");
	if (m_cacheSet.size() == 0) {
		if (wxFileExists(cacheFile)) {
			wxRemoveFile(cacheFile);
		}
		return;
	}
	wxSvgXmlDocument* xml = new wxSvgXmlDocument;
	wxSvgXmlNode* root = new wxSvgXmlNode(wxSVGXML_ELEMENT_NODE, wxT("cache"));
	for (CacheSet::iterator it = m_cacheSet.begin(); it != m_cacheSet.end(); it++) {
		root->AddChild(it->GetXML());
	}
	xml->SetRoot(root);
	xml->Save(cacheFile);
	delete xml;
}

/**
 * Shows prompt dialog and clears or saves cache 
 */
void Cache::ShowClearPrompt(wxWindow* parent) {
	// check entries
	for (CacheSet::iterator it = m_cacheSet.begin(); it != m_cacheSet.end(); it++) {
		wxString filename = GetFileName(it->GetIdx());
		if (!wxFile::Exists(filename)) {
			m_cacheSet.erase(it);
		}
	}
	if (GetCount() == 0) {
		Save();
		return;
	}
	bool clear = s_config.GetCacheClearPrompt() == 1;
	if (s_config.GetCacheClearPrompt() == 0) {
		wxString msg = wxPLURAL("There is %d file (%0.1f MB) in the transcoding cache.",
				"There are %d files (%0.1f MB) in the transcoding cache.", GetCount());
		msg = wxString::Format(msg, GetCount(), ((float) GetSize()) / 1024) + wxT("\n");
		msg += _("Do you want to clear the cache?");
		MessageDlg msgDlg(parent, msg, _("Confirm"), wxYES_NO|wxICON_QUESTION);
		clear = msgDlg.ShowModal() == wxYES;
		s_config.SetCacheClearPrompt(msgDlg.IsShowAgain() ? 0 : (clear ? 1 : 2));
	}
	if (clear) {
		Clear();
	} else {
		Save();
	}
}
