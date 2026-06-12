#include "ctElementRangeFinder.h"

// Retained as a no-op stub for resource file backward compatibility.

CfxControl *Create_Control_ElementRangeFinder(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementRangeFinder(pOwner, pParent);
}

CctControl_ElementRangeFinder::CctControl_ElementRangeFinder(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_RangeFinder(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTRANGEFINDER);
    InitLockProperties("Required;Style");

    _required = true;
    _autoNext = false;
    _clearOnEnter = false;
    _snapDateTime = _snapGPS = false;
    memset(&_snapDateTimeValue, 0, sizeof(_snapDateTimeValue));
    memset(&_snapGPSValue, 0, sizeof(_snapGPSValue));
    _rangeFinderId = nullptr;

    InitXGuid(&_elementRangeFinderId);
    InitXGuid(&_elementRangeId);
    InitXGuid(&_elementRangeUnitsId);
    InitXGuid(&_elementPolarBearingId);
    InitXGuid(&_elementPolarBearingModeId);
    InitXGuid(&_elementPolarInclinationId);
    InitXGuid(&_elementPolarRollId);
    InitXGuid(&_elementAzimuthId);
    InitXGuid(&_elementInclinationId);
    InitXGuid(&_nextScreenId);

    _portGlobalValue = nullptr;
    _showPortOverride = false;
}

CctControl_ElementRangeFinder::~CctControl_ElementRangeFinder()
{
    MEM_FREE(_rangeFinderId);
    _rangeFinderId = nullptr;
    MEM_FREE(_portGlobalValue);
    _portGlobalValue = nullptr;
}

void CctControl_ElementRangeFinder::DefineProperties(CfxFiler &F)
{
    CfxControl_RangeFinder::DefineProperties(F);

    F.DefineValue("AutoNext", dtBoolean, &_autoNext, F_FALSE);
    F.DefineValue("ClearOnEnter", dtBoolean, &_clearOnEnter, F_FALSE);
    F.DefineValue("Required", dtBoolean, &_required, F_TRUE);
    F.DefineValue("ShowPortSelect", dtBoolean, &_showPortOverride, F_FALSE);

    F.DefineXOBJECT("ElementRangeFinder", &_elementRangeFinderId);
    F.DefineXOBJECT("ElementRange", &_elementRangeId);
    F.DefineXOBJECT("ElementRangeUnits", &_elementRangeUnitsId);
    F.DefineXOBJECT("ElementPolarBearing", &_elementPolarBearingId);
    F.DefineXOBJECT("ElementPolarBearingMode", &_elementPolarBearingModeId);
    F.DefineXOBJECT("ElementPolarInclination", &_elementPolarInclinationId);
    F.DefineXOBJECT("ElementPolarRoll", &_elementPolarRollId);
    F.DefineXOBJECT("ElementAzimuth", &_elementAzimuthId);
    F.DefineXOBJECT("ElementInclination", &_elementInclinationId);
    F.DefineXOBJECT("NextScreenId", &_nextScreenId);

    F.DefineValue("RangeFinderId", dtPText, &_rangeFinderId);
    F.DefineValue("PortGlobalValue", dtPText, &_portGlobalValue);
    F.DefineValue("SnapDateTime", dtBoolean, &_snapDateTime);
    F.DefineValue("SnapGPS", dtBoolean, &_snapGPS);
}

void CctControl_ElementRangeFinder::DefineState(CfxFiler &F)
{
    CfxControl_RangeFinder::DefineState(F);

    F.DefineValue("RangeState",     dtByte,    &_range.State);
    F.DefineValue("PortIdOverride", dtByte,    &_portIdOverride);
    F.DefineValue("PortGlobalValue", dtPText,  &_portGlobalValue);
    F.DefineValue("RangeTimeStamp", dtInt32,   &_range.TimeStamp);
    F.DefineRange(&_range.Range);
    F.DefineGPS(&_snapGPSValue.Position);
    F.DefineDateTime(&_snapDateTimeValue);
}

void CctControl_ElementRangeFinder::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_ElementRangeFinder::DefineResources(CfxFilerResource &F)
{
}
