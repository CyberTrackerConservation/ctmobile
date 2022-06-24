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

#include "fxMemo.h"
#include "fxApplication.h"
#include "fxUtils.h"

CfxControl *Create_Control_Memo(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Memo(pOwner, pParent);
}

CfxControl_Memo::CfxControl_Memo(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_MEMO);
    InitLockProperties("Caption;Font");
    InitBorder(bsSingle, 0);

    _caption = 0;
    _minHeight = 16;
    _alignment = TEXT_ALIGN_LEFT;
    _rightToLeft = FALSE;

    _onClick.Object = NULL;

    _wordWrap = TRUE;
    _autoHeight = FALSE;
    
    _scrollBar = new CfxControl_ScrollBarV2(pOwner, this);
    _scrollBar->SetComposite(TRUE);

    _scrollBarWidth = 14;

    InitFont(F_FONT_DEFAULT_M);
    _textColor = _borderColor;
}

CfxControl_Memo::~CfxControl_Memo()
{
    MEM_FREE(_caption);
}

VOID CfxControl_Memo::SetOnClick(CfxPersistent *pObject, NotifyMethod OnClick)
{
    _onClick.Object = pObject;
    _onClick.Method = OnClick;
}

VOID CfxControl_Memo::SetCaption(const CHAR *Caption)
{
    MEM_FREE(_caption);
    _caption = (CHAR *) MEM_ALLOC(strlen(Caption) + 1);
    strcpy(_caption, Caption);
    UpdateScrollBar();
}

UINT CfxControl_Memo::GetScrollBarWidth()
{
    return _scrollBar->GetVisible() ? (UINT)_scrollBarWidth : 0;
}

VOID CfxControl_Memo::UpdateScrollBar()
{
    UINT sw = _scrollBarWidth;

    _scrollBar->SetColor(_color);
    _scrollBar->SetBorderColor(_borderColor);

    CfxResource *resource = GetResource();
    if (resource)
    {
        CfxCanvas *canvas = GetCanvas(this);

        UINT w = GetClientWidth() - 4;
        UINT h = GetClientHeight() - 4;

        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        FXRECT rect = {0, 0, w-1, h-1};
        canvas->CalcTextRect(font, _caption, &rect);

        if (_autoHeight && ((_dock == dkTop) || (_dock == dkBottom) || (_dock == dkNone)))
        {
            ResizeHeight(max(16, rect.Bottom + (UINT)_borderWidth * 2 + _borderLineWidth * 2 + 4));
            h = GetClientHeight() - 4;
        }

        _scrollBar->SetVisible((UINT)rect.Bottom > h);
        if (_scrollBar->GetVisible())
        {
            if (IsLive())
            {
                sw = (_scrollBarWidth * GetEngine(this)->GetScaleScroller()) / 100;
            }

            rect.Right = w - (UINT)sw - 1;
            rect.Bottom = h - 1;
            GetCanvas(this)->CalcTextRect(font, _caption, &rect);

            UINT fh = canvas->FontHeight(font);
            _scrollBar->Setup(0, (rect.Bottom + 1) / fh, h / fh, 1);
        }
        else
        {
            _scrollBar->Setup(0, 0, 0, 0);
        }
        
        resource->ReleaseFont(font);
    }
    _scrollBar->SetBounds(_width - _borderWidth - sw, _borderWidth, _scrollBar->GetVisible() ? sw : 0, GetClientHeight());
}

VOID CfxControl_Memo::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);
    UpdateScrollBar();
}

VOID CfxControl_Memo::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _height = max((INT)_minHeight, _height);

    _scrollBar->Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _scrollBar->SetCanScale(FALSE);
    _scrollBarWidth = ComputeScrollBarWidth(_scrollBarWidth, _borderLineWidth, ScaleX, ScaleHitSize);
}

VOID CfxControl_Memo::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
#endif
}

VOID CfxControl_Memo::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Alignment",    dtInt32,    &_alignment, F_ZERO);
    F.DefineValue("AutoHeight",   dtBoolean,  &_autoHeight, F_FALSE);
    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
    F.DefineValue("Caption",      dtPText,    &_caption, F_NULL);
    F.DefineValue("MinHeight",    dtInt32,    &_minHeight, "16");
    F.DefineValue("RightToLeft",  dtBoolean,  &_rightToLeft, F_FALSE);
    F.DefineValue("ScrollBarWidth", dtInt32,  &_scrollBarWidth, "9");

    if (F.IsReader())
    {
        UpdateScrollBar();
    }

    /*_borderColor = RGB(241, 227, 199);
    _textColor = RGB(74, 37, 0);*/

}

VOID CfxControl_Memo::DefinePropertiesUI(CfxFilerUI &F) 
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
    F.DefineValue("Caption",      dtPText,    &_caption,     "MEMO;ROWHEIGHT(64)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Minimum height", dtInt32,  &_minHeight, "MIN(0)");
    F.DefineValue("Right to left", dtBoolean, &_rightToLeft);
    F.DefineValue("Scroll width", dtInt32,    &_scrollBarWidth, "MIN(7);MAX(40)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}


VOID CfxControl_Memo::OnPenClick(INT X, INT Y)
{
    if (_onClick.Object)
    {
        ExecuteEvent(&_onClick, this);
    }
}

VOID CfxControl_Memo::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
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
            FXRECT rect = *pRect;
            rect.Right -= GetScrollBarWidth();
            rect.Top -= (INT)_scrollBar->GetValue() * pCanvas->FontHeight(font);
            InflateRect(&rect, -2, -2);
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, &rect, _alignment | TEXT_ALIGN_WORDWRAP | (_rightToLeft ? TEXT_ALIGN_RIGHT : 0), _caption);
            resource->ReleaseFont(font);
        }
    }
}
