#pragma once

#include "pch.h"
#include "fxControl.h"

const GUID GUID_CONTROL_ELEMENTFILTER = {0x71fa9209, 0x9b77, 0x4a40, {0xb4, 0xc1, 0x84, 0x2e, 0xee, 0x84, 0x1d, 0xce}};

//
// CctControl_ElementFilter
//
class CctControl_ElementFilter: public CfxControl
{
protected:
    char *_caption = nullptr;
    COLOR _textColor = 0;
    int _minHeight = 0, _minWidth = 0;
    XGUID _filterScreenId = 0;
    bool _filtered = false;
    bool _penDown = false, _penOver = false;
    void SetCaption(const char *pCaption);
    void ExecuteFilter();
public:
    CctControl_ElementFilter(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementFilter();

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

	void OnPenDown(int X, int Y) override;
    void OnPenUp(int X, int Y) override;
    void OnPenMove(int X, int Y) override;

    void OnPenClick(int X, int Y) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

extern CfxControl *Create_Control_ElementFilter(CfxPersistent *pOwner, CfxControl *pParent);

