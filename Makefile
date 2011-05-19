all: xpdf

OBJS=CoreOutputDev.o GlobalParamsGUI.o PDFCore.o XPDFApp.o XPDFCore.o XPDFTree.o XPDFViewer.o parseargs.o xpdf.o

CPPFLAGS += $(strip $(shell pkg-config --cflags poppler))
CPPFLAGS += -DHAVE_DIRENT_H
CPPFLAGS += -DHAVE_X11_XPM_H

CPPFLAGS += -Wall
CPPFLAGS += -Wno-write-strings

CXXFLAGS += -Os

LOADLIBES += $(strip $(shell pkg-config --libs poppler))
LOADLIBES += -lXm
LOADLIBES += -lXpm
LOADLIBES += -lXt
LOADLIBES += -lX11
LOADLIBES += -lfontconfig

CC=$(CXX)

xpdf: $(OBJS)

clean:
	rm -f $(OBJS) xpdf

.PHONY: clean all
