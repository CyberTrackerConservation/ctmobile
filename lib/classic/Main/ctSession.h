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
#include "fxControl.h"
#include "fxAction.h"
#include "fxApplication.h"
#include "fxButton.h"
#include "fxDatabase.h"
#include "ctSighting.h"
#include "ctWaypoint.h"
#include "ctHistoryItem.h"

// Header for the sighting database records
const UINT MAGIC_SESSION = 'SESS';

// Version control
const UINT RESOURCE_VERSION = 515;

// Session stuff
const GUID GUID_CONTROL_SESSION   = {0x82e911e2, 0x528f, 0x4d64, {0xbd, 0x81, 0x04, 0x5a, 0x2a, 0x04, 0xef, 0x0d}};

const DOUBLE WAYPOINT_DEFAULT_GPS_ACCURACY = GPS_DEFAULT_ACCURACY;
const DOUBLE SIGHTING_DEFAULT_GPS_ACCURACY = GPS_DEFAULT_ACCURACY;
const UINT SIGHTING_DEFAULT_FIX_COUNT      = 1;

// GUIDs shared with Server
const GUID GUID_ELEMENT_DATE        = {0x4764f5e6, 0x15a1, 0x48bf, {0x80, 0x8a, 0xf6, 0x73, 0xed, 0x7c, 0xdc, 0xda}};
const GUID GUID_ELEMENT_TIME        = {0xeb86279a, 0xe032, 0x43d2, {0xb4, 0xa8, 0x8b, 0x8b, 0x28, 0x92, 0xb1, 0x0e}};
const GUID GUID_ELEMENT_LATITUDE    = {0xf6636a2d, 0x2e9a, 0x474d, {0xa7, 0x7d, 0x2e, 0x1b, 0xae, 0x1e, 0xf8, 0x57}};
const GUID GUID_ELEMENT_LONGITUDE   = {0xd93e86a8, 0x2629, 0x44fe, {0xaf, 0xc4, 0x2e, 0xc5, 0x10, 0xfb, 0x18, 0x20}};
const GUID GUID_ELEMENT_ALTITUDE    = {0x3e2df3a5, 0x4d01, 0x47b9, {0x8e, 0xc6, 0xa3, 0x10, 0x83, 0xa3, 0xbc, 0x18}};
const GUID GUID_ELEMENT_ACCURACY    = {0x4fb2c6b9, 0x08dd, 0x47b9, {0x93, 0xf1, 0xaf, 0x5f, 0xa8, 0xee, 0x63, 0xc4}};
const GUID GUID_ELEMENT_SPEED       = {0xf5e5552f, 0x7b23, 0x4f0d, {0x81, 0x54, 0x77, 0xb5, 0xfa, 0x49, 0xe9, 0x6c}};
const GUID GUID_ELEMENT_HEADING     = {0xed2aa133, 0x5768, 0x46f7, {0xa5, 0xa9, 0xc3, 0xa1, 0x6f, 0xbc, 0x79, 0xfa}};
const GUID GUID_ELEMENT_PHOTO       = {0x82D16C8E, 0x776E, 0x4e8b, {0xA4, 0x59, 0x6E, 0xBF, 0x62, 0xE5 ,0x00, 0x76}};
const GUID GUID_ELEMENT_BARCODE		= {0x64cce46f, 0x0704, 0x4a84, {0xab, 0xc7, 0x87, 0x30, 0x23, 0xb5, 0xa9, 0x1e}};
const GUID GUID_ELEMENT_SOUND       = {0xba3e0b6b, 0x7e07, 0x47db, {0xac, 0x2d, 0xfc, 0x46, 0x83, 0x1c, 0xd8, 0xa7}};
const GUID GUID_ELEMENT_USERNAME    = {0x7662336F, 0xD7B2, 0x4962, {0xBC, 0x7A, 0x9C, 0xC2, 0x37, 0xDA, 0x2E, 0xAA}};
const GUID GUID_ELEMENT_STATUS      = {0xE0A63989, 0xB292, 0x43bc, {0x81, 0xFC, 0xFB, 0x73, 0xB7, 0x28, 0x8B, 0xDC}};

const GUID GUID_ELEMENT_RANGEFINDER_ID    = {0xe3948bc5, 0xb998, 0x47f0, {0xae, 0xe, 0xdd, 0x95, 0x4b, 0x50, 0x3f, 0x81}};
const GUID GUID_ELEMENT_RANGE             = {0xff625ba3, 0x316a, 0x490b, {0x81, 0xa6, 0xd9, 0x61, 0xc6, 0x8e, 0xa9, 0x71}};
const GUID GUID_ELEMENT_RANGE_UNITS       = {0xf643ae74, 0xbdb1, 0x4b5c, {0xa7, 0x6f, 0xcb, 0x79, 0x51, 0xc8, 0x9d, 0xfc}};
const GUID GUID_ELEMENT_POLAR_BEARING     = {0x1680e1e3, 0x442a, 0x4dff, {0x9e, 0x72, 0x0d, 0x3e, 0x16, 0x6f, 0x81, 0x27}};
const GUID GUID_ELEMENT_POLAR_BEARINGMODE = {0xd69380ab, 0xaff5, 0x488b, {0x9b, 0xc1, 0x4c, 0x4c, 0x5d, 0x8e, 0x96, 0xe4}};
const GUID GUID_ELEMENT_POLAR_INCLINATION = {0x013923d9, 0x958d, 0x43fd, {0x98, 0x08, 0x76, 0x83, 0xda, 0x6a, 0xbf, 0x7b}};
const GUID GUID_ELEMENT_POLAR_ROLL        = {0x8d5ddc83, 0xf1ec, 0x45f1, {0x9e, 0x21, 0xde, 0xef, 0x2a, 0x2c, 0x98, 0x5e}};
const GUID GUID_ELEMENT_AZIMUTH           = {0xfbdf7b37, 0x4d9c, 0x4c42, {0xaa, 0x2a, 0x63, 0x01, 0xe1, 0xbc, 0xf8, 0x50}};
const GUID GUID_ELEMENT_INCLINATION       = {0x457a5d9d, 0x0a09, 0x44b2, {0x87, 0x11, 0xa3, 0xaf, 0x82, 0x68, 0x4e, 0x1b}};

const GUID GUID_ELEMENT_TRANSECT_LENGTH   = {0x96f17b05, 0x7881, 0x4910, {0x90, 0xcc, 0x1b, 0x53, 0xce, 0xea, 0x9, 0x1f}};
const GUID GUID_ELEMENT_TRANSECT_PERPENDICULAR_DISTANCE = {0x6b9c0ccf, 0xba3b, 0x494e, {0x9e, 0xea, 0xaa, 0xa3, 0x53, 0x3e, 0x1e, 0x30}};

//const GUID GUID_ELEMENT_HORIZONTAL_DISTANCE       = {0x0d81815c, 0xc9c6, 0x48a2, {0xba, 0xb1, 0xc5, 0xfb, 0xf8, 0x17, 0x27, 0x8e}};
//const GUID GUID_ELEMENT_HORIZONTAL_DISTANCE_UNITS = {0xba0792a1, 0x2c2f, 0x4773, {0xb8, 0xfb, 0x4a, 0xeb, 0x04, 0x70, 0xee, 0xfc}};
//const GUID GUID_ELEMENT_SLOPE_DISTANCE            = {0x2b42073f, 0x4e26, 0x4f53, {0xa8, 0x5b, 0x2b, 0xa1, 0x01, 0x93, 0xf3, 0x2c}};
//const GUID GUID_ELEMENT_SLOPE_DISTANCE_UNITS      = {0xbe97c747, 0xa8d2, 0x480b, {0x93, 0x22, 0xe5, 0x28, 0x62, 0x9d, 0x21, 0x29}};

// Must be packed for compatibility with the Server
#pragma pack(push, 1) 
struct CTRESOURCEHEADER: public FXRESOURCEHEADER
{
    CHAR ServerFileName[256];
    GUID SequenceId;
    FXGPS_ACCURACY SightingAccuracy;
    UINT SightingFixCount;
    FXGPS_ACCURACY WaypointAccuracy;
    UINT WaypointTimer;
    BOOL ManualMapOnSkip;
    BOOL ForceTitleBar;
    BOOL UseSD;
    BYTE Padding;
    FXSENDDATA SendData;
    BOOL AllowManualSend;
    BOOL ClearDataOnSend;
    UINT AutoSendTimeout;
    BOOL TestTime;
    BOOL GpsTimeSync;
    INT GpsTimeZone;
    INT GpsSkipTimeout;
    BOOL ManualOnSkip;
    BOOL ManualAllowSkip;
    BOOL KioskMode;
    BOOL SimpleCamera;
    BYTE Padding1;
    BYTE Padding2;
    BYTE Padding3;
    BOOL DisableEditing;
    BOOL ResetStateOnSync;
    BOOL ClearOnNext;
    BYTE Projection;
    
    BYTE UTMZone;
    CHAR OwnerName[128];
    CHAR OwnerOrganization[128];
    CHAR OwnerAddress1[64];
    CHAR OwnerAddress2[64];
    CHAR OwnerAddress3[64];
    CHAR OwnerPhone[64];
    CHAR OwnerEmail[64];
    CHAR OwnerWebSite[64];

    CHAR WebUpdateUrl[256];
    UINT WebUpdateFrequency;
    BOOL WebUpdateCheckOnLaunch;
};
#pragma pack(pop)

enum SESSIONEVENT
{
    seEnter,         // The screen has loaded
    seEnterBack,     // Screen has loaded from a back 
    seEnterNext,     // Screen has loaded from a next
    seLeave,         // Leaving the screen
    seCanNext,       // Allow next
    seBeforeNext,    // Next is about to run
    seNext,          // Next has happened
    seAfterNext,     // Controls have added their state
    seCanBack,       // Can we go back
    seBack,          // Back has happened
    seNewWaypoint,   // A new waypoint has been added
    seSetJumpTarget, // Set intended jump target
    seGlobalChanged  // Global value has changed
};

struct STOREITEM
{
    GUID PrimaryId;
    UINT ControlId;
    VOID *Data;
};

struct GLOBALVALUE
{
    CHAR Name[32];
    DOUBLE Value;
};

struct DIALOGSTATE
{
    GUID Id;
    VOID *StateData;
};

struct GLOBAL_CHANGED_PARAMS
{
    CfxPersistent *pSender;
    const CHAR *Name;
    DOUBLE OldValue, NewValue;
};

const CHAR GPS_WAYPOINT_FONT[] = "Arial Narrow,7,";

//*************************************************************************************************

class CctSession;

//*************************************************************************************************
//
// CctJsonBuilder
//
class CctJsonBuilder
{
protected:
    CctSession *_session;
    CfxFileStream *_fileStream;
    UINT _protocol;
    static VOID ResolveElement(CctSession *pSession, CfxResource *pResource, XGUID ElementId, CfxString &Name);
    BOOL WriteMediaAttribute(ATTRIBUTE *pAttribute);
    BOOL WriteSightingGeoJson(CctSighting *pSighting, FXALERT *pAlert, BOOL First, FXFILELIST *pMediaFileList);
    BOOL WriteSightingEsri(CctSighting *pSighting, BOOL First, FXFILELIST *pMediaFileList);
public:
    CctJsonBuilder(CctSession *pSession, CfxFileStream *pFileStream, UINT Protocol);

    BOOL WriteHeader();
    BOOL WriteSighting(CctSighting *pSighting, FXALERT *pAlert, BOOL First, FXFILELIST *pMediaFileList);
    BOOL WriteWaypoint(WAYPOINT *pWaypoint, BOOL First);
    BOOL WriteFooter();

    static VOID WriteSightingToStream(CctSession *pSession, CctSighting *pSighting, CfxStream &Data);
};

//*************************************************************************************************
//
// CctAlert
//
class CctAlert: public CfxPersistent
{
protected:
    BOOL _active;
    CHAR *_url;
    CHAR *_userName, *_password;
    CHAR *_caId;
    CHAR *_description, *_type;
    UINT _level;
    UINT _protocol;
    XGUIDLIST _matchElements;

    UINT _pingFrequency;        // 0 means one-time alert, otherwise it's a ping
    BOOL _pingOnly;
    XGUID _patrolElementId;     // Element value to add to alert

    BOOL _committed;            // If Commit() returns true on sighting save
    CHAR *_patrolValue;         // Computed PatrolElementId from sighting
    FXALERT _alert;             // Built from above attributes

    BOOL Build(CctSighting *pSighting, FXALERT *pAlert);
    BOOL Match(CctSighting *pSighting);

public:
    CctAlert(CfxPersistent *pOwner);
    ~CctAlert();

    UINT GetPingFrequency() { return _pingFrequency; }

    BOOL GetCommitted()     { return _committed;     }

    BOOL Commit(CctSession *pSession, CctSighting *pSighting);
    VOID ActivateAlarm();

    BOOL Compare(CctAlert *pAlert);

    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnAlarm();
};

//*************************************************************************************************
//
// CctRetainState
//
class CctRetainState: public CfxPersistent
{
protected:
    CfxControl *_parent;
    BOOL _retain;
    CHAR *_globalValue;
    DOUBLE _lastValue;
public:
    CctRetainState(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctRetainState();

    VOID SetRetain(BOOL Value) { _retain = Value; }
    VOID SetGlobalValue(const CHAR *Value) { MEM_FREE(_globalValue); _globalValue = AllocString(Value); }

    BOOL MustRetain() { return _retain; }
    BOOL MustResetState();

    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};

struct RETAINSTATE
{
    CfxControl *Control;
    CctRetainState *RetainState;
};

//*************************************************************************************************
//
// CctWaypointManager
//
class CctWaypointManager: public CfxPersistent
{
protected:
    BOOL _connected;
    UINT _uniqueness;
    BYTE _fixTryCount;
    UINT _lastUserEventTime;
    BOOL _lastSuccess;
    BOOL _gpsWasConnected;
    BOOL _gpsLocked;
    UINT _timeout;
    UINT _minTimeout;
    FXGPS_ACCURACY _accuracy;
    BOOL _alive;
    GUID _alarmInstanceId;

    BOOL _pendingEvent;
    INT _lockCount;

    VOID LockGPS();
    VOID UnlockGPS();
    VOID SetupTimer(UINT ElapseInSeconds);
    CctSession *GetSession() { return (CctSession *)_owner; }
    UINT GetFinalTimeout();
    VOID SaveAlarmInstanceId();
    BOOL SameAlarmInstanceId();
public:
    CctWaypointManager(CfxPersistent *pOwner);
    ~CctWaypointManager();

    VOID BeginUpdate();
    VOID EndUpdate();
    BOOL InUpdate();

    VOID OnAlarm();
    VOID OnTimer();
    VOID OnTrackTimer();

    VOID DefineState(CfxFiler &F);
    BOOL StoreRecord(FXGPS *pGPS, BOOL CheckTime = TRUE);
    VOID StoreRecordFromSighting(CctSighting *pSighting);

    BOOL GetAlive()   { return _alive;   }

    UINT GetTimeout() { return _timeout; }
    VOID SetTimeout(UINT Value);

    UINT GetMinTimeout() { return _minTimeout; }
    VOID SetMinTimeout(UINT Value);

    FXGPS_ACCURACY *GetAccuracy()  { return &_accuracy;  }
    VOID SetAccuracy(FXGPS_ACCURACY *pValue) { _accuracy = *pValue; }
    VOID DrawStatusIcon(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, COLOR BackColor, COLOR ForeColor, BOOL Invert);

    VOID Connect();
    VOID Disconnect();
    VOID Restart();
};

//*************************************************************************************************
//
// CctTransferManager
//
struct HASH_GUID
{
    std::uint32_t Data1;
    std::uint16_t Data2;
    std::uint16_t Data3;
    std::uint8_t  Data4[8];
};

static_assert(sizeof(HASH_GUID) == 128 / CHAR_BIT, "HASH_GUID");

namespace std
{
    template<> struct hash<HASH_GUID>
    {
        size_t operator()(const HASH_GUID& guid) const noexcept
        {
            const std::uint64_t* p = reinterpret_cast<const std::uint64_t*>(&guid);
            std::hash<std::uint64_t> hash;
            return hash(p[0]) ^ hash(p[1]);
        }
    };
}

inline bool operator== (const HASH_GUID& a, const HASH_GUID& b)
{
    return std::memcmp(&a, &b, sizeof(HASH_GUID)) == 0;
}

inline HASH_GUID makeHashGuid(const GUID *pGuid)
{
    auto result = HASH_GUID();
    memcpy(&result, pGuid, sizeof(HASH_GUID));
    return result;
}

class CctTransferManager: public CfxPersistent
{
protected:
    UINT _dataUniqueness;
    UINT _autoSendTimeout;
    TList<CctAlert *> _alerts;
    std::unordered_map<HASH_GUID, bool> _sentHistoryHash;

    CHAR *AllocSentHistoryFileName(const CHAR *pExtension);
    BOOL LoadSentHistory(const CHAR *pExtension);
    BOOL SaveSentHistory(const CHAR *pExtension);
    VOID DeleteSentHistory(const CHAR *pExtension);
    BOOL HasSentHistoryId(GUID *pId);
    VOID AddSentHistoryId(GUID *pId);

    CctSession *GetSession() { return (CctSession *)_owner; }
    BOOL CreateFileInQueue(CHAR *pTargetFileName, CHAR *pTargetFileExtension, BYTE *pData, UINT DataSize);
    BOOL CreateFileInQueue(CHAR *pTargetFileName, CHAR *pTargetFileExtension, CHAR *pSourceFileName, BOOL Compress, BOOL CopyNotMove);
    BOOL PushToOutgoing(CHAR *pFileName, CHAR *pFileExtension, FXSENDDATA *pSendData);
    BOOL PushMediaToOutgoing(CHAR *pSourceFileName, CHAR *pTargetFileName, UINT TargetId, FXSENDDATA *pSendData);
public:
    CctTransferManager(CfxPersistent *pOwner);
    ~CctTransferManager();

    VOID SetAutoSendTimeout(UINT Value) { _autoSendTimeout = Value; }
    VOID Reset();

    VOID ResetSentHistory();
    VOID RemoveSentHistoryId(const CHAR *pExtension, GUID *pId);

    VOID DefineState(CfxFiler &F);

    BOOL HasOutgoingData();
    BOOL HasTransferData();
    BOOL HasExportData();
    BOOL HasPendingAlert();

    VOID ClearAlerts();
    VOID AddAlert(CctAlert *pAlert);
    VOID RemoveAlert(CctAlert *pAlert);
    VOID CommitAlerts(CctSighting *pSighting);

    CHAR *BuildFileName(CHAR *pPrefix = NULL);

    BOOL CreateCTX(CHAR *pFileName, FXFILELIST *pOutMediaFileList, BOOL *pOutHasData, BOOL *pOutHasMore, FXEXPORTFILEINFO* pExportFileInfo = NULL);
    BOOL CreateJSON(CHAR *pFileName, UINT Protocol, FXFILELIST *pOutMediaFileList, BOOL *pOutHasData, INT* startSightingIndex, INT* startWaypointIndex, BOOL *pOverflow);

    BOOL CreateTransfer(BOOL ForceClear = FALSE);
    BOOL CreateAlert(CctSighting *pSighting, FXALERT *pAlert);
    BOOL CreatePing(FXALERT *pAlert);

    VOID Update();
    VOID Disconnect();

    VOID OnAlarm();

    static BOOL FlushData(CfxHost *host, const CHAR *pAppName);
};

//*************************************************************************************************
//
// CctUpdateManager
//
class CctUpdateManager: public CfxPersistent
{
private:
    DOUBLE _lastCheckTime;
    DOUBLE _lastUpdateTime;
    CHAR *_updateReadyPath;
    CHAR *_updateTag;
    CHAR *_updateUrl;
    CctSession *GetSession() { return (CctSession *)_owner; }
    BOOL IsUpdateSupported();
    VOID CheckForUpdate();
    VOID UpdateInPlace(const CHAR *pSrcPath, const CHAR *pTag, const CHAR *pUrl);
public:
    CctUpdateManager(CfxPersistent *pOwner);
    ~CctUpdateManager();

    VOID DefineState(CfxFiler &F);

    VOID OnAlarm();
    VOID Connect();
    VOID Disconnect();

    VOID OnSessionCommit();

    static BOOL HasData(CfxHost* host, const CHAR *pAppName);
    static VOID Install(const CHAR *pSrcPath, const CHAR *pDstPath, const CHAR *pAppName, const CHAR *pTag, const CHAR *pUrl);

    VOID DoUpdateReady(const CHAR *pAppId, const CHAR *pStampId, const CHAR *pTargetPath, const CHAR *pTag, const CHAR *pUrl, UINT ErrorCode);
};

//
// CctSession
//
typedef VOID (CfxPersistent::*EnumWaypointCallback)(WAYPOINT *pWaypoint);
typedef VOID (CfxPersistent::*EnumHistoryWaypointCallback)(HISTORY_WAYPOINT *pHistoryWaypoint);

class CctSession: public CfxControl
{
protected:
    CfxScreen *_screen, *_screenRight;
    CfxControl_Button *_messageWindow;

    CfxResource *_resource;
    CHAR _name[MAX_PATH];
    CHAR *_sdPath;
    CHAR *_applicationPath;
    CHAR *_mediaPath;
    CHAR *_outgoingPath;
    CHAR *_tempPath;

    CHAR *_userName;
    CHAR *_password;

    UINT _stateSlot;
    XGUID _firstScreenId;

    CfxDatabase *_sightingDB;
    CfxDatabase *_stateDB;
    
    TList<STOREITEM> _store;
    TList<GLOBALVALUE> _globalValues;
    TList<RETAINSTATE> _retainStates;

    CHAR *_sightingAccuracyName;
    FXGPS_ACCURACY _sightingAccuracy;
    UINT _sightingFixCount;
    BOOL _rangeFinderLastState, _rangeFinderConnected;
    XGUID _defaultSave1Target, _defaultSave2Target;

    FXGPS_POSITION _lastGPS;
    BOOL _saving;
    BOOL _transferOnSave;

    UINT _dataUniqueness;
    BOOL _shareDataEnabled;

    CfxEventManager _events;
    CfxActionManager _actions;
    CfxActionManager _stateActions;
    CctSighting _sighting;
    CctSighting _backupSighting;
    CctWaypointManager _waypointManager;
    CctTransferManager _transferManager;
    CctUpdateManager _updateManager;

    DBID _sightingId;
    GUID _resourceStampId;

    TList<FXGOTO> _customGotos;
    CHAR *_pendingGotoTitle;

    BOOL _connected;
    BOOL _faulted;
    COLOR _messageBackColor;
    CHAR _messageText[128];
    TList<DIALOGSTATE> _dialogStates;

    HISTORY_ITEM *_historyItemDef, *_historyItem;
    UINT _historyItemCount, _historyItemIndex;
    UINT _historyItemSize;

    WAYPOINT _lastWaypoint;

    BOOL MatchProfileExact(CTRESOURCEHEADER *pHeader, UINT Width, UINT Height, UINT DPI, FXPROFILE *pMatch);
    BOOL MatchProfileFuzzy(CTRESOURCEHEADER *pHeader, UINT Width, UINT Height, UINT DPI, FXPROFILE *pMatch);

    VOID ClearDialogs();

    BOOL InitPaths();
    VOID FreePaths();

    BOOL SD_Initialize();
    VOID SD_Finalize();
    BOOL SD_WriteSighting(CctSighting *pSighting);
    BOOL SD_WriteWaypoint(WAYPOINT *pWaypoint);

    BOOL ArchiveSighting(CctSighting *pSighting);
    BOOL ArchiveWaypoint(WAYPOINT *pWaypoint);

    BOOL ValidateSightingHeader(CfxDatabase *SightingDatabase);
    VOID ForceTitleBar(CfxScreen *pScreen);

    VOID LoadState();
    VOID LoadScreen(CfxScreen *pScreen, XGUID ScreenId, VOID *pState);
    VOID SaveScreen(CfxScreen *pScreen, XGUID ScreenId, UINT PathIndex);
    VOID LoadCurrentScreen();
    VOID LoadCurrentScreenAndState();
    VOID SaveCurrentScreen();

    CfxControl_Button *GetMessageWindow();

    VOID OnShowMenu(CfxPersistent *pSender, VOID *pParams);
    VOID OnMessageClick(CfxControl *pSender, VOID *pParams);
    VOID OnMessagePaint(CfxControl *pSender, VOID *pParams);

    VOID ProcessElementEffects();

    VOID LeaveScreen();
    VOID EnterScreen();

public:
    CctSession(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctSession();

    BOOL GetConnected()          { return _connected; }
    GUID GetCurrentSequenceId();
    BOOL Connect(CHAR *pName, UINT StateSlot, XGUID FirstScreenId = XGUID_0);
    VOID Disconnect();
    VOID Reconnect(DBID SightingId = INVALID_DBID, BOOL DoRepaint = TRUE);
    VOID Run();

    CTRESOURCEHEADER *GetResourceHeader() { return (CTRESOURCEHEADER *)_resource->GetHeader(this); }

    CHAR *GetName()     { return _name;                        }
    BOOL IsEditing()    { return _sightingId != INVALID_DBID;  }
    BOOL IsSaving()     { return _saving;                      }
    BOOL InDialog()     { return _dialogStates.GetCount() > 0; }

    VOID GetDefaultSaveTargets(XGUID *pSave1Target, XGUID *pSave2Target) { *pSave1Target = _defaultSave1Target; *pSave2Target = _defaultSave2Target; }
    VOID SetDefaultSaveTargets(XGUID Save1Target, XGUID Save2Target)     { _defaultSave1Target = Save1Target; _defaultSave2Target = Save2Target;     }

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) {}

    VOID InitSightingDatabase(BOOL *pCreated = NULL);
    VOID SaveState();

    CfxResource *GetResource()  { return _resource; }
    CfxDatabase *GetSightingDatabase();
    VOID FreeSightingDatabase(CfxDatabase *pDatabase);
    CfxDatabase *GetStateDatabase();
    VOID FreeStateDatabase(CfxDatabase *pDatabase);
    VOID SetDatabaseCaching(BOOL Enabled);

    VOID RegisterEvent(SESSIONEVENT Event, CfxControl *pControl, NotifyMethod Method);
    VOID UnregisterEvent(SESSIONEVENT Event, CfxControl *pControl);

    VOID RegisterRetainState(CfxControl *pControl, CctRetainState *pRetainState);
    VOID UnregisterRetainState(CfxControl *pControl);

    BOOL CanDoBack();
    VOID DoBack();
    BOOL CanDoNext();
    VOID DoNext();
    VOID DoCommit(XGUID TargetScreenId, BOOL DoRepaint=TRUE);
    VOID DoCommitInplace();
    VOID DoSkipToScreen(XGUID TargetScreenId);
    VOID DoJumpToScreen(XGUID BaseScreenId, XGUID JumpScreenId);
    VOID DoHome(XGUID TargetScreenId);

    VOID DoStartEdit(DBID SightingId);
    VOID DoAcceptEdit();
    VOID DoCancelEdit();

    VOID ShowDialog(const GUID *pDialogId, CfxControl *pSender, VOID *pParam, UINT ParamSize);

    VOID Fault(BOOL Condition, CHAR *pError);

    VOID ShowMessage(const CHAR *pMessage, COLOR BackColor = 0x00800000);
    VOID ShowUpdated();
    BOOL HasFaulted() { return _faulted; }
    BOOL HasMessage() { return GetMessageWindow()->GetVisible(); }

    CHAR *GetUserName() { return _userName; }
    CHAR *GetPassword() { return _password; }

    VOID SetCredentials(CHAR *pUserName, CHAR *pPassword);
    BOOL StartLogin(CHAR *pUserName, CHAR *pPassword);
    VOID Confirm(CfxControl *pControl, CHAR *pTitle, CHAR *pMessage);
    VOID Password(CfxControl *pControl);
    VOID PortSelect(CfxControl *pControl, BYTE PortId);

    UINT InsertScreen(XGUID ScreenId);
    UINT AppendScreen(XGUID ScreenId);
    VOID DeleteScreen(UINT Index);

    UINT MergeAttributes(UINT ControlId, ATTRIBUTES *pNewAttributes, ATTRIBUTES *pOldAttributes = NULL);

    CfxScreen *GetWindow();
    CctWaypointManager *GetWaypointManager() { return &_waypointManager;   }
    VOID RestartWaypointTimer();
    BOOL StoreWaypoint(WAYPOINT *pWaypoint, BOOL CheckTime = TRUE);
    VOID EnumWaypoints(CfxControl *pControl, EnumWaypointCallback Callback);
    CctTransferManager *GetTransferManager() { return &_transferManager;   }
    CctUpdateManager *GetUpdateManager() { return &_updateManager; }
    BOOL UseSDStorage();

    BOOL EnumHistoryInit(HISTORY_ITEM **ppHistoryItemDef);
    BOOL EnumHistorySetIndex(UINT Index);
    BOOL EnumHistoryNext(HISTORY_ITEM **ppHistoryItem);
    VOID EnumHistoryDone();
    VOID EnumHistoryWaypoints(CfxControl *pControl, EnumHistoryWaypointCallback Callback);
    UINT GetHistoryItemCount()         { return _historyItemCount;         }
    HISTORY_ROW *GetHistoryItemRow(HISTORY_ITEM *pHistoryItem, UINT Index);

    UINT GetDataUniqueness()           { return _dataUniqueness;           }
    VOID IncDataUniqueness()           { _dataUniqueness++;                }
    VOID ResetDataUniqueness()         { _dataUniqueness = 0;              }

    ATTRIBUTES *GetCurrentAttributes() { return _sighting.GetAttributes(); }
    CctSighting *GetCurrentSighting()  { return &_sighting;                }
    UINT GetSightingCount();
    DBID GetCurrentSightingId()        { return _sightingId;               }
    GUID *GetCurrentSightingUniqueId() { return _sighting.GetUniqueId();   }
    XGUID *GetCurrentScreenId();
    XGUID *GetFirstScreenId()          { return &_firstScreenId;           }
    FXGPS_POSITION *GetLastSightingGPS() { return &_lastGPS;               }

    VOID ClearStore();
    STOREITEM *GetStoreItem(GUID PrimaryId, UINT ControlId);
    STOREITEM *SetStoreItem(GUID PrimaryId, UINT ControlId, VOID *pData);
    VOID SaveControlState(CfxControl *pControl, BOOL Flush = TRUE);
    VOID LoadControlState(CfxControl *pControl); 
    VOID LoadControlStates();
    VOID SaveControlStates();

    VOID ClearGlobalValues();
    GLOBALVALUE *EnumGlobalValues(UINT Index);
    GLOBALVALUE *GetGlobalValue(const CHAR *pName);
    VOID SetGlobalValue(CfxPersistent *pSender, const CHAR *pName, DOUBLE Value);

    VOID ClearCustomGotos()                 { _customGotos.Clear();              }
    UINT AddCustomGoto(FXGOTO *pGoto)       { return _customGotos.Add(*pGoto);   }
    VOID DeleteCustomGoto(UINT Index)       { _customGotos.Delete(Index);        }
    UINT GetCustomGotoCount()               { return _customGotos.GetCount();    }
    FXGOTO *GetCustomGoto(UINT Index)       { return _customGotos.GetPtr(Index); }
    
    CHAR *GetPendingGotoTitle()             { return _pendingGotoTitle;          }
    VOID SetPendingGotoTitle(CHAR *pTitle)  { MEM_FREE(_pendingGotoTitle); _pendingGotoTitle = AllocString(pTitle); }

    BOOL GetShareDataEnabled()              { return _shareDataEnabled;          }
    VOID SetShareDataEnabled(BOOL Value)    { _shareDataEnabled = Value;         }

    CHAR *GetSightingAccuracyName()         { return _sightingAccuracyName;      }
    FXGPS_ACCURACY *GetSightingAccuracy()   { return &_sightingAccuracy;         }
    VOID SetSightingAccuracy(FXGPS_ACCURACY *pValue)    { _sightingAccuracy = *pValue; }
    UINT GetSightingFixCount()              { return _sightingFixCount;          }
    VOID SetSightingFixCount(UINT Value)    { _sightingFixCount = Value;         }

    BOOL GetRangeFinderConnected()          { return _rangeFinderConnected;      }
    VOID SetRangeFinderConnected(BOOL Value);

    BOOL GetTransferOnSave()                { return _transferOnSave;            }
    VOID SetTransferOnSave(BOOL Value)      { _transferOnSave = Value;           }

    BOOL WriteElementsFile(CHAR *pFileName);

    CHAR *AllocMediaFileName(CHAR *pMediaName);
    BOOL DeleteMedia(CHAR *pMediaName, BOOL KeepBackup = FALSE);
    BOOL BackupMedia(CHAR *pMediaName);
    CHAR *GetMediaName(CHAR *pFileName);

    CHAR *GetOutgoingPath()                   { return _outgoingPath; }
    CHAR *AllocTempMaxPath(BOOL Clear);
    CHAR *GetApplicationPath()                { return _applicationPath; }
    CHAR *GetBackupPath()                     { return _sdPath; }

    CHAR *GetUpdateTag();   // Tag from the update def file
    CHAR *GetUpdateUrl();   // Url to use for the update def file

    VOID ExecuteAction(CfxAction *pAction);
};

extern VOID DrawWaypointIcon(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Value, COLOR BackColor, COLOR ForeColor, BOOL Fill, BOOL StrikeOut, BOOL Invert);

extern BOOL RegisterSessionEvent(SESSIONEVENT Event, CfxControl *pControl, NotifyMethod Method);
extern BOOL UnregisterSessionEvent(SESSIONEVENT Event, CfxControl *pControl);
extern BOOL RegisterRetainState(CfxControl *pControl, CctRetainState *pRetainState);
extern BOOL UnregisterRetainState(CfxControl *pControl);

extern CfxControl *Create_Control_Session(CfxPersistent *pOwner, CfxControl *pParent);

// Utility API
extern CctSession *GetSession(CfxControl *pControl);
extern BOOL IsSessionEditing(CfxControl *pControl);

#ifdef CLIENT_DLL
extern VOID HackFixResultElementLockProperties(CHAR **ppLockProperties);
#else
#define HackFixResultElementLockProperties(x);
#endif
