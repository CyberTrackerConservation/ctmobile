#pragma once

#include "pch.h"
#include "fxButton.h"

const GUID GUID_CONTROL_ELEMENTBARCODE = { 0x7234220a, 0x65bf, 0x4fb6, { 0x96, 0x37, 0xbc, 0xc7, 0x6, 0x84, 0xa8, 0xf4 } };

//
// CctControl_ElementBarcode
//
class CctControl_ElementBarcode: public CfxControl_Button
{
protected:
    GUID _sightingUniqueId;
    XGUID _elementId = 0;
    bool _supported = false;
    bool _required = false;
    bool _waitForTaken = false, _pending = false;
    char *_filterBefore = nullptr;
    char *_filterAfter = nullptr;

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_ElementBarcode(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementBarcode();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPenClick(int X, int Y) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
    
    void OnBarcodeScan(char *pBarcode, bool Success) override;

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
};

extern CfxControl *Create_Control_ElementBarcode(CfxPersistent *pOwner, CfxControl *pParent);
