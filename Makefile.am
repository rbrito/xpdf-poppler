bin_PROGRAMS = xpdf

xpdf_CPPFLAGS = $(PKG_CONFIG_CFLAGS)
xpdf_CPPFLAGS += -DHAVE_DIRENT_H
xpdf_CPPFLAGS += -DHAVE_X11_XPM_H
xpdf_CPPFLAGS += -DSYSTEM_XPDFRC=\"$(sysconfdir)/xpdfrc\"

xpdf_CXXFLAGS = -Wall -Wno-write-strings

xpdf_SOURCES = CoreOutputDev.cc GlobalParamsGUI.cc PDFCore.cc		\
	XPDFApp.cc XPDFCore.cc XPDFTree.cc XPDFViewer.cc parseargs.cc	\
	xpdf.cc about-text.h config.h CoreOutputDev.h			\
	GlobalParamsGUI.h parseargs.h PDFCore.h XPDFApp.h XPDFCore.h	\
	XPDFTree.h XPDFTreeP.h XPDFViewer.h

bin_SCRIPTS = zxpdf

desktopdir = $(datadir)/applications
desktop_DATA = xpdf.desktop

xpdfconfdir = $(sysconfdir)/xpdf
xpdfconf_DATA = xpdfrc

doc_DATA = doc/sample-xpdfrc

man_MANS = doc/xpdfrc.5 doc/xpdf.1 doc/zxpdf.1

EXTRA_DIST = autogen.sh icons
