/*************************************************************************************************/
//                                                                                                *
//   Copyright (C) 2005              Software                                                     *
//                                                                                                *
//   Original author: Justin Steventon (justin@steventon.com)                                     *
//                                                                                                *
//   You may retrieve the latest version of this file via the              Software home page,    *
//   located at: http://www.            .com.                                                     *
//                                                                                                *
//   The contents of this file are used subject to the Mozilla Public License Version 1.1         *
//   (the "License"); you may not use this file except in compliance with the License. You may    *
//   obtain a copy of the License from http://www.mozilla.org/MPL.                                *
//                                                                                                *
//   Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY  *
//   OF ANY KIND, either express or implied. See the License for the specific language governing  *
//   rights and limitations under the License.                                                    *
//                                                                                                *
//************************************************************************************************/

#include "fxUtils.h"
#include "fxApplication.h"
#include "ctElementRangeFinder.h"
#include "ctElement.h"
#include "ctActions.h"
#include "ctDialog_ComPortSelect.h"

CfxControl *Create_Control_ElementRangeFinder(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementRangeFinder(pOwner, pParent);
}

CctControl_ElementRangeFinder::CctControl_ElementRangeFinder(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_RangeFinder(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTRANGEFINDER);

    InitLockProperties("Required;Style");

    _required = TRUE;
    _autoNext = FALSE;
    _clearOnEnter = FALSE;
    _snapDateTime = _snapGPS = FALSE;
    memset(&_snapDateTimeValue, 0, sizeof(_snapDateTimeValue));
    memset(&_snapGPSValue, 0, sizeof(_snapGPSValue));

    _rangeFinderId = NULL;

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

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementRangeFinder::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementRangeFinder::OnSessionNext);

    _showPortOverride = FALSE;

    _portGlobalValue = NULL;

    _buttonPortSelect = new CfxControl_Button(pOwner, this);
    _buttonPortSelect->SetComposite(TRUE);
    _buttonPortSelect->SetVisible(_showPortOverride);
    _buttonPortSelect->SetBorder(bsNone, 0, 0, _borderColor);
    _buttonPortSelect->SetOnClick(this, (NotifyMethod)&CctControl_ElementRangeFinder::OnPortSelectClick);
    _buttonPortSelect->SetOnPaint(this, (NotifyMethod)&CctControl_ElementRangeFinder::OnPortSelectPaint);

    Reset();
}

CctControl_ElementRangeFinder::~CctControl_ElementRangeFinder()
{
    Reset(_keepConnected);

    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    MEM_FREE(_rangeFinderId);
    _rangeFinderId = NULL;

    MEM_FREE(_portGlobalValue);
    _portGlobalValue = NULL;
}

VOID CctControl_ElementRangeFinder::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    CctSession *session = GetSession(this); 

    if (_required && !_range.HasRange())
    {
        session->ShowMessage("Range not acquired");
        *((BOOL *)pParams) = FALSE;
        return;
    }

    if (_range.HasRange() && _snapGPS && !TestGPS(&_snapGPSValue, session->GetSightingAccuracy()))
    {
        session->ShowMessage("GPS not acquired");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_ElementRangeFinder::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    // Do nothing if the sender is the session and a next screen is assigned. 
    if ((pSender != this) && !IsNullXGuid(&_nextScreenId))
    {
        return;
    }

    CctSession *session = GetSession(this);

    {
        CctAction_MergeAttribute action(this);

        FXRANGE_DATA *pRange = &_range.Range;
        CHAR *szUnits = NULL;

        if (pRange->Flags & RANGE_FLAG_RANGE)
        {
            if (_rangeFinderId != NULL)
            {
                action.SetupAdd(_id, _elementRangeFinderId, dtPText, &_rangeFinderId);
            }

            action.SetupAdd(_id, _elementRangeId, dtDouble, &pRange->Range);
            szUnits = AllocString("M");
            szUnits[0] = pRange->RangeUnits;
            action.SetupAdd(_id, _elementRangeUnitsId, dtPText, &szUnits);
            MEM_FREE(szUnits);
        }

        if (pRange->Flags & RANGE_FLAG_POLAR_BEARING)
        {
            action.SetupAdd(_id, _elementPolarBearingId, dtDouble, &pRange->PolarBearing);
            switch (pRange->PolarBearingMode)
            {
            case 'M': szUnits = AllocString("Magnetic north"); break;
            case 'T': szUnits = AllocString("True north"); break;
            case 'E': szUnits = AllocString("Encoder"); break;
            case 'D': szUnits = AllocString("Difference"); break;
            default:  szUnits = AllocString("Unknown"); break;
            }
            action.SetupAdd(_id, _elementPolarBearingModeId, dtPText, &szUnits);
            MEM_FREE(szUnits);
        }

        if (pRange->Flags & RANGE_FLAG_POLAR_INCLINATION)
        {
            action.SetupAdd(_id, _elementPolarInclinationId, dtDouble, &pRange->PolarInclination);
        }

        if (pRange->Flags & RANGE_FLAG_POLAR_ROLL)
        {
            action.SetupAdd(_id, _elementPolarRollId, dtDouble, &pRange->PolarRoll);
        }

        if (pRange->Flags & RANGE_FLAG_AZIMUTH)
        {
            action.SetupAdd(_id, _elementAzimuthId, dtDouble, &pRange->Azimuth);
        }

        if (pRange->Flags & RANGE_FLAG_INCLINATION)
        {
            action.SetupAdd(_id, _elementInclinationId, dtDouble, &pRange->Inclination);
        }
         
        if (action.IsValid())
        {
            session->ExecuteAction(&action);
        }
    }

    if (_range.HasRange() && _snapDateTime && (_snapDateTimeValue.Year != 0))
    {
        CctAction_SnapDateTime action(this);
        action.Setup(_id, &_snapDateTimeValue);
        session->ExecuteAction(&action);
    }

    if (_range.HasRange() && _snapGPS && TestGPS(&_snapGPSValue, session->GetSightingAccuracy()))
    {
        CctAction_SnapGPS action(this);
        action.Setup(&_snapGPSValue.Position);
        session->ExecuteAction(&action);
    }
}

VOID CctControl_ElementRangeFinder::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl_RangeFinder::SetBounds(Left, Top, Width, Height);

    UINT spacer = 4 * _borderWidth;
    UINT size = _borderWidth * 24;
    _buttonPortSelect->SetBounds(Width - size - spacer, spacer, size, size);
}

VOID CctControl_ElementRangeFinder::OnPortSelectClick(CfxControl *pSender, VOID *pParams)
{
    PORTSELECT_PARAMS params;
    params.ControlId = GetId();
    params.PortId = _portId;
    if (_portIdOverride != 0)
    {
        params.PortId = _portIdOverride;
    }

    Reset();

    GetSession(this)->ShowDialog(&GUID_DIALOG_PORTSELECT, NULL, &params, sizeof(PORTSELECT_PARAMS)); 
}

VOID CctControl_ElementRangeFinder::OnDialogResult(GUID *pDialogId, VOID *pParams)
{
    // Ignore overrides from the global value
    MEM_FREE(_portGlobalValue);
    _portGlobalValue = NULL;

    _portIdOverride = (BYTE)(UINT64)pParams;
    GetSession(this)->SaveControlState(this);

    Reset();
    _tryConnect = TRUE;
    Repaint();
}

VOID CctControl_ElementRangeFinder::OnPortSelectPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPort(pParams->Rect, pParams->Canvas->InvertColor(_color), pParams->Canvas->InvertColor(_borderColor), _buttonPortSelect->GetDown()); 
}

VOID CctControl_ElementRangeFinder::DefineProperties(CfxFiler &F)
{
    CfxControl_RangeFinder::DefineProperties(F);

    // Null id is automatically assigned to static
    #ifdef CLIENT_DLL
        if (IsNullXGuid(&_elementRangeFinderId)) _elementRangeFinderId = GUID_ELEMENT_RANGEFINDER_ID;
        if (IsNullXGuid(&_elementRangeId)) _elementRangeId = GUID_ELEMENT_RANGE;
        if (IsNullXGuid(&_elementRangeUnitsId)) _elementRangeUnitsId = GUID_ELEMENT_RANGE_UNITS;
        if (IsNullXGuid(&_elementPolarBearingId)) _elementPolarBearingId = GUID_ELEMENT_POLAR_BEARING;
        if (IsNullXGuid(&_elementPolarBearingModeId)) _elementPolarBearingModeId = GUID_ELEMENT_POLAR_BEARINGMODE;
        if (IsNullXGuid(&_elementPolarInclinationId)) _elementPolarInclinationId = GUID_ELEMENT_POLAR_INCLINATION;
        if (IsNullXGuid(&_elementPolarRollId)) _elementPolarRollId = GUID_ELEMENT_POLAR_ROLL;
        if (IsNullXGuid(&_elementAzimuthId)) _elementAzimuthId = GUID_ELEMENT_AZIMUTH;
        if (IsNullXGuid(&_elementInclinationId)) _elementInclinationId = GUID_ELEMENT_INCLINATION;
    #endif

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

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);
        _buttonPortSelect->SetVisible(_showPortOverride && !_legacyMode);
    }
}

VOID CctControl_ElementRangeFinder::DefineState(CfxFiler &F)
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

VOID CctControl_ElementRangeFinder::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Auto next",    dtBoolean,  &_autoNext);
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Clear on enter", dtBoolean, &_clearOnEnter);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("COM - Auto detect", dtBoolean, &_legacyMode);
    F.DefineValue("COM - Global value", dtPText, &_portGlobalValue);
    F.DefineCOM(&_portId, &_comPort);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    
    F.DefineValue("Element azimuth", dtGuid, &_elementAzimuthId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Element inclination", dtGuid, &_elementInclinationId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Element range", dtGuid, &_elementRangeId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Element range units", dtGuid, &_elementRangeUnitsId, "EDITOR(ScreenElementsEditor)");

    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Keep connected", dtBoolean, &_keepConnected);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("RangeFinder id", dtPText,  &_rangeFinderId);
    F.DefineValue("Required",     dtBoolean,  &_required);
    F.DefineValue("Show port select", dtBoolean, &_showPortOverride);
    F.DefineValue("Show port state", dtBoolean, &_showPortState);
    F.DefineValue("Snap date and time", dtBoolean, &_snapDateTime);
    F.DefineValue("Snap GPS",     dtBoolean,  &_snapGPS);
    F.DefineValue("Style",        dtByte,     &_style,       "LIST(State Range All)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);

    F.DefineStaticLink("Next screen",  &_nextScreenId);
#endif
}

VOID CctControl_ElementRangeFinder::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_RangeFinder::DefineResources(F);

    F.DefineObject(_elementRangeFinderId,           eaName);
    F.DefineObject(_elementRangeId,                 eaName);
    F.DefineObject(_elementRangeUnitsId,            eaName);
    F.DefineObject(_elementPolarBearingId,          eaName);
    F.DefineObject(_elementPolarBearingModeId,      eaName);
    F.DefineObject(_elementPolarInclinationId,      eaName);
    F.DefineObject(_elementPolarRollId,             eaName);
    F.DefineObject(_elementAzimuthId,               eaName);
    F.DefineObject(_elementInclinationId,           eaName);

    F.DefineObject(_nextScreenId);

    F.DefineField(_elementRangeFinderId,           dtText);
    F.DefineField(_elementRangeId,                 dtDouble);
    F.DefineField(_elementRangeUnitsId,            dtText);
    F.DefineField(_elementPolarBearingId,          dtDouble);
    F.DefineField(_elementPolarBearingModeId,      dtText);
    F.DefineField(_elementPolarInclinationId,      dtDouble);
    F.DefineField(_elementPolarRollId,             dtDouble);
    F.DefineField(_elementAzimuthId,               dtDouble);
    F.DefineField(_elementInclinationId,           dtDouble);
#endif
}

VOID CctControl_ElementRangeFinder::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (_tryConnect && IsLive())
    {
        GetSession(this)->LoadControlState(this);

        if (_portGlobalValue)
        {
            GLOBALVALUE *globalValue = GetSession(this)->GetGlobalValue(_portGlobalValue);
            if (globalValue)
            {
                _portIdOverride = (BYTE)globalValue->Value;
            }
        }
    }

    CfxControl_RangeFinder::OnPaint(pCanvas, pRect);
}

VOID CctControl_ElementRangeFinder::OnRange(FXRANGE *pRange)
{
    GetEngine(this)->SetLastRange(pRange);

    // Snapshot attributes that must correlate with the range
    if (pRange->HasRange())
    {
        // Snap the date and time
        if (_snapDateTime)
        {
            GetHost(this)->GetDateTime(&_snapDateTimeValue);
        }

        // Snap the GPS position
        if (_snapGPS)
        {
            _snapGPSValue = *GetEngine(this)->GetGPS();
        }
    }

    // Check for auto-next
    if (_autoNext && pRange->HasRange())
    {
        if (!_range.HasRange()) return;

        if (!IsNullXGuid(&_nextScreenId)) 
        {
            BOOL canNext = TRUE;
            OnSessionCanNext(this, &canNext);
            if (canNext)
            {
                OnSessionNext(this, NULL);
                ResetRange();
                GetSession(this)->DoSkipToScreen(_nextScreenId);
            }
        }
        else
        {
            GetEngine(this)->KeyPress(KEY_RIGHT);
        }

        return;
    }

    Repaint();
}
