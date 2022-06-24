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

#include "fxSystem.h"
#include "fxUtils.h"

CfxControl *Create_Control_System(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_System(pOwner, pParent);
}

CfxControl_System::CfxControl_System(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SYSTEM, TRUE);
    InitBorder(bsSingle, 1);
    
    _caption = NULL;
    _minHeight = 0;
    _alignment = TEXT_ALIGN_HCENTER;

    _style = scsTime;

    _textColor = _borderColor;
}

CfxControl_System::~CfxControl_System()
{
    MEM_FREE(_caption);
	_caption = NULL;
}

VOID CfxControl_System::SetCaption(CHAR *pCaption)
{
    MEM_FREE(_caption);
    _caption = NULL;
    _caption = AllocString(pCaption);
}

VOID CfxControl_System::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
#endif
}

VOID CfxControl_System::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Alignment",    dtInt32,    &_alignment, F_TWO);
    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
	F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("MinHeight",    dtInt32,    &_minHeight, F_ZERO);
    F.DefineValue("Style",        dtInt8,     &_style, F_ONE);
}

VOID CfxControl_System::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    CHAR items[256];
    sprintf(items, "LIST(Left=%d Center=%d Right=%d)", TEXT_ALIGN_LEFT, TEXT_ALIGN_HCENTER, TEXT_ALIGN_RIGHT);
    F.DefineValue("Alignment",    dtInt32,    &_alignment,   items);

    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,       "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Minimum height", dtInt32,  &_minHeight, "MIN(0)");
    F.DefineValue("Style",        dtInt8,     &_style, "LIST(Battery Time)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CfxControl_System::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    CfxResource *resource = GetResource(); 

    switch (_style)
    {
    case scsBattery:
        pCanvas->State.LineWidth = max(1, _borderLineWidth);
        pCanvas->DrawBattery(GetHost(this)->GetBatteryLevel(), pRect, _alignment, _color, _textColor, FALSE, _transparent);
        break;

    case scsTime:
        {
            FXFONTRESOURCE *font = resource->GetFont(this, _font);
            if (font)
            {
                FXDATETIME dateTime;
                GetHost(this)->GetDateTime(&dateTime);
                pCanvas->DrawTime(&dateTime, pRect, font, _alignment | TEXT_ALIGN_VCENTER, _color, _textColor, FALSE, _transparent);
                resource->ReleaseFont(font);
            }
        }
        break;
    }
}

VOID CfxControl_System::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _height = max((INT)_minHeight, _height);
}

//*************************************************************************************************
