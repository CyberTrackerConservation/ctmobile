#include "fxRangeFinder.h"
#include "fxUtils.h"

// Retained as a no-op stub for resource file backward compatibility.

CfxControl *Create_Control_RangeFinder(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_RangeFinder(pOwner, pParent);
}

CfxControl_RangeFinder::CfxControl_RangeFinder(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_RANGEFINDER);
    InitLockProperties("Style");
    InitBorder(ControlBorderStyle::Single, 1);
    InitFont(F_FONT_DEFAULT_S);
    _textColor = _borderColor;

    memset(&_range, 0, sizeof(_range));

    _legacyMode = true;
    _keepConnected = false;
    _portId = _portIdOverride = _portIdConnected = 0;
    InitCOM(&_portId, &_comPort);
    _connected = _tryConnect = _hasData = _showPortState = false;
    _style = RangeFinderControlStyle::Range;
}

void CfxControl_RangeFinder::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("KeepConnected", dtBoolean, &_keepConnected, F_FALSE);
    F.DefineValue("LegacyMode",   dtBoolean,  &_legacyMode, F_TRUE);
    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
    F.DefineValue("Style",        dtInt8,     &_style, F_ONE);
    F.DefineValue("ShowPortState",dtBoolean,  &_showPortState, F_FALSE);

    F.DefineCOM(&_portId, &_comPort);
}

void CfxControl_RangeFinder::DefineState(CfxFiler &F)
{
    CfxControl::DefineState(F);
}

void CfxControl_RangeFinder::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CfxControl_RangeFinder::DefineResources(CfxFilerResource &F)
{
}
