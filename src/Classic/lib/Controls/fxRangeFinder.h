#pragma once
#include "pch.h"
#include "fxControl.h"

// Retained as a no-op stub for resource file backward compatibility.

const GUID GUID_CONTROL_RANGEFINDER = {0xda6b83f4, 0x61f1, 0x4b9c, {0xb5, 0xaf, 0xdf, 0x89, 0x92, 0x71, 0x2c, 0x87}};

enum class RangeFinderControlStyle : int8_t {State=0, Range, All};

class CfxControl_RangeFinder: public CfxControl
{
protected:
    COLOR _textColor = 0;
    RangeFinderControlStyle _style = RangeFinderControlStyle::State;
    FXRANGE _range;

    bool _legacyMode = false;
    BYTE _portId = 0, _portIdOverride = 0, _portIdConnected = 0;
    FX_COMPORT _comPort;
    bool _connected = false, _tryConnect = false;
    bool _keepConnected = false, _hasData = false, _showPortState = false;

    void ResetRange() { memset(&_range, 0, sizeof(_range)); }

public:
    CfxControl_RangeFinder(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
    virtual void OnRange(FXRANGE *pRange) {}
};

extern CfxControl *Create_Control_RangeFinder(CfxPersistent *pOwner, CfxControl *pParent);
