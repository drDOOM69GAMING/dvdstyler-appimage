/////////////////////////////////////////////////////////////////////////////
// Name:        DVD.h
// Purpose:     The class to store a DVD Structure (Titles/Menus)
// Author:      Alex Thuering
// Created:     29.01.2003
// RCS-ID:      $Id: DVD.h,v 1.89 2016/03/12 17:48:16 ntalex Exp $
// Copyright:   (c) Alex Thuering
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#ifndef DVD_H
#define DVD_H

#include "Titleset.h"
#include "SysUtils.h"

enum DvdResolution {
	dvdSD = 0,
	dvdHalfHD,
	dvdHDV,
	dvdFullHD
};

enum DiscCapacity {
	dcCD = 0,
	dcDVD1,
	dcDVD2,
	dcDVD5,
	dcDVD9,
	dcUNLIMITED
};

enum DefaultPostCommand {
	cmdCALL_LAST_MENU = 0,
	cmdCALL_ROOT_MENU,
	cmdJUMP_NEXT_TITLE
};

class DVD {
public:
    DVD();
    ~DVD();
	
    /** Returns disc label */
    wxString GetLabel() { return m_label; }
    /** Sets disc label */
    void SetLabel(wxString label) { m_label = ClipDiscLabel(label); }
	
    /** Returns DVD resolution (SD / Half HD / Full HD)  */
    DvdResolution GetDvdResolution() { return m_dvdResolution; }
    /** Sets DVD resolution */
    void SetDvdResolution(DvdResolution dvdResolution) { m_dvdResolution = dvdResolution; }
    /** Returns true, if it's a HD DVD  */
	bool IsHD() { return m_dvdResolution > dvdSD; }
    /** Returns disc capacity */
    DiscCapacity GetCapacity() { return m_capacity; }
    /** Sets disc capacity */
    void SetCapacity(DiscCapacity capacity) { m_capacity = capacity; }
    /** Returns disc capacity in KB */
    long GetCapacityValue();
	
    /** Returns true if video bitrate will be calculated automatically */
	bool IsVideoBitrateAuto() { return m_videoBitrateAuto; }
	/** Sets if video bitrate will be calculated automatically */
	void SetVideoBitrateAuto(bool bitrateAuto) { m_videoBitrateAuto = bitrateAuto; }
	/** Returns video bitrate */
    int GetVideoBitrate() { return m_videoBitrate; }
    /** Sets video bitrate */
    void SetVideoBitrate(int bitrate) { m_videoBitrate = bitrate; }
    /** Calculates video bitrate if it set to auto */
    void CalculateVideoBitrate(long& encSize, long& encDuration, long& fixSize, long& fixDuration, long& fixAvgBitrate);
	/** Returns audio bitrate */
    int GetAudioBitrate() { return m_audioBitrate; }
    /** Sets audio bitrate */
    void SetAudioBitrate(int bitrate) { m_audioBitrate = bitrate; }
	
	/** Returns true if creation of jumppads enabled */
	bool IsJumppadsEnabled() { return m_jumppad; }
	/** Enables creation of jumppads */
	void EnableJumppads(bool enable = true) { m_jumppad = enable; }
	
	/** Returns true if creation of empty menu enabled */
	bool IsEmptyMenuEnabled() { return m_emptyMenu; }
	/** Enables creation of empty menu */
	void EnableEmptyMenu(bool enable = true) { m_emptyMenu = enable; }
	
	/** Returns titlesets */
	TitlesetArray& GetTitlesets() { return m_titlesets; }
	/** Returns vmgm menus */
	Vmgm& GetVmgm() { return m_vmgm; }
	
	/** Returns count of audio streams */
	unsigned int GetAudioStreamCount();
	/** Returns count of subtitle streams */
	unsigned int GetSubtitleStreamsCount();
	
	/**
	 * Adds a new Titleset
	 * @return Index of new titleset (tsi)
	 **/
	int AddTitleset();
	/**
	 * Adds a new Menu
	 * @return Index of new PGC (pgci)
	 **/
	int AddMenu(int tsi = -1);
	/**
	 * Adds a new Menu
	 * @return Index of new PGC (pgci)
	 **/
	int AddMenu(Menu* menu, int tsi = -1);
	/**
	 * Adds a new PGC
	 * @return Index of new PGC (pgci)
	 **/
	int AddPgc(int tsi, bool menu, Pgc* pgc = NULL);
	/** Returns pgcs */
	PgcArray& GetPgcArray(int tsi, bool menus);
	/** Returns PGC from given titleset */
	Pgc* GetPgc(int tsi, bool isMenu, int pgci);
	/** Returns vob from given titleset, pgc */
	Vob* GetVob(int tsi, bool isMenu, int pgci, int vobi);
	/** Returns vob with menu from given titleset and pgc */
	Vob* GetMenuVob(int tsi, int pgci);
	/** Returns menu from titleset tsi und pgc pgci */	
	Menu* GetMenu(int tsi, int pgci);
	/** Returns true if at least one menu exist  */
	bool HasMenus();
	/** Returns true if all menus are ok */
	bool CheckMenus();
	/** Returns true if all actions are valid */
	bool CheckActions(bool skipInvalidTarget);
	/** Updates menus locations. Call it after moving of menus. */
	void UpdateMenusLocations();
	/** Updates image in buttons with given jump action */
	bool UpdateButtonImageFor(int actionTsi, int actionPgci);
	/** Fix coordinates of buttons if they are out of range */
	void FixButtonCoordinates();
	/** Returns true if button with given jump action exits */
	bool HasButtonWithJumpAction(int tsi, int pgci, bool isMenu = false);
	
	/** returns id for specified vob */
	static inline int MakeId(int tsi, int pgci, int vobi, bool menu = false)
	{ return ((tsi+1)<<24) + ((pgci*2+(menu ? 0 : 1))<<8) + vobi; }
	static inline int GetTsi(int id)  { return (id>>24)-1; }
	static inline int GetPgci(int id) { return ((id>>8) & 0xFFFF) / 2; }
	static inline bool IsMenu(int id) { return !(((id>>8) & 0xFFFF) % 2); }
	static inline int GetVobi(int id) { return id & 0xFF; }
	
	/** Returns estimated size of DVD in KB */
	long GetSize(bool afterTranscoding = false);
	/** Returns size required for generation of DVD in KB */
	long GetRequiredSize(Cache* cache);
	
	/** Returns number of g-register to implement "play all" or -1 if there is no "play all"-button */
	inline int GetPlayAllRegister() { return m_playAllRegister; }
	/** Returns number of g-register to implement "remember last selected button" */
	inline int GetRememberLastButtonRegister() { return m_rememberLastButtonRegister; }
	/** Returns number of g-register to implement "call last menu" command */
	inline int GetLastMenuRegister() { return m_lastMenuRegister; }
	/** Returns true if at least one title use "call last menu" command */
	bool HasCallLastMenu();
	/** Returns true if at least one button has checked flag "Play all titlesets" */
	bool HasPlayAllTitlesetButton();

	/** Returns video format of DVD */
	inline VideoFormat GetVideoFormat() { return m_videoFormat; }
	/** Sets video format of DVD */
	void SetVideoFormat(VideoFormat format);
	/** Returns audio format of DVD */
	inline AudioFormat GetAudioFormat() { return m_audioFormat; }
	/** Sets audio format of DVD */
	void SetAudioFormat(AudioFormat format);
	/** Returns aspect ratio of DVD */
	inline AspectRatio GetAspectRatio() { return m_aspectRatio; }
	/** Sets aspect ratio of DVD */
	void SetAspectRatio(AspectRatio format);
    /** Returns default title post command */
    DefaultPostCommand GetDefPostCommand() { return m_defPostCommand; }
    /** Returns default title post command as string */
    wxString GetDefPostCommandStr();
    /** Sets default title post command */
    void SetDefPostCommand(DefaultPostCommand defPostCommand, bool updateTitles = false);
	
    /** Loads a project file */
	bool Open(wxString fname, wxProgressDialog* progressDlg = NULL);
    /** Saves a project file */
	bool Save(wxString fname = wxEmptyString);
    /** Stores object data to string */
	wxString Serialize();
    /** Restores object from data */
	void Deserialize(const wxString& data);
    /** Returns project file name with path */
    wxString GetFilename() { return m_filename; }
    /** Returns path of loaded project file */
    wxString GetPath(bool withSep = true);
    /** Returns template file name with path */
    wxString GetTemplateFile() { return m_templateFile; }
    /** Returns template for title selection menu */
    wxSvgXmlNode* GetTitleMenuTemplate() { return titleMenuTemplate; }
    /** Returns if project uses template */
    bool HasTemplate() { return m_templateFile.length() > 0; }
    /** Returns output directory */
    wxString GetOutputDir() { return m_outputDir; }
    /** Sets output directory */
    void SetOutputDir(const wxString& outputDir) { m_outputDir = outputDir; }
    /** Returns ISO file name with path */
    wxString GetIsoFile() { return m_isoFile; }
    /** Sets ISO file name with path */
    void SetIsoFile(const wxString& isoFile) { m_isoFile = isoFile; }
	
    /** Saves a configuration for dvdauthor */
	bool SaveDVDAuthor(wxString fname);
	
	void RenderThumbnail(wxString fname);
	
	static wxArrayString GetVideoFormatLabels(bool copy = false, bool none = false, bool menu = false, bool hd = false);
	static wxArrayString GetAudioFormatLabels(bool copy = false, bool none = false, bool pcm = true);
	static wxArrayString GetSubtitleFormatLabels(bool copy = false, bool none = false);
	static wxArrayString GetAspectRatioLabels(bool autom = false);
	static wxArrayString GetDvdResolutionLabels();
	static wxArrayString GetCapacityLabels();
	static wxArrayString GetDefPostCommandLabels();
	static wxArrayString GetVideoBitrateLabels();
	static wxArrayString GetVideoFormatNames();
	static wxArrayString GetAudioFormatNames();
	static VideoFormat GetVideoFormatByName(wxString name);
	static AudioFormat GetAudioFormatByName(wxString name);
	static wxString GetVideoFormatName(VideoFormat format);
	static wxString GetAudioFormatName(AudioFormat format);
	/** Returns list of available language codes for audio tracks (ISO 639) */
	static wxArrayString GetAudioLanguageCodes();
	/** Returns map of languages to language codes (ISO 639) */
	static map<wxString, wxString>& GetLanguageMap();
	/** Returns list of languages (ISO 639) */
	static wxArrayString& GetLanguageNames();
	/** Returns list of encodings supported by libiconv */
	static wxArrayString& GetCharsets();
	
private:
	/** project file name */
	wxString m_filename;
	/** template file name */
	wxString m_templateFile;
	/** Output sub-directory */
	wxString m_outputDir;
	/** ISO file name */
	wxString m_isoFile;
	/** disc label */
    wxString m_label;
    /** DVD resolution (SD / Half HD / Full HD) */
    DvdResolution m_dvdResolution;
    /** dics capacity */
    DiscCapacity m_capacity;
    /** Default post command (last menu / root menu / next title) */
    DefaultPostCommand m_defPostCommand;
    bool m_videoBitrateAuto;
    int m_videoBitrate;
    int m_audioBitrate;
	bool m_jumppad;
	bool m_emptyMenu;
	TitlesetArray m_titlesets;
	Vmgm m_vmgm;
	int m_playAllRegister;
	int m_rememberLastButtonRegister;
	int m_lastMenuRegister;
	VideoFormat m_videoFormat;
	AudioFormat m_audioFormat;
	AspectRatio m_aspectRatio;
	wxSvgXmlNode* titleMenuTemplate;
	/** Returns true if at least one button has checked flag "Play all" */
	bool HasPlayAllButton();
	/** Returns true if at least one menu has checked flag "Remember last selected button" */
	bool HasRememberLastButton(bool checkIfNeedRegister = false);
	/** Returns number of g-regsiter that is not used */
	int FindFreeRegister();
	/** Returns true if given g-register is used in pgc pre/post or in menu button action */
	bool IsRegisterUsed(int g);
    /** Returns XML Document */
	wxSvgXmlDocument* GetXml();
	/** Initializes object with XML data */
	bool PutXML(const wxSvgXmlDocument& xml, wxProgressDialog* progressDlg);
};

#endif // DVD_H
