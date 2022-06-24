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
    XGUID _screenId;
    INT _index;
public:
    CctAction_AppendScreen(CfxPersistent *pOwner);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID Setup(XGUID Value) { _screenId = Value; }

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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
    VOID Execute(CfxPersistent *pSender);
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
    UINT _controlId;
    ATTRIBUTES _oldAttributes, _newAttributes;
public:
    CctAction_MergeAttribute(CfxPersistent *pOwner);
    ~CctAction_MergeAttribute();

    BOOL IsValid()           { return _controlId != 0;           }
    UINT GetAttributeCount() { return _newAttributes.GetCount(); }

    VOID SetControlId(UINT Value) { _controlId = Value; }
    VOID SetupAdd(UINT ControlId, XGUID ElementId, FXDATATYPE Type = dtNull, VOID *pData = NULL, XGUID ValueId = XGUID_0);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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
    BOOL _accuracyEnabled;
    FXGPS_ACCURACY _sightingAccuracy, _lastSightingAccuracy;
    UINT _sightingFixCount, _lastSightingFixCount;
    FXGPS_ACCURACY _waypointAccuracy, _lastWaypointAccuracy;
    UINT _waypointTimer, _lastWaypointTimer;
    BOOL _useRangeFinderForAltitude, _lastUseRangeFinderForAltitude;
public:
    CctAction_ConfigureGPS(CfxPersistent *pOwner);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);

    VOID Setup(BOOL AccuracyEnabled, FXGPS_ACCURACY *pSightingAccuracy, UINT SightingFixCount, FXGPS_ACCURACY *pWaypointAccuracy, UINT WaypointTimer, BOOL UseRangeFinderForAltitude);
    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);

    VOID Setup(FXGPS_POSITION *pGPS);

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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
    DOUBLE _dateTime, _lastDateTime;
    CctAction_MergeAttribute *_mergeAttributes;
public:
    CctAction_SnapDateTime(CfxPersistent *pOwner);
    ~CctAction_SnapDateTime();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);

    VOID Setup(UINT ControlId, FXDATETIME *pDateTime, XGUID DateElementId = XGUID_0, XGUID TimeElementId = XGUID_0);

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
};
extern CfxAction *Create_Action_SnapDateTime(CfxPersistent *pOwner);

//*************************************************************************************************
//
// CctAction_ConfigureRangeFinder
//
const GUID GUID_ACTION_CONFIGURERANGEFINDER = {0x7bdcf6fa, 0xcf75, 0x48c0, {0xa7, 0x6d, 0xc3, 0x75, 0xc9, 0x5e, 0xb9, 0x68}};

class CctAction_ConfigureRangeFinder: public CfxAction 
{
protected:
    BOOL _state, _lastState;
public:
    CctAction_ConfigureRangeFinder(CfxPersistent *pOwner);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);

    VOID Setup(BOOL State);
    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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
    XGUID _lastSave1Target, _lastSave2Target;
    XGUID _save1Target, _save2Target;
public:
    CctAction_ConfigureSaveTargets(CfxPersistent *pOwner);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);
    VOID DefineState(CfxFiler &F);

    VOID Setup(XGUID Save1Target, XGUID Save2Target);

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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

    VOID Setup(CHAR *pName, DOUBLE Value);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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
    CHAR *_oldTitle, *_newTitle;
public:
    CctAction_SetPendingGoto(CfxPersistent *pOwner);
    ~CctAction_SetPendingGoto();

    VOID Setup(CHAR *pTitle);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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
    UINT _index;
    FXGOTO _goto;
public:
    CctAction_AddCustomGoto(CfxPersistent *pOwner);

    VOID Setup(FXGOTO *pGoto);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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
    UINT _controlId;
    UINT _index;
    BOOL _oldValue, _newValue;
public:
    CctAction_SetElementListHistory(CfxPersistent *pOwner);

    VOID DefineProperties(CfxFiler &F);

    VOID Setup(UINT ControlId, UINT Index, BOOL OldValue, BOOL NewValue) { _controlId = ControlId; _index = Index; _oldValue = OldValue; _newValue = NewValue; }

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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

    VOID DefineProperties(CfxFiler &F);

    VOID Setup(CctAlert *pAlert);

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
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
    BOOL _oldValue, _newValue;
public:
    CctAction_TransferOnSave(CfxPersistent *pOwner);

    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);

    VOID Setup(BOOL OldValue, BOOL NewValue);

    VOID Execute(CfxPersistent *pSender);
    VOID Rollback(CfxPersistent *pSender);
};

extern CfxAction *Create_Action_TransferOnSave(CfxPersistent *pOwner);

//*************************************************************************************************
