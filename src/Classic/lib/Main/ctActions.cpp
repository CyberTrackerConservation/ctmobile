#include "fxControl.h"
#include "fxAction.h"
#include "fxUtils.h"
#include "ctActions.h"
#include "ctElement.h"
#include "ctElementList.h"

//*************************************************************************************************
// CctAction_AppendScreen

CfxAction *Create_Action_AppendScreen(CfxPersistent *pOwner)
{
    return new CctAction_AppendScreen(pOwner);
}

CctAction_AppendScreen::CctAction_AppendScreen(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_APPENDSCREEN);
   
    _index = -1;
    InitXGuid(&_screenId);
}

void CctAction_AppendScreen::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);
    F.DefineXOBJECT("ScreenId", &_screenId);
}

void CctAction_AppendScreen::DefineState(CfxFiler &F)
{
    F.DefineValue("Index", dtInt32, &_index);
}

void CctAction_AppendScreen::DefineResources(CfxFilerResource &F)
{
}

void CctAction_AppendScreen::Execute(CfxPersistent *pSender)
{
    if (IsNullXGuid(&_screenId)) return;

    _index = ((CctSession *)pSender)->AppendScreen(_screenId);
}

void CctAction_AppendScreen::Rollback(CfxPersistent *pSender)
{
    if (IsNullXGuid(&_screenId)) return;

    ((CctSession *)pSender)->DeleteScreen(_index);
}

//*************************************************************************************************
// CctAction_InsertScreen

CfxAction *Create_Action_InsertScreen(CfxPersistent *pOwner)
{
    return new CctAction_InsertScreen(pOwner);
}

CctAction_InsertScreen::CctAction_InsertScreen(CfxPersistent *pOwner): CctAction_AppendScreen(pOwner)
{
    InitAction(&GUID_ACTION_INSERTSCREEN);
}

void CctAction_InsertScreen::Execute(CfxPersistent *pSender)
{
    if (IsNullXGuid(&_screenId)) return;

    _index = ((CctSession *)pSender)->InsertScreen(_screenId);
}

//*************************************************************************************************
// CctAction_MergeAttribute

CfxAction *Create_Action_MergeAttribute(CfxPersistent *pOwner)
{
    return new CctAction_MergeAttribute(pOwner);
}

CctAction_MergeAttribute::CctAction_MergeAttribute(CfxPersistent *pOwner): CfxAction(pOwner), _oldAttributes(), _newAttributes()
{
    InitAction(&GUID_ACTION_MERGEATTRIBUTE);
    _controlId = 0;
}

CctAction_MergeAttribute::~CctAction_MergeAttribute()
{
    ::ClearAttributes(&_oldAttributes);
    ::ClearAttributes(&_newAttributes);
}

void CctAction_MergeAttribute::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);
    ::DefineAttributes(F, &_newAttributes);
}

void CctAction_MergeAttribute::DefineState(CfxFiler &F)
{
    F.DefineValue("ControlId", dtInt32, &_controlId);
    ::DefineAttributes(F, &_oldAttributes);
}

void CctAction_MergeAttribute::DefineResources(CfxFilerResource &F)
{
}

void CctAction_MergeAttribute::SetupAdd(int ControlId, XGUID ElementId, FXDATATYPE Type, void *pData, XGUID ValueId)
{
    // Must have valid control id
    FX_ASSERT(ControlId != 0);

    // All control ids must be the same
    FX_ASSERT((_controlId == 0) || (_controlId == ControlId));
    
    // Must have good elementId
    FX_ASSERT(!IsNullXGuid(&ElementId));

    // Set our internal ControlId
    _controlId = ControlId;

    // Create an attribute and append to our list
    ATTRIBUTE attribute = {0};
    attribute.ControlId = ControlId;
    attribute.ElementId = ElementId;
    attribute.ValueId = ValueId;
    attribute.Value = Type_CreateVariant(Type, pData);
    _newAttributes.Add(attribute);
}

void CctAction_MergeAttribute::Execute(CfxPersistent *pSender)
{
    FX_ASSERT(_controlId != 0);
    ClearAttributes(&_oldAttributes);

    ((CctSession *)pSender)->MergeAttributes(_controlId, &_newAttributes, &_oldAttributes);
}

void CctAction_MergeAttribute::Rollback(CfxPersistent *pSender)
{
    FX_ASSERT(_controlId != 0);

    ((CctSession *)pSender)->MergeAttributes(_controlId, &_oldAttributes);
}

//*************************************************************************************************
// CctAction_ConfigureGPS

CfxAction *Create_Action_ConfigureGPS(CfxPersistent *pOwner)
{
    return new CctAction_ConfigureGPS(pOwner);
}

CctAction_ConfigureGPS::CctAction_ConfigureGPS(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_CONFIGUREGPS);

    _accuracyEnabled = false;

    InitGpsAccuracy(&_sightingAccuracy);
    _sightingFixCount = 1;
    InitGpsAccuracy(&_waypointAccuracy);
    _waypointTimer = 0;
    InitGpsAccuracy(&_lastSightingAccuracy);
    InitGpsAccuracy(&_lastWaypointAccuracy);
    _lastSightingFixCount = _lastWaypointTimer = 0;
}

void CctAction_ConfigureGPS::Setup(bool AccuracyEnabled, FXGPS_ACCURACY *pSightingAccuracy, int SightingFixCount, FXGPS_ACCURACY *pWaypointAccuracy, int WaypointTimer)
{
    _accuracyEnabled = AccuracyEnabled;
    _sightingAccuracy = *pSightingAccuracy;
    _sightingFixCount = SightingFixCount;
    _waypointAccuracy = *pWaypointAccuracy;
    _waypointTimer = WaypointTimer;
}

void CctAction_ConfigureGPS::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);

    F.DefineValue("AccuracyEnabled", dtBoolean, &_accuracyEnabled);
    F.DefineValue("SightingAccuracy", dtDouble, &_sightingAccuracy.DilutionOfPrecision);
    F.DefineValue("SightingAccuracyMaxSpeed", dtDouble, &_sightingAccuracy.MaximumSpeed);
    F.DefineValue("SightingFixCount", dtInt32,  &_sightingFixCount);
    F.DefineValue("WaypointAccuracy", dtDouble, &_waypointAccuracy.DilutionOfPrecision);
    F.DefineValue("WaypointAccuracyMaxSpeed", dtDouble, &_waypointAccuracy.MaximumSpeed);
    F.DefineValue("WaypointTimer", dtInt32, &_waypointTimer);

    // Retained for resource file backward compatibility
    bool useRangeFinderForAltitudeDummy = false;
    F.DefineValue("UseRangeFinderForAltitude", dtBoolean, &useRangeFinderForAltitudeDummy);
}

void CctAction_ConfigureGPS::DefineState(CfxFiler &F)
{
    F.DefineValue("LastSightingAccuracy", dtDouble, &_lastSightingAccuracy.DilutionOfPrecision);
    F.DefineValue("LastSightingAccuracyMaxSpeed", dtDouble, &_lastSightingAccuracy.MaximumSpeed);
    F.DefineValue("LastSightingFixCount", dtInt32,  &_lastSightingFixCount);
    F.DefineValue("LastWaypointAccuracy", dtDouble, &_lastWaypointAccuracy.DilutionOfPrecision);
    F.DefineValue("LastWaypointAccuracyMaxSpeed", dtDouble, &_lastWaypointAccuracy.MaximumSpeed);
    F.DefineValue("LastWaypointTimer", dtInt32, &_lastWaypointTimer);

    // Retained for state file backward compatibility
    bool lastUseRangeFinderForAltitudeDummy = false;
    F.DefineValue("LastUseRangeFinderForAltitude", dtBoolean, &lastUseRangeFinderForAltitudeDummy);
}

void CctAction_ConfigureGPS::Execute(CfxPersistent *pSender)
{
    CctSession *session = (CctSession *)pSender;

    _lastSightingAccuracy = *session->GetSightingAccuracy();
    _lastSightingFixCount = session->GetSightingFixCount();
    _lastWaypointTimer = GetHost(this)->GetTrackTimer();

    session->SetSightingFixCount(_sightingFixCount);
    GetHost(this)->SetTrackTimer(_waypointTimer);

    if (_accuracyEnabled)
    {
        session->SetSightingAccuracy(&_sightingAccuracy);
    }
}

void CctAction_ConfigureGPS::Rollback(CfxPersistent *pSender)
{
    CctSession *session = (CctSession *)pSender;

    if (!session->IsSaving())
    {
        session->SetSightingFixCount(_lastSightingFixCount);
        GetHost(this)->SetTrackTimer(_lastWaypointTimer);

        if (_accuracyEnabled)
        {
            session->SetSightingAccuracy(&_lastSightingAccuracy);
        }
    }
}

//*************************************************************************************************
// CctAction_ConfigureRangeFinder (no-op stub for resource file backward compatibility)

CfxAction *Create_Action_ConfigureRangeFinder(CfxPersistent *pOwner)
{
    return new CctAction_ConfigureRangeFinder(pOwner);
}

CctAction_ConfigureRangeFinder::CctAction_ConfigureRangeFinder(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_CONFIGURERANGEFINDER);
    _state = _lastState = false;
}

void CctAction_ConfigureRangeFinder::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);
    F.DefineValue("State", dtBoolean, &_state);
}

void CctAction_ConfigureRangeFinder::DefineState(CfxFiler &F)
{
    F.DefineValue("LastState", dtBoolean, &_lastState);
}

//*************************************************************************************************
// CctAction_SnapGPS

CfxAction *Create_Action_SnapGPS(CfxPersistent *pOwner)
{
    return new CctAction_SnapGPS(pOwner);
}

CctAction_SnapGPS::CctAction_SnapGPS(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_SNAPGPS);

    memset(&_gps, 0, sizeof(_gps));
    memset(&_lastGps, 0, sizeof(_lastGps));
}

void CctAction_SnapGPS::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);
    F.DefineGPS(&_gps);
}

void CctAction_SnapGPS::DefineState(CfxFiler &F)
{
    CfxAction::DefineState(F);
    F.DefineGPS(&_lastGps);
}

void CctAction_SnapGPS::Setup(FXGPS_POSITION *pGPS)
{
    memcpy(&_gps, pGPS, sizeof(_gps));
}

void CctAction_SnapGPS::Execute(CfxPersistent *pSender)
{
    FXGPS_POSITION *sightingGPS = ((CctSession *)pSender)->GetCurrentSighting()->GetGPS();

    memcpy(&_lastGps, sightingGPS, sizeof(FXGPS_POSITION));
    memcpy(sightingGPS, &_gps, sizeof(FXGPS_POSITION)); 
}

void CctAction_SnapGPS::Rollback(CfxPersistent *pSender)
{
    FXGPS_POSITION *sightingGPS = ((CctSession *)pSender)->GetCurrentSighting()->GetGPS();
    memcpy(sightingGPS, &_lastGps, sizeof(FXGPS_POSITION)); 
}

//*************************************************************************************************
// CctAction_SnapDateTime

CfxAction *Create_Action_SnapDateTime(CfxPersistent *pOwner)
{
    return new CctAction_SnapDateTime(pOwner);
}

CctAction_SnapDateTime::CctAction_SnapDateTime(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_SNAPDATETIME);
    _dateTime = _lastDateTime = 0;
    _mergeAttributes = new CctAction_MergeAttribute(pOwner);
}

CctAction_SnapDateTime::~CctAction_SnapDateTime()
{
    delete _mergeAttributes;
    _mergeAttributes = nullptr;
}

void CctAction_SnapDateTime::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);
    F.DefineValue("DateTime", dtDateTime, &_dateTime);
    _mergeAttributes->DefineProperties(F);
}

void CctAction_SnapDateTime::DefineState(CfxFiler &F)
{
    CfxAction::DefineState(F);
    F.DefineValue("DateTime", dtDateTime, &_lastDateTime);
    _mergeAttributes->DefineState(F);
}

void CctAction_SnapDateTime::Setup(int ControlId, FXDATETIME *pDateTime, XGUID DateElementId, XGUID TimeElementId)
{
    _dateTime = EncodeDateTime(pDateTime);
    double rawDate = (int)_dateTime;
    double rawTime = _dateTime - rawDate;

    if (!IsNullXGuid(&DateElementId))
    {
        _mergeAttributes->SetupAdd(ControlId, DateElementId, dtDate, &rawDate);
    }

    if (!IsNullXGuid(&TimeElementId))
    {
        _mergeAttributes->SetupAdd(ControlId, TimeElementId, dtTime, &rawTime);
    }
}

void CctAction_SnapDateTime::Execute(CfxPersistent *pSender)
{
    if (_mergeAttributes->GetAttributeCount() > 0)
    {
        _mergeAttributes->Execute(pSender);
    }
    else
    {
        FXDATETIME *dateTime = ((CctSession *)pSender)->GetCurrentSighting()->GetDateTime();
        _lastDateTime = EncodeDateTime(dateTime);
        DecodeDateTime(_dateTime, dateTime);
    }
}

void CctAction_SnapDateTime::Rollback(CfxPersistent *pSender)
{
    if (_mergeAttributes->GetAttributeCount() > 0)
    {
        _mergeAttributes->Rollback(pSender);
    }
    else
    {
        FXDATETIME *dateTime = ((CctSession *)pSender)->GetCurrentSighting()->GetDateTime();
        DecodeDateTime(_lastDateTime, dateTime);
    }
}

//*************************************************************************************************
// CctAction_ConfigureSaveTargets

CfxAction *Create_Action_ConfigureSaveTargets(CfxPersistent *pOwner)
{
    return new CctAction_ConfigureSaveTargets(pOwner);
}

CctAction_ConfigureSaveTargets::CctAction_ConfigureSaveTargets(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_CONFIGURESAVETARGETS);
    InitXGuid(&_lastSave1Target);
    InitXGuid(&_lastSave2Target);
    InitXGuid(&_save1Target);
    InitXGuid(&_save2Target);
}

void CctAction_ConfigureSaveTargets::Setup(XGUID Save1Target, XGUID Save2Target)
{
    _save1Target = Save1Target;
    _save2Target = Save2Target;
}

void CctAction_ConfigureSaveTargets::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);
    F.DefineXOBJECT("Save1Target", &_save1Target);
    F.DefineXOBJECT("Save2Target", &_save2Target);
}

void CctAction_ConfigureSaveTargets::DefineState(CfxFiler &F)
{
    F.DefineXOBJECT("LastSave1Target", &_lastSave1Target);
    F.DefineXOBJECT("LastSave2Target", &_lastSave2Target);
}

void CctAction_ConfigureSaveTargets::DefineResources(CfxFilerResource &F)
{
}

void CctAction_ConfigureSaveTargets::Execute(CfxPersistent *pSender)
{
    CctSession *session = (CctSession *)pSender;
    
    session->GetDefaultSaveTargets(&_lastSave1Target, &_lastSave2Target);

    XGUID st1 = IsNullXGuid(&_save1Target) ? _lastSave1Target : _save1Target;
    XGUID st2 = IsNullXGuid(&_save2Target) ? _lastSave2Target : _save2Target;
    
    session->SetDefaultSaveTargets(st1, st2);
}

void CctAction_ConfigureSaveTargets::Rollback(CfxPersistent *pSender)
{
    CctSession *session = (CctSession *)pSender;

    session->SetDefaultSaveTargets(_lastSave1Target, _lastSave2Target);
}

//*************************************************************************************************
// CctAction_SetGlobalValue

CfxAction *Create_Action_SetGlobalValue(CfxPersistent *pOwner)
{
    return new CctAction_SetGlobalValue(pOwner);
}

CctAction_SetGlobalValue::CctAction_SetGlobalValue(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_SETGLOBALVALUE);
    memset(&_oldValue, 0, sizeof(_oldValue));
    memset(&_newValue, 0, sizeof(_newValue));
}

void CctAction_SetGlobalValue::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);

    F.DefineValue("NewName", dtText, &_newValue.Name);
    F.DefineValue("NewValue", dtDouble, &_newValue.Value);
}

void CctAction_SetGlobalValue::DefineState(CfxFiler &F)
{
    F.DefineValue("OldName", dtText, &_oldValue.Name);
    F.DefineValue("OldValue", dtDouble, &_oldValue.Value);
}

void CctAction_SetGlobalValue::Setup(char *pName, double Value)
{
    // Must have valid name
    FX_ASSERT(pName != nullptr);

    strncpy(_newValue.Name, pName, sizeof(_newValue.Name)-1);
    _newValue.Value = Value;
}

void CctAction_SetGlobalValue::Execute(CfxPersistent *pSender)
{
    ((CctSession *)pSender)->SetGlobalValue(pSender, _newValue.Name, _newValue.Value);
}

void CctAction_SetGlobalValue::Rollback(CfxPersistent *pSender)
{
    ((CctSession *)pSender)->SetGlobalValue(this, _oldValue.Name, _oldValue.Value);
}

//*************************************************************************************************
// CctAction_SetPendingGoto

CfxAction *Create_Action_SetPendingGoto(CfxPersistent *pOwner)
{
    return new CctAction_SetPendingGoto(pOwner);
}

CctAction_SetPendingGoto::CctAction_SetPendingGoto(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_SETPENDINGGOTO);

    _oldTitle = _newTitle = nullptr;
}

CctAction_SetPendingGoto::~CctAction_SetPendingGoto()
{
    MEM_FREE(_oldTitle);
    MEM_FREE(_newTitle);
}

void CctAction_SetPendingGoto::Setup(char *pTitle)
{
    MEM_FREE(_newTitle);
    _newTitle = AllocString(pTitle);
}

void CctAction_SetPendingGoto::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);

    F.DefineValue("NewTitle", dtPText, &_newTitle);
}

void CctAction_SetPendingGoto::DefineState(CfxFiler &F)
{
    F.DefineValue("OldTitle", dtPText, &_oldTitle);
}

void CctAction_SetPendingGoto::Execute(CfxPersistent *pSender)
{
    CctSession *session = (CctSession *)pSender;

    MEM_FREE(_oldTitle);
    _oldTitle = AllocString(session->GetPendingGotoTitle());

    session->SetPendingGotoTitle(_newTitle);
}

void CctAction_SetPendingGoto::Rollback(CfxPersistent *pSender)
{
    ((CctSession *)pSender)->SetPendingGotoTitle(_oldTitle);
}

//*************************************************************************************************
// CctAction_AddCustomGoto

CfxAction *Create_Action_AddCustomGoto(CfxPersistent *pOwner)
{
    return new CctAction_AddCustomGoto(pOwner);
}

CctAction_AddCustomGoto::CctAction_AddCustomGoto(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_ADDCUSTOMGOTO);

    _index = (int)-1;
    memset(&_goto, 0, sizeof(FXGOTO));
}

void CctAction_AddCustomGoto::Setup(FXGOTO *pGoto)
{
    memset(&_goto, 0, sizeof(FXGOTO));

    if (pGoto)
    {
        memcpy(&_goto, pGoto, sizeof(FXGOTO));
    }
}

void CctAction_AddCustomGoto::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);

    F.DefineGOTO(&_goto);
}

void CctAction_AddCustomGoto::DefineState(CfxFiler &F)
{
    F.DefineValue("Index", dtInt32, &_index);
}

void CctAction_AddCustomGoto::Execute(CfxPersistent *pSender)
{
    CctSession *session = (CctSession *)pSender;

    _index = session->AddCustomGoto(&_goto);
}

void CctAction_AddCustomGoto::Rollback(CfxPersistent *pSender)
{
    CctSession *session = (CctSession *)pSender;
    if (!session->IsSaving()) 
    {
        session->DeleteCustomGoto(_index);
    }
}

//*************************************************************************************************
// CctAction_SetElementListHistory

CfxAction *Create_Action_SetElementListHistory(CfxPersistent *pOwner)
{
    return new CctAction_SetElementListHistory(pOwner);
}

CctAction_SetElementListHistory::CctAction_SetElementListHistory(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_SETELEMENTLISTHISTORY);
   
    _controlId = 0;
    _index = 0;
    _oldValue = _newValue = false;
}

void CctAction_SetElementListHistory::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);
    F.DefineValue("ControlId", dtInt32, &_controlId);
    F.DefineValue("Index", dtInt32, &_index);
    F.DefineValue("OldValue", dtBoolean, &_oldValue);
    F.DefineValue("NewValue", dtBoolean, &_newValue);
}

void CctAction_SetElementListHistory::Execute(CfxPersistent *pSender)
{
    CctControl_ElementList *elementList = (CctControl_ElementList *)((CctSession *)pSender)->FindControl(_controlId);
    if (elementList == nullptr) return;

    elementList->SetHistoryState(_index, _newValue);
}

void CctAction_SetElementListHistory::Rollback(CfxPersistent *pSender)
{
    if (((CctSession *)pSender)->IsSaving()) return;

    CctControl_ElementList *elementList = (CctControl_ElementList *)((CctSession *)pSender)->FindControl(_controlId);
    if (elementList == nullptr) return;

    elementList->SetHistoryState(_index, _oldValue);
}

//*************************************************************************************************
// CctAction_ConfigureAlert

CfxAction *Create_Action_ConfigureAlert(CfxPersistent *pOwner)
{
    return new CctAction_ConfigureAlert(pOwner);
}

CctAction_ConfigureAlert::CctAction_ConfigureAlert(CfxPersistent *pOwner): CfxAction(pOwner), _alert(pOwner)
{
    InitAction(&GUID_ACTION_CONFIGUREALERT);
}

void CctAction_ConfigureAlert::Setup(CctAlert *pAlert)
{
    _alert.Assign(pAlert);
}

void CctAction_ConfigureAlert::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);
    _alert.DefineProperties(F);
}

void CctAction_ConfigureAlert::Execute(CfxPersistent *pSender)
{
    ((CctSession *)pSender)->GetTransferManager()->AddAlert(&_alert);
}

void CctAction_ConfigureAlert::Rollback(CfxPersistent *pSender)
{
    ((CctSession *)pSender)->GetTransferManager()->RemoveAlert(&_alert);
}

//*************************************************************************************************
// CctAction_TransferOnSave

CfxAction *Create_Action_TransferOnSave(CfxPersistent *pOwner)
{
    return new CctAction_TransferOnSave(pOwner);
}

CctAction_TransferOnSave::CctAction_TransferOnSave(CfxPersistent *pOwner): CfxAction(pOwner)
{
    InitAction(&GUID_ACTION_TRANSFERONSAVE);
    _oldValue = _newValue = false;
}

void CctAction_TransferOnSave::Setup(bool OldValue, bool NewValue)
{
    _oldValue = OldValue;
    _newValue = NewValue;
}

void CctAction_TransferOnSave::DefineState(CfxFiler &F)
{
    F.DefineValue("OldValue", dtBoolean, &_oldValue);
}

void CctAction_TransferOnSave::DefineProperties(CfxFiler &F)
{
    CfxAction::DefineProperties(F);
    F.DefineValue("NewValue", dtBoolean, &_newValue, F_FALSE);
}

void CctAction_TransferOnSave::Execute(CfxPersistent *pSender)
{
    ((CctSession *)pSender)->SetTransferOnSave(_newValue);
}

void CctAction_TransferOnSave::Rollback(CfxPersistent *pSender)
{
    ((CctSession *)pSender)->SetTransferOnSave(_oldValue);
}
