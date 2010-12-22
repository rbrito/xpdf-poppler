INCLUDES = -I@top_srcdir@ @POPPLER_CFLAGS@

EXTRA_DIST = \
	autogen.sh \
	icons \
	doc

noinst_HEADERS = \
	about-text.h \
	config.h \
	CoreOutputDev.h \
	GlobalParams.h \
	parseargs.h \
	PDFCore.h \
	XPDFApp.h \
	XPDFCore.h \
	XPDFTree.h \
	XPDFTreeP.h \
	XPDFViewer.h

man_MANS = \
	doc/xpdfrc.5 \
	doc/xpdf.1

bin_PROGRAMS = xpdf
xpdf_LDADD = \
	@X11_LIBS@ \
	@POPPLER_LIBS@ \
	@FONTCONFIG_LIBS@ \
	@MOTIF_LIBS@
xpdf_SOURCES = \
	CoreOutputDev.cc \
	GlobalParams.cc \
	PDFCore.cc \
	XPDFApp.cc \
	XPDFCore.cc \
	XPDFTree.cc \
	XPDFViewer.cc \
	parseargs.cc \
	xpdf.cc
