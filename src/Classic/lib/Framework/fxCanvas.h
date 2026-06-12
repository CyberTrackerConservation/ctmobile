#pragma once
#include "pch.h"
#include "fxTypes.h"
#include "fxClasses.h"

struct CanvasState
{
    int LineX = 0, LineY = 0;
    int LineWidth = 0;
    COLOR LineColor = 0, BrushColor = 0, TextColor = 0;
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
    char Name[64] = {};
    UINT16 Height = 0;
    UINT16 Style = 0;
    void *Handle = nullptr;
};

//
// Canvas
//
class CfxCanvas: public CfxPersistent
{
private:
    QPainter _painter;
    QImage _image;

    void InvertBlock(int X1, int Y1, int X2, int Y2);
    void LoadFromFile(const char *pFileName, int Width, int Height);
    void *MakeFont(const char *pName, UINT16 Height, UINT16 Style);
    void FreeFont(void *Handle);

protected:
    CfxStream _renderBuffer;
    int _width = 0, _height = 0;
    BYTE _bitDepth = 0;
    TList<FX_FONT_CACHE_ITEM> _fontCache;
    int _scaleFont = 0;

    BYTE _cornerOffsets[MAX_CORNER_WIDTH + 1] = {};
    bool ClipRect(FXRECT *pRect);
    bool ClipLine(FXLINE *pLine);
    bool ClipPixel(int X, int Y);
public:
    CanvasState State;

    CfxCanvas(CfxPersistent *pOwner, int ScaleFont);
    virtual ~CfxCanvas();

    QImage *GetImage() { return &_image; }

    int GetWidth()         { return _width;    }
    int GetHeight()        { return _height;   }
    BYTE GetBitDepth()      { return _bitDepth; }

    void SetClipRect(FXRECT *pRect);
    void ClearClipRect();
    void SetBitDepth(BYTE BitDepth);
    COLOR InvertColor(COLOR Color, bool Invert = true);

    // Override per platform
    void BeginPaint() {}
    void EndPaint()   {}

    void SetSize(int Width, int Height);
    COLOR GetPixel(int X, int Y);
    void SetPixel(int X, int Y, COLOR Color);
    void SetBlock(int X1, int Y1, int X2, int Y2, COLOR Color);
    void CopyRect(CfxCanvas *pCanvas, int X1, int Y1, int X2, int Y2, int DX1, int DY1);
    void CopyRect(FXBITMAPRESOURCE *pBitmap, int X1, int Y1, int X2, int Y2, int DX1, int DY1, bool Invert);
    void StretchCopyRect(CfxCanvas *pCanvas, int X1, int Y1, int X2, int Y2, int DX1, int DY1, int DX2, int DY2);
    void StretchCopyRect(FXBITMAPRESOURCE *pBitmap, int X1, int Y1, int X2, int Y2, int DX1, int DY1, int DX2, int DY2, bool Invert, bool Transparent);
    void SystemDrawText(const char *pText, FXRECT *pRect, COLOR Color, COLOR BkColor);
    void ImportLine(int X, int Y, COLOR *pBuffer, int Size);
    FXBITMAPRESOURCE *CreateBitmapFromFile(const char *pFileName, int Width, int Height);

    // Static shapes
    void DrawFirstArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawLastArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawBackArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawNextArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawUpArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawDownArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawSkipArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawTriangle(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Fill, bool Invert, bool Transparent = false);
    void DrawTarget(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, int Heading, bool Transparent = false);
    void DrawCompass(FXRECT *pRect, FXFONTRESOURCE *pFont, COLOR BackColor, COLOR ForeColor, bool Invert, int Heading, bool Transparent = false);
    void DrawFlag(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Fill, bool Invert, bool Transparent = false, bool Center = true);
    void DrawCross(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, int Thickness, bool Invert, bool Transparent = false);
    void DrawCheck(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawAction(FXRECT *pRect);
    void DrawPlay(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawPort(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawPortState(FXRECT *pRect, FXFONTRESOURCE *pFont, int PortId, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent);
    void DrawRecord(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawStop(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawHome(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawRange(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawBearing(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawInclination(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawRoll(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawUpload(bool Filled, FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);
    void DrawHeading(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, int Heading, bool Transparent = false);
    void DrawBusy(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert);
    void DrawShare(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent = false);

    // Dynamic shapes
    int DrawTime(FXDATETIME *pDateTime, FXRECT *pRect, FXFONTRESOURCE *pFont, int AlignFlags, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent);
    int DrawBattery(int Level, FXRECT *pRect, int AlignFlags, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent);

    //
    // Clipped drawing functions
    //

    // Lines
    void SetLinePixel(int X, int Y);
    void Line(int X1, int Y1, int X2, int Y2);
    void MoveTo(int X, int Y);
    void LineTo(int X, int Y);

    // Rectangle
    void Rectangle(int Left, int Top, int Width, int Height);
    void FillRect(int Left, int Top, int Width, int Height);
    void FrameRect(int Left, int Top, int Width, int Height);

    void Rectangle(FXRECT *pRect);
    void FillRect(FXRECT *pRect);
    void FrameRect(FXRECT *pRect);

    // Text
    int GetScaleFont()           { return _scaleFont;  }
    void SetScaleFont(int Value) { _scaleFont = Value; }
    void *FindFont(FXFONTRESOURCE *pFont);
    int FontHeight(FXFONTRESOURCE *pFont);
    int TextWidth(FXFONTRESOURCE *pFont, const char *pText, int Length=0);
    int TextHeight(FXFONTRESOURCE *pFont, const char *pText, int Length=0, int *pOffsetY=nullptr);
    void CalcTextRect(FXFONTRESOURCE *pFont, const char *pText, FXRECT *pRect);
    void DrawText(FXFONTRESOURCE *pFont, int X, int Y, const char *pText, int Length=0);
    void DrawTextRect(FXFONTRESOURCE *pFont, FXRECT *pRect, int AlignFlags, const char *pText, int Length = 0);
    void DrawTextRect(FXFONTRESOURCE *pFont, FXRECT *pRect, int AlignFlags, const QString &Text);

    // Bitmap
    void Draw(CfxCanvas *pCanvas, int X, int Y);
    void Draw(FXBITMAPRESOURCE *pBitmap, int X, int Y, bool Invert=false);
    void StretchDraw(FXBITMAPRESOURCE *pBitmap, int X, int Y, int Width, int Height, bool Invert=false, bool Transparent=false);
    void StretchDraw(FXBITMAPRESOURCE *pBitmap, FXRECT *pSrcRect, FXRECT *pDstRect, bool Invert=false, bool Transparent=false);

    // Triangle
    void FillTriangle(int X1, int Y1, int X2, int Y2, int X3, int Y3);
    void Triangle(int X1, int Y1, int X2, int Y2, int X3, int Y3);

    // Polygon
    void FillPolygon(FXPOINTLIST *VertexList, int X, int Y);
    void Polygon(FXPOINTLIST *VertexList, int X, int Y);

    // Ellipse
    void FillEllipse(int Xc, int Yc, int RadiusX, int RadiusY);
    void Ellipse(int Xc, int Yc, int RadiusX, int RadiusY);

    // RoundRect
    void RoundRect(int Left, int Top, int Width, int Height, int CornerWidth);
    void RoundRect(FXRECT *pRect, int CornerWidth);
    void FillRoundRect(int Left, int Top, int Width, int Height, int CornerWidth);
    void FillRoundRect(FXRECT *pRect, int CornerWidth);

    // Control Corners
    void PushCorners(FXRECT *pRect, int CornerWidth);
    void PopCorners(FXRECT *pRect);
};


