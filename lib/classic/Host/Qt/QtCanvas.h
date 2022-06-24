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
#include "fxPlatform.h"
#include "fxCanvas.h"

#include <QPainter>
#include <QImage>
#include <QFont>
#include <QFontMetrics>

// Canvas object
class CCanvas_Qt: public CfxCanvas
{
private:
    QPainter *_painter;
    QImage *_image;

    QPainter *_textPainter;
    QImage *_textImage;

    VOID InvertBlock(INT X1, INT Y1, INT X2, INT Y2);
    VOID LoadFromFile(CHAR *pFileName, UINT Width, UINT Height);

    VOID *MakeFont(CHAR *pName, UINT16 Height, UINT16 Style);
    VOID FreeFont(VOID *Handle);

public:
    CCanvas_Qt(CfxPersistent *pOwner, UINT ScaleFont);
    ~CCanvas_Qt();

    QImage *GetImage() { return _image; }

    UINT TextWidth(FXFONTRESOURCE *pFont, CHAR *pText, UINT Length=0);
    UINT TextHeight(FXFONTRESOURCE *pFont, CHAR *pText, UINT Length=0, INT *pOffsetY=NULL);
    VOID CalcTextRect(FXFONTRESOURCE *pFont, CHAR *pText, FXRECT *pRect);
    VOID DrawText(FXFONTRESOURCE *pFont, INT X, INT Y, CHAR *pText, UINT Length=0);
    VOID DrawTextRect(FXFONTRESOURCE *pFont, FXRECT *pRect, UINT AlignFlags, CHAR *pText, UINT Length = 0);

    VOID SetSize(UINT Width, UINT Height);
    COLOR GetPixel(INT X, INT Y);
    VOID SetPixel(INT X, INT Y, COLOR Color);
    VOID SetBlock(INT X1, INT Y1, INT X2, INT Y2, COLOR Color);
    VOID ImportLine(INT X, INT Y, COLOR *pBuffer, INT Size);
    VOID CopyRect(CfxCanvas *pCanvas, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1);
    VOID CopyRect(FXBITMAPRESOURCE *pBitmap, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1, BOOL Invert);

    VOID StretchCopyRect(CfxCanvas *pCanvas, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1, INT DX2, INT DY2);
    VOID StretchCopyRect(FXBITMAPRESOURCE *pBitmap, INT X1, INT Y1, INT X2, INT Y2, INT DX1, INT DY1, INT DX2, INT DY2, BOOL Invert, BOOL Transparent);
    VOID SystemDrawText(CHAR *pText, FXRECT *pRect, COLOR Color, COLOR BkColor);

    // Line
    //VOID Line(INT X1, INT Y1, INT X2, INT Y2);

    // Triangle
    //VOID FillTriangle(INT X1, INT Y1, INT X2, INT Y2, INT X3, INT Y3);
    //VOID Triangle(INT X1, INT Y1, INT X2, INT Y2, INT X3, INT Y3);

    // Ellipse
    VOID FillEllipse(INT Xc, INT Yc, UINT RadiusX, UINT RadiusY);
    VOID Ellipse(INT Xc, INT Yc, UINT RadiusX, UINT RadiusY);

    FXBITMAPRESOURCE *CreateBitmapFromFile(CHAR *pFileName, UINT Width, UINT Height);
};

