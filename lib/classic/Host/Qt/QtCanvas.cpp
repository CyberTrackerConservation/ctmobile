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

#include "QtCanvas.h"
#include "fxApplication.h"
#include "fxUtils.h"
#include "QtUtils.h"
#include <QBitmap>

CHAR *AllocCleanText(const CHAR *pText, UINT *pLength, BOOL SingleLine)
{
    UINT Length = pLength ? *pLength : 0;
    Length = Length ? Length : strlen(pText);
    UINT OutLength = 0;

    CHAR *pOutText = (CHAR *)MEM_ALLOC(Length + 1);
    CHAR *pOutCurr = pOutText;

    const CHAR *pInCurr = pText;

    for (UINT i = 0; i < Length; i++, pInCurr++) {
        CHAR c = *pInCurr;

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

CCanvas_Qt::CCanvas_Qt(CfxPersistent *pOwner, UINT ScaleFont): CfxCanvas(pOwner, 32, ScaleFont), _image(NULL), _painter(NULL)
{
}

CCanvas_Qt::~CCanvas_Qt()
{
    delete _painter;
    delete _image;
}

VOID *CCanvas_Qt::MakeFont(CHAR *pName, UINT16 Height, UINT16 Style)
{
    QFont *font = new QFont(pName);
    font->setPixelSize(Height);

    if (Style & 1) {
        font->setBold(TRUE);
    }

    if (Style & 2) {
        font->setItalic(TRUE);
    }

    font->setStyleStrategy(QFont::PreferAntialias);

    return (VOID *)font;
}

VOID CCanvas_Qt::FreeFont(VOID *Handle)
{
    QFont *font = (QFont *)Handle;
    delete font;
}

UINT CCanvas_Qt::TextWidth(FXFONTRESOURCE *pFont, CHAR *pText, UINT Length)
{
    QFont *font = (QFont *)FindFont(pFont);
    QFontMetrics fontMetrics(*font);
    CHAR *pCleanText = AllocCleanText(pText, &Length, TRUE);
    UINT w = fontMetrics.boundingRect(pCleanText).width();
    MEM_FREE(pCleanText);
    return (UINT)w;
}

UINT CCanvas_Qt::TextHeight(FXFONTRESOURCE *pFont, CHAR *pText, UINT Length, INT *pOffsetY)
{
    QFont *font = (QFont *)FindFont(pFont);

    QFontMetrics fontMetrics(*font);
    auto br = fontMetrics.boundingRect(pText);
    UINT w = br.width();
    UINT h = fontMetrics.height();
    QImage image(w, h, QImage::Format_RGB32);
    QPainter painter(&image);

    image.fill(QColor(0, 0, 0, 255));
    painter.setFont(*font);
    painter.setPen(QColor(255, 255, 255, 255));
    painter.drawText(QRect(0, 0, w, h), Qt::TextFlag::TextDontClip, "0");

    INT minY = h;
    INT maxY = 0;

    for (INT j = 0; j < h; j++)
    {
        UINT32 *bits = (UINT32 *)image.scanLine(j);
        for (INT i = w; i > 0; i--, bits++)
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

    if (pOffsetY != NULL)
    {
        *pOffsetY = minY;
    }

    return maxY - minY;
}

VOID CCanvas_Qt::CalcTextRect(FXFONTRESOURCE *pFont, CHAR *pText, FXRECT *pRect)
{
    if (!pText)
    {
        pRect->Right = pRect->Left;
        pRect->Bottom = pRect->Top;
        return;
    }

    pRect->Left = pRect->Top = 0;

    CHAR *pCleanText = AllocCleanText(pText, NULL, FALSE);
    QFont *font = (QFont *)FindFont(pFont);
    QFontMetrics fontMetrics(*font);
    QRect b = fontMetrics.boundingRect(0, 0, pRect->Right - 1, pRect->Top - 1, Qt::AlignLeft | Qt::TextWordWrap, pCleanText);
    pRect->Bottom = b.bottom();
    MEM_FREE(pCleanText);
}

VOID CCanvas_Qt::DrawText(FXFONTRESOURCE *pFont, INT X, INT Y, CHAR *pText, UINT Length)
{
    FXRECT rect;
    rect.Left = X;
    rect.Top = Y;
    rect.Right = max(X, _width - 1);
    rect.Bottom = max(Y, _height - 1);

    DrawTextRect(pFont, &rect, TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP, pText, Length);
}

VOID CCanvas_Qt::DrawTextRect(FXFONTRESOURCE *pFont, FXRECT *pRect, UINT AlignFlags, CHAR *pText, UINT Length)
{
    _painter->setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    UINT Flags = 0;

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

    BOOL singleLine = ((AlignFlags & TEXT_ALIGN_BOTTOM) != 0) ||
                      ((AlignFlags & TEXT_ALIGN_WORDWRAP) == 0);
    if (singleLine) {
        Flags |= Qt::TextSingleLine;
    }

    if (AlignFlags & TEXT_ALIGN_WORDWRAP) {
        Flags |= Qt::TextWordWrap;
    }


    QFont *font = (QFont *)FindFont(pFont);
    CHAR *pCleanText = AllocCleanText(pText, &Length, singleLine);

    const QFont &oldFont = _painter->font();
    _painter->setPen(QColor(FixColor(State.TextColor)));

    _painter->setFont(*font);
    _painter->drawText(pRect->Left, pRect->Top,
                       pRect->Right - pRect->Left + 1,
                       pRect->Bottom - pRect->Top + 1,
                       Flags,
                       pCleanText);
    _painter->setFont(oldFont);

    MEM_FREE(pCleanText);
}

VOID CCanvas_Qt::SetSize(UINT Width, UINT Height)
{
    if ((_width == Width) && (_height == Height)) return;

    delete _painter;
    delete _image;
    _image = new QImage(Width, Height, QImage::Format_RGB16);
    _image->fill(0xFFFFFF);
    _painter = new QPainter(_image);
    _painter->setRenderHint(QPainter::TextAntialiasing);

    _width = Width;
    _height = Height;

    ClearClipRect();
}

COLOR CCanvas_Qt::GetPixel(INT X, INT Y)
{
    return (COLOR)_image->pixel(QPoint(X, Y));
}

VOID CCanvas_Qt::SetPixel(INT X, INT Y, COLOR Color)
{
     _painter->fillRect(X, Y, 1, 1, QColor(Color));
}

VOID CCanvas_Qt::InvertBlock(INT X1, INT Y1, INT X2, INT Y2)
{
    QImage tempImage(X2 - X1 + 1, Y2 - Y1 + 1, QImage::Format_RGB16);
    QPainter tempPainter(&tempImage);
    tempPainter.drawImage(0, 0, *_image, X1, Y1, X2 - X1 + 1, Y2 - Y1 + 1);
    tempImage.invertPixels();

    _painter->setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    _painter->drawImage(X1, Y1, tempImage);
}

VOID CCanvas_Qt::SetBlock(INT X1, INT Y1, INT X2, INT Y2, COLOR Color)
{
    if ((X1 > X2) || (Y1 > Y2)) return;

    INT height = Y2 - Y1 + 1;
    INT width = X2 - X1 + 1;
    UINT32 c = FixColor(Color);

    _painter->setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    _painter->fillRect(X1, Y1, width, height, c);

/*
    UINT32 *bitsBase = (UINT32 *)_image->scanLine(Y1) + X1;
    UINT32 *bits = bitsBase;

    for (INT j=height; j>0; j--)
    {
        for (INT i=width; i>0; i--)
        {
            *bits = c;
            bits++;
        }

        bits = bitsBase + (_image->bytesPerLine() / 4);
        bitsBase = bits;
    }*/
}

VOID CCanvas_Qt::StretchCopyRect(CfxCanvas *pCanvas, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1, INT DX2, INT DY2)
{
    QImage *srcImage = ((CCanvas_Qt *)pCanvas)->GetImage();
    QPixmap pixMap;

    if (!pixMap.convertFromImage(*srcImage)) {
        return;
    }

    _painter->setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    _painter->drawPixmap(DX1, DY1, DX2 - DX1 + 1, DY2 - DY1 + 1,
                         pixMap,
                         X1, Y1, X2 - X1 + 1, Y2 - Y1 + 1);
}

VOID CCanvas_Qt::CopyRect(CfxCanvas *pCanvas, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1)
{
    StretchCopyRect(pCanvas, X1, Y1, X2, Y2, DX1, DY1, DX1 + (X2 - X1), DY1 + (Y2 - Y1));
}

VOID CCanvas_Qt::StretchCopyRect(FXBITMAPRESOURCE *pBitmap, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1, INT DX2, INT DY2, BOOL Invert, BOOL Transparent)
{
    QImage srcImage(&pBitmap->Pixels[0], pBitmap->Width, pBitmap->Height, pBitmap->RowBytes, QImage::Format_RGB16);
    QPixmap pixMap;

    if (!pixMap.convertFromImage(srcImage)) {
        return;
    }

    _painter->setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    if (Transparent) {
        pixMap.setMask(pixMap.createMaskFromColor(srcImage.pixel(0, 0), Qt::MaskInColor));
    }

    _painter->drawPixmap(DX1, DY1, DX2 - DX1 + 1, DY2 - DY1 + 1,
                         pixMap,
                         X1, Y1, X2 - X1 + 1, Y2 - Y1 + 1);

    if (Invert) {
        InvertBlock(DX1, DY1, DX2, DY2);
    }
}

VOID CCanvas_Qt::CopyRect(FXBITMAPRESOURCE *pBitmap, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1, BOOL Invert)
{
    StretchCopyRect(pBitmap, X1, Y2, X2, Y2, DX1, DY1, DX1 + (X2 - X1), DY1 + (Y2 - Y1), Invert, FALSE);
}

VOID CCanvas_Qt::SystemDrawText(CHAR *pText, FXRECT *pRect, COLOR Color, COLOR BkColor)
{
    SetBlock(pRect->Left, pRect->Top, pRect->Right, pRect->Bottom, BkColor);

    const QFont &oldFont = _painter->font();

    _painter->setPen(QColor(Color));
    _painter->setFont(QFont("Arial", (16 * _scaleFont) / 100));
    _painter->drawText(pRect->Left, pRect->Top,
                       pRect->Right - pRect->Left + 1,
                       pRect->Bottom - pRect->Top + 1,
                       Qt::AlignCenter | Qt::AlignVCenter,
                       pText);
    _painter->setFont(oldFont);

}

VOID CCanvas_Qt::ImportLine(INT X, INT Y, COLOR *pBuffer, INT Size)
{
    COLOR *pSrc = pBuffer;
    UINT16 *pDst = (UINT16 *)_image->scanLine(Y) + X;

    for (INT i=0; i<Size; i++) {
        COLOR color = *pSrc;
        *pDst = (UINT16)(((color & 0xF8) << 8) | ((color & 0xFC00) >> 5) | ((color & 0xF80000) >> 19));
        pSrc++;
        pDst++;
    }
}

VOID CCanvas_Qt::LoadFromFile(CHAR *pFileName, UINT Width, UINT Height)
{
    QImage image;
    image.load(pFileName);

    if ((image.width() == 0) || (image.height() == 0)) {
        return;
    }

    UINT w = image.width();
    UINT h = image.height();
    UINT cw = Width;
    UINT ch = Height;
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
    _painter->drawImage(QRect(0, 0, w, h), image, QRect(0, 0, image.width(), image.height()));
}

FXBITMAPRESOURCE *CCanvas_Qt::CreateBitmapFromFile(CHAR *pFileName, UINT Width, UINT Height)
{
    FXBITMAPRESOURCE *result = NULL;
    CCanvas_Qt canvas(this, _scaleFont);

    canvas.LoadFromFile(pFileName, Width, Height);

    if ((canvas._width == 0) || (canvas._height == 0)) {
        return result;
    }

    int size = canvas.GetImage()->bytesPerLine() * canvas._height;

    result = (FXBITMAPRESOURCE *)MEM_ALLOC(size + sizeof(FXBITMAPRESOURCE));
    if (result == NULL) {
        return result;
    }

    result->Magic = MAGIC_BITMAP;
    result->Width = (UINT16)canvas._width;
    result->Height = (UINT16)canvas._height;
    result->BitDepth = 16;
    result->RowBytes = (UINT16)canvas.GetImage()->bytesPerLine();
    result->Compress = 0;
    result->Size = size;

    memcpy(&result->Pixels[0], canvas.GetImage()->bits(), size);

    return result;
}

//VOID CCanvas_Qt::Line(INT X1, INT Y1, INT X2, INT Y2)
//{
//    _painter->setClipRect(State.ClipRect.Left,
//                          State.ClipRect.Top,
//                          State.ClipRect.Right - State.ClipRect.Left + 1,
//                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

//    QPen pen;
//    pen.setColor(QColor(FixColor(State.LineColor)));
//    pen.setWidth(State.LineWidth);

//    _painter->setPen(pen);
//    _painter->setRenderHint(QPainter::Antialiasing);
//    _painter->drawLine(X1, Y1, X2, Y2);
//}

//VOID CCanvas_Qt::FillTriangle(INT X1, INT Y1, INT X2, INT Y2, INT X3, INT Y3)
//{
//    _painter->setClipRect(State.ClipRect.Left,
//                          State.ClipRect.Top,
//                          State.ClipRect.Right - State.ClipRect.Left + 1,
//                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

//    _painter->setBrush(QColor(FixColor(State.BrushColor)));
//    _painter->setPen(Qt::NoPen);
//    _painter->setRenderHint(QPainter::Antialiasing);

//    QPoint points[3];
//    points[0].setX(X1); points[0].setY(Y1);
//    points[1].setX(X2); points[1].setY(Y2);
//    points[2].setX(X3); points[2].setY(Y3);

//    _painter->drawPolygon(points, 3);
//}

//VOID CCanvas_Qt::Triangle(INT X1, INT Y1, INT X2, INT Y2, INT X3, INT Y3)
//{
//    _painter->setClipRect(State.ClipRect.Left,
//                          State.ClipRect.Top,
//                          State.ClipRect.Right - State.ClipRect.Left + 1,
//                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

//    QPen pen;
//    pen.setColor(QColor(FixColor(State.LineColor)));
//    pen.setWidth(State.LineWidth);

//    QPoint points[3];
//    points[0].setX(X1); points[0].setY(Y1);
//    points[1].setX(X2); points[1].setY(Y2);
//    points[2].setX(X3); points[2].setY(Y3);

//    _painter->setBrush(Qt::NoBrush);
//    _painter->setPen(pen);
//    _painter->setRenderHint(QPainter::Antialiasing);
//    _painter->drawLine(X1, Y1, X2, Y2);
//    _painter->drawLine(X2, Y2, X3, Y3);
//    _painter->drawLine(X1, Y1, X3, Y3);
//}

VOID CCanvas_Qt::FillEllipse(INT Xc, INT Yc, UINT RadiusX, UINT RadiusY)
{
    _painter->setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    _painter->setBrush(QColor(FixColor(State.BrushColor)));
    _painter->setPen(Qt::NoPen);
    _painter->setRenderHint(QPainter::Antialiasing);
    _painter->drawEllipse(QPoint(Xc, Yc), RadiusX, RadiusY);
}

VOID CCanvas_Qt::Ellipse(INT Xc, INT Yc, UINT RadiusX, UINT RadiusY)
{
    _painter->setClipRect(State.ClipRect.Left,
                          State.ClipRect.Top,
                          State.ClipRect.Right - State.ClipRect.Left + 1,
                          State.ClipRect.Bottom - State.ClipRect.Top + 1);

    QPen pen;
    pen.setColor(QColor(FixColor(State.LineColor)));
    pen.setWidth(State.LineWidth);

    _painter->setBrush(Qt::NoBrush);
    _painter->setPen(pen);
    _painter->setRenderHint(QPainter::Antialiasing);
    _painter->drawEllipse(QPoint(Xc, Yc), RadiusX, RadiusY);
}
