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

#include "fxScrollBox.h"

#include "fxApplication.h"
#include "fxUtils.h"

CfxControl *Create_Control_ScrollBox(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_ScrollBox(pOwner, pParent);
}

//*************************************************************************************************
// CfxControl_ScrollBox

CfxControl_ScrollBox::CfxControl_ScrollBox(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SCROLLBOX, TRUE);
    InitBorder(_borderStyle, 0);

    _scrollBarWidth = 14;

    _container = new CfxControl_Panel(pOwner, this);
    _container->SetComposite(TRUE);
    _container->SetBorderStyle(bsNone);
    _container->SetBorderWidth(0);

    _scrollBar = new CfxControl_ScrollBarV2(_owner, this);
    _scrollBar->SetComposite(TRUE);
    _scrollBar->SetOnChange(this, (NotifyMethod)&CfxControl_ScrollBox::OnScrollChange);

    _once = FALSE;
}

CfxControl_ScrollBox::~CfxControl_ScrollBox()
{
}

UINT CfxControl_ScrollBox::GetScrollBarWidth()
{
    return _scrollBar->GetVisible() ? _scrollBarWidth : 0;
}

VOID CfxControl_ScrollBox::OnScrollChange(CfxControl *pSender, VOID *pParams)
{
    _container->SetBounds(_borderWidth, _borderWidth - _scrollBar->GetValue(), _scrollBar->GetLeft(), GetClientHeight() + _scrollBar->GetValue());
}

VOID CfxControl_ScrollBox::UpdateScrollBar()
{
    INT maxY = 0;
    INT minY = 0;
    INT value = _scrollBar->GetValue();

    _scrollBar->SetValue(0);

    for (UINT i=0; i<_container->GetControlCount(); i++)
    {
        CfxControl *control = _container->GetControl(i);
        minY = min(minY, control->GetTop());
        maxY = max(maxY, (INT)(control->GetTop() + control->GetHeight()));
    }

    _scrollBar->SetVisible(maxY > (INT)GetClientHeight());

    UINT sw = _scrollBar->GetVisible() ? _scrollBarWidth : 0;
    if (IsLive())
    {
        sw = (sw * GetEngine(this)->GetScaleScroller()) / 100;
    }

	_scrollBar->SetBounds(_width - _borderWidth - sw, _borderWidth, sw, GetClientHeight());
    _scrollBar->Setup(0, maxY, _container->GetClientHeight(), 1);
    _scrollBar->SetColor(_color);
    _scrollBar->SetBorderColor(_borderColor);

    _scrollBar->SetBounds(_width - _borderWidth - sw, _borderWidth, sw, GetClientHeight());
    _container->SetBounds(_borderWidth, _borderWidth - _scrollBar->GetValue(), _scrollBar->GetLeft(), GetClientHeight() + _scrollBar->GetValue());

    _scrollBar->SetValue(value);
}

VOID CfxControl_ScrollBox::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _scrollBar->Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _scrollBar->SetCanScale(FALSE);
    _scrollBarWidth = ComputeScrollBarWidth(_scrollBarWidth, _borderLineWidth, ScaleX, ScaleHitSize);
}

VOID CfxControl_ScrollBox::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl_Panel::SetBounds(Left, Top, Width, Height);
    
    _scrollBar->SetBounds(_width - _borderWidth - GetScrollBarWidth(), _borderWidth, GetScrollBarWidth(), GetClientHeight());
    _container->SetBounds(_borderWidth, _borderWidth - _scrollBar->GetValue(), _scrollBar->GetLeft(), GetClientHeight() + _scrollBar->GetValue());
}

VOID CfxControl_ScrollBox::DefineProperties(CfxFiler &F)
{
    if (F.DefineBegin("Container"))
    {
        _container->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Scrollbar"))
    {
        _scrollBar->DefineProperties(F);
        F.DefineEnd();
    }

    F.DefineValue("Version",         dtInt32,   &_version, F_ZERO);
    F.DefineValue("LockProperties",  dtPText,   &_lockProperties, _lockPropertiesDefault);
    F.DefineValue("Id",              dtInt32,   &_id);
    F.DefineValue("BorderWidth",     dtInt32,   &_borderWidth, F_ZERO);
    F.DefineValue("BorderLineWidth", dtInt32,   &_borderLineWidth, F_ONE);
    F.DefineValue("BorderStyle",     dtInt8,    &_borderStyle, F_ONE);
    F.DefineValue("BorderColor",     dtColor,   &_borderColor, F_COLOR_BLACK);
    F.DefineValue("Color",           dtColor,   &_color, F_COLOR_WHITE);
    F.DefineValue("Composite",       dtBoolean, &_composite, F_FALSE);
    F.DefineValue("Visible",         dtBoolean, &_visible, F_TRUE);
    F.DefineValue("Transparent",     dtBoolean, &_transparent, F_TRUE);
    F.DefineValue("Align",           dtByte,    &_dock);
    F.DefineValue("Left",            dtInt32,   &_left);
    F.DefineValue("Top",             dtInt32,   &_top);
    F.DefineValue("Width",           dtInt32,   &_width);
    F.DefineValue("Height",          dtInt32,   &_height);

    F.DefineValue("ScrollBarWidth", dtInt32,   &_scrollBarWidth, "9");

	if (F.IsReader())
	{
        _once = FALSE;
	}
}

VOID CfxControl_ScrollBox::DefineState(CfxFiler &F)
{
    _container->DefineState(F);
    _scrollBar->DefineState(F);
}

VOID CfxControl_ScrollBox::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Scroll width", dtInt32,     &_scrollBarWidth);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}

VOID CfxControl_ScrollBox::DefineResources(CfxFilerResource &F)
{
    _scrollBar->DefineResources(F);
    _container->DefineResources(F);
}

VOID CfxControl_ScrollBox::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_once)
    { 
        _once = TRUE;
        UpdateScrollBar();
        GetEngine(this)->AlignControls(this);
    }

    CfxControl_Panel::OnPaint(pCanvas, pRect);
}