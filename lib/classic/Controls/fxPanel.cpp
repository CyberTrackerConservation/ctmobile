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

#include "fxPanel.h"
#include "fxUtils.h"

CfxControl *Create_Control_Panel(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Panel(pOwner, pParent);
}

CfxControl_Panel::CfxControl_Panel(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_PANEL, TRUE);
    InitBorder(bsSingle, 1);
    
    _caption = NULL;
    _autoHeight = FALSE;
    _minHeight = 0;
    _alignment = TEXT_ALIGN_HCENTER;

    _useScreenName = FALSE;

    _onClick.Object = NULL;
    _onPaint.Object = NULL;
    _onResize.Object = NULL;

    _textColor = 0;
}

CfxControl_Panel::~CfxControl_Panel()
{
    MEM_FREE(_caption);
	_caption = NULL;
}

VOID CfxControl_Panel::UpdateHeight()
{
    if (!_autoHeight) return;
    if ((_dock != dkTop) && (_dock != dkBottom) && (_dock != dkNone)) return;
    if (!_useScreenName && ((_caption == NULL) || (_caption[0] == '\0'))) return;

    CfxResource *resource = GetResource();
    if (!resource) return;
    
    ResizeHeight(max(_minHeight, GetDefaultFontHeight() + (UINT)_borderWidth * 2 + _borderLineWidth * 2 + 4));
}

VOID CfxControl_Panel::SetCaption(const CHAR *pCaption)
{
    MEM_FREE(_caption);
    _caption = NULL;
    _caption = AllocString(pCaption);
    UpdateHeight();
}

VOID CfxControl_Panel::SetTextColor(COLOR Color)
{
    _textColor = Color;
}

VOID CfxControl_Panel::SetOnClick(CfxPersistent *pObject, NotifyMethod OnClick)
{
    _onClick.Object = pObject;
    _onClick.Method = OnClick;
}

VOID CfxControl_Panel::SetOnPaint(CfxPersistent *pObject, NotifyMethod OnPaint)
{
    _onPaint.Object = pObject;
    _onPaint.Method = OnPaint;
}

VOID CfxControl_Panel::SetOnResize(CfxPersistent *pObject, NotifyMethod OnResize)
{
    _onResize.Object = pObject;
    _onResize.Method = OnResize;
}

VOID CfxControl_Panel::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
#endif
}

VOID CfxControl_Panel::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    /*if ((_dock == 1) && (_height == 32)) {
        _color = RGB(241, 227, 199);
        _textColor = RGB(74, 37, 0);
    }

    if ((_borderStyle > 0) && (_borderColor == 0)) {
        _borderColor = RGB(241, 227, 199);
    }*/

    F.DefineValue("Alignment",    dtInt32,    &_alignment, F_TWO);
    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
	F.DefineValue("Caption",      dtPText,    &_caption, F_NULL);
    F.DefineValue("MinHeight",    dtInt32,    &_minHeight, F_ZERO);
    F.DefineValue("UseScreenName", dtBoolean, &_useScreenName, F_FALSE);
    F.DefineValue("AutoHeight",    dtBoolean, &_autoHeight, F_FALSE);

    /*#ifdef CLIENT_DLL
    if (F.IsReader())
    {
        if (_stricmp(_font, "MS Sans Serif,12,B") == 0) 
        {
            strcpy(_font, "Arial,12,B");
        }
    }
    #endif*/

    if (F.IsReader())
    {
        UpdateHeight();
    }


}

VOID CfxControl_Panel::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    CHAR items[256];
    sprintf(items, "LIST(Left=%d Center=%d Right=%d)", TEXT_ALIGN_LEFT, TEXT_ALIGN_HCENTER, TEXT_ALIGN_RIGHT);
    F.DefineValue("Alignment",    dtInt32,    &_alignment,   items);
    F.DefineValue("Auto height",  dtBoolean,  &_autoHeight);

    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,       "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Minimum height", dtInt32,  &_minHeight, "MIN(0)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Use screen name", dtBoolean, &_useScreenName);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CfxControl_Panel::OnPenClick(INT X, INT Y)
{
    if (_onClick.Object)
    {
        ExecuteEvent(&_onClick, this);
    }
}

VOID CfxControl_Panel::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (_onPaint.Object)
    {
        FXPAINTDATA paintData = {pCanvas, pRect, FALSE};
        ExecuteEvent(&_onPaint, this, &paintData);
        if (!paintData.DefaultPaint) return;
    }

    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    CfxResource *resource = GetResource(); 

    CHAR *caption = GetVisibleCaption();
    
    if (caption && (strlen(caption) > 0))
    {
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, pRect, _alignment | TEXT_ALIGN_VCENTER, caption);
            resource->ReleaseFont(font);
        }
    }
}

VOID CfxControl_Panel::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    if (_height == 1)
    {
        _height = max(1, ScaleBorder / 100);
        _color = (_borderStyle == bsNone) ? _color : _borderColor;
    }
    else if (_width == 1)
    {
        _width = max(1, ScaleBorder / 100);
        _color = (_borderStyle == bsNone) ? _color : _borderColor;
    }
    else 
    {
        CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
        _height = max((INT)_minHeight, _height);
    }

    UpdateHeight();
}

VOID CfxControl_Panel::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    if (_onResize.Object)
    {
        ExecuteEvent(&_onResize, this);
    }
}

//*************************************************************************************************
