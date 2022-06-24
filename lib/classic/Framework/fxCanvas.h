/*************************************************************************************************/
//                                                                                                *
//   Copyright (C) 2005              Software                                                     *
//                                                                                                *
//   Original author: Justin Steventon (justin@steventon.com)                                     *
//                                                                                                *
//   You may retrieve the latest version of this file via the              Software home page,    *
//   located at: http://www.            .com.                                                     *
//                                                                                                *
//   The contents of this file are used subject to the Mozilla Public License Version 1.1         *
//   (the "License"); you may not use this file except in compliance with the License. You may    *
//   obtain a copy of the License from http://www.mozilla.org/MPL.                                *
//                                                                                                *
//   Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY  *
//   OF ANY KIND, either express or implied. See the License for the specific language governing  *
//   rights and limitations under the License.                                                    *
//                                                                                                *
//************************************************************************************************/

#pragma once
#include "pch.h"
#include "fxTypes.h"
#include "fxClasses.h"

struct CanvasState
{
    INT LineX, LineY;
    INT LineWidth;
    COLOR LineColor, BrushColor, TextColor;
    FXRECT ClipRect;
};

// Text alignment flags
#define TEXT_ALIGN_NONE     0
#define TEXT_ALIGN_LEFT     0 
#define TEXT_ALIGN_TOP      0
#define TEXT_ALIGN_RIGHT    1
#define TEXT_ALIGN_HCENTER  2
#define TEXT_ALIGN_BOTTOM   4
#define TEXT_ALIGN_VCENTER  8
#define TEXT_ALIGN_WORDWRAP 16

#define MAX_CORNER_WIDTH    64

// Draws the polygon described by the point list PointList in color
#define POLYGON(pCanvas, PointList, X, Y)                 \
   polygon.Length = sizeof(PointList) / sizeof(FXPOINT);  \
   polygon.Points = PointList;                            \
   pCanvas->Polygon(&polygon, X, Y);

#define FILLPOLYGON(pCanvas, PointList, X, Y)             \
   polygon.Length = sizeof(PointList) / sizeof(FXPOINT);  \
   polygon.Points = PointList;                            \
   pCanvas->FillPolygon(&polygon, X, Y);

struct FX_FONT_CACHE_ITEM
{
    CHAR Name[64];
    UINT16 Height;
    UINT16 Style;
    VOID *Handle;
};

//
// Canvas
//
class CfxCanvas: public CfxPersistent
{
protected:
    CfxStream _renderBuffer;
    UINT _width, _height;
    BYTE _bitDepth;
    TList<FX_FONT_CACHE_ITEM> _fontCache;
    UINT _scaleFont;

    BYTE _cornerOffsets[MAX_CORNER_WIDTH + 1];
    BOOL ClipRect(FXRECT *pRect);
    BOOL ClipLine(FXLINE *pLine);
    BOOL ClipPixel(INT X, INT Y);
    BOOL AdvanceTextLine(FXFONTRESOURCE *pFont, UINT LineWidth, CHAR **ppText, UINT *pCharCount);
    virtual VOID *MakeFont(CHAR *pName, UINT16 Height, UINT16 Style) { return NULL; }
    virtual VOID FreeFont(VOID *Handle) { }
public:
    CanvasState State;

    CfxCanvas(CfxPersistent *pOwner, BYTE BitDepth, UINT ScaleFont);
    virtual ~CfxCanvas();

    UINT GetWidth()         { return _width;    }
    UINT GetHeight()        { return _height;   }
    BYTE GetBitDepth()      { return _bitDepth; }

    VOID SetClipRect(FXRECT *pRect);  
    VOID ClearClipRect();
    VOID SetBitDepth(BYTE BitDepth);
    COLOR InvertColor(COLOR Color, BOOL Invert = TRUE);
    
    // Override per platform
    virtual VOID BeginPaint() {}
    virtual VOID EndPaint()   {}

    virtual VOID SetSize(UINT Width, UINT Height)=0;
    virtual COLOR GetPixel(INT X, INT Y)=0;
    virtual VOID SetPixel(INT X, INT Y, COLOR Color)=0;
    virtual VOID SetBlock(INT X1, INT Y1, INT X2, INT Y2, COLOR Color)=0;
    virtual VOID CopyRect(CfxCanvas *pCanvas, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1)=0;
    virtual VOID CopyRect(FXBITMAPRESOURCE *pBitmap, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1, BOOL Invert)=0;
    virtual VOID StretchCopyRect(CfxCanvas *pCanvas, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1, INT DX2, INT DY2)=0;
    virtual VOID StretchCopyRect(FXBITMAPRESOURCE *pBitmap, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1, INT DX2, INT DY2, BOOL Invert, BOOL Transparent)=0;
    virtual VOID SystemDrawText(CHAR *pText, FXRECT *pRect, COLOR Color, COLOR BkColor)=0;
    virtual VOID ImportLine(INT X, INT Y, COLOR *pBuffer, INT Size)=0;
    virtual FXBITMAPRESOURCE *CreateBitmapFromFile(CHAR *pFileName, UINT Width, UINT Height) { return NULL; }

    // Static shapes
    VOID DrawFirstArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawLastArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawBackArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawNextArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawUpArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawDownArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawSkipArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawTriangle(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Fill, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawTarget(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, INT Heading, BOOL Transparent = FALSE);
    VOID DrawCompass(FXRECT *pRect, FXFONTRESOURCE *pFont, COLOR BackColor, COLOR ForeColor, BOOL Invert, INT Heading, BOOL Transparent = FALSE);
    VOID DrawFlag(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Fill, BOOL Invert, BOOL Transparent = FALSE, BOOL Center = TRUE);
    VOID DrawCross(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, UINT Thickness, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawCheck(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawAction(FXRECT *pRect);
    VOID DrawPlay(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawPort(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawPortState(FXRECT *pRect, FXFONTRESOURCE *pFont, UINT PortId, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent);
    VOID DrawRecord(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawStop(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawHome(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawRange(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawBearing(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawInclination(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawRoll(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawUpload(BOOL Filled, FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent = FALSE);
    VOID DrawHeading(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, INT Heading, BOOL Transparent = FALSE);
    VOID DrawBusy(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert); 

    // Dynamic shapes
    UINT DrawTime(FXDATETIME *pDateTime, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT AlignFlags, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent);
    UINT DrawBattery(UINT Level, FXRECT *pRect, UINT AlignFlags, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent);

    //
    // Clipped drawing functions
    //

    // Lines
    VOID SetLinePixel(INT X, INT Y);
    virtual VOID Line(INT X1, INT Y1, INT X2, INT Y2);
    VOID MoveTo(INT X, INT Y); 
    VOID LineTo(INT X, INT Y); 

    // Rectangle
    VOID Rectangle(INT Left, INT Top, UINT Width, UINT Height);
    VOID FillRect(INT Left, INT Top, UINT Width, UINT Height);
    VOID FrameRect(INT Left, INT Top, UINT Width, UINT Height);

    virtual VOID Rectangle(FXRECT *pRect); 
    virtual VOID FillRect(FXRECT *pRect);
    virtual VOID FrameRect(FXRECT *pRect);

    // Text
    UINT GetScaleFont()           { return _scaleFont;  }
    VOID SetScaleFont(UINT Value) { _scaleFont = Value; }
    VOID *FindFont(FXFONTRESOURCE *pFont);
    virtual UINT FontHeight(FXFONTRESOURCE *pFont);
    virtual UINT TextWidth(FXFONTRESOURCE *pFont, CHAR *pText, UINT Length=0);
    virtual UINT TextHeight(FXFONTRESOURCE *pFont, CHAR *pText, UINT Length=0, INT *pOffsetY=NULL);
    virtual VOID CalcTextRect(FXFONTRESOURCE *pFont, CHAR *pText, FXRECT *pRect);
    virtual VOID DrawText(FXFONTRESOURCE *pFont, INT X, INT Y, CHAR *pText, UINT Length=0); 
    virtual VOID DrawTextRect(FXFONTRESOURCE *pFont, FXRECT *pRect, UINT AlignFlags, CHAR *pText, UINT Length = 0);

    // Bitmap
    VOID Draw(CfxCanvas *pCanvas, INT X, INT Y);
    VOID Draw(FXBITMAPRESOURCE *pBitmap, INT X, INT Y, BOOL Invert=FALSE);
    VOID StretchDraw(FXBITMAPRESOURCE *pBitmap, INT X, INT Y, UINT Width, UINT Height, BOOL Invert=FALSE, BOOL Transparent=FALSE);
    VOID StretchDraw(FXBITMAPRESOURCE *pBitmap, FXRECT *pSrcRect, FXRECT *pDstRect, BOOL Invert=FALSE, BOOL Transparent=FALSE);

    // Triangle
    virtual VOID FillTriangle(INT X1, INT Y1, INT X2, INT Y2, INT X3, INT Y3);
    virtual VOID Triangle(INT X1, INT Y1, INT X2, INT Y2, INT X3, INT Y3);

    // Polygon
    VOID FillPolygon(FXPOINTLIST *VertexList, INT X, INT Y);
    VOID Polygon(FXPOINTLIST *VertexList, INT X, INT Y);

    // Ellipse
    virtual VOID FillEllipse(INT Xc, INT Yc, UINT RadiusX, UINT RadiusY);
    virtual VOID Ellipse(INT Xc, INT Yc, UINT RadiusX, UINT RadiusY);

    // RoundRect
    VOID RoundRect(INT Left, INT Top, UINT Width, UINT Height, UINT CornerWidth);
    VOID RoundRect(FXRECT *pRect, UINT CornerWidth); 
    VOID FillRoundRect(INT Left, INT Top, UINT Width, UINT Height, UINT CornerWidth);
    VOID FillRoundRect(FXRECT *pRect, UINT CornerWidth);

    // Control Corners
    VOID PushCorners(FXRECT *pRect, UINT CornerWidth);
    VOID PopCorners(FXRECT *pRect);
};


