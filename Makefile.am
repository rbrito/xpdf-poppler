INCLUDES = -I@top_srcdir@ @POPPLER_CFLAGS@

EXTRA_DIST = \
	autogen.sh \
	icons \
	doc

man_MANS = \
	doc/xpdfrc.5 \
	doc/xpdf.1

bin_PROGRAMS = xpdf
xpdf_LDADD = \
	$(X11_LIBS) \
	$(POPPLER_LIBS) \
	$(FONTCONFIG_LIBS) \
	$(MOTIF_LIBS)
xpdf_CPPFLAGS = \
	$(X11_CFLAGS) \
	$(POPPLER_CFLAGS) \
	$(FONTCONFIG_CFLAGS) \
	-DSYSTEM_XPDFRC=\"$(sysconfdir)/xpdfrc\"
xpdf_SOURCES = \
	CoreOutputDev.cc \
	GlobalParamsGUI.cc \
	PDFCore.cc \
	XPDFApp.cc \
	XPDFCore.cc \
	XPDFTree.cc \
	XPDFViewer.cc \
	parseargs.cc \
	xpdf.cc \
	about-text.h \
	config.h \
	CoreOutputDev.h \
	GlobalParamsGUI.h \
	parseargs.h \
	PDFCore.h \
	XPDFApp.h \
	XPDFCore.h \
	XPDFTree.h \
	XPDFTreeP.h \
	XPDFViewer.h
