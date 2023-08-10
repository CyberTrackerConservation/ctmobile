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

#include "fxHost.h"
#include "fxApplication.h"
#include "QtUtils.h"
#include "QtCanvas.h"
#include "QtTransfer.h"
#include "QtSessionItem.h"

#include "Location.h"
#include "WaveFileRecorder.h"

class CHost_Qt: public CfxHost
{
protected:
    CtClassicSessionItem *_window;

    GUID _deviceId;

    QMutex _renderMutex;

    CHAR *_applicationPath;
    UINT _batteryLevel, _batteryState, _powerState;
    bool _gpsOn = false;
    FXGPS _gps;
    UINT _gpsTimeStamp;

    DOUBLE _gpsTimeOffset;
    QMap<UINT, QTimer*> _timers;

    QString _recordingFileName;
    WaveFileRecorder _audioRecorder;
    QTimer _audioTimer;

    QMediaPlayer _mediaPlayer;

    FXFILEMAP _enumFileMap;
    UINT _enumFileIndex;

    QtTransfer _transfer;
    FXSENDSTATE _transferState = {SS_IDLE, 0, 0, ""};
    void transferStateChanged(int state);

    QGeoPositionInfoSource* m_positionSource = nullptr;
    QGeoPositionInfoSource* m_positionSourcePing = nullptr;
    QTcpSocket m_simulateSocket;

    QNetworkAccessManager _networkAccessManager;
    QList<int> _networkRequests;
    int _networkRequestMaxId = 1;

    int _trackTimeout = 0;
    LocationStreamer _trackStreamer;
    QTimer _trackTimerHandler;

private slots:
    void onTrackTimer(Location* update);

public:
    CHost_Qt(CfxPersistent *pOwner, CtClassicSessionItem *pWindow, FXPROFILE *pProfile, const CHAR *pApplicationPath);
    ~CHost_Qt();

    CfxDatabase *CreateDatabase(CHAR *pDatabaseName);
    CfxResource *CreateResource(CHAR *pResourceName);
    CfxCanvas *CreateCanvas(UINT Width, UINT Height);

    VOID RequestMediaScan(const CHAR *pFileName);

    BOOL SnapDocuments();
    BOOL DeleteDocument(GUID DocumentId);

    VOID SetKioskMode(BOOL Enabled);

    VOID SetTrackTimer(INT Timeout);

    VOID Startup();
    VOID Shutdown();
    VOID Paint();

    BOOL HasExternalCloseButton();
    VOID ShowSIP();
    VOID HideSIP();

    HANDLE CreateEditControl(BOOL SingleLine = FALSE, BOOL Password = FALSE);
    VOID DestroyEditControl(HANDLE Control);
    VOID MoveEditControl(HANDLE Control, INT Left, INT Top, UINT Width, UINT Height);
    VOID FocusEditControl(HANDLE Control);
    CHAR *GetEditControlText(HANDLE Control);
    VOID SetEditControlText(HANDLE Control, CHAR *pValue);
    VOID SetEditControlFont(HANDLE Control, FXFONTRESOURCE *pFont);
    
    BOOL IsCameraSupported();
    BOOL ShowCameraDialog(CHAR *pFileNameNoExt, FXIMAGE_QUALITY ImageQuality);

    VOID ShowSkyplot(INT Left, INT Top, UINT Width, UINT Height, BOOL Visible);
    VOID ShowToast(const CHAR* pMessage);
    VOID ShowExports();
    VOID ShareData();

    BOOL IsBarcodeSupported();
    BOOL ShowBarcodeDialog(CHAR *pBarcode);

    BOOL IsEsriSupported();
    BOOL TestEsriCredentials(CHAR *pUserName, CHAR *pPassword);

    BOOL IsSoundSupported()         { return TRUE; }
    BOOL PlaySound(FXSOUNDRESOURCE *pSound);
    BOOL StopSound();
    BOOL IsSoundPlaying();

    BOOL RequestPermissionRecordAudio();
    BOOL StartRecording(CHAR *pFileNameNoExt, UINT Duration, UINT Frequency);
    BOOL StopRecording();
    BOOL PlayRecording(CHAR *pFileName);
    VOID CancelRecording();
    BOOL IsRecording();

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    BOOL GetDeviceId(GUID *pGuid);
    CHAR *GetDeviceName();

    BOOL CreateGuid(GUID *pGuid);

    VOID GetDateTime(FXDATETIME *pDateTime);
    VOID SetDateTime(FXDATETIME *pDateTime);
    VOID SetTimeZone(INT BiasInMinutes);
    VOID SetDateTimeFromGPS(FXDATETIME *pDateTime);
    CHAR *GetDateString(FXDATETIME *pDateTime);
    CHAR *GetDateStringUTC(FXDATETIME *pDateTime, BOOL FormatAsUTC = FALSE);

    VOID SetTimer(UINT Elapse, UINT TimerId);
    VOID KillTimer(UINT TimerId);

    VOID SetAlarm(UINT ElapseInSeconds);
    VOID KillAlarm();

    VOID RequestUrl(CHAR *pUrl, CHAR *pUserName, CHAR *pPassword, CfxStream &Data, CHAR *pFileName, UINT *pId);
    VOID CancelUrl(UINT Id);

    BOOL ConnectPort(BYTE PortId, FX_COMPORT *pComPort);
    BOOL IsPortConnected(BYTE PortId);
    VOID DisconnectPort(BYTE PortId);
    VOID WritePortData(BYTE PortId, BYTE *pData, UINT Length);

    VOID StartTransfer(CHAR *pOutgoingPath);
    VOID CancelTransfer();
    VOID GetTransferState(FXSENDSTATE *pState);
    
    VOID SetPowerState(FXPOWERMODE Mode);
    FXPOWERMODE GetPowerState();
    FXBATTERYSTATE GetBatteryState();
    UINT8 GetBatteryLevel();
    VOID SetBatteryState(UINT Level, UINT State, UINT PowerState);

    UINT GetFreeMemory();

    FXGPS *GetGPSPtr()      { return &_gps; }
    VOID GPSDataReceived()  { _gpsTimeStamp = FxGetTickCount(); }
    VOID ResetGPSDataReceived() { _gpsTimeStamp = 0; }    VOID GetGPS(FXGPS *pGPS);
    VOID SetGPS(BOOL OnOff);
    VOID ResetGPSTimeouts();
    BYTE GetGPSPowerupTimeSeconds();
    BOOL HasSmartGPSPower();

    VOID GetRange(FXRANGE *pRange);
    VOID SetRangeFinder(BOOL OnOff);
    VOID ResetRangeFinderTimeouts();

    BOOL AppendRecord(CHAR *pFileName, VOID *pRecord, UINT RecordSize);
    BOOL DeleteRecords(CHAR *pFileName);

    BOOL EnumRecordInit(CHAR *pFileName);
    BOOL EnumRecordSetIndex(UINT Index, UINT RecordSize);
    BOOL EnumRecordNext(VOID *pRecord, UINT RecordSize);
    BOOL EnumRecordDone();

    CHAR *AllocPathApplication(const CHAR *pFileName);
    CHAR *AllocPathSD();

    VOID RegisterExportFile(CHAR *pExportFilePath, FXEXPORTFILEINFO* exportFileInfo) override;
    VOID ArchiveSighting(GUID* pId, FXDATETIME* dateTime, FXGPS_POSITION* gps, const QVariantMap& data) override;
    VOID ArchiveWaypoint(GUID* pId, FXDATETIME* dateTime, FXGPS_POSITION* gps) override;
};
