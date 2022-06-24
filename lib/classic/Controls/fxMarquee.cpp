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

#include "fxApplication.h"
#include "fxMarquee.h"
#include "fxUtils.h"

CfxControl *Create_Control_Marquee(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Marquee(pOwner, pParent);
}

CfxControl_Marquee::CfxControl_Marquee(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_MARQUEE, TRUE);
    InitLockProperties("Caption;Scroll Rate");
    
    _caption = 0;

    InitFont(F_FONT_DEFAULT_M);
    _textColor = _borderColor;

    _timeDatum = 0;
    _rate = 15;
}

CfxControl_Marquee::~CfxControl_Marquee()
{
    MEM_FREE(_caption);
    _caption = NULL;
}

VOID CfxControl_Marquee::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
#endif
}

VOID CfxControl_Marquee::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("ScrollRate",   dtInt32,    &_rate, "15");
}

VOID CfxControl_Marquee::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
    F.DefineValue("Scroll Rate",  dtInt32,    &_rate);
#endif
}

VOID CfxControl_Marquee::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    SetPaintTimer(50);

    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    CfxResource *resource = GetResource(); 

    if (_caption && (strlen(_caption) > 0))
    {
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            UINT w = pRect->Right - pRect->Left + 1;
            UINT h = pRect->Bottom - pRect->Top + 1;
            INT x = (w - pCanvas->TextWidth(font, _caption)) / 2;
            INT y = (h - pCanvas->FontHeight(font)) / 2;

            if (IsLive())
            {
                UINT time = FxGetTickCount();
                if (_timeDatum == 0)
                {
                    _timeDatum = time;
                }

                INT pixelsPerSecond = _rate;
                x = w - (INT)(((time - _timeDatum) * pixelsPerSecond / FxGetTicksPerSecond()) % (w + pCanvas->TextWidth(font, _caption)));
            }

            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawText(font, pRect->Left + x, pRect->Top + y, _caption);
            resource->ReleaseFont(font);
        }
    }
}
