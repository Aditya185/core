/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifndef INCLUDED_FILTER_SOURCE_FLASH_SWFWRITER_HXX
#define INCLUDED_FILTER_SOURCE_FLASH_SWFWRITER_HXX

#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/i18n/XBreakIterator.hpp>
#include <vcl/checksum.hxx>
#include <vcl/font.hxx>
#include <vcl/gradient.hxx>
#include <vcl/vclptr.hxx>
#include <unotools/tempfile.hxx>
#include <tools/color.hxx>
#include <tools/gen.hxx>
#include <tools/stream.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <osl/diagnose.h>

#include <vector>
#include <stack>
#include <map>

class GDIMetaFile;
class BitmapEx;
class Gradient;
class SvtGraphicFill;
class SvtGraphicStroke;
class LineInfo;
namespace basegfx { class B2DPolygon; }
namespace tools
{
    class Polygon;
    class PolyPolygon;
}

inline sal_uInt16 uInt16_( sal_Int32 nValue )
{
    OSL_ENSURE( (nValue >= 0) && (static_cast<sal_uInt32>(nValue) <= 0xffff), "overflow while converting sal_Int32 to sal_uInt16" );
    return static_cast<sal_uInt16>(nValue);
}

inline sal_Int16 Int16_( sal_Int32 nValue )
{
    OSL_ENSURE( (nValue >= -32768) && (nValue <= 32767), "overflow while converting sal_Int32 to sal_Int16" );
    return static_cast<sal_Int16>(nValue);
}

class VirtualDevice;

sal_uInt16 getMaxBitsSigned( sal_Int32 nValue );

namespace swf {

const sal_uInt8 TAG_END             = 0;
const sal_uInt8 TAG_SHOWFRAME       = 1;

const sal_uInt8 TAG_DEFINEBUTTON    = 7;

const sal_uInt8 TAG_BACKGROUNDCOLOR = 9;

const sal_uInt8 TAG_DOACTION        = 12;
const sal_uInt8 TAG_STARTSOUND      = 15;

const sal_uInt8 TAG_SOUNDSTREAMBLOCK = 19;
const sal_uInt8 TAG_SOUNDSTREAMHEAD = 18;
const sal_uInt8 TAG_SOUNDSTREAMHEAD2 = 45;

const sal_uInt8 TAG_JPEGTABLES      = 8;
const sal_uInt8 TAG_DEFINEBITS      = 6;
const sal_uInt8 TAG_DEFINEBITSLOSSLESS = 20;
const sal_uInt8 TAG_DEFINEBITSJPEG2 = 21;
const sal_uInt8 TAG_DEFINEBITSJPEG3 = 35;
const sal_uInt8 TAG_DEFINEBITSLOSSLESS2 = 36;
const sal_uInt8 TAG_DEFINEEDITTEXT= 37;
const sal_uInt8 TAG_PLACEOBJECT     = 4;
const sal_uInt8 TAG_PLACEOBJECT2    = 26;
const sal_uInt8 TAG_REMOVEOBJECT2   = 28;

const sal_uInt8 TAG_DEFINEFONT      = 10;
const sal_uInt8 TAG_DEFINETEXT      = 11;
const sal_uInt8 TAG_DEFINESHAPE3    = 32;
const sal_uInt8 TAG_DEFINESPRITE    = 39;

const sal_uInt8 TAG_FRAMELABEL      = 43;

const sal_uInt8 TAG_HEADER          = 0xff;


/** converts a double to a 16.16 flash fixed value */
sal_uInt32 getFixed( double fValue );


typedef ::std::map<BitmapChecksum, sal_uInt16> ChecksumCache;

/** container class to create bit structures */
class BitStream
{
public:
    BitStream();

    void writeUB( sal_uInt32 nValue, sal_uInt16 nBits );
    void writeSB( sal_Int32 nValue, sal_uInt16 nBits );
    void writeFB( sal_uInt32 nValue, sal_uInt16 nBits );

    void pad();
    void writeTo( SvStream& out );

    sal_uInt32 getOffset() const;
private:

    std::vector< sal_uInt8 > maData;
    sal_uInt8 mnBitPos;
    sal_uInt8 mnCurrentByte;
};


/** this class collects all used glyphs for a given fonts and maps
    characters to glyph ids.
*/
class FlashFont
{
public:
    FlashFont( const vcl::Font& rFont, sal_uInt16 nId );
    ~FlashFont();

    sal_uInt16 getGlyph( sal_uInt16 nChar, VirtualDevice* pVDev );

    void write( SvStream& out );

    sal_uInt16 getID() const { return mnId; }
    const vcl::Font& getFont() const { return maFont; }

private:
    const vcl::Font maFont;
    std::map<sal_uInt16, sal_uInt16> maGlyphIndex;
    sal_uInt16 mnNextIndex;
    sal_uInt16 mnId;
    BitStream maGlyphData;
    std::vector< sal_uInt16 > maGlyphOffsets;
};

/** this class helps creating flash tags */
class Tag : public SvMemoryStream
{
public:
    explicit Tag( sal_uInt8 nTagId );

    sal_uInt8 getTagId() const { return mnTagId; }

    void write( SvStream& out );

    void addUI32( sal_uInt32 nValue );
    void addUI16( sal_uInt16 nValue );
    void addUI8( sal_uInt8 nValue );
    void addBits( BitStream& rIn );

    void addRGBA( const Color& rColor );
    void addRGB( const Color& rColor );
    void addRect( const tools::Rectangle& rRect );
    void addMatrix( const ::basegfx::B2DHomMatrix& rMatrix ); // #i73264#
    void addStream( SvStream& rIn );

    static void writeMatrix( SvStream& rOut, const ::basegfx::B2DHomMatrix& rMatrix ); // #i73264#
    static void writeRect( SvStream& rOut, const tools::Rectangle& rRect );

private:
    sal_uInt8 mnTagId;
};


/** this class helps to define flash sprites */
class Sprite
{
public:
    explicit Sprite( sal_uInt16 nId );
    ~Sprite();

    void write( SvStream& out );
    void addTag( std::unique_ptr<Tag> pNewTag );

private:
    std::vector< std::unique_ptr<Tag> > maTags;
    sal_uInt16  mnId;
    sal_uInt32  mnFrames;
};


/** this class stores a flash fill style for shapes */
class FillStyle
{
public:
    enum FillStyleType { solid = 0x00, linear_gradient = 0x10, radial_gradient = 0x12, tiled_bitmap = 0x40, clipped_bitmap = 0x41 };

    /** this c'tor creates a solid fill style */
    explicit FillStyle( const Color& rSolidColor );

    /** this c'tor creates a linear or radial gradient fill style */
    FillStyle( const tools::Rectangle& rBoundRect, const Gradient& rGradient );

    /** this c'tor creates a tiled or clipped bitmap fill style */
    FillStyle( sal_uInt16 nBitmapId, bool bClipped, const ::basegfx::B2DHomMatrix& rMatrix ); // #i73264#

    void addTo( Tag* pTag ) const;

private:
    void Impl_addGradient( Tag* pTag ) const;

    FillStyleType   meType;
    ::basegfx::B2DHomMatrix     maMatrix; // #i73264#
    sal_uInt16      mnBitmapId;
    Color           maColor;
    Gradient        maGradient;
    tools::Rectangle       maBoundRect;
};


/** this class creates a flash movie from vcl geometry */
class Writer
{
    friend class FlashFont;

public:
    /** creates a writer for a new flash movie.
        nDocWidth and nDocHeight are the dimensions of the movie.
        They must be in 100th/mm.

        An invisible shape with the size of the document is placed at depth 1
        and it clips all shapes on depth 2 and 3.
    */
    Writer( sal_Int32 nTWIPWidthOutput, sal_Int32 nTWIPHeightOutput, sal_Int32 nDocWidth, sal_Int32 nDocHeight, sal_Int32 nJPEGcompressMode );
    ~Writer();

    void storeTo( css::uno::Reference< css::io::XOutputStream > const &xOutStream );

    // geometry
    void setClipping( const tools::PolyPolygon* pClipPolyPolygon );

    /** defines a flash shape from a filled polygon.
        The coordinates must be in twips */
    sal_uInt16 defineShape( const tools::Polygon& rPoly, const FillStyle& rFillStyle );

    /** defines a flash shape from a filled polypolygon.
        The coordinates must be in twips */
    sal_uInt16 defineShape( const tools::PolyPolygon& rPolyPoly, const FillStyle& rFillStyle );

    /** defines a flash shape from an outlined polypolygon.
        The coordinates must be in twips */
    sal_uInt16 defineShape( const tools::PolyPolygon& rPolyPoly, sal_uInt16 nLineWidth, const Color& rLineColor );

    /** defines a flash shape from a vcl metafile.
        The mapmode of the metafile is used to map all coordinates to twips.
        A character id of a flash sprite is returned that contains all geometry
        from the metafile.
    */
    sal_uInt16 defineShape( const GDIMetaFile& rMtf );

    /** defines a bitmap and returns its flash id.
    */
    sal_uInt16 defineBitmap( const BitmapEx& bmpSource, sal_Int32 nJPEGQualityLevel  );

    // control tags

    /** inserts a place shape tag into the movie stream or the current sprite */
    void placeShape( sal_uInt16 nID, sal_uInt16 nDepth, sal_Int32 x, sal_Int32 y );

    /** inserts a remove shape tag into the movie stream or the current sprite */
    void removeShape( sal_uInt16 nDepth );

    /** inserts a show frame tag into the movie stream or the current sprite */
    void showFrame();

    /** creates a new sprite and sets it as the current sprite for editing.
        Only one sprite can be edited at one time */
    sal_uInt16 startSprite();

    /** ends editing of the current sprites and adds it to the movie stream */
    void endSprite();

    /** inserts a doaction tag with an ActionStop */
    void stop();

    /** inserts a doaction tag with an ActionStop, place a button on depth nDepth that
        continues playback on click */
    void waitOnClick( sal_uInt16 nDepth );

    /** inserts a doaction tag with an ActionGotoFrame */
    void gotoFrame( sal_uInt16 nFrame );

private:
    Point                   map( const Point& rPoint ) const;
    Size                    map( const Size& rSize ) const;
    void                    map( tools::PolyPolygon& rPolyPolygon ) const;
    sal_Int32               mapRelative( sal_Int32 n100thMM ) const;

    void startTag( sal_uInt8 nTagId );
    void endTag();
    sal_uInt16 createID() { return mnNextId++; }

    void Impl_writeBmp( sal_uInt16 nBitmapId, sal_uInt32 width, sal_uInt32 height, sal_uInt8 const *pCompressed, sal_uInt32 compressed_size );
    void Impl_writeImage( const BitmapEx& rBmpEx, const Point& rPt, const Size& rSz, const Point& rSrcPt, const Size& rSrcSz, const tools::Rectangle& rClipRect, bool bMap );
    void Impl_writeJPEG(sal_uInt16 nBitmapId, const sal_uInt8* pJpgData, sal_uInt32 nJpgDataLength, sal_uInt8 const *pCompressed, sal_uInt32 compressed_size );
    void Impl_handleLineInfoPolyPolygons(const LineInfo& rInfo, const basegfx::B2DPolygon& rLinePolygon);
    void Impl_writeActions( const GDIMetaFile& rMtf );
    void Impl_writePolygon( const tools::Polygon& rPoly, bool bFilled );
    void Impl_writePolygon( const tools::Polygon& rPoly, bool bFilled, const Color& rFillColor, const Color& rLineColor );
    void Impl_writePolyPolygon( const tools::PolyPolygon& rPolyPoly, bool bFilled, sal_uInt8 nTransparence = 0);
    void Impl_writePolyPolygon( const tools::PolyPolygon& rPolyPoly, bool bFilled, const Color& rFillColor, const Color& rLineColor );
    void Impl_writeText( const Point& rPos, const OUString& rText, const long* pDXArray, long nWidth );
    void Impl_writeText( const Point& rPos, const OUString& rText, const long* pDXArray, long nWidth, Color aTextColor );
    void Impl_writeGradientEx( const tools::PolyPolygon& rPolyPoly, const Gradient& rGradient );
    void Impl_writeLine( const Point& rPt1, const Point& rPt2, const Color* pLineColor = nullptr );
    void Impl_writeRect( const tools::Rectangle& rRect, long nRadX, long nRadY );
    void Impl_writeEllipse( const Point& rCenter, long nRadX, long nRadY );
    bool Impl_writeFilling( SvtGraphicFill const & rFilling );
    bool Impl_writeStroke( SvtGraphicStroke const & rStroke );

    FlashFont& Impl_getFont( const vcl::Font& rFont );

    static void Impl_addPolygon( BitStream& rBits, const tools::Polygon& rPoly, bool bFilled );

    static void Impl_addShapeRecordChange( BitStream& rBits, sal_Int16 dx, sal_Int16 dy, bool bFilled );
    static void Impl_addStraightEdgeRecord( BitStream& rBits, sal_Int16 dx, sal_Int16 dy );
    static void Impl_addCurvedEdgeRecord( BitStream& rBits, sal_Int16 control_dx, sal_Int16 control_dy, sal_Int16 anchor_dx, sal_Int16 anchor_dy );
    static void Impl_addEndShapeRecord( BitStream& rBits );

    static void Impl_addStraightLine( BitStream& rBits,
                                  Point& rLastPoint,
                                  const double P2x, const double P2y );
    static void Impl_addQuadBezier( BitStream& rBits,
                                Point& rLastPoint,
                                const double P2x, const double P2y,
                                const double P3x, const double P3y );
    static void Impl_quadBezierApprox( BitStream& rBits,
                                   Point& rLastPoint,
                                   const double d2,
                                   const double P1x, const double P1y,
                                   const double P2x, const double P2y,
                                   const double P3x, const double P3y,
                                   const double P4x, const double P4y );

    css::uno::Reference < css::i18n::XBreakIterator > const & Impl_GetBreakIterator();

private:
    css::uno::Reference< css::i18n::XBreakIterator > mxBreakIterator;

    std::vector<std::unique_ptr<FlashFont>> maFonts;

    sal_Int32 mnDocWidth;
    sal_Int32 mnDocHeight;

    // AS: Scaling factor for output.
    double mnDocXScale;
    double mnDocYScale;

    sal_uInt16 mnPageButtonId;

    VclPtrInstance<VirtualDevice> mpVDev;

    const tools::PolyPolygon* mpClipPolyPolygon;

    /** holds the information of the objects defined in the movie stream
        while executing defineShape
    */
    std::vector<sal_uInt16> maShapeIds;

    std::unique_ptr<Tag> mpTag;
    std::unique_ptr<Sprite> mpSprite;
    std::stack<Sprite*> mvSpriteStack;
    ChecksumCache mBitmapCache;

    sal_uInt16 mnNextId;
    sal_uInt32  mnFrames;

    utl::TempFile maMovieTempFile;
    utl::TempFile maFontsTempFile;

    SvStream* mpMovieStream;
    SvStream* mpFontsStream;

    sal_uInt8 mnGlobalTransparency;
    sal_Int32 mnJPEGCompressMode;
};


}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
