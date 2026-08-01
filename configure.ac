dnl Process this file with autoconf to produce a configure script.

dnl initialization
AC_INIT([DVDStyler],[3.3b7],[http://www.dvdstyler.org])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

dnl options
AM_OPTIONS_WXCONFIG
AC_ARG_ENABLE(debug, [  --enable-debug          Enable debugging information], USE_DEBUG="$enableval", USE_DEBUG="no")

AC_MSG_CHECKING(for install location)
case "$prefix" in
NONE)	AC_CACHE_VAL(m_cv_prefix,m_cv_prefix=$ac_default_prefix);;
*)	m_cv_prefix=$prefix ;;
esac
AC_MSG_RESULT($m_cv_prefix)
case "$m_cv_prefix" in /*) ;; *)
	AC_MSG_WARN([--prefix=$prefix must be an absolute path name, using $ac_default_prefix])
	m_cv_prefix=$ac_default_prefix
esac
prefix=$m_cv_prefix


dnl Checks for programs.
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_OBJCXX
AC_PROG_LEX(noyywrap)
if test "x$LEX" = "x:"; then
  AC_MSG_ERROR([
    DVDStyler requires lex (flex).
    Please check that flex is in path.
])
fi
AC_PROG_YACC
if test "x$YACC" = "x:"; then
  AC_MSG_ERROR([
    DVDStyler requires yacc (bison).
    Please check that bison is in path.
])
fi
PKG_PROG_PKG_CONFIG()

dnl debug flags

case "$USE_DEBUG" in
yes)
  DEBUG_FLAGS="-g -Wall -Wno-unused-local-typedefs -DDEBUG"
  ;;    
*)
  DEBUG_FLAGS="-DNDEBUG"
  ;;
esac
CXXFLAGS="$CXXFLAGS $DEBUG_FLAGS -fno-strict-aliasing "

dnl check wxWidgets >= 3.0
WX_CONFIG_CHECK([3.0.0], [WXFOUND=1], [WXFOUND=0], [html,core,net,adv,media])
if test "$WXFOUND" != 1; then
    AC_MSG_ERROR([
        Please check that wx-config is in path, the directory
        where wxWidgets libraries are installed (returned by
        'wx-config --libs' command) is in LD_LIBRARY_PATH or
        equivalent variable and wxWidgets is version 3.0 or above.
    ])
fi
CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS "
CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS "
LIBS="$LIBS $WX_LIBS "

AC_MSG_CHECKING([for wxWidgets media])
WX_OK=no
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <wx/mediactrl.h>]], [[wxMediaCtrl media;]])],[WX_OK=yes],[WX_OK=no])
AC_MSG_RESULT([$WX_OK])
if test "x$WX_OK" = x"no"; then
  AC_MSG_ERROR([
    wxWidgets media library is missing. Install the wxGTK media development
    package (e.g. libwxgtk-media3.0-gtk3-dev on Ubuntu).
])
fi

dnl check libav/ffmpeg
PKG_CHECK_MODULES(LIBAV, [libavformat >= 53.20.0 libavcodec >= 53.34.0 libavutil libswscale libavfilter >= 2.15.0])
CXXFLAGS="$CXXFLAGS $LIBAV_CFLAGS"
LIBS="$LIBS $LIBAV_LIBS"
AC_CHECK_HEADER([libavformat/avformat.h],[],[AC_MSG_ERROR([*** missing ffmpeg library])],[
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <stdint.h>])

dnl check for wxSVG
PKG_CHECK_MODULES(wxSVG, [libwxsvg >= 1.5.23], [LIBS="$LIBS $wxSVG_LIBS"], [AC_MSG_ERROR([*** missing wxSVG library >= 1.5.23 ***])])
AC_MSG_CHECKING([for wxSVG library with ffmpeg/libav support])
WXSVGLIBAV=no
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <wxSVG/mediadec_ffmpeg.h>]], [[wxFfmpegMediaDecoder *a = new wxFfmpegMediaDecoder;]])],[WXSVGLIBAV=yes],[WXSVGLIBAV=no])
AC_MSG_RESULT([$WXSVGLIBAV])
if test "x$WXSVGLIBAV" = x"no"; then
  AC_MSG_ERROR([
    DVDStyler requires the wxSVG library with ffmpeg/libav support.
])
fi

dnl check gnomeui-2.0
CHECK_WX_BUILT_WITH_GTK()
if test x"$GTK_USEDVERSION" = x"2" ; then
  PKG_CHECK_MODULES(GNOMEUI2, libgnomeui-2.0, GNOME2="-DGNOME2",
    AC_MSG_WARN([libgnomeui will not be used for rendering of thumbnails]))
  CXXFLAGS="$CXXFLAGS $GNOME2"
  LIBS="$LIBS $GNOMEUI2_LIBS "
elif test x"$GTK_USEDVERSION" = x"3" ; then
  PKG_CHECK_MODULES(GNOME_DESKTOP_3, gnome-desktop-3, GNOME3="-DGNOME3",
    AC_MSG_WARN([gnome-desktop will not be used for rendering of thumbnails]))
  CXXFLAGS="$CXXFLAGS $GNOME3 $GNOME_DESKTOP_3_CFLAGS"
  LIBS="$LIBS $GNOME_DESKTOP_3_LIBS"
fi

dnl check libexif
PKG_CHECK_MODULES(LIBEXIF, libexif, [LIBS="$LIBS $LIBEXIF_LIBS"], [AC_MSG_ERROR([*** missing libexif ***])])

dnl fontconfig
PKG_CHECK_MODULES(FONTCONFIG, fontconfig)
CXXFLAGS="$CXXFLAGS $FONTCONFIG_CFLAGS"
LIBS="$LIBS $FONTCONFIG_LIBS"

PKG_CHECK_MODULES(LIBUDEV, libudev, [CXXFLAGS="$CXXFLAGS $LIBUDEV_CFLAGS -DHAVE_LIBUDEV";LIBS="$LIBS $LIBUDEV_LIBS"],
  AC_MSG_WARN([libudev will not be used to get list of devices]))

PKG_CHECK_MODULES(LIBDBUS, dbus-1, [CXXFLAGS="$CXXFLAGS $LIBDBUS_CFLAGS -DHAVE_LIBDBUS";LIBS="$LIBS $LIBDBUS_LIBS"],
  AC_MSG_WARN([libdbus is missing - the option to turn off computer will not be available]))

CXXFLAGS="$CXXFLAGS -DDATADIR=\\\"$datadir/$PACKAGE\\\""

dnl vars
AC_SUBST(LDFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(WX_CONFIG_NAME)

dnl check avconv
AC_PATH_PROG(AVCONV_PATH, avconv, "")
if test "x$AVCONV_PATH" = x; then
	dnl check ffmpeg
	AC_PATH_PROG(FFMPEG_PATH, ffmpeg, "")
	if test "x$FFMPEG_PATH" = x; then
	  AC_MSG_ERROR([
	    DVDStyler requires libav or ffmpeg tools.
	    Please check that avconv or ffmpeg is in path.
	])
	fi
	CXXFLAGS="$CXXFLAGS -DUSE_FFMPEG=1"
fi

dnl check msgfmt
AC_PATH_PROG(MSGFMT_PATH, msgfmt, "")
if test "x$MSGFMT_PATH" = x; then
  AC_MSG_ERROR([
    DVDStyler requires gettext-tools.
    Please check that msgfmt is in path.
])
fi

dnl check xmlto
AC_PATH_PROG(XMLTO, xmlto, "")
if test "x$XMLTO" = x; then
  AC_MSG_ERROR([
    DVDStyler requires xmlto to build DocBook documentation.
    Please check that xmlto is in path.
])
fi

dnl check zip
AC_PATH_PROG(ZIP, zip, "")
if test "x$ZIP" = x; then
  AC_MSG_ERROR([
    DVDStyler requires zip to build html help file.
    Please check that zip is in path.
])
fi
ZIP="$ZIP -X"

dnl check dvdauthor
AC_PATH_PROG(DVDAUTHOR_PATH, dvdauthor, "")
if test "x$DVDAUTHOR_PATH" = x; then
  AC_MSG_ERROR([
    DVDStyler requires dvdauthor >= 0.7.0.
    Please check that dvdauthor is in path.
])
fi
AC_PATH_PROG(MKISOFS_PATH, mkisofs, "")
if test "x$MKISOFS_PATH" = x; then
  AC_MSG_ERROR([
    DVDStyler requires mkisofs (cdrtools).
    Please check that mkisofs is in path.
])
fi
case "${host_os}" in
	*darwin*)
		osx=true
		LDFLAGS="$LDFLAGS -framework DiscRecording"
		;;
	*)
		AC_PATH_PROG(GROWISOFS_PATH, growisofs, "")
		if test "x$GROWISOFS_PATH" = x; then
		  AC_MSG_ERROR([
		    DVDStyler requires growisofs (dvd+rw-tools).
		    Please check that growisofs is in path.
		])
		fi
		;;
esac
AM_CONDITIONAL([OSX], [test x$osx = xtrue])

dnl create Makefiles
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/rc/Makefile
  wxVillaLib/Makefile
  locale/Makefile
  backgrounds/Makefile
  buttons/Makefile
  docs/Makefile
  objects/Makefile
  data/Makefile
  templates/Makefile
  transitions/Makefile
  src/Version.h
  Info.plist
])
AC_OUTPUT
