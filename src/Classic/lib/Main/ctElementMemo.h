#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "fxMemo.h"
#include "ctElement.h"


//
// CctControl_ElementMemo
//
const GUID GUID_CONTROL_ELEMENTMEMO = {0xc921c8a, 0xafb7, 0x47fe, {0x90, 0xbd, 0x97, 0xb6, 0x84, 0x59, 0x47, 0x8}};

class CctControl_ElementMemo: public CfxControl_Memo
{
protected:
    XGUIDLIST _elements;
    ElementAttribute _attribute = ElementAttribute::Name;

    void UpdateText();
    XGUIDLIST *GetElements();
public:
    CctControl_ElementMemo(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_ElementMemo(CfxPersistent *pOwner, CfxControl *pParent);

//
// CctControl_ElementPanel
//
const GUID GUID_CONTROL_ELEMENTPANEL = {0x465569d0, 0x3a00, 0x4ad5, {0xb4, 0x8d, 0x6b, 0xb7, 0x88, 0xff, 0x3f, 0xa1}};

class CctControl_ElementPanel: public CfxControl_Panel
{
protected:
    XGUIDLIST _elements;
    ElementAttribute _attribute = ElementAttribute::Name;

    void UpdateText();
    XGUIDLIST *GetElements();
public:
    CctControl_ElementPanel(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_ElementPanel(CfxPersistent *pOwner, CfxControl *pParent);

