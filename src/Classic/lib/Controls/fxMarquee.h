#pragma once
#include "pch.h"
#include "fxControl.h"

const GUID GUID_CONTROL_MARQUEE = {0xfc003d20, 0x75d1, 0x4b84, {0x88, 0x6c, 0xc4, 0xc1, 0x32, 0x7c, 0x70, 0xee}};

//
// CfxControl_Marquee
//
class CfxControl_Marquee: public CfxControl
{
protected:
    char *_caption = nullptr;
    COLOR _textColor = 0;
    int _timeDatum = 0;
    int _rate = 0;
public:
    CfxControl_Marquee(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Marquee();

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

extern CfxControl *Create_Control_Marquee(CfxPersistent *pOwner, CfxControl *pParent);

