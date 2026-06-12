#pragma once
#include "pch.h"
#include "fxControl.h"
#include "fxPanel.h"
#include "fxScrollBar.h"

const GUID GUID_CONTROL_SCROLLBOX = {0x4753aa0b, 0x2072, 0x4b03, {0x88, 0x1d, 0x0c, 0xd7, 0x67, 0x53, 0x22, 0x85}};

//
// CfxControl_ScrollBox
//
class CfxControl_ScrollBox: public CfxControl_Panel
{
protected:
    CfxControl_ScrollBarV2 *_scrollBar = nullptr;
    int _scrollBarWidth = 0;
    CfxControl *_container = nullptr;
    bool _once = false;

    int GetScrollBarWidth();
    void UpdateScrollBar();
    void OnScrollChange(CfxControl *pSender, void *pParams);
public:
    CfxControl_ScrollBox(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_ScrollBox();

    CfxControl *GetChildParent() override { return _container; }

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
    void SetBounds(int Left, int Top, int Width, int Height) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

extern CfxControl *Create_Control_ScrollBox(CfxPersistent *pOwner, CfxControl *pParent);

