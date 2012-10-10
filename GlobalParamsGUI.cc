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
#include "GlobalParamsGUI.h"
#include "GfxFont.h"

#if MULTITHREADED
#  define lockGlobalParams            gLockMutex(&mutex)
#  define unlockGlobalParams          gUnlockMutex(&mutex)
#else
#  define lockGlobalParams
#  define unlockGlobalParams
#endif

#include "NameToUnicodeTable.h"
#include "UnicodeMapTables.h"
#include "UTF8.h"

//------------------------------------------------------------------------

#define cidToUnicodeCacheSize     4
#define unicodeToUnicodeCacheSize 4

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

//------------------------------------------------------------------------
// PSFontParam16
//------------------------------------------------------------------------

PSFontParam16::PSFontParam16(GooString *nameA, int wModeA,
			 GooString *psFontNameA, GooString *encodingA) {
  name = nameA;
  wMode = wModeA;
  psFontName = psFontNameA;
  encoding = encodingA;
}

PSFontParam16::~PSFontParam16() {
  delete name;
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

//------------------------------------------------------------------------
// parsing
//------------------------------------------------------------------------

GlobalParamsGUI::GlobalParamsGUI(char *cfgFileName) {
  GooString *fileName;
  FILE *f;

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
    fileName = new GooString(xpdfSysConfigFile);
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
	  error(errIO, -1, "Couldn't find included config file: '%s' (%s:%d)",
		incFile->getCString(), fileName->getCString(), line);
	}
      } else {
	error(errConfig, -1, "Bad 'include' config file command (%s:%d)",
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
      warnDeprecated("enableT1lib", "Freetype font engine is now used.");
    } else if (!cmd->cmp("enableFreeType")) {
      warnDeprecated("enableFreeType", "Freetype font engine is now always used.");
    } else if (!cmd->cmp("enableFreeTypeHinting")) {
      parseYesNo("enableFreeTypeHinting", &enableFreeTypeHinting , tokens, fileName, line);
    } else if (!cmd->cmp("enableFreeTypeSlightHinting")) {
      parseYesNo("enableFreeTypeSlightHinting", &enableFreeTypeSlightHinting, tokens, fileName, line);
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
      error(errConfig, -1, "Unknown config file command '%s' (%s:%d)",
	    cmd->getCString(), fileName->getCString(), line);
      if (!cmd->cmp("displayFontX") ||
	  !cmd->cmp("displayNamedCIDFontX") ||
	  !cmd->cmp("displayCIDFontX")) {
	error(errConfig, -1, "-- Xpdf no longer supports X fonts");
      } else if (!cmd->cmp("t1libControl") || !cmd->cmp("freetypeControl")) {
	error(errConfig, -1, "-- The t1libControl and freetypeControl options have been replaced");
	error(errConfig, -1, "   by the enableT1lib, enableFreeType, and antialias options");
      } else if (!cmd->cmp("fontpath") || !cmd->cmp("fontmap")) {
	error(errConfig, -1, "-- the config file format has changed since Xpdf 0.9x");
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
    error(errConfig, -1, "Bad 'nameToUnicode' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  name = (GooString *)tokens->get(1);
  if (!(f = fopen(name->getCString(), "r"))) {
    error(errIO, -1, "Couldn't open 'nameToUnicode' file '%s'",
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
      error(errConfig, -1, "Bad line in 'nameToUnicode' file (%s:%d)",
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
    error(errIO, -1, "Couldn't open 'nameToUnicode' file '%s'",
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
      error(errConfig, -1, "Bad line in 'nameToUnicode' file (%s:%d)",
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
    error(errConfig, -1, "Bad 'cidToUnicode' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'unicodeToUnicode' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'unicodeMap' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'cMapDir' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'toUnicodeDir' config file command (%s:%d)",
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
  error(errConfig, -1, "Bad 'display*Font*' config file command (%s:%d)",
	fileName->getCString(), line);
}

void GlobalParamsGUI::parsePSPaperSize(GooList *tokens, GooString *fileName,
				    int line) {
  GooString *tok;

  if (tokens->getLength() == 2) {
    tok = (GooString *)tokens->get(1);
    if (!setPSPaperSize(tok->getCString())) {
      error(errConfig, -1, "Bad 'psPaperSize' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'psPaperSize' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParamsGUI::parsePSImageableArea(GooList *tokens, GooString *fileName,
					int line) {
  if (tokens->getLength() != 5) {
    error(errConfig, -1, "Bad 'psImageableArea' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'psLevel' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'psLevel' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParamsGUI::parsePSFile(GooList *tokens, GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(errConfig, -1, "Bad 'psFile' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  if (psFile) {
    delete psFile;
  }
  psFile = ((GooString *)tokens->get(1))->copy();
}

void GlobalParamsGUI::parsePSFont(GooList *tokens, GooString *fileName, int line) {
  PSFontParam16 *param;

  if (tokens->getLength() != 3) {
    error(errConfig, -1, "Bad 'psFont' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  param = new PSFontParam16(((GooString *)tokens->get(1))->copy(), 0,
			  ((GooString *)tokens->get(2))->copy(), NULL);
  psFonts->add(param->name, param);
}

void GlobalParamsGUI::parsePSFont16(char *cmdName, GooList *fontList,
				 GooList *tokens, GooString *fileName, int line) {
  PSFontParam16 *param;
  int wMode;
  GooString *tok;

  if (tokens->getLength() != 5) {
    error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(2);
  if (!tok->cmp("H")) {
    wMode = 0;
  } else if (!tok->cmp("V")) {
    wMode = 1;
  } else {
    error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  param = new PSFontParam16(((GooString *)tokens->get(1))->copy(),
			  wMode,
			  ((GooString *)tokens->get(3))->copy(),
			  ((GooString *)tokens->get(4))->copy());
  fontList->append(param);
}

void GlobalParamsGUI::parseTextEncoding(GooList *tokens, GooString *fileName,
				     int line) {
  if (tokens->getLength() != 2) {
    error(errConfig, -1, "Bad 'textEncoding' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  delete textEncoding;
  textEncoding = ((GooString *)tokens->get(1))->copy();
}

void GlobalParamsGUI::parseTextEOL(GooList *tokens, GooString *fileName, int line) {
  GooString *tok;

  if (tokens->getLength() != 2) {
    error(errConfig, -1, "Bad 'textEOL' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'textEOL' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParamsGUI::parseFontDir(GooList *tokens, GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(errConfig, -1, "Bad 'fontDir' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  fontDirs->append(((GooString *)tokens->get(1))->copy());
}

void GlobalParamsGUI::parseInitialZoom(GooList *tokens,
				    GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(errConfig, -1, "Bad 'initialZoom' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'screenType' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'screenType' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParamsGUI::parseBind(GooList *tokens, GooString *fileName, int line) {
  KeyBinding *binding;
  GooList *cmds;
  int code, mods, context, i;

  if (tokens->getLength() < 4) {
    error(errConfig, -1, "Bad 'bind' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad 'unbind' config file command (%s:%d)",
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

bool GlobalParamsGUI::parseKey(GooString *modKeyStr, GooString *contextStr,
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
    error(errConfig, -1, "Bad key/modifier in '%s' config file command (%s:%d)",
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
	error(errConfig, -1, "Bad context in '%s' config file command (%s:%d)",
	      cmdName, fileName->getCString(), line);
	return gFalse;
      }
      if (!*p0) {
	break;
      }
      if (*p0 != ',') {
	error(errConfig, -1, "Bad context in '%s' config file command (%s:%d)",
	      cmdName, fileName->getCString(), line);
	return gFalse;
      }
      ++p0;
    }
  }

  return true;
}

void GlobalParamsGUI::parseCommand(char *cmdName, GooString **val,
				GooList *tokens, GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  if (*val) {
    delete *val;
  }
  *val = ((GooString *)tokens->get(1))->copy();
}

void GlobalParamsGUI::parseYesNo(char *cmdName, bool *flag,
			      GooList *tokens, GooString *fileName, int line) {
  GooString *tok;

  if (tokens->getLength() != 2) {
    error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (!parseYesNo2(tok->getCString(), flag)) {
    error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
  }
}

bool GlobalParamsGUI::parseYesNo2(char *token, bool *flag) {
  if (!strcmp(token, "yes")) {
    *flag = true;
  } else if (!strcmp(token, "no")) {
    *flag = gFalse;
  } else {
    return gFalse;
  }
  return true;
}

void GlobalParamsGUI::parseInteger(char *cmdName, int *val,
				GooList *tokens, GooString *fileName, int line) {
  GooString *tok;
  int i;

  if (tokens->getLength() != 2) {
    error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (tok->getLength() == 0) {
    error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
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
      error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
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
    error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (tok->getLength() == 0) {
    error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
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
      error(errConfig, -1, "Bad '%s' config file command (%s:%d)",
	    cmdName, fileName->getCString(), line);
      return;
    }
  }
  *val = atof(tok->getCString());
}

//------------------------------------------------------------------------
// accessors
//------------------------------------------------------------------------

GooString *GlobalParamsGUI::getPSFile() {
  GooString *s;

  lockGlobalParamsGUI;
  s = psFile ? psFile->copy() : NULL;
  unlockGlobalParamsGUI;
  return s;
}

bool GlobalParamsGUI::getPSCrop() {
  bool f;

  lockGlobalParamsGUI;
  f = psCrop;
  unlockGlobalParamsGUI;
  return f;
}

GooString *GlobalParamsGUI::getInitialZoom() {
  GooString *s;

  lockGlobalParamsGUI;
  s = initialZoom->copy();
  unlockGlobalParamsGUI;
  return s;
}

bool GlobalParamsGUI::getContinuousView() {
  bool f;

  lockGlobalParamsGUI;
  f = continuousView;
  unlockGlobalParamsGUI;
  return f;
}

bool GlobalParamsGUI::getEnableFreeTypeHinting() {
  bool f;

  lockGlobalParamsGUI;
  f = enableFreeTypeHinting;
  unlockGlobalParamsGUI;
  return f;
}

bool GlobalParamsGUI::getEnableFreeTypeSlightHinting() {
  bool f;

  lockGlobalParamsGUI;
  f = enableFreeTypeSlightHinting;
  unlockGlobalParamsGUI;
  return f;
}

bool GlobalParamsGUI::getAntialias() {
  bool f;

  lockGlobalParamsGUI;
  f = antialias;
  unlockGlobalParamsGUI;
  return f;
}

bool GlobalParamsGUI::getVectorAntialias() {
  bool f;

  lockGlobalParamsGUI;
  f = vectorAntialias;
  unlockGlobalParamsGUI;
  return f;
}

bool GlobalParamsGUI::getStrokeAdjust() {
  bool f;

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

bool GlobalParamsGUI::getMapNumericCharNames() {
  bool map;

  lockGlobalParamsGUI;
  map = mapNumericCharNames;
  unlockGlobalParamsGUI;
  return map;
}

bool GlobalParamsGUI::getMapUnknownCharNames() {
  bool map;

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

bool GlobalParamsGUI::getPrintCommands() {
  bool p;

  lockGlobalParamsGUI;
  p = printCommands;
  unlockGlobalParamsGUI;
  return p;
}

bool GlobalParamsGUI::getProfileCommands() {
  bool p;

  lockGlobalParamsGUI;
  p = profileCommands;
  unlockGlobalParamsGUI;
  return p;
}

bool GlobalParamsGUI::getErrQuiet() {
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

bool GlobalParamsGUI::setPSPaperSize(char *size) {
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
  return true;
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

void GlobalParamsGUI::setPSCrop(bool crop) {
  lockGlobalParamsGUI;
  psCrop = crop;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSExpandSmaller(bool expand) {
  lockGlobalParamsGUI;
  psExpandSmaller = expand;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSShrinkLarger(bool shrink) {
  lockGlobalParamsGUI;
  psShrinkLarger = shrink;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSCenter(bool center) {
  lockGlobalParamsGUI;
  psCenter = center;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSDuplex(bool duplex) {
  lockGlobalParamsGUI;
  psDuplex = duplex;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSLevel(PSLevel level) {
  lockGlobalParamsGUI;
  psLevel = level;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSEmbedType1(bool embed) {
  lockGlobalParamsGUI;
  psEmbedType1 = embed;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSEmbedTrueType(bool embed) {
  lockGlobalParamsGUI;
  psEmbedTrueType = embed;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSEmbedCIDPostScript(bool embed) {
  lockGlobalParamsGUI;
  psEmbedCIDPostScript = embed;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSEmbedCIDTrueType(bool embed) {
  lockGlobalParamsGUI;
  psEmbedCIDTrueType = embed;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSPreload(bool preload) {
  lockGlobalParamsGUI;
  psPreload = preload;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSOPI(bool opi) {
  lockGlobalParamsGUI;
  psOPI = opi;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPSASCIIHex(bool hex) {
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

bool GlobalParamsGUI::setTextEOL(char *s) {
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
  return true;
}

void GlobalParamsGUI::setTextPageBreaks(bool pageBreaks) {
  lockGlobalParamsGUI;
  textPageBreaks = pageBreaks;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setTextKeepTinyChars(bool keep) {
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

void GlobalParamsGUI::setContinuousView(bool cont) {
  lockGlobalParamsGUI;
  continuousView = cont;
  unlockGlobalParamsGUI;
}

bool GlobalParamsGUI::setEnableFreeTypeHinting(char *s) {
  bool ok;

  lockGlobalParamsGUI;
  ok = parseYesNo2(s, &enableFreeTypeHinting);
  unlockGlobalParamsGUI;
  return ok;
}

bool GlobalParamsGUI::setEnableFreeTypeSlightHinting(char *s) {
  bool ok;

  lockGlobalParamsGUI;
  ok = parseYesNo2(s, &enableFreeTypeSlightHinting);
  unlockGlobalParamsGUI;
  return ok;
}

bool GlobalParamsGUI::setAntialias(char *s) {
  bool ok;

  lockGlobalParamsGUI;
  ok = parseYesNo2(s, &antialias);
  unlockGlobalParamsGUI;
  return ok;
}

bool GlobalParamsGUI::setVectorAntialias(char *s) {
  bool ok;

  lockGlobalParamsGUI;
  ok = parseYesNo2(s, &vectorAntialias);
  unlockGlobalParamsGUI;
  return ok;
}

void GlobalParamsGUI::setStrokeAdjust(bool adjust)
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

void GlobalParamsGUI::setMapNumericCharNames(bool map) {
  lockGlobalParamsGUI;
  mapNumericCharNames = map;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setMapUnknownCharNames(bool map) {
  lockGlobalParamsGUI;
  mapUnknownCharNames = map;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setPrintCommands(bool printCommandsA) {
  lockGlobalParamsGUI;
  printCommands = printCommandsA;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setProfileCommands(bool profileCommandsA) {
  lockGlobalParamsGUI;
  profileCommands = profileCommandsA;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::setErrQuiet(bool errQuietA) {
  lockGlobalParamsGUI;
  errQuiet = errQuietA;
  unlockGlobalParamsGUI;
}

void GlobalParamsGUI::addSecurityHandler(XpdfSecurityHandler *handler) {
}

XpdfSecurityHandler *GlobalParamsGUI::getSecurityHandler(char *name) {
  (void)name;

  return NULL;
}

/*** Auxiliary functions ***/

/* Display a deprecation warning about legacy option */
void GlobalParamsGUI::warnDeprecated(char *option, char *message = NULL) {
  fprintf(stderr, "Warning: option '%s' is deprecated and will be ignored!\n", option);
  if (message)
    fputs(message, stderr);
}
