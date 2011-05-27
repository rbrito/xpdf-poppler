bin_PROGRAMS = xpdf-poppler

xpdf_poppler_CPPFLAGS = $(PKG_CONFIG_CFLAGS)
xpdf_poppler_CPPFLAGS += -DHAVE_DIRENT_H
xpdf_poppler_CPPFLAGS += -DHAVE_X11_XPM_H
xpdf_poppler_CPPFLAGS += -DSYSTEM_XPDFRC=\"$(xpdfconfdir)/$(rcfile)\"

xpdf_poppler_CXXFLAGS = -Wall -Wno-write-strings

xpdf_poppler_SOURCES = CoreOutputDev.cc GlobalParamsGUI.cc PDFCore.cc	\
	XPDFApp.cc XPDFCore.cc XPDFTree.cc XPDFViewer.cc parseargs.cc	\
	xpdf.cc about-text.h config.h CoreOutputDev.h			\
	GlobalParamsGUI.h parseargs.h PDFCore.h XPDFApp.h XPDFCore.h	\
	XPDFTree.h XPDFTreeP.h XPDFViewer.h

bin_SCRIPTS = zxpdf-poppler

desktopdir = $(datadir)/applications
desktop_DATA = xpdf-poppler.desktop

rcfile = xpdf-poppler-rc

xpdfconfdir = $(sysconfdir)/xpdf-poppler
xpdfconf_DATA = $(rcfile)

doc_DATA = doc/sample-xpdf-poppler-rc

man_MANS = doc/xpdf-poppler-rc.5 doc/xpdf-poppler.1 doc/zxpdf-poppler.1

EXTRA_DIST = autogen.sh icons
