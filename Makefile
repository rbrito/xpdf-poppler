all: xpdf

OBJS=CoreOutputDev.o GlobalParams.o PDFCore.o XPDFApp.o XPDFCore.o XPDFTree.o XPDFViewer.o parseargs.o xpdf.o

CC=$(CXX)

CPPFLAGS += $(strip $(shell pkg-config --cflags poppler))
CPPFLAGS += -DHAVE_DIRENT_H

CPPFLAGS += -Wall -Wno-sign-compare

CXXFLAGS += -Os
CXXFLAGS += -Wno-write-strings

CFLAGS += $(CXXFLAGS)

LOADLIBES += $(shell pkg-config --libs poppler)
LOADLIBES += -lXm

LDFLAGS += -Os

xpdf: $(OBJS)

clean:
	rm -f $(OBJS) xpdf

.PHONY: clean all
