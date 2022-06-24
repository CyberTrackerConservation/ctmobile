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
#include "fxControl.h"
#include "fxPanel.h"
#include "fxScrollBar.h"

const GUID GUID_CONTROL_LIST = {0xe2c85632, 0x45ae, 0x4b5f, {0x98, 0x18, 0x34, 0x36, 0x65, 0x7b, 0x6a, 0x3f}};

//
// CfxListPainter
//
class CfxListPainter: public CfxPersistent
{
protected:
    CfxControl *_parent;
public:
    CfxListPainter(CfxPersistent *pOwner, CfxControl *pParent): CfxPersistent(pOwner) { _parent = pParent; }
    virtual ~CfxListPainter()           { }
        
    virtual VOID DefineResources(CfxFilerResource &F) { }
    virtual VOID DefineProperties(CfxFiler &F)        { }

    virtual UINT GetItemCount()       { return 0; }
    virtual VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index) {}
    virtual VOID BeforePaint(CfxCanvas *pCanvas, FXRECT *pRect) { }
    virtual VOID AfterPaint(CfxCanvas *pCanvas, FXRECT *pRect)  { }
};

//
// CfxControl_List
//
class CfxControl_List: public CfxControl_Panel
{
protected:
    CfxControl_ScrollBarV2 *_scrollBar;
    CfxListPainter *_painter;

    UINT _itemHeight, _minItemHeight, _minItemWidth, _columnCount;
    UINT _scrollBarWidth;
    BOOL _showLines;
	BOOL _autoHeight;
    BOOL _scaleForHitSize;

    UINT GetVisibleItemCount();
    virtual UINT GetColumnCount();
    UINT GetScrollBarWidth();
    VOID UpdateScrollBar();
    BOOL HitTest(INT X, INT Y, INT *pIndex, INT *pOffsetX, INT *pOffsetY);
public:
    CfxControl_List(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_List();

    CfxListPainter *GetPainter()        { return _painter;    }
    VOID SetPainter(CfxListPainter *pPainter) { _painter = pPainter; }

    BOOL GetShowLines()                 { return _showLines;  }
    VOID SetShowLines(const BOOL Value) { _showLines = Value; }

    UINT GetItemHeight()              { return _itemHeight;  }
    VOID SetItemHeight(UINT Value)    { _itemHeight = Value; }

    VOID ResetState();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    virtual UINT GetItemCount() 
    { 
        return _painter ? _painter->GetItemCount() : 0; 
    }

    virtual VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index) 
    {
        if (_painter)
        {
            _painter->PaintItem(pCanvas, pRect, pFont, Index);
        }
    }
};

extern CfxControl *Create_Control_List(CfxPersistent *pOwner, CfxControl *pParent);

