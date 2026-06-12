#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "ctElement.h"

const GUID GUID_CONTROL_ELEMENTCONTAINER = {0x9418a58, 0x7e96, 0x4e83, {0xb4, 0x34, 0x4, 0x45, 0x21, 0xc3, 0x48, 0x6a}};

//
// CctControl_ElementContainer
//
class CctControl_ElementContainer: public CfxControl
{
protected:
    XGUIDLIST _elements;
public:
    CctControl_ElementContainer(CfxPersistent *pOwner, CfxControl *pParent);

    XGUIDLIST *GetElements()    { return &_elements; }

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_ElementContainer(CfxPersistent *pOwner, CfxControl *pParent);

// Utility API
extern XGUIDLIST *GetContainerElements(CfxControl *pControl);
