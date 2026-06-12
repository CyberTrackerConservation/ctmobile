#pragma once
#include "pch.h"
#include "ctSession.h"

//*************************************************************************************************
//
// CctControl_ConfigureGPS
//

const GUID GUID_CONTROL_CONFIGUREGPS = {0x440b2c86, 0xd385, 0x4efc, {0x8a, 0xce, 0x5d, 0x3c, 0xa, 0x4a, 0x1, 0x6b}};
const int CONFIGURE_GPS_VERSION = 1;
class CctControl_ConfigureGPS: public CfxControl
{
protected:
    bool _accuracyEnabled = false;
    FXGPS_ACCURACY _sightingAccuracy;
    int _sightingFixCount = 0;
    FXGPS_ACCURACY _waypointAccuracy;
    int _waypointTimer = 0;
    void OnSessionEvent(CfxControl *pSender, void *pParams);
public:
    CctControl_ConfigureGPS(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ConfigureGPS();
    int GetVersion() override { return CONFIGURE_GPS_VERSION; }
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    bool _required = false, _autoConnect = false;
    bool _connected = false;
    XGUID _latId = 0, _lonId = 0, _altId = 0, _accId = 0;

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_SnapGPS(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_SnapGPS();
    void DefineResources(CfxFilerResource &F) override;
    void DefineState(CfxFiler &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    bool _required = false;
    XGUID _latId = 0, _lonId = 0, _altId = 0, _accId = 0;

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionEnter(CfxControl *pSender, void *pParams);
public:
    CctControl_SnapLastGPS(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_SnapLastGPS();
    void DefineResources(CfxFilerResource &F) override;
    void DefineState(CfxFiler &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    double _dateTime = 0.0;
    XGUID _dateId = 0, _timeId = 0;
    bool _connectedGPS = false;

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_SnapDateTime(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_SnapDateTime();
    void DefineResources(CfxFilerResource &F) override;
    void DefineState(CfxFiler &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    XGUID _elementId = 0;
    char *_value = nullptr;
    bool _unique = false;
    bool _stamp = false;
    bool _appId = false;
    bool _updateTag = false;
    bool _empty = false;
    XGUID _targetScreenOnEmptyId = 0;
    void OnSessionEnter(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_AddAttribute(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_AddAttribute();
    void DefineResources(CfxFilerResource &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    XGUID _elementId = 0;
    char *_value = nullptr;
    void OnSessionEvent(CfxControl *pSender, void *pParams);
public:
    CctControl_AddUserName(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_AddUserName();
    void DefineResources(CfxFilerResource &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
};
extern CfxControl *Create_Control_AddUserName(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_ConfigureRangeFinder (no-op stub for resource file backward compatibility)
//

const GUID GUID_CONTROL_CONFIGURERANGEFINDER = {0x4a032825, 0x18bd, 0x4884, {0xbe, 0xb3, 0x17, 0xab, 0xc9, 0xf1, 0x84, 0xb3}};

class CctControl_ConfigureRangeFinder: public CfxControl
{
protected:
    bool _state = false;
public:
    CctControl_ConfigureRangeFinder(CfxPersistent *pOwner, CfxControl *pParent);
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    XGUID _save1Target = 0, _save2Target = 0;
    void OnSessionEvent(CfxControl *pSender, void *pParams);
public:
    CctControl_ConfigureSaveTargets(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ConfigureSaveTargets();
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
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
    char *_resetKey = nullptr;
    bool _immediate = false;
    void OnSessionEventEnter(CfxControl *pSender, void *pParams);
    void OnSessionEventLeave(CfxControl *pSender, void *pParams);
    void Execute(CctSession *pSession);
public:
    CctControl_ResetStateKey(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ResetStateKey();
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    XGUID _titleElementId = 0;
    char *_titlePrefix = nullptr;
    void OnSessionEvent(CfxControl *pSender, void *pParams);
public:
    CctControl_SetPendingGoto(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_SetPendingGoto();
    void DefineResources(CfxFilerResource &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    char *_title = nullptr;
    double _latitude = 0.0, _longitude = 0.0;
    void OnSessionEvent(CfxControl *pSender, void *pParams);
public:
    CctControl_AddCustomGoto(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_AddCustomGoto();
    void DefineResources(CfxFilerResource &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    void OnSessionEvent(CfxControl *pSender, void *pParams);
public:
    CctControl_WriteTrackBreak(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_WriteTrackBreak();
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    void OnSessionEvent(CfxControl *pSender, void *pParams);
public:
    CctControl_ConfigureAlert(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ConfigureAlert();

    void DefineResources(CfxFilerResource &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    bool _enabled = false;
    void OnSessionEvent(CfxControl *pSender, void *pParams);
public:
    CctControl_TransferOnSave(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_TransferOnSave();

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
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
    int _lowestAttributeCount = 0;
    ATTRIBUTES _attributes;
    void OnSessionEvent(CfxControl *pSender, void *pParams);
public:
    CctControl_RemoveLoopedAttributes(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_RemoveLoopedAttributes();

    void DefineState(CfxFiler &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
};
extern CfxControl *Create_Control_RemoveLoopedAttributes(CfxPersistent *pOwner, CfxControl *pParent);
