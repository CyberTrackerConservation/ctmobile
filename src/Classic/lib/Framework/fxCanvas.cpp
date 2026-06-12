#include "fxUtils.h"
#include "fxCanvas.h"
#include "fxApplication.h"
#include "fxMath.h"

char *AllocCleanText(const char *pText, int *pLength, bool SingleLine)
{
    int Length = pLength ? *pLength : 0;
    Length = Length ? Length : strlen(pText);
    int OutLength = 0;

    char *pOutText = (char *)MEM_ALLOC(Length + 1);
    char *pOutCurr = pOutText;

    const char *pInCurr = pText;

    for (int i = 0; i < Length; i++, pInCurr++) {
        char c = *pInCurr;

        // If a tail byte then skip ahead
        if ((c & 0xc0) == 0x80)
        {
            *pOutCurr = c;
            pOutCurr++;
            continue;
        }

        // Skip LF
        if (c == '\r') {
            continue;
        }

        // Skip CR if SingleLine
        if (SingleLine && (c == '\n')) {
            continue;
        }

        OutLength++;

        *pOutCurr = c;
        pOutCurr++;
    }

    *pOutCurr = 0;

    if (pLength) {
        *pLength = OutLength;
    }

    return pOutText;
}

__inline COLOR FixColor(COLOR Color)
{
    COLOR t = Color;
    return (t & 0xFF00) | ((t & 0xFF) << 16) | ((t & 0xFF0000) >> 16) | 0xFF000000;
}

CfxCanvas::CfxCanvas(CfxPersistent *pOwner, int ScaleFont): CfxPersistent(pOwner), _renderBuffer(), _fontCache()
{
    memset(&State, 0, sizeof(State));
    _width = _height = 0;
    _bitDepth = 32;
    _scaleFont = ScaleFont;
    State.LineWidth = 1;
}

CfxCanvas::~CfxCanvas()
{
    for (int i = 0; i < _fontCache.GetCount(); i++)
    {
        FreeFont(_fontCache.GetPtr(i)->Handle);
    }

    if (_painter.isActive())
    {
        _painter.end();
    }
}

void *CfxCanvas::FindFont(FXFONTRESOURCE *pFont)
{
    FX_FONT_CACHE_ITEM *currItem = nullptr;

    for (int i = 0; i < _fontCache.GetCount(); i++, currItem++)
    {
        if (currItem == nullptr)
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

    if (newItem.Handle != nullptr)
    {
        _fontCache.Add(newItem);
    }

    return newItem.Handle;
}

bool CfxCanvas::ClipRect(FXRECT *pRect)
{
    return IntersectRect(pRect, pRect, &State.ClipRect);
}

void CfxCanvas::SetBitDepth(BYTE BitDepth)
{
    int width = _width;
    int height = _height;

    SetSize(0, 0);
    _bitDepth = BitDepth;
    SetSize(width, height);
}

COLOR CfxCanvas::InvertColor(COLOR Color, bool Invert)
{ 
    if (_bitDepth >= 16)
    {
        return Invert ? Color ^ 0xFFFFFF : Color;
    }

    // Monochrome color invert
    int intensity = (Color & 0xFF) + ((Color & 0xFF00) >> 8) + ((Color & 0xFF0000) >> 16);
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

bool CfxCanvas::ClipPixel(int X, int Y)
{
    return ((X >= State.ClipRect.Left) && (X <= State.ClipRect.Right) &&
            (Y >= State.ClipRect.Top) && (Y <= State.ClipRect.Bottom));
}

void CfxCanvas::SetLinePixel(int X, int Y)
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

void SetOutcodes(union OutcodeUnion *pU, FXRECT *pRect, int X, int Y)
{
    pU->outcodes = 0;
    pU->ocs.code0 = (X < XUL);
    pU->ocs.code1 = (Y < YUL);
    pU->ocs.code2 = (X > XLR);
    pU->ocs.code3 = (Y > YLR);
}

bool CfxCanvas::ClipLine(FXLINE *pLine)
{
    int X1 = pLine->X1;
    int Y1 = pLine->Y1;
    int X2 = pLine->X2;
    int Y2 = pLine->Y2;

    union OutcodeUnion ocu1, ocu2;
    bool inside, outside;
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

void CfxCanvas::Line(int X1, int Y1, int X2, int Y2)
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
    int dx = X2 - X1;
    int dy = (int)FxAbs((int)Y2 - (int)Y1);
    int sy = (Y2 > Y1) ? 1 : -1;

    int dx2 = dx << 1;
    int dy2 = dy << 1;

    if (dy <= dx)           
    { 
        int e = dy2 - dx;

        for (int i=0; i<=dx; ++i) 
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
        int e = dx2 - dy;

        for (int i=0; i<=dy; ++i) 
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

void CfxCanvas::MoveTo(int X, int Y)
{
    State.LineX = X;
    State.LineY = Y;
} 

void CfxCanvas::LineTo(int X, int Y)
{
    Line(State.LineX, State.LineY, X, Y);
    MoveTo(X, Y);
} 

//*************************************************************************************************
// Clip Rectangle

void CfxCanvas::SetClipRect(FXRECT *pRect)  
{
    ClearClipRect();
    IntersectRect(&State.ClipRect, pRect, &State.ClipRect);
} 

void CfxCanvas::ClearClipRect()           
{
    State.ClipRect.Left = State.ClipRect.Top = 0;
    State.ClipRect.Right = _width - 1;
    State.ClipRect.Bottom = _height - 1;
} 

//*************************************************************************************************
// Static shapes

void CfxCanvas::DrawFirstArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    State.BrushColor = InvertColor(ForeColor, Invert);

    int w1 = pRect->Right - pRect->Left + 1;
    int w2 = w1 / 2 - 2;
    int h1 = pRect->Bottom - pRect->Top + 1;
    int h2 = h1 / 2 - 2;

    int h = min(h2, w2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{h, 0}, {h, h*2}, {0, h}};

    int x = pRect->Left + (w1 - h) / 2 + h/2;
    int y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);

    FXRECT rect = {x - h, y, x-1, y + h*2};
    FillRect(&rect);
}

void CfxCanvas::DrawBackArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    State.BrushColor = InvertColor(ForeColor, Invert);

    int w1 = pRect->Right - pRect->Left + 1;
    int w2 = w1 / 2 - 2;
    int h1 = pRect->Bottom - pRect->Top + 1;
    int h2 = h1 / 2 - 2;

    int h = min(h2, w2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{h, 0}, {h, h*2}, {0, h}};

    int x = pRect->Left + (w1 - h) / 2;
    int y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);
}

void CfxCanvas::DrawNextArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    int w1 = pRect->Right - pRect->Left + 1;
    int w2 = w1 / 2 - 2;
    int h1 = pRect->Bottom - pRect->Top + 1;
    int h2 = h1 / 2 - 2;

    int h = min(h2, w2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{0, 0}, {h, h}, {0, h*2}};

    int x = pRect->Left + (w1 - h) / 2;
    int y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);
}

void CfxCanvas::DrawPlay(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    int w1 = pRect->Right - pRect->Left + 1;
    int h1 = pRect->Bottom - pRect->Top + 1;
    int h = min(w1 / 2 - 2, h1 / 2 - 2);

    State.BrushColor = InvertColor(ForeColor, Invert);
    FillRect(pRect);
    State.BrushColor = InvertColor(BackColor, Invert);

    h = min(h1 / 3 - 2, w1 / 3 - 2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{0, 0}, {h, h}, {0, h*2}};
    int x = pRect->Left + w1 / 2 - h / 3;
    int y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);
}

void CfxCanvas::DrawPort(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
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
    int w = rect.Right - rect.Left + 1;
    int h = rect.Bottom - rect.Top + 1;
    int t = min(w, h);

    InflateRect(&rect, - t / 5, - t / 5); 
    w = rect.Right - rect.Left + 1;
    h = rect.Bottom - rect.Top + 1;
    double h5 = h / 5;

    FillRect(rect.Left, rect.Top, w, (int)h5);
    FillRect(rect.Left, rect.Top + (int)h5 * 2, w, (int)h5);
    FillRect(rect.Left, rect.Top + (int)h5 * 4, w, (int)h5);
}

void CfxCanvas::DrawPortState(FXRECT *pRect, FXFONTRESOURCE *pFont, int PortId, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    int w1 = pRect->Right - pRect->Left + 1;
    int h1 = pRect->Bottom - pRect->Top + 1;
    int h = min(w1 / 2 - 2, h1 / 2 - 2);

    int xc = pRect->Left + w1 / 2;
    int yc = pRect->Top + h1 / 2;
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

    char portIdText[8];
    sprintf(portIdText, "%lu", PortId);
    
    int offsetY;
    int textH = TextHeight(pFont, portIdText, 0, &offsetY);
    int textW = TextWidth(pFont, portIdText);

    State.TextColor = InvertColor(ForeColor, Invert);

    FXRECT rText = *pRect;
    rText.Top = yc - offsetY - textH / 2;
    rText.Bottom = rText.Top + textH;
    rText.Left = xc - textW / 2 + 1;
    rText.Right = rText.Left + textW;

    DrawText(pFont, rText.Left, rText.Top, portIdText);
}

void CfxCanvas::DrawRecord(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    int w1 = pRect->Right - pRect->Left + 1;
    int h1 = pRect->Bottom - pRect->Top + 1;
    int h = min(w1 / 2 - 2, h1 / 2 - 2);

    State.BrushColor = InvertColor(ForeColor, Invert);
    int xc = pRect->Left + w1 / 2;
    int yc = pRect->Top + h1 / 2;
    FillRect(pRect);
    State.BrushColor = InvertColor(BackColor, Invert);

    h = min(h1 / 4 - 2, w1 / 4 - 2);
    FillEllipse(xc, yc, h, h); 

    h = h + State.LineWidth * 4;
    State.LineColor = InvertColor(BackColor, Invert);
    Ellipse(xc, yc, h, h);
}

void CfxCanvas::DrawStop(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(BackColor, !Invert);

    int w1 = pRect->Right - pRect->Left + 1;
    int h1 = pRect->Bottom - pRect->Top + 1;
    int h = min(w1 / 2 - 2, h1 / 2 - 2);

    State.BrushColor = InvertColor(BackColor, !Invert);
    int xc = pRect->Left + w1 / 2;
    int yc = pRect->Top + h1 / 2;
    //FillEllipse(xc, yc, h, h); 
    FillRect(pRect);
    State.BrushColor = InvertColor(BackColor, Invert);

    h = min(h1 / 4 - 2, w1 / 4 - 2);
    FillRect(xc - h, yc - h, h * 2 + 1, h * 2 + 1);
}

void CfxCanvas::DrawLastArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    int w1 = pRect->Right - pRect->Left + 1;
    int w2 = w1 / 2 - 2;
    int h1 = pRect->Bottom - pRect->Top + 1;
    int h2 = h1 / 2 - 2;

    int h = min(h2, w2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{0, 0}, {h, h}, {0, h*2}};

    int x = pRect->Left + (w1 - h) / 2 - h/2;
    int y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);

    FXRECT rect = {x + h + 1, y, x + h*2, y + h*2};
    FillRect(&rect);
}

void CfxCanvas::DrawUpArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    int w1 = pRect->Right - pRect->Left - 1;
    int w2 = (w1 - 1)  / 2;
    int h1 = pRect->Bottom - pRect->Top - 1;
    int h2 = (h1 - 1) / 2;
    int h = min(h2, w2);

    for (int i=0; i<=h; i++)
    {
        FillRect(pRect->Left + w2 - i + 1, pRect->Top + h1/2 - h/2 + i, i * 2 + 1, 1);
    }
}

void CfxCanvas::DrawDownArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    int w1 = pRect->Right - pRect->Left - 1;
    int w2 = (w1 - 1)  / 2;
    int h1 = pRect->Bottom - pRect->Top - 1;
    int h2 = (h1 - 1) / 2;
    int h = min(h2, w2);

    for (int i=0; i<=h; i++)
    {
        FillRect(pRect->Left + w2 - i + 1, pRect->Top + h1/2 + h/2 - i, i * 2 + 1, 1);
    }
}

void CfxCanvas::DrawSkipArrow(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    int w1 = pRect->Right - pRect->Left + 1;
    int w2 = w1 / 2 - 2;
    int h1 = pRect->Bottom - pRect->Top + 1;
    int h2 = h1 / 2 - 2;

    int h = min(h2, w2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{0, 0}, {h, h}, {0, h*2}};

    int x = pRect->Left + (w1 - h) / 2 + (h / 3);
    int y = pRect->Top + h1 / 2 - h;
    FILLPOLYGON(this, triangle, x, y);
    x -= (h * 2) / 3;
    FILLPOLYGON(this, triangle, x, y);
}

void CfxCanvas::DrawTriangle(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Fill, bool Invert, bool Transparent)
{
    State.BrushColor = InvertColor(BackColor, Invert);
    if (!Transparent)
    {
        FillRect(pRect);
    }
    State.LineColor = InvertColor(ForeColor, Invert);

    FXRECT rect = *pRect;
    InflateRect(&rect, -(int)(State.LineWidth-1), -(int)(State.LineWidth-1));

    int width = rect.Right - rect.Left + 1;
    int height = rect.Bottom - rect.Top + 1;
    int h = min(height / 2 - 2, width / 2 - 2);

    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{h, 0}, {h*2, h*2}, {0, h*2}};

    int x = rect.Left + (width - h*2) / 2;
    int y = rect.Top + height / 2 - h;

    if (Fill)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FILLPOLYGON(this, triangle, x, y);
    }

    State.LineColor = InvertColor(ForeColor, Invert);
    POLYGON(this, triangle, x, y);
}

void CfxCanvas::DrawTarget(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, int Heading, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    int cx = pRect->Left + (pRect->Right - pRect->Left + 1) / 2;
    int cy = pRect->Top + (pRect->Bottom - pRect->Top + 1) / 2;
    int thick = State.LineWidth;
    int thick2 = thick * 2;

    FXRECT rect = *pRect;
    int w = rect.Right - rect.Left + 1;
    int h = rect.Bottom - rect.Top + 1;
    int t = min(w, h);
    int t2 = t / 2;
    int t4 = t / 4;
    State.LineColor = InvertColor(ForeColor, Invert);
    Ellipse(cx, cy, t2 - 1, t2 - 1);
    
    int segLen = (t * 5 + 11) / 12;

    State.BrushColor = InvertColor(ForeColor, Invert);
    FillRect(cx - thick - segLen, cy, segLen, thick);
    FillRect(cx + thick2, cy, segLen, thick);
    FillRect(cx, cy - thick - segLen, thick, segLen);
    FillRect(cx, cy + thick2, thick, segLen);

    FXPOINTLIST polygon;
    FXPOINT tri[] = {{-t4, -t2 + thick}, {0, -t2 - t4 + thick}, {t4, -t2 + thick}}; 

    int i;
    FXPOINT *p;

    double headingRad = DEG2RAD(Heading);
    double cosHeading = cos(headingRad);
    double sinHeading = sin(headingRad);
    double x, y;

    for (i = 0; i < ARRAYSIZE(tri); i++)
    {
        p = &tri[i];
        x = cosHeading * p->X - sinHeading * p->Y;
        y = sinHeading * p->X + cosHeading * p->Y;
        p->X = (int)x;
        p->Y = (int)y;
    }

    State.BrushColor = InvertColor(ForeColor, Invert);
    FILLPOLYGON(this, tri, cx, cy);
}

void CfxCanvas::DrawCompass(FXRECT *pRect, FXFONTRESOURCE *pFont, COLOR BackColor, COLOR ForeColor, bool Invert, int Heading, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    int textSpacer = (pFont != nullptr) ? FontHeight(pFont) + 1 : 0;

    int cx = pRect->Left + (pRect->Right - pRect->Left + 1) / 2;
    int cy = pRect->Top + (pRect->Bottom - pRect->Top + 1) / 2;
    int thick = State.LineWidth;
    int thick2 = thick * 2;
    int thick4 = thick * 4;
    int thick8 = thick * 8;

    FXRECT rect = *pRect;
    InflateRect(&rect, -thick4, -thick4);
    InflateRect(&rect, -textSpacer, -textSpacer);
    int w = rect.Right - rect.Left + 1;
    int h = rect.Bottom - rect.Top + 1;
    int t = min(w, h);
    int t2 = t / 2;
    int t4 = max(t / 8, 4);
    State.LineColor = InvertColor(ForeColor, Invert);
    Ellipse(cx, cy, t2 - 1, t2 - 1);
    
    int segLen = (t * 5 + 11) / 12;

    State.BrushColor = InvertColor(ForeColor, Invert);
    //FillRect(cx - thick - segLen, cy, segLen, thick);
    //FillRect(cx + thick2, cy, segLen, thick);
    //FillRect(cx, cy - thick - segLen, thick, segLen);
    //FillRect(cx, cy + thick2, thick, segLen);

    FXPOINTLIST polygon;
    FXPOINT tri1[] = {{-t4, 0}, {0, t2 - thick8}, {t4, 0}}; 
    FXPOINT tri2[] = {{-t4, 0}, {0, -t2 + thick8}, {t4, 0}}; 

    int i;
    FXPOINT *p;

    double headingRad = DEG2RAD((360 + ((180 + 360) - Heading)) % 360);
    double cosHeading = cos(headingRad);
    double sinHeading = sin(headingRad);
    double x, y;

    for (i = 0; i < ARRAYSIZE(tri1); i++)
    {
        p = &tri1[i];
        x = cosHeading * p->X - sinHeading * p->Y;
        y = sinHeading * p->X + cosHeading * p->Y;
        p->X = (int)x;
        p->Y = (int)y;

        p = &tri2[i];
        x = cosHeading * p->X - sinHeading * p->Y;
        y = sinHeading * p->X + cosHeading * p->Y;
        p->X = (int)x;
        p->Y = (int)y;
    }

    State.BrushColor = InvertColor(ForeColor, Invert);
    FILLPOLYGON(this, tri1, cx, cy);
    POLYGON(this, tri1, cx, cy);
    POLYGON(this, tri2, cx, cy);

    State.BrushColor = InvertColor(ForeColor, Invert);
    for (i = 0; i < 16; i++)
    {
        headingRad = DEG2RAD((double)i * (360.0 / 16.0));
        cosHeading = cos(headingRad);
        sinHeading = sin(headingRad);

        int ax = 0;
        int ay = -t2;
        bool s = ((i & 1) != 0);

        x = cosHeading * ax - sinHeading * ay;
        y = sinHeading * ax + cosHeading * ay;

        FillEllipse((int)x + cx, (int)y + cy, s ? thick2 : thick4, s ? thick2 : thick4);

        if (pFont && !s)
        {
            static const char* headings[] =
            {
                "N", "NE", "E", "SE", "S", "SW", "W", "NW"
            };

            int fontHeight = FontHeight(pFont);
            ax = 0;
            ay = -t2 - fontHeight;
            x = cosHeading * ax - sinHeading * ay;
            y = sinHeading * ax + cosHeading * ay;

            FXRECT textRect;
            textRect.Left = textRect.Right = (int)x + cx;
            textRect.Top = textRect.Bottom = (int)y + cy;
            InflateRect(&textRect, fontHeight, fontHeight / 2);

            DrawTextRect(pFont, &textRect, TEXT_ALIGN_VCENTER | TEXT_ALIGN_HCENTER, headings[i / 2]);
        }
    }
}

void CfxCanvas::DrawFlag(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Fill, bool Invert, bool Transparent, bool Center)
{
    State.BrushColor = InvertColor(BackColor, Invert);
    if (!Transparent)
    {
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    int t = (pRect->Right - pRect->Left + 1) / 4;
    int l = Center ? t : 0;
   
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

void CfxCanvas::DrawCross(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, int Thickness, bool Invert, bool Transparent)
{
    State.BrushColor = InvertColor(BackColor, Invert);
    if (!Transparent)
    {
        FillRect(pRect);
    }
    
    if (Thickness >= 2)
    {
        State.BrushColor = InvertColor(ForeColor, Invert);

        int w = pRect->Right - pRect->Left + 1;
        int h = pRect->Bottom - pRect->Top + 1;
        int nh = min(h, w);

        int x = pRect->Left + (w - nh) / 2;
        int y = pRect->Top + (h - nh) / 2;
        h = nh;

        FXPOINTLIST polygon;
        FXPOINT triangle1[] = {{0, (int)Thickness}, {h-(int)Thickness, h}, {h, h-(int)Thickness}, {(int)Thickness, 0}};
        FILLPOLYGON(this, triangle1, x, y);

        FXPOINT triangle2[] = {{h-(int)Thickness, 0}, {0, h-(int)Thickness}, {(int)Thickness, h}, {h, (int)Thickness}};
        FILLPOLYGON(this, triangle2, x, y);
    }
    else
    {
        State.LineColor = InvertColor(ForeColor, Invert);

        int x1 = pRect->Left;
        int y1 = pRect->Top;
        int x2 = pRect->Right;
        int y2 = pRect->Bottom;

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

void CfxCanvas::DrawCheck(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    State.BrushColor = InvertColor(BackColor, Invert);
    if (!Transparent)
    {
        FillRect(pRect);
    }

    State.BrushColor = InvertColor(ForeColor, Invert);
    State.LineColor = InvertColor(ForeColor, Invert);

    int w = pRect->Right - pRect->Left + 1;
    int h = pRect->Bottom - pRect->Top + 1;
    int nh = min(h, w);
    int nh2 = nh / 3 * 2;
    int nh3 = nh / 3 * 3;

    int x = pRect->Left + (w - nh3) / 2 + 1;
    int y = pRect->Top + (h - nh2) / 2 + 1 - nh/3;

    h = nh;

    FXPOINTLIST polygon;
    FXPOINT triangle1[] = {{0, h-h/3}, {h/3, h}, {h/3 + 2, h-2}, {2, h-h/3-2}};
    FILLPOLYGON(this, triangle1, x, y);

    FXPOINT triangle2[] = {{h/3, h}, {(h/3)*3, h-h/3*2}, {(h/3)*3-2, h-h/3*2-2}, {h/3-2, h-2}};
    FILLPOLYGON(this, triangle2, x, y);
}

void CfxCanvas::DrawAction(FXRECT *pRect)
{
    State.BrushColor = 0;
    FillRect(pRect);

    State.LineColor = 0xFFFFFF;
    State.BrushColor = 0;
    
    FXRECT rect = *pRect;
    for (int i=0; i<4; i++)
    {
        Rectangle(&rect);
        InflateRect(&rect, -2, -2);
    }

    State.BrushColor = 0xFFFFFF;
    FillRect(&rect);
}

void CfxCanvas::DrawHome(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    int w1 = pRect->Right - pRect->Left - 1;
    int w2 = w1 / 2;
    int h1 = pRect->Bottom - pRect->Top - 1;
    int h2 = h1 / 2;
    int h = min(h2, w2);

    int x1 = pRect->Left + w2 + 1;
    int y1 = pRect->Top + h2 - h + 2;
    int w = 1;
    
    for (int i=0; i<h; i++)
    {
        FillRect(x1, y1, w, 1);
        x1--;
        y1++;
        w+=2;
    }

    int spacer = 1 + h/3;
    x1 += spacer;
    w = w - spacer * 2;
    FillRect(x1, y1, w, h-1);
}

void CfxCanvas::DrawRange(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    int radius = (pRect->Bottom - pRect->Top) / 2;

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

void CfxCanvas::DrawBearing(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    int radius = (pRect->Bottom - pRect->Top) / 2;

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

void CfxCanvas::DrawInclination(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    int radius = (pRect->Bottom - pRect->Top) / 2;

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

void CfxCanvas::DrawRoll(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    int radius = (pRect->Bottom - pRect->Top) / 2;

    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);

    Ellipse(pRect->Left + radius, pRect->Top + radius, radius, radius);
    Line(pRect->Left, pRect->Top + radius, pRect->Left + radius*2, pRect->Top + radius);
}

void CfxCanvas::DrawUpload(bool Filled, FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    int w = pRect->Right - pRect->Left + 1;
    int h = pRect->Bottom - pRect->Top + 1;

    int t = (int)min(w, h) - 1;
    int x = pRect->Left + (w - t) / 2 + 1;
    int y = pRect->Top + (h - t) / 2 + 1;
    int triSide = t / 2;

    y += (t - (t * 5) / 6) / 2 - 1;

    State.BrushColor = InvertColor(ForeColor, Invert);

    // Draw box
    FXRECT rect;
    int boxW    = t / 2;
    int boxH    = t / 3;
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

void CfxCanvas::DrawHeading(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, int Heading, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    FXRECT rect = *pRect;
    int w = rect.Right - rect.Left + 1;
    int h = rect.Bottom - rect.Top + 1;
    int cx = pRect->Left + w / 2;
    int cy = pRect->Top + h / 2;

    int t = min(w, h) - 1;
    InflateRect(&rect, - (t / 12), - (t / 12));
    w = rect.Right - rect.Left + 1;
    h = rect.Bottom - rect.Top + 1;
    t = min(w, h) - 1;

    int ttri = t / 3;
    int tbox = (t * 2) / 3;
    int t12 = t / 12;

    FXPOINTLIST polygon;
    FXPOINT tri[] = {{-40, -20}, {0, -60}, {40, -20}}; 
    FXPOINT box[] = {{20, -30}, {20, 55}, {-20, 55}, {-20, -30}};

    int i;
    FXPOINT *p;

    double headingRad = DEG2RAD(Heading);
    double cosHeading = cos(headingRad);
    double sinHeading = sin(headingRad);
    double x, y;

    for (i = 0; i < ARRAYSIZE(tri); i++)
    {
        p = &tri[i];
        p->X *= t12;
        p->Y *= t12;
        x = cosHeading * p->X - sinHeading * p->Y;
        y = sinHeading * p->X + cosHeading * p->Y;
        p->X = (int)x / 10;
        p->Y = (int)y / 10;
    }

    for (i = 0; i < ARRAYSIZE(box); i++)
    {
        p = &box[i];
        p->X *= t12;
        p->Y *= t12;
        x = cosHeading * p->X - sinHeading * p->Y;
        y = sinHeading * p->X + cosHeading * p->Y;
        p->X = (int)x / 10;
        p->Y = (int)y / 10;
    }

    State.BrushColor = InvertColor(ForeColor, Invert);
    FILLPOLYGON(this, tri, cx, cy);
    FILLPOLYGON(this, box, cx, cy);

    State.LineColor = InvertColor(ForeColor, Invert);
    Ellipse(cx, cy, t/2 + 5 * State.LineWidth, t/2 + 5 * State.LineWidth);
}

void CfxCanvas::DrawBusy(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert)
{
    State.BrushColor = InvertColor(BackColor, Invert);
    FillRect(pRect);

    FXRECT rect = *pRect;
    int w = rect.Right - rect.Left + 1;
    int h = rect.Bottom - rect.Top + 1;
    
    rect.Right = rect.Left + w / 2;
    rect.Bottom = rect.Top + h / 2;
    State.BrushColor = InvertColor(ForeColor, Invert);
    FillRect(&rect);

    rect = *pRect;
    rect.Left = rect.Right - w / 2;
    rect.Top = rect.Bottom - h / 2;
    FillRect(&rect);
}

int CfxCanvas::DrawTime(FXDATETIME *pDateTime, FXRECT *pRect, FXFONTRESOURCE *pFont, int AlignFlags, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    char timeText[16];
    sprintf(timeText, "%02d:%02d", pDateTime->Hour, pDateTime->Minute);
    State.TextColor = InvertColor(ForeColor, Invert);
    DrawTextRect(pFont, pRect, AlignFlags, timeText);

    return TextWidth(pFont, timeText);
}

int CfxCanvas::DrawBattery(int Level, FXRECT *pRect, int AlignFlags, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }

    FXRECT rect = *pRect;
    rect.Top += State.LineWidth;
    rect.Bottom -= State.LineWidth;
    int w = rect.Right - rect.Left + 1;
    int h = rect.Bottom - rect.Top + 1;
    int h4 = h / 4;

    int nw = min(w, h) * 2;
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

    int lw = State.LineWidth;

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

void CfxCanvas::DrawShare(FXRECT *pRect, COLOR BackColor, COLOR ForeColor, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        State.BrushColor = InvertColor(BackColor, Invert);
        FillRect(pRect);
    }
    State.BrushColor = InvertColor(ForeColor, Invert);
    State.LineColor = InvertColor(ForeColor, Invert);

    int w = pRect->Right - pRect->Left - 1;
    int h = pRect->Bottom - pRect->Top - 1;

    int r = min(w, h) / 8;
    int x = pRect->Left + 1;
    int y = pRect->Top + 1;
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

void CfxCanvas::Rectangle(int Left, int Top, int Width, int Height)
{
    if ((Width == 0) || (Height == 0)) return;
    FXRECT rect = {Left, Top, Left + (int)Width - 1, Top + (int)Height -1};
    Rectangle(&rect);
} 

void CfxCanvas::Rectangle(FXRECT *pRect)
{
    FillRect(pRect);
    if (State.LineColor != State.BrushColor)
    {
        FrameRect(pRect);
    }
} 

void CfxCanvas::FillRect(int Left, int Top, int Width, int Height)
{
	if ((Width == 0) || (Height == 0)) return;
    FXRECT rect = {Left, Top, Left + (int)Width - 1, Top + (int)Height - 1};
    FillRect(&rect);
} 

void CfxCanvas::FillRect(FXRECT *pRect)
{
    FXRECT rect = *pRect;
    if (!ClipRect(&rect)) return;

    SetBlock(rect.Left, rect.Top, rect.Right, rect.Bottom, State.BrushColor);
} 

void CfxCanvas::FrameRect(int Left, int Top, int Width, int Height)
{
    if ((Width == 0) || (Height == 0)) return;
    FXRECT rect = {Left, Top, Left + (int)Width - 1, Top + (int)Height - 1};
    FrameRect(&rect);
} 

void CfxCanvas::FrameRect(FXRECT *pRect)
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

            int x1 = pRect->Left;
            int y1 = pRect->Top;
            int w = pRect->Right - x1 + 1;
            int h = pRect->Bottom - y1 + 1;

            int lw = min(w, State.LineWidth);
            int lh = min(h, State.LineWidth);

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

int CfxCanvas::FontHeight(FXFONTRESOURCE *pFont)
{
    return (pFont->Height * _scaleFont) / 100;
}

//*************************************************************************************************
// Bitmap

void CfxCanvas::Draw(CfxCanvas *pCanvas, int X, int Y)
{
    // Blank bitmap
    if (!pCanvas->_width || !pCanvas->_height)
    {
        return;
    }

    // Entirely outside
    if ((X > State.ClipRect.Right) || (Y > State.ClipRect.Bottom) ||
        (X + (int)pCanvas->_width < State.ClipRect.Left) || (Y + (int)pCanvas->_height < State.ClipRect.Top))
    {
        return;
    }

    int X1 = 0;
    int Y1 = 0;
    int X2 = pCanvas->_width-1;
    int Y2 = pCanvas->_height-1;
    int DX = X;
    int DY = Y;

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

void CfxCanvas::Draw(FXBITMAPRESOURCE *pBitmap, int X, int Y, bool Invert)
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

    int X1 = 0;
    int Y1 = 0;
    int X2 = pBitmap->Width-1;
    int Y2 = pBitmap->Height-1;
    int DX = X;
    int DY = Y;

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

void CfxCanvas::StretchDraw(FXBITMAPRESOURCE *pBitmap, int X, int Y, int Width, int Height, bool Invert, bool Transparent)
{
    // Blank bitmap
    if (!pBitmap->Width || !pBitmap->Height)
    {
        return;
    }

    // Entirely outside
    if ((X > State.ClipRect.Right) || (Y > State.ClipRect.Bottom) ||
        (X + (int)Width < State.ClipRect.Left) || (Y + (int)Height < State.ClipRect.Top))
    {
        return;
    }

    int X1 = 0;
    int Y1 = 0;
    int X2 = pBitmap->Width-1;
    int Y2 = pBitmap->Height-1;
    int DX1 = X;
    int DY1 = Y;
    int DX2 = X + Width - 1;
    int DY2 = Y + Height - 1;

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

        int sw = X2 - X1;
        int sh = Y2 - Y1;
        int dw = DX2 - DX1;
        int dh = DY2 - DY1;

        int w, h;

        int nx1 = X1;
        int ny1 = Y1;

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
        float xScale = (float)X2 / (float)Width;
        float yScale = (float)Y2 / (float)Height;

        // Clip Left, Top
        if (DX1 < State.ClipRect.Left)
        {
            X1 = (int)((int)(State.ClipRect.Left - DX1) * xScale);
            DX1 = State.ClipRect.Left;
        }
        if (DY1 < State.ClipRect.Top)
        {
            Y1 = (int)((int)(State.ClipRect.Top - DY1) * yScale);
            DY1 = State.ClipRect.Top;
        }
        // Clip Right, Bottom
        if (DX2 >= State.ClipRect.Right)
        {
            X2 += (int)((int)(State.ClipRect.Right - DX2) * xScale);
            DX2 = State.ClipRect.Right;
        }
        if (DY2 > State.ClipRect.Bottom)
        {
            Y2 += (int)((int)(State.ClipRect.Bottom - DY2) * yScale);
            DY2 = State.ClipRect.Bottom;
        }

        // Copy the clipped rect
        StretchCopyRect(pBitmap, X1, Y1, X2, Y2, DX1, DY1, DX2, DY2, Invert, Transparent);
    }
} 

void CfxCanvas::StretchDraw(FXBITMAPRESOURCE *pBitmap, FXRECT *pSrcRect, FXRECT *pDstRect, bool Invert, bool Transparent)
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

    int dx1 = pDstRect->Left;
    int dy1 = pDstRect->Top;
    int dx2 = pDstRect->Right;
    int dy2 = pDstRect->Bottom;

    int sx1 = pSrcRect->Left;
    int sy1 = pSrcRect->Top;
    int sx2 = pSrcRect->Right;
    int sy2 = pSrcRect->Bottom;

    // Entirely outside
    if ((dx1 > State.ClipRect.Right) || (dy1 > State.ClipRect.Bottom) ||
        (dx2 < State.ClipRect.Left) || (dy2 < State.ClipRect.Top))
    {
        return;
    }

    int sw = sx2 - sx1 + 1;
    int sh = sy2 - sy1 + 1;
    int dw = dx2 - dx1 + 1;
    int dh = dy2 - dy1 + 1;

    float xScale = (float)sw / (float)dw;
    float yScale = (float)sh / (float)dh;

    // Clip Left, Top
    if (dx1 < State.ClipRect.Left)
    {
        sx1 = (int)((int)(State.ClipRect.Left - dx1) * xScale);
        dx1 = State.ClipRect.Left;
    }
    if (dy1 < State.ClipRect.Top)
    {
        sy1 = (int)((int)(State.ClipRect.Top - dy1) * yScale);
        dy1 = State.ClipRect.Top;
    }
    // Clip Right, Bottom
    if (dx2 > State.ClipRect.Right)
    {
        sx2 += (int)((int)(State.ClipRect.Right - dx2) * xScale);
        dx2 = State.ClipRect.Right;
    }
    if (dy2 > State.ClipRect.Bottom)
    {
        sy2 += (int)((int)(State.ClipRect.Bottom - dy2) * yScale);
        dy2 = State.ClipRect.Bottom;
    }

    // Copy the clipped rect
    StretchCopyRect(pBitmap, sx1, sy1, sx2, sy2, dx1, dy1, dx2, dy2, Invert, Transparent);
} 

//*************************************************************************************************
// Triangle

void ScanEdge(int X1, int Y1, int X2, int Y2, int *pBuffer)
{
    int dy = Y2 - Y1;
    int dx = X2 - X1;
    int dir = 1;
    
    if (dx < 0)
    {
        dx = -dx;
        dir = -dir;
    }

    if (dx >= dy)
    {
        int iy = dx / 2;

        *pBuffer = X1;
        pBuffer++;
        for (int i=0; i<=dx; i++)
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
        int ix = dy / 2;

        for (int i=0; i<=dy; i++)
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

void CfxCanvas::FillTriangle(int X1, int Y1, int X2, int Y2, int X3, int Y3)
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

    int startPosition = _renderBuffer.GetPosition();
    int position = (startPosition + 3) & ~3;
    int bufferSize = ((Y3 - Y1 + 1) + (Y2 - Y1 + 1) + (Y3 - Y2 + 1) + 3) * sizeof(int); 
    
    if (_renderBuffer.GetCapacity() < position + bufferSize * 2)
    {
        _renderBuffer.SetCapacity(position + bufferSize * 2);
    }

    int *fillA0 = (int *)((BYTE *)_renderBuffer.GetMemory() + position);
    int *fillA1 = (int *)((BYTE *)fillA0 + (Y3 - Y1 + 1) * sizeof(int));
    int *fillA2 = (int *)((BYTE *)fillA1 + (Y2 - Y1) * sizeof(int));

    ScanEdge(X1, Y1, X3, Y3, &fillA0[0]);
    ScanEdge(X1, Y1, X2, Y2, &fillA1[0]);
    ScanEdge(X2, Y2, X3, Y3, &fillA2[0]);

    for (int i=Y1; i<=Y3; i++)
    {
        int x1 = *fillA0;
        int x2 = *fillA1;
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

void CfxCanvas::Triangle(int X1, int Y1, int X2, int Y2, int X3, int Y3)
{
    MoveTo(X1, Y1);
    LineTo(X2, Y2);
    LineTo(X3, Y3);
    LineTo(X1, Y1);
}

//*************************************************************************************************
// Polygon 

void CfxCanvas::FillPolygon(FXPOINTLIST *VertexList, int X, int Y)
{
    FX_ASSERT(VertexList->Length >= 3);

    int x1 = VertexList->Points[0].X + X;
    int y1 = VertexList->Points[0].Y + Y;

    int *p = &(VertexList->Points[1].X);
    for (int i=VertexList->Length; i>2; i--)
    {
        int x2 = *(p)   + X;
        int y2 = *(p+1) + Y;
        int x3 = *(p+2) + X;
        int y3 = *(p+3) + Y;

        FillTriangle(x1, y1, x2, y2, x3, y3);
        p += 2;
    }
}

void CfxCanvas::Polygon(FXPOINTLIST *VertexList, int X, int Y)
{
    FXPOINT *point = VertexList->Points;
    int x1 = point->X;
    int y1 = point->Y;

    MoveTo(point->X + X, point->Y + Y);
    for (int i=1; i < VertexList->Length; i++)
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

typedef void (*FnSet4Pixels)(CfxCanvas *pCanvas, int X, int Y, int XC, int YC, void *pParams);

void CalcEllipse(int Xc, int Yc, int RadiusX, int RadiusY, CfxCanvas *pCanvas, FnSet4Pixels pSet4Pixels, void *pParams = 0)
{
    int x = 0;
    int y = RadiusY;

    int a = RadiusX;
    int b = RadiusY;

    int asquared = a * a;
    int twoasquared = 2 * asquared;
    int bsquared = b * b;
    int twobsquared = 2 * bsquared;

    int d  = bsquared - asquared * b + asquared / 4;
    int dx = 0;
    int dy = twoasquared * b;

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

//*************************************************************************************************
// RoundRect

void Set4PixelsRoundRect(CfxCanvas *pCanvas, int X, int Y, int Xc, int Yc, void *pParams)
{
    FXRECT *clipRect = &pCanvas->State.ClipRect;
    int x, y;

    int dx = ((FXPOINT *)pParams)->X;
    int dy = ((FXPOINT *)pParams)->Y;
    COLOR color = pCanvas->State.LineColor;

    switch (pCanvas->State.LineWidth)
    {
    case 0: break;
    case 1:
        {
            // Set clip parameters
            x = Xc - X;
            bool x1ok = (x >= clipRect->Left) && (x <= clipRect->Right);
            x = Xc + X + dx;
            bool x2ok = (x >= clipRect->Left) && (x <= clipRect->Right);
            y = Yc - Y;
            bool y1ok = (y >= clipRect->Top) && (y <= clipRect->Bottom);
            y = Yc + Y + dy;
            bool y2ok = (y >= clipRect->Top) && (y <= clipRect->Bottom);

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
            int radius = pCanvas->State.LineWidth;
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

void CfxCanvas::RoundRect(int Left, int Top, int Width, int Height, int CornerWidth)
{
    FXRECT rect = {Left, Top, Left + (int)Width - 1, Top + (int)Height -1};
    RoundRect(&rect, CornerWidth);
}

void CfxCanvas::RoundRect(FXRECT *pRect, int CornerWidth)
{
    FXRECT rect = *pRect;
    int width = rect.Right - rect.Left + 1;
    int height = rect.Bottom - rect.Top + 1;

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

void CfxCanvas::FillRoundRect(int Left, int Top, int Width, int Height, int CornerWidth)
{
    FXRECT rect = {Left, Top, Left + (int)Width - 1, Top + (int)Height -1};
    RoundRect(&rect, CornerWidth);
}

void Set4PixelsFillRoundRect(CfxCanvas *pCanvas, int X, int Y, int Xc, int Yc, void *pParams)
{
    COLOR color = pCanvas->State.BrushColor;
    FXRECT *clipRect = &pCanvas->State.ClipRect;

    int dx = ((FXPOINT *)pParams)->X;
    int dy = ((FXPOINT *)pParams)->Y;

    FXRECT rect = {Xc - X, Yc - Y, Xc + X + dx, Yc + Y + dy};
    IntersectRect(&rect, &rect, clipRect);
    
    pCanvas->SetBlock(rect.Left, rect.Top, rect.Right, rect.Top, color);
    pCanvas->SetBlock(rect.Left, rect.Bottom, rect.Right, rect.Bottom, color);
}

void CfxCanvas::FillRoundRect(FXRECT *pRect, int CornerWidth)
{
    FXRECT rect = *pRect;
    int width = rect.Right - rect.Left + 1;
    int height = rect.Bottom - rect.Top + 1;

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

void StoreCurve(CfxCanvas *pCanvas, int X, int Y, int Xc, int Yc, void *pParams)
{
    BYTE *offset = &((BYTE *)pParams)[Yc - Y];
    *offset = (BYTE)(Xc - X);
}

void CfxCanvas::PushCorners(FXRECT *pRect, int CornerWidth)
{
    CornerWidth = min(MAX_CORNER_WIDTH, CornerWidth);

    FXRECT rect = *pRect;

    int x1 = rect.Left;
    int y1 = rect.Top;
    int x2 = rect.Right;
    int y2 = rect.Bottom;

    int dX = (x2 - x1) - CornerWidth * 2;
    int dY = (y2 - y1) - CornerWidth * 2;

    int position = _renderBuffer.GetPosition();
    int header = 'PIXE';
    bool empty = true;
    _renderBuffer.Write(&header, sizeof(header));

    if (!ClipRect(&rect) || (dX <= 0) || (dY <= 0))
    {
        _renderBuffer.Write(&empty, sizeof(empty));
        _renderBuffer.Write(&position, sizeof(position));

        return;
    }

    CalcEllipse(CornerWidth, CornerWidth, CornerWidth, CornerWidth, this, &StoreCurve, &_cornerOffsets);

    empty = false;
    _renderBuffer.Write(&empty, sizeof(empty));
    _renderBuffer.Write(&CornerWidth, sizeof(CornerWidth));
    _renderBuffer.Write(&_cornerOffsets[0], CornerWidth * sizeof(BYTE));

    for (int j=0; j<CornerWidth; j++)
    {
        for (int i=0; i<_cornerOffsets[j]; i++)
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

void CfxCanvas::PopCorners(FXRECT *pRect)
{
    FXRECT rect = *pRect;

    int x1 = rect.Left;
    int x2 = rect.Right;
    int y1 = rect.Top;
    int y2 = rect.Bottom;

    int position;
    _renderBuffer.SetPosition(_renderBuffer.GetPosition() - sizeof(position));
    _renderBuffer.Read(&position, sizeof(position));
    _renderBuffer.SetPosition(position);

    int header;
    _renderBuffer.Read(&header, sizeof(header));
    FX_ASSERT(header == 'PIXE');

    bool empty;
    _renderBuffer.Read(&empty, sizeof(empty));
    if (empty)
    {
        _renderBuffer.SetPosition(position);
        return;
    }

    int cornerWidth;
    _renderBuffer.Read(&cornerWidth, sizeof(cornerWidth));
    FX_ASSERT(cornerWidth <= MAX_CORNER_WIDTH);

    int dX = (x2 - x1) - cornerWidth * 2;
    int dY = (y2 - y1) - cornerWidth * 2;

    _renderBuffer.Read(&_cornerOffsets[0], cornerWidth * sizeof(BYTE));

    for (int j=0; j<cornerWidth; j++)
    {
        for (int i=0; i<_cornerOffsets[j]; i++)
        {
            COLOR colors[4];

            _renderBuffer.Read(&colors[0], sizeof(colors));

            int x, y;

            x = x1 + i;
            bool x1ok = (x >= State.ClipRect.Left) && (x <= State.ClipRect.Right);
            x = x2 - i;
            bool x2ok = (x >= State.ClipRect.Left) && (x <= State.ClipRect.Right);
            y = y1 + j;
            bool y1ok = (y >= State.ClipRect.Top) && (y <= State.ClipRect.Bottom);
            y = y2 - j;
            bool y2ok = (y >= State.ClipRect.Top) && (y <= State.ClipRect.Bottom);

            if (x1ok && y1ok) SetPixel(x1 + i, y1 + j, colors[0]);
            if (x2ok && y1ok) SetPixel(x2 - i, y1 + j, colors[1]);
            if (x1ok && y2ok) SetPixel(x1 + i, y2 - j, colors[2]);
            if (x2ok && y2ok) SetPixel(x2 - i, y2 - j, colors[3]);
        }
    }

    _renderBuffer.SetPosition(position);
}

void *CfxCanvas::MakeFont(const char *pName, UINT16 Height, UINT16 Style)
{
    QFont *font = new QFont(pName);
    font->setPixelSize(Height);

    if (Style & 1) {
        font->setBold(true);
    }

    if (Style & 2) {
        font->setItalic(true);
    }

    font->setStyleStrategy(QFont::PreferAntialias);

    return (void *)font;
}

void CfxCanvas::FreeFont(void *Handle)
{
    QFont *font = (QFont *)Handle;
    delete font;
}

int CfxCanvas::TextWidth(FXFONTRESOURCE *pFont, const char *pText, int Length)
{
    QFont *font = (QFont *)FindFont(pFont);
    QFontMetrics fontMetrics(*font);
    char *pCleanText = AllocCleanText(pText, &Length, true);
    int w = fontMetrics.boundingRect(pCleanText).width();
    MEM_FREE(pCleanText);
    return (int)w;
}

int CfxCanvas::TextHeight(FXFONTRESOURCE *pFont, const char *pText, int Length, int *pOffsetY)
{
    QFont *font = (QFont *)FindFont(pFont);

    QFontMetrics fontMetrics(*font);
    auto br = fontMetrics.boundingRect(pText);
    int w = br.width();
    int h = fontMetrics.height();
    QImage image(w, h, QImage::Format_RGB32);
    QPainter painter(&image);

    image.fill(QColor(0, 0, 0, 255));
    painter.setFont(*font);
    painter.setPen(QColor(255, 255, 255, 255));
    painter.drawText(QRect(0, 0, w, h), Qt::TextFlag::TextDontClip, "0");

    int minY = h;
    int maxY = 0;

    for (int j = 0; j < h; j++)
    {
        UINT32 *bits = (UINT32 *)image.scanLine(j);
        for (int i = w; i > 0; i--, bits++)
        {
            auto color = *bits & 0xFFFFFF;
            if (color != 0)
            {
                minY = min(j, minY);
                maxY = max(j, maxY);
                break;
            }
        }
    }

    if (pOffsetY != nullptr)
    {
        *pOffsetY = minY;
    }

    return maxY - minY;
}

void CfxCanvas::CalcTextRect(FXFONTRESOURCE *pFont, const char *pText, FXRECT *pRect)
{
    if (!pText)
    {
        pRect->Right = pRect->Left;
        pRect->Bottom = pRect->Top;
        return;
    }

    pRect->Left = pRect->Top = 0;

    char *pCleanText = AllocCleanText(pText, nullptr, false);
    QFont *font = (QFont *)FindFont(pFont);
    QFontMetrics fontMetrics(*font);
    QRect b = fontMetrics.boundingRect(0, 0, pRect->Right - 1, pRect->Top - 1, Qt::AlignLeft | Qt::TextWordWrap, pCleanText);
    pRect->Bottom = b.bottom();
    MEM_FREE(pCleanText);
}

void CfxCanvas::DrawText(FXFONTRESOURCE *pFont, int X, int Y, const char *pText, int Length)
{
    FXRECT rect;
    rect.Left = X;
    rect.Top = Y;
    rect.Right = max(X, _width - 1);
    rect.Bottom = max(Y, _height - 1);

    DrawTextRect(pFont, &rect, TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP, pText, Length);
}

void CfxCanvas::DrawTextRect(FXFONTRESOURCE *pFont, FXRECT *pRect, int AlignFlags, const char *pText, int Length)
{
    _painter.setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    int Flags = 0;

    if (AlignFlags & TEXT_ALIGN_LEFT) {
        Flags |= Qt::AlignLeft;
    }

    if (AlignFlags & TEXT_ALIGN_TOP) {
        Flags |= Qt::AlignTop;
    }

    if (AlignFlags & TEXT_ALIGN_RIGHT) {
        Flags |= Qt::AlignRight;
    }

    if (AlignFlags & TEXT_ALIGN_BOTTOM) {
        Flags |= Qt::AlignBottom;
    }

    if (AlignFlags & TEXT_ALIGN_HCENTER) {
        Flags |= Qt::AlignCenter;
    }

    if (AlignFlags & TEXT_ALIGN_VCENTER) {
        Flags |= Qt::AlignVCenter;
    }

    bool singleLine = ((AlignFlags & TEXT_ALIGN_BOTTOM) != 0) ||
                      ((AlignFlags & TEXT_ALIGN_WORDWRAP) == 0);
    if (singleLine) {
        Flags |= Qt::TextSingleLine;
    }

    if (AlignFlags & TEXT_ALIGN_WORDWRAP) {
        Flags |= Qt::TextWordWrap;
    }


    QFont *font = (QFont *)FindFont(pFont);
    char *pCleanText = AllocCleanText(pText, &Length, singleLine);

    const QFont &oldFont = _painter.font();
    _painter.setPen(QColor(FixColor(State.TextColor)));

    _painter.setFont(*font);
    _painter.drawText(pRect->Left, pRect->Top,
                       pRect->Right - pRect->Left + 1,
                       pRect->Bottom - pRect->Top + 1,
                       Flags,
                       pCleanText);
    _painter.setFont(oldFont);

    MEM_FREE(pCleanText);
}

void CfxCanvas::DrawTextRect(FXFONTRESOURCE *pFont, FXRECT *pRect, int AlignFlags, const QString &Text)
{
    QByteArray utf8 = Text.toUtf8();
    DrawTextRect(pFont, pRect, AlignFlags, utf8.constData(), utf8.size());
}

void CfxCanvas::SetSize(int Width, int Height)
{
    if ((_width == Width) && (_height == Height)) return;

    if (_painter.isActive())
    {
        _painter.end();
    }
    _image = QImage(Width, Height, QImage::Format_RGB32);
    _image.fill(0xFFFFFF);
    _painter.begin(&_image);
    _painter.setRenderHint(QPainter::TextAntialiasing);

    _width = Width;
    _height = Height;

    ClearClipRect();
}

COLOR CfxCanvas::GetPixel(int X, int Y)
{
    return (COLOR)_image.pixel(QPoint(X, Y));
}

void CfxCanvas::SetPixel(int X, int Y, COLOR Color)
{
     _painter.fillRect(X, Y, 1, 1, QColor(Color));
}

void CfxCanvas::InvertBlock(int X1, int Y1, int X2, int Y2)
{
    QImage tempImage(X2 - X1 + 1, Y2 - Y1 + 1, QImage::Format_RGB32);
    QPainter tempPainter(&tempImage);
    tempPainter.drawImage(0, 0, _image, X1, Y1, X2 - X1 + 1, Y2 - Y1 + 1);
    tempImage.invertPixels();

    _painter.setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    _painter.drawImage(X1, Y1, tempImage);
}

void CfxCanvas::SetBlock(int X1, int Y1, int X2, int Y2, COLOR Color)
{
    if ((X1 > X2) || (Y1 > Y2)) return;

    int height = Y2 - Y1 + 1;
    int width = X2 - X1 + 1;
    UINT32 c = FixColor(Color);

    _painter.setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    _painter.fillRect(X1, Y1, width, height, c);
}

void CfxCanvas::StretchCopyRect(CfxCanvas *pCanvas, int X1, int Y1, int X2, int Y2, int DX1, int DY1, int DX2, int DY2)
{
    QImage *srcImage = pCanvas->GetImage();
    QPixmap pixMap;

    if (!pixMap.convertFromImage(*srcImage)) {
        return;
    }

    _painter.setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    _painter.drawPixmap(DX1, DY1, DX2 - DX1 + 1, DY2 - DY1 + 1,
                         pixMap,
                         X1, Y1, X2 - X1 + 1, Y2 - Y1 + 1);
}

void CfxCanvas::CopyRect(CfxCanvas *pCanvas, int X1, int Y1, int X2, int Y2, int DX1, int DY1)
{
    StretchCopyRect(pCanvas, X1, Y1, X2, Y2, DX1, DY1, DX1 + (X2 - X1), DY1 + (Y2 - Y1));
}

void CfxCanvas::StretchCopyRect(FXBITMAPRESOURCE *pBitmap, int X1, int Y1, int X2, int Y2, int DX1, int DY1, int DX2, int DY2, bool Invert, bool Transparent)
{
    QImage::Format format = (pBitmap->BitDepth == 16) ? QImage::Format_RGB16 : QImage::Format_RGB32;
    QImage srcImage(&pBitmap->Pixels[0], pBitmap->Width, pBitmap->Height, pBitmap->RowBytes, format);
    QPixmap pixMap;

    if (!pixMap.convertFromImage(srcImage)) {
        return;
    }

    _painter.setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    if (Transparent) {
        pixMap.setMask(pixMap.createMaskFromColor(srcImage.pixel(0, 0), Qt::MaskInColor));
    }

    _painter.drawPixmap(DX1, DY1, DX2 - DX1 + 1, DY2 - DY1 + 1,
                         pixMap,
                         X1, Y1, X2 - X1 + 1, Y2 - Y1 + 1);

    if (Invert) {
        InvertBlock(DX1, DY1, DX2, DY2);
    }
}

void CfxCanvas::CopyRect(FXBITMAPRESOURCE *pBitmap, int X1, int Y1, int X2, int Y2, int DX1, int DY1, bool Invert)
{
    StretchCopyRect(pBitmap, X1, Y2, X2, Y2, DX1, DY1, DX1 + (X2 - X1), DY1 + (Y2 - Y1), Invert, false);
}

void CfxCanvas::SystemDrawText(const char *pText, FXRECT *pRect, COLOR Color, COLOR BkColor)
{
    SetBlock(pRect->Left, pRect->Top, pRect->Right, pRect->Bottom, BkColor);

    const QFont &oldFont = _painter.font();

    _painter.setPen(QColor(Color));
    _painter.setFont(QFont("Arial", (16 * _scaleFont) / 100));
    _painter.drawText(pRect->Left, pRect->Top,
                       pRect->Right - pRect->Left + 1,
                       pRect->Bottom - pRect->Top + 1,
                       Qt::AlignCenter | Qt::AlignVCenter,
                       pText);
    _painter.setFont(oldFont);

}

void CfxCanvas::ImportLine(int X, int Y, COLOR *pBuffer, int Size)
{
    COLOR *pSrc = pBuffer;
    UINT32 *pDst = (UINT32 *)_image.scanLine(Y) + X;

    for (int i=0; i<Size; i++) {
        *pDst = FixColor(*pSrc);
        pSrc++;
        pDst++;
    }
}

void CfxCanvas::LoadFromFile(const char *pFileName, int Width, int Height)
{
    QImageReader imageReader(pFileName);
    imageReader.setAutoTransform(true);
    QImage image = imageReader.read();

    if ((image.width() == 0) || (image.height() == 0)) {
        return;
    }

    int w = image.width();
    int h = image.height();
    int cw = Width;
    int ch = Height;
    float xyaspect;

    if ((w > cw) || (h > ch))
    {
        if ((w > 0) && (h > 0))
        {
            xyaspect = (float)w / (float)h;
            if (w > h)
            {
                w = cw;
                h = (int)((float)cw / xyaspect);
                if (h > ch) // whoops, too big
                {
                    h = ch;
                    w = (int)((float)ch * xyaspect);
                }
            }
            else
            {
                h = ch;
                w = (int)((float)ch * xyaspect);
                if (w > cw) // whoops, too big
                {
                    w = cw;
                    h = (int)((float)cw / xyaspect);
                }
            }
        }
        else
        {
            w = cw;
            h = ch;
        }
    }

    SetSize(w, h);
    _painter.drawImage(QRect(0, 0, w, h), image, QRect(0, 0, image.width(), image.height()));
}

FXBITMAPRESOURCE *CfxCanvas::CreateBitmapFromFile(const char *pFileName, int Width, int Height)
{
    FXBITMAPRESOURCE *result = nullptr;
    CfxCanvas canvas(this, _scaleFont);

    canvas.LoadFromFile(pFileName, Width, Height);

    if ((canvas._width == 0) || (canvas._height == 0)) {
        return result;
    }

    int size = canvas.GetImage()->bytesPerLine() * canvas._height;

    result = (FXBITMAPRESOURCE *)MEM_ALLOC(size + sizeof(FXBITMAPRESOURCE));
    if (result == nullptr) {
        return result;
    }

    result->Magic = MAGIC_BITMAP;
    result->Width = (UINT16)canvas._width;
    result->Height = (UINT16)canvas._height;
    result->BitDepth = 32;
    result->RowBytes = (UINT16)canvas.GetImage()->bytesPerLine();
    result->Compress = 0;
    result->Size = size;

    memcpy(&result->Pixels[0], canvas.GetImage()->bits(), size);

    return result;
}

void CfxCanvas::FillEllipse(int Xc, int Yc, int RadiusX, int RadiusY)
{
    _painter.setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    _painter.setBrush(QColor(FixColor(State.BrushColor)));
    _painter.setPen(Qt::NoPen);
    _painter.setRenderHint(QPainter::Antialiasing);
    _painter.drawEllipse(QPoint(Xc, Yc), RadiusX, RadiusY);
}

void CfxCanvas::Ellipse(int Xc, int Yc, int RadiusX, int RadiusY)
{
    _painter.setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    QPen pen;
    pen.setColor(QColor(FixColor(State.LineColor)));
    pen.setWidth(State.LineWidth);

    _painter.setBrush(Qt::NoBrush);
    _painter.setPen(pen);
    _painter.setRenderHint(QPainter::Antialiasing);
    _painter.drawEllipse(QPoint(Xc, Yc), RadiusX, RadiusY);
}
