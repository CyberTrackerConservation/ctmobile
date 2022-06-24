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
// CctControl_ConfigureGPS
//

const GUID GUID_CONTROL_CONFIGUREGPS = {0x440b2c86, 0xd385, 0x4efc, {0x8a, 0xce, 0x5d, 0x3c, 0xa, 0x4a, 0x1, 0x6b}};
const UINT CONFIGURE_GPS_VERSION = 1;
class CctControl_ConfigureGPS: public CfxControl
{
protected:
    BOOL _accuracyEnabled;
    FXGPS_ACCURACY _sightingAccuracy;
    UINT _sightingFixCount;
    FXGPS_ACCURACY _waypointAccuracy;
    UINT _waypointTimer;
    BOOL _useRangeFinderForAltitude;
    VOID OnSessionEvent(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ConfigureGPS(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ConfigureGPS();
    UINT GetVersion() { return CONFIGURE_GPS_VERSION; }
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_ConfigureGPS(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_SnapGPS
//

const GUID GUID_CONTROL_SNAPGPS = {0x96d1ed0, 0x442d, 0x41a3, {0xbb, 0x4e, 0x19, 0x11, 0x3e, 0xbd, 0x7e, 0xf9}};
class CctControl_SnapGPS: public CfxControl
{
protected:
    FXGPS_POSITION _gps;
    BOOL _required, _autoConnect;
    BOOL _connected;
    XGUID _latId, _lonId, _altId, _accId;

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_SnapGPS(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_SnapGPS();
    VOID DefineResources(CfxFilerResource &F);
    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_SnapGPS(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_SnapLastGPS
//

const GUID GUID_CONTROL_SNAPLASTGPS = {0x582e62e2, 0xc95b, 0x4b87, {0x94, 0xd3, 0xee, 0x86, 0x3, 0xc8, 0x5, 0x6a}};

class CctControl_SnapLastGPS: public CfxControl
{
protected:
    FXGPS_POSITION _gps;
    BOOL _required;
    XGUID _latId, _lonId, _altId, _accId;

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionEnter(CfxControl *pSender, VOID *pParams);
public:
    CctControl_SnapLastGPS(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_SnapLastGPS();
    VOID DefineResources(CfxFilerResource &F);
    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_SnapLastGPS(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_SnapDateTime
//

const GUID GUID_CONTROL_SNAPDATETIME = {0xfd691f38, 0x4527, 0x4a7f, {0x9d, 0xa3, 0xe5, 0x84, 0x18, 0xf9, 0xfe, 0x24}};
class CctControl_SnapDateTime: public CfxControl
{
protected:
    DOUBLE _dateTime;
    XGUID _dateId, _timeId;
    BOOL _connectedGPS;

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_SnapDateTime(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_SnapDateTime();
    VOID DefineResources(CfxFilerResource &F);
    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_SnapDateTime(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_AddAttribute
//

const GUID GUID_CONTROL_ADDATTRIBUTE = {0xb76d0bf3, 0xf4ac, 0x44ff, {0xa1, 0xe6, 0x2, 0x70, 0x71, 0x27, 0xc9, 0x49}};
class CctControl_AddAttribute: public CfxControl
{
protected:
    XGUID _elementId;
    CHAR *_value;
    BOOL _unique;
    BOOL _stamp;
    BOOL _appId;
    BOOL _updateTag;
    BOOL _empty;
    XGUID _targetScreenOnEmptyId;
    VOID OnSessionEnter(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_AddAttribute(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_AddAttribute();
    VOID DefineResources(CfxFilerResource &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_AddAttribute(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_AddUserName
//

const GUID GUID_CONTROL_ADDUSERNAME = {0xc2c18eae, 0xfd2f, 0x4151, {0xa0, 0x34, 0x61, 0x75, 0x33, 0x5d, 0x43, 0xb2}};

class CctControl_AddUserName: public CfxControl
{
protected:
    XGUID _elementId;
    CHAR *_value;
    VOID OnSessionEvent(CfxControl *pSender, VOID *pParams);
public:
    CctControl_AddUserName(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_AddUserName();
    VOID DefineResources(CfxFilerResource &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_AddUserName(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_ConfigureRangeFinder
//

const GUID GUID_CONTROL_CONFIGURERANGEFINDER = {0x4a032825, 0x18bd, 0x4884, {0xbe, 0xb3, 0x17, 0xab, 0xc9, 0xf1, 0x84, 0xb3}};

class CctControl_ConfigureRangeFinder: public CfxControl
{
protected:
    BOOL _state;
    VOID OnSessionEvent(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ConfigureRangeFinder(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ConfigureRangeFinder();
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_ConfigureRangeFinder(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_ConfigureSaveTargets
//

const GUID GUID_CONTROL_CONFIGURESAVETARGETS = {0x73eeedcd, 0x38ad, 0x43b3, {0xb7, 0xa6, 0x79, 0xef, 0x3, 0x8d, 0x62, 0xff}};

class CctControl_ConfigureSaveTargets: public CfxControl
{
protected:
    XGUID _save1Target, _save2Target;
    VOID OnSessionEvent(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ConfigureSaveTargets(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ConfigureSaveTargets();
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};
extern CfxControl *Create_Control_ConfigureSaveTargets(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_ResetStateKey
//

const GUID GUID_CONTROL_RESETSTATEKEY = {0xcf47fe86, 0xcb3d, 0x4e85, {0xa8, 0x23, 0x9d, 0x99, 0x9, 0x6d, 0x7f, 0x81}};

class CctControl_ResetStateKey: public CfxControl
{
protected:
    CHAR *_resetKey;
    BOOL _immediate;
    VOID OnSessionEventEnter(CfxControl *pSender, VOID *pParams);
    VOID OnSessionEventLeave(CfxControl *pSender, VOID *pParams);
    VOID Execute(CctSession *pSession);
public:
    CctControl_ResetStateKey(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ResetStateKey();
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_ResetStateKey(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_SetPendingGoto
//

const GUID GUID_CONTROL_SETPENDINGGOTO = {0x5a73461f, 0xf436, 0x4392, {0xac, 0xaf, 0x54, 0x4e, 0x8, 0xbd, 0x65, 0x98}};

class CctControl_SetPendingGoto: public CfxControl
{
protected:
    XGUID _titleElementId;
    CHAR *_titlePrefix;
    VOID OnSessionEvent(CfxControl *pSender, VOID *pParams);
public:
    CctControl_SetPendingGoto(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_SetPendingGoto();
    VOID DefineResources(CfxFilerResource &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_SetPendingGoto(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_AddCustomGoto
//

const GUID GUID_CONTROL_ADDCUSTOMGOTO = {0x71cfc237, 0x36ea, 0x41b9, {0x82, 0x8a, 0xd0, 0x21, 0xb0, 0xf0, 0xec, 0x66}};

class CctControl_AddCustomGoto: public CfxControl
{
protected:
    CHAR *_title;
    DOUBLE _latitude, _longitude;
    VOID OnSessionEvent(CfxControl *pSender, VOID *pParams);
public:
    CctControl_AddCustomGoto(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_AddCustomGoto();
    VOID DefineResources(CfxFilerResource &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_AddCustomGoto(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_WriteTrackBreak
//

const GUID GUID_CONTROL_WRITETRACKBREAK = {0x6c3a9a7b, 0x5ea, 0x4d29, {0x89, 0x47, 0x54, 0xdd, 0x38, 0x2b, 0xef, 0x86}};

class CctControl_WriteTrackBreak: public CfxControl
{
protected:
    VOID OnSessionEvent(CfxControl *pSender, VOID *pParams);
public:
    CctControl_WriteTrackBreak(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_WriteTrackBreak();
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_WriteTrackBreak(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_ConfigureAlert
//

const GUID GUID_CONTROL_CONFIGUREALERT = { 0x935a0794, 0x78bf, 0x4689, { 0xb6, 0xf5, 0xf3, 0xec, 0x30, 0x70, 0xbd, 0x20 } };

class CctControl_ConfigureAlert: public CfxControl
{
protected:
    CctAlert _alert;
    VOID OnSessionEvent(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ConfigureAlert(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ConfigureAlert();

    VOID DefineResources(CfxFilerResource &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_ConfigureAlert(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_TransferOnSave
//

const GUID GUID_CONTROL_TRANSFERONSAVE = { 0x6f58f58, 0xa9bc, 0x4c23, { 0xa1, 0xa, 0x4a, 0x35, 0xcc, 0x46, 0x5d, 0x6 } };

class CctControl_TransferOnSave: public CfxControl
{
protected:
    BOOL _enabled;
    VOID OnSessionEvent(CfxControl *pSender, VOID *pParams);
public:
    CctControl_TransferOnSave(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_TransferOnSave();

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_TransferOnSave(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_RemoveLoopedAttributes
//

const GUID GUID_CONTROL_REMOVELOOPEDATTRIBUTES = { 0x4c4bc759, 0xa3dd, 0x4eeb, { 0xad, 0x56, 0x45, 0x3e, 0x10, 0x1a, 0xe8, 0x12 } };

class CctControl_RemoveLoopedAttributes: public CfxControl
{
protected:
    UINT _lowestAttributeCount;
    ATTRIBUTES _attributes;
    VOID OnSessionEvent(CfxControl *pSender, VOID *pParams);
public:
    CctControl_RemoveLoopedAttributes(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_RemoveLoopedAttributes();

    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};
extern CfxControl *Create_Control_RemoveLoopedAttributes(CfxPersistent *pOwner, CfxControl *pParent);
