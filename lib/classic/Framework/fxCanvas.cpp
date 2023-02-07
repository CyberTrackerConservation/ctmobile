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

#include "fxUtils.h"
#include "fxCanvas.h"
#include "fxMath.h"

CfxCanvas::CfxCanvas(CfxPersistent *pOwner, BYTE BitDepth, UINT ScaleFont): CfxPersistent(pOwner), _renderBuffer(), _fontCache() 
{ 
    memset(&State, 0, sizeof(State)); 
    _width = _height = 0;
    _bitDepth = BitDepth;
    _scaleFont = ScaleFont;
    State.LineWidth = 1;
}

CfxCanvas::~CfxCanvas()
{
    for (UINT i = 0; i < _fontCache.GetCount(); i++)
    {
        FreeFont(_fontCache.GetPtr(i)->Handle);
    }
}

VOID *CfxCanvas::FindFont(FXFONTRESOURCE *pFont)
{
    FX_FONT_CACHE_ITEM *currItem = NULL;

    for (UINT i = 0; i < _fontCache.GetCount(); i++, currItem++)
    {
        if (currItem == NULL)
        {
            currItem = _fontCache.GetPtr(0);
        }

        if (currItem->Height != FontHeight(pFont)) 
        {
            continue;
        }

        if (currItem->Style != pFont->Style) 
        {
            continue;
        }

        if (strcmp(currItem->Name, pFont->Name) != 0)
        {
            continue;
        }

        return currItem->Handle;
    }

    FX_FONT_CACHE_ITEM newItem = {0};
    strcpy(newItem.Name, pFont->Name);
    newItem.Height = (UINT16)FontHeight(pFont);
    newItem.Style = pFont->Style;
    newItem.Handle = MakeFont(pFont->Name, newItem.Height, pFont->Style);

    if (newItem.Handle != NULL)
    {
        _fontCache.Add(newItem);
    }

    return newItem.Handle;
}

BOOL CfxCanvas::ClipRect(FXRECT *pRect)
{
    return IntersectRect(pRect, pRect, &State.ClipRect);
}

VOID CfxCanvas::SetBitDepth(BYTE BitDepth)
{
    UINT width = _width;
    UINT height = _height;

    SetSize(0, 0);
    _bitDepth = BitDepth;
    SetSize(width, height);
}

COLOR CfxCanvas::InvertColor(COLOR Color, BOOL Invert)
{ 
    if (_bitDepth >= 16)
    {
        return Invert ? Color ^ 0xFFFFFF : Color;
    }

    // Monochrome color invert
    INT intensity = (Color & 0xFF) + ((Color & 0xFF00) >> 8) + ((Color & 0xFF0000) >> 16);
    if (Invert)
    {
        return (intensity < 382) ? 0xFFFFFF : 0x0;
    }
    else
    {
        return (intensity < 382) ? 0 : 0xFFFFFF;
    }
}

//*************************************************************************************************
// Pixel

BOOL CfxCanvas::ClipPixel(INT X, INT Y)
{
    return ((X >= State.ClipRect.Left) && (X <= State.ClipRect.Right) &&
            (Y >= State.ClipRect.Top) && (Y <= State.ClipRect.Bottom));
}

VOID CfxCanvas::SetLinePixel(INT X, INT Y)
{
    switch (State.LineWidth)
    {
    case 0: return;
    case 1: 
        SetPixel(X, Y, State.LineColor); 
        break;
    default:
        {
            COLOR lastBrushColor = State.BrushColor;
            State.BrushColor = State.LineColor;
            FillEllipse(X, Y, State.LineWidth/2, State.LineWidth/2);
            State.BrushColor = lastBrushColor;
        }
        break;
    }        
}

//*************************************************************************************************
// Line

union OutcodeUnion
{  
    struct
    {
        unsigned code0 : 1;
        unsigned code1 : 1;
        unsigned code2 : 1;
        unsigned code3 : 1;
    } ocs;
    int outcodes;
};

#define XUL pRect->Left
#define YUL pRect->Top
#define XLR pRect->Right
#define YLR pRect->Bottom

VOID SetOutcodes(union OutcodeUnion *pU, FXRECT *pRect, INT X, INT Y)
{
    pU->outcodes = 0;
    pU->ocs.code0 = (X < XUL);
    pU->ocs.code1 = (Y < YUL);
    pU->ocs.code2 = (X > XLR);
    pU->ocs.code3 = (Y > YLR);
}

BOOL CfxCanvas::ClipLine(FXLINE *pLine)
{
    INT X1 = pLine->X1;
    INT Y1 = pLine->Y1;
    INT X2 = pLine->X2;
    INT Y2 = pLine->Y2;

    union OutcodeUnion ocu1, ocu2;
    BOOL inside, outside;
    FXRECT *pRect = &State.ClipRect;
    
    SetOutcodes(&ocu1, pRect, X1, Y1);
    SetOutcodes(&ocu2, pRect, X2, Y2);

    inside  = ((ocu1.outcodes | ocu2.outcodes) == 0);
    outside = ((ocu1.outcodes & ocu2.outcodes) != 0);

    while (!outside && !inside)
    {
        if (ocu1.outcodes == 0)
        {
            Swap(&X1, &X2);
            Swap(&Y1, &Y2);
            
            union OutcodeUnion tocu = ocu1;
            ocu1 = ocu2;
            ocu2 = tocu;
        }

        if (ocu1.ocs.code0)         // Clip left
        {
            Y1 += (Y2 - Y1) * (XUL - X1) / (X2 - X1);
            X1 = XUL;
        }
        else if (ocu1.ocs.code1)    // Clip above
        {
            X1 += (X2 - X1) * (YUL - Y1) / (Y2 - Y1);
            Y1 = YUL;
        }
        else if (ocu1.ocs.code2)    // Clip right
        {
            Y1 += (Y2 - Y1) * (XLR - X1) / (X2 - X1);
            X1 = XLR;
        }
        else if (ocu1.ocs.code3)
        {
            X1 += (X2 - X1) * (YLR - Y1) / (Y2 - Y1);
            Y1 = YLR;
        }

        SetOutcodes(&ocu1, pRect, X1, Y1);

        inside  = ((ocu1.outcodes | ocu2.outcodes) == 0);
        outside = ((ocu1.outcodes & ocu2.outcodes) != 0);
    }

    pLine->X1 = X1;
    pLine->Y1 = Y1;
    pLine->X2 = X2;
    pLine->Y2 = Y2;

    return inside;    
}

VOID CfxCanvas::Line(INT X1, INT Y1, INT X2, INT Y2)
{
    FXLINE line = {X1, Y1, X2, Y2};
    if (!ClipLine(&line)) return;

    X1 = line.X1;
    Y1 = line.Y1;
    X2 = line.X2;
    Y2 = line.Y2;

    // Vertical line
    if (X1 == X2)
    {
        if (Y1 > Y2) Swap(&Y1, &Y2);
        COLOR lastBrushColor = State.BrushColor;
        State.BrushColor = State.LineColor;
        FillRect(X1, Y1, State.LineWidth, Y2-Y1+1);
        State.BrushColor = lastBrushColor;
        return;
    }

    if (X1 > X2)
    {
        Swap(&X1, &X2);
        Swap(&Y1, &Y2);
    }

    // Horizontal line
    if (Y1 == Y2)
    {
        if (X1 > X2) Swap(&X1, &X2);
        COLOR lastBrushColor = State.BrushColor;
        State.BrushColor = State.LineColor;
        FillRect(X1, Y1, X2-X1+1, State.LineWidth);
        State.BrushColor = lastBrushColor;
        return;
    }

    // All other lines
    INT dx = X2 - X1;
    INT dy = (INT)FxAbs((INT)Y2 - (INT)Y1);
    INT sy = (Y2 > Y1) ? 1 : -1;

    INT dx2 = dx << 1;
    INT dy2 = dy << 1;

    if (dy <= dx)           
    { 
        INT e = dy2 - dx;

        for (INT i=0; i<=dx; ++i) 
        {
            SetLinePixel(X1, Y1);

            while (e >= 0) 
            {
                Y1 += sy;
                e -= dx2;
            }

            X1++;
            e += dy2;
        }
    } 
    else 
    {
        INT e = dx2 - dy;

        for (INT i=0; i<=dy; ++i) 
        {
            SetLinePixel(X1, Y1);

            while (e >= 0) 
            {
                X1++;
                e -= dy2;
            }

            Y1 += sy;
            e += dx2;
        }
    }
} 

VOID CfxCanvas::MoveTo(INT X, INT Y)
{
    State.LineX = X;
    State.LineY = Y;
} 

VOID CfxCanvas::LineTo(INT X, INT Y)
{
    Line(State.LineX, State.LineY, X, Y);
    MoveTo(X, Y);
} 

//*************************************************************************************************
// Clip Rectangle

VOID CfxCanvas::SetClipRect(FXRECT *pRect)  
{
    ClearClipRect();
    IntersectRect(&State.ClipRect, pRect, &State.ClipRect);
} 

VOID CfxCanvas::ClearClipRect()           
{
    State.ClipRect.Left = State.ClipRect.Top = 0;
    State.ClipRect.Right = _width - 1;
    State.ClipRect.Bottom = _height - 1;
} 

//*************************************************************************************************
// Static shapes

VOID CfxCanvas::DrawFirstArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    State.BrushColor = InvertColor(ForeColor, Invert);

    INT w1 = pRect->Right - pRect->Left + 1;
    INT w2 = w1 / 2 - 2;
    INT h1 = pRect->Bottom - pRect->Top + 1;
    INT h2 = h1 / 2 - 2;

    INT h = min(h2, w2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{h, 0}, {h, h*2}, {0, h}};

    INT x = pRect->Left + (w1 - h) / 2 + h/2;
    INT y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);

    FXRECT rect = {x - h, y, x-1, y + h*2};
    FillRect(&rect);
}

VOID CfxCanvas::DrawBackArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    State.BrushColor = InvertColor(ForeColor, Invert);

    INT w1 = pRect->Right - pRect->Left + 1;
    INT w2 = w1 / 2 - 2;
    INT h1 = pRect->Bottom - pRect->Top + 1;
    INT h2 = h1 / 2 - 2;

    INT h = min(h2, w2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{h, 0}, {h, h*2}, {0, h}};

    INT x = pRect->Left + (w1 - h) / 2;
    INT y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);
}

VOID CfxCanvas::DrawNextArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    INT w1 = pRect->Right - pRect->Left + 1;
    INT w2 = w1 / 2 - 2;
    INT h1 = pRect->Bottom - pRect->Top + 1;
    INT h2 = h1 / 2 - 2;

    INT h = min(h2, w2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{0, 0}, {h, h}, {0, h*2}};

    INT x = pRect->Left + (w1 - h) / 2;
    INT y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);
}

VOID CfxCanvas::DrawPlay(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    INT w1 = pRect->Right - pRect->Left + 1;
    INT h1 = pRect->Bottom - pRect->Top + 1;
    INT h = min(w1 / 2 - 2, h1 / 2 - 2);

    State.BrushColor = InvertColor(ForeColor, Invert);
    FillRect(pRect);
    State.BrushColor = InvertColor(BackColor, Invert);

    h = min(h1 / 3 - 2, w1 / 3 - 2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{0, 0}, {h, h}, {0, h*2}};
    INT x = pRect->Left + w1 / 2 - h / 3;
    INT y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);
}

VOID CfxCanvas::DrawPort(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);
    FillRect(pRect);
    State.BrushColor = InvertColor(BackColor, Invert);

    FXRECT rect;
    rect = *pRect;
    INT w = rect.Right - rect.Left + 1;
    INT h = rect.Bottom - rect.Top + 1;
    INT t = min(w, h);

    InflateRect(&rect, - t / 5, - t / 5); 
    w = rect.Right - rect.Left + 1;
    h = rect.Bottom - rect.Top + 1;
    DOUBLE h5 = h / 5;

    FillRect(rect.Left, rect.Top, w, (INT)h5);
    FillRect(rect.Left, rect.Top + (INT)h5 * 2, w, (INT)h5);
    FillRect(rect.Left, rect.Top + (INT)h5 * 4, w, (INT)h5);
}

VOID CfxCanvas::DrawPortState(FXRECT *pRect, FXFONTRESOURCE *pFont, UINT PortId, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    INT w1 = pRect->Right - pRect->Left + 1;
    INT h1 = pRect->Bottom - pRect->Top + 1;
    INT h = min(w1 / 2 - 2, h1 / 2 - 2);

    INT xc = pRect->Left + w1 / 2;
    INT yc = pRect->Top + h1 / 2;
    h = h + State.LineWidth * 4;
    State.LineColor = InvertColor(ForeColor, Invert);
    if (Invert)
    {
        State.BrushColor = ForeColor;
        FillEllipse(xc, yc, h, h);
    }
    else
    {
        State.LineColor = ForeColor;
        Ellipse(xc, yc, h, h);
    }

    CHAR portIdText[8];
    sprintf(portIdText, "%lu", PortId);
    
    INT offsetY;
    UINT textH = TextHeight(pFont, portIdText, 0, &offsetY);
    UINT textW = TextWidth(pFont, portIdText);

    State.TextColor = InvertColor(ForeColor, Invert);

    FXRECT rText = *pRect;
    rText.Top = yc - offsetY - textH / 2;
    rText.Bottom = rText.Top + textH;
    rText.Left = xc - textW / 2 + 1;
    rText.Right = rText.Left + textW;

    DrawText(pFont, rText.Left, rText.Top, portIdText);
}

VOID CfxCanvas::DrawRecord(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    INT w1 = pRect->Right - pRect->Left + 1;
    INT h1 = pRect->Bottom - pRect->Top + 1;
    INT h = min(w1 / 2 - 2, h1 / 2 - 2);

    State.BrushColor = InvertColor(ForeColor, Invert);
    INT xc = pRect->Left + w1 / 2;
    INT yc = pRect->Top + h1 / 2;
    FillRect(pRect);
    State.BrushColor = InvertColor(BackColor, Invert);

    h = min(h1 / 4 - 2, w1 / 4 - 2);
    FillEllipse(xc, yc, h, h); 

    h = h + State.LineWidth * 4;
    State.LineColor = InvertColor(BackColor, Invert);
    Ellipse(xc, yc, h, h);
}

VOID CfxCanvas::DrawStop(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(BackColor, !Invert);

    INT w1 = pRect->Right - pRect->Left + 1;
    INT h1 = pRect->Bottom - pRect->Top + 1;
    INT h = min(w1 / 2 - 2, h1 / 2 - 2);

    State.BrushColor = InvertColor(BackColor, !Invert);
    INT xc = pRect->Left + w1 / 2;
    INT yc = pRect->Top + h1 / 2;
    //FillEllipse(xc, yc, h, h); 
    FillRect(pRect);
    State.BrushColor = InvertColor(BackColor, Invert);

    h = min(h1 / 4 - 2, w1 / 4 - 2);
    FillRect(xc - h, yc - h, h * 2 + 1, h * 2 + 1);
}

VOID CfxCanvas::DrawLastArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    INT w1 = pRect->Right - pRect->Left + 1;
    INT w2 = w1 / 2 - 2;
    INT h1 = pRect->Bottom - pRect->Top + 1;
    INT h2 = h1 / 2 - 2;

    INT h = min(h2, w2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{0, 0}, {h, h}, {0, h*2}};

    INT x = pRect->Left + (w1 - h) / 2 - h/2;
    INT y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);

    FXRECT rect = {x + h + 1, y, x + h*2, y + h*2};
    FillRect(&rect);
}

VOID CfxCanvas::DrawUpArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    INT w1 = pRect->Right - pRect->Left - 1;
    INT w2 = (w1 - 1)  / 2;
    INT h1 = pRect->Bottom - pRect->Top - 1;
    INT h2 = (h1 - 1) / 2;
    INT h = min(h2, w2);

    for (INT i=0; i<=h; i++)
    {
        FillRect(pRect->Left + w2 - i + 1, pRect->Top + h1/2 - h/2 + i, i * 2 + 1, 1);
    }
}

VOID CfxCanvas::DrawDownArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    INT w1 = pRect->Right - pRect->Left - 1;
    INT w2 = (w1 - 1)  / 2;
    INT h1 = pRect->Bottom - pRect->Top - 1;
    INT h2 = (h1 - 1) / 2;
    INT h = min(h2, w2);

    for (INT i=0; i<=h; i++)
    {
        FillRect(pRect->Left + w2 - i + 1, pRect->Top + h1/2 + h/2 - i, i * 2 + 1, 1);
    }
}

VOID CfxCanvas::DrawSkipArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    INT w1 = pRect->Right - pRect->Left + 1;
    INT w2 = w1 / 2 - 2;
    INT h1 = pRect->Bottom - pRect->Top + 1;
    INT h2 = h1 / 2 - 2;

    INT h = min(h2, w2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{0, 0}, {h, h}, {0, h*2}};

    INT x = pRect->Left + (w1 - h) / 2 + (h / 3);
    INT y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);
    x -= (h * 2) / 3;
    FILLPOLYGON(this, triangle, x, y);
}

VOID CfxCanvas::DrawTriangle(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Fill, BOOL Invert, BOOL Transparent)
{
    State.BrushColor = InvertColor(BackColor, Invert);
    if (!Transparent)
    {
        FillRect(pRect);
    }
    State.LineColor = InvertColor(ForeColor, Invert);

    FXRECT rect = *pRect;
    InflateRect(&rect, -(INT)(State.LineWidth-1), -(INT)(State.LineWidth-1));

    INT width = rect.Right - rect.Left + 1;
    INT height = rect.Bottom - rect.Top + 1;
    INT h = min(height / 2 - 2, width / 2 - 2);

    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{h, 0}, {h*2, h*2}, {0, h*2}};

    INT x = rect.Left + (width - h*2) / 2;
    INT y = rect.Top + height / 2 - h;

    if (Fill)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FILLPOLYGON(this, triangle, x, y);
    }

    State.LineColor = InvertColor(ForeColor, Invert);
    POLYGON(this, triangle, x, y);
}

VOID CfxCanvas::DrawTarget(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, INT Heading, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    INT cx = pRect->Left + (pRect->Right - pRect->Left + 1) / 2;
    INT cy = pRect->Top + (pRect->Bottom - pRect->Top + 1) / 2;
    INT thick = State.LineWidth;
    INT thick2 = thick * 2;

    FXRECT rect = *pRect;
    UINT w = rect.Right - rect.Left + 1;
    UINT h = rect.Bottom - rect.Top + 1;
    INT t = min(w, h);
    INT t2 = t / 2;
    INT t4 = t / 4;
    State.LineColor = InvertColor(ForeColor, Invert);
    Ellipse(cx, cy, t2 - 1, t2 - 1);
    
    INT segLen = (t * 5 + 11) / 12;

    State.BrushColor = InvertColor(ForeColor, Invert);
    FillRect(cx - thick - segLen, cy, segLen, thick);
    FillRect(cx + thick2, cy, segLen, thick);
    FillRect(cx, cy - thick - segLen, thick, segLen);
    FillRect(cx, cy + thick2, thick, segLen);

    FXPOINTLIST polygon;
    FXPOINT tri[] = {{-t4, -t2 + thick}, {0, -t2 - t4 + thick}, {t4, -t2 + thick}}; 

    INT i;
    FXPOINT *p;

    DOUBLE headingRad = DEG2RAD(Heading);
    DOUBLE cosHeading = cos(headingRad);
    DOUBLE sinHeading = sin(headingRad);
    DOUBLE x, y;

    for (i = 0; i < ARRAYSIZE(tri); i++)
    {
        p = &tri[i];
        x = cosHeading * p->X - sinHeading * p->Y;
        y = sinHeading * p->X + cosHeading * p->Y;
        p->X = (INT)x;
        p->Y = (INT)y;
    }

    State.BrushColor = InvertColor(ForeColor, Invert);
    FILLPOLYGON(this, tri, cx, cy);
}

VOID CfxCanvas::DrawCompass(FXRECT *pRect, FXFONTRESOURCE *pFont, COLOR BackColor, COLOR ForeColor, BOOL Invert, INT Heading, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    INT textSpacer = (pFont != NULL) ? FontHeight(pFont) + 1 : 0;

    INT cx = pRect->Left + (pRect->Right - pRect->Left + 1) / 2;
    INT cy = pRect->Top + (pRect->Bottom - pRect->Top + 1) / 2;
    INT thick = State.LineWidth;
    INT thick2 = thick * 2;
    INT thick4 = thick * 4;
    INT thick8 = thick * 8;

    FXRECT rect = *pRect;
    InflateRect(&rect, -thick4, -thick4);
    InflateRect(&rect, -textSpacer, -textSpacer);
    UINT w = rect.Right - rect.Left + 1;
    UINT h = rect.Bottom - rect.Top + 1;
    INT t = min(w, h);
    INT t2 = t / 2;
    INT t4 = max(t / 8, 4);
    State.LineColor = InvertColor(ForeColor, Invert);
    Ellipse(cx, cy, t2 - 1, t2 - 1);
    
    INT segLen = (t * 5 + 11) / 12;

    State.BrushColor = InvertColor(ForeColor, Invert);
    //FillRect(cx - thick - segLen, cy, segLen, thick);
    //FillRect(cx + thick2, cy, segLen, thick);
    //FillRect(cx, cy - thick - segLen, thick, segLen);
    //FillRect(cx, cy + thick2, thick, segLen);

    FXPOINTLIST polygon;
    FXPOINT tri1[] = {{-t4, 0}, {0, t2 - thick8}, {t4, 0}}; 
    FXPOINT tri2[] = {{-t4, 0}, {0, -t2 + thick8}, {t4, 0}}; 

    INT i;
    FXPOINT *p;

    DOUBLE headingRad = DEG2RAD((360 + ((180 + 360) - Heading)) % 360);
    DOUBLE cosHeading = cos(headingRad);
    DOUBLE sinHeading = sin(headingRad);
    DOUBLE x, y;

    for (i = 0; i < ARRAYSIZE(tri1); i++)
    {
        p = &tri1[i];
        x = cosHeading * p->X - sinHeading * p->Y;
        y = sinHeading * p->X + cosHeading * p->Y;
        p->X = (INT)x;
        p->Y = (INT)y;

        p = &tri2[i];
        x = cosHeading * p->X - sinHeading * p->Y;
        y = sinHeading * p->X + cosHeading * p->Y;
        p->X = (INT)x;
        p->Y = (INT)y;
    }

    State.BrushColor = InvertColor(ForeColor, Invert);
    FILLPOLYGON(this, tri1, cx, cy);
    POLYGON(this, tri1, cx, cy);
    POLYGON(this, tri2, cx, cy);

    State.BrushColor = InvertColor(ForeColor, Invert);
    for (i = 0; i < 16; i++)
    {
        headingRad = DEG2RAD((DOUBLE)i * (360.0 / 16.0));
        cosHeading = cos(headingRad);
        sinHeading = sin(headingRad);

        INT ax = 0;
        INT ay = -t2;
        BOOL s = ((i & 1) != 0);

        x = cosHeading * ax - sinHeading * ay;
        y = sinHeading * ax + cosHeading * ay;

        FillEllipse((INT)x + cx, (INT)y + cy, s ? thick2 : thick4, s ? thick2 : thick4);

        if (pFont && !s)
        {
            static CHAR* headings[] = 
            {
                "N", "NE", "E", "SE", "S", "SW", "W", "NW"
            };

            UINT fontHeight = FontHeight(pFont);
            ax = 0;
            ay = -t2 - fontHeight;
            x = cosHeading * ax - sinHeading * ay;
            y = sinHeading * ax + cosHeading * ay;

            FXRECT textRect;
            textRect.Left = textRect.Right = (INT)x + cx;
            textRect.Top = textRect.Bottom = (INT)y + cy;
            InflateRect(&textRect, fontHeight, fontHeight / 2);

            DrawTextRect(pFont, &textRect, TEXT_ALIGN_VCENTER | TEXT_ALIGN_HCENTER, headings[i / 2]);
        }
    }
}

VOID CfxCanvas::DrawFlag(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Fill, BOOL Invert, BOOL Transparent, BOOL Center)
{
    State.BrushColor = InvertColor(BackColor, Invert);
    if (!Transparent)
    {
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    INT t = (pRect->Right - pRect->Left + 1) / 4;
    INT l = Center ? t : 0;
   
    if (Fill)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillTriangle(pRect->Left + l, pRect->Top, pRect->Left + l + t*2, pRect->Top + t, pRect->Left + l, pRect->Top + t*2);
    }

    State.LineColor = InvertColor(ForeColor, Invert);
    MoveTo(pRect->Left + l, pRect->Top);
    LineTo(pRect->Left + l, pRect->Bottom);
    
    Triangle(pRect->Left + l, pRect->Top, pRect->Left + l + t*2, pRect->Top + t, pRect->Left + l, pRect->Top + t*2);
}

VOID CfxCanvas::DrawCross(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, UINT Thickness, BOOL Invert, BOOL Transparent)
{
    State.BrushColor = InvertColor(BackColor, Invert);
    if (!Transparent)
    {
        FillRect(pRect);
    }
    
    if (Thickness >= 2)
    {
        State.BrushColor = InvertColor(ForeColor, Invert);

        INT w = pRect->Right - pRect->Left + 1;
        INT h = pRect->Bottom - pRect->Top + 1;
        INT nh = min(h, w);

        INT x = pRect->Left + (w - nh) / 2;
        INT y = pRect->Top + (h - nh) / 2;
        h = nh;

        FXPOINTLIST polygon;
        FXPOINT triangle1[] = {{0, Thickness}, {h-Thickness, h}, {h, h-Thickness}, {Thickness, 0}};
        FILLPOLYGON(this, triangle1, x, y);

        FXPOINT triangle2[] = {{h-Thickness, 0}, {0, h-Thickness}, {Thickness, h}, {h, Thickness}};
        FILLPOLYGON(this, triangle2, x, y);
    }
    else
    {
        State.LineColor = InvertColor(ForeColor, Invert);

        INT x1 = pRect->Left;
        INT y1 = pRect->Top;
        INT x2 = pRect->Right;
        INT y2 = pRect->Bottom;

        MoveTo(x1 + 1, y1 + 1);
        LineTo(x2 - 1, y2 - 1);
        MoveTo(x1 + 1, y2 - 1);
        LineTo(x2 - 1, y1 + 1);

        MoveTo(x1 + 1, y1);
        LineTo(x2, y2 - 1);
        MoveTo(x1, y1 + 1);
        LineTo(x2 - 1, y2);

        MoveTo(x1, y2 - 1);
        LineTo(x2 - 1, y1);
        MoveTo(x1 + 1, y2);
        LineTo(x2, y1 + 1);
    }
}

VOID CfxCanvas::DrawCheck(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    State.BrushColor = InvertColor(BackColor, Invert);
    if (!Transparent)
    {
        FillRect(pRect);
    }

    State.BrushColor = InvertColor(ForeColor, Invert);
    State.LineColor = InvertColor(ForeColor, Invert);

    INT w = pRect->Right - pRect->Left + 1;
    INT h = pRect->Bottom - pRect->Top + 1;
    UINT nh = min(h, w);
    UINT nh2 = nh / 3 * 2;
    UINT nh3 = nh / 3 * 3;

    INT x = pRect->Left + (w - nh3) / 2 + 1;
    INT y = pRect->Top + (h - nh2) / 2 + 1 - nh/3;

    h = nh;

    FXPOINTLIST polygon;
    FXPOINT triangle1[] = {{0, h-h/3}, {h/3, h}, {h/3 + 2, h-2}, {2, h-h/3-2}};
    FILLPOLYGON(this, triangle1, x, y);

    FXPOINT triangle2[] = {{h/3, h}, {(h/3)*3, h-h/3*2}, {(h/3)*3-2, h-h/3*2-2}, {h/3-2, h-2}};
    FILLPOLYGON(this, triangle2, x, y);
}

VOID CfxCanvas::DrawAction(FXRECT *pRect)
{
    State.BrushColor = 0;
    FillRect(pRect);

    State.LineColor = 0xFFFFFF;
    State.BrushColor = 0;
    
    FXRECT rect = *pRect;
    for (INT i=0; i<4; i++)
    {
        Rectangle(&rect);
        InflateRect(&rect, -2, -2);
    }

    State.BrushColor = 0xFFFFFF;
    FillRect(&rect);
}

VOID CfxCanvas::DrawHome(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    INT w1 = pRect->Right - pRect->Left - 1;
    INT w2 = w1 / 2;
    INT h1 = pRect->Bottom - pRect->Top - 1;
    INT h2 = h1 / 2;
    INT h = min(h2, w2);

    INT x1 = pRect->Left + w2 + 1;
    INT y1 = pRect->Top + h2 - h + 2;
    INT w = 1;
    
    for (INT i=0; i<h; i++)
    {
        FillRect(x1, y1, w, 1);
        x1--;
        y1++;
        w+=2;
    }

    INT spacer = 1 + h/3;
    x1 += spacer;
    w = w - spacer * 2;
    FillRect(x1, y1, w, h-1);
}

VOID CfxCanvas::DrawRange(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    UINT radius = (pRect->Bottom - pRect->Top) / 2;

    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    MoveTo(pRect->Left, pRect->Top + radius);
    LineTo(pRect->Left + radius*2, pRect->Top + radius);
    LineTo(pRect->Left + radius, pRect->Top);
    MoveTo(pRect->Left + radius*2, pRect->Top + radius);
    LineTo(pRect->Left + radius, pRect->Top + radius*2);
}

VOID CfxCanvas::DrawBearing(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    UINT radius = (pRect->Bottom - pRect->Top) / 2;

    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    Ellipse(pRect->Left + radius, pRect->Top + radius, radius, radius);
    Line(pRect->Left + radius, pRect->Top, pRect->Left + radius, pRect->Top + radius*2);
    Line(pRect->Left, pRect->Top + radius, pRect->Left + radius*2, pRect->Top + radius);
    Line(pRect->Left, pRect->Top, pRect->Left + radius*2, pRect->Top + radius * 2);
    Line(pRect->Left, pRect->Top + radius * 2, pRect->Left + radius*2, pRect->Top);
}

VOID CfxCanvas::DrawInclination(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    UINT radius = (pRect->Bottom - pRect->Top) / 2;

    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    Line(pRect->Left, pRect->Top, pRect->Left, pRect->Top + radius*2);
    Line(pRect->Left, pRect->Top + radius*2, pRect->Left + radius*2, pRect->Top + radius*2);
    Line(pRect->Left, pRect->Top + radius*2, pRect->Left + radius*2, pRect->Top);
}

VOID CfxCanvas::DrawRoll(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    UINT radius = (pRect->Bottom - pRect->Top) / 2;

    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    Ellipse(pRect->Left + radius, pRect->Top + radius, radius, radius);
    Line(pRect->Left, pRect->Top + radius, pRect->Left + radius*2, pRect->Top + radius);
}

VOID CfxCanvas::DrawUpload(BOOL Filled, FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    UINT w = pRect->Right - pRect->Left + 1;
    UINT h = pRect->Bottom - pRect->Top + 1;

    UINT t = min(w, h) - 1;
    INT x = pRect->Left + (w - t) / 2 + 1;
    INT y = pRect->Top + (h - t) / 2 + 1;
    UINT triSide = t / 2;

    y += (t - (t * 5) / 6) / 2 - 1;

    State.BrushColor = InvertColor(ForeColor, Invert);

    // Draw box
    FXRECT rect;
    UINT boxW    = t / 2;
    UINT boxH    = t / 3;
    rect.Left    = x + t / 2 - boxW / 2;
    rect.Right   = rect.Left + boxW;
    rect.Top     = y + triSide;
    rect.Bottom  = y + boxH + triSide;
    if (Filled) 
    {
        FillRect(&rect); 
    }
    else 
    {
        FrameRect(&rect);
    }

    // Draw triangle
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{triSide, 0}, {triSide * 2, triSide}, {0, triSide}};
    if (Filled)
    {
        FILLPOLYGON(this, triangle, x + t / 2 - triSide, y);
    }
    else
    {
        POLYGON(this, triangle, x + t / 2 - triSide, y);
        rect.Bottom = rect.Top + State.LineWidth;
        rect.Left += State.LineWidth;
        rect.Right -= State.LineWidth;
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(&rect);
    }
}

VOID CfxCanvas::DrawHeading(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, INT Heading, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    FXRECT rect = *pRect;
    UINT w = rect.Right - rect.Left + 1;
    UINT h = rect.Bottom - rect.Top + 1;
    INT cx = pRect->Left + w / 2;
    INT cy = pRect->Top + h / 2;

    INT t = min(w, h) - 1;
    InflateRect(&rect, - (t / 12), - (t / 12));
    w = rect.Right - rect.Left + 1;
    h = rect.Bottom - rect.Top + 1;
    t = min(w, h) - 1;

    UINT ttri = t / 3;
    UINT tbox = (t * 2) / 3;
    UINT t12 = t / 12;

    FXPOINTLIST polygon;
    FXPOINT tri[] = {{-40, -20}, {0, -60}, {40, -20}}; 
    FXPOINT box[] = {{20, -30}, {20, 55}, {-20, 55}, {-20, -30}};

    INT i;
    FXPOINT *p;

    DOUBLE headingRad = DEG2RAD(Heading);
    DOUBLE cosHeading = cos(headingRad);
    DOUBLE sinHeading = sin(headingRad);
    DOUBLE x, y;

    for (i = 0; i < ARRAYSIZE(tri); i++)
    {
        p = &tri[i];
        p->X *= t12;
        p->Y *= t12;
        x = cosHeading * p->X - sinHeading * p->Y;
        y = sinHeading * p->X + cosHeading * p->Y;
        p->X = (INT)x / 10;
        p->Y = (INT)y / 10;
    }

    for (i = 0; i < ARRAYSIZE(box); i++)
    {
        p = &box[i];
        p->X *= t12;
        p->Y *= t12;
        x = cosHeading * p->X - sinHeading * p->Y;
        y = sinHeading * p->X + cosHeading * p->Y;
        p->X = (INT)x / 10;
        p->Y = (INT)y / 10;
    }

    State.BrushColor = InvertColor(ForeColor, Invert);
    FILLPOLYGON(this, tri, cx, cy);
    FILLPOLYGON(this, box, cx, cy);

    State.LineColor = InvertColor(ForeColor, Invert);
    Ellipse(cx, cy, t/2 + 5 * State.LineWidth, t/2 + 5 * State.LineWidth);
}

VOID CfxCanvas::DrawBusy(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert)
{
    State.BrushColor = InvertColor(BackColor, Invert);
    FillRect(pRect);

    FXRECT rect = *pRect;
    INT w = rect.Right - rect.Left + 1;
    INT h = rect.Bottom - rect.Top + 1;
    
    rect.Right = rect.Left + w / 2;
    rect.Bottom = rect.Top + h / 2;
    State.BrushColor = InvertColor(ForeColor, Invert);
    FillRect(&rect);

    rect = *pRect;
    rect.Left = rect.Right - w / 2;
    rect.Top = rect.Bottom - h / 2;
    FillRect(&rect);
}

UINT CfxCanvas::DrawTime(FXDATETIME *pDateTime, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT AlignFlags, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    CHAR timeText[16];
    sprintf(timeText, "%02d:%02d", pDateTime->Hour, pDateTime->Minute);
    State.TextColor = InvertColor(ForeColor, Invert);
    DrawTextRect(pFont, pRect, AlignFlags, timeText);

    return TextWidth(pFont, timeText);
}

UINT CfxCanvas::DrawBattery(UINT Level, FXRECT *pRect, UINT AlignFlags, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    FXRECT rect = *pRect;
    rect.Top += State.LineWidth;
    rect.Bottom -= State.LineWidth;
    INT w = rect.Right - rect.Left + 1;
    INT h = rect.Bottom - rect.Top + 1;
    INT h4 = h / 4;

    INT nw = min(w, h) * 2;
    nw = min(nw, w);

    if (AlignFlags & TEXT_ALIGN_RIGHT)
    {
        rect.Left = rect.Right - nw;
    }
    else if (AlignFlags & TEXT_ALIGN_HCENTER)
    {
        rect.Left = rect.Left + w / 2 - nw / 2;
        rect.Right = rect.Left + nw;
    }
    else 
    {
        rect.Right = rect.Left + nw;
    }

    FXRECT rectT;
    rectT = rect;
    rectT.Left = rect.Right - (Level * nw) / 100;

    State.BrushColor = ForeColor;
    FillRect(&rectT);

    State.BrushColor = BackColor;

    rectT = rect;
    rectT.Right = rectT.Left + h4;
    rectT.Bottom = rectT.Top + h4;
    FillRect(&rectT);

    rectT.Top = rect.Bottom - h4;
    rectT.Bottom = rect.Bottom;
    FillRect(&rectT);

    State.BrushColor = ForeColor;

    INT lw = State.LineWidth;

    FillRect(rect.Left + h4, rect.Top, nw - h4, lw);
    FillRect(rect.Left + h4, rect.Bottom - lw + 1, nw - h4, lw);
    FillRect(rect.Right - lw, rect.Top, lw, h);

    FillRect(rect.Left + h4, rect.Top, lw, h4);
    FillRect(rect.Left + h4, rect.Bottom - h4, lw, h4);
    FillRect(rect.Left, rect.Top + h4, lw, h - h4 * 2 - 1);

    FillRect(rect.Left, rect.Top + h4, h4 + lw, lw);
    FillRect(rect.Left, rect.Bottom - h4, h4 + lw, lw);

    return nw;
}

VOID CfxCanvas::DrawShare(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);
    State.LineColor = InvertColor(ForeColor, Invert);

    INT w = pRect->Right - pRect->Left - 1;
    INT h = pRect->Bottom - pRect->Top - 1;

    INT r = min(w, h) / 8;
    INT x = pRect->Left + 1;
    INT y = pRect->Top + 1;
    w--;
    h--;

    FillEllipse(x + w / 8, y + h / 2, r, r);
    FillEllipse(x + w - w / 8, y + h / 8, r, r);
    FillEllipse(x + w - w / 8, y + h - h / 8, r, r);

    Line(x + w / 8, y + h / 2, x + w - w / 8, y + h / 8);
    Line(x + w / 8, y + h / 2, x + w - w / 8, y + h - h / 8);
}

//*************************************************************************************************
// Rectangle

VOID CfxCanvas::Rectangle(INT Left, INT Top, UINT Width, UINT Height)
{
    if ((Width == 0) || (Height == 0)) return;
    FXRECT rect = {Left, Top, Left + Width - 1, Top + Height -1};
    Rectangle(&rect);
} 

VOID CfxCanvas::Rectangle(FXRECT *pRect)
{
    FillRect(pRect);
    if (State.LineColor != State.BrushColor)
    {
        FrameRect(pRect);
    }
} 

VOID CfxCanvas::FillRect(INT Left, INT Top, UINT Width, UINT Height)
{
	if ((Width == 0) || (Height == 0)) return;
    FXRECT rect = {Left, Top, Left + Width - 1, Top + Height - 1};
    FillRect(&rect);
} 

VOID CfxCanvas::FillRect(FXRECT *pRect)
{
    FXRECT rect = *pRect;
    if (!ClipRect(&rect)) return;

    SetBlock(rect.Left, rect.Top, rect.Right, rect.Bottom, State.BrushColor);
} 

VOID CfxCanvas::FrameRect(INT Left, INT Top, UINT Width, UINT Height)
{
    if ((Width == 0) || (Height == 0)) return;
    FXRECT rect = {Left, Top, Left + Width - 1, Top + Height - 1};
    FrameRect(&rect);
} 

VOID CfxCanvas::FrameRect(FXRECT *pRect)
{
    switch (State.LineWidth)
    {
    case 0: return;
    case 1:
        {
            FXRECT rect = *pRect;
            if (!ClipRect(&rect)) return;

            if (pRect->Top == rect.Top)
            {
                SetBlock(rect.Left, rect.Top, rect.Right, rect.Top, State.LineColor);
            }

            if (pRect->Bottom == rect.Bottom)
            {
                SetBlock(rect.Left, rect.Bottom, rect.Right, rect.Bottom, State.LineColor);
            }

            if (pRect->Left == rect.Left)
            {
                SetBlock(rect.Left, rect.Top, rect.Left, rect.Bottom, State.LineColor);
            }

            if (pRect->Right == rect.Right)
            {
                SetBlock(rect.Right, rect.Top, rect.Right, rect.Bottom, State.LineColor);
            }
        }
        break;
    
    default:
        {
            COLOR lastBrushColor = State.BrushColor;
            State.BrushColor = State.LineColor;

            INT x1 = pRect->Left;
            INT y1 = pRect->Top;
            INT w = pRect->Right - x1 + 1;
            INT h = pRect->Bottom - y1 + 1;

            INT lw = min(w, State.LineWidth);
            INT lh = min(h, State.LineWidth);

            FillRect(x1, y1, w, lh);
            FillRect(x1, pRect->Bottom - lh + 1, w, lh);
            y1 += lh;
            h -= lh*2;

            if (h > 0)
            {
                FillRect(x1, y1, lh, h);
                FillRect(pRect->Right - lh + 1, y1, lh, h);
            }

            State.BrushColor = lastBrushColor;
        }
        break;
    }
} 

//*************************************************************************************************
// Text

UINT CfxCanvas::FontHeight(FXFONTRESOURCE *pFont)
{
    return (pFont->Height * _scaleFont) / 100;
}

UINT CfxCanvas::TextWidth(FXFONTRESOURCE *pFont, CHAR *pText, UINT Length)
{
    return 0;
}

UINT CfxCanvas::TextHeight(FXFONTRESOURCE *pFont, CHAR *pText, UINT Length, INT *pOffsetY)
{
    if (pOffsetY)
    {
        *pOffsetY = 0;
    }

    return 0;
}

BOOL CfxCanvas::AdvanceTextLine(FXFONTRESOURCE *pFont, UINT LineWidth, CHAR **ppText, UINT *pCharCount)
{
    FX_ASSERT(ppText && pCharCount);

    if (!*ppText || !**ppText)
    {
        return FALSE;
    }

    *pCharCount = 0;

    INT i = 0;
    CHAR *cursor = *ppText;
    CHAR tab[5] = "    ";
    UINT spaceWidth = TextWidth(pFont, " ", 1);

    while (1)
    {
        CHAR *nextWord = cursor;
        INT nextWordLength = 0;
        while (cursor[nextWordLength] > ' ') nextWordLength++;
        
        CHAR letter = *cursor;
        if (letter == '\0')
        {
            break;
        }
        
        // Spaces
        if (nextWordLength == 0)
        {
            nextWordLength = 1;
        }

        // New line
        if (letter == '\r')
        {
            cursor++;
            continue;
        }

        // Line feed
        if (letter == '\n')
        {
            cursor++;
            break;
        }

        // Tab
        if (letter == '\t')
        {
            nextWord = &tab[0];
            nextWordLength = strlen(nextWord);
        }

        // For newline and if we're running off the end
        INT nextWordWidth = (INT)TextWidth(pFont, nextWord, nextWordLength);
        if ((i > 0) && (i + nextWordWidth >= (INT)LineWidth))
        {
            *pCharCount -= 1;
            break;
        }

        *pCharCount += nextWordLength;
        cursor += nextWordLength;
        i = i + nextWordWidth + spaceWidth;

        if (*cursor == ' ') 
        {
            cursor++;
            *pCharCount += 1;
        }
    }

    *ppText = cursor;

    return TRUE;//*pCharCount > 0;
}

VOID CfxCanvas::CalcTextRect(FXFONTRESOURCE *pFont, CHAR *pText, FXRECT *pRect)
{
    if (!pText)
    {
        pRect->Right = pRect->Left;
        pRect->Bottom = pRect->Top;
        return;
    }
    
    UINT charCount;
    INT h = 0;
    INT lineWidth = pRect->Right - pRect->Left + 1;
    CHAR *cursor = pText;
    while (AdvanceTextLine(pFont, lineWidth, &cursor, &charCount))
    {
        h += FontHeight(pFont);
    }

    pRect->Top = pRect->Left = 0;
    pRect->Bottom = h;
}

VOID CfxCanvas::DrawText(FXFONTRESOURCE *pFont, INT X, INT Y, CHAR *pText, UINT Length)
{
} 

VOID CfxCanvas::DrawTextRect(FXFONTRESOURCE *pFont, FXRECT *pRect, UINT AlignFlags, CHAR *pText, UINT Length)
{
}

//*************************************************************************************************
// Bitmap

VOID CfxCanvas::Draw(CfxCanvas *pCanvas, INT X, INT Y)
{
    // Blank bitmap
    if (!pCanvas->_width || !pCanvas->_height)
    {
        return;
    }

    // Entirely outside
    if ((X > State.ClipRect.Right) || (Y > State.ClipRect.Bottom) ||
        (X + (INT)pCanvas->_width < State.ClipRect.Left) || (Y + (INT)pCanvas->_height < State.ClipRect.Top))
    {
        return;
    }

    INT X1 = 0;
    INT Y1 = 0;
    INT X2 = pCanvas->_width-1;
    INT Y2 = pCanvas->_height-1;
    INT DX = X;
    INT DY = Y;

    // Clip Left, Top
    if (DX < State.ClipRect.Left)
    {
        X1 = State.ClipRect.Left - DX;
        DX = State.ClipRect.Left;
    }
    if (DY < State.ClipRect.Top)
    {
        Y1 = State.ClipRect.Top - DY;
        DY = State.ClipRect.Top;
    }

    // Clip Right, Bottom
    if (X + X2 >= State.ClipRect.Right)
    {
        X2 = State.ClipRect.Right - X;
    }
    if (Y + Y2 > State.ClipRect.Bottom)
    {
        Y2 = State.ClipRect.Bottom - Y;
    }

    // Copy the clipped rect
    CopyRect(pCanvas, X1, Y1, X2, Y2, DX, DY);    
}

VOID CfxCanvas::Draw(FXBITMAPRESOURCE *pBitmap, INT X, INT Y, BOOL Invert)
{
    // Blank bitmap
    if (!pBitmap->Width || !pBitmap->Height)
    {
        return;
    }

    // Entirely outside
    if ((X > State.ClipRect.Right) || (Y > State.ClipRect.Bottom) ||
        (X + pBitmap->Width < State.ClipRect.Left) || (Y + pBitmap->Height < State.ClipRect.Top))
    {
        return;
    }

    INT X1 = 0;
    INT Y1 = 0;
    INT X2 = pBitmap->Width-1;
    INT Y2 = pBitmap->Height-1;
    INT DX = X;
    INT DY = Y;

    // Clip Left, Top
    if (DX < State.ClipRect.Left)
    {
        X1 = State.ClipRect.Left - DX;
        DX = State.ClipRect.Left;
    }
    if (DY < State.ClipRect.Top)
    {
        Y1 = State.ClipRect.Top - DY;
        DY = State.ClipRect.Top;
    }

    // Clip Right, Bottom
    if (X + X2 >= State.ClipRect.Right)
    {
        X2 = State.ClipRect.Right - X;
    }
    if (Y + Y2 > State.ClipRect.Bottom)
    {
        Y2 = State.ClipRect.Bottom - Y;
    }

    // Copy the clipped rect
    CopyRect(pBitmap, X1, Y1, X2, Y2, DX, DY, Invert);    
} 

VOID CfxCanvas::StretchDraw(FXBITMAPRESOURCE *pBitmap, INT X, INT Y, UINT Width, UINT Height, BOOL Invert, BOOL Transparent)
{
    // Blank bitmap
    if (!pBitmap->Width || !pBitmap->Height)
    {
        return;
    }

    // Entirely outside
    if ((X > State.ClipRect.Right) || (Y > State.ClipRect.Bottom) ||
        (X + (INT)Width < State.ClipRect.Left) || (Y + (INT)Height < State.ClipRect.Top))
    {
        return;
    }

    INT X1 = 0;
    INT Y1 = 0;
    INT X2 = pBitmap->Width-1;
    INT Y2 = pBitmap->Height-1;
    INT DX1 = X;
    INT DY1 = Y;
    INT DX2 = X + Width - 1;
    INT DY2 = Y + Height - 1;

    // No stretching in monochrome
    if (_bitDepth == 1)
    {
        if ((X2 == 31) && (Y2 == 31) && (Width < 20) && (Height < 20) && (pBitmap->BitDepth == 1))
        {
            pBitmap = (FXBITMAPRESOURCE *)((BYTE *)pBitmap + pBitmap->Size + sizeof(FXBITMAPRESOURCE) - 1);
            pBitmap = (FXBITMAPRESOURCE *)(((UINT64)pBitmap + 1) & ~1);
            FX_ASSERT(pBitmap->Magic == MAGIC_BITMAP);
            FX_ASSERT(pBitmap->Width == 16);
            FX_ASSERT(pBitmap->Height == 16);

            X2 = 15;
            Y2 = 15;
        }

        // Clip Left, Top
        if (DX1 < State.ClipRect.Left)
        {
            X1 = State.ClipRect.Left - DX1;
            DX1 = State.ClipRect.Left;
        }
        if (DY1 < State.ClipRect.Top)
        {
            Y1 = State.ClipRect.Top - DY1;
            DY1 = State.ClipRect.Top;
        }
        // Clip Right, Bottom
        if (DX2 >= State.ClipRect.Right)
        {
            X2 += State.ClipRect.Right - DX2;
            DX2 = State.ClipRect.Right;
        }
        if (DY2 > State.ClipRect.Bottom)
        {
            Y2 += State.ClipRect.Bottom - DY2;
            DY2 = State.ClipRect.Bottom;
        }

        INT sw = X2 - X1;
        INT sh = Y2 - Y1;
        INT dw = DX2 - DX1;
        INT dh = DY2 - DY1;

        INT w, h;

        INT nx1 = X1;
        INT ny1 = Y1;

        if (sw > dw)
        {
            nx1 += (sw - dw) >> 1;
            w = dw;
        }
        else
        {
            DX1 += (dw - sw) >> 1;
            w = sw;
        }        

        if (sh > dh)
        {
            ny1 += (sh - dh) >> 1;
            h = dh;
        }
        else
        {
            DY1 += (dh - sh) >> 1;
            h = sh;
        }        
            
        CopyRect(pBitmap, nx1, ny1, nx1 + w, ny1 + h, DX1, DY1, Invert);
    }
    else
    {
        FLOAT xScale = (FLOAT)X2 / (FLOAT)Width;
        FLOAT yScale = (FLOAT)Y2 / (FLOAT)Height;

        // Clip Left, Top
        if (DX1 < State.ClipRect.Left)
        {
            X1 = (INT)((INT)(State.ClipRect.Left - DX1) * xScale);
            DX1 = State.ClipRect.Left;
        }
        if (DY1 < State.ClipRect.Top)
        {
            Y1 = (INT)((INT)(State.ClipRect.Top - DY1) * yScale);
            DY1 = State.ClipRect.Top;
        }
        // Clip Right, Bottom
        if (DX2 >= State.ClipRect.Right)
        {
            X2 += (INT)((INT)(State.ClipRect.Right - DX2) * xScale);
            DX2 = State.ClipRect.Right;
        }
        if (DY2 > State.ClipRect.Bottom)
        {
            Y2 += (INT)((INT)(State.ClipRect.Bottom - DY2) * yScale);
            DY2 = State.ClipRect.Bottom;
        }

        // Copy the clipped rect
        StretchCopyRect(pBitmap, X1, Y1, X2, Y2, DX1, DY1, DX2, DY2, Invert, Transparent);
    }
} 

VOID CfxCanvas::StretchDraw(FXBITMAPRESOURCE *pBitmap, FXRECT *pSrcRect, FXRECT *pDstRect, BOOL Invert, BOOL Transparent)
{
    // Unsupported in monochrome
    if (_bitDepth == 1)
    {
        return;
    }

    // Blank bitmap
    if (!pBitmap->Width || !pBitmap->Height)
    {
        return;
    }

    INT dx1 = pDstRect->Left;
    INT dy1 = pDstRect->Top;
    INT dx2 = pDstRect->Right;
    INT dy2 = pDstRect->Bottom;

    INT sx1 = pSrcRect->Left;
    INT sy1 = pSrcRect->Top;
    INT sx2 = pSrcRect->Right;
    INT sy2 = pSrcRect->Bottom;

    // Entirely outside
    if ((dx1 > State.ClipRect.Right) || (dy1 > State.ClipRect.Bottom) ||
        (dx2 < State.ClipRect.Left) || (dy2 < State.ClipRect.Top))
    {
        return;
    }

    INT sw = sx2 - sx1 + 1;
    INT sh = sy2 - sy1 + 1;
    INT dw = dx2 - dx1 + 1;
    INT dh = dy2 - dy1 + 1;

    FLOAT xScale = (FLOAT)sw / (FLOAT)dw;
    FLOAT yScale = (FLOAT)sh / (FLOAT)dh;

    // Clip Left, Top
    if (dx1 < State.ClipRect.Left)
    {
        sx1 = (INT)((INT)(State.ClipRect.Left - dx1) * xScale);
        dx1 = State.ClipRect.Left;
    }
    if (dy1 < State.ClipRect.Top)
    {
        sy1 = (INT)((INT)(State.ClipRect.Top - dy1) * yScale);
        dy1 = State.ClipRect.Top;
    }
    // Clip Right, Bottom
    if (dx2 > State.ClipRect.Right)
    {
        sx2 += (INT)((INT)(State.ClipRect.Right - dx2) * xScale);
        dx2 = State.ClipRect.Right;
    }
    if (dy2 > State.ClipRect.Bottom)
    {
        sy2 += (INT)((INT)(State.ClipRect.Bottom - dy2) * yScale);
        dy2 = State.ClipRect.Bottom;
    }

    // Copy the clipped rect
    StretchCopyRect(pBitmap, sx1, sy1, sx2, sy2, dx1, dy1, dx2, dy2, Invert, Transparent);
} 

//*************************************************************************************************
// Triangle

VOID ScanEdge(INT X1, INT Y1, INT X2, INT Y2, INT *pBuffer)
{
    INT dy = Y2 - Y1;
    INT dx = X2 - X1;
    INT dir = 1;
    
    if (dx < 0)
    {
        dx = -dx;
        dir = -dir;
    }

    if (dx >= dy)
    {
        INT iy = dx / 2;

        *pBuffer = X1;
        pBuffer++;
        for (INT i=0; i<=dx; i++)
        {
            X1 += dir;
            iy += dy;
            if (iy > dx)
            {
                *pBuffer = X1;
                pBuffer++;
                iy -= dx;
            }
        }
    }
    else
    {
        INT ix = dy / 2;

        for (INT i=0; i<=dy; i++)
        {
            *pBuffer = X1;
            pBuffer++;
            
            ix += dx;
            if (ix > dy)
            {
                X1 += dir;
                ix -= dy;
            }
        }
    }
}

VOID CfxCanvas::FillTriangle(INT X1, INT Y1, INT X2, INT Y2, INT X3, INT Y3)
{
    // Y1 < Y2
    if (Y2 < Y1)
    {
        Swap(&X1, &X2);
        Swap(&Y1, &Y2);
    }

    // Y2 < Y3
    if (Y3 < Y2)
    {
        Swap(&X2, &X3);
        Swap(&Y2, &Y3);
    }

    // Y1 topmost
    if (Y2 < Y1)
    {
        Swap(&X1, &X2);
        Swap(&Y1, &Y2);
    }

    UINT startPosition = _renderBuffer.GetPosition();
    UINT position = (startPosition + 3) & ~3;
    UINT bufferSize = ((Y3 - Y1 + 1) + (Y2 - Y1 + 1) + (Y3 - Y2 + 1) + 3) * sizeof(INT); 
    
    if (_renderBuffer.GetCapacity() < position + bufferSize * 2)
    {
        _renderBuffer.SetCapacity(position + bufferSize * 2);
    }

    INT *fillA0 = (INT *)((BYTE *)_renderBuffer.GetMemory() + position);
    INT *fillA1 = (INT *)((BYTE *)fillA0 + (Y3 - Y1 + 1) * sizeof(INT));
    INT *fillA2 = (INT *)((BYTE *)fillA1 + (Y2 - Y1) * sizeof(INT));

    ScanEdge(X1, Y1, X3, Y3, &fillA0[0]);
    ScanEdge(X1, Y1, X2, Y2, &fillA1[0]);
    ScanEdge(X2, Y2, X3, Y3, &fillA2[0]);

    for (INT i=Y1; i<=Y3; i++)
    {
        INT x1 = *fillA0;
        INT x2 = *fillA1;
        fillA0++;
        fillA1++;

        if ((i < State.ClipRect.Top) || (i > State.ClipRect.Bottom)) continue;

        if (x1 > x2)
        {
            Swap(&x1, &x2);
        }

        if ((x1 > State.ClipRect.Right) || (x2 < State.ClipRect.Left)) continue;

        x1 = max(x1, State.ClipRect.Left);
        x2 = min(x2, State.ClipRect.Right);
        
        SetBlock(x1, i, x2, i, State.BrushColor);
    }

    _renderBuffer.SetPosition(startPosition);
}

VOID CfxCanvas::Triangle(INT X1, INT Y1, INT X2, INT Y2, INT X3, INT Y3)
{
    MoveTo(X1, Y1);
    LineTo(X2, Y2);
    LineTo(X3, Y3);
    LineTo(X1, Y1);
}

//*************************************************************************************************
// Polygon 

VOID CfxCanvas::FillPolygon(FXPOINTLIST *VertexList, INT X, INT Y)
{
    FX_ASSERT(VertexList->Length >= 3);

    INT x1 = VertexList->Points[0].X + X;
    INT y1 = VertexList->Points[0].Y + Y;

    INT *p = &(VertexList->Points[1].X);
    for (UINT i=VertexList->Length; i>2; i--)
    {
        INT x2 = *(p)   + X;
        INT y2 = *(p+1) + Y;
        INT x3 = *(p+2) + X;
        INT y3 = *(p+3) + Y;

        FillTriangle(x1, y1, x2, y2, x3, y3);
        p += 2;
    }
}

VOID CfxCanvas::Polygon(FXPOINTLIST *VertexList, INT X, INT Y)
{
    FXPOINT *point = VertexList->Points;
    INT x1 = point->X;
    INT y1 = point->Y;

    MoveTo(point->X + X, point->Y + Y);
    for (UINT i=1; i < VertexList->Length; i++)
    {
        point++;
        LineTo(point->X + X, point->Y + Y);
    }
    
    if ((point->X != x1) || (point->Y != y1))
    {
        LineTo(x1 + X, y1 + Y);
    }
}

//*************************************************************************************************
// Ellipse

typedef VOID (*FnSet4Pixels)(CfxCanvas *pCanvas, INT X, INT Y, INT XC, INT YC, VOID *pParams);

VOID CalcEllipse(INT Xc, INT Yc, UINT RadiusX, UINT RadiusY, CfxCanvas *pCanvas, FnSet4Pixels pSet4Pixels, VOID *pParams = 0)
{
    INT x = 0;
    INT y = RadiusY;

    INT a = RadiusX;
    INT b = RadiusY;

    INT asquared = a * a;
    INT twoasquared = 2 * asquared;
    INT bsquared = b * b;
    INT twobsquared = 2 * bsquared;

    INT d  = bsquared - asquared * b + asquared / 4;
    INT dx = 0;
    INT dy = twoasquared * b;

    while (dx < dy)
    {
        (*pSet4Pixels)(pCanvas, x, y, Xc, Yc, pParams);
        if (d > 0)
        {
            --y;
            dy -= twoasquared;
            d -=dy;
        }

        ++x;
        dx += twobsquared;
        d += bsquared + dx;
    }

    d += (3 * (asquared - bsquared) / 2 - (dx + dy)) / 2;

    while (y >= 0)
    {
        (*pSet4Pixels)(pCanvas, x, y, Xc, Yc, pParams);

        if (d < 0)
        { 
            ++x;
            dx += twobsquared;
            d += dx;
        }
        
        --y;
        dy -= twoasquared;
        d += asquared - dy;
    }
}

VOID Fill4Pixels(CfxCanvas *pCanvas, INT X, INT Y, INT Xc, INT Yc, VOID *pParams)
{
    COLOR color = pCanvas->State.BrushColor;
    FXRECT *clipRect = &pCanvas->State.ClipRect;

    INT x1 = Xc - X;
    INT x2 = Xc + X;
    if ((x1 > clipRect->Right) || (x2 < clipRect->Left)) return;
    INT y1 = Yc - Y;
    INT y2 = Yc + Y;
    if ((y1 > clipRect->Bottom) || (y2 < clipRect->Top)) return;

    x1 = max(x1, clipRect->Left);
    x2 = min(x2, clipRect->Right);

    if (y1 >= clipRect->Top)
    {
        pCanvas->SetBlock(x1, y1, x2, y1, color);
    }

    if (y2 <= clipRect->Bottom)
    {
        pCanvas->SetBlock(x1, y2, x2, y2, color);
    }
}

VOID CfxCanvas::FillEllipse(INT Xc, INT Yc, UINT RadiusX, UINT RadiusY)
{
    if ((RadiusX == 1) && (RadiusY == 1))
    {
        FillRect(Xc, Yc, 2, 2);
    }
    else
    {
        CalcEllipse(Xc, Yc, RadiusX, RadiusY, this, &Fill4Pixels);
    }
}

VOID Set4Pixels(CfxCanvas *pCanvas, INT X, INT Y, INT Xc, INT Yc, VOID *pParams)
{
    FXRECT *clipRect = &pCanvas->State.ClipRect;
    INT x, y;

    // Set clip parameters
    x = Xc - X;
    BOOL x1ok = (x >= clipRect->Left) && (x <= clipRect->Right);
    x = Xc + X;
    BOOL x2ok = (x >= clipRect->Left) && (x <= clipRect->Right);
    y = Yc - Y;
    BOOL y1ok = (y >= clipRect->Top) && (y <= clipRect->Bottom);
    y = Yc + Y;
    BOOL y2ok = (y >= clipRect->Top) && (y <= clipRect->Bottom);

    // Draw the pixels
    if (x1ok && y1ok) pCanvas->SetLinePixel(Xc - X, Yc - Y);
    if (x2ok && y1ok) pCanvas->SetLinePixel(Xc + X, Yc - Y);
    if (x1ok && y2ok) pCanvas->SetLinePixel(Xc - X, Yc + Y);
    if (x2ok && y2ok) pCanvas->SetLinePixel(Xc + X, Yc + Y);
}

VOID CfxCanvas::Ellipse(INT Xc, INT Yc, UINT RadiusX, UINT RadiusY)
{
    CalcEllipse(Xc, Yc, RadiusX, RadiusY, this, &Set4Pixels);
}

//*************************************************************************************************
// RoundRect

VOID Set4PixelsRoundRect(CfxCanvas *pCanvas, INT X, INT Y, INT Xc, INT Yc, VOID *pParams)
{
    FXRECT *clipRect = &pCanvas->State.ClipRect;
    INT x, y;

    INT dx = ((FXPOINT *)pParams)->X;
    INT dy = ((FXPOINT *)pParams)->Y;
    COLOR color = pCanvas->State.LineColor;

    switch (pCanvas->State.LineWidth)
    {
    case 0: break;
    case 1:
        {
            // Set clip parameters
            x = Xc - X;
            BOOL x1ok = (x >= clipRect->Left) && (x <= clipRect->Right);
            x = Xc + X + dx;
            BOOL x2ok = (x >= clipRect->Left) && (x <= clipRect->Right);
            y = Yc - Y;
            BOOL y1ok = (y >= clipRect->Top) && (y <= clipRect->Bottom);
            y = Yc + Y + dy;
            BOOL y2ok = (y >= clipRect->Top) && (y <= clipRect->Bottom);

            // Draw the pixels
            if (x1ok && y1ok) pCanvas->SetPixel(Xc - X, Yc - Y, color);
            if (x2ok && y1ok) pCanvas->SetPixel(Xc + X + dx, Yc - Y, color);
            if (x1ok && y2ok) pCanvas->SetPixel(Xc - X, Yc + Y + dy, color);
            if (x2ok && y2ok) pCanvas->SetPixel(Xc + X + dx, Yc + Y + dy, color);
        }
        break;

    case 2:
        {
            COLOR lastBrushColor = pCanvas->State.BrushColor;
            pCanvas->State.BrushColor = color;

            pCanvas->FillRect(Xc - X, Yc - Y, 2, 2);
            pCanvas->FillRect(Xc + X + dx - 1, Yc - Y, 2, 2);
            pCanvas->FillRect(Xc - X, Yc + Y + dy - 1, 2, 2);
            pCanvas->FillRect(Xc + X + dx - 1, Yc + Y + dy - 1, 2, 2);

            pCanvas->State.BrushColor = lastBrushColor;
        }
        break;

    default:
        {
            UINT radius = pCanvas->State.LineWidth;
            COLOR lastBrushColor = pCanvas->State.BrushColor;
            pCanvas->State.BrushColor = color;

            pCanvas->FillEllipse(Xc - X, Yc - Y, radius, radius);
            pCanvas->FillEllipse(Xc + X + dx, Yc - Y, radius, radius);
            pCanvas->FillEllipse(Xc - X, Yc + Y + dy, radius, radius);
            pCanvas->FillEllipse(Xc + X + dx, Yc + Y + dy, radius, radius);

            pCanvas->State.BrushColor = lastBrushColor;
        }
        break;
    }
}

VOID CfxCanvas::RoundRect(INT Left, INT Top, UINT Width, UINT Height, UINT CornerWidth)
{
    FXRECT rect = {Left, Top, Left + Width - 1, Top + Height -1};
    RoundRect(&rect, CornerWidth);
}

VOID CfxCanvas::RoundRect(FXRECT *pRect, UINT CornerWidth)
{
    FXRECT rect = *pRect;
    UINT width = rect.Right - rect.Left + 1;
    UINT height = rect.Bottom - rect.Top + 1;

    FXPOINT delta;
    delta.X = width - CornerWidth * 2 - 1;
    delta.Y = height - CornerWidth * 2 - 1;

    if ((delta.X <= 0) || (delta.Y <= 0))
    {
        FrameRect(pRect);
        return;
    }

    if (!ClipRect(&rect)) return;

    CalcEllipse(pRect->Left + CornerWidth, pRect->Top + CornerWidth, CornerWidth, CornerWidth, this, &Set4PixelsRoundRect, &delta);

    if ((State.LineWidth == 1) && memcmp(&rect, pRect, sizeof(FXRECT)) == 0)
    {
        // No clipping required
        SetBlock(rect.Left + CornerWidth, rect.Top, rect.Right - CornerWidth, rect.Top, State.LineColor);
        SetBlock(rect.Left + CornerWidth, rect.Bottom, rect.Right - CornerWidth, rect.Bottom, State.LineColor);
        SetBlock(rect.Left, rect.Top + CornerWidth, rect.Left, rect.Bottom - CornerWidth, State.LineColor);
        SetBlock(rect.Right, rect.Top + CornerWidth, rect.Right, rect.Bottom - CornerWidth, State.LineColor);
    }
    else
    {
        COLOR lastColor = State.BrushColor;
        State.BrushColor = State.LineColor;
        FillRect(pRect->Left + CornerWidth, pRect->Top, pRect->Right - pRect->Left + 1 - CornerWidth * 2, State.LineWidth);
        FillRect(pRect->Left + CornerWidth, pRect->Bottom - State.LineWidth + 1, pRect->Right - pRect->Left + 1 - CornerWidth * 2, State.LineWidth);
        FillRect(pRect->Left, pRect->Top + CornerWidth, State.LineWidth, pRect->Bottom - pRect->Top + 1 - CornerWidth * 2);
        FillRect(pRect->Right - State.LineWidth + 1, pRect->Top + CornerWidth, State.LineWidth, pRect->Bottom - pRect->Top + 1 - CornerWidth * 2);
        State.BrushColor = lastColor;
    }
}

VOID CfxCanvas::FillRoundRect(INT Left, INT Top, UINT Width, UINT Height, UINT CornerWidth)
{
    FXRECT rect = {Left, Top, Left + Width - 1, Top + Height -1};
    RoundRect(&rect, CornerWidth);
}

VOID Set4PixelsFillRoundRect(CfxCanvas *pCanvas, INT X, INT Y, INT Xc, INT Yc, VOID *pParams)
{
    COLOR color = pCanvas->State.BrushColor;
    FXRECT *clipRect = &pCanvas->State.ClipRect;

    INT dx = ((FXPOINT *)pParams)->X;
    INT dy = ((FXPOINT *)pParams)->Y;

    FXRECT rect = {Xc - X, Yc - Y, Xc + X + dx, Yc + Y + dy};
    IntersectRect(&rect, &rect, clipRect);
    
    pCanvas->SetBlock(rect.Left, rect.Top, rect.Right, rect.Top, color);
    pCanvas->SetBlock(rect.Left, rect.Bottom, rect.Right, rect.Bottom, color);
}

VOID CfxCanvas::FillRoundRect(FXRECT *pRect, UINT CornerWidth)
{
    FXRECT rect = *pRect;
    UINT width = rect.Right - rect.Left + 1;
    UINT height = rect.Bottom - rect.Top + 1;

    FXPOINT delta;
    delta.X = width - CornerWidth * 2 - 1;
    delta.Y = height - CornerWidth * 2 - 1;

    if ((delta.X <= 0) || (delta.Y <= 0))
    {
        FillRect(pRect);
        return;
    }

    if (!ClipRect(&rect)) return;

    CalcEllipse(pRect->Left + CornerWidth, pRect->Top + CornerWidth, CornerWidth, CornerWidth, this, &Set4PixelsFillRoundRect, &delta);

    // Middle piece
    FillRect(pRect->Left, pRect->Top + CornerWidth, width, height - CornerWidth * 2);
}

//*************************************************************************************************
// Control Corners

VOID StoreCurve(CfxCanvas *pCanvas, INT X, INT Y, INT Xc, INT Yc, VOID *pParams)
{
    BYTE *offset = &((BYTE *)pParams)[Yc - Y];
    *offset = (BYTE)(Xc - X);
}

VOID CfxCanvas::PushCorners(FXRECT *pRect, UINT CornerWidth)
{
    CornerWidth = min(MAX_CORNER_WIDTH, CornerWidth);

    FXRECT rect = *pRect;

    INT x1 = rect.Left;
    INT y1 = rect.Top;
    INT x2 = rect.Right;
    INT y2 = rect.Bottom;

    INT dX = (x2 - x1) - CornerWidth * 2;
    INT dY = (y2 - y1) - CornerWidth * 2;

    UINT position = _renderBuffer.GetPosition();
    UINT header = 'PIXE';
    BOOL empty = TRUE;
    _renderBuffer.Write(&header, sizeof(header));

    if (!ClipRect(&rect) || (dX <= 0) || (dY <= 0))
    {
        _renderBuffer.Write(&empty, sizeof(empty));
        _renderBuffer.Write(&position, sizeof(position));

        return;
    }

    CalcEllipse(CornerWidth, CornerWidth, CornerWidth, CornerWidth, this, &StoreCurve, &_cornerOffsets);

    empty = FALSE;
    _renderBuffer.Write(&empty, sizeof(empty));
    _renderBuffer.Write(&CornerWidth, sizeof(CornerWidth));
    _renderBuffer.Write(&_cornerOffsets[0], CornerWidth * sizeof(BYTE));

    for (UINT j=0; j<CornerWidth; j++)
    {
        for (INT i=0; i<_cornerOffsets[j]; i++)
        {
            COLOR colors[4];

            colors[0] = GetPixel(x1 + i, y1 + j);
            colors[1] = GetPixel(x2 - i, y1 + j);
            colors[2] = GetPixel(x1 + i, y2 - j);
            colors[3] = GetPixel(x2 - i, y2 - j);

            _renderBuffer.Write(&colors[0], sizeof(colors));
        }
    }

    _renderBuffer.Write(&position, sizeof(position));
}

VOID CfxCanvas::PopCorners(FXRECT *pRect)
{
    FXRECT rect = *pRect;

    INT x1 = rect.Left;
    INT x2 = rect.Right;
    INT y1 = rect.Top;
    INT y2 = rect.Bottom;

    UINT position;
    _renderBuffer.SetPosition(_renderBuffer.GetPosition() - sizeof(position));
    _renderBuffer.Read(&position, sizeof(position));
    _renderBuffer.SetPosition(position);

    UINT header;
    _renderBuffer.Read(&header, sizeof(header));
    FX_ASSERT(header == 'PIXE');

    BOOL empty;
    _renderBuffer.Read(&empty, sizeof(empty));
    if (empty)
    {
        _renderBuffer.SetPosition(position);
        return;
    }

    UINT cornerWidth;
    _renderBuffer.Read(&cornerWidth, sizeof(cornerWidth));
    FX_ASSERT(cornerWidth <= MAX_CORNER_WIDTH);

    INT dX = (x2 - x1) - cornerWidth * 2;
    INT dY = (y2 - y1) - cornerWidth * 2;

    _renderBuffer.Read(&_cornerOffsets[0], cornerWidth * sizeof(BYTE));

    for (UINT j=0; j<cornerWidth; j++)
    {
        for (INT i=0; i<_cornerOffsets[j]; i++)
        {
            COLOR colors[4];

            _renderBuffer.Read(&colors[0], sizeof(colors));

            INT x, y;

            x = x1 + i;
            BOOL x1ok = (x >= State.ClipRect.Left) && (x <= State.ClipRect.Right);
            x = x2 - i;
            BOOL x2ok = (x >= State.ClipRect.Left) && (x <= State.ClipRect.Right);
            y = y1 + j;
            BOOL y1ok = (y >= State.ClipRect.Top) && (y <= State.ClipRect.Bottom);
            y = y2 - j;
            BOOL y2ok = (y >= State.ClipRect.Top) && (y <= State.ClipRect.Bottom);

            if (x1ok && y1ok) SetPixel(x1 + i, y1 + j, colors[0]);
            if (x2ok && y1ok) SetPixel(x2 - i, y1 + j, colors[1]);
            if (x1ok && y2ok) SetPixel(x1 + i, y2 - j, colors[2]);
            if (x2ok && y2ok) SetPixel(x2 - i, y2 - j, colors[3]);
        }
    }

    _renderBuffer.SetPosition(position);
}
