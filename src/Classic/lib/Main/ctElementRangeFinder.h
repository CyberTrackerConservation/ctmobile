#pragma once

#include "pch.h"
#include "fxRangeFinder.h"
#include "fxButton.h"
#include "ctSession.h"

// Retained as a no-op stub for resource file backward compatibility.

const GUID GUID_CONTROL_ELEMENTRANGEFINDER = {0x8f3f0fc9, 0x70cf, 0x4b7d, { 0xaa, 0xd2, 0x48, 0x66, 0x6a, 0x2, 0xcd, 0x6f}};

class CctControl_ElementRangeFinder: public CfxControl_RangeFinder
{
protected:
    bool _required = false, _autoNext = false;
    bool _clearOnEnter = false;
    bool _snapDateTime = false, _snapGPS = false;
    FXDATETIME _snapDateTimeValue;
    FXGPS _snapGPSValue;
    char *_rangeFinderId = nullptr;
    XGUID _elementRangeFinderId = 0;
    XGUID _elementRangeId = 0, _elementRangeUnitsId = 0;
    XGUID _elementPolarBearingId = 0, _elementPolarBearingModeId = 0;
    XGUID _elementPolarInclinationId = 0;
    XGUID _elementPolarRollId = 0;
    XGUID _elementAzimuthId = 0;
    XGUID _elementInclinationId = 0;
    XGUID _nextScreenId = 0;
    char *_portGlobalValue = nullptr;
    bool _showPortOverride = false;
public:
    CctControl_ElementRangeFinder(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementRangeFinder();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_ElementRangeFinder(CfxPersistent *pOwner, CfxControl *pParent);
