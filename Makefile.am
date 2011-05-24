EXTRA_DIST = autogen.sh icons doc

man_MANS = doc/xpdfrc.5 doc/xpdf.1

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
