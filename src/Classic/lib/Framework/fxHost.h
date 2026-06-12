#pragma once
#include "pch.h"
#include "fxTypes.h"
#include "fxCanvas.h"
#include "fxDatabase.h"
#include "fxControl.h"
#include "fxEngine.h"
#include "WaveFileRecorder.h"
#include "Location.h"

class ClassicView;

using namespace CyberTracker;

enum class FXPOWERMODE : int8_t {Off=0, Idle, On};
enum class FXBATTERYSTATE : int8_t {Critical=0, Low, High, Charge};

//
// CfxHost
//
class CfxHost: public CfxPersistent
{
protected:
    FXPROFILE _profile;
    int _left = 0, _top = 0;
    CfxCanvas *_canvas = nullptr;
    bool _useResourceScale = false;
    double _gpsTimeOffset = 0.0;

    ClassicView *_window = nullptr;
    GUID _deviceId;
    QMutex _renderMutex;
    char *_applicationPath = nullptr;
    int _batteryLevel = 0, _batteryState = 0, _powerState = 0;
    bool _gpsOn = false;
    FXGPS _gps;
    QMap<int, QTimer*> _timers;
    QString _recordingFileName;
    WaveFileRecorder _audioRecorder;
    QTimer _audioTimer;
    QMediaPlayer _mediaPlayer;
    FXFILEMAP _enumFileMap;
    int _enumFileIndex = 0;
    QGeoPositionInfoSource* m_positionSource = nullptr;
    QNetworkAccessManager _networkAccessManager;
    QList<int> _networkRequests;
    int _networkRequestMaxId = 1;

public:
    CfxHost(CfxPersistent *pOwner, ClassicView *pWindow, FXPROFILE *pProfile, const char *pApplicationPath);
    ~CfxHost();
    void SetBatteryState(int Level, int State, int PowerState);
    FXGPS *GetGPSPtr() { return &_gps; }

    void RequestMediaScan(const char *pFileName);

    void GetBounds(FXRECT *pRect);
    FXPROFILE *GetProfile()              { return &_profile; }

    bool GetUseResourceScale()           { return _useResourceScale;  }
    void SetUseResourceScale(bool Value) { _useResourceScale = Value; }

    CfxCanvas *GetCanvas();

    double GetGPSTimeOffset() { return _gpsTimeOffset; }
    void SetGPSTimeOffset(double Value) { _gpsTimeOffset = Value; }

    CfxDatabase *CreateDatabase(char *pDatabaseName);
    CfxResource *CreateResource(char *pResourceName);
    CfxCanvas *CreateCanvas(int Width, int Height);

    void Startup();
    void Shutdown();
    void Paint();

    bool GetTrackTimerRunning() const;
    int GetTrackTimer() const;
    void SetTrackTimer(int Timeout);

    bool HasExternalCloseButton();
    bool HasExternalStateData();

    void RemoveReplicatedMedia(char *pFileName);
    bool IsCameraSupported();
    bool ShowCameraDialog(char *pFileNameNoExt, FXIMAGE_QUALITY ImageQuality, bool SimpleMode);

    void ShowToast(const char *pMessage);

    bool IsUploadQueueBusy();

    bool IsBarcodeSupported();
    bool ShowBarcodeDialog(char *pBarcode);

    bool IsPhoneNumberSupported();
    bool ShowPhoneNumberDialog();

    bool IsEsriSupported();

    bool IsSoundSupported();
    bool PlaySound(FXSOUNDRESOURCE *pSound);
    bool StopSound();
    bool IsPlaying();

    bool RequestPermissionRecordAudio();
    bool StartRecording(char *pFileNameNoExt, int Duration, int Frequency);
    bool StopRecording();
    bool PlayRecording(char *pFileName);
    void CancelRecording();
    bool IsRecording();

    void SetBounds(int Left, int Top, int Width, int Height);
    bool GetDeviceId(GUID *pGuid);
    char *GetDeviceName();

    bool CreateGuid(GUID *pGuid);

    void GetDateTime(FXDATETIME *pDateTime);
    void SetDateTime(FXDATETIME *pDateTime);
    void SetTimeZone(int BiasInMinutes);
    void SetDateTimeFromGPS(FXDATETIME *pDateTime) {}
    char *GetDateString(FXDATETIME *pDateTime);
    static char *GetDateStringUTC(FXDATETIME *pDateTime, bool FormatAsUTC = false);

    void SetTimer(int Elapse, int TimerId);
    void KillTimer(int TimerId);

    void SetAlarm(int ElapseInSeconds);
    void KillAlarm();

    void RequestUrl(char *pUrl, char *pUserName, char *pPassword, CfxStream &Data, char *pFileName, int *pId);
    void CancelUrl(int Id);

    bool ConnectPort(BYTE PortId, FX_COMPORT *pComPort);
    bool IsPortConnected(BYTE PortId);
    void DisconnectPort(BYTE PortId);
    void WritePortData(BYTE PortId, BYTE *pData, int Length);

    void SetPowerState(FXPOWERMODE Mode);
    FXPOWERMODE GetPowerState();

    FXBATTERYSTATE GetBatteryState();
    int GetBatteryLevel();

    int GetFreeMemory();

    void GetGPS(FXGPS *pGPS);
    void SetGPS(bool OnOff);
    void ResetGPSTimeouts();
    void SetupGPSSimulator(double Latitude, double Longitude, int Radius);
    BYTE GetGPSPowerupTimeSeconds();
    bool HasSmartGPSPower();

    bool AppendRecord(char *pFileName, void *pRecord, int RecordSize);
    bool DeleteRecords(char *pFileName);
    bool EnumRecordInit(char *pFileName);
    bool EnumRecordSetIndex(int Index, int RecordSize);
    bool EnumRecordNext(void *pRecord, int RecordSize);
    bool EnumRecordDone();

    char *AllocPathApplication(const char *pFileName);
    char *AllocPathSD();

    bool IsUpdateSupported();
    void CheckForUpdate(GUID AppId, GUID StampId, char *pWebUpdateUrl, char *pTargetPath);

    void RegisterExportFile(char *pExportFilePath, FXEXPORTFILEINFO* exportFileInfo);

    void AddToUploadQueue(const QStringList& files, FXSENDDATA *sendData, FXEXPORTFILEINFO* exportFileInfo);
};
