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
        
    virtual void DefineResources(CfxFilerResource &F) { }
    virtual void DefineProperties(CfxFiler &F)        { }

    virtual int GetItemCount()       { return 0; }
    virtual void PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index) {}
    virtual void BeforePaint(CfxCanvas *pCanvas, FXRECT *pRect) { }
    virtual void AfterPaint(CfxCanvas *pCanvas, FXRECT *pRect)  { }
};

//
// CfxControl_List
//
class CfxControl_List: public CfxControl_Panel
{
protected:
    CfxControl_ScrollBarV2 *_scrollBar = nullptr;
    CfxListPainter *_painter = nullptr;

    int _itemHeight = 0, _minItemHeight = 0, _minItemWidth = 0, _columnCount = 0;
    int _scrollBarWidth = 0;
    bool _showLines = false;
	bool _autoHeight = false;
    bool _scaleForHitSize = false;

    int _penStartScrollValue = 0;
    int _lastDragY = 0;
    int _lastDragTime = 0;
    int _velocity = 0;
    QTimer *_momentumTimer = nullptr;

    int GetVisibleItemCount();
    virtual int GetColumnCount();
    int GetScrollBarWidth();
    void UpdateScrollBar();
    void MomentumTick();
    bool HitTest(int X, int Y, int *pIndex, int *pOffsetX, int *pOffsetY);
public:
    CfxControl_List(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_List();

    CfxListPainter *GetPainter()        { return _painter;    }
    void SetPainter(CfxListPainter *pPainter) { _painter = pPainter; }

    bool GetShowLines()                 { return _showLines;  }
    void SetShowLines(const bool Value) { _showLines = Value; }

    int GetItemHeight()              { return _itemHeight;  }
    void SetItemHeight(int Value)    { _itemHeight = Value; }

    void ResetState() override;

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void SetBounds(int Left, int Top, int Width, int Height) override;
    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    void OnPenDown(int X, int Y) override;
    void OnPenUp(int X, int Y) override;
    void OnPenDrag(int StartX, int StartY, int X, int Y) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    virtual int GetItemCount() 
    { 
        return _painter ? _painter->GetItemCount() : 0; 
    }

    virtual void PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index) 
    {
        if (_painter)
        {
            _painter->PaintItem(pCanvas, pRect, pFont, Index);
        }
    }
};

extern CfxControl *Create_Control_List(CfxPersistent *pOwner, CfxControl *pParent);

