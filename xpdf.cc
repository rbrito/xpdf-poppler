//========================================================================
//
// xpdf.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <poppler-config.h>
#include "poppler/goo/GooString.h"
#include "parseargs.h"
#include "poppler/goo/gfile.h"
#include "poppler/goo/gmem.h"
#include "GlobalParamsGUI.h"
#include <GlobalParams.h>
#include "poppler/Object.h"
#include "XPDFApp.h"
#include "config.h"

//------------------------------------------------------------------------
// command line options
//------------------------------------------------------------------------

static bool contView = false;
static char enableT1libStr[16] = "";
static char enableFreeTypeStr[16] = "";
static char antialiasStr[16] = "";
static char vectorAntialiasStr[16] = "";
static char autohintingStr[16] = "no";
static char psFileArg[256];
static char paperSize[15] = "";
static int paperWidth = 0;
static int paperHeight = 0;
static bool level1 = false;
static char textEncName[128] = "";
static char textEOL[16] = "";
static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static bool fullScreen = false;
static char remoteName[100] = "xpdf_";
static char remoteCmd[512] = "";
static bool doRemoteReload = false;
static bool doRemoteRaise = false;
static bool doRemoteQuit = false;
static bool printCommands = false;
static bool quiet = false;
static char cfgFileName[256] = "";
static bool printVersion = false;
static bool printHelp = false;

static ArgDesc argDesc[] = {
  {"-g",          argStringDummy, NULL,           0,
   "initial window geometry"},
  {"-geometry",   argStringDummy, NULL,           0,
   "initial window geometry"},
  {"-title",      argStringDummy, NULL,           0,
   "window title"},
  {"-cmap",       argFlagDummy,   NULL,           0,
   "install a private colormap"},
  {"-rgb",        argIntDummy,    NULL,           0,
   "biggest RGB cube to allocate (default is 5)"},
  {"-rv",         argFlagDummy,   NULL,           0,
   "reverse video"},
  {"-papercolor", argStringDummy, NULL,           0,
   "color of paper background"},
  {"-mattecolor", argStringDummy, NULL,		  0,
   "color of background outside actual page"},
  {"-z",          argStringDummy, NULL,           0,
   "initial zoom level (percent, 'page', 'width')"},
  {"-cont",       argFlag,        &contView,      0,
   "start in continuous view mode" },
#if HAVE_T1LIB_H
  {"-t1lib",      argString,      enableT1libStr, sizeof(enableT1libStr),
   "enable t1lib font rasterizer: yes, no"},
#endif
#if HAVE_FREETYPE_FREETYPE_H | HAVE_FREETYPE_H
  {"-freetype",   argString,      enableFreeTypeStr, sizeof(enableFreeTypeStr),
   "enable FreeType font rasterizer: yes, no"},
#endif
  {"-aa",         argString,      antialiasStr,   sizeof(antialiasStr),
   "enable font anti-aliasing: yes, no"},
  {"-aaVector",   argString,      vectorAntialiasStr, sizeof(vectorAntialiasStr),
   "enable vector anti-aliasing: yes, no"},
  {"-ah",         argString,      autohintingStr, sizeof(autohintingStr),
   "enable font auto-hinting: yes, no (default no)"},
  {"-ps",         argString,      psFileArg,      sizeof(psFileArg),
   "default PostScript file name or command"},
  {"-paper",      argString,      paperSize,      sizeof(paperSize),
   "paper size (letter, legal, A4, A3, match)"},
  {"-paperw",     argInt,         &paperWidth,    0,
   "paper width, in points"},
  {"-paperh",     argInt,         &paperHeight,   0,
   "paper height, in points"},
  {"-level1",     argFlag,        &level1,        0,
   "generate Level 1 PostScript"},
  {"-enc",    argString,   textEncName,    sizeof(textEncName),
   "output text encoding name"},
  {"-eol",    argString,   textEOL,        sizeof(textEOL),
   "output end-of-line convention (unix, dos, or mac)"},
  {"-opw",        argString,      ownerPassword,  sizeof(ownerPassword),
   "owner password (for encrypted files)"},
  {"-upw",        argString,      userPassword,   sizeof(userPassword),
   "user password (for encrypted files)"},
  {"-fullscreen", argFlag,        &fullScreen,    0,
   "run in full-screen (presentation) mode"},
  {"-remote",     argString,      remoteName + 5, sizeof(remoteName) - 5,
   "start/contact xpdf remote server with specified name"},
  {"-exec",       argString,      remoteCmd,      sizeof(remoteCmd),
   "execute command on xpdf remote server (with -remote only)"},
  {"-reload",     argFlag,        &doRemoteReload, 0,
   "reload xpdf remote server window (with -remote only)"},
  {"-raise",      argFlag,        &doRemoteRaise, 0,
   "raise xpdf remote server window (with -remote only)"},
  {"-quit",       argFlag,        &doRemoteQuit,  0,
   "kill xpdf remote server (with -remote only)"},
  {"-cmd",        argFlag,        &printCommands, 0,
   "print commands as they're executed"},
  {"-q",          argFlag,        &quiet,         0,
   "don't print any messages or errors"},
  {"-cfg",        argString,      cfgFileName,    sizeof(cfgFileName),
   "configuration file to use in place of .xpdfrc"},
  {"-v",          argFlag,        &printVersion,  0,
   "print copyright and version info"},
  {"-h",          argFlag,        &printHelp,     0,
   "print usage information"},
  {"-help",       argFlag,        &printHelp,     0,
   "print usage information"},
  {"--help",  argFlag,     &printHelp,     0,
   "print usage information"},
  {"-?",      argFlag,     &printHelp,     0,
   "print usage information"},
  {NULL}
};

//------------------------------------------------------------------------

int main(int argc, char *argv[]) {
  XPDFApp *app;
  GooString *fileName;
  int pg;
  GooString *destName;
  GooString *userPasswordStr, *ownerPasswordStr;
  bool ok;
  int exitCode;

  exitCode = 0;
  userPasswordStr = ownerPasswordStr = NULL;

  // parse args
  ok = parseArgs(argDesc, &argc, argv);
  if (!ok || printVersion || printHelp) {
    fprintf(stderr, "xpdf version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("xpdf", "[<PDF-file> [<page> | +<dest>]]", argDesc);
    }
    exitCode = 99;
    goto done0;
  }

  // poppler's globalParams
  globalParams = new GlobalParams;

  // read config file
  globalParamsGUI = new GlobalParamsGUI(cfgFileName);
  globalParamsGUI->setupBaseFonts(NULL);
  if (contView) {
    globalParamsGUI->setContinuousView(contView);
  }
  if (psFileArg[0]) {
    globalParamsGUI->setPSFile(psFileArg);
  }
  if (paperSize[0]) {
    if (!globalParamsGUI->setPSPaperSize(paperSize)) {
      fprintf(stderr, "Invalid paper size\n");
    }
  } else {
    if (paperWidth) {
      globalParamsGUI->setPSPaperWidth(paperWidth);
    }
    if (paperHeight) {
      globalParamsGUI->setPSPaperHeight(paperHeight);
    }
  }
  if (level1) {
    globalParamsGUI->setPSLevel(psLevel1);
  }
  if (textEncName[0]) {
    globalParamsGUI->setTextEncoding(textEncName);
  }
  if (textEOL[0]) {
    if (!globalParamsGUI->setTextEOL(textEOL)) {
      fprintf(stderr, "Bad '-eol' value on command line\n");
    }
  }
  if (enableT1libStr[0]) {
    if (!globalParamsGUI->setEnableT1lib(enableT1libStr)) {
      fprintf(stderr, "Bad '-t1lib' value on command line\n");
    }
  }
  if (enableFreeTypeStr[0]) {
    if (!globalParamsGUI->setEnableFreeType(enableFreeTypeStr)) {
      fprintf(stderr, "Bad '-freetype' value on command line\n");
    }
  }
  if (antialiasStr[0]) {
    if (!globalParamsGUI->setAntialias(antialiasStr)) {
      fprintf(stderr, "Bad '-aa' value on command line\n");
    }
  }
  if (vectorAntialiasStr[0]) {
    if (!globalParamsGUI->setVectorAntialias(vectorAntialiasStr)) {
      fprintf(stderr, "Bad '-aaVector' value on command line\n");
    }
  }
  if (autohintingStr[0]) {
    if (!globalParamsGUI->setEnableFreeTypeHinting(autohintingStr)) {
      fprintf(stderr, "Bad '-ah' value on command line\n");
    }
  }
  if (printCommands) {
    globalParamsGUI->setPrintCommands(printCommands);
  }
  if (quiet) {
    globalParamsGUI->setErrQuiet(quiet);
  }

  // create the XPDFApp object
  app = new XPDFApp(&argc, argv);

  // the initialZoom parameter can be set in either the config file or
  // as an X resource (or command line arg)
  if (app->getInitialZoom()) {
    globalParamsGUI->setInitialZoom(app->getInitialZoom()->getCString());
  }

  // check command line
  ok = ok && argc >= 1 && argc <= 3;
  if (remoteCmd[0]) {
    ok = ok && remoteName[5] && !doRemoteReload && !doRemoteRaise &&
         !doRemoteQuit && argc == 1;
  }
  if (doRemoteReload) {
    ok = ok && remoteName[5] && !doRemoteQuit && argc == 1;
  }
  if (doRemoteRaise) {
    ok = ok && remoteName[5] && !doRemoteQuit;
  }
  if (doRemoteQuit) {
    ok = ok && remoteName[5] && argc == 1;
  }
  if (!ok || printVersion || printHelp) {
    fprintf(stderr, "xpdf version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("xpdf", "[<PDF-file> [<page> | +<dest>]]", argDesc);
    }
    exitCode = 99;
    goto done1;
  }
  if (argc >= 2) {
    fileName = new GooString(argv[1]);
  } else {
    fileName = NULL;
  }
  pg = 1;
  destName = NULL;
  if (argc == 3) {
    if (argv[2][0] == '+') {
      destName = new GooString(&argv[2][1]);
    } else {
      pg = atoi(argv[2]);
      if (pg < 0) {
	fprintf(stderr, "Invalid page number (%d)\n", pg);
	exitCode = 99;
	goto done2;
      }
    }
  }

  // handle remote server stuff
  if (remoteName[5]) {
    app->setRemoteName(remoteName);
    if (app->remoteServerRunning()) {
      if (fileName) {
	if (destName) {
	  app->remoteOpenAtDest(fileName, destName, doRemoteRaise);
	} else {
	  app->remoteOpen(fileName, pg, doRemoteRaise);
	}
      } else if (remoteCmd[0]) {
	app->remoteExec(remoteCmd);
      } else if (doRemoteReload) {
	app->remoteReload(doRemoteRaise);
      } else if (doRemoteRaise) {
	app->remoteRaise();
      } else if (doRemoteQuit) {
	app->remoteQuit();
      }
      goto done2;
    }
    if (doRemoteQuit) {
      goto done2;
    }
  }

  // set options
  app->setFullScreen(fullScreen);

  // check for password string(s)
  ownerPasswordStr = ownerPassword[0] != '\001' ? new GooString(ownerPassword)
                                                : (GooString *)NULL;
  userPasswordStr = userPassword[0] != '\001' ? new GooString(userPassword)
                                              : (GooString *)NULL;

  // open the file and run the main loop
  if (destName) {
    if (!app->openAtDest(fileName, destName,
			 ownerPasswordStr, userPasswordStr)) {
      exitCode = 1;
      goto done2;
    }
  } else {
    if (!app->open(fileName, pg, ownerPasswordStr, userPasswordStr)) {
      exitCode = 1;
      goto done2;
    }
  }
  app->run();

  exitCode = 0;

  // clean up
 done2:
  delete userPasswordStr;
  delete ownerPasswordStr;
  delete destName;
  delete fileName;
 done1:
  delete app;
  delete globalParamsGUI;
  delete globalParams;

  // check for memory leaks
 done0:
  Object::memCheck(stderr);
  gMemReport(stderr);

  return exitCode;
}
