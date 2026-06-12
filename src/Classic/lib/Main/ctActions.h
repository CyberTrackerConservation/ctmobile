#pragma once
#include "pch.h"
#include "ctSession.h"

//*************************************************************************************************
//
// CctAction_AppendScreen
//
const GUID GUID_ACTION_APPENDSCREEN = {0x8cbccafe, 0xd381, 0x4283, {0xbd, 0x11, 0xe0, 0xac, 0x5a, 0xc9, 0xb4, 0x4b}};

class CctAction_AppendScreen: public CfxAction 
{
protected:
    XGUID _screenId = 0;
    int _index = 0;
public:
    CctAction_AppendScreen(CfxPersistent *pOwner);

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void Setup(XGUID Value) { _screenId = Value; }

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};
extern CfxAction *Create_Action_AppendScreen(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_InsertScreen
//
const GUID GUID_ACTION_INSERTSCREEN = {0xc09a2039, 0xc5d1, 0x4ff3, {0xbb, 0x6a, 0xcd, 0x22, 0x6f, 0x6d, 0xfc, 0x0e}};

class CctAction_InsertScreen: public CctAction_AppendScreen 
{
public:
    CctAction_InsertScreen(CfxPersistent *pOwner);
    void Execute(CfxPersistent *pSender) override;
};
extern CfxAction *Create_Action_InsertScreen(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_MergeAttribute
//
const GUID GUID_ACTION_MERGEATTRIBUTE = {0x5a771677, 0x75ab, 0x4118, {0xaf, 0xc6, 0x2, 0x59, 0x4e, 0xc3, 0xeb, 0x1c}};

class CctAction_MergeAttribute: public CfxAction 
{
protected:
    int _controlId = 0;
    ATTRIBUTES _oldAttributes, _newAttributes;
public:
    CctAction_MergeAttribute(CfxPersistent *pOwner);
    ~CctAction_MergeAttribute();

    bool IsValid()           { return _controlId != 0;           }
    int GetAttributeCount() { return _newAttributes.GetCount(); }

    void SetControlId(int Value) { _controlId = Value; }
    void SetupAdd(int ControlId, XGUID ElementId, FXDATATYPE Type = dtNull, void *pData = nullptr, XGUID ValueId = XGUID_0);

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};

extern CfxAction *Create_Action_MergeAttribute(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_ConfigureGPS
//
const GUID GUID_ACTION_CONFIGUREGPS = {0xf159ecd2, 0xa5ce, 0x441d, {0x81, 0x52, 0x45, 0xeb, 0x7d, 0xcc, 0x98, 0x9c}};

class CctAction_ConfigureGPS: public CfxAction 
{
protected:
    bool _accuracyEnabled = false;
    FXGPS_ACCURACY _sightingAccuracy, _lastSightingAccuracy;
    int _sightingFixCount = 0, _lastSightingFixCount = 0;
    FXGPS_ACCURACY _waypointAccuracy, _lastWaypointAccuracy;
    int _waypointTimer = 0, _lastWaypointTimer = 0;
public:
    CctAction_ConfigureGPS(CfxPersistent *pOwner);

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;

    void Setup(bool AccuracyEnabled, FXGPS_ACCURACY *pSightingAccuracy, int SightingFixCount, FXGPS_ACCURACY *pWaypointAccuracy, int WaypointTimer);
    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};

extern CfxAction *Create_Action_ConfigureGPS(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_SnapGPS
//
const GUID GUID_ACTION_SNAPGPS = {0x6a3c11d3, 0xebb3, 0x4c8c, {0xb4, 0x38, 0x6c, 0xfd, 0x28, 0x88, 0x19, 0xb6}};

class CctAction_SnapGPS: public CfxAction 
{
protected:
    FXGPS_POSITION _gps, _lastGps;
public:
    CctAction_SnapGPS(CfxPersistent *pOwner);

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;

    void Setup(FXGPS_POSITION *pGPS);

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};
extern CfxAction *Create_Action_SnapGPS(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_SnapDateTime
//
const GUID GUID_ACTION_SNAPDATETIME = {0xf76cb98c, 0xa677, 0x49fb, {0xae, 0xef, 0x93, 0xa2, 0xa7, 0xca, 0x65, 0xa7}};

class CctAction_SnapDateTime: public CfxAction 
{
protected:
    double _dateTime = 0.0, _lastDateTime = 0.0;
    CctAction_MergeAttribute *_mergeAttributes = nullptr;
public:
    CctAction_SnapDateTime(CfxPersistent *pOwner);
    ~CctAction_SnapDateTime();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;

    void Setup(int ControlId, FXDATETIME *pDateTime, XGUID DateElementId = XGUID_0, XGUID TimeElementId = XGUID_0);

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};
extern CfxAction *Create_Action_SnapDateTime(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_ConfigureRangeFinder (no-op stub for resource file backward compatibility)
//
const GUID GUID_ACTION_CONFIGURERANGEFINDER = {0x7bdcf6fa, 0xcf75, 0x48c0, {0xa7, 0x6d, 0xc3, 0x75, 0xc9, 0x5e, 0xb9, 0x68}};

class CctAction_ConfigureRangeFinder: public CfxAction
{
protected:
    bool _state = false, _lastState = false;
public:
    CctAction_ConfigureRangeFinder(CfxPersistent *pOwner);
    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void Setup(bool State) { _state = State; }
    void Execute(CfxPersistent *pSender) override {}
    void Rollback(CfxPersistent *pSender) override {}
};

extern CfxAction *Create_Action_ConfigureRangeFinder(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_ConfigureSaveTargets
//
const GUID GUID_ACTION_CONFIGURESAVETARGETS = {0x318fa478, 0xfe45, 0x43fc, {0xa9, 0x54, 0x6a, 0xcc, 0xb1, 0xee, 0xe8, 0xe9}};

class CctAction_ConfigureSaveTargets: public CfxAction 
{
protected:
    XGUID _lastSave1Target = 0, _lastSave2Target = 0;
    XGUID _save1Target = 0, _save2Target = 0;
public:
    CctAction_ConfigureSaveTargets(CfxPersistent *pOwner);

    void DefineProperties(CfxFiler &F) override;
    void DefineResources(CfxFilerResource &F) override;
    void DefineState(CfxFiler &F) override;

    void Setup(XGUID Save1Target, XGUID Save2Target);

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};

extern CfxAction *Create_Action_ConfigureSaveTargets(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_SetGlobalValue
//
const GUID GUID_ACTION_SETGLOBALVALUE = {0xd2ab6a, 0x9a34, 0x411d, {0x98, 0xbb, 0x54, 0x76, 0x6b, 0x94, 0x3e, 0x69}};

class CctAction_SetGlobalValue: public CfxAction 
{
protected:
    GLOBALVALUE _oldValue, _newValue;
public:
    CctAction_SetGlobalValue(CfxPersistent *pOwner);

    void Setup(char *pName, double Value);

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};

extern CfxAction *Create_Action_SetGlobalValue(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_SetPendingGoto
//
const GUID GUID_ACTION_SETPENDINGGOTO = {0x3f66ccff, 0x96d4, 0x41e1, {0xbc, 0xee, 0xa7, 0x9d, 0x1f, 0x44, 0x6c, 0x6}};

class CctAction_SetPendingGoto: public CfxAction 
{
protected:
    char *_oldTitle = nullptr, *_newTitle = nullptr;
public:
    CctAction_SetPendingGoto(CfxPersistent *pOwner);
    ~CctAction_SetPendingGoto();

    void Setup(char *pTitle);

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};

extern CfxAction *Create_Action_SetPendingGoto(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_AddCustomGoto
//
const GUID GUID_ACTION_ADDCUSTOMGOTO = {0x57e0b41f, 0x6709, 0x4150, {0xbe, 0xa5, 0x66, 0x2f, 0x83, 0x9d, 0xd4, 0x5e}};

class CctAction_AddCustomGoto: public CfxAction 
{
protected:
    int _index = 0;
    FXGOTO _goto;
public:
    CctAction_AddCustomGoto(CfxPersistent *pOwner);

    void Setup(FXGOTO *pGoto);

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};

extern CfxAction *Create_Action_AddCustomGoto(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_SetElementListHistory
//
const GUID GUID_ACTION_SETELEMENTLISTHISTORY = {0x2a1e7339, 0x56fd, 0x41b0, {0x97, 0x17, 0xa6, 0x4, 0xf5, 0xd2, 0x1b, 0xfc}};

class CctAction_SetElementListHistory: public CfxAction 
{
protected:
    int _controlId = 0;
    int _index = 0;
    bool _oldValue = false, _newValue = false;
public:
    CctAction_SetElementListHistory(CfxPersistent *pOwner);

    void DefineProperties(CfxFiler &F) override;

    void Setup(int ControlId, int Index, bool OldValue, bool NewValue) { _controlId = ControlId; _index = Index; _oldValue = OldValue; _newValue = NewValue; }

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};
extern CfxAction *Create_Action_SetElementListHistory(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_ConfigureAlert
//
const GUID GUID_ACTION_CONFIGUREALERT = { 0xe46d0453, 0x80ff, 0x41da, { 0x96, 0x58, 0xc1, 0xa4, 0xf9, 0xab, 0xe3, 0xa2 } };

class CctAction_ConfigureAlert: public CfxAction 
{
protected:
    CctAlert _alert;
public:
    CctAction_ConfigureAlert(CfxPersistent *pOwner);

    void DefineProperties(CfxFiler &F) override;

    void Setup(CctAlert *pAlert);

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};

extern CfxAction *Create_Action_ConfigureAlert(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_TransferOnSave
//
const GUID GUID_ACTION_TRANSFERONSAVE = { 0xf45a28cd, 0x48f1, 0x4ae0, { 0xa7, 0x87, 0x81, 0x52, 0x5, 0xc7, 0x0, 0x44 } };

class CctAction_TransferOnSave: public CfxAction 
{
protected:
    bool _oldValue = false, _newValue = false;
public:
    CctAction_TransferOnSave(CfxPersistent *pOwner);

    void DefineState(CfxFiler &F) override;
    void DefineProperties(CfxFiler &F) override;

    void Setup(bool OldValue, bool NewValue);

    void Execute(CfxPersistent *pSender) override;
    void Rollback(CfxPersistent *pSender) override;
};

extern CfxAction *Create_Action_TransferOnSave(CfxPersistent *pOwner);

//*************************************************************************************************
