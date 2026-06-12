#include "fxGPS.h"
#include "fxTranslate.h"
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

    _accuracyEnabled = false;
    _dock = ControlDock::Action;
    InitGpsAccuracy(&_sightingAccuracy);
    _sightingFixCount = 1;
    InitGpsAccuracy(&_waypointAccuracy);
    _waypointTimer = 0;

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_ConfigureGPS::OnSessionEvent);
}

CctControl_ConfigureGPS::~CctControl_ConfigureGPS()
{
    UnregisterSessionEvent(seEnter, this);
}

void CctControl_ConfigureGPS::OnSessionEvent(CfxControl *pSender, void *pParams)
{
    CctAction_ConfigureGPS action(this);
    action.Setup(_accuracyEnabled, &_sightingAccuracy, _sightingFixCount, &_waypointAccuracy, _waypointTimer);
    ((CctSession *)pSender)->ExecuteAction(&action);
}

void CctControl_ConfigureGPS::DefineProperties(CfxFiler &F)
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

    // Retained for resource file backward compatibility
    bool useRangeFinderForAltitudeDummy = false;
    F.DefineValue("UseRangeFinderForAltitude", dtBoolean, &useRangeFinderForAltitudeDummy);

    if (F.IsReader())
    {
        if (_version < CONFIGURE_GPS_VERSION)
        {
            _accuracyEnabled = (_sightingAccuracy.DilutionOfPrecision < 49.0) ||
                               (_waypointAccuracy.DilutionOfPrecision < 49.0);
        }
    }
}

void CctControl_ConfigureGPS::DefinePropertiesUI(CfxFilerUI &F)
{
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

    _dock = ControlDock::Action;
    _required = true;
    _autoConnect = true;
    _connected = false;

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

void CctControl_SnapGPS::OnSessionCanNext(CfxControl *pSender, void *pParams)
{    
    CctSession *session = (CctSession *)pSender; 
    CfxEngine *engine = GetEngine(this);
    FXGPS_POSITION *gps = nullptr;
    bool fixGood = false;

    if (session->IsEditing())
    {
        gps = &_gps;
        fixGood = true;
    }
    else
    {
        gps = &engine->GetGPS()->Position;    
        fixGood = TestGPS(engine->GetGPS(), session->GetSightingAccuracy());
    }

    // Block "Next" if the accuracy doesn't meet the requirements
    if (!fixGood && _required)
    {
        session->ShowMessage(fxTranslate("GPS not acquired"));
        *((bool *)pParams) = false;
    }
    else if (session->GetResourceHeader()->GpsTimeSync && !engine->GetGPS()->TimeInSync)
    {
        session->ShowMessage(fxTranslate("Time not found"));
        *((bool *)pParams) = false;
    }
    else
    {
        _gps = *gps;
    }
}

void CctControl_SnapGPS::OnSessionNext(CfxControl *pSender, void *pParams)
{
    // Do nothing if we're editing
    CctSession *session = (CctSession *)pSender; 

    // Save the GPS position to the sighting
    if (TestGPS(&_gps, session->GetSightingAccuracy()))
    {
        CctAction_MergeAttribute action(this);
        bool useAdd = false;

        if (!IsNullXGuid(&_latId))      // Latitude
        {
            action.SetupAdd(_id, _latId, dtDouble, &_gps.Latitude);
            useAdd = true;
        }
        
        if (!IsNullXGuid(&_lonId))      // Longitude
        {
            action.SetupAdd(_id, _lonId, dtDouble, &_gps.Longitude);
            useAdd = true;
        }

        if (!IsNullXGuid(&_altId))      // Altitude
        {
            action.SetupAdd(_id, _altId, dtDouble, &_gps.Altitude);
            useAdd = true;
        }

        if (!IsNullXGuid(&_accId))      // Accuracy
        {
            action.SetupAdd(_id, _accId, dtDouble, &_gps.Accuracy);
            useAdd = true;
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

void CctControl_SnapGPS::DefineResources(CfxFilerResource &F)
{
}

void CctControl_SnapGPS::DefineState(CfxFiler &F)
{
    F.DefineGPS(&_gps);
}

void CctControl_SnapGPS::DefineProperties(CfxFiler &F)
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
        _connected = true;
        GetEngine(this)->LockGPS();
    }
}

void CctControl_SnapGPS::DefinePropertiesUI(CfxFilerUI &F)
{
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

    _dock = ControlDock::Action;
    _required = true;

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

void CctControl_SnapLastGPS::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    CctSession *session = (CctSession *)pSender; 
    bool fixGood = TestGPS(&_gps, session->GetSightingAccuracy());

    // Block "Next" if the accuracy doesn't meet the requirements
    if (!fixGood && _required)
    {
        session->ShowMessage(fxTranslate("GPS not acquired"));
        *((bool *)pParams) = false;
    }
}

void CctControl_SnapLastGPS::OnSessionEnter(CfxControl *pSender, void *pParams)
{
    // Do nothing if we're editing
    CctSession *session = (CctSession *)pSender; 
    CfxEngine *engine = GetEngine(this);
    FXGPS_POSITION *gps = nullptr;
    bool fixGood = false;

    if (session->IsEditing())
    {
        gps = &_gps;
        fixGood = true;
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
        bool useAdd = false;

        if (!IsNullXGuid(&_latId))      // Latitude
        {
            action.SetupAdd(_id, _latId, dtDouble, &_gps.Latitude);
            useAdd = true;
        }
        
        if (!IsNullXGuid(&_lonId))      // Longitude
        {
            action.SetupAdd(_id, _lonId, dtDouble, &_gps.Longitude);
            useAdd = true;
        }

        if (!IsNullXGuid(&_altId))      // Altitude
        {
            action.SetupAdd(_id, _altId, dtDouble, &_gps.Altitude);
            useAdd = true;
        }

        if (!IsNullXGuid(&_accId))      // Accuracy
        {
            action.SetupAdd(_id, _accId, dtDouble, &_gps.Accuracy);
            useAdd = true;
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

void CctControl_SnapLastGPS::DefineResources(CfxFilerResource &F)
{
}

void CctControl_SnapLastGPS::DefineState(CfxFiler &F)
{
    F.DefineGPS(&_gps);
}

void CctControl_SnapLastGPS::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    F.DefineValue("Required", dtBoolean, &_required);
    F.DefineXOBJECT("LatitudeId", &_latId);
    F.DefineXOBJECT("LongitudeId", &_lonId);
    F.DefineXOBJECT("AltitudeId", &_altId);
    F.DefineXOBJECT("AccuracyId", &_accId);
}

void CctControl_SnapLastGPS::DefinePropertiesUI(CfxFilerUI &F)
{
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

    _dock = ControlDock::Action;
    _connectedGPS = false;

    _dateTime = 0;
    InitXGuid(&_dateId);
    InitXGuid(&_timeId);

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_SnapDateTime::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_SnapDateTime::OnSessionNext);

    if (IsLive() && GetEngine(this)->GetGPSTimeSync()) 
    {
        _connectedGPS = true;
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

void CctControl_SnapDateTime::OnSessionCanNext(CfxControl *pSender, void *pParams)
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

    session->ShowMessage(fxTranslate("GPS needed for time"));
    *((bool *)pParams) = false;
}

void CctControl_SnapDateTime::OnSessionNext(CfxControl *pSender, void *pParams)
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

void CctControl_SnapDateTime::DefineResources(CfxFilerResource &F)
{
}

void CctControl_SnapDateTime::DefineState(CfxFiler &F)
{
    F.DefineValue("SnapDateTime", dtDateTime, &_dateTime);
}

void CctControl_SnapDateTime::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineXOBJECT("DateId", &_dateId);
    F.DefineXOBJECT("TimeId", &_timeId);
}

void CctControl_SnapDateTime::DefinePropertiesUI(CfxFilerUI &F)
{
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

    _dock = ControlDock::Action;
    InitXGuid(&_elementId);
    _value = 0;
    _unique = false;
    _stamp = false;
    _updateTag = false;
    _appId = false;
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

void CctControl_AddAttribute::OnSessionEnter(CfxControl *pSender, void *pParams)
{
    _empty = true;

    if (IsNullXGuid(&_elementId)) return;

    CctAction_MergeAttribute action(this);

    char *temp = (char *)MEM_ALLOC(64 * 3 + (_value ? strlen(_value) : 0));
    char *idString = nullptr;

    if (_value)
    {
        strcat(temp, _value);
    }

    if (_appId)
    {
        idString = AllocGuidName(&GetSession(this)->GetResourceHeader()->SequenceId, nullptr);
        strcat(temp, idString);
        MEM_FREE(idString);
    }

    if (_unique)
    {
        GUID id = {0};
        GetHost(this)->CreateGuid(&id);
        idString = AllocGuidName(&id, nullptr);
        strcat(temp, idString);
        MEM_FREE(idString);
    }

    if (_stamp)
    {
        idString = AllocGuidName(&GetSession(this)->GetResourceHeader()->StampId, nullptr);
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
        _empty = false;
        action.SetupAdd(_id, _elementId, dtPText, &temp);
    }
    else
    {
        action.SetupAdd(_id, _elementId);
    }

    MEM_FREE(temp);

    ((CctSession *)pSender)->ExecuteAction(&action);
}

void CctControl_AddAttribute::OnSessionNext(CfxControl *pSender, void *pParams)
{
    if (IsNullXGuid(&_targetScreenOnEmptyId)) return;

    if (_empty)
    {
        CctAction_InsertScreen action(this);
        action.Setup(_targetScreenOnEmptyId);
        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

void CctControl_AddAttribute::DefineResources(CfxFilerResource &F)
{
}

void CctControl_AddAttribute::DefineProperties(CfxFiler &F)
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

void CctControl_AddAttribute::DefinePropertiesUI(CfxFilerUI &F)
{
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

    _dock = ControlDock::Action;
    InitXGuid(&_elementId);

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_AddUserName::OnSessionEvent);
}

CctControl_AddUserName::~CctControl_AddUserName()
{
    UnregisterSessionEvent(seEnter, this);
}

void CctControl_AddUserName::OnSessionEvent(CfxControl *pSender, void *pParams)
{
    if (IsNullXGuid(&_elementId)) return;

    char *userName = GetHost(this)->GetDeviceName();
    char *value = nullptr;
    Type_DataFromText(userName, dtPText, &value);

    CctAction_MergeAttribute action(this);
    action.SetupAdd(_id, _elementId, dtPText, &value);
    ((CctSession *)pSender)->ExecuteAction(&action);

    MEM_FREE(userName);
    Type_FreeAndNil((void **)&value);
}

void CctControl_AddUserName::DefineResources(CfxFilerResource &F)
{
}

void CctControl_AddUserName::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineXOBJECT("ElementId", &_elementId);

    // Null id is automatically assigned to static


    HackFixResultElementLockProperties(&_lockProperties);
}

void CctControl_AddUserName::DefinePropertiesUI(CfxFilerUI &F)
{
}

//*************************************************************************************************
// CctControl_ConfigureRangeFinder (no-op stub for resource file backward compatibility)

CfxControl *Create_Control_ConfigureRangeFinder(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ConfigureRangeFinder(pOwner, pParent);
}

CctControl_ConfigureRangeFinder::CctControl_ConfigureRangeFinder(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_CONFIGURERANGEFINDER);
    _dock = ControlDock::Action;
    _state = true;
}

void CctControl_ConfigureRangeFinder::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    F.DefineValue("State", dtBoolean, &_state);
}

void CctControl_ConfigureRangeFinder::DefinePropertiesUI(CfxFilerUI &F)
{
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

    _dock = ControlDock::Action;
    InitXGuid(&_save1Target);
    InitXGuid(&_save2Target);

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_ConfigureSaveTargets::OnSessionEvent);
}

CctControl_ConfigureSaveTargets::~CctControl_ConfigureSaveTargets()
{
    UnregisterSessionEvent(seEnter, this);
}

void CctControl_ConfigureSaveTargets::OnSessionEvent(CfxControl *pSender, void *pParams)
{
    CctAction_ConfigureSaveTargets action(this);
    action.Setup(_save1Target, _save2Target);
    ((CctSession *)pSender)->ExecuteAction(&action);
}

void CctControl_ConfigureSaveTargets::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    F.DefineXOBJECT("Save1Target", &_save1Target);
    F.DefineXOBJECT("Save2Target", &_save2Target);
}

void CctControl_ConfigureSaveTargets::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_ConfigureSaveTargets::DefineResources(CfxFilerResource &F)
{
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

    _dock = ControlDock::Action;
    _resetKey = nullptr;
    _immediate = true;

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_ResetStateKey::OnSessionEventEnter);
    RegisterSessionEvent(seAfterNext, this, (NotifyMethod)&CctControl_ResetStateKey::OnSessionEventLeave);
}

CctControl_ResetStateKey::~CctControl_ResetStateKey()
{
    UnregisterSessionEvent(seEnter, this);
    UnregisterSessionEvent(seAfterNext, this);

    MEM_FREE(_resetKey);
    _resetKey = nullptr;
}

void CctControl_ResetStateKey::Execute(CctSession *pSession)
{
    double value;

    GLOBALVALUE *globalValue = pSession->GetGlobalValue(_resetKey);
    value = globalValue != nullptr ? globalValue->Value : 1;

    CctAction_SetGlobalValue action(this);
    action.Setup(_resetKey, value + 1);
    pSession->ExecuteAction(&action);
}

void CctControl_ResetStateKey::OnSessionEventEnter(CfxControl *pSender, void *pParams)
{
    if (!_resetKey || !_immediate) return;

    Execute((CctSession *)pSender);
}

void CctControl_ResetStateKey::OnSessionEventLeave(CfxControl *pSender, void *pParams)
{
    if (!_resetKey || _immediate) return;

    Execute((CctSession *)pSender);
}

void CctControl_ResetStateKey::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    F.DefineValue("Immediate", dtBoolean, &_immediate, F_TRUE);
    F.DefineValue("ResetKey", dtPText, &_resetKey);
}

void CctControl_ResetStateKey::DefinePropertiesUI(CfxFilerUI &F)
{
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

    _dock = ControlDock::Action;
    InitXGuid(&_titleElementId);
    _titlePrefix = nullptr;

    RegisterSessionEvent(seAfterNext, this, (NotifyMethod)&CctControl_SetPendingGoto::OnSessionEvent);
}

CctControl_SetPendingGoto::~CctControl_SetPendingGoto()
{
    UnregisterSessionEvent(seAfterNext, this);
    MEM_FREE(_titlePrefix);
}

void CctControl_SetPendingGoto::OnSessionEvent(CfxControl *pSender, void *pParams)
{
    if (IsNullXGuid(&_titleElementId) && (_titlePrefix == nullptr)) return;

    QString valueText;

    if (!IsNullXGuid(&_titleElementId))
    {
        ATTRIBUTE *attribute = GetSession(this)->GetCurrentSighting()->FindAttribute(&_titleElementId);

        if (attribute)
        {
            GetAttributeText(this, attribute, valueText);
        }
    }

    QByteArray valueUtf8 = valueText.toUtf8();
    int titleLen = valueUtf8.size() + (_titlePrefix ? strlen(_titlePrefix) : 0) + 1;
    char *title = (char *)MEM_ALLOC(titleLen);
    memset(title, 0, titleLen);

    if (_titlePrefix)
    {
        strcpy(title, _titlePrefix);
    }

    if (!valueText.isEmpty())
    {
        strcat(title, valueUtf8.constData());
    }

    if (strlen(title) > 0)
    {
        CctAction_SetPendingGoto action(this);
        action.Setup(title);
        ((CctSession *)pSender)->ExecuteAction(&action);
    }

    MEM_FREE(title);
}

void CctControl_SetPendingGoto::DefineResources(CfxFilerResource &F)
{
}

void CctControl_SetPendingGoto::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineXOBJECT("TitleElementId", &_titleElementId);
    F.DefineValue("TitlePrefix", dtPText, &_titlePrefix);
}

void CctControl_SetPendingGoto::DefinePropertiesUI(CfxFilerUI &F)
{
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

    _dock = ControlDock::Action;
    _title = nullptr;
    _latitude = _longitude = 0;

    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_AddCustomGoto::OnSessionEvent);
}

CctControl_AddCustomGoto::~CctControl_AddCustomGoto()
{
    UnregisterSessionEvent(seNext, this);
    MEM_FREE(_title);
}

void CctControl_AddCustomGoto::OnSessionEvent(CfxControl *pSender, void *pParams)
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

void CctControl_AddCustomGoto::DefineResources(CfxFilerResource &F)
{
}

void CctControl_AddCustomGoto::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Title", dtPText, &_title);
    F.DefineValue("Latitude", dtDouble, &_latitude);
    F.DefineValue("Longitude", dtDouble, &_longitude);
}

void CctControl_AddCustomGoto::DefinePropertiesUI(CfxFilerUI &F)
{
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

    _dock = ControlDock::Action;

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_WriteTrackBreak::OnSessionEvent);
}

CctControl_WriteTrackBreak::~CctControl_WriteTrackBreak()
{
    UnregisterSessionEvent(seEnter, this);
}

void CctControl_WriteTrackBreak::OnSessionEvent(CfxControl *pSender, void *pParams)
{
}

void CctControl_WriteTrackBreak::DefinePropertiesUI(CfxFilerUI &F)
{
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

    _dock = ControlDock::Action;

    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_ConfigureAlert::OnSessionEvent);
}

CctControl_ConfigureAlert::~CctControl_ConfigureAlert()
{
    UnregisterSessionEvent(seEnter, this);
}

void CctControl_ConfigureAlert::OnSessionEvent(CfxControl *pSender, void *pParams)
{
    CctAction_ConfigureAlert action(this);
    action.Setup(&_alert);
    ((CctSession *)pSender)->ExecuteAction(&action);
}

void CctControl_ConfigureAlert::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ConfigureAlert::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    _alert.DefineProperties(F);
}

void CctControl_ConfigureAlert::DefinePropertiesUI(CfxFilerUI &F)
{
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
    _enabled = true;
    _dock = ControlDock::Action;
    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_TransferOnSave::OnSessionEvent);
}

CctControl_TransferOnSave::~CctControl_TransferOnSave()
{
    UnregisterSessionEvent(seEnter, this);
}

void CctControl_TransferOnSave::OnSessionEvent(CfxControl *pSender, void *pParams)
{
    CctAction_TransferOnSave action(this);
    CctSession *session = ((CctSession *)pSender);
    action.Setup(session->GetTransferOnSave(), _enabled);
    session->ExecuteAction(&action);
}

void CctControl_TransferOnSave::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    F.DefineValue("Enabled", dtBoolean, &_enabled, F_TRUE);
}

void CctControl_TransferOnSave::DefinePropertiesUI(CfxFilerUI &F)
{
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
    _dock = ControlDock::Action;
    _lowestAttributeCount = (int)-1;
    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_RemoveLoopedAttributes::OnSessionEvent);
}

CctControl_RemoveLoopedAttributes::~CctControl_RemoveLoopedAttributes()
{
    UnregisterSessionEvent(seEnter, this);
}

void CctControl_RemoveLoopedAttributes::OnSessionEvent(CfxControl *pSender, void *pParams)
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
        for (int i=0; i<_attributes.GetCount(); i++)
        {
            ATTRIBUTE *attribute = _attributes.GetPtr(i);
            action.SetupAdd(_id, attribute->ElementId, (FXDATATYPE)attribute->Value->Type, attribute->Value->Data, attribute->ValueId);
        }
        session->ExecuteAction(&action);
    }
}

void CctControl_RemoveLoopedAttributes::DefineState(CfxFiler &F)
{
    F.DefineValue("LowestAttributeCount", dtInt32, &_lowestAttributeCount);
    DefineAttributes(F, &_attributes);
}

void CctControl_RemoveLoopedAttributes::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
}

void CctControl_RemoveLoopedAttributes::DefinePropertiesUI(CfxFilerUI &F)
{
}
