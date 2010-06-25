//========================================================================
//
// config.h
//
// Copyright 1996-2007 Glyph & Cog, LLC
//
//========================================================================

#ifndef CONFIG_H
#define CONFIG_H

//------------------------------------------------------------------------
// version
//------------------------------------------------------------------------

// xpdf version
#define xpdfVersion          "3.02"
#define xpdfVersionNum       3.02
#define xpdfMajorVersion     3
#define xpdfMinorVersion     2
#define xpdfUpdateVersion    0
#define xpdfMajorVersionStr  "3"
#define xpdfMinorVersionStr  "2"
#define xpdfUpdateVersionStr "0"

// supported PDF version
#define supportedPDFVersionStr "1.7"
#define supportedPDFVersionNum 1.7

// copyright notice
#undef xpdfCopyright
#define xpdfCopyright "Copyright 1996-2007 Glyph & Cog, LLC"

//------------------------------------------------------------------------
// paper size
//------------------------------------------------------------------------

// default paper size (in points) for PostScript output
#ifdef A4_PAPER
#define defPaperWidth  595    // ISO A4 (210x297 mm)
#define defPaperHeight 842
#else
#define defPaperWidth  612    // American letter (8.5x11")
#define defPaperHeight 792
#endif

//------------------------------------------------------------------------
// config file (xpdfrc) path
//------------------------------------------------------------------------

// user config file name, relative to the user's home directory
#define xpdfUserConfigFile ".xpdfrc"

// system config file name (set via the configure script)
#ifdef SYSTEM_XPDFRC
#define xpdfSysConfigFile SYSTEM_XPDFRC
#else
#define xpdfSysConfigFile "xpdfrc"
#endif

//------------------------------------------------------------------------
// X-related constants
//------------------------------------------------------------------------

// default maximum size of color cube to allocate
#define defaultRGBCube 5

// number of fonts (combined t1lib, FreeType, X server) to cache
#define xOutFontCacheSize 64

// number of Type 3 fonts to cache
#define xOutT3FontCacheSize 8

//------------------------------------------------------------------------
// popen
//------------------------------------------------------------------------

#define POPEN_READ_MODE "r"
#endif
