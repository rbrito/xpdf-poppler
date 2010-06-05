LIBS=$(shell pkg-config --libs poppler) -lXm
INCLUDES=$(strip $(shell pkg-config --cflags poppler))

CPPFLAGS+= $(INCLUDES) -DHAVE_DIRENT_H -Os

xpdf: CoreOutputDev.o GlobalParams.o PDFCore.o XPDFApp.o XPDFCore.o XPDFTree.o XPDFViewer.o parseargs.o xpdf.o
	$(CXX) -o xpdf $(LIBS) *.o

clean:
	rm -f *.o xpdf

.PHONY: clean
