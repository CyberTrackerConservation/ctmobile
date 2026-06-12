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
const int MAGIC_SESSION = 'SESS';

// Version control
const int RESOURCE_VERSION = 515;

// Session stuff
const GUID GUID_CONTROL_SESSION   = {0x82e911e2, 0x528f, 0x4d64, {0xbd, 0x81, 0x04, 0x5a, 0x2a, 0x04, 0xef, 0x0d}};

const double WAYPOINT_DEFAULT_GPS_ACCURACY = GPS_DEFAULT_ACCURACY;
const double SIGHTING_DEFAULT_GPS_ACCURACY = GPS_DEFAULT_ACCURACY;
const int SIGHTING_DEFAULT_FIX_COUNT      = 1;

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
    char ServerFileName[256] = {};
    GUID SequenceId;
    FXGPS_ACCURACY SightingAccuracy;
    int SightingFixCount = 0;
    FXGPS_ACCURACY WaypointAccuracy;
    int WaypointTimer = 0;
    uint8_t ManualMapOnSkip = 0;
    uint8_t ForceTitleBar = 0;
    uint8_t UseSD = 0;
    BYTE Padding = 0;
    FXSENDDATA SendData;
    uint8_t AllowManualSend = 0;
    uint8_t ClearDataOnSend = 0;
    int AutoSendTimeout = 0;
    uint8_t TestTime = 0;
    uint8_t GpsTimeSync = 0;
    int GpsTimeZone = 0;
    int GpsSkipTimeout = 0;
    uint8_t ManualOnSkip = 0;
    uint8_t ManualAllowSkip = 0;
    uint8_t KioskMode = 0;
    uint8_t SimpleCamera = 0;
    BYTE Padding1 = 0;
    BYTE Padding2 = 0;
    BYTE Padding3 = 0;
    uint8_t DisableEditing = 0;
    uint8_t ResetStateOnSync = 0;
    uint8_t ClearOnNext = 0;
    BYTE Projection = 0;

    BYTE UTMZone = 0;
    char OwnerName[128] = {};
    char OwnerOrganization[128] = {};
    char OwnerAddress1[64] = {};
    char OwnerAddress2[64] = {};
    char OwnerAddress3[64] = {};
    char OwnerPhone[64] = {};
    char OwnerEmail[64] = {};
    char OwnerWebSite[64] = {};

    char WebUpdateUrl[256] = {};
    int WebUpdateFrequency = 0;
    uint8_t WebUpdateCheckOnLaunch = 0;
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
    int ControlId = 0;
    void *Data = nullptr;
};

struct GLOBALVALUE
{
    char Name[32] = {};
    double Value = 0.0;
};

struct DIALOGSTATE
{
    GUID Id;
    void *StateData = nullptr;
};

struct GLOBAL_CHANGED_PARAMS
{
    CfxPersistent *pSender = nullptr;
    const char *Name = nullptr;
    double OldValue = 0.0, NewValue = 0.0;
};

const char GPS_WAYPOINT_FONT[] = "Arial Narrow,7,";

//*************************************************************************************************

class CctSession;

//*************************************************************************************************
//
// CctJsonBuilder
//
class CctJsonBuilder
{
protected:
    CctSession *_session = nullptr;
    CfxFileStream *_fileStream = nullptr;
    int _protocol = 0;
    static void ResolveElement(CctSession *pSession, CfxResource *pResource, XGUID ElementId, QString &Name);
    bool WriteSightingGeoJson(CctSighting *pSighting, FXALERT *pAlert, bool First, FXFILELIST *pMediaFileList);
    bool WriteSightingEsri(CctSighting *pSighting, bool First, FXFILELIST *pMediaFileList);
public:
    CctJsonBuilder(CctSession *pSession, CfxFileStream *pFileStream, int Protocol);

    bool WriteHeader();
    bool WriteSighting(CctSighting *pSighting, FXALERT *pAlert, bool First, FXFILELIST *pMediaFileList);
    bool WriteWaypoint(WAYPOINT *pWaypoint, bool First);
    bool WriteFooter();

    static void WriteSightingToStream(CctSession *pSession, CctSighting *pSighting, CfxStream &Data);
};

//*************************************************************************************************
//
// CctAlert
//
class CctAlert: public CfxPersistent
{
protected:
    bool _active = false;
    char *_url = nullptr;
    char *_userName = nullptr, *_password = nullptr;
    char *_caId = nullptr;
    char *_description = nullptr, *_type = nullptr;
    int _level = 0;
    int _protocol = 0;
    XGUIDLIST _matchElements;

    int _pingFrequency = 0;        // 0 means one-time alert, otherwise it's a ping
    bool _pingOnly = false;
    XGUID _patrolElementId = 0;     // Element value to add to alert

    bool _committed = false;            // If Commit() returns true on sighting save
    char *_patrolValue = nullptr;         // Computed PatrolElementId from sighting
    FXALERT _alert;             // Built from above attributes

    bool Build(CctSighting *pSighting, FXALERT *pAlert);
    bool Match(CctSighting *pSighting);

public:
    CctAlert(CfxPersistent *pOwner);
    ~CctAlert();

    int GetPingFrequency() { return _pingFrequency; }

    bool GetCommitted()     { return _committed;     }

    bool Commit(CctSession *pSession, CctSighting *pSighting);
    void ActivateAlarm();

    bool Compare(CctAlert *pAlert);

    void DefineState(CfxFiler &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnAlarm() override;
};

//*************************************************************************************************
//
// CctRetainState
//
class CctRetainState: public CfxPersistent
{
protected:
    CfxControl *_parent = nullptr;
    bool _retain = false;
    char *_globalValue = nullptr;
    double _lastValue = 0.0;
public:
    CctRetainState(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctRetainState();

    void SetRetain(bool Value) { _retain = Value; }
    void SetGlobalValue(const char *Value) { MEM_FREE(_globalValue); _globalValue = AllocString(Value); }

    bool MustRetain() { return _retain; }
    bool MustResetState();

    void DefineState(CfxFiler &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
};

struct RETAINSTATE
{
    CfxControl *Control = nullptr;
    CctRetainState *RetainState = nullptr;
};

//*************************************************************************************************
//
// CctTransferManager
//
class CctTransferManager: public CfxPersistent
{
    Q_OBJECT

protected:
    int _autoSendTimeout = 0;
    TList<CctAlert *> _alerts;

    void ExportLegacyFiles();

    CctSession *GetSession() { return qobject_cast<CctSession *>(GetOwner()); }
    bool CreateFileInQueue(const char *pTargetFileName, const char *pTargetFileExtension, BYTE *pData, int DataSize);
    bool CreateFileInQueue(const char *pTargetFileName, const char *pTargetFileExtension, const char *pSourceFileName, bool Compress, bool CopyNotMove);
    bool PushToOutgoing(const char *pFileName, const char *pFileExtension, FXSENDDATA *pSendData);
    bool PushMediaToOutgoing(const char *pSourceFileName, const char *pTargetFileName, int TargetId, FXSENDDATA *pSendData);
public:
    CctTransferManager(CfxPersistent *pOwner);
    ~CctTransferManager();

    void SetAutoSendTimeout(int Value) { _autoSendTimeout = Value; }
    void Reset();

    void DefineState(CfxFiler &F) override;

    bool HasOutgoingData();
    bool HasTransferData();
    bool HasExportData();
    bool HasPendingAlert();

    void ClearAlerts();
    void AddAlert(CctAlert *pAlert);
    void RemoveAlert(CctAlert *pAlert);
    void CommitAlerts(CctSighting *pSighting);

    char *BuildFileName(const char *pPrefix = nullptr);

    bool HasUnsentData();

    bool CreateCTX(char *pFileName, FXFILELIST *pOutMediaFileList, bool *pOutHasData, FXEXPORTFILEINFO* pExportFileInfo = nullptr);
    bool CreateJSON(char *pFileName, int Protocol, const QByteArray& classicBlob, FXFILELIST *pOutMediaFileList);
    bool CreateJSONWaypoints(char *pFileName, int Protocol);

    void MakeWAY(char *pFilePath, FXEXPORTFILEINFO* pExportFileInfo);

    bool CreateTransfer(bool ForceClear = false);
    bool CreateAlert(CctSighting *pSighting, FXALERT *pAlert);
    bool CreatePing(FXALERT *pAlert);

    void Update();
    void Disconnect();

    void OnAlarm() override;

    static bool FlushData(const QString& projectUid);
};

//*************************************************************************************************
//
// CctUpdateManager
//
class CctUpdateManager: public CfxPersistent
{
    Q_OBJECT

private:
    double _lastCheckTime = 0.0;
    double _lastUpdateTime = 0.0;
public:
    CctUpdateManager(CfxPersistent *pOwner);

    void DefineState(CfxFiler &F) override;

    static void Install(const char *pSrcPath, const char *pDstPath, const char *pAppName, const char *pTag, const char *pUrl);
};

//*************************************************************************************************
//
// CctSightingDatabase
//
class CctSightingDatabase: public CfxPersistent
{
public:
    struct Entry {
        QString sightingUid;
        QByteArray classicBlob;
    };

    CctSightingDatabase(CfxPersistent *pOwner);
    ~CctSightingDatabase() override;

    bool Load(CctSighting *pSighting, const QString &sightingUid);
    bool Enum(int Index, QString *pSightingUid);
    int GetCount();
    bool TestId(const QString &sightingUid);
    bool GetIndex(const QString &sightingUid, int *pIndex);

    void MakeExportDAT(char *pFilePath, FXFILELIST *pOutMediaFileList, FXEXPORTFILEINFO* pExportFileInfo);
    void MakeDAT();
    void MarkUploaded();

    bool HasUnsentData();
    void DeleteAll();
    bool DeleteOne(const QString& sightingUid);

private:
    QList<Entry> _entries;
    QMap<QString, int> _uidToIndex;
    bool _loaded = false;
    void EnsureLoaded();
};

//*************************************************************************************************

//
// CctSession
//
typedef void (CfxPersistent::*EnumWaypointCallback)(WAYPOINT *pWaypoint);
typedef void (CfxPersistent::*EnumHistoryWaypointCallback)(HISTORY_WAYPOINT *pHistoryWaypoint);

class CctSession: public CfxControl
{
    Q_OBJECT

protected:
    CfxScreen *_screen = nullptr, *_screenRight = nullptr;
    CfxControl_Button *_messageWindow = nullptr;

    CfxResource *_resource = nullptr;
    char _name[MAX_PATH] = {};
    char *_sdPath = nullptr;
    char *_applicationPath = nullptr;
    char *_mediaPath = nullptr;
    char *_outgoingPath = nullptr;
    char *_tempPath = nullptr;

    char *_userName = nullptr;
    char *_password = nullptr;

    int _stateSlot = 0;
    XGUID _firstScreenId = 0;

    TList<STOREITEM> _store;
    TList<GLOBALVALUE> _globalValues;
    TList<RETAINSTATE> _retainStates;

    char *_sightingAccuracyName = nullptr;
    FXGPS_ACCURACY _sightingAccuracy;
    int _sightingFixCount = 0;
    XGUID _defaultSave1Target = 0, _defaultSave2Target = 0;

    FXGPS_POSITION _lastGPS;
    bool _saving = false;
    bool _transferOnSave = false;

    bool _shareDataEnabled = false;

    CfxEventManager _events;
    CfxActionManager _actions;
    CfxActionManager _stateActions;
    CctSighting _sighting;
    CctSighting _backupSighting;
    CctTransferManager _transferManager;
    CctUpdateManager _updateManager;

    DBID _sightingId = INVALID_DBID;
    GUID _resourceStampId;

    TList<FXGOTO> _customGotos;
    char *_pendingGotoTitle = nullptr;

    bool _connected = false;
    bool _faulted = false;
    COLOR _messageBackColor = 0;
    char _messageText[128] = {};
    TList<DIALOGSTATE> _dialogStates;

    HISTORY_ITEM *_historyItemDef = nullptr, *_historyItem = nullptr;
    int _historyItemCount = 0, _historyItemIndex = 0;
    int _historyItemSize = 0;

    WAYPOINT _lastWaypoint;

    bool MatchProfileExact(CTRESOURCEHEADER *pHeader, int Width, int Height, int DPI, FXPROFILE *pMatch);
    bool MatchProfileFuzzy(CTRESOURCEHEADER *pHeader, int Width, int Height, int DPI, FXPROFILE *pMatch);

    void ClearDialogs();

    bool InitPaths();
    void FreePaths();

    bool SD_Initialize();
    void SD_Finalize();
    bool SD_WriteSighting(CctSighting *pSighting);
    bool ValidateSightingHeader(CfxDatabase *SightingDatabase);
    void ForceTitleBar(CfxScreen *pScreen);

    void LoadState();
    void LoadScreen(CfxScreen *pScreen, XGUID ScreenId, void *pState);
    void SaveScreen(CfxScreen *pScreen, XGUID ScreenId, int PathIndex);
    void LoadCurrentScreen();
    void LoadCurrentScreenAndState();

    CfxControl_Button *GetMessageWindow();

    void OnShowMenu(CfxPersistent *pSender, void *pParams);
    void OnMessageClick(CfxControl *pSender, void *pParams);
    void OnMessagePaint(CfxControl *pSender, void *pParams);

    void ProcessElementEffects();

    void LeaveScreen();
    void EnterScreen();

public:
    CctSession(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctSession();

    bool GetConnected()          { return _connected; }
    GUID GetCurrentSequenceId();
    bool Connect(char *pName, int StateSlot, XGUID FirstScreenId = XGUID_0);
    void Disconnect();
    void Reconnect(const QString &sightingUid = QString(), bool DoRepaint = true);
    void Run();

    CTRESOURCEHEADER *GetResourceHeader() { return (CTRESOURCEHEADER *)_resource->GetHeader(this); }

    char *GetName()     { return _name;                        }
    bool IsEditing()    { return _sightingId != INVALID_DBID;  }
    bool IsSaving()     { return _saving;                      }
    bool InDialog()     { return _dialogStates.GetCount() > 0; }

    void GetDefaultSaveTargets(XGUID *pSave1Target, XGUID *pSave2Target) { *pSave1Target = _defaultSave1Target; *pSave2Target = _defaultSave2Target; }
    void SetDefaultSaveTargets(XGUID Save1Target, XGUID Save2Target)     { _defaultSave1Target = Save1Target; _defaultSave2Target = Save2Target;     }

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void SetBounds(int Left, int Top, int Width, int Height) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override {}

    bool InitSightingDatabase();
    void SaveState();
    void SaveCurrentScreen();

    // Session state serialization (used by ClassicProvider::save/load).
    QByteArray SaveStateBlob(QStringList* attachments);
    void LoadStateBlob(const QByteArray& blob);

    CfxResource *GetResource() override { return _resource; }

    CfxDatabase *GetSightingDatabase(bool *pCreated = nullptr);
    void FreeSightingDatabase(CfxDatabase *pDatabase);
    std::unique_ptr<CctSightingDatabase> CreateSightingDatabasePtr();

    void RegisterEvent(SESSIONEVENT Event, CfxControl *pControl, NotifyMethod Method);
    void UnregisterEvent(SESSIONEVENT Event, CfxControl *pControl);

    void RegisterRetainState(CfxControl *pControl, CctRetainState *pRetainState);
    void UnregisterRetainState(CfxControl *pControl);

    bool CanDoBack();
    void DoBack();
    bool CanDoNext();
    void DoNext();
    void DoCommit(XGUID TargetScreenId, bool DoRepaint=true);
    void DoCommitInplace();
    void DoSkipToScreen(XGUID TargetScreenId);
    void DoJumpToScreen(XGUID BaseScreenId, XGUID JumpScreenId);
    void DoHome(XGUID TargetScreenId);

    void DoStartEdit(const QString &sightingUid);
    void DoAcceptEdit();
    void DoCancelEdit();

    void ShowDialog(const GUID *pDialogId, CfxControl *pSender, void *pParam, int ParamSize);

    void Fault(bool Condition, const char *pError);

    void ShowMessage(const char *pMessage, COLOR BackColor = 0x00800000);
    void ShowUpdated();
    bool HasFaulted() { return _faulted; }
    bool HasMessage() { return GetMessageWindow()->GetVisible(); }

    char *GetUserName() { return _userName; }
    char *GetPassword() { return _password; }

    void SetCredentials(char *pUserName, char *pPassword);
    void PortSelect(CfxControl *pControl, BYTE PortId);

    int InsertScreen(XGUID ScreenId);
    int AppendScreen(XGUID ScreenId);
    void DeleteScreen(int Index);

    int MergeAttributes(int ControlId, ATTRIBUTES *pNewAttributes, ATTRIBUTES *pOldAttributes = nullptr);

    CfxScreen *GetWindow();
    CctTransferManager *GetTransferManager() { return &_transferManager;   }

    bool UseSDStorage();

    bool EnumHistoryInit(HISTORY_ITEM **ppHistoryItemDef);
    bool EnumHistorySetIndex(int Index);
    bool EnumHistoryNext(HISTORY_ITEM **ppHistoryItem);
    void EnumHistoryDone();
    void EnumHistoryWaypoints(CfxControl *pControl, EnumHistoryWaypointCallback Callback);
    int GetHistoryItemCount()         { return _historyItemCount;         }
    HISTORY_ROW *GetHistoryItemRow(HISTORY_ITEM *pHistoryItem, int Index);

    bool EditSighting(const QString& sightingUid);
    bool DeleteSighting(const QString& sightingUid);

    ATTRIBUTES *GetCurrentAttributes() { return _sighting.GetAttributes(); }
    CctSighting *GetCurrentSighting()  { return &_sighting;                }
    int GetSightingCount();
    DBID GetCurrentSightingId()        { return _sightingId;               }
    QString GetCurrentSightingUid();

    GUID *GetCurrentSightingUniqueId() { return _sighting.GetUniqueId();   }
    XGUID *GetCurrentScreenId();
    XGUID *GetFirstScreenId()          { return &_firstScreenId;           }
    FXGPS_POSITION *GetLastSightingGPS() { return &_lastGPS;               }

    void ClearStore();
    STOREITEM *GetStoreItem(GUID PrimaryId, int ControlId);
    STOREITEM *SetStoreItem(GUID PrimaryId, int ControlId, void *pData);
    void SaveControlState(CfxControl *pControl, bool Flush = true);
    void LoadControlState(CfxControl *pControl); 
    void LoadControlStates();
    void SaveControlStates();

    void ClearGlobalValues();
    GLOBALVALUE *EnumGlobalValues(int Index);
    GLOBALVALUE *GetGlobalValue(const char *pName);
    void SetGlobalValue(CfxPersistent *pSender, const char *pName, double Value);

    void ClearCustomGotos()                 { _customGotos.Clear();              }
    int AddCustomGoto(FXGOTO *pGoto)       { return _customGotos.Add(*pGoto);   }
    void DeleteCustomGoto(int Index)       { _customGotos.Delete(Index);        }
    int GetCustomGotoCount()               { return _customGotos.GetCount();    }
    FXGOTO *GetCustomGoto(int Index)       { return _customGotos.GetPtr(Index); }
    
    char *GetPendingGotoTitle()             { return _pendingGotoTitle;          }
    void SetPendingGotoTitle(char *pTitle)  { MEM_FREE(_pendingGotoTitle); _pendingGotoTitle = AllocString(pTitle); }

    bool GetShareDataEnabled()              { return _shareDataEnabled;          }
    void SetShareDataEnabled(bool Value)    { _shareDataEnabled = Value;         }

    char *GetSightingAccuracyName()         { return _sightingAccuracyName;      }
    FXGPS_ACCURACY *GetSightingAccuracy()   { return &_sightingAccuracy;         }
    void SetSightingAccuracy(FXGPS_ACCURACY *pValue)    { _sightingAccuracy = *pValue; }
    int GetSightingFixCount()              { return _sightingFixCount;          }
    void SetSightingFixCount(int Value)    { _sightingFixCount = Value;         }

    bool GetTransferOnSave()                { return _transferOnSave;            }
    void SetTransferOnSave(bool Value)      { _transferOnSave = Value;           }

    bool WriteElementsFile(char *pFileName);

    char *AllocMediaFileName(const char *pMediaName);
    bool DeleteMedia(char *pMediaName, bool KeepBackup = false);
    bool BackupMedia(char *pMediaName);
    char *GetMediaName(char *pFileName);

    char *GetOutgoingPath()                   { return _outgoingPath; }
    char *AllocTempMaxPath(bool Clear);
    char *GetApplicationPath()                { return _applicationPath; }
    char *GetBackupPath()                     { return _sdPath; }

    char *GetUpdateTag();   // Tag from the update def file
    char *GetUpdateUrl();   // Url to use for the update def file

    void ExecuteAction(CfxAction *pAction);

    void ShowExports();
    void ShowUploadQueue();
    void ShowMapPage();
    void ShowSightingEditorPage();
    void ShowTextEditorPage(int ControlId);
    void ShowGPSReaderPage(XGUID SaveTarget, int SkipTimeout, bool SkipAllowed);
    void ShowConfirm(int ControlId, const char *pMessage);
    void ShareData();
    void SubmitData();
};

extern void DrawWaypointIcon(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Value, COLOR BackColor, COLOR ForeColor, bool Fill, bool StrikeOut, bool Invert);

extern bool RegisterSessionEvent(SESSIONEVENT Event, CfxControl *pControl, NotifyMethod Method);
extern bool UnregisterSessionEvent(SESSIONEVENT Event, CfxControl *pControl);
extern bool RegisterRetainState(CfxControl *pControl, CctRetainState *pRetainState);
extern bool UnregisterRetainState(CfxControl *pControl);

extern CfxControl *Create_Control_Session(CfxPersistent *pOwner, CfxControl *pParent);

// Utility API
extern CctSession *GetSession(CfxControl *pControl);
extern bool IsSessionEditing(CfxControl *pControl);

#define HackFixResultElementLockProperties(x);
