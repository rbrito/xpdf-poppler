//========================================================================
//
// GlobalParamsGUI.cc
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

#include "config.h"

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#ifdef ENABLE_PLUGINS
#  ifndef WIN32
#    include <dlfcn.h>
#  endif
#endif
#ifdef WIN32
#  include <shlobj.h>
#endif
#if HAVE_PAPER_H
#include <paper.h>
#endif
#include <fontconfig/fontconfig.h>
#include "goo/gmem.h"
#include "goo/GooString.h"
#include "goo/GooList.h"
#include "goo/GooHash.h"
#include "goo/gfile.h"
#include "Error.h"
#include "NameToCharCode.h"
#include "CharCodeToUnicode.h"
#include "UnicodeMap.h"
#include "poppler/CMap.h"
#include "FontEncodingTables.h"
#include <langinfo.h>
#include <locale.h>
#ifdef ENABLE_PLUGINS
#  include "XpdfPluginAPI.h"
#endif
#include "GlobalParamsGUI.h"
#include "GfxFont.h"

#ifdef WIN32
#  define strcasecmp stricmp
#endif

#if MULTITHREADED
#  define lockGlobalParamsGUI            gLockMutex(&mutex)
#  define lockUnicodeMapCache         gLockMutex(&unicodeMapCacheMutex)
#  define lockCMapCache               gLockMutex(&cMapCacheMutex)
#  define unlockGlobalParamsGUI          gUnlockMutex(&mutex)
#  define unlockUnicodeMapCache       gUnlockMutex(&unicodeMapCacheMutex)
#  define unlockCMapCache             gUnlockMutex(&cMapCacheMutex)
#else
#  define lockGlobalParamsGUI
#  define lockUnicodeMapCache
#  define lockCMapCache
#  define unlockGlobalParamsGUI
#  define unlockUnicodeMapCache
#  define unlockCMapCache
#endif

#ifndef FC_WEIGHT_BOOK
#define FC_WEIGHT_BOOK 75
#endif

#include "NameToUnicodeTable.h"
#include "UnicodeMapTables.h"
#include "UTF8.h"

#ifdef ENABLE_PLUGINS
#  ifdef WIN32
extern XpdfPluginVecTable xpdfPluginVecTable;
#  endif
#endif

//------------------------------------------------------------------------

#define cidToUnicodeCacheSize     4
#define unicodeToUnicodeCacheSize 4

//------------------------------------------------------------------------

static struct {
  char *name;
  char *t1FileName;
  char *ttFileName;
} displayFontTab[] = {
  {"Courier",               "n022003l.pfb", "cour.ttf"},
  {"Courier-Bold",          "n022004l.pfb", "courbd.ttf"},
  {"Courier-BoldOblique",   "n022024l.pfb", "courbi.ttf"},
  {"Courier-Oblique",       "n022023l.pfb", "couri.ttf"},
  {"Helvetica",             "n019003l.pfb", "arial.ttf"},
  {"Helvetica-Bold",        "n019004l.pfb", "arialbd.ttf"},
  {"Helvetica-BoldOblique", "n019024l.pfb", "arialbi.ttf"},
  {"Helvetica-Oblique",     "n019023l.pfb", "ariali.ttf"},
  {"Symbol",                "s050000l.pfb", NULL},
  {"Times-Bold",            "n021004l.pfb", "timesbd.ttf"},
  {"Times-BoldItalic",      "n021024l.pfb", "timesbi.ttf"},
  {"Times-Italic",          "n021023l.pfb", "timesi.ttf"},
  {"Times-Roman",           "n021003l.pfb", "times.ttf"},
  {"ZapfDingbats",          "d050000l.pfb", NULL},
  {NULL}
};

#ifdef WIN32
static char *displayFontDirs[] = {
  "c:/windows/fonts",
  "c:/winnt/fonts",
  NULL
};
#else
static char *displayFontDirs[] = {
  "/usr/share/ghostscript/fonts",
  "/usr/local/share/ghostscript/fonts",
  "/usr/share/fonts/default/Type1",
  "/usr/share/fonts/default/ghostscript",
  "/usr/share/fonts/type1/gsfonts",
  NULL
};
#endif

//------------------------------------------------------------------------

GlobalParamsGUI *globalParamsGUI = NULL;

//------------------------------------------------------------------------
// DisplayFontParam
//------------------------------------------------------------------------

DisplayFontParam::DisplayFontParam(GooString *nameA,
				   DisplayFontParamKind kindA) {
  name = nameA;
  kind = kindA;
  switch (kind) {
  case displayFontT1:
    t1.fileName = NULL;
    break;
  case displayFontTT:
    tt.fileName = NULL;
    break;
  }
}

DisplayFontParam::~DisplayFontParam() {
  delete name;
  switch (kind) {
  case displayFontT1:
    if (t1.fileName) {
      delete t1.fileName;
    }
    break;
  case displayFontTT:
    if (tt.fileName) {
      delete tt.fileName;
    }
    break;
  }
}

#ifdef WIN32

//------------------------------------------------------------------------
// WinFontInfo
//------------------------------------------------------------------------

class WinFontInfo: public DisplayFontParam {
public:

  GBool bold, italic;

  static WinFontInfo *make(GooString *nameA, GBool boldA, GBool italicA,
			   HKEY regKey, char *winFontDir);
  WinFontInfo(GooString *nameA, GBool boldA, GBool italicA,
	      GooString *fileNameA);
  virtual ~WinFontInfo();
  GBool equals(WinFontInfo *fi);
};

WinFontInfo *WinFontInfo::make(GooString *nameA, GBool boldA, GBool italicA,
			       HKEY regKey, char *winFontDir) {
  GooString *regName;
  GooString *fileNameA;
  char buf[MAX_PATH];
  DWORD n;
  char c;
  int i;

  //----- find the font file
  fileNameA = NULL;
  regName = nameA->copy();
  if (boldA) {
    regName->append(" Bold");
  }
  if (italicA) {
    regName->append(" Italic");
  }
  regName->append(" (TrueType)");
  n = sizeof(buf);
  if (RegQueryValueEx(regKey, regName->getCString(), NULL, NULL,
		      (LPBYTE)buf, &n) == ERROR_SUCCESS) {
    fileNameA = new GooString(winFontDir);
    fileNameA->append('\\')->append(buf);
  }
  delete regName;
  if (!fileNameA) {
    delete nameA;
    return NULL;
  }

  //----- normalize the font name
  i = 0;
  while (i < nameA->getLength()) {
    c = nameA->getChar(i);
    if (c == ' ' || c == ',' || c == '-') {
      nameA->del(i);
    } else {
      ++i;
    }
  }

  return new WinFontInfo(nameA, boldA, italicA, fileNameA);
}

WinFontInfo::WinFontInfo(GooString *nameA, GBool boldA, GBool italicA,
			 GooString *fileNameA):
  DisplayFontParam(nameA, displayFontTT)
{
  bold = boldA;
  italic = italicA;
  tt.fileName = fileNameA;
}

WinFontInfo::~WinFontInfo() {
}

GBool WinFontInfo::equals(WinFontInfo *fi) {
  return !name->cmp(fi->name) && bold == fi->bold && italic == fi->italic;
}

//------------------------------------------------------------------------
// WinFontList
//------------------------------------------------------------------------

class WinFontList {
public:

  WinFontList(char *winFontDirA);
  ~WinFontList();
  WinFontInfo *find(GooString *font);

private:

  void add(WinFontInfo *fi);
  static int CALLBACK enumFunc1(CONST LOGFONT *font,
				CONST TEXTMETRIC *metrics,
				DWORD type, LPARAM data);
  static int CALLBACK enumFunc2(CONST LOGFONT *font,
				CONST TEXTMETRIC *metrics,
				DWORD type, LPARAM data);

  GooList *fonts;			// [WinFontInfo]
  HDC dc;			// (only used during enumeration)
  HKEY regKey;			// (only used during enumeration)
  char *winFontDir;		// (only used during enumeration)
};

WinFontList::WinFontList(char *winFontDirA) {
  OSVERSIONINFO version;
  char *path;

  fonts = new GooList();
  dc = GetDC(NULL);
  winFontDir = winFontDirA;
  version.dwOSVersionInfoSize = sizeof(version);
  GetVersionEx(&version);
  if (version.dwPlatformId == VER_PLATFORM_WIN32_NT) {
    path = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts\\";
  } else {
    path = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Fonts\\";
  }
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0,
		   KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
		   &regKey) == ERROR_SUCCESS) {
    EnumFonts(dc, NULL, &WinFontList::enumFunc1, (LPARAM)this);
    RegCloseKey(regKey);
  }
  ReleaseDC(NULL, dc);
}

WinFontList::~WinFontList() {
  deleteGooList(fonts, WinFontInfo);
}

void WinFontList::add(WinFontInfo *fi) {
  int i;

  for (i = 0; i < fonts->getLength(); ++i) {
    if (((WinFontInfo *)fonts->get(i))->equals(fi)) {
      delete fi;
      return;
    }
  }
  fonts->append(fi);
}

WinFontInfo *WinFontList::find(GooString *font) {
  GooString *name;
  GBool bold, italic;
  WinFontInfo *fi;
  char c;
  int n, i;

  name = font->copy();

  // remove space, comma, dash chars
  i = 0;
  while (i < name->getLength()) {
    c = name->getChar(i);
    if (c == ' ' || c == ',' || c == '-') {
      name->del(i);
    } else {
      ++i;
    }
  }
  n = name->getLength();

  // remove trailing "MT" (Foo-MT, Foo-BoldMT, etc.)
  if (!strcmp(name->getCString() + n - 2, "MT")) {
    name->del(n - 2, 2);
    n -= 2;
  }

  // look for "Italic"
  if (!strcmp(name->getCString() + n - 6, "Italic")) {
    name->del(n - 6, 6);
    italic = gTrue;
    n -= 6;
  } else {
    italic = gFalse;
  }

  // look for "Bold"
  if (!strcmp(name->getCString() + n - 4, "Bold")) {
    name->del(n - 4, 4);
    bold = gTrue;
    n -= 4;
  } else {
    bold = gFalse;
  }

  // remove trailing "MT" (FooMT-Bold, etc.)
  if (!strcmp(name->getCString() + n - 2, "MT")) {
    name->del(n - 2, 2);
    n -= 2;
  }

  // remove trailing "PS"
  if (!strcmp(name->getCString() + n - 2, "PS")) {
    name->del(n - 2, 2);
    n -= 2;
  }

  // search for the font
  fi = NULL;
  for (i = 0; i < fonts->getLength(); ++i) {
    fi = (WinFontInfo *)fonts->get(i);
    if (!fi->name->cmp(name) && fi->bold == bold && fi->italic == italic) {
      break;
    }
    fi = NULL;
  }

  delete name;
  return fi;
}

int CALLBACK WinFontList::enumFunc1(CONST LOGFONT *font,
				    CONST TEXTMETRIC *metrics,
				    DWORD type, LPARAM data) {
  WinFontList *fl = (WinFontList *)data;

  EnumFonts(fl->dc, font->lfFaceName, &WinFontList::enumFunc2, (LPARAM)fl);
  return 1;
}

int CALLBACK WinFontList::enumFunc2(CONST LOGFONT *font,
				    CONST TEXTMETRIC *metrics,
				    DWORD type, LPARAM data) {
  WinFontList *fl = (WinFontList *)data;
  WinFontInfo *fi;

  if (type & TRUETYPE_FONTTYPE) {
    if ((fi = WinFontInfo::make(new GooString(font->lfFaceName),
				font->lfWeight >= 600,
				font->lfItalic ? gTrue : gFalse,
				fl->regKey, fl->winFontDir))) {
      fl->add(fi);
    }
  }
  return 1;
}

#endif // WIN32

//------------------------------------------------------------------------
// PSFontParam
//------------------------------------------------------------------------

PSFontParam::PSFontParam(GooString *pdfFontNameA, int wModeA,
			 GooString *psFontNameA, GooString *encodingA) {
  pdfFontName = pdfFontNameA;
  wMode = wModeA;
  psFontName = psFontNameA;
  encoding = encodingA;
}

PSFontParam::~PSFontParam() {
  delete pdfFontName;
  delete psFontName;
  if (encoding) {
    delete encoding;
  }
}

//------------------------------------------------------------------------
// KeyBinding
//------------------------------------------------------------------------

KeyBinding::KeyBinding(int codeA, int modsA, int contextA, char *cmd0) {
  code = codeA;
  mods = modsA;
  context = contextA;
  cmds = new GooList();
  cmds->append(new GooString(cmd0));
}

KeyBinding::KeyBinding(int codeA, int modsA, int contextA,
		       char *cmd0, char *cmd1) {
  code = codeA;
  mods = modsA;
  context = contextA;
  cmds = new GooList();
  cmds->append(new GooString(cmd0));
  cmds->append(new GooString(cmd1));
}

KeyBinding::KeyBinding(int codeA, int modsA, int contextA, GooList *cmdsA) {
  code = codeA;
  mods = modsA;
  context = contextA;
  cmds = cmdsA;
}

KeyBinding::~KeyBinding() {
  deleteGooList(cmds, GooString);
}

#ifdef ENABLE_PLUGINS
//------------------------------------------------------------------------
// Plugin
//------------------------------------------------------------------------

class Plugin {
public:

  static Plugin *load(char *type, char *name);
  ~Plugin();

private:

#ifdef WIN32
  Plugin(HMODULE libA);
  HMODULE lib;
#else
  Plugin(void *dlA);
  void *dl;
#endif
};

Plugin *Plugin::load(char *type, char *name) {
  GooString *path;
  Plugin *plugin;
  XpdfPluginVecTable *vt;
  XpdfBool (*xpdfInitPlugin)(void);
#ifdef WIN32
  HMODULE libA;
#else
  void *dlA;
#endif

  path = globalParamsGUI->getBaseDir();
  appendToPath(path, "plugins");
  appendToPath(path, type);
  appendToPath(path, name);

#ifdef WIN32
  path->append(".dll");
  if (!(libA = LoadLibrary(path->getCString()))) {
    error(-1, "Failed to load plugin '%s'",
	  path->getCString());
    goto err1;
  }
  if (!(vt = (XpdfPluginVecTable *)
	         GetProcAddress(libA, "xpdfPluginVecTable"))) {
    error(-1, "Failed to find xpdfPluginVecTable in plugin '%s'",
	  path->getCString());
    goto err2;
  }
#else
  //~ need to deal with other extensions here
  path->append(".so");
  if (!(dlA = dlopen(path->getCString(), RTLD_NOW))) {
    error(-1, "Failed to load plugin '%s': %s",
	  path->getCString(), dlerror());
    goto err1;
  }
  if (!(vt = (XpdfPluginVecTable *)dlsym(dlA, "xpdfPluginVecTable"))) {
    error(-1, "Failed to find xpdfPluginVecTable in plugin '%s'",
	  path->getCString());
    goto err2;
  }
#endif

  if (vt->version != xpdfPluginVecTable.version) {
    error(-1, "Plugin '%s' is wrong version", path->getCString());
    goto err2;
  }
  memcpy(vt, &xpdfPluginVecTable, sizeof(xpdfPluginVecTable));

#ifdef WIN32
  if (!(xpdfInitPlugin = (XpdfBool (*)(void))
	                     GetProcAddress(libA, "xpdfInitPlugin"))) {
    error(-1, "Failed to find xpdfInitPlugin in plugin '%s'",
	  path->getCString());
    goto err2;
  }
#else
  if (!(xpdfInitPlugin = (XpdfBool (*)(void))dlsym(dlA, "xpdfInitPlugin"))) {
    error(-1, "Failed to find xpdfInitPlugin in plugin '%s'",
	  path->getCString());
    goto err2;
  }
#endif

  if (!(*xpdfInitPlugin)()) {
    error(-1, "Initialization of plugin '%s' failed",
	  path->getCString());
    goto err2;
  }

#ifdef WIN32
  plugin = new Plugin(libA);
#else
  plugin = new Plugin(dlA);
#endif

  delete path;
  return plugin;

 err2:
#ifdef WIN32
  FreeLibrary(libA);
#else
  dlclose(dlA);
#endif
 err1:
  delete path;
  return NULL;
}

#ifdef WIN32
Plugin::Plugin(HMODULE libA) {
  lib = libA;
}
#else
Plugin::Plugin(void *dlA) {
  dl = dlA;
}
#endif

Plugin::~Plugin() {
  void (*xpdfFreePlugin)(void);

#ifdef WIN32
  if ((xpdfFreePlugin = (void (*)(void))
                            GetProcAddress(lib, "xpdfFreePlugin"))) {
    (*xpdfFreePlugin)();
  }
  FreeLibrary(lib);
#else
  if ((xpdfFreePlugin = (void (*)(void))dlsym(dl, "xpdfFreePlugin"))) {
    (*xpdfFreePlugin)();
  }
  dlclose(dl);
#endif
}

#endif // ENABLE_PLUGINS

//------------------------------------------------------------------------
// parsing
//------------------------------------------------------------------------

GlobalParamsGUI::GlobalParamsGUI(char *cfgFileName) {
  UnicodeMap *map;
  GooString *fileName;
  FILE *f;
  int i;

  FcInit();
  FCcfg = FcConfigGetCurrent();

#if MULTITHREADED
  gInitMutex(&mutex);
  gInitMutex(&unicodeMapCacheMutex);
  gInitMutex(&cMapCacheMutex);
#endif

  // scan the encoding in reverse because we want the lowest-numbered
  // index for each char name ('space' is encoded twice)
  macRomanReverseMap = new NameToCharCode();
  for (i = 255; i >= 0; --i) {
    if (macRomanEncoding[i]) {
      macRomanReverseMap->add(macRomanEncoding[i], (CharCode)i);
    }
  }

#ifdef WIN32
  // baseDir will be set by a call to setBaseDir
  baseDir = new GooString();
#else
  baseDir = appendToPath(getHomeDir(), ".xpdf");
#endif
  nameToUnicode = new NameToCharCode();
  cidToUnicodes = new GooHash(gTrue);
  unicodeToUnicodes = new GooHash(gTrue);
  residentUnicodeMaps = new GooHash();
  unicodeMaps = new GooHash(gTrue);
  cMapDirs = new GooHash(gTrue);
  toUnicodeDirs = new GooList();
  displayFonts = new GooHash();
  displayCIDFonts = new GooHash();
  displayNamedCIDFonts = new GooHash();
#if HAVE_PAPER_H
  char *paperName;
  const struct paper *paperType;
  paperinit();
  if ((paperName = systempapername())) {
    paperType = paperinfo(paperName);
    psPaperWidth = (int)paperpswidth(paperType);
    psPaperHeight = (int)paperpsheight(paperType);
  } else {
    error(-1, "No paper information available - using defaults");
    psPaperWidth = defPaperWidth;
    psPaperHeight = defPaperHeight;
  }
  paperdone();
#else
  psPaperWidth = defPaperWidth;
  psPaperHeight = defPaperHeight;
#endif
  psImageableLLX = psImageableLLY = 0;
  psImageableURX = psPaperWidth;
  psImageableURY = psPaperHeight;
  psCrop = gTrue;
  psExpandSmaller = gFalse;
  psShrinkLarger = gTrue;
  psCenter = gTrue;
  psDuplex = gFalse;
  psLevel = psLevel2;
  psFile = NULL;
  psFonts = new GooHash();
  psNamedFonts16 = new GooList();
  psFonts16 = new GooList();
  psEmbedType1 = gTrue;
  psEmbedTrueType = gTrue;
  psEmbedCIDPostScript = gTrue;
  psEmbedCIDTrueType = gTrue;
  psPreload = gFalse;
  psOPI = gFalse;
  psASCIIHex = gFalse;
  setlocale(LC_ALL,"");
  setlocale(LC_NUMERIC,"POSIX");
  if (strcmp("UTF-8",nl_langinfo(CODESET)))
         textEncoding = new GooString("Latin1");
  else
         textEncoding = new GooString("UTF-8");
#if defined(WIN32)
  textEOL = eolDOS;
#elif defined(MACOS)
  textEOL = eolMac;
#else
  textEOL = eolUnix;
#endif
  textPageBreaks = gTrue;
  textKeepTinyChars = gFalse;
  fontDirs = new GooList();
  initialZoom = new GooString("125");
  continuousView = gFalse;
  enableT1lib = gTrue;
  enableFreeType = gTrue;
  enableFreeTypeHinting = gFalse;
  antialias = gTrue;
  vectorAntialias = gTrue;
  strokeAdjust = gTrue;
  screenType = screenUnset;
  screenSize = -1;
  screenDotRadius = -1;
  screenGamma = 1.0;
  screenBlackThreshold = 0.0;
  screenWhiteThreshold = 1.0;
  urlCommand = NULL;
  movieCommand = NULL;
  mapNumericCharNames = gTrue;
  mapUnknownCharNames = gFalse;
  createDefaultKeyBindings();
  printCommands = gFalse;
  profileCommands = gFalse;
  errQuiet = gFalse;

  cidToUnicodeCache = new CharCodeToUnicodeCache(cidToUnicodeCacheSize);
  unicodeToUnicodeCache =
      new CharCodeToUnicodeCache(unicodeToUnicodeCacheSize);
  unicodeMapCache = new UnicodeMapCache();
  cMapCache = new CMapCache();

#ifdef WIN32
  winFontList = NULL;
#endif

#ifdef ENABLE_PLUGINS
  plugins = new GooList();
  securityHandlers = new GooList();
#endif

  // set up the initial nameToUnicode table
  for (i = 0; nameToUnicodeTab[i].name; ++i) {
    nameToUnicode->add(nameToUnicodeTab[i].name, nameToUnicodeTab[i].u);
  }

  // set up the residentUnicodeMaps table
  map = new UnicodeMap("Latin1", gFalse,
		       latin1UnicodeMapRanges, latin1UnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("ASCII7", gFalse,
		       ascii7UnicodeMapRanges, ascii7UnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("Symbol", gFalse,
		       symbolUnicodeMapRanges, symbolUnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("ZapfDingbats", gFalse, zapfDingbatsUnicodeMapRanges,
		       zapfDingbatsUnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("UTF-8", gTrue, &mapUTF8);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("UCS-2", gTrue, &mapUCS2);
  residentUnicodeMaps->add(map->getEncodingName(), map);

  // look for a user config file, then a system-wide config file
  f = NULL;
  fileName = NULL;
  if (cfgFileName && cfgFileName[0]) {
    fileName = new GooString(cfgFileName);
    if (!(f = fopen(fileName->getCString(), "r"))) {
      delete fileName;
    }
  }
  if (!f) {
    fileName = appendToPath(getHomeDir(), xpdfUserConfigFile);
    if (!(f = fopen(fileName->getCString(), "r"))) {
      delete fileName;
    }
  }
  if (!f) {
#if defined(WIN32) && !defined(__CYGWIN32__)
    char buf[512];
    i = GetModuleFileName(NULL, buf, sizeof(buf));
    if (i <= 0 || i >= sizeof(buf)) {
      // error or path too long for buffer - just use the current dir
      buf[0] = '\0';
    }
    fileName = grabPath(buf);
    appendToPath(fileName, xpdfSysConfigFile);
#else
    fileName = new GooString(xpdfSysConfigFile);
#endif
    if (!(f = fopen(fileName->getCString(), "r"))) {
      delete fileName;
    }
  }
  if (f) {
    parseFile(fileName, f);
    delete fileName;
    fclose(f);
  }
}

void GlobalParamsGUI::createDefaultKeyBindings() {
  keyBindings = new GooList();

  //----- mouse buttons
  keyBindings->append(new KeyBinding(xpdfKeyCodeMousePress1, xpdfKeyModNone,
				     xpdfKeyContextAny, "startSelection"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeMouseRelease1, xpdfKeyModNone,
				     xpdfKeyContextAny, "endSelection",
				     "followLinkNoSel"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeMousePress2, xpdfKeyModNone,
				     xpdfKeyContextAny, "startPan"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeMouseRelease2, xpdfKeyModNone,
				     xpdfKeyContextAny, "endPan"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeMousePress3, xpdfKeyModNone,
				     xpdfKeyContextAny, "postPopupMenu"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeMousePress4, xpdfKeyModNone,
				     xpdfKeyContextAny,
				     "scrollUpPrevPage(16)"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeMousePress5, xpdfKeyModNone,
				     xpdfKeyContextAny,
				     "scrollDownNextPage(16)"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeMousePress6, xpdfKeyModNone,
				     xpdfKeyContextAny, "scrollLeft(16)"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeMousePress7, xpdfKeyModNone,
				     xpdfKeyContextAny, "scrollRight(16)"));

  //----- keys
  keyBindings->append(new KeyBinding(xpdfKeyCodeHome, xpdfKeyModCtrl,
				     xpdfKeyContextAny, "gotoPage(1)"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeHome, xpdfKeyModNone,
				     xpdfKeyContextAny, "scrollToTopLeft"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeEnd, xpdfKeyModCtrl,
				     xpdfKeyContextAny, "gotoLastPage"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeEnd, xpdfKeyModNone,
				     xpdfKeyContextAny,
				     "scrollToBottomRight"));
  keyBindings->append(new KeyBinding(xpdfKeyCodePgUp, xpdfKeyModNone,
				     xpdfKeyContextAny, "pageUp"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeBackspace, xpdfKeyModNone,
				     xpdfKeyContextAny, "pageUp"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeDelete, xpdfKeyModNone,
				     xpdfKeyContextAny, "pageUp"));
  keyBindings->append(new KeyBinding(xpdfKeyCodePgDn, xpdfKeyModNone,
				     xpdfKeyContextAny, "pageDown"));
  keyBindings->append(new KeyBinding(' ', xpdfKeyModNone,
				     xpdfKeyContextAny, "pageDown"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeLeft, xpdfKeyModNone,
				     xpdfKeyContextAny, "scrollLeft(16)"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeRight, xpdfKeyModNone,
				     xpdfKeyContextAny, "scrollRight(16)"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeUp, xpdfKeyModNone,
				     xpdfKeyContextAny, "scrollUp(16)"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeDown, xpdfKeyModNone,
				     xpdfKeyContextAny, "scrollDown(16)"));
  keyBindings->append(new KeyBinding('o', xpdfKeyModNone,
				     xpdfKeyContextAny, "open"));
  keyBindings->append(new KeyBinding('O', xpdfKeyModNone,
				     xpdfKeyContextAny, "open"));
  keyBindings->append(new KeyBinding('r', xpdfKeyModNone,
				     xpdfKeyContextAny, "reload"));
  keyBindings->append(new KeyBinding('R', xpdfKeyModNone,
				     xpdfKeyContextAny, "reload"));
  keyBindings->append(new KeyBinding('f', xpdfKeyModNone,
				     xpdfKeyContextAny, "find"));
  keyBindings->append(new KeyBinding('F', xpdfKeyModNone,
				     xpdfKeyContextAny, "find"));
  keyBindings->append(new KeyBinding('f', xpdfKeyModCtrl,
				     xpdfKeyContextAny, "find"));
  keyBindings->append(new KeyBinding('g', xpdfKeyModCtrl,
				     xpdfKeyContextAny, "findNext"));
  keyBindings->append(new KeyBinding('p', xpdfKeyModCtrl,
				     xpdfKeyContextAny, "print"));
  keyBindings->append(new KeyBinding('n', xpdfKeyModNone,
				     xpdfKeyContextScrLockOff, "nextPage"));
  keyBindings->append(new KeyBinding('N', xpdfKeyModNone,
				     xpdfKeyContextScrLockOff, "nextPage"));
  keyBindings->append(new KeyBinding('n', xpdfKeyModNone,
				     xpdfKeyContextScrLockOn,
				     "nextPageNoScroll"));
  keyBindings->append(new KeyBinding('N', xpdfKeyModNone,
				     xpdfKeyContextScrLockOn,
				     "nextPageNoScroll"));
  keyBindings->append(new KeyBinding('p', xpdfKeyModNone,
				     xpdfKeyContextScrLockOff, "prevPage"));
  keyBindings->append(new KeyBinding('P', xpdfKeyModNone,
				     xpdfKeyContextScrLockOff, "prevPage"));
  keyBindings->append(new KeyBinding('p', xpdfKeyModNone,
				     xpdfKeyContextScrLockOn,
				     "prevPageNoScroll"));
  keyBindings->append(new KeyBinding('P', xpdfKeyModNone,
				     xpdfKeyContextScrLockOn,
				     "prevPageNoScroll"));
  keyBindings->append(new KeyBinding('v', xpdfKeyModNone,
				     xpdfKeyContextAny, "goForward"));
  keyBindings->append(new KeyBinding('b', xpdfKeyModNone,
				     xpdfKeyContextAny, "goBackward"));
  keyBindings->append(new KeyBinding('g', xpdfKeyModNone,
				     xpdfKeyContextAny, "focusToPageNum"));
  keyBindings->append(new KeyBinding('0', xpdfKeyModNone,
				     xpdfKeyContextAny, "zoomPercent(125)"));
  keyBindings->append(new KeyBinding('+', xpdfKeyModNone,
				     xpdfKeyContextAny, "zoomIn"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeAdd, xpdfKeyModNone,
				     xpdfKeyContextAny, "zoomIn"));
  keyBindings->append(new KeyBinding('-', xpdfKeyModNone,
				     xpdfKeyContextAny, "zoomOut"));
  keyBindings->append(new KeyBinding(xpdfKeyCodeSubtract, xpdfKeyModNone,
				     xpdfKeyContextAny, "zoomOut"));
  keyBindings->append(new KeyBinding('z', xpdfKeyModNone,
				     xpdfKeyContextAny, "zoomFitPage"));
  keyBindings->append(new KeyBinding('w', xpdfKeyModNone,
				     xpdfKeyContextAny, "zoomFitWidth"));
  keyBindings->append(new KeyBinding('f', xpdfKeyModAlt,
				     xpdfKeyContextAny,
				     "toggleFullScreenMode"));
  keyBindings->append(new KeyBinding('l', xpdfKeyModCtrl,
				     xpdfKeyContextAny, "redraw"));
  keyBindings->append(new KeyBinding('w', xpdfKeyModCtrl,
				     xpdfKeyContextAny, "closeWindow"));
  keyBindings->append(new KeyBinding('?', xpdfKeyModNone,
				     xpdfKeyContextAny, "about"));
  keyBindings->append(new KeyBinding('q', xpdfKeyModNone,
				     xpdfKeyContextAny, "quit"));
  keyBindings->append(new KeyBinding('Q', xpdfKeyModNone,
				     xpdfKeyContextAny, "quit"));
}

void GlobalParamsGUI::parseFile(GooString *fileName, FILE *f) {
  int line;
  char buf[512];

  line = 1;
  while (getLine(buf, sizeof(buf) - 1, f)) {
    parseLine(buf, fileName, line);
    ++line;
  }
}

void GlobalParamsGUI::parseLine(char *buf, GooString *fileName, int line) {
  GooList *tokens;
  GooString *cmd, *incFile;
  char *p1, *p2;
  FILE *f2;

  // break the line into tokens
  tokens = new GooList();
  p1 = buf;
  while (*p1) {
    for (; *p1 && isspace(*p1); ++p1) ;
    if (!*p1) {
      break;
    }
    if (*p1 == '"' || *p1 == '\'') {
      for (p2 = p1 + 1; *p2 && *p2 != *p1; ++p2) ;
      ++p1;
    } else {
      for (p2 = p1 + 1; *p2 && !isspace(*p2); ++p2) ;
    }
    tokens->append(new GooString(p1, p2 - p1));
    p1 = *p2 ? p2 + 1 : p2;
  }

  // parse the line
  if (tokens->getLength() > 0 &&
      ((GooString *)tokens->get(0))->getChar(0) != '#') {
    cmd = (GooString *)tokens->get(0);
    if (!cmd->cmp("include")) {
      if (tokens->getLength() == 2) {
	incFile = (GooString *)tokens->get(1);
	if ((f2 = fopen(incFile->getCString(), "r"))) {
	  parseFile(incFile, f2);
	  fclose(f2);
	} else {
	  error(-1, "Couldn't find included config file: '%s' (%s:%d)",
		incFile->getCString(), fileName->getCString(), line);
	}
      } else {
	error(-1, "Bad 'include' config file command (%s:%d)",
	      fileName->getCString(), line);
      }
    } else if (!cmd->cmp("nameToUnicode")) {
      parseNameToUnicode(tokens, fileName, line);
    } else if (!cmd->cmp("cidToUnicode")) {
      parseCIDToUnicode(tokens, fileName, line);
    } else if (!cmd->cmp("unicodeToUnicode")) {
      parseUnicodeToUnicode(tokens, fileName, line);
    } else if (!cmd->cmp("unicodeMap")) {
      parseUnicodeMap(tokens, fileName, line);
    } else if (!cmd->cmp("cMapDir")) {
      parseCMapDir(tokens, fileName, line);
    } else if (!cmd->cmp("toUnicodeDir")) {
      parseToUnicodeDir(tokens, fileName, line);
    } else if (!cmd->cmp("displayFontT1")) {
      parseDisplayFont(tokens, displayFonts, displayFontT1, fileName, line);
    } else if (!cmd->cmp("displayFontTT")) {
      parseDisplayFont(tokens, displayFonts, displayFontTT, fileName, line);
    } else if (!cmd->cmp("displayNamedCIDFontT1")) {
      parseDisplayFont(tokens, displayNamedCIDFonts,
		       displayFontT1, fileName, line);
    } else if (!cmd->cmp("displayCIDFontT1")) {
      parseDisplayFont(tokens, displayCIDFonts,
		       displayFontT1, fileName, line);
    } else if (!cmd->cmp("displayNamedCIDFontTT")) {
      parseDisplayFont(tokens, displayNamedCIDFonts,
		       displayFontTT, fileName, line);
    } else if (!cmd->cmp("displayCIDFontTT")) {
      parseDisplayFont(tokens, displayCIDFonts,
		       displayFontTT, fileName, line);
    } else if (!cmd->cmp("psFile")) {
      parsePSFile(tokens, fileName, line);
    } else if (!cmd->cmp("psFont")) {
      parsePSFont(tokens, fileName, line);
    } else if (!cmd->cmp("psNamedFont16")) {
      parsePSFont16("psNamedFont16", psNamedFonts16,
		    tokens, fileName, line);
    } else if (!cmd->cmp("psFont16")) {
      parsePSFont16("psFont16", psFonts16, tokens, fileName, line);
    } else if (!cmd->cmp("psPaperSize")) {
      parsePSPaperSize(tokens, fileName, line);
    } else if (!cmd->cmp("psImageableArea")) {
      parsePSImageableArea(tokens, fileName, line);
    } else if (!cmd->cmp("psCrop")) {
      parseYesNo("psCrop", &psCrop, tokens, fileName, line);
    } else if (!cmd->cmp("psExpandSmaller")) {
      parseYesNo("psExpandSmaller", &psExpandSmaller,
		 tokens, fileName, line);
    } else if (!cmd->cmp("psShrinkLarger")) {
      parseYesNo("psShrinkLarger", &psShrinkLarger, tokens, fileName, line);
    } else if (!cmd->cmp("psCenter")) {
      parseYesNo("psCenter", &psCenter, tokens, fileName, line);
    } else if (!cmd->cmp("psDuplex")) {
      parseYesNo("psDuplex", &psDuplex, tokens, fileName, line);
    } else if (!cmd->cmp("psLevel")) {
      parsePSLevel(tokens, fileName, line);
    } else if (!cmd->cmp("psEmbedType1Fonts")) {
      parseYesNo("psEmbedType1", &psEmbedType1, tokens, fileName, line);
    } else if (!cmd->cmp("psEmbedTrueTypeFonts")) {
      parseYesNo("psEmbedTrueType", &psEmbedTrueType,
		 tokens, fileName, line);
    } else if (!cmd->cmp("psEmbedCIDPostScriptFonts")) {
      parseYesNo("psEmbedCIDPostScript", &psEmbedCIDPostScript,
		 tokens, fileName, line);
    } else if (!cmd->cmp("psEmbedCIDTrueTypeFonts")) {
      parseYesNo("psEmbedCIDTrueType", &psEmbedCIDTrueType,
		 tokens, fileName, line);
    } else if (!cmd->cmp("psPreload")) {
      parseYesNo("psPreload", &psPreload, tokens, fileName, line);
    } else if (!cmd->cmp("psOPI")) {
      parseYesNo("psOPI", &psOPI, tokens, fileName, line);
    } else if (!cmd->cmp("psASCIIHex")) {
      parseYesNo("psASCIIHex", &psASCIIHex, tokens, fileName, line);
    } else if (!cmd->cmp("textEncoding")) {
      parseTextEncoding(tokens, fileName, line);
    } else if (!cmd->cmp("textEOL")) {
      parseTextEOL(tokens, fileName, line);
    } else if (!cmd->cmp("textPageBreaks")) {
      parseYesNo("textPageBreaks", &textPageBreaks,
		 tokens, fileName, line);
    } else if (!cmd->cmp("textKeepTinyChars")) {
      parseYesNo("textKeepTinyChars", &textKeepTinyChars,
		 tokens, fileName, line);
    } else if (!cmd->cmp("fontDir")) {
      parseFontDir(tokens, fileName, line);
    } else if (!cmd->cmp("initialZoom")) {
      parseInitialZoom(tokens, fileName, line);
    } else if (!cmd->cmp("continuousView")) {
      parseYesNo("continuousView", &continuousView, tokens, fileName, line);
    } else if (!cmd->cmp("enableT1lib")) {
      parseYesNo("enableT1lib", &enableT1lib, tokens, fileName, line);
    } else if (!cmd->cmp("enableFreeType")) {
      parseYesNo("enableFreeType", &enableFreeType, tokens, fileName, line);
    } else if (!cmd->cmp("antialias")) {
      parseYesNo("antialias", &antialias, tokens, fileName, line);
    } else if (!cmd->cmp("vectorAntialias")) {
      parseYesNo("vectorAntialias", &vectorAntialias,
		 tokens, fileName, line);
    } else if (!cmd->cmp("strokeAdjust")) {
      parseYesNo("strokeAdjust", &strokeAdjust, tokens, fileName, line);
    } else if (!cmd->cmp("screenType")) {
      parseScreenType(tokens, fileName, line);
    } else if (!cmd->cmp("screenSize")) {
      parseInteger("screenSize", &screenSize, tokens, fileName, line);
    } else if (!cmd->cmp("screenDotRadius")) {
      parseInteger("screenDotRadius", &screenDotRadius,
		   tokens, fileName, line);
    } else if (!cmd->cmp("screenGamma")) {
      parseFloat("screenGamma", &screenGamma,
		 tokens, fileName, line);
    } else if (!cmd->cmp("screenBlackThreshold")) {
      parseFloat("screenBlackThreshold", &screenBlackThreshold,
		 tokens, fileName, line);
    } else if (!cmd->cmp("screenWhiteThreshold")) {
      parseFloat("screenWhiteThreshold", &screenWhiteThreshold,
		 tokens, fileName, line);
    } else if (!cmd->cmp("urlCommand")) {
      parseCommand("urlCommand", &urlCommand, tokens, fileName, line);
    } else if (!cmd->cmp("movieCommand")) {
      parseCommand("movieCommand", &movieCommand, tokens, fileName, line);
    } else if (!cmd->cmp("mapNumericCharNames")) {
      parseYesNo("mapNumericCharNames", &mapNumericCharNames,
		 tokens, fileName, line);
    } else if (!cmd->cmp("mapUnknownCharNames")) {
      parseYesNo("mapUnknownCharNames", &mapUnknownCharNames,
		 tokens, fileName, line);
    } else if (!cmd->cmp("bind")) {
      parseBind(tokens, fileName, line);
    } else if (!cmd->cmp("unbind")) {
      parseUnbind(tokens, fileName, line);
    } else if (!cmd->cmp("printCommands")) {
      parseYesNo("printCommands", &printCommands, tokens, fileName, line);
    } else if (!cmd->cmp("errQuiet")) {
      parseYesNo("errQuiet", &errQuiet, tokens, fileName, line);
    } else {
      error(-1, "Unknown config file command '%s' (%s:%d)",
	    cmd->getCString(), fileName->getCString(), line);
      if (!cmd->cmp("displayFontX") ||
	  !cmd->cmp("displayNamedCIDFontX") ||
	  !cmd->cmp("displayCIDFontX")) {
	error(-1, "-- Xpdf no longer supports X fonts");
      } else if (!cmd->cmp("t1libControl") || !cmd->cmp("freetypeControl")) {
	error(-1, "-- The t1libControl and freetypeControl options have been replaced");
	error(-1, "   by the enableT1lib, enableFreeType, and antialias options");
      } else if (!cmd->cmp("fontpath") || !cmd->cmp("fontmap")) {
	error(-1, "-- the config file format has changed since Xpdf 0.9x");
      }
    }
  }

  deleteGooList(tokens, GooString);
}

void GlobalParamsGUI::parseNameToUnicode(GooList *tokens, GooString *fileName,
					 int line) {
  GooString *name;
  char *tok1, *tok2;
  FILE *f;
  char buf[256];
  int line2;
  Unicode u;

  if (tokens->getLength() != 2) {
    error(-1, "Bad 'nameToUnicode' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  name = (GooString *)tokens->get(1);
  if (!(f = fopen(name->getCString(), "r"))) {
    error(-1, "Couldn't open 'nameToUnicode' file '%s'",
	  name->getCString());
    return;
  }
  line2 = 1;
  while (getLine(buf, sizeof(buf), f)) {
    tok1 = strtok(buf, " \t\r\n");
    tok2 = strtok(NULL, " \t\r\n");
    if (tok1 && tok2) {
      sscanf(tok1, "%x", &u);
      nameToUnicode->add(tok2, u);
    } else {
      error(-1, "Bad line in 'nameToUnicode' file (%s:%d)",
	    name->getCString(), line2);
    }
    ++line2;
  }
  fclose(f);
}

void GlobalParamsGUI::parseNameToUnicode(GooString *name) {
  char *tok1, *tok2;
  FILE *f;
  char buf[256];
  int line;
  Unicode u;

  if (!(f = fopen(name->getCString(), "r"))) {
    error(-1, "Couldn't open 'nameToUnicode' file '%s'",
	  name->getCString());
    return;
  }
  line = 1;
  while (getLine(buf, sizeof(buf), f)) {
    tok1 = strtok(buf, " \t\r\n");
    tok2 = strtok(NULL, " \t\r\n");
    if (tok1 && tok2) {
      sscanf(tok1, "%x", &u);
      nameToUnicode->add(tok2, u);
    } else {
      error(-1, "Bad line in 'nameToUnicode' file (%s:%d)",
	    name->getCString(), line);
    }
    ++line;
  }
  fclose(f);
}

void GlobalParamsGUI::parseCIDToUnicode(GooList *tokens, GooString *fileName,
				     int line) {
  GooString *collection, *name, *old;

  if (tokens->getLength() != 3) {
    error(-1, "Bad 'cidToUnicode' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  collection = (GooString *)tokens->get(1);
  name = (GooString *)tokens->get(2);
  if ((old = (GooString *)cidToUnicodes->remove(collection))) {
    delete old;
  }
  cidToUnicodes->add(collection->copy(), name->copy());
}

void GlobalParamsGUI::parseUnicodeToUnicode(GooList *tokens, GooString *fileName,
					 int line) {
  GooString *font, *file, *old;

  if (tokens->getLength() != 3) {
    error(-1, "Bad 'unicodeToUnicode' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  font = (GooString *)tokens->get(1);
  file = (GooString *)tokens->get(2);
  if ((old = (GooString *)unicodeToUnicodes->remove(font))) {
    delete old;
  }
  unicodeToUnicodes->add(font->copy(), file->copy());
}

void GlobalParamsGUI::parseUnicodeMap(GooList *tokens, GooString *fileName,
				   int line) {
  GooString *encodingName, *name, *old;

  if (tokens->getLength() != 3) {
    error(-1, "Bad 'unicodeMap' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  encodingName = (GooString *)tokens->get(1);
  name = (GooString *)tokens->get(2);
  if ((old = (GooString *)unicodeMaps->remove(encodingName))) {
    delete old;
  }
  unicodeMaps->add(encodingName->copy(), name->copy());
}

void GlobalParamsGUI::parseCMapDir(GooList *tokens, GooString *fileName, int line) {
  GooString *collection, *dir;
  GooList *list;

  if (tokens->getLength() != 3) {
    error(-1, "Bad 'cMapDir' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  collection = (GooString *)tokens->get(1);
  dir = (GooString *)tokens->get(2);
  if (!(list = (GooList *)cMapDirs->lookup(collection))) {
    list = new GooList();
    cMapDirs->add(collection->copy(), list);
  }
  list->append(dir->copy());
}

void GlobalParamsGUI::parseToUnicodeDir(GooList *tokens, GooString *fileName,
				     int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad 'toUnicodeDir' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  toUnicodeDirs->append(((GooString *)tokens->get(1))->copy());
}

void GlobalParamsGUI::parseDisplayFont(GooList *tokens, GooHash *fontHash,
				    DisplayFontParamKind kind,
				    GooString *fileName, int line) {
  DisplayFontParam *param, *old;

  if (tokens->getLength() < 2) {
    goto err1;
  }
  param = new DisplayFontParam(((GooString *)tokens->get(1))->copy(), kind);

  switch (kind) {
  case displayFontT1:
    if (tokens->getLength() != 3) {
      goto err2;
    }
    param->t1.fileName = ((GooString *)tokens->get(2))->copy();
    break;
  case displayFontTT:
    if (tokens->getLength() != 3) {
      goto err2;
    }
    param->tt.fileName = ((GooString *)tokens->get(2))->copy();
    break;
  }

  if ((old = (DisplayFontParam *)fontHash->remove(param->name))) {
    delete old;
  }
  fontHash->add(param->name, param);
  return;

 err2:
  delete param;
 err1:
  error(-1, "Bad 'display*Font*' config file command (%s:%d)",
	fileName->getCString(), line);
}

void GlobalParamsGUI::parsePSPaperSize(GooList *tokens, GooString *fileName,
				    int line) {
  GooString *tok;

  if (tokens->getLength() == 2) {
    tok = (GooString *)tokens->get(1);
    if (!setPSPaperSize(tok->getCString())) {
      error(-1, "Bad 'psPaperSize' config file command (%s:%d)",
	    fileName->getCString(), line);
    }
  } else if (tokens->getLength() == 3) {
    tok = (GooString *)tokens->get(1);
    psPaperWidth = atoi(tok->getCString());
    tok = (GooString *)tokens->get(2);
    psPaperHeight = atoi(tok->getCString());
    psImageableLLX = psImageableLLY = 0;
    psImageableURX = psPaperWidth;
    psImageableURY = psPaperHeight;
  } else {
    error(-1, "Bad 'psPaperSize' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParamsGUI::parsePSImageableArea(GooList *tokens, GooString *fileName,
					int line) {
  if (tokens->getLength() != 5) {
    error(-1, "Bad 'psImageableArea' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  psImageableLLX = atoi(((GooString *)tokens->get(1))->getCString());
  psImageableLLY = atoi(((GooString *)tokens->get(2))->getCString());
  psImageableURX = atoi(((GooString *)tokens->get(3))->getCString());
  psImageableURY = atoi(((GooString *)tokens->get(4))->getCString());
}

void GlobalParamsGUI::parsePSLevel(GooList *tokens, GooString *fileName, int line) {
  GooString *tok;

  if (tokens->getLength() != 2) {
    error(-1, "Bad 'psLevel' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (!tok->cmp("level1")) {
    psLevel = psLevel1;
  } else if (!tok->cmp("level1sep")) {
    psLevel = psLevel1Sep;
  } else if (!tok->cmp("level2")) {
    psLevel = psLevel2;
  } else if (!tok->cmp("level2sep")) {
    psLevel = psLevel2Sep;
  } else if (!tok->cmp("level3")) {
    psLevel = psLevel3;
  } else if (!tok->cmp("level3Sep")) {
    psLevel = psLevel3Sep;
  } else {
    error(-1, "Bad 'psLevel' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParamsGUI::parsePSFile(GooList *tokens, GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad 'psFile' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  if (psFile) {
    delete psFile;
  }
  psFile = ((GooString *)tokens->get(1))->copy();
}

void GlobalParamsGUI::parsePSFont(GooList *tokens, GooString *fileName, int line) {
  PSFontParam *param;

  if (tokens->getLength() != 3) {
    error(-1, "Bad 'psFont' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  param = new PSFontParam(((GooString *)tokens->get(1))->copy(), 0,
			  ((GooString *)tokens->get(2))->copy(), NULL);
  psFonts->add(param->pdfFontName, param);
}

void GlobalParamsGUI::parsePSFont16(char *cmdName, GooList *fontList,
				 GooList *tokens, GooString *fileName, int line) {
  PSFontParam *param;
  int wMode;
  GooString *tok;

  if (tokens->getLength() != 5) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(2);
  if (!tok->cmp("H")) {
    wMode = 0;
  } else if (!tok->cmp("V")) {
    wMode = 1;
  } else {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  param = new PSFontParam(((GooString *)tokens->get(1))->copy(),
			  wMode,
			  ((GooString *)tokens->get(3))->copy(),
			  ((GooString *)tokens->get(4))->copy());
  fontList->append(param);
}

void GlobalParamsGUI::parseTextEncoding(GooList *tokens, GooString *fileName,
				     int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad 'textEncoding' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  delete textEncoding;
  textEncoding = ((GooString *)tokens->get(1))->copy();
}

void GlobalParamsGUI::parseTextEOL(GooList *tokens, GooString *fileName, int line) {
  GooString *tok;

  if (tokens->getLength() != 2) {
    error(-1, "Bad 'textEOL' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (!tok->cmp("unix")) {
    textEOL = eolUnix;
  } else if (!tok->cmp("dos")) {
    textEOL = eolDOS;
  } else if (!tok->cmp("mac")) {
    textEOL = eolMac;
  } else {
    error(-1, "Bad 'textEOL' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParamsGUI::parseFontDir(GooList *tokens, GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad 'fontDir' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  fontDirs->append(((GooString *)tokens->get(1))->copy());
}

void GlobalParamsGUI::parseInitialZoom(GooList *tokens,
				    GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad 'initialZoom' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  delete initialZoom;
  initialZoom = ((GooString *)tokens->get(1))->copy();
}

void GlobalParamsGUI::parseScreenType(GooList *tokens, GooString *fileName,
				   int line) {
  GooString *tok;

  if (tokens->getLength() != 2) {
    error(-1, "Bad 'screenType' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (!tok->cmp("dispersed")) {
    screenType = screenDispersed;
  } else if (!tok->cmp("clustered")) {
    screenType = screenClustered;
  } else if (!tok->cmp("stochasticClustered")) {
    screenType = screenStochasticClustered;
  } else {
    error(-1, "Bad 'screenType' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParamsGUI::parseBind(GooList *tokens, GooString *fileName, int line) {
  KeyBinding *binding;
  GooList *cmds;
  int code, mods, context, i;

  if (tokens->getLength() < 4) {
    error(-1, "Bad 'bind' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  if (!parseKey((GooString *)tokens->get(1), (GooString *)tokens->get(2),
		&code, &mods, &context,
		"bind", tokens, fileName, line)) {
    return;
  }
  for (i = 0; i < keyBindings->getLength(); ++i) {
    binding = (KeyBinding *)keyBindings->get(i);
    if (binding->code == code &&
	binding->mods == mods &&
	binding->context == context) {
      delete (KeyBinding *)keyBindings->del(i);
      break;
    }
  }
  cmds = new GooList();
  for (i = 3; i < tokens->getLength(); ++i) {
    cmds->append(((GooString *)tokens->get(i))->copy());
  }
  keyBindings->append(new KeyBinding(code, mods, context, cmds));
}

void GlobalParamsGUI::parseUnbind(GooList *tokens, GooString *fileName, int line) {
  KeyBinding *binding;
  int code, mods, context, i;

  if (tokens->getLength() != 3) {
    error(-1, "Bad 'unbind' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  if (!parseKey((GooString *)tokens->get(1), (GooString *)tokens->get(2),
		&code, &mods, &context,
		"unbind", tokens, fileName, line)) {
    return;
  }
  for (i = 0; i < keyBindings->getLength(); ++i) {
    binding = (KeyBinding *)keyBindings->get(i);
    if (binding->code == code &&
	binding->mods == mods &&
	binding->context == context) {
      delete (KeyBinding *)keyBindings->del(i);
      break;
    }
  }
}

GBool GlobalParamsGUI::parseKey(GooString *modKeyStr, GooString *contextStr,
			     int *code, int *mods, int *context,
			     char *cmdName,
			     GooList *tokens, GooString *fileName, int line) {
  char *p0;

  *mods = xpdfKeyModNone;
  p0 = modKeyStr->getCString();
  while (1) {
    if (!strncmp(p0, "shift-", 6)) {
      *mods |= xpdfKeyModShift;
      p0 += 6;
    } else if (!strncmp(p0, "ctrl-", 5)) {
      *mods |= xpdfKeyModCtrl;
      p0 += 5;
    } else if (!strncmp(p0, "alt-", 4)) {
      *mods |= xpdfKeyModAlt;
      p0 += 4;
    } else {
      break;
    }
  }

  if (!strcmp(p0, "space")) {
    *code = ' ';
  } else if (!strcmp(p0, "tab")) {
    *code = xpdfKeyCodeTab;
  } else if (!strcmp(p0, "return")) {
    *code = xpdfKeyCodeReturn;
  } else if (!strcmp(p0, "enter")) {
    *code = xpdfKeyCodeEnter;
  } else if (!strcmp(p0, "backspace")) {
    *code = xpdfKeyCodeBackspace;
  } else if (!strcmp(p0, "insert")) {
    *code = xpdfKeyCodeInsert;
  } else if (!strcmp(p0, "delete")) {
    *code = xpdfKeyCodeDelete;
  } else if (!strcmp(p0, "home")) {
    *code = xpdfKeyCodeHome;
  } else if (!strcmp(p0, "end")) {
    *code = xpdfKeyCodeEnd;
  } else if (!strcmp(p0, "pgup")) {
    *code = xpdfKeyCodePgUp;
  } else if (!strcmp(p0, "pgdn")) {
    *code = xpdfKeyCodePgDn;
  } else if (!strcmp(p0, "left")) {
    *code = xpdfKeyCodeLeft;
  } else if (!strcmp(p0, "right")) {
    *code = xpdfKeyCodeRight;
  } else if (!strcmp(p0, "up")) {
    *code = xpdfKeyCodeUp;
  } else if (!strcmp(p0, "down")) {
    *code = xpdfKeyCodeDown;
  } else if (p0[0] == 'f' && p0[1] >= '1' && p0[1] <= '9' && !p0[2]) {
    *code = xpdfKeyCodeF1 + (p0[1] - '1');
  } else if (p0[0] == 'f' &&
	     ((p0[1] >= '1' && p0[1] <= '2' && p0[2] >= '0' && p0[2] <= '9') ||
	      (p0[1] == '3' && p0[2] >= '0' && p0[2] <= '5')) &&
	     !p0[3]) {
    *code = xpdfKeyCodeF1 + 10 * (p0[1] - '0') + (p0[2] - '0') - 1;
  } else if (!strncmp(p0, "mousePress", 10) &&
	     p0[10] >= '1' && p0[10] <= '7' && !p0[11]) {
    *code = xpdfKeyCodeMousePress1 + (p0[10] - '1');
  } else if (!strncmp(p0, "mouseRelease", 12) &&
	     p0[12] >= '1' && p0[12] <= '7' && !p0[13]) {
    *code = xpdfKeyCodeMouseRelease1 + (p0[12] - '1');
  } else if (*p0 >= 0x20 && *p0 <= 0x7e && !p0[1]) {
    *code = (int)*p0;
  } else {
    error(-1, "Bad key/modifier in '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return gFalse;
  }

  p0 = contextStr->getCString();
  if (!strcmp(p0, "any")) {
    *context = xpdfKeyContextAny;
  } else {
    *context = xpdfKeyContextAny;
    while (1) {
      if (!strncmp(p0, "fullScreen", 10)) {
	*context |= xpdfKeyContextFullScreen;
	p0 += 10;
      } else if (!strncmp(p0, "window", 6)) {
	*context |= xpdfKeyContextWindow;
	p0 += 6;
      } else if (!strncmp(p0, "continuous", 10)) {
	*context |= xpdfKeyContextContinuous;
	p0 += 10;
      } else if (!strncmp(p0, "singlePage", 10)) {
	*context |= xpdfKeyContextSinglePage;
	p0 += 10;
      } else if (!strncmp(p0, "overLink", 8)) {
	*context |= xpdfKeyContextOverLink;
	p0 += 8;
      } else if (!strncmp(p0, "offLink", 7)) {
	*context |= xpdfKeyContextOffLink;
	p0 += 7;
      } else if (!strncmp(p0, "outline", 7)) {
	*context |= xpdfKeyContextOutline;
	p0 += 7;
      } else if (!strncmp(p0, "mainWin", 7)) {
	*context |= xpdfKeyContextMainWin;
	p0 += 7;
      } else if (!strncmp(p0, "scrLockOn", 9)) {
	*context |= xpdfKeyContextScrLockOn;
	p0 += 9;
      } else if (!strncmp(p0, "scrLockOff", 10)) {
	*context |= xpdfKeyContextScrLockOff;
	p0 += 10;
      } else {
	error(-1, "Bad context in '%s' config file command (%s:%d)",
	      cmdName, fileName->getCString(), line);
	return gFalse;
      }
      if (!*p0) {
	break;
      }
      if (*p0 != ',') {
	error(-1, "Bad context in '%s' config file command (%s:%d)",
	      cmdName, fileName->getCString(), line);
	return gFalse;
      }
      ++p0;
    }
  }

  return gTrue;
}

void GlobalParamsGUI::parseCommand(char *cmdName, GooString **val,
				GooList *tokens, GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  if (*val) {
    delete *val;
  }
  *val = ((GooString *)tokens->get(1))->copy();
}

void GlobalParamsGUI::parseYesNo(char *cmdName, GBool *flag,
			      GooList *tokens, GooString *fileName, int line) {
  GooString *tok;

  if (tokens->getLength() != 2) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (!parseYesNo2(tok->getCString(), flag)) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
  }
}

GBool GlobalParamsGUI::parseYesNo2(char *token, GBool *flag) {
  if (!strcmp(token, "yes")) {
    *flag = gTrue;
  } else if (!strcmp(token, "no")) {
    *flag = gFalse;
  } else {
    return gFalse;
  }
  return gTrue;
}

void GlobalParamsGUI::parseInteger(char *cmdName, int *val,
				GooList *tokens, GooString *fileName, int line) {
  GooString *tok;
  int i;

  if (tokens->getLength() != 2) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (tok->getLength() == 0) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  if (tok->getChar(0) == '-') {
    i = 1;
  } else {
    i = 0;
  }
  for (; i < tok->getLength(); ++i) {
    if (tok->getChar(i) < '0' || tok->getChar(i) > '9') {
      error(-1, "Bad '%s' config file command (%s:%d)",
	    cmdName, fileName->getCString(), line);
      return;
    }
  }
  *val = atoi(tok->getCString());
}

void GlobalParamsGUI::parseFloat(char *cmdName, double *val,
			      GooList *tokens, GooString *fileName, int line) {
  GooString *tok;
  int i;

  if (tokens->getLength() != 2) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (tok->getLength() == 0) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  if (tok->getChar(0) == '-') {
    i = 1;
  } else {
    i = 0;
  }
  for (; i < tok->getLength(); ++i) {
    if (!((tok->getChar(i) >= '0' && tok->getChar(i) <= '9') ||
	  tok->getChar(i) == '.')) {
      error(-1, "Bad '%s' config file command (%s:%d)",
	    cmdName, fileName->getCString(), line);
      return;
    }
  }
  *val = atof(tok->getCString());
}

void GlobalParamsGUI::addCIDToUnicode(GooString *collection,
				   GooString *fileName) {
  GooString *old;

  if ((old = (GooString *)cidToUnicodes->remove(collection))) {
    delete old;
  }
  cidToUnicodes->add(collection->copy(), fileName->copy());
}

void GlobalParamsGUI::addUnicodeMap(GooString *encodingName, GooString *fileName)
{
  GooString *old;

  if ((old = (GooString *)unicodeMaps->remove(encodingName))) {
    delete old;
  }
  unicodeMaps->add(encodingName->copy(), fileName->copy());
}

void GlobalParamsGUI::addCMapDir(GooString *collection, GooString *dir) {
  GooList *list;

  if (!(list = (GooList *)cMapDirs->lookup(collection))) {
    list = new GooList();
    cMapDirs->add(collection->copy(), list);
  }
  list->append(dir->copy());
}

GlobalParamsGUI::~GlobalParamsGUI() {
  GooHashIter *iter;
  GooString *key;
  GooList *list;

  delete macRomanReverseMap;

  delete baseDir;
  delete nameToUnicode;
  deleteGooHash(cidToUnicodes, GooString);
  deleteGooHash(unicodeToUnicodes, GooString);
  deleteGooHash(residentUnicodeMaps, UnicodeMap);
  deleteGooHash(unicodeMaps, GooString);
  deleteGooList(toUnicodeDirs, GooString);
  deleteGooHash(displayFonts, DisplayFontParam);
  deleteGooHash(displayCIDFonts, DisplayFontParam);
  deleteGooHash(displayNamedCIDFonts, DisplayFontParam);
#ifdef WIN32
  if (winFontList) {
    delete winFontList;
  }
#endif
  if (psFile) {
    delete psFile;
  }
  deleteGooHash(psFonts, PSFontParam);
  deleteGooList(psNamedFonts16, PSFontParam);
  deleteGooList(psFonts16, PSFontParam);
  delete textEncoding;
  deleteGooList(fontDirs, GooString);

  cMapDirs->startIter(&iter);
  while (cMapDirs->getNext(&iter, &key, (void **)&list)) {
    deleteGooList(list, GooString);
  }
  delete cMapDirs;

  delete cidToUnicodeCache;
  delete unicodeToUnicodeCache;
  delete unicodeMapCache;
  delete cMapCache;

#ifdef ENABLE_PLUGINS
  delete securityHandlers;
  deleteGooList(plugins, Plugin);
#endif

#if MULTITHREADED
  gDestroyMutex(&mutex);
  gDestroyMutex(&unicodeMapCacheMutex);
  gDestroyMutex(&cMapCacheMutex);
#endif
}

//------------------------------------------------------------------------

void GlobalParamsGUI::setBaseDir(char *dir) {
  delete baseDir;
  baseDir = new GooString(dir);
}

void GlobalParamsGUI::setupBaseFonts(char *dir) {
  GooString *fontName;
  GooString *fileName;
#ifdef WIN32
  HMODULE shell32Lib;
  BOOL (__stdcall *SHGetSpecialFolderPathFunc)(HWND hwndOwner,
					       LPTSTR lpszPath,
					       int nFolder,
					       BOOL fCreate);
  char winFontDir[MAX_PATH];
#endif
  FILE *f;
  DisplayFontParamKind kind;
  DisplayFontParam *dfp;
  int i, j;

#ifdef WIN32
  // SHGetSpecialFolderPath isn't available in older versions of
  // shell32.dll (Win95 and WinNT4), so do a dynamic load
  winFontDir[0] = '\0';
  if ((shell32Lib = LoadLibrary("shell32.dll"))) {
    if ((SHGetSpecialFolderPathFunc =
	 (BOOL (__stdcall *)(HWND hwndOwner, LPTSTR lpszPath,
			     int nFolder, BOOL fCreate))
	 GetProcAddress(shell32Lib, "SHGetSpecialFolderPathA"))) {
      if (!(*SHGetSpecialFolderPathFunc)(NULL, winFontDir,
					 CSIDL_FONTS, FALSE)) {
	winFontDir[0] = '\0';
      }
    }
  }
#endif
  for (i = 0; displayFontTab[i].name; ++i) {
    fontName = new GooString(displayFontTab[i].name);
    if (getDisplayFont(fontName)) {
      delete fontName;
      continue;
    }
    fileName = NULL;
    kind = displayFontT1; // make gcc happy
    if (dir) {
      fileName = appendToPath(new GooString(dir), displayFontTab[i].t1FileName);
      kind = displayFontT1;
      if ((f = fopen(fileName->getCString(), "rb"))) {
	fclose(f);
      } else {
	delete fileName;
	fileName = NULL;
      }
    }
#ifdef WIN32
    if (!fileName && winFontDir[0] && displayFontTab[i].ttFileName) {
      fileName = appendToPath(new GooString(winFontDir),
			      displayFontTab[i].ttFileName);
      kind = displayFontTT;
      if ((f = fopen(fileName->getCString(), "rb"))) {
	fclose(f);
      } else {
	delete fileName;
	fileName = NULL;
      }
    }
    // SHGetSpecialFolderPath(CSIDL_FONTS) doesn't work on Win 2k Server
    // or Win2003 Server, or with older versions of shell32.dll, so check
    // the "standard" directories
    if (displayFontTab[i].ttFileName) {
      for (j = 0; !fileName && displayFontDirs[j]; ++j) {
	fileName = appendToPath(new GooString(displayFontDirs[j]),
				displayFontTab[i].ttFileName);
	kind = displayFontTT;
	if ((f = fopen(fileName->getCString(), "rb"))) {
	  fclose(f);
	} else {
	  delete fileName;
	  fileName = NULL;
	}
      }
    }
#else
    for (j = 0; !fileName && displayFontDirs[j]; ++j) {
      fileName = appendToPath(new GooString(displayFontDirs[j]),
			      displayFontTab[i].t1FileName);
      kind = displayFontT1;
      if ((f = fopen(fileName->getCString(), "rb"))) {
	fclose(f);
      } else {
	delete fileName;
	fileName = NULL;
      }
    }
#endif
    if (!fileName) {
      error(-1, "No display font for '%s'", displayFontTab[i].name);
      delete fontName;
      continue;
    }
    dfp = new DisplayFontParam(fontName, kind);
    dfp->t1.fileName = fileName;
    globalParamsGUI->addDisplayFont(dfp);
  }

#ifdef WIN32
  if (winFontDir[0]) {
    winFontList = new WinFontList(winFontDir);
  }
#endif
}

//------------------------------------------------------------------------
// accessors
//------------------------------------------------------------------------

CharCode GlobalParamsGUI::getMacRomanCharCode(char *charName) {
  // no need to lock - macRomanReverseMap is constant
  return macRomanReverseMap->lookup(charName);
}

GooString *GlobalParamsGUI::getBaseDir() {
  GooString *s;

  lockGlobalParamsGUI;
  s = baseDir->copy();
  unlockGlobalParamsGUI;
  return s;
}

Unicode GlobalParamsGUI::mapNameToUnicode(char *charName) {
  // no need to lock - nameToUnicode is constant
  return nameToUnicode->lookup(charName);
}

UnicodeMap *GlobalParamsGUI::getResidentUnicodeMap(GooString *encodingName) {
  UnicodeMap *map;

  lockGlobalParamsGUI;
  map = (UnicodeMap *)residentUnicodeMaps->lookup(encodingName);
  unlockGlobalParamsGUI;
  if (map) {
    map->incRefCnt();
  }
  return map;
}

FILE *GlobalParamsGUI::getUnicodeMapFile(GooString *encodingName) {
  GooString *fileName;
  FILE *f;

  lockGlobalParamsGUI;
  if ((fileName = (GooString *)unicodeMaps->lookup(encodingName))) {
    f = fopen(fileName->getCString(), "r");
  } else {
    f = NULL;
  }
  unlockGlobalParamsGUI;
  return f;
}

FILE *GlobalParamsGUI::findCMapFile(GooString *collection, GooString *cMapName) {
  GooList *list;
  GooString *dir;
  GooString *fileName;
  FILE *f;
  int i;

  lockGlobalParamsGUI;
  if (!(list = (GooList *)cMapDirs->lookup(collection))) {
    unlockGlobalParamsGUI;
    return NULL;
  }
  for (i = 0; i < list->getLength(); ++i) {
    dir = (GooString *)list->get(i);
    fileName = appendToPath(dir->copy(), cMapName->getCString());
    f = fopen(fileName->getCString(), "r");
    delete fileName;
    if (f) {
      unlockGlobalParamsGUI;
      return f;
    }
  }
  unlockGlobalParamsGUI;
  return NULL;
}

FILE *GlobalParamsGUI::findToUnicodeFile(GooString *name) {
  GooString *dir, *fileName;
  FILE *f;
  int i;

  lockGlobalParamsGUI;
  for (i = 0; i < toUnicodeDirs->getLength(); ++i) {
    dir = (GooString *)toUnicodeDirs->get(i);
    fileName = appendToPath(dir->copy(), name->getCString());
    f = fopen(fileName->getCString(), "r");
    delete fileName;
    if (f) {
      unlockGlobalParamsGUI;
      return f;
    }
  }
  unlockGlobalParamsGUI;
  return NULL;
}

GBool findModifier(const char *name, const char *modifier, const char **start)
{
  const char *match;

  if (name == NULL)
    return gFalse;

  match = strstr(name, modifier);
  if (match) {
    if (*start == NULL || match < *start)
      *start = match;
    return gTrue;
  }
  else {
    return gFalse;
  }
}

static FcPattern *buildFcPattern(GfxFont *font)
{
  int weight = FC_WEIGHT_NORMAL,
      slant = FC_SLANT_ROMAN,
      width = FC_WIDTH_NORMAL,
      spacing = FC_PROPORTIONAL;
  bool deleteFamily = false;
  char *family, *name, *lang, *modifiers;
  const char *start;
  FcPattern *p;

  // this is all heuristics will be overwritten if font had proper info
  name = font->getName()->getCString();

  modifiers = strchr (name, ',');
  if (modifiers == NULL)
    modifiers = strchr (name, '-');

  // remove the - from the names, for some reason, Fontconfig does not
  // understand "MS-Mincho" but does with "MS Mincho"
  int len = strlen(name);
  for (int i = 0; i < len; i++)
    name[i] = (name[i] == '-' ? ' ' : name[i]);

  start = NULL;
  findModifier(modifiers, "Regular", &start);
  findModifier(modifiers, "Roman", &start);

  if (findModifier(modifiers, "Oblique", &start))
    slant = FC_SLANT_OBLIQUE;
  if (findModifier(modifiers, "Italic", &start))
    slant = FC_SLANT_ITALIC;
  if (findModifier(modifiers, "Bold", &start))
    weight = FC_WEIGHT_BOLD;
  if (findModifier(modifiers, "Light", &start))
    weight = FC_WEIGHT_LIGHT;
  if (findModifier(modifiers, "Condensed", &start))
    width = FC_WIDTH_CONDENSED;

  if (start) {
    // There have been "modifiers" in the name, crop them to obtain
    // the family name
    family = new char[len+1];
    strcpy(family, name);
    int pos = (modifiers - name);
    family[pos] = '\0';
    deleteFamily = true;
  }
  else {
    family = name;
  }

  // use font flags
  if (font->isFixedWidth())
    spacing = FC_MONO;
  if (font->isBold())
    weight = FC_WEIGHT_BOLD;
  if (font->isItalic())
    slant = FC_SLANT_ITALIC;

  // if the FontDescriptor specified a family name use it
  if (font->getFamily()) {
    if (deleteFamily) {
      delete[] family;
      deleteFamily = false;
    }
    family = font->getFamily()->getCString();
  }

  // if the FontDescriptor specified a weight use it
  switch (font -> getWeight())
  {
    case GfxFont::W100: weight = FC_WEIGHT_EXTRALIGHT; break;
    case GfxFont::W200: weight = FC_WEIGHT_LIGHT; break;
    case GfxFont::W300: weight = FC_WEIGHT_BOOK; break;
    case GfxFont::W400: weight = FC_WEIGHT_NORMAL; break;
    case GfxFont::W500: weight = FC_WEIGHT_MEDIUM; break;
    case GfxFont::W600: weight = FC_WEIGHT_DEMIBOLD; break;
    case GfxFont::W700: weight = FC_WEIGHT_BOLD; break;
    case GfxFont::W800: weight = FC_WEIGHT_EXTRABOLD; break;
    case GfxFont::W900: weight = FC_WEIGHT_BLACK; break;
    default: break;
  }

  // if the FontDescriptor specified a width use it
  switch (font -> getStretch())
  {
    case GfxFont::UltraCondensed: width = FC_WIDTH_ULTRACONDENSED; break;
    case GfxFont::ExtraCondensed: width = FC_WIDTH_EXTRACONDENSED; break;
    case GfxFont::Condensed: width = FC_WIDTH_CONDENSED; break;
    case GfxFont::SemiCondensed: width = FC_WIDTH_SEMICONDENSED; break;
    case GfxFont::Normal: width = FC_WIDTH_NORMAL; break;
    case GfxFont::SemiExpanded: width = FC_WIDTH_SEMIEXPANDED; break;
    case GfxFont::Expanded: width = FC_WIDTH_EXPANDED; break;
    case GfxFont::ExtraExpanded: width = FC_WIDTH_EXTRAEXPANDED; break;
    case GfxFont::UltraExpanded: width = FC_WIDTH_ULTRAEXPANDED; break;
    default: break;
  }

  // find the language we want the font to support
  if (font->isCIDFont())
  {
    GooString *collection = ((GfxCIDFont *)font)->getCollection();
    if (collection)
    {
      if (strcmp(collection->getCString(), "Adobe-GB1") == 0)
        lang = "zh-cn"; // Simplified Chinese
      else if (strcmp(collection->getCString(), "Adobe-CNS1") == 0)
        lang = "zh-tw"; // Traditional Chinese
      else if (strcmp(collection->getCString(), "Adobe-Japan1") == 0)
        lang = "ja"; // Japanese
      else if (strcmp(collection->getCString(), "Adobe-Japan2") == 0)
        lang = "ja"; // Japanese
      else if (strcmp(collection->getCString(), "Adobe-Korea1") == 0)
        lang = "ko"; // Korean
      else if (strcmp(collection->getCString(), "Adobe-UCS") == 0)
        lang = "xx";
      else if (strcmp(collection->getCString(), "Adobe-Identity") == 0)
        lang = "xx";
      else
      {
        error(-1, "Unknown CID font collection, please report to poppler bugzilla.");
        lang = "xx";
      }
    }
    else lang = "xx";
  }
  else lang = "xx";

  p = FcPatternBuild(NULL,
                    FC_FAMILY, FcTypeString, family,
                    FC_SLANT, FcTypeInteger, slant,
                    FC_WEIGHT, FcTypeInteger, weight,
                    FC_WIDTH, FcTypeInteger, width,
                    FC_SPACING, FcTypeInteger, spacing,
                    FC_LANG, FcTypeString, lang,
                    NULL);
  if (deleteFamily)
    delete[] family;
  return p;
}

DisplayFontParam *GlobalParamsGUI::getDisplayFont(GfxFont *font) {
  DisplayFontParam *dfp;
  FcPattern *p=0;

  GooString *fontName = font->getName();
  if (!fontName) return NULL;

  lockGlobalParamsGUI;
  dfp = font->dfp;
  if (!dfp)
  {
    FcChar8* s;
    char * ext;
    FcResult res;
    FcFontSet *set;
    int i;
    p = buildFcPattern(font);

    if (!p)
      goto fin;
    FcConfigSubstitute(FCcfg, p, FcMatchPattern);
    FcDefaultSubstitute(p);
    set = FcFontSort(FCcfg, p, FcFalse, NULL, &res);
    if (!set)
      goto fin;
    for (i = 0; i < set->nfont; ++i)
    {
      res = FcPatternGetString(set->fonts[i], FC_FILE, 0, &s);
      if (res != FcResultMatch || !s)
        continue;
      ext = strrchr((char*)s,'.');
      if (!ext)
        continue;
      if (!strncasecmp(ext,".ttf",4) || !strncasecmp(ext, ".ttc", 4))
      {
        dfp = new DisplayFontParam(fontName->copy(), displayFontTT);
        dfp->tt.fileName = new GooString((char*)s);
        FcPatternGetInteger(set->fonts[i], FC_INDEX, 0, &(dfp->tt.faceIndex));
      }
      else if (!strncasecmp(ext,".pfa",4) || !strncasecmp(ext,".pfb",4))
      {
        dfp = new DisplayFontParam(fontName->copy(), displayFontT1);
        dfp->t1.fileName = new GooString((char*)s);
      }
      else
        continue;
      font->dfp = dfp;
      break;
    }
    FcFontSetDestroy(set);
  }
fin:
  if (p)
    FcPatternDestroy(p);

  unlockGlobalParamsGUI;
  return dfp;
}

DisplayFontParam *GlobalParamsGUI::getDisplayFont(GooString *fontName) {
  DisplayFontParam *dfp;

  lockGlobalParamsGUI;
  dfp = (DisplayFontParam *)displayFonts->lookup(fontName);
#ifdef WIN32
  if (!dfp && winFontList) {
    dfp = winFontList->find(fontName);
  }
#endif
  unlockGlobalParamsGUI;
  return dfp;
}

DisplayFontParam *GlobalParamsGUI::getDisplayCIDFont(GooString *fontName,
						  GooString *collection) {
  DisplayFontParam *dfp;

  lockGlobalParamsGUI;
  if (!fontName ||
      !(dfp = (DisplayFontParam *)displayNamedCIDFonts->lookup(fontName))) {
    dfp = (DisplayFontParam *)displayCIDFonts->lookup(collection);
  }
  unlockGlobalParamsGUI;
  return dfp;
}

GooString *GlobalParamsGUI::getPSFile() {
  GooString *s;

  lockGlobalParamsGUI;
  s = psFile ? psFile->copy() : (GooString *)NULL;
  unlockGlobalParamsGUI;
  return s;
}

int GlobalParamsGUI::getPSPaperWidth() {
  int w;

  lockGlobalParamsGUI;
  w = psPaperWidth;
  unlockGlobalParamsGUI;
  return w;
}

int GlobalParamsGUI::getPSPaperHeight() {
  int h;

  lockGlobalParamsGUI;
  h = psPaperHeight;
  unlockGlobalParamsGUI;
  return h;
}

void GlobalParamsGUI::getPSImageableArea(int *llx, int *lly, int *urx, int *ury) {
  lockGlobalParamsGUI;
  *llx = psImageableLLX;
  *lly = psImageableLLY;
  *urx = psImageableURX;
  *ury = psImageableURY;
  unlockGlobalParamsGUI;
}

GBool GlobalParamsGUI::getPSCrop() {
  GBool f;

  lockGlobalParamsGUI;
  f = psCrop;
  unlockGlobalParamsGUI;
  return f;
}

GBool GlobalParamsGUI::getPSExpandSmaller() {
  GBool f;

  lockGlobalParamsGUI;
  f = psExpandSmaller;
  unlockGlobalParamsGUI;
  return f;
}

GBool GlobalParamsGUI::getPSShrinkLarger() {
  GBool f;

  lockGlobalParamsGUI;
  f = psShrinkLarger;
  unlockGlobalParamsGUI;
  return f;
}

GBool GlobalParamsGUI::getPSCenter() {
  GBool f;

  lockGlobalParamsGUI;
  f = psCenter;
  unlockGlobalParamsGUI;
  return f;
}

GBool GlobalParamsGUI::getPSDuplex() {
  GBool d;

  lockGlobalParamsGUI;
  d = psDuplex;
  unlockGlobalParamsGUI;
  return d;
}

PSLevel GlobalParamsGUI::getPSLevel() {
  PSLevel level;

  lockGlobalParamsGUI;
  level = psLevel;
  unlockGlobalParamsGUI;
  return level;
}

PSFontParam *GlobalParamsGUI::getPSFont(GooString *fontName) {
  PSFontParam *p;

  lockGlobalParamsGUI;
  p = (PSFontParam *)psFonts->lookup(fontName);
  unlockGlobalParamsGUI;
  return p;
}

PSFontParam *GlobalParamsGUI::getPSFont16(GooString *fontName,
				       GooString *collection, int wMode) {
  PSFontParam *p;
  int i;

  lockGlobalParamsGUI;
  p = NULL;
  if (fontName) {
    for (i = 0; i < psNamedFonts16->getLength(); ++i) {
      p = (PSFontParam *)psNamedFonts16->get(i);
      if (!p->pdfFontName->cmp(fontName) &&
	  p->wMode == wMode) {
	break;
      }
      p = NULL;
    }
  }
  if (!p && collection) {
    for (i = 0; i < psFonts16->getLength(); ++i) {
      p = (PSFontParam *)psFonts16->get(i);
      if (!p->pdfFontName->cmp(collection) &&
	  p->wMode == wMode) {
	break;
      }
      p = NULL;
    }
  }
  unlockGlobalParamsGUI;
  return p;
}

GBool GlobalParamsGUI::getPSEmbedType1() {
  GBool e;

  lockGlobalParamsGUI;
  e = psEmbedType1;
  unlockGlobalParamsGUI;
  return e;
}

GBool GlobalParamsGUI::getPSEmbedTrueType() {
  GBool e;

  lockGlobalParamsGUI;
  e = psEmbedTrueType;
  unlockGlobalParamsGUI;
  return e;
}

GBool GlobalParamsGUI::getPSEmbedCIDPostScript() {
  GBool e;

  lockGlobalParamsGUI;
  e = psEmbedCIDPostScript;
  unlockGlobalParamsGUI;
  return e;
}

GBool GlobalParamsGUI::getPSEmbedCIDTrueType() {
  GBool e;

  lockGlobalParamsGUI;
  e = psEmbedCIDTrueType;
  unlockGlobalParamsGUI;
  return e;
}

GBool GlobalParamsGUI::getPSPreload() {
  GBool preload;

  lockGlobalParamsGUI;
  preload = psPreload;
  unlockGlobalParamsGUI;
  return preload;
}

GBool GlobalParamsGUI::getPSOPI() {
  GBool opi;

  lockGlobalParamsGUI;
  opi = psOPI;
  unlockGlobalParamsGUI;
  return opi;
}

GBool GlobalParamsGUI::getPSASCIIHex() {
  GBool ah;

  lockGlobalParamsGUI;
  ah = psASCIIHex;
  unlockGlobalParamsGUI;
  return ah;
}

GooString *GlobalParamsGUI::getTextEncodingName() {
  GooString *s;

  lockGlobalParamsGUI;
  s = textEncoding->copy();
  unlockGlobalParamsGUI;
  return s;
}

EndOfLineKind GlobalParamsGUI::getTextEOL() {
  EndOfLineKind eol;

  lockGlobalParamsGUI;
  eol = textEOL;
  unlockGlobalParamsGUI;
  return eol;
}

GBool GlobalParamsGUI::getTextPageBreaks() {
  GBool pageBreaks;

  lockGlobalParamsGUI;
  pageBreaks = textPageBreaks;
  unlockGlobalParamsGUI;
  return pageBreaks;
}

GBool GlobalParamsGUI::getTextKeepTinyChars() {
  GBool tiny;

  lockGlobalParamsGUI;
  tiny = textKeepTinyChars;
  unlockGlobalParamsGUI;
  return tiny;
}

GooString *GlobalParamsGUI::findFontFile(GooString *fontName, char **exts) {
  GooString *dir, *fileName;
  char **ext;
  FILE *f;
  int i;

  lockGlobalParamsGUI;
  for (i = 0; i < fontDirs->getLength(); ++i) {
    dir = (GooString *)fontDirs->get(i);
    for (ext = exts; *ext; ++ext) {
      fileName = appendToPath(dir->copy(), fontName->getCString());
      fileName->append(*ext);
      if ((f = fopen(fileName->getCString(), "rb"))) {
	fclose(f);
	unlockGlobalParamsGUI;
	return fileName;
      }
      delete fileName;
    }
  }
  unlockGlobalParamsGUI;
  return NULL;
}

GooString *GlobalParamsGUI::getInitialZoom() {
  GooString *s;

  lockGlobalParamsGUI;
  s = initialZoom->copy();
  unlockGlobalParamsGUI;
  return s;
}

GBool GlobalParamsGUI::getContinuousView() {
  GBool f;

  lockGlobalParamsGUI;
  f = continuousView;
  unlockGlobalParamsGUI;
  return f;
}

GBool GlobalParamsGUI::getEnableT1lib() {
  GBool f;

  lockGlobalParamsGUI;
  f = enableT1lib;
  unlockGlobalParamsGUI;
  return f;
}

GBool GlobalParamsGUI::getEnableFreeType() {
  GBool f;

  lockGlobalParamsGUI;
  f = enableFreeType;
  unlockGlobalParamsGUI;
  return f;
}

GBool GlobalParamsGUI::getEnableFreeTypeHinting() {
  GBool f;

  lockGlobalParamsGUI;
  f = enableFreeTypeHinting;
  unlockGlobalParamsGUI;
  return f;
}

GBool GlobalParamsGUI::getAntialias() {
  GBool f;

  lockGlobalParamsGUI;
  f = antialias;
  unlockGlobalParamsGUI;
  return f;
}

GBool GlobalParamsGUI::getVectorAntialias() {
  GBool f;

  lockGlobalParamsGUI;
  f = vectorAntialias;
  unlockGlobalParamsGUI;
  return f;
}

GBool GlobalParamsGUI::getStrokeAdjust() {
  GBool f;

  lockGlobalParamsGUI;
  f = strokeAdjust;
  unlockGlobalParamsGUI;
  return f;
}

ScreenType GlobalParamsGUI::getScreenType() {
  ScreenType t;

  lockGlobalParamsGUI;
  t = screenType;
  unlockGlobalParamsGUI;
  return t;
}

int GlobalParamsGUI::getScreenSize() {
  int size;

  lockGlobalParamsGUI;
  size = screenSize;
  unlockGlobalParamsGUI;
  return size;
}

int GlobalParamsGUI::getScreenDotRadius() {
  int r;

  lockGlobalParamsGUI;
  r = screenDotRadius;
  unlockGlobalParamsGUI;
  return r;
}

double GlobalParamsGUI::getScreenGamma() {
  double gamma;

  lockGlobalParamsGUI;
  gamma = screenGamma;
  unlockGlobalParamsGUI;
  return gamma;
}

double GlobalParamsGUI::getScreenBlackThreshold() {
  double thresh;

  lockGlobalParamsGUI;
  thresh = screenBlackThreshold;
  unlockGlobalParamsGUI;
  return thresh;
}

double GlobalParamsGUI::getScreenWhiteThreshold() {
  double thresh;

  lockGlobalParamsGUI;
  thresh = screenWhiteThreshold;
  unlockGlobalParamsGUI;
  return thresh;
}

GBool GlobalParamsGUI::getMapNumericCharNames() {
  GBool map;

  lockGlobalParamsGUI;
  map = mapNumericCharNames;
  unlockGlobalParamsGUI;
  return map;
}

GBool GlobalParamsGUI::getMapUnknownCharNames() {
  GBool map;

  lockGlobalParamsGUI;
  map = mapUnknownCharNames;
  unlockGlobalParamsGUI;
  return map;
}

GooList *GlobalParamsGUI::getKeyBinding(int code, int mods, int context) {
  KeyBinding *binding;
  GooList *cmds;
  int modMask;
  int i, j;

  lockGlobalParamsGUI;
  cmds = NULL;
  // for ASCII chars, ignore the shift modifier
  modMask = code <= 0xff ? ~xpdfKeyModShift : ~0;
  for (i = 0; i < keyBindings->getLength(); ++i) {
    binding = (KeyBinding *)keyBindings->get(i);
    if (binding->code == code &&
	(binding->mods & modMask) == (mods & modMask) &&
	(~binding->context | context) == ~0) {
      cmds = new GooList();
      for (j = 0; j < binding->cmds->getLength(); ++j) {
	cmds->append(((GooString *)binding->cmds->get(j))->copy());
      }
      break;
    }
  }
  unlockGlobalParamsGUI;
  return cmds;
}

GBool GlobalParamsGUI::getPrintCommands() {
  GBool p;

  lockGlobalParamsGUI;
  p = printCommands;
  unlockGlobalParamsGUI;
  return p;
}

GBool GlobalParamsGUI::getProfileCommands() {
  GBool p;

  lockGlobalParamsGUI;
  p = profileCommands;
  unlockGlobalParamsGUI;
  return p;
}

GBool GlobalParamsGUI::getErrQuiet() {
  // no locking -- this function may get called from inside a locked
  // section
  return errQuiet;
}

CharCodeToUnicode *GlobalParamsGUI::getCIDToUnicode(GooString *collection) {
  GooString *fileName;
  CharCodeToUnicode *ctu;

  lockGlobalParamsGUI;
  if (!(ctu = cidToUnicodeCache->getCharCodeToUnicode(collection))) {
    if ((fileName = (GooString *)cidToUnicodes->lookup(collection)) &&
	(ctu = CharCodeToUnicode::parseCIDToUnicode(fileName, collection))) {
      cidToUnicodeCache->add(ctu);
    }
  }
  unlockGlobalParamsGUI;
  return ctu;
}

CharCodeToUnicode *GlobalParamsGUI::getUnicodeToUnicode(GooString *fontName) {
  CharCodeToUnicode *ctu;
  GooHashIter *iter;
  GooString *fontPattern, *fileName;

  lockGlobalParamsGUI;
  fileName = NULL;
  unicodeToUnicodes->startIter(&iter);
  while (unicodeToUnicodes->getNext(&iter, &fontPattern, (void **)&fileName)) {
    if (strstr(fontName->getCString(), fontPattern->getCString())) {
      unicodeToUnicodes->killIter(&iter);
      break;
    }
    fileName = NULL;
  }
  if (fileName) {
    if (!(ctu = unicodeToUnicodeCache->getCharCodeToUnicode(fileName))) {
      if ((ctu = CharCodeToUnicode::parseUnicodeToUnicode(fileName))) {
	unicodeToUnicodeCache->add(ctu);
      }
    }
  } else {
    ctu = NULL;
  }
  unlockGlobalParamsGUI;
  return ctu;
}

UnicodeMap *GlobalParamsGUI::getUnicodeMap(GooString *encodingName) {
  return getUnicodeMap2(encodingName);
}

UnicodeMap *GlobalParamsGUI::getUnicodeMap2(GooString *encodingName) {
  UnicodeMap *map;

  if (!(map = getResidentUnicodeMap(encodingName))) {
    lockUnicodeMapCache;
    map = unicodeMapCache->getUnicodeMap(encodingName);
    unlockUnicodeMapCache;
  }
  return map;
}

CMap *GlobalParamsGUI::getCMap(GooString *collection, GooString *cMapName, Stream *stream) {
  CMap *cMap;

  lockCMapCache;
  cMap = cMapCache->getCMap(collection, cMapName, stream);
  unlockCMapCache;
  return cMap;
}

UnicodeMap *GlobalParamsGUI::getTextEncoding() {
  return getUnicodeMap2(textEncoding);
}

//------------------------------------------------------------------------
// functions to set parameters
//------------------------------------------------------------------------

void GlobalParamsGUI::addDisplayFont(DisplayFontParam *param) {
  DisplayFontParam *old;

  lockGlobalParamsGUI;
  if ((old = (DisplayFontParam *)displayFonts->remove(param->name))) {
    delete old;
  }
  displayFonts->add(param->name, param);
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSFile(char *file) {
  lockGlobalParamsGUI;
  if (psFile) {
    delete psFile;
  }
  psFile = new GooString(file);
  unlockGlobalParamsGUI;
}

GBool GlobalParamsGUI::setPSPaperSize(char *size) {
  lockGlobalParamsGUI;
  if (!strcmp(size, "match")) {
    psPaperWidth = psPaperHeight = -1;
  } else if (!strcmp(size, "letter")) {
    psPaperWidth = 612;
    psPaperHeight = 792;
  } else if (!strcmp(size, "legal")) {
    psPaperWidth = 612;
    psPaperHeight = 1008;
  } else if (!strcmp(size, "A4")) {
    psPaperWidth = 595;
    psPaperHeight = 842;
  } else if (!strcmp(size, "A3")) {
    psPaperWidth = 842;
    psPaperHeight = 1190;
  } else {
    unlockGlobalParamsGUI;
    return gFalse;
  }
  psImageableLLX = psImageableLLY = 0;
  psImageableURX = psPaperWidth;
  psImageableURY = psPaperHeight;
  unlockGlobalParamsGUI;
  return gTrue;
}

void GlobalParamsGUI::setPSPaperWidth(int width) {
  lockGlobalParamsGUI;
  psPaperWidth = width;
  psImageableLLX = 0;
  psImageableURX = psPaperWidth;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSPaperHeight(int height) {
  lockGlobalParamsGUI;
  psPaperHeight = height;
  psImageableLLY = 0;
  psImageableURY = psPaperHeight;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSImageableArea(int llx, int lly, int urx, int ury) {
  lockGlobalParamsGUI;
  psImageableLLX = llx;
  psImageableLLY = lly;
  psImageableURX = urx;
  psImageableURY = ury;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSCrop(GBool crop) {
  lockGlobalParamsGUI;
  psCrop = crop;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSExpandSmaller(GBool expand) {
  lockGlobalParamsGUI;
  psExpandSmaller = expand;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSShrinkLarger(GBool shrink) {
  lockGlobalParamsGUI;
  psShrinkLarger = shrink;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSCenter(GBool center) {
  lockGlobalParamsGUI;
  psCenter = center;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSDuplex(GBool duplex) {
  lockGlobalParamsGUI;
  psDuplex = duplex;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSLevel(PSLevel level) {
  lockGlobalParamsGUI;
  psLevel = level;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSEmbedType1(GBool embed) {
  lockGlobalParamsGUI;
  psEmbedType1 = embed;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSEmbedTrueType(GBool embed) {
  lockGlobalParamsGUI;
  psEmbedTrueType = embed;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSEmbedCIDPostScript(GBool embed) {
  lockGlobalParamsGUI;
  psEmbedCIDPostScript = embed;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSEmbedCIDTrueType(GBool embed) {
  lockGlobalParamsGUI;
  psEmbedCIDTrueType = embed;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSPreload(GBool preload) {
  lockGlobalParamsGUI;
  psPreload = preload;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSOPI(GBool opi) {
  lockGlobalParamsGUI;
  psOPI = opi;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSASCIIHex(GBool hex) {
  lockGlobalParamsGUI;
  psASCIIHex = hex;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setTextEncoding(char *encodingName) {
  lockGlobalParamsGUI;
  delete textEncoding;
  textEncoding = new GooString(encodingName);
  unlockGlobalParamsGUI;
}

GBool GlobalParamsGUI::setTextEOL(char *s) {
  lockGlobalParamsGUI;
  if (!strcmp(s, "unix")) {
    textEOL = eolUnix;
  } else if (!strcmp(s, "dos")) {
    textEOL = eolDOS;
  } else if (!strcmp(s, "mac")) {
    textEOL = eolMac;
  } else {
    unlockGlobalParamsGUI;
    return gFalse;
  }
  unlockGlobalParamsGUI;
  return gTrue;
}

void GlobalParamsGUI::setTextPageBreaks(GBool pageBreaks) {
  lockGlobalParamsGUI;
  textPageBreaks = pageBreaks;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setTextKeepTinyChars(GBool keep) {
  lockGlobalParamsGUI;
  textKeepTinyChars = keep;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setInitialZoom(char *s) {
  lockGlobalParamsGUI;
  delete initialZoom;
  initialZoom = new GooString(s);
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setContinuousView(GBool cont) {
  lockGlobalParamsGUI;
  continuousView = cont;
  unlockGlobalParamsGUI;
}

GBool GlobalParamsGUI::setEnableT1lib(char *s) {
  GBool ok;

  lockGlobalParamsGUI;
  ok = parseYesNo2(s, &enableT1lib);
  unlockGlobalParamsGUI;
  return ok;
}

GBool GlobalParamsGUI::setEnableFreeType(char *s) {
  GBool ok;

  lockGlobalParamsGUI;
  ok = parseYesNo2(s, &enableFreeType);
  unlockGlobalParamsGUI;
  return ok;
}

GBool GlobalParamsGUI::setEnableFreeTypeHinting(char *s) {
  GBool ok;

  lockGlobalParamsGUI;
  ok = parseYesNo2(s, &enableFreeTypeHinting);
  unlockGlobalParamsGUI;
  return ok;
}

GBool GlobalParamsGUI::setAntialias(char *s) {
  GBool ok;

  lockGlobalParamsGUI;
  ok = parseYesNo2(s, &antialias);
  unlockGlobalParamsGUI;
  return ok;
}

GBool GlobalParamsGUI::setVectorAntialias(char *s) {
  GBool ok;

  lockGlobalParamsGUI;
  ok = parseYesNo2(s, &vectorAntialias);
  unlockGlobalParamsGUI;
  return ok;
}

void GlobalParamsGUI::setStrokeAdjust(GBool adjust)
{
  lockGlobalParamsGUI;
  strokeAdjust = adjust;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setScreenType(ScreenType st)
{
  lockGlobalParamsGUI;
  screenType = st;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setScreenSize(int size)
{
  lockGlobalParamsGUI;
  screenSize = size;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setScreenDotRadius(int radius)
{
  lockGlobalParamsGUI;
  screenDotRadius = radius;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setScreenGamma(double gamma)
{
  lockGlobalParamsGUI;
  screenGamma = gamma;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setScreenBlackThreshold(double blackThreshold)
{
  lockGlobalParamsGUI;
  screenBlackThreshold = blackThreshold;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setScreenWhiteThreshold(double whiteThreshold)
{
  lockGlobalParamsGUI;
  screenWhiteThreshold = whiteThreshold;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setMapNumericCharNames(GBool map) {
  lockGlobalParamsGUI;
  mapNumericCharNames = map;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setMapUnknownCharNames(GBool map) {
  lockGlobalParamsGUI;
  mapUnknownCharNames = map;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPrintCommands(GBool printCommandsA) {
  lockGlobalParamsGUI;
  printCommands = printCommandsA;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setProfileCommands(GBool profileCommandsA) {
  lockGlobalParamsGUI;
  profileCommands = profileCommandsA;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setErrQuiet(GBool errQuietA) {
  lockGlobalParamsGUI;
  errQuiet = errQuietA;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::addSecurityHandler(XpdfSecurityHandler *handler) {
#ifdef ENABLE_PLUGINS
  lockGlobalParamsGUI;
  securityHandlers->append(handler);
  unlockGlobalParamsGUI;
#endif
}

XpdfSecurityHandler *GlobalParamsGUI::getSecurityHandler(char *name) {
#ifdef ENABLE_PLUGINS
  XpdfSecurityHandler *hdlr;
  int i;

  lockGlobalParamsGUI;
  for (i = 0; i < securityHandlers->getLength(); ++i) {
    hdlr = (XpdfSecurityHandler *)securityHandlers->get(i);
    if (!strcasecmp(hdlr->name, name)) {
      unlockGlobalParamsGUI;
      return hdlr;
    }
  }
  unlockGlobalParamsGUI;

  if (!loadPlugin("security", name)) {
    return NULL;
  }
  deleteGooList(keyBindings, KeyBinding);

  lockGlobalParamsGUI;
  for (i = 0; i < securityHandlers->getLength(); ++i) {
    hdlr = (XpdfSecurityHandler *)securityHandlers->get(i);
    if (!strcmp(hdlr->name, name)) {
      unlockGlobalParamsGUI;
      return hdlr;
    }
  }
  unlockGlobalParamsGUI;
#else
  (void)name;
#endif

  return NULL;
}

#ifdef ENABLE_PLUGINS
//------------------------------------------------------------------------
// plugins
//------------------------------------------------------------------------

GBool GlobalParamsGUI::loadPlugin(char *type, char *name) {
  Plugin *plugin;

  if (!(plugin = Plugin::load(type, name))) {
    return gFalse;
  }
  lockGlobalParamsGUI;
  plugins->append(plugin);
  unlockGlobalParamsGUI;
  return gTrue;
}

#endif // ENABLE_PLUGINS
