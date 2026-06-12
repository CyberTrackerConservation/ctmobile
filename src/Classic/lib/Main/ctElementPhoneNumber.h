#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "ctElement.h"

const GUID GUID_CONTROL_ELEMENTPHONENUMBER = {0x46d640cc, 0xff89, 0x4374, {0x9e, 0x21, 0xed, 0x83, 0xdc, 0xf7, 0xc7, 0x93}};

//
// CctControl_ElementCamera
//
class CctControl_ElementPhoneNumber: public CfxControl_Panel
{
protected:
    XGUID _elementId = 0;
    bool _required = false;
    bool _waitForTaken = false, _pending = false;
    char *_defaultCaption = nullptr;

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_ElementPhoneNumber(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementPhoneNumber();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPenClick(int X, int Y) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
    void OnPhoneNumberTaken(char *pPhoneNumber, bool Success) override;
};

extern CfxControl *Create_Control_ElementPhoneNumber(CfxPersistent *pOwner, CfxControl *pParent);
