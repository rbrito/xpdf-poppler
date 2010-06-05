//========================================================================
//
// XPDFViewer.h
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef XPDFVIEWER_H
#define XPDFVIEWER_H

#include <poppler-config.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#define Object XtObject
#include <Xm/XmAll.h>
#undef Object
#include "poppler/goo/gtypes.h"
#include "XPDFCore.h"

#if (XmVERSION <= 1) && !defined(__sgi)
#define DISABLE_OUTLINE
#endif

#if (XmVERSION >= 2 && !defined(LESSTIF_VERSION))
#  define USE_COMBO_BOX 1
#else
#  undef USE_COMBO_BOX
#endif

class GooString;
class GooList;
class UnicodeMap;
class LinkDest;
class XPDFApp;
class XPDFViewer;

//------------------------------------------------------------------------

struct XPDFViewerCmd {
  char *name;
  int nArgs;
  GBool requiresDoc;
  GBool requiresEvent;
  void (XPDFViewer::*func)(GooString *args[], int nArgs, XEvent *event);
};

//------------------------------------------------------------------------

//------------------------------------------------------------------------
// XPDFViewer
//------------------------------------------------------------------------

class XPDFViewer {
public:

  XPDFViewer(XPDFApp *appA, GooString *fileName,
	     int pageA, GooString *destName, GBool fullScreen,
	     GooString *ownerPassword, GooString *userPassword);
  XPDFViewer(XPDFApp *appA, PDFDoc *doc, int pageA,
	     GooString *destName, GBool fullScreen);
  GBool isOk() { return ok; }
  ~XPDFViewer();

  void open(GooString *fileName, int pageA, GooString *destName);
  void clear();
  void reloadFile();

  void execCmd(GooString *cmd, XEvent *event);

  Widget getWindow() { return win; }

private:

  //----- load / display
  GBool loadFile(GooString *fileName, GooString *ownerPassword = NULL,
		 GooString *userPassword = NULL);
  void displayPage(int pageA, double zoomA, int rotateA,
                   GBool scrollToTop, GBool addToHist);
  void displayDest(LinkDest *dest, double zoomA, int rotateA,
		   GBool addToHist);
  void getPageAndDest(int pageA, GooString *destName,
		      int *pageOut, LinkDest **destOut);

  //----- hyperlinks / actions
  void doLink(int wx, int wy, GBool onlyIfNoSelection, GBool newWin);
  static void actionCbk(void *data, char *action);

  //----- keyboard/mouse input
  static void keyPressCbk(void *data, KeySym key, Guint modifiers,
			  XEvent *event);
  static void mouseCbk(void *data, XEvent *event);
  int getModifiers(Guint modifiers);
  int getContext(Guint modifiers);

  //----- command functions
  void cmdAbout(GooString *args[], int nArgs, XEvent *event);
  void cmdCloseOutline(GooString *args[], int nArgs, XEvent *event);
  void cmdCloseWindow(GooString *args[], int nArgs, XEvent *event);
  void cmdContinuousMode(GooString *args[], int nArgs, XEvent *event);
  void cmdEndPan(GooString *args[], int nArgs, XEvent *event);
  void cmdEndSelection(GooString *args[], int nArgs, XEvent *event);
  void cmdFind(GooString *args[], int nArgs, XEvent *event);
  void cmdFindNext(GooString *args[], int nArgs, XEvent *event);
  void cmdFocusToDocWin(GooString *args[], int nArgs, XEvent *event);
  void cmdFocusToPageNum(GooString *args[], int nArgs, XEvent *event);
  void cmdFollowLink(GooString *args[], int nArgs, XEvent *event);
  void cmdFollowLinkInNewWin(GooString *args[], int nArgs, XEvent *event);
  void cmdFollowLinkInNewWinNoSel(GooString *args[], int nArgs, XEvent *event);
  void cmdFollowLinkNoSel(GooString *args[], int nArgs, XEvent *event);
  void cmdFullScreenMode(GooString *args[], int nArgs, XEvent *event);
  void cmdGoBackward(GooString *args[], int nArgs, XEvent *event);
  void cmdGoForward(GooString *args[], int nArgs, XEvent *event);
  void cmdGotoDest(GooString *args[], int nArgs, XEvent *event);
  void cmdGotoLastPage(GooString *args[], int nArgs, XEvent *event);
  void cmdGotoLastPageNoScroll(GooString *args[], int nArgs, XEvent *event);
  void cmdGotoPage(GooString *args[], int nArgs, XEvent *event);
  void cmdGotoPageNoScroll(GooString *args[], int nArgs, XEvent *event);
  void cmdNextPage(GooString *args[], int nArgs, XEvent *event);
  void cmdNextPageNoScroll(GooString *args[], int nArgs, XEvent *event);
  void cmdOpen(GooString *args[], int nArgs, XEvent *event);
  void cmdOpenFile(GooString *args[], int nArgs, XEvent *event);
  void cmdOpenFileAtDest(GooString *args[], int nArgs, XEvent *event);
  void cmdOpenFileAtDestInNewWin(GooString *args[], int nArgs, XEvent *event);
  void cmdOpenFileAtPage(GooString *args[], int nArgs, XEvent *event);
  void cmdOpenFileAtPageInNewWin(GooString *args[], int nArgs, XEvent *event);
  void cmdOpenFileInNewWin(GooString *args[], int nArgs, XEvent *event);
  void cmdOpenInNewWin(GooString *args[], int nArgs, XEvent *event);
  void cmdOpenOutline(GooString *args[], int nArgs, XEvent *event);
  void cmdPageDown(GooString *args[], int nArgs, XEvent *event);
  void cmdPageUp(GooString *args[], int nArgs, XEvent *event);
  void cmdPostPopupMenu(GooString *args[], int nArgs, XEvent *event);
  void cmdPrevPage(GooString *args[], int nArgs, XEvent *event);
  void cmdPrevPageNoScroll(GooString *args[], int nArgs, XEvent *event);
  void cmdPrint(GooString *args[], int nArgs, XEvent *event);
  void cmdQuit(GooString *args[], int nArgs, XEvent *event);
  void cmdRaise(GooString *args[], int nArgs, XEvent *event);
  void cmdRedraw(GooString *args[], int nArgs, XEvent *event);
  void cmdReload(GooString *args[], int nArgs, XEvent *event);
  void cmdRun(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollDown(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollDownNextPage(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollLeft(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollOutlineDown(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollOutlineUp(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollRight(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollToBottomEdge(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollToBottomRight(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollToLeftEdge(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollToRightEdge(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollToTopEdge(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollToTopLeft(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollUp(GooString *args[], int nArgs, XEvent *event);
  void cmdScrollUpPrevPage(GooString *args[], int nArgs, XEvent *event);
  void cmdSinglePageMode(GooString *args[], int nArgs, XEvent *event);
  void cmdStartPan(GooString *args[], int nArgs, XEvent *event);
  void cmdStartSelection(GooString *args[], int nArgs, XEvent *event);
  void cmdToggleContinuousMode(GooString *args[], int nArgs, XEvent *event);
  void cmdToggleFullScreenMode(GooString *args[], int nArgs, XEvent *event);
  void cmdToggleOutline(GooString *args[], int nArgs, XEvent *event);
  void cmdWindowMode(GooString *args[], int nArgs, XEvent *event);
  void cmdZoomFitPage(GooString *args[], int nArgs, XEvent *event);
  void cmdZoomFitWidth(GooString *args[], int nArgs, XEvent *event);
  void cmdZoomIn(GooString *args[], int nArgs, XEvent *event);
  void cmdZoomOut(GooString *args[], int nArgs, XEvent *event);
  void cmdZoomPercent(GooString *args[], int nArgs, XEvent *event);
  void cmdZoomToSelection(GooString *args[], int nArgs, XEvent *event);

  //----- GUI code: main window
  void initWindow(GBool fullScreen);
  void initToolbar(Widget parent);
#ifndef DISABLE_OUTLINE
  void initPanedWin(Widget parent);
#endif
  void initCore(Widget parent, GBool fullScreen);
  void initPopupMenu();
  void addToolTip(Widget widget, char *text);
  void mapWindow();
  void closeWindow();
  int getZoomIdx();
  void setZoomIdx(int idx);
  void setZoomVal(double z);
  static void prevPageCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
  static void prevTenPageCbk(Widget widget, XtPointer ptr,
			     XtPointer callData);
  static void nextPageCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
  static void nextTenPageCbk(Widget widget, XtPointer ptr,
			     XtPointer callData);
  static void backCbk(Widget widget, XtPointer ptr,
		      XtPointer callData);
  static void forwardCbk(Widget widget, XtPointer ptr,
			 XtPointer callData);
#if USE_COMBO_BOX
  static void zoomComboBoxCbk(Widget widget, XtPointer ptr,
			      XtPointer callData);
#else
  static void zoomMenuCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
#endif
  static void findCbk(Widget widget, XtPointer ptr,
		      XtPointer callData);
  static void printCbk(Widget widget, XtPointer ptr,
		       XtPointer callData);
  static void aboutCbk(Widget widget, XtPointer ptr,
		       XtPointer callData);
  static void quitCbk(Widget widget, XtPointer ptr,
		      XtPointer callData);
  static void openCbk(Widget widget, XtPointer ptr,
		      XtPointer callData);
  static void openInNewWindowCbk(Widget widget, XtPointer ptr,
				 XtPointer callData);
  static void reloadCbk(Widget widget, XtPointer ptr,
			XtPointer callData);
  static void saveAsCbk(Widget widget, XtPointer ptr,
			XtPointer callData);
  static void continuousModeToggleCbk(Widget widget, XtPointer ptr,
				      XtPointer callData);
  static void fullScreenToggleCbk(Widget widget, XtPointer ptr,
				  XtPointer callData);
  static void rotateCCWCbk(Widget widget, XtPointer ptr,
			   XtPointer callData);
  static void rotateCWCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
  static void zoomToSelectionCbk(Widget widget, XtPointer ptr,
				 XtPointer callData);
  static void closeCbk(Widget widget, XtPointer ptr,
		       XtPointer callData);
  static void closeMsgCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
  static void pageNumCbk(Widget widget, XtPointer ptr,
			 XtPointer callData);
  static void updateCbk(void *data, GooString *fileName,
			int pageNum, int numPages, char *linkString);

  //----- GUI code: outline
#ifndef DISABLE_OUTLINE
  void setupOutline();
  void setupOutlineItems(GooList *items, Widget parent, UnicodeMap *uMap);
  static void outlineSelectCbk(Widget widget, XtPointer ptr,
			       XtPointer callData);
#endif

  //----- GUI code: "about" dialog
  void initAboutDialog();

  //----- GUI code: "open" dialog
  void initOpenDialog();
  void mapOpenDialog(GBool openInNewWindowA);
  static void openOkCbk(Widget widget, XtPointer ptr,
			XtPointer callData);

  //----- GUI code: "find" dialog
  void initFindDialog();
  static void findFindCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);
  void mapFindDialog();
  void doFind(GBool next);
  static void findCloseCbk(Widget widget, XtPointer ptr,
			   XtPointer callData);

  //----- GUI code: "save as" dialog
  void initSaveAsDialog();
  void mapSaveAsDialog();
  static void saveAsOkCbk(Widget widget, XtPointer ptr,
			  XtPointer callData);

  //----- GUI code: "print" dialog
  void initPrintDialog();
  void setupPrintDialog();
  static void printWithCmdBtnCbk(Widget widget, XtPointer ptr,
				 XtPointer callData);
  static void printToFileBtnCbk(Widget widget, XtPointer ptr,
				XtPointer callData);

  static void printAllPagesBtnCbk(Widget widget, XtPointer ptr,
				 XtPointer callData);
  static void printEvenPagesBtnCbk(Widget widget, XtPointer ptr,
				 XtPointer callData);
  static void printOddPagesBtnCbk(Widget widget, XtPointer ptr,
				 XtPointer callData);
  static void printBackOrderBtnCbk(Widget widget, XtPointer ptr,
				 XtPointer callData);

  static void printPrintCbk(Widget widget, XtPointer ptr,
			    XtPointer callData);

  //----- Motif support
  XmFontList createFontList(char *xlfd);

  static XPDFViewerCmd cmdTab[];

  XPDFApp *app;
  GBool ok;

  Display *display;
  int screenNum;
  Widget win;			// top-level window
  Widget form;
  Widget panedWin;
#ifndef DISABLE_OUTLINE
  Widget outlineScroll;
  Widget outlineTree;
  Widget *outlineLabels;
  int outlineLabelsLength;
  int outlineLabelsSize;
  Dimension outlinePaneWidth;
#endif
  XPDFCore *core;
  Widget toolBar;
  Widget backBtn;
  Widget prevTenPageBtn;
  Widget prevPageBtn;
  Widget nextPageBtn;
  Widget nextTenPageBtn;
  Widget forwardBtn;
  Widget pageNumText;
  Widget pageCountLabel;
#if USE_COMBO_BOX
  Widget zoomComboBox;
#else
  Widget zoomMenu;
  Widget *zoomMenuBtns;
#endif
  Widget zoomWidget;
  Widget findBtn;
  Widget printBtn;
  Widget aboutBtn;
  Widget linkLabel;
  Widget quitBtn;
  Widget popupMenu;

  Widget aboutDialog;
  XmFontList aboutBigFont, aboutVersionFont, aboutFixedFont;

  Widget openDialog;
  GBool openInNewWindow;

  Widget findDialog;
  Widget findText;
  Widget findBackwardToggle;
  Widget findCaseSensitiveToggle;

  Widget saveAsDialog;

  Widget printDialog;
  Widget printWithCmdBtn;
  Widget printToFileBtn;
  Widget printCmdText;
  Widget printFileText;
  Widget printFirstPage;
  Widget printLastPage;

  Widget printAllPages, printEvenPages, printOddPages, printBackOrder;
};

#endif
