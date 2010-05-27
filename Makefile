LIBS=$(shell pkg-config --libs poppler) -lXm
POPPLERBASE=$(strip $(shell pkg-config --cflags poppler))
INCLUDES=$(POPPLERBASE) $(POPPLERBASE)/goo $(POPPLERBASE)/splash

CPPFLAGS+= $(INCLUDES)

xpdf: CoreOutputDev.o PDFCore.o XPDFApp.o XPDFCore.o XPDFTree.o XPDFViewer.o parseargs.o xpdf.o
	$(CXX) -o xpdf $(LIBS) *.o

clean:
	rm -f *.o xpdf

.PHONY: clean
