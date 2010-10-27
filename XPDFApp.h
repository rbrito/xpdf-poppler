//========================================================================
//
// XPDFApp.h
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef XPDFAPP_H
#define XPDFAPP_H

#include <poppler-config.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#define Object XtObject
#include <Xm/XmAll.h>
#undef Object
#include "poppler/splash/SplashTypes.h"

class GooString;
class GooList;
class PDFDoc;
class XPDFViewer;

//------------------------------------------------------------------------

#define xpdfAppName "Xpdf"

//------------------------------------------------------------------------
// XPDFApp
//------------------------------------------------------------------------

class XPDFApp {
public:

  XPDFApp(int *argc, char *argv[]);
  ~XPDFApp();

  XPDFViewer *open(GooString *fileName, int page = 1,
		   GooString *ownerPassword = NULL,
		   GooString *userPassword = NULL);
  XPDFViewer *openAtDest(GooString *fileName, GooString *dest,
			 GooString *ownerPassword = NULL,
			 GooString *userPassword = NULL);
  XPDFViewer *reopen(XPDFViewer *viewer, PDFDoc *doc, int page,
		     bool fullScreenA);
  void close(XPDFViewer *viewer, bool closeLast);
  void quit();

  void run();

  //----- remote server
  void setRemoteName(char *remoteName);
  bool remoteServerRunning();
  void remoteExec(char *cmd);
  void remoteOpen(GooString *fileName, int page, bool raise);
  void remoteOpenAtDest(GooString *fileName, GooString *dest, bool raise);
  void remoteReload(bool raise);
  void remoteRaise();
  void remoteQuit();

  //----- resource/option values
  GooString *getGeometry() { return geometry; }
  GooString *getTitle() { return title; }
  bool getInstallCmap() { return installCmap; }
  int getRGBCubeSize() { return rgbCubeSize; }
  bool getReverseVideo() { return reverseVideo; }
  SplashColorPtr getPaperRGB() { return paperRGB; }
  unsigned long getPaperPixel() { return paperPixel; }
  unsigned long getMattePixel(bool fullScreenA)
    { return fullScreenA ? fullScreenMattePixel : mattePixel; }
  GooString *getInitialZoom() { return initialZoom; }
  void setFullScreen(bool fullScreenA) { fullScreen = fullScreenA; }
  bool getFullScreen() { return fullScreen; }

  XtAppContext getAppContext() { return appContext; }
  Widget getAppShell() { return appShell; }

private:

  void getResources();
  static void remoteMsgCbk(Widget widget, XtPointer ptr,
			   XEvent *event, Boolean *cont);

  Display *display;
  int screenNum;
  XtAppContext appContext;
  Widget appShell;
  GooList *viewers;		// [XPDFViewer]

  Atom remoteAtom;
  Window remoteXWin;
  XPDFViewer *remoteViewer;
  Widget remoteWin;

  //----- resource/option values
  GooString *geometry;
  GooString *title;
  bool installCmap;
  int rgbCubeSize;
  bool reverseVideo;
  SplashColor paperRGB;
  unsigned long paperPixel;
  unsigned long mattePixel;
  unsigned long fullScreenMattePixel;
  GooString *initialZoom;
  bool fullScreen;
};

#endif
