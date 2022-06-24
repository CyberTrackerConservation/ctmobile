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
#include "fxUtils.h"
#include "fxList.h"

CfxControl *Create_Control_List(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_List(pOwner, pParent);
}

//*************************************************************************************************
// CfxControl_List

CfxControl_List::CfxControl_List(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_LIST);
    InitBorder(bsSingle, 0);
    
    _painter = NULL;

    _scrollBar = new CfxControl_ScrollBarV2(pOwner, this);
    _scrollBar->SetComposite(TRUE);

	_autoHeight = FALSE;
    _scaleForHitSize = TRUE;

    _itemHeight = 33;
    _minItemHeight = 16;
    _minItemWidth = 32;

    _columnCount = 1;
    _scrollBarWidth = 14;
    _showLines = TRUE;
}

CfxControl_List::~CfxControl_List()
{
    _painter = NULL;
}

UINT CfxControl_List::GetScrollBarWidth()
{
    return _scrollBar->GetVisible() ? (_scrollBar->GetWidth() - (UINT)_borderLineWidth) : 0;
}

UINT CfxControl_List::GetVisibleItemCount()
{
    UINT lineWidth = (UINT)_borderLineWidth * (_showLines ? 1 : 0);
    UINT t = (GetClientHeight() - lineWidth + (UINT)_itemHeight - 1) / ((UINT)_itemHeight + lineWidth);
    return t * GetColumnCount();
}

UINT CfxControl_List::GetColumnCount()
{
    INT w = GetClientWidth() - GetScrollBarWidth();
    return max(1, min(w / (UINT)_minItemWidth, (UINT)_columnCount));
}

VOID CfxControl_List::UpdateScrollBar()
{
    if (_autoHeight && ((_dock == dkTop) || (_dock == dkBottom) || (_dock == dkNone)))
	{
		UINT visibleRows = GetItemCount() / _columnCount;
		UINT lineWidth = (UINT)_borderLineWidth * (_showLines ? 1 : 0);
		UINT maxHeight = _parent == NULL ? _height : _parent->GetHeight();
        ResizeHeight(min(maxHeight, (_itemHeight + lineWidth) * visibleRows + lineWidth));
	}

    UINT lineWidth = (UINT)_borderLineWidth * (_showLines ? 1 : 0);
    UINT t = _height == 0 ? 0 : (GetClientHeight() - lineWidth) / ((UINT)_itemHeight + lineWidth);
    t *= GetColumnCount();

    UINT sw = _scrollBarWidth;
    if (IsLive())
    {
        sw = (_scrollBarWidth * GetEngine(this)->GetScaleScroller()) / 100;
    }

	_scrollBar->SetBounds(_width - _borderWidth - sw, _borderWidth, sw, GetClientHeight());
    _scrollBar->Setup(0, GetItemCount(), t, GetColumnCount());
    _scrollBar->SetVisible(GetItemCount() > t);
    _scrollBar->SetColor(_color);
    _scrollBar->SetBorderColor(_borderColor == _color ? _textColor : _borderColor);
    _scrollBar->SetBorderLineWidth((UINT)_borderLineWidth);
}

BOOL CfxControl_List::HitTest(INT X, INT Y, INT *pIndex, INT *pOffsetX, INT *pOffsetY)
{
    UINT lineWidth = (UINT)_borderLineWidth * (_showLines ? 1 : 0);
    UINT columnCount = GetColumnCount();
    UINT columnWidth = (GetClientWidth() - GetScrollBarWidth()) / columnCount;
    UINT visibleRows = GetVisibleItemCount() / columnCount;

    INT row = (Y - (UINT)_borderWidth) / ((UINT)_itemHeight + lineWidth);
    INT col = (X - (UINT)_borderWidth) / (UINT)columnWidth;

    *pIndex = row * columnCount + _scrollBar->GetValue();
    *pIndex = ((*pIndex + (columnCount - 1)) / columnCount) * columnCount + col;

    *pOffsetX = X - (UINT)_borderWidth - col * (UINT)columnWidth;
    *pOffsetY = Y - (UINT)_borderWidth - row * ((UINT)_itemHeight + lineWidth);

    return ((*pIndex >= 0) && (*pIndex < (INT)GetItemCount()));
}

VOID CfxControl_List::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

	UpdateScrollBar();
}

VOID CfxControl_List::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    if (!_scaleForHitSize) 
    {
        ScaleHitSize = 0;
    }

    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _scrollBar->Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _scrollBar->SetCanScale(FALSE);
    _scrollBarWidth = ComputeScrollBarWidth(_scrollBarWidth, _borderLineWidth, ScaleX, ScaleHitSize);

    _minItemHeight = max(ScaleHitSize, _minItemHeight); 
    _itemHeight = (_itemHeight * ScaleY) / 100;
    _itemHeight = max(_itemHeight, _minItemHeight);

    if (ScaleX > 100)
    {
        _minItemWidth = (_minItemWidth * ScaleX) / 100;
    }

    if (ScaleY > 100)
    {
        _minItemHeight = (_minItemHeight * ScaleY) / 100;
    }

    _minItemWidth = max(ScaleHitSize, _minItemWidth);

    UpdateScrollBar();
}

VOID CfxControl_List::ResetState()
{
    CfxControl_Panel::ResetState();
    _scrollBar->ResetState();
}

VOID CfxControl_List::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
    if (_painter)
    {
        _painter->DefineResources(F);
    }
#endif
}

VOID CfxControl_List::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (_painter)
    {
        _painter->DefineProperties(F);
    }

    F.DefineValue("AutoHeight",     dtBoolean, &_autoHeight, F_FALSE);
    F.DefineValue("ScaleForHitSize", dtBoolean,&_scaleForHitSize, F_TRUE);

    F.DefineValue("BorderColor",    dtColor,   &_borderColor, F_COLOR_BLACK);
    F.DefineValue("Color",          dtColor,   &_color, F_COLOR_WHITE);
    F.DefineValue("TextColor",      dtColor,   &_textColor, F_COLOR_BLACK);

    F.DefineValue("ItemHeight",     dtInt32,   &_itemHeight, "33");
    F.DefineValue("MinItemHeight",  dtInt32,   &_minItemHeight, "16");
    F.DefineValue("MinItemWidth",   dtInt32,   &_minItemWidth, "32");
    F.DefineValue("ColumnCount",    dtInt32,   &_columnCount, F_ONE);
    F.DefineValue("ScrollBarWidth", dtInt32,   &_scrollBarWidth, "9");
    F.DefineValue("ShowLines",      dtBoolean, &_showLines, F_TRUE);

    // Force a Min Item Height of 30: remove later
    if (_minItemHeight == 32)
    {
        _minItemHeight = 30;
    }

    // Force an Item Height of 34: remove later
    if (_itemHeight == 36)
    {
        _itemHeight = 34;
    }

    if (F.IsReader())
    {
        _scrollBar->SetValue(0);
    }
}

VOID CfxControl_List::DefineState(CfxFiler &F)
{
    _scrollBar->DefineState(F);
}

VOID CfxControl_List::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Auto height",  dtBoolean,   &_autoHeight);
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Columns",      dtInt32,     &_columnCount, "MIN(1);MAX(8)");
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Item height",  dtInt32,     &_itemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Minimum item height", dtInt32, &_minItemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Minimum item width",  dtInt32, &_minItemWidth,   "MIN(32);MAX(640)");
    F.DefineValue("Scale for touch", dtBoolean,&_scaleForHitSize);
    F.DefineValue("Scroll width", dtInt32,     &_scrollBarWidth,    "MIN(7);MAX(40)");
    F.DefineValue("Show lines",   dtBoolean,   &_showLines);
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}

VOID CfxControl_List::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    pCanvas->State.TextColor = _textColor;

    if (GetItemCount() == 0)
    {
        CfxControl_Panel::OnPaint(pCanvas, pRect);
        return;
    }
    
    if (_painter)
    {
        _painter->BeforePaint(pCanvas, pRect);
    }

    CfxResource *resource = GetResource(); 
    FXFONTRESOURCE *font = resource->GetFont(this, _font);

    // Calculations to speed up drawing
    UINT lineWidth   = (UINT)_borderLineWidth * (_showLines ? 1 : 0);
    UINT columnCount = GetColumnCount();
    UINT columnWidth = (GetClientWidth() - GetScrollBarWidth()) / columnCount;
    UINT visibleRows = GetVisibleItemCount() / columnCount;
    INT index = ((_scrollBar->GetValue() + (columnCount - 1)) / columnCount) * columnCount;
    INT startIndex = index;
    INT count = GetItemCount();
    
    UINT xstart = pRect->Left;
    UINT ystart = pRect->Top;
    
    for (UINT j=0; j<visibleRows; j++)
    {
        xstart = pRect->Left;

        for (INT i=0; i<(INT)columnCount; i++)
        {
            UINT colWidth = columnWidth;

            if (i == (INT)columnCount - 1)
            {
                colWidth = GetClientWidth() - GetScrollBarWidth() - xstart + pRect->Left;
            }

            FXRECT rectLines = {xstart, ystart, xstart + colWidth - 1, ystart + (UINT)_itemHeight + lineWidth * 2 - 1};
            if (index < count)
            {
                pCanvas->State.BrushColor = _color;
                pCanvas->State.LineColor = _borderColor;
                pCanvas->State.LineWidth = lineWidth;
                _transparent ? pCanvas->FrameRect(&rectLines) : pCanvas->Rectangle(&rectLines);
                
                pCanvas->State.LineWidth = 1;
                InflateRect(&rectLines, -(INT)lineWidth, -(INT)lineWidth);

                PaintItem(pCanvas, &rectLines, font, index);
            }
            else if (!_transparent)
            {
                rectLines.Top += lineWidth;
                if (i > 0)
                {
                    rectLines.Left += lineWidth;//showLines;
                }
                if (i == (INT)columnCount - 1)
                {
                    rectLines.Right = pRect->Right;
                }
                pCanvas->State.BrushColor = _color;
                pCanvas->FillRect(&rectLines);
            }

            xstart += columnWidth - lineWidth;
            index++;
        }
        ystart += (UINT)_itemHeight + lineWidth;
    }

    if (!_transparent)
    {
        FXRECT rectBottom = *pRect;
        rectBottom.Top += visibleRows * ((UINT)_itemHeight + lineWidth) + lineWidth;
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(&rectBottom);
    }

    resource->ReleaseFont(font);

    if (_painter)
    {
        _painter->AfterPaint(pCanvas, pRect);
    }
}

