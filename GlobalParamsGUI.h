//========================================================================
//
// GlobalParamsGUI.h
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef GLOBALPARAMSGUI_H
#define GLOBALPARAMSGUI_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "config.h"
#include <stdio.h>
#include <fontconfig/fontconfig.h>
#include <GlobalParams.h>
#include "goo/gtypes.h"
#include "CharTypes.h"

#if MULTITHREADED
#include "goo/GooMutex.h"
#endif

class GooString;
class GooList;
class GooHash;
class NameToCharCode;
class CharCodeToUnicode;
class CharCodeToUnicodeCache;
class UnicodeMap;
class UnicodeMapCache;
class CMap;
class CMapCache;
struct XpdfSecurityHandler;
class GlobalParamsGUI;
class GfxFont;

//------------------------------------------------------------------------

// The global parameters object.
extern GlobalParamsGUI *globalParamsGUI;

//------------------------------------------------------------------------

/*enum DisplayFontParamKind {
  displayFontT1,
  displayFontTT
};

struct DisplayFontParamT1 {
  GooString *fileName;
};

struct DisplayFontParamTT {
  GooString *fileName;
  int faceIndex;
};

class DisplayFontParam {
public:

  GooString *name;		// font name for 8-bit fonts and named
				//   CID fonts; collection name for
				//   generic CID fonts
  DisplayFontParamKind kind;
  union {
    DisplayFontParamT1 t1;
    DisplayFontParamTT tt;
  };

  DisplayFontParam(GooString *nameA, DisplayFontParamKind kindA);
  virtual ~DisplayFontParam();
};

//------------------------------------------------------------------------

class PSFontParam {
public:

  GooString *pdfFontName;		// PDF font name for 8-bit fonts and
				//   named 16-bit fonts; char collection
				//   name for generic 16-bit fonts
  int wMode;			// writing mode (0=horiz, 1=vert) for
				//   16-bit fonts
  GooString *psFontName;		// PostScript font name
  GooString *encoding;		// encoding, for 16-bit fonts only

  PSFontParam(GooString *pdfFontNameA, int wModeA,
	      GooString *psFontNameA, GooString *encodingA);
  ~PSFontParam();
};*/

//------------------------------------------------------------------------

/*enum PSLevel {
  psLevel1,
  psLevel1Sep,
  psLevel2,
  psLevel2Sep,
  psLevel3,
  psLevel3Sep
};*/

//------------------------------------------------------------------------

/*enum EndOfLineKind {
  eolUnix,			// LF
  eolDOS,			// CR+LF
  eolMac			// CR
};*/

//------------------------------------------------------------------------

/*enum ScreenType {
  screenUnset,
  screenDispersed,
  screenClustered,
  screenStochasticClustered
};*/

//------------------------------------------------------------------------

class KeyBinding {
public:

  int code;			// 0x20 .. 0xfe = ASCII,
				//   >=0x10000 = special keys, mouse buttons,
				//   etc. (xpdfKeyCode* symbols)
  int mods;			// modifiers (xpdfKeyMod* symbols, or-ed
				//   together)
  int context;			// context (xpdfKeyContext* symbols, or-ed
				//   together)
  GooList *cmds;			// list of commands [GooString]

  KeyBinding(int codeA, int modsA, int contextA, char *cmd0);
  KeyBinding(int codeA, int modsA, int contextA, char *cmd0, char *cmd1);
  KeyBinding(int codeA, int modsA, int contextA, GooList *cmdsA);
  ~KeyBinding();
};

#define xpdfKeyCodeTab            0x1000
#define xpdfKeyCodeReturn         0x1001
#define xpdfKeyCodeEnter          0x1002
#define xpdfKeyCodeBackspace      0x1003
#define xpdfKeyCodeInsert         0x1004
#define xpdfKeyCodeDelete         0x1005
#define xpdfKeyCodeHome           0x1006
#define xpdfKeyCodeEnd            0x1007
#define xpdfKeyCodePgUp           0x1008
#define xpdfKeyCodePgDn           0x1009
#define xpdfKeyCodeLeft           0x100a
#define xpdfKeyCodeRight          0x100b
#define xpdfKeyCodeUp             0x100c
#define xpdfKeyCodeDown           0x100d
#define xpdfKeyCodeF1             0x1100
#define xpdfKeyCodeF35            0x1122
#define xpdfKeyCodeAdd            0x1200
#define xpdfKeyCodeSubtract       0x1201
#define xpdfKeyCodeMousePress1    0x2001
#define xpdfKeyCodeMousePress2    0x2002
#define xpdfKeyCodeMousePress3    0x2003
#define xpdfKeyCodeMousePress4    0x2004
#define xpdfKeyCodeMousePress5    0x2005
#define xpdfKeyCodeMousePress6    0x2006
#define xpdfKeyCodeMousePress7    0x2007
#define xpdfKeyCodeMouseRelease1  0x2101
#define xpdfKeyCodeMouseRelease2  0x2102
#define xpdfKeyCodeMouseRelease3  0x2103
#define xpdfKeyCodeMouseRelease4  0x2104
#define xpdfKeyCodeMouseRelease5  0x2105
#define xpdfKeyCodeMouseRelease6  0x2106
#define xpdfKeyCodeMouseRelease7  0x2107
#define xpdfKeyModNone            0
#define xpdfKeyModShift           (1 << 0)
#define xpdfKeyModCtrl            (1 << 1)
#define xpdfKeyModAlt             (1 << 2)
#define xpdfKeyContextAny         0
#define xpdfKeyContextFullScreen  (1 << 0)
#define xpdfKeyContextWindow      (2 << 0)
#define xpdfKeyContextContinuous  (1 << 2)
#define xpdfKeyContextSinglePage  (2 << 2)
#define xpdfKeyContextOverLink    (1 << 4)
#define xpdfKeyContextOffLink     (2 << 4)
#define xpdfKeyContextOutline     (1 << 6)
#define xpdfKeyContextMainWin     (2 << 6)
#define xpdfKeyContextScrLockOn   (1 << 8)
#define xpdfKeyContextScrLockOff  (2 << 8)

//------------------------------------------------------------------------

class GlobalParamsGUI {
public:

  // Initialize the global parameters by attempting to read a config
  // file.
  GlobalParamsGUI(char *cfgFileName = NULL);

  ~GlobalParamsGUI();

  void setBaseDir(char *dir);
  void setupBaseFonts(char *dir);

  void parseLine(char *buf, GooString *fileName, int line);

  //----- accessors

  CharCode getMacRomanCharCode(char *charName);

  GooString *getBaseDir();
  Unicode mapNameToUnicode(char *charName);
  UnicodeMap *getResidentUnicodeMap(GooString *encodingName);
  FILE *getUnicodeMapFile(GooString *encodingName);
  FILE *findCMapFile(GooString *collection, GooString *cMapName);
  FILE *findToUnicodeFile(GooString *name);
  DisplayFontParam *getDisplayFont(GfxFont *font);
  DisplayFontParam *getDisplayFont(GooString *fontName);
  DisplayFontParam *getDisplayCIDFont(GooString *fontName, GooString *collection);
  GooString *getPSFile();
  int getPSPaperWidth();
  int getPSPaperHeight();
  void getPSImageableArea(int *llx, int *lly, int *urx, int *ury);
  GBool getPSDuplex();
  GBool getPSCrop();
  GBool getPSExpandSmaller();
  GBool getPSShrinkLarger();
  GBool getPSCenter();
  PSLevel getPSLevel();
  PSFontParam *getPSFont(GooString *fontName);
  PSFontParam *getPSFont16(GooString *fontName, GooString *collection, int wMode);
  GBool getPSEmbedType1();
  GBool getPSEmbedTrueType();
  GBool getPSEmbedCIDPostScript();
  GBool getPSEmbedCIDTrueType();
  GBool getPSPreload();
  GBool getPSOPI();
  GBool getPSASCIIHex();
  GooString *getTextEncodingName();
  EndOfLineKind getTextEOL();
  GBool getTextPageBreaks();
  GBool getTextKeepTinyChars();
  GooString *findFontFile(GooString *fontName, char **exts);
  GooString *getInitialZoom();
  GBool getContinuousView();
  GBool getEnableT1lib();
  GBool getEnableFreeType();
  GBool getEnableFreeTypeHinting();
  GBool getAntialias();
  GBool getVectorAntialias();
  GBool getStrokeAdjust();
  ScreenType getScreenType();
  int getScreenSize();
  int getScreenDotRadius();
  double getScreenGamma();
  double getScreenBlackThreshold();
  double getScreenWhiteThreshold();
  GooString *getURLCommand() { return urlCommand; }
  GooString *getMovieCommand() { return movieCommand; }
  GBool getMapNumericCharNames();
  GBool getMapUnknownCharNames();
  GooList *getKeyBinding(int code, int mods, int context);
  GBool getPrintCommands();
  GBool getProfileCommands();
  GBool getErrQuiet();

  CharCodeToUnicode *getCIDToUnicode(GooString *collection);
  CharCodeToUnicode *getUnicodeToUnicode(GooString *fontName);
  UnicodeMap *getUnicodeMap(GooString *encodingName);
  CMap *getCMap(GooString *collection, GooString *cMapName, Stream *stream);
  UnicodeMap *getTextEncoding();
#ifdef ENABLE_PLUGINS
  GBool loadPlugin(char *type, char *name);
#endif

  //----- functions to set parameters
  void addDisplayFont(DisplayFontParam *param);
  void setPSFile(char *file);
  GBool setPSPaperSize(char *size);
  void setPSPaperWidth(int width);
  void setPSPaperHeight(int height);
  void setPSImageableArea(int llx, int lly, int urx, int ury);
  void setPSDuplex(GBool duplex);
  void setPSCrop(GBool crop);
  void setPSExpandSmaller(GBool expand);
  void setPSShrinkLarger(GBool shrink);
  void setPSCenter(GBool center);
  void setPSLevel(PSLevel level);
  void setPSEmbedType1(GBool embed);
  void setPSEmbedTrueType(GBool embed);
  void setPSEmbedCIDPostScript(GBool embed);
  void setPSEmbedCIDTrueType(GBool embed);
  void setPSPreload(GBool preload);
  void setPSOPI(GBool opi);
  void setPSASCIIHex(GBool hex);
  void setTextEncoding(char *encodingName);
  GBool setTextEOL(char *s);
  void setTextPageBreaks(GBool pageBreaks);
  void setTextKeepTinyChars(GBool keep);
  void setInitialZoom(char *s);
  void setContinuousView(GBool cont);
  GBool setEnableT1lib(char *s);
  GBool setEnableFreeType(char *s);
  GBool setEnableFreeTypeHinting(char *s);
  GBool setAntialias(char *s);
  GBool setVectorAntialias(char *s);
  void setStrokeAdjust(GBool strokeAdjust);
  void setScreenType(ScreenType st);
  void setScreenSize(int size);
  void setScreenDotRadius(int radius);
  void setScreenGamma(double gamma);
  void setScreenBlackThreshold(double blackThreshold);
  void setScreenWhiteThreshold(double whiteThreshold);
  void setMapNumericCharNames(GBool map);
  void setMapUnknownCharNames(GBool map);
  void setPrintCommands(GBool printCommandsA);
  void setProfileCommands(GBool profileCommandsA);
  void setErrQuiet(GBool errQuietA);

  //----- security handlers

  void addSecurityHandler(XpdfSecurityHandler *handler);
  XpdfSecurityHandler *getSecurityHandler(char *name);

  GBool parseYesNo2(char *token, GBool *flag);
private:

  void createDefaultKeyBindings();
  void parseFile(GooString *fileName, FILE *f);
  void parseNameToUnicode(GooList *tokens, GooString *fileName, int line);
  void parseNameToUnicode(GooString *name);
  void parseCIDToUnicode(GooList *tokens, GooString *fileName, int line);
  void parseUnicodeToUnicode(GooList *tokens, GooString *fileName, int line);
  void parseUnicodeMap(GooList *tokens, GooString *fileName, int line);
  void parseCMapDir(GooList *tokens, GooString *fileName, int line);
  void parseToUnicodeDir(GooList *tokens, GooString *fileName, int line);
  void parseDisplayFont(GooList *tokens, GooHash *fontHash,
			DisplayFontParamKind kind,
			GooString *fileName, int line);
  void parsePSFile(GooList *tokens, GooString *fileName, int line);
  void parsePSPaperSize(GooList *tokens, GooString *fileName, int line);
  void parsePSImageableArea(GooList *tokens, GooString *fileName, int line);
  void parsePSLevel(GooList *tokens, GooString *fileName, int line);
  void parsePSFont(GooList *tokens, GooString *fileName, int line);
  void parsePSFont16(char *cmdName, GooList *fontList,
		     GooList *tokens, GooString *fileName, int line);
  void parseTextEncoding(GooList *tokens, GooString *fileName, int line);
  void parseTextEOL(GooList *tokens, GooString *fileName, int line);
  void parseFontDir(GooList *tokens, GooString *fileName, int line);
  void parseInitialZoom(GooList *tokens, GooString *fileName, int line);
  void parseScreenType(GooList *tokens, GooString *fileName, int line);
  void parseBind(GooList *tokens, GooString *fileName, int line);
  void parseUnbind(GooList *tokens, GooString *fileName, int line);
  GBool parseKey(GooString *modKeyStr, GooString *contextStr,
		 int *code, int *mods, int *context,
		 char *cmdName,
		 GooList *tokens, GooString *fileName, int line);
  void parseCommand(char *cmdName, GooString **val,
		    GooList *tokens, GooString *fileName, int line);
  void parseYesNo(char *cmdName, GBool *flag,
		  GooList *tokens, GooString *fileName, int line);
  void parseInteger(char *cmdName, int *val,
		    GooList *tokens, GooString *fileName, int line);
  void parseFloat(char *cmdName, double *val,
		  GooList *tokens, GooString *fileName, int line);
  UnicodeMap *getUnicodeMap2(GooString *encodingName);

  void addCIDToUnicode(GooString *collection, GooString *fileName);
  void addUnicodeMap(GooString *encodingName, GooString *fileName);
  void addCMapDir(GooString *collection, GooString *dir);

  //----- static tables

  NameToCharCode *		// mapping from char name to
    macRomanReverseMap;		//   MacRomanEncoding index

  //----- user-modifiable settings

  GooString *baseDir;		// base directory - for plugins, etc.
  NameToCharCode *		// mapping from char name to Unicode
    nameToUnicode;
  GooHash *cidToUnicodes;		// files for mappings from char collections
				//   to Unicode, indexed by collection name
				//   [GooString]
  GooHash *unicodeToUnicodes;	// files for Unicode-to-Unicode mappings,
				//   indexed by font name pattern [GooString]
  GooHash *residentUnicodeMaps;	// mappings from Unicode to char codes,
				//   indexed by encoding name [UnicodeMap]
  GooHash *unicodeMaps;		// files for mappings from Unicode to char
				//   codes, indexed by encoding name [GooString]
  GooHash *cMapDirs;		// list of CMap dirs, indexed by collection
				//   name [GooList[GooString]]
  GooList *toUnicodeDirs;		// list of ToUnicode CMap dirs [GooString]
  GooHash *displayFonts;		// display font info, indexed by font name
				//   [DisplayFontParam]
  GooHash *displayCIDFonts;	// display CID font info, indexed by
				//   collection [DisplayFontParam]
  GooHash *displayNamedCIDFonts;	// display CID font info, indexed by
				//   font name [DisplayFontParam]
  GooString *psFile;		// PostScript file or command (for xpdf)
  int psPaperWidth;		// paper size, in PostScript points, for
  int psPaperHeight;		//   PostScript output
  int psImageableLLX,		// imageable area, in PostScript points,
      psImageableLLY,		//   for PostScript output
      psImageableURX,
      psImageableURY;
  GBool psCrop;			// crop PS output to CropBox
  GBool psExpandSmaller;	// expand smaller pages to fill paper
  GBool psShrinkLarger;		// shrink larger pages to fit paper
  GBool psCenter;		// center pages on the paper
  GBool psDuplex;		// enable duplexing in PostScript?
  PSLevel psLevel;		// PostScript level to generate
  GooHash *psFonts;		// PostScript font info, indexed by PDF
				//   font name [PSFontParam]
  GooList *psNamedFonts16;	// named 16-bit fonts [PSFontParam]
  GooList *psFonts16;		// generic 16-bit fonts [PSFontParam]
  GBool psEmbedType1;		// embed Type 1 fonts?
  GBool psEmbedTrueType;	// embed TrueType fonts?
  GBool psEmbedCIDPostScript;	// embed CID PostScript fonts?
  GBool psEmbedCIDTrueType;	// embed CID TrueType fonts?
  GBool psPreload;		// preload PostScript images and forms into
				//   memory
  GBool psOPI;			// generate PostScript OPI comments?
  GBool psASCIIHex;		// use ASCIIHex instead of ASCII85?
  GooString *textEncoding;	// encoding (unicodeMap) to use for text
				//   output
  EndOfLineKind textEOL;	// type of EOL marker to use for text
				//   output
  GBool textPageBreaks;		// insert end-of-page markers?
  GBool textKeepTinyChars;	// keep all characters in text output
  GooList *fontDirs;		// list of font dirs [GooString]
  GooString *initialZoom;		// initial zoom level
  GBool continuousView;		// continuous view mode
  GBool enableT1lib;		// t1lib enable flag
  GBool enableFreeType;		// FreeType enable flag
  GBool enableFreeTypeHinting;  // FreeType hinting enable flag
  GBool antialias;		// anti-aliasing enable flag
  GBool vectorAntialias;	// vector anti-aliasing enable flag
  GBool strokeAdjust;		// stroke adjustment enable flag
  ScreenType screenType;	// halftone screen type
  int screenSize;		// screen matrix size
  int screenDotRadius;		// screen dot radius
  double screenGamma;		// screen gamma correction
  double screenBlackThreshold;	// screen black clamping threshold
  double screenWhiteThreshold;	// screen white clamping threshold
  GooString *urlCommand;		// command executed for URL links
  GooString *movieCommand;	// command executed for movie annotations
  GBool mapNumericCharNames;	// map numeric char names (from font subsets)?
  GBool mapUnknownCharNames;	// map unknown char names?
  GooList *keyBindings;		// key & mouse button bindings [KeyBinding]
  GBool printCommands;		// print the drawing commands
  GBool profileCommands;	// profile the drawing commands
  GBool errQuiet;		// suppress error messages?

  CharCodeToUnicodeCache *cidToUnicodeCache;
  CharCodeToUnicodeCache *unicodeToUnicodeCache;
  UnicodeMapCache *unicodeMapCache;
  CMapCache *cMapCache;

  FcConfig *FCcfg;

#ifdef ENABLE_PLUGINS
  GooList *plugins;		// list of plugins [Plugin]
  GooList *securityHandlers;	// list of loaded security handlers
				//   [XpdfSecurityHandler]
#endif

#if MULTITHREADED
  GooMutex mutex;
  GooMutex unicodeMapCacheMutex;
  GooMutex cMapCacheMutex;
#endif
};

#endif
