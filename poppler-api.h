//========================================================================
//
// poppler-api.h
//
// Copyright 2012 Andrew Savchenko
//
//========================================================================

#ifndef XPDF_POPPLER_API_H
#define XPDF_POPPLER_API_H

/* Poppler has changed API from between versions 0.18 to 0.20.
 * Since we're going to support both of them, this header is needed.
 * Base program code is targeted on 0.18, 0.20-related changes goes here. */

#include <poppler-config.h>

// 0.18 hasn't any VERSION var exported
#ifdef POPPLER_VERSION
// Class interface is not changed, only class name
#define PSFontParam PSFontParam16

// This piece was remove in favour of fontconfig,
// though it is still needed for xpdf
    
enum DisplayFontParamKind {
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
  void setFileName(GooString *fileNameA) {
    if (displayFontT1 == kind)
        t1.fileName = fileNameA;
    else {
        assert(displayFontTT == kind);
        tt.fileName = fileNameA;
    }
  }  
  virtual ~DisplayFontParam();
};

#endif

#endif /* XPDF_POPPLER_API_H*/
