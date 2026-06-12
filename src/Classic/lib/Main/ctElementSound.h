#pragma once
#include "pch.h"
#include "fxPanel.h"
#include "ctElement.h"
#include "fxSound.h"

//*************************************************************************************************
//
// CctControl_ElementSound
//

const GUID GUID_CONTROL_ELEMENTSOUND = {0x2687cee1, 0x589, 0x4ee7, {0xb8, 0x16, 0xe0, 0xe4, 0x69, 0x92, 0xb5, 0xd7}};

class CctControl_ElementSound: public CfxControl_SoundBase
{
protected:
    XGUIDLIST _elements;

    XGUIDLIST *GetElements();
    FXSOUNDRESOURCE *GetSound() override;
    void ReleaseSound(FXSOUNDRESOURCE *pSound) override;
public:
    CctControl_ElementSound(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_ElementSound(CfxPersistent *pOwner, CfxControl *pParent);

