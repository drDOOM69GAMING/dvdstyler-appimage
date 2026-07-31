bin_PROGRAMS = dvdstyler
dvdstyler_SOURCES = Config.cpp Languages.cpp Utils.cpp SysUtils.cpp Cache.cpp\
 mediaenc.cpp mediaenc_ffmpeg.cpp mediatrc_ffmpeg.cpp mediactrl_ffmpeg.cpp Palette3D.cpp\
 DVDAction.cpp TextSub.cpp Stream.cpp Cell.cpp Vob.cpp Pgc.cpp SubStream.cpp Titleset.cpp DVD.cpp dvdvml.c dvdvmy.c\
 MenuObject.cpp MenuPalettes.cpp Menu.cpp Slideshow.cpp MPEG.cpp About.cpp AnimationDlg.cpp\
 MessageDlg.cpp SettingsDlg.cpp NewProjectDlg.cpp WelcomeDlg.cpp DVDPropDlg.cpp TemplateDlg.cpp\
 VideoFrameDlg.cpp MenuActionsDlg.cpp MenuCellsDlg.cpp MenuPropDlg.cpp MenuObjectPropDlg.cpp MenuEditor.cpp\
 VobListBox.cpp AudioPropDlg.cpp VideoPropDlg.cpp SubtitlePropDlg.cpp ChaptersDlg.cpp SlideDlg.cpp TitlePropDlg.cpp\
 TitlesetManager.cpp BurnDlg.cpp ProgressDlg.cpp MenuObjectDef.cpp ButtonEditDlg.cpp ButtonAddParamDlg.cpp\
 DirCtrl.cpp StatusBar.cpp MainWin.cpp dvdstyler.cpp\
 Process.cpp ProcessTranscode.cpp ProcessCleanTemp.cpp ProcessProjectInfo.cpp ProcessMenu.cpp ProcessEncode.cpp\
 ProcessMenuTransitions.cpp ProcessSlideshow.cpp ProcessSubtitles.cpp ProcessDvdFilesystem.cpp ProcessPreview.cpp\
 ProcessAskOutput.cpp ProcessIsoImage.cpp ProcessEccData.cpp ProcessFormatDvd.cpp ProcessBurn.cpp 

if OSX
dvdstyler_SOURCES += BurnDlgOSX.mm
endif

AM_CXXFLAGS = @CXXFLAGS@ -I..
LDADD = ../wxVillaLib/libwxvilla.a -ljpeg

EXTRA_DIST = dvdvml.l dvdvmy.y
MAINTAINERCLEANFILES = dvdvml.c dvdvmy.c dvdvmy.h

dvdvml.c: dvdvml.l
	$(LEX) -s -B -Cem -o$@ -Pdvdvm $<

dvdvmy.c dvdvmy.h: dvdvmy.y
	$(YACC) -o dvdvmy.c -d -p dvdvm $<

dvdvml.o: dvdvmy.h

SUBDIRS = rc

run: $(bin_PROGRAMS)
	./$(bin_PROGRAMS)
