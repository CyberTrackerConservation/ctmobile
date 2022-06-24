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

#include "fxGPS.h"
#include "ctActions.h"
#include "ctActionControls.h"
#include "ctElement.h"

//*************************************************************************************************
// CctControl_ConfigureGPS

CfxControl *Create_Control_ConfigureGPS(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ConfigureGPS(pOwner, pParent);
}

CctControl_ConfigureGPS::CctControl_ConfigureGPS(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_CONFIGUREGPS);
    InitLockProperties("Track timer (in seconds)");

    _accuracyEnabled = FALSE;
    _dock = dkAction;
    InitGpsAccuracy(&_sightingAccuracy);
    _sightingFixCount = 1;
    InitGpsAccuracy(&_waypointAccuracy);
    _waypointTimer = 0;

    _useRangeFinderForAltitude = FALSE;

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_ConfigureGPS::OnSessionEvent);
}

CctControl_ConfigureGPS::~CctControl_ConfigureGPS()
{
    UnregisterSessionEvent(seEnter, this);
}

VOID CctControl_ConfigureGPS::OnSessionEvent(CfxControl *pSender, VOID *pParams)
{
    CctAction_ConfigureGPS action(this);
    action.Setup(_accuracyEnabled, &_sightingAccuracy, _sightingFixCount, &_waypointAccuracy, _waypointTimer, _useRangeFinderForAltitude);
    ((CctSession *)pSender)->ExecuteAction(&action);
}

VOID CctControl_ConfigureGPS::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    // CONFIGURE_GPS_VERSION
    F.DefineValue("Version", dtInt32,   &_version);

    F.DefineValue("AccuracyEnabled", dtBoolean, &_accuracyEnabled);
    F.DefineValue("SightingAccuracy", dtDouble, &_sightingAccuracy.DilutionOfPrecision);
    F.DefineValue("SightingAccuracyMaxSpeed", dtDouble, &_sightingAccuracy.MaximumSpeed);
    F.DefineValue("SightingFixCount", dtInt32, &_sightingFixCount);
    F.DefineValue("WaypointAccuracy", dtDouble, &_waypointAccuracy.DilutionOfPrecision);
    F.DefineValue("WaypointAccuracyMaxSpeed", dtDouble, &_waypointAccuracy.MaximumSpeed);
    F.DefineValue("WaypointTimer", dtInt32, &_waypointTimer);
    F.DefineValue("UseRangeFinderForAltitude", dtBoolean, &_useRangeFinderForAltitude);

    if (F.IsReader())
    {
        if (_version < CONFIGURE_GPS_VERSION)
        {
            _accuracyEnabled = (_sightingAccuracy.DilutionOfPrecision < 49.0) ||
                               (_waypointAccuracy.DilutionOfPrecision < 49.0);
        }
    }
}

VOID CctControl_ConfigureGPS::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Filter enabled", dtBoolean, &_accuracyEnabled);
    F.DefineValue("Sighting accuracy (DOP)", dtDouble, &_sightingAccuracy.DilutionOfPrecision, "MIN(0.01);MAX(49)");
    F.DefineValue("Sighting max speed (km/h)", dtDouble, &_sightingAccuracy.MaximumSpeed, "MIN(0);MAX(10000)");
    F.DefineValue("Sighting fix count", dtInt32, &_sightingFixCount, "MIN(1);MAX(60)");
    F.DefineValue("Track accuracy (DOP)", dtDouble, &_waypointAccuracy.DilutionOfPrecision, "MIN(0.01);MAX(49)");
    F.DefineValue("Track max speed (km/h)", dtDouble, &_waypointAccuracy.MaximumSpeed, "MIN(0);MAX(10000)");
    F.DefineValue("Track timer (in seconds)", dtInt32, &_waypointTimer, "MIN(0);MAX(1000)");
    F.DefineValue("Use RangeFinder for altitude", dtBoolean, &_useRangeFinderForAltitude);
#endif
}

//*************************************************************************************************
// CctControl_SnapGPS

CfxControl *Create_Control_SnapGPS(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_SnapGPS(pOwner, pParent);
}

CctControl_SnapGPS::CctControl_SnapGPS(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SNAPGPS);
    InitLockProperties("Required");

    memset(&_gps, 0, sizeof(_gps));

    _dock = dkAction;
    _required = TRUE;
    _autoConnect = TRUE;
    _connected = FALSE;

    InitXGuid(&_latId);
    InitXGuid(&_lonId);
    InitXGuid(&_altId);
    InitXGuid(&_accId);

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_SnapGPS::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_SnapGPS::OnSessionNext);
}

CctControl_SnapGPS::~CctControl_SnapGPS()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    if (_connected)
    {
        GetEngine(this)->UnlockGPS();
    }
}

VOID CctControl_SnapGPS::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{    
    CctSession *session = (CctSession *)pSender; 
    CfxEngine *engine = GetEngine(this);
    FXGPS_POSITION *gps = NULL;
    BOOL fixGood = FALSE;

    if (session->IsEditing())
    {
        gps = &_gps;
        fixGood = TRUE;
    }
    else
    {
        gps = &engine->GetGPS()->Position;    
        fixGood = TestGPS(engine->GetGPS(), session->GetSightingAccuracy());
    }

    // Block "Next" if the accuracy doesn't meet the requirements
    if (!fixGood && _required)
    {
        session->ShowMessage("GPS not acquired");
        *((BOOL *)pParams) = FALSE;
    }
    else if (session->GetResourceHeader()->GpsTimeSync && !engine->GetGPS()->TimeInSync)
    {
        session->ShowMessage("Time not found");
        *((BOOL *)pParams) = FALSE;
    }
    else
    {
        _gps = *gps;
    }
}

VOID CctControl_SnapGPS::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    // Do nothing if we're editing
    CctSession *session = (CctSession *)pSender; 

    // Save the GPS position to the sighting
    if (TestGPS(&_gps, session->GetSightingAccuracy()))
    {
        CctAction_MergeAttribute action(this);
        BOOL useAdd = FALSE;

        if (!IsNullXGuid(&_latId))      // Latitude
        {
            action.SetupAdd(_id, _latId, dtDouble, &_gps.Latitude);
            useAdd = TRUE;
        }
        
        if (!IsNullXGuid(&_lonId))      // Longitude
        {
            action.SetupAdd(_id, _lonId, dtDouble, &_gps.Longitude);
            useAdd = TRUE;
        }

        if (!IsNullXGuid(&_altId))      // Altitude
        {
            action.SetupAdd(_id, _altId, dtDouble, &_gps.Altitude);
            useAdd = TRUE;
        }

        if (!IsNullXGuid(&_accId))      // Accuracy
        {
            action.SetupAdd(_id, _accId, dtDouble, &_gps.Accuracy);
            useAdd = TRUE;
        }

        if (useAdd)                     // Use manual attributes
        {
            session->ExecuteAction(&action);
        }
        else                            // Use standard GPS
        {
            CctAction_SnapGPS action(this);
            action.Setup(&_gps);
            session->ExecuteAction(&action);
        }
    }
}

VOID CctControl_SnapGPS::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    F.DefineObject(_latId, eaName);
    F.DefineObject(_lonId, eaName);
    F.DefineObject(_altId, eaName);
    F.DefineObject(_accId, eaName);
    F.DefineField(_latId, dtDouble);
    F.DefineField(_lonId, dtDouble);
    F.DefineField(_altId, dtDouble);
    F.DefineField(_accId, dtDouble);
#endif
}

VOID CctControl_SnapGPS::DefineState(CfxFiler &F)
{
    F.DefineGPS(&_gps);
}

VOID CctControl_SnapGPS::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Required", dtBoolean, &_required);
    F.DefineValue("AutoConnect",  dtBoolean,  &_autoConnect);

    F.DefineXOBJECT("LatitudeId", &_latId);
    F.DefineXOBJECT("LongitudeId", &_lonId);
    F.DefineXOBJECT("AltitudeId", &_altId);
    F.DefineXOBJECT("AccuracyId", &_accId);

    if (F.IsReader() && IsLive() && _autoConnect && !_connected)
    {
        _connected = TRUE;
        GetEngine(this)->LockGPS();
    }
}

VOID CctControl_SnapGPS::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Auto connect", dtBoolean, &_autoConnect);
    F.DefineValue("Required", dtBoolean, &_required);

    F.DefineValue("Latitude Element", dtGuid,  &_latId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Longitude Element", dtGuid,  &_lonId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Altitude Element", dtGuid,  &_altId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Accuracy Element", dtGuid,  &_accId, "EDITOR(ScreenElementsEditor)");
#endif
}

//*************************************************************************************************
// CctControl_SnapLastGPS

CfxControl *Create_Control_SnapLastGPS(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_SnapLastGPS(pOwner, pParent);
}

CctControl_SnapLastGPS::CctControl_SnapLastGPS(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SNAPLASTGPS);
    InitLockProperties("Required");

	memset(&_gps, 0, sizeof(_gps));

    _dock = dkAction;
    _required = TRUE;

    InitXGuid(&_latId);
    InitXGuid(&_lonId);
    InitXGuid(&_altId);
    InitXGuid(&_accId);

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_SnapLastGPS::OnSessionCanNext);
    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_SnapLastGPS::OnSessionEnter);
}

CctControl_SnapLastGPS::~CctControl_SnapLastGPS()
{
    UnregisterSessionEvent(seEnter, this);
    UnregisterSessionEvent(seCanNext, this);
}

VOID CctControl_SnapLastGPS::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    CctSession *session = (CctSession *)pSender; 
    BOOL fixGood = TestGPS(&_gps, session->GetSightingAccuracy());

    // Block "Next" if the accuracy doesn't meet the requirements
    if (!fixGood && _required)
    {
        session->ShowMessage("GPS not acquired");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_SnapLastGPS::OnSessionEnter(CfxControl *pSender, VOID *pParams)
{
    // Do nothing if we're editing
    CctSession *session = (CctSession *)pSender; 
    CfxEngine *engine = GetEngine(this);
    FXGPS_POSITION *gps = NULL;
    BOOL fixGood = FALSE;

    if (session->IsEditing())
    {
        gps = &_gps;
        fixGood = TRUE;
    }
    else
    {
        gps = session->GetLastSightingGPS();    
        fixGood = TestGPS(gps, session->GetSightingAccuracy());
    }

    // Block "Next" if the accuracy doesn't meet the requirements
    if (!fixGood)
    {
        return;
    }
    else
    {
        _gps = *gps;
    }

    // Save the GPS position to the sighting
    if (fixGood)
    {
        CctAction_MergeAttribute action(this);
        BOOL useAdd = FALSE;

        if (!IsNullXGuid(&_latId))      // Latitude
        {
            action.SetupAdd(_id, _latId, dtDouble, &_gps.Latitude);
            useAdd = TRUE;
        }
        
        if (!IsNullXGuid(&_lonId))      // Longitude
        {
            action.SetupAdd(_id, _lonId, dtDouble, &_gps.Longitude);
            useAdd = TRUE;
        }

        if (!IsNullXGuid(&_altId))      // Altitude
        {
            action.SetupAdd(_id, _altId, dtDouble, &_gps.Altitude);
            useAdd = TRUE;
        }

        if (!IsNullXGuid(&_accId))      // Accuracy
        {
            action.SetupAdd(_id, _accId, dtDouble, &_gps.Accuracy);
            useAdd = TRUE;
        }

        if (useAdd)                     // Use manual attributes
        {
            session->ExecuteAction(&action);
        }
        else                            // Use standard GPS
        {
            CctAction_SnapGPS action(this);
            action.Setup(&_gps);
            session->ExecuteAction(&action);
        }
    }
}

VOID CctControl_SnapLastGPS::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    F.DefineObject(_latId, eaName);
    F.DefineObject(_lonId, eaName);
    F.DefineObject(_altId, eaName);
    F.DefineObject(_accId, eaName);
    F.DefineField(_latId, dtDouble);
    F.DefineField(_lonId, dtDouble);
    F.DefineField(_altId, dtDouble);
    F.DefineField(_accId, dtDouble);
#endif
}

VOID CctControl_SnapLastGPS::DefineState(CfxFiler &F)
{
    F.DefineGPS(&_gps);
}

VOID CctControl_SnapLastGPS::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    F.DefineValue("Required", dtBoolean, &_required);
    F.DefineXOBJECT("LatitudeId", &_latId);
    F.DefineXOBJECT("LongitudeId", &_lonId);
    F.DefineXOBJECT("AltitudeId", &_altId);
    F.DefineXOBJECT("AccuracyId", &_accId);
}

VOID CctControl_SnapLastGPS::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Required", dtBoolean, &_required);

    F.DefineValue("Latitude Element", dtGuid,  &_latId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Longitude Element", dtGuid,  &_lonId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Altitude Element", dtGuid,  &_altId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Accuracy Element", dtGuid,  &_accId, "EDITOR(ScreenElementsEditor)");
#endif
}

//*************************************************************************************************
// CctControl_SnapDateTime

CfxControl *Create_Control_SnapDateTime(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_SnapDateTime(pOwner, pParent);
}

CctControl_SnapDateTime::CctControl_SnapDateTime(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SNAPDATETIME);
    InitLockProperties("Date Element;Time Element");

    _dock = dkAction;
    _connectedGPS = FALSE;

    _dateTime = 0;
    InitXGuid(&_dateId);
    InitXGuid(&_timeId);

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_SnapDateTime::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_SnapDateTime::OnSessionNext);

    if (IsLive() && GetEngine(this)->GetGPSTimeSync()) 
    {
        _connectedGPS = TRUE;
        GetEngine(this)->LockGPS();
    }
}

CctControl_SnapDateTime::~CctControl_SnapDateTime()
{
    UnregisterSessionEvent(seCanNext, this);
    UnregisterSessionEvent(seNext, this);

    if (_connectedGPS)
    {
        GetEngine(this)->UnlockGPS();
    }
}

VOID CctControl_SnapDateTime::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    // Do nothing if we're editing
    CctSession *session = (CctSession *)pSender; 
    CfxEngine *engine = GetEngine(this);

    if (session->IsEditing())
    {
        return;
    }

    if (!engine->GetGPSTimeSync()) 
    {
        return;
    }

    if (engine->GetGPSTimeUsedOnce())
    {
        return;
    }

    session->ShowMessage("GPS needed for time");
    *((BOOL *)pParams) = FALSE;
}

VOID CctControl_SnapDateTime::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    // Do nothing if we're editing
    CctSession *session = (CctSession *)pSender;
    FXDATETIME dateTime = {0};

    if (session->IsEditing())
    {
        if (_dateTime == 0) return;
        DecodeDateTime(_dateTime, &dateTime);
    }
    else
    {
        GetHost(this)->GetDateTime(&dateTime);
        _dateTime = EncodeDateTime(&dateTime);
    }

    CctAction_SnapDateTime action(this);
    action.Setup(_id, &dateTime, _dateId, _timeId);
    session->ExecuteAction(&action);
}

VOID CctControl_SnapDateTime::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    F.DefineObject(_dateId, eaName);
    F.DefineObject(_timeId, eaName);
    F.DefineField(_dateId, dtDate);
    F.DefineField(_timeId, dtTime);
#endif
}

VOID CctControl_SnapDateTime::DefineState(CfxFiler &F)
{
    F.DefineValue("SnapDateTime", dtDateTime, &_dateTime);
}

VOID CctControl_SnapDateTime::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineXOBJECT("DateId", &_dateId);
    F.DefineXOBJECT("TimeId", &_timeId);
}

VOID CctControl_SnapDateTime::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Date Element", dtGuid,  &_dateId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Time Element", dtGuid,  &_timeId, "EDITOR(ScreenElementsEditor)");
#endif
}

//*************************************************************************************************
// CctControl_AddAttribute

CfxControl *Create_Control_AddAttribute(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_AddAttribute(pOwner, pParent);
}

CctControl_AddAttribute::CctControl_AddAttribute(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ADDATTRIBUTE);
    InitLockProperties("Result Element;Value");

    _dock = dkAction;
    InitXGuid(&_elementId);
    _value = 0;
    _unique = FALSE;
    _stamp = FALSE;
    _updateTag = FALSE;
    _appId = FALSE;
    InitXGuid(&_targetScreenOnEmptyId);

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_AddAttribute::OnSessionEnter);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_AddAttribute::OnSessionNext);
}

CctControl_AddAttribute::~CctControl_AddAttribute()
{
    UnregisterSessionEvent(seEnter, this);
    UnregisterSessionEvent(seNext, this);
    MEM_FREE(_value);
}

VOID CctControl_AddAttribute::OnSessionEnter(CfxControl *pSender, VOID *pParams)
{
    _empty = TRUE;

    if (IsNullXGuid(&_elementId)) return;

    CctAction_MergeAttribute action(this);

    CHAR *temp = (CHAR *)MEM_ALLOC(64 * 3 + (_value ? strlen(_value) : 0));
    CHAR *idString = NULL;

    if (_value)
    {
        strcat(temp, _value);
    }

    if (_appId)
    {
        idString = AllocGuidName(&GetSession(this)->GetResourceHeader()->SequenceId, NULL);
        strcat(temp, idString);
        MEM_FREE(idString);
    }

    if (_unique)
    {
        GUID id = {0};
        GetHost(this)->CreateGuid(&id);
        idString = AllocGuidName(&id, NULL);
        strcat(temp, idString);
        MEM_FREE(idString);
    }

    if (_stamp)
    {
        idString = AllocGuidName(&GetSession(this)->GetResourceHeader()->StampId, NULL);
        strcat(temp, idString);
        MEM_FREE(idString);
    }

    if (_updateTag)
    {
        idString = GetSession(this)->GetUpdateTag();
        if (idString && *idString)
        {
            strcat(temp, idString);
        }
        MEM_FREE(idString);
    }

    if (temp[0])
    {
        _empty = FALSE;
        action.SetupAdd(_id, _elementId, dtPText, &temp);
    }
    else
    {
        action.SetupAdd(_id, _elementId);
    }

    MEM_FREE(temp);

    ((CctSession *)pSender)->ExecuteAction(&action);
}

VOID CctControl_AddAttribute::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    if (IsNullXGuid(&_targetScreenOnEmptyId)) return;

    if (_empty)
    {
        CctAction_InsertScreen action(this);
        action.Setup(_targetScreenOnEmptyId);
        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

VOID CctControl_AddAttribute::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineField(_elementId, dtText);
#endif
}

VOID CctControl_AddAttribute::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("AppId", dtBoolean, &_appId);
    F.DefineXOBJECT("ElementId", &_elementId);
    F.DefineValue("Unique", dtBoolean, &_unique);
    F.DefineValue("Stamp", dtBoolean, &_stamp);
    F.DefineValue("UpdateTag", dtBoolean, &_updateTag);
    F.DefineValue("Value", dtPText, &_value);
    F.DefineXOBJECT("TargetScreenOnEmpty", &_targetScreenOnEmptyId);

    HackFixResultElementLockProperties(&_lockProperties);
}

VOID CctControl_AddAttribute::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Application id",  dtBoolean, &_appId);
    F.DefineValue("Result Element", dtGuid,  &_elementId, "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Unique id",  dtBoolean, &_unique);
    F.DefineValue("Update stamp id", dtBoolean, &_stamp);
    F.DefineValue("Update tag", dtBoolean, &_updateTag);
    F.DefineValue("Value",   dtPText, &_value);
    F.DefineStaticLink("Link on empty", &_targetScreenOnEmptyId);
#endif
}

//*************************************************************************************************
// CctControl_AddUserName

CfxControl *Create_Control_AddUserName(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_AddUserName(pOwner, pParent);
}

CctControl_AddUserName::CctControl_AddUserName(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ADDUSERNAME);
    InitLockProperties("Result Element");

    _dock = dkAction;
    InitXGuid(&_elementId);

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_AddUserName::OnSessionEvent);
}

CctControl_AddUserName::~CctControl_AddUserName()
{
    UnregisterSessionEvent(seEnter, this);
}

VOID CctControl_AddUserName::OnSessionEvent(CfxControl *pSender, VOID *pParams)
{
    if (IsNullXGuid(&_elementId)) return;

    CHAR *userName = GetHost(this)->GetDeviceName();
    CHAR *value = NULL;
    Type_DataFromText(userName, dtPText, &value);

    CctAction_MergeAttribute action(this);
    action.SetupAdd(_id, _elementId, dtPText, &value);
    ((CctSession *)pSender)->ExecuteAction(&action);

    MEM_FREE(userName);
    Type_FreeAndNil((VOID **)&value);
}

VOID CctControl_AddUserName::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineField(_elementId, dtText);
#endif
}

VOID CctControl_AddUserName::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineXOBJECT("ElementId", &_elementId);

    // Null id is automatically assigned to static

    #ifdef CLIENT_DLL
        if (IsNullXGuid(&_elementId))
        {
            _elementId = GUID_ELEMENT_USERNAME;
        }
    #endif

    HackFixResultElementLockProperties(&_lockProperties);
}

VOID CctControl_AddUserName::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Result Element", dtGuid,  &_elementId, "EDITOR(ScreenElementsEditor);REQUIRED");
#endif
}

//*************************************************************************************************
// CctControl_ConfigureRangeFinder

CfxControl *Create_Control_ConfigureRangeFinder(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ConfigureRangeFinder(pOwner, pParent);
}

CctControl_ConfigureRangeFinder::CctControl_ConfigureRangeFinder(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_CONFIGURERANGEFINDER);
    InitLockProperties("Connected");

    _dock = dkAction;
    _state = TRUE;

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_ConfigureRangeFinder::OnSessionEvent);
}

CctControl_ConfigureRangeFinder::~CctControl_ConfigureRangeFinder()
{
    UnregisterSessionEvent(seEnter, this);
}

VOID CctControl_ConfigureRangeFinder::OnSessionEvent(CfxControl *pSender, VOID *pParams)
{
    CctAction_ConfigureRangeFinder action(this);
    action.Setup(_state);
    ((CctSession *)pSender)->ExecuteAction(&action);
}

VOID CctControl_ConfigureRangeFinder::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    F.DefineValue("State", dtBoolean, &_state);
}

VOID CctControl_ConfigureRangeFinder::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Connected", dtBoolean, &_state);
#endif
}

//*************************************************************************************************
// CctControl_ConfigureSaveTargets

CfxControl *Create_Control_ConfigureSaveTargets(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ConfigureSaveTargets(pOwner, pParent);
}

CctControl_ConfigureSaveTargets::CctControl_ConfigureSaveTargets(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_CONFIGURESAVETARGETS);
    InitLockProperties("Default save 1 target;Default save 2 target");

    _dock = dkAction;
    InitXGuid(&_save1Target);
    InitXGuid(&_save2Target);

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_ConfigureSaveTargets::OnSessionEvent);
}

CctControl_ConfigureSaveTargets::~CctControl_ConfigureSaveTargets()
{
    UnregisterSessionEvent(seEnter, this);
}

VOID CctControl_ConfigureSaveTargets::OnSessionEvent(CfxControl *pSender, VOID *pParams)
{
    CctAction_ConfigureSaveTargets action(this);
    action.Setup(_save1Target, _save2Target);
    ((CctSession *)pSender)->ExecuteAction(&action);
}

VOID CctControl_ConfigureSaveTargets::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    F.DefineXOBJECT("Save1Target", &_save1Target);
    F.DefineXOBJECT("Save2Target", &_save2Target);
}

VOID CctControl_ConfigureSaveTargets::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineStaticLink("Default save 1 target", &_save1Target);
    F.DefineStaticLink("Default save 2 target", &_save2Target);
#endif
}

VOID CctControl_ConfigureSaveTargets::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineObject(_save1Target);
    F.DefineObject(_save2Target);
#endif
}

//*************************************************************************************************
// CctControl_ResetStateKey

CfxControl *Create_Control_ResetStateKey(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ResetStateKey(pOwner, pParent);
}

CctControl_ResetStateKey::CctControl_ResetStateKey(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_RESETSTATEKEY);
    InitLockProperties("Reset key");

    _dock = dkAction;
    _resetKey = NULL;
    _immediate = TRUE;

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_ResetStateKey::OnSessionEventEnter);
    RegisterSessionEvent(seAfterNext, this, (NotifyMethod)&CctControl_ResetStateKey::OnSessionEventLeave);
}

CctControl_ResetStateKey::~CctControl_ResetStateKey()
{
    UnregisterSessionEvent(seEnter, this);
    UnregisterSessionEvent(seAfterNext, this);

    MEM_FREE(_resetKey);
    _resetKey = NULL;
}

VOID CctControl_ResetStateKey::Execute(CctSession *pSession)
{
    DOUBLE value;

    GLOBALVALUE *globalValue = pSession->GetGlobalValue(_resetKey);
    value = globalValue != NULL ? globalValue->Value : 1;

    CctAction_SetGlobalValue action(this);
    action.Setup(_resetKey, value + 1);
    pSession->ExecuteAction(&action);
}

VOID CctControl_ResetStateKey::OnSessionEventEnter(CfxControl *pSender, VOID *pParams)
{
    if (!_resetKey || !_immediate) return;

    Execute((CctSession *)pSender);
}

VOID CctControl_ResetStateKey::OnSessionEventLeave(CfxControl *pSender, VOID *pParams)
{
    if (!_resetKey || _immediate) return;

    Execute((CctSession *)pSender);
}

VOID CctControl_ResetStateKey::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    F.DefineValue("Immediate", dtBoolean, &_immediate, F_TRUE);
    F.DefineValue("ResetKey", dtPText, &_resetKey);
}

VOID CctControl_ResetStateKey::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Immediate", dtBoolean, &_immediate);
    F.DefineValue("Reset key", dtPText, &_resetKey);
#endif
}

//*************************************************************************************************
// CctControl_SetPendingGoto

CfxControl *Create_Control_SetPendingGoto(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_SetPendingGoto(pOwner, pParent);
}

CctControl_SetPendingGoto::CctControl_SetPendingGoto(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SETPENDINGGOTO);
    InitLockProperties("Title Element;Title prefix");

    _dock = dkAction;
    InitXGuid(&_titleElementId);
    _titlePrefix = NULL;

    RegisterSessionEvent(seAfterNext, this, (NotifyMethod)&CctControl_SetPendingGoto::OnSessionEvent);
}

CctControl_SetPendingGoto::~CctControl_SetPendingGoto()
{
    UnregisterSessionEvent(seAfterNext, this);
    MEM_FREE(_titlePrefix);
}

VOID CctControl_SetPendingGoto::OnSessionEvent(CfxControl *pSender, VOID *pParams)
{
    if (IsNullXGuid(&_titleElementId) && (_titlePrefix == NULL)) return;

    CfxString valueText("");

    if (!IsNullXGuid(&_titleElementId))
    {
        ATTRIBUTE *attribute = GetSession(this)->GetCurrentSighting()->FindAttribute(&_titleElementId);

        if (attribute)
        {
            GetAttributeText(this, attribute, valueText);
        }
    }

    UINT titleLen = valueText.Length() + (_titlePrefix ? strlen(_titlePrefix) : 0) + 1;
    CHAR *title = (CHAR *)MEM_ALLOC(titleLen);
    memset(title, 0, titleLen);
    
    if (_titlePrefix)
    {
        strcpy(title, _titlePrefix);
    }

    if (valueText.Length() > 0)
    {
        strcat(title, valueText.Get());
    }

    if (strlen(title) > 0)
    {
        CctAction_SetPendingGoto action(this);
        action.Setup(title);
        ((CctSession *)pSender)->ExecuteAction(&action);
    }

    MEM_FREE(title);
}

VOID CctControl_SetPendingGoto::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    F.DefineObject(_titleElementId, eaName);

    if (IsLive() && !IsNullXGuid(&_titleElementId))
    {
        ATTRIBUTE *attribute = GetSession(this)->GetCurrentSighting()->FindAttribute(&_titleElementId);
        F.DefineObject(attribute->ValueId, eaName);
    }
#endif
}

VOID CctControl_SetPendingGoto::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineXOBJECT("TitleElementId", &_titleElementId);
    F.DefineValue("TitlePrefix", dtPText, &_titlePrefix);
}

VOID CctControl_SetPendingGoto::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Title Element", dtGuid,  &_titleElementId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Title prefix",  dtPText, &_titlePrefix);
#endif
}

//*************************************************************************************************
// CctControl_AddCustomGoto

CfxControl *Create_Control_AddCustomGoto(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_AddCustomGoto(pOwner, pParent);
}

CctControl_AddCustomGoto::CctControl_AddCustomGoto(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ADDCUSTOMGOTO);
    InitLockProperties("Title;Latitude;Longitude");

    _dock = dkAction;
    _title = NULL;
    _latitude = _longitude = 0;

    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_AddCustomGoto::OnSessionEvent);
}

CctControl_AddCustomGoto::~CctControl_AddCustomGoto()
{
    UnregisterSessionEvent(seNext, this);
    MEM_FREE(_title);
}

VOID CctControl_AddCustomGoto::OnSessionEvent(CfxControl *pSender, VOID *pParams)
{
    if (!_title || (strlen(_title) == 0)) return;

    FXGOTO addGoto = {MAGIC_GOTO};
    
    strncpy(addGoto.Title, _title, ARRAYSIZE(addGoto.Title)-1);
    addGoto.X = _longitude;
    addGoto.Y = _latitude;

    CctAction_AddCustomGoto action(this);
    action.Setup(&addGoto);
    ((CctSession *)pSender)->ExecuteAction(&action);
}

VOID CctControl_AddCustomGoto::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
#endif
}

VOID CctControl_AddCustomGoto::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Title", dtPText, &_title);
    F.DefineValue("Latitude", dtDouble, &_latitude);
    F.DefineValue("Longitude", dtDouble, &_longitude);
}

VOID CctControl_AddCustomGoto::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Latitude",  dtDouble, &_latitude, "MIN(-90);MAX(90)");
    F.DefineValue("Longitude", dtDouble, &_longitude, "MIN(-180);MAX(180)");
    F.DefineValue("Title",     dtPText, &_title);
#endif
}

//*************************************************************************************************
// CctControl_WriteTrackBreak

CfxControl *Create_Control_WriteTrackBreak(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_WriteTrackBreak(pOwner, pParent);
}

CctControl_WriteTrackBreak::CctControl_WriteTrackBreak(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_WRITETRACKBREAK);
    InitLockProperties("");

    _dock = dkAction;

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_WriteTrackBreak::OnSessionEvent);
}

CctControl_WriteTrackBreak::~CctControl_WriteTrackBreak()
{
    UnregisterSessionEvent(seEnter, this);
}

VOID CctControl_WriteTrackBreak::OnSessionEvent(CfxControl *pSender, VOID *pParams)
{
    WAYPOINT waypoint = {0};
    waypoint.Magic = MAGIC_WAYPOINT;

    ((CctSession *)pSender)->StoreWaypoint(&waypoint, TRUE);
}

VOID CctControl_WriteTrackBreak::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
#endif
}

//*************************************************************************************************
// CctControl_ConfigureAlert

CfxControl *Create_Control_ConfigureAlert(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ConfigureAlert(pOwner, pParent);
}

CctControl_ConfigureAlert::CctControl_ConfigureAlert(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent), _alert(pOwner)
{
    InitControl(&GUID_CONTROL_CONFIGUREALERT);
    InitLockProperties("");

    _dock = dkAction;

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_ConfigureAlert::OnSessionEvent);
}

CctControl_ConfigureAlert::~CctControl_ConfigureAlert()
{
    UnregisterSessionEvent(seEnter, this);
}

VOID CctControl_ConfigureAlert::OnSessionEvent(CfxControl *pSender, VOID *pParams)
{
    CctAction_ConfigureAlert action(this);
    action.Setup(&_alert);
    ((CctSession *)pSender)->ExecuteAction(&action);
}

VOID CctControl_ConfigureAlert::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    _alert.DefineResources(F);
#endif
}

VOID CctControl_ConfigureAlert::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    _alert.DefineProperties(F);
}

VOID CctControl_ConfigureAlert::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    _alert.DefinePropertiesUI(F);
#endif
}

//*************************************************************************************************
// CctControl_TransferOnSave

CfxControl *Create_Control_TransferOnSave(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_TransferOnSave(pOwner, pParent);
}

CctControl_TransferOnSave::CctControl_TransferOnSave(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_TRANSFERONSAVE);
    InitLockProperties("");
    _enabled = TRUE;
    _dock = dkAction;
    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_TransferOnSave::OnSessionEvent);
}

CctControl_TransferOnSave::~CctControl_TransferOnSave()
{
    UnregisterSessionEvent(seEnter, this);
}

VOID CctControl_TransferOnSave::OnSessionEvent(CfxControl *pSender, VOID *pParams)
{
    CctAction_TransferOnSave action(this);
    CctSession *session = ((CctSession *)pSender);
    action.Setup(session->GetTransferOnSave(), _enabled);
    session->ExecuteAction(&action);
}

VOID CctControl_TransferOnSave::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    F.DefineValue("Enabled", dtBoolean, &_enabled, F_TRUE);
}

VOID CctControl_TransferOnSave::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Enabled", dtBoolean, &_enabled);
#endif
}

//*************************************************************************************************
// CctControl_RemoveLoopedAttributes

CfxControl *Create_Control_RemoveLoopedAttributes(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_RemoveLoopedAttributes(pOwner, pParent);
}

CctControl_RemoveLoopedAttributes::CctControl_RemoveLoopedAttributes(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent), _attributes()
{
    InitControl(&GUID_CONTROL_REMOVELOOPEDATTRIBUTES);
    InitLockProperties("");
    _dock = dkAction;
    _lowestAttributeCount = (UINT)-1;
    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_RemoveLoopedAttributes::OnSessionEvent);
}

CctControl_RemoveLoopedAttributes::~CctControl_RemoveLoopedAttributes()
{
    UnregisterSessionEvent(seEnter, this);
}

VOID CctControl_RemoveLoopedAttributes::OnSessionEvent(CfxControl *pSender, VOID *pParams)
{
    CctSession *session = ((CctSession *)pSender);

    _lowestAttributeCount = min(_lowestAttributeCount, session->GetCurrentAttributes()->GetCount());

    if (_lowestAttributeCount < session->GetCurrentAttributes()->GetCount())
    {
        MergeAttributeRange(&_attributes, session->GetCurrentAttributes(), _lowestAttributeCount, session->GetCurrentAttributes()->GetCount()-1);
        session->GetCurrentSighting()->RemoveAttributesFromIndex(_lowestAttributeCount);
    }

    if (_attributes.GetCount() > 0)
    {
        CctAction_MergeAttribute action(this);
        for (UINT i=0; i<_attributes.GetCount(); i++)
        {
            ATTRIBUTE *attribute = _attributes.GetPtr(i);
            action.SetupAdd(_id, attribute->ElementId, (FXDATATYPE)attribute->Value->Type, attribute->Value->Data, attribute->ValueId);
        }
        session->ExecuteAction(&action);
    }
}

VOID CctControl_RemoveLoopedAttributes::DefineState(CfxFiler &F)
{
    F.DefineValue("LowestAttributeCount", dtInt32, &_lowestAttributeCount);
    DefineAttributes(F, &_attributes);
}

VOID CctControl_RemoveLoopedAttributes::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
}

VOID CctControl_RemoveLoopedAttributes::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
#endif
}
