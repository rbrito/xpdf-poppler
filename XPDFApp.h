//========================================================================
//
// XPDFApp.h
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef XPDFAPP_H
#define XPDFAPP_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#define Object XtObject
#include <Xm/XmAll.h>
#undef Object
#include "gtypes.h"
#include "SplashTypes.h"

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
		     GBool fullScreenA);
  void close(XPDFViewer *viewer, GBool closeLast);
  void quit();

  void run();

  //----- remote server
  void setRemoteName(char *remoteName);
  GBool remoteServerRunning();
  void remoteExec(char *cmd);
  void remoteOpen(GooString *fileName, int page, GBool raise);
  void remoteOpenAtDest(GooString *fileName, GooString *dest, GBool raise);
  void remoteReload(GBool raise);
  void remoteRaise();
  void remoteQuit();

  //----- resource/option values
  GooString *getGeometry() { return geometry; }
  GooString *getTitle() { return title; }
  GBool getInstallCmap() { return installCmap; }
  int getRGBCubeSize() { return rgbCubeSize; }
  GBool getReverseVideo() { return reverseVideo; }
  SplashColorPtr getPaperRGB() { return paperRGB; }
  Gulong getPaperPixel() { return paperPixel; }
  Gulong getMattePixel(GBool fullScreenA)
    { return fullScreenA ? fullScreenMattePixel : mattePixel; }
  GooString *getInitialZoom() { return initialZoom; }
  void setFullScreen(GBool fullScreenA) { fullScreen = fullScreenA; }
  GBool getFullScreen() { return fullScreen; }

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
  GBool installCmap;
  int rgbCubeSize;
  GBool reverseVideo;
  SplashColor paperRGB;
  Gulong paperPixel;
  Gulong mattePixel;
  Gulong fullScreenMattePixel;
  GooString *initialZoom;
  GBool fullScreen;
};

#endif
