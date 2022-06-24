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
#include "fxTypes.h"
#include "fxCanvas.h"
#include "fxDatabase.h"
#include "fxControl.h"
#include "fxEngine.h"

//
// CfxGPS
//
class CfxGPS
{
public:
    CfxGPS() {}
    virtual ~CfxGPS() {}
    virtual BOOL IsPresent()=0;
    virtual VOID ResetTimeouts()=0;
    virtual VOID GetState(FXGPS *pGPS)=0;
    virtual VOID SetState(BOOL TurnOn)=0;
    virtual WCHAR *GetDeviceName()=0;
    virtual BOOL CanIterate()=0;
    virtual VOID Iterate()=0;
};

//
// CfxRangeFinder
//
class CfxRangeFinder
{
public:
    CfxRangeFinder() {}
    virtual ~CfxRangeFinder() {}
    virtual BOOL IsPresent()=0;
    virtual VOID ResetTimeouts()=0;
    virtual VOID GetState(FXRANGE *pRange)=0;
    virtual VOID SetState(BOOL TurnOn)=0;
    virtual BOOL CanIterate()=0;
    virtual VOID Iterate()=0;
};

enum FXPOWERMODE {powOff, powIdle, powOn};
enum FXBATTERYSTATE {batCritical, batLow, batHigh, batCharge};

//
// CfxHost
//
class CfxHost: public CfxPersistent
{
protected:
    FXPROFILE _profile;
    INT _left, _top;
    CfxCanvas *_canvas;
    BOOL _useResourceScale;
    DOUBLE _gpsTimeOffset;
public:
    CfxHost(CfxPersistent *pOwner, FXPROFILE *pProfile);
    ~CfxHost();

    virtual VOID RequestMediaScan(const CHAR *pFileName) { }

    VOID GetBounds(FXRECT *pRect);
    FXPROFILE *GetProfile()              { return &_profile; }

    BOOL GetUseResourceScale()           { return _useResourceScale;  }
    VOID SetUseResourceScale(BOOL Value) { _useResourceScale = Value; }
    
    CfxCanvas *GetCanvas();

    DOUBLE GetGPSTimeOffset() { return _gpsTimeOffset; }
    VOID SetGPSTimeOffset(DOUBLE Value) { _gpsTimeOffset = Value; }

    virtual CfxDatabase *CreateDatabase(CHAR *pDatabaseName)=0;
    virtual CfxResource *CreateResource(CHAR *pResourceName)=0;
    virtual CfxCanvas *CreateCanvas(UINT Width, UINT Height)=0;

    virtual VOID TranslatePoint(INT *pX, INT *pY) {};
    virtual VOID Startup()=0;
    virtual VOID Shutdown()=0;
    virtual VOID Paint()=0;

    virtual VOID SetKioskMode(BOOL Enabled) {};

    virtual VOID SetTrackTimer(INT Timeout) {}

    virtual BOOL HasExternalCloseButton();
    virtual BOOL HasExternalStateData();
    virtual VOID HideSIP();
    virtual VOID ShowSIP();

    virtual HANDLE CreateEditControl(BOOL SingleLine = FALSE, BOOL Password = FALSE)=0;
    virtual VOID DestroyEditControl(HANDLE Control)=0;
    virtual VOID MoveEditControl(HANDLE Control, INT Left, INT Top, UINT Width, UINT Height)=0;
    virtual VOID FocusEditControl(HANDLE Control)=0;
    virtual CHAR *GetEditControlText(HANDLE Control)=0;
    virtual VOID SetEditControlText(HANDLE Control, CHAR *pValue)=0;
    virtual VOID SetEditControlFont(HANDLE Control, FXFONTRESOURCE *pFont)=0;

    virtual BOOL IsCameraSupported();
    virtual BOOL ShowCameraDialog(CHAR *pFileNameNoExt, FXIMAGE_QUALITY ImageQuality);

    virtual VOID ShowSkyplot(INT Left, INT Top, UINT Width, UINT Height, BOOL Visible);

    virtual VOID ShowExports();

    virtual BOOL IsBarcodeSupported();
    virtual BOOL ShowBarcodeDialog(CHAR *pBarcode);

    virtual BOOL IsPhoneNumberSupported();
    virtual BOOL ShowPhoneNumberDialog();

    virtual BOOL IsEsriSupported();
    virtual BOOL TestEsriCredentials(CHAR *pUserName, CHAR *pPassword);

    virtual BOOL IsSoundSupported();

    virtual BOOL PlaySound(FXSOUNDRESOURCE *pSound);
    virtual BOOL StopSound();
    virtual BOOL IsPlaying();

    virtual BOOL StartRecording(CHAR *pFileNameNoExt, UINT Duration, UINT Frequency);
    virtual BOOL StopRecording();
    virtual BOOL PlayRecording(CHAR *pFileName);
    virtual VOID CancelRecording();
    virtual BOOL IsRecording();

    virtual VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    virtual BOOL GetDeviceId(GUID *pGuid);
    virtual CHAR *GetDeviceName();

    virtual BOOL CreateGuid(GUID *pGuid);

    virtual VOID GetDateTime(FXDATETIME *pDateTime)=0;
    virtual VOID SetDateTime(FXDATETIME *pDateTime)=0;
    virtual VOID SetTimeZone(INT BiasInMinutes)=0;
    virtual VOID SetDateTimeFromGPS(FXDATETIME *pDateTime) {}
    virtual CHAR *GetDateString(FXDATETIME *pDateTime)=0;
    virtual CHAR *GetDateStringUTC(FXDATETIME *pDateTime, BOOL FormatAsUTC = FALSE)=0;

    virtual VOID SetTimer(UINT Elapse, UINT TimerId)=0;
    virtual VOID KillTimer(UINT TimerId)=0;

    virtual VOID SetAlarm(UINT ElapseInSeconds);
    virtual VOID KillAlarm();

    virtual VOID RequestUrl(CHAR *pUrl, CHAR *pUserName, CHAR *pPassword, CfxStream &Data, CHAR *pFileName, UINT *pId);
    virtual VOID CancelUrl(UINT Id);

    virtual BOOL ConnectPort(BYTE PortId, FX_COMPORT *pComPort);
    virtual BOOL IsPortConnected(BYTE PortId);
    virtual VOID DisconnectPort(BYTE PortId);
    virtual VOID WritePortData(BYTE PortId, BYTE *pData, UINT Length);

    virtual VOID StartTransfer(CHAR *pOutgoingPath);
    virtual VOID CancelTransfer();
    virtual VOID GetTransferState(FXSENDSTATE *pState);

    virtual VOID SetPowerState(FXPOWERMODE Mode);
    virtual FXPOWERMODE GetPowerState();

    virtual FXBATTERYSTATE GetBatteryState();
    virtual UINT8 GetBatteryLevel();
    
    virtual UINT GetFreeMemory();

    virtual VOID GetGPS(FXGPS *pGPS);
    virtual VOID SetGPS(BOOL OnOff);
    virtual VOID ResetGPSTimeouts();
    virtual VOID SetupGPSSimulator(DOUBLE Latitude, DOUBLE Longitude, UINT Radius);
    virtual BYTE GetGPSPowerupTimeSeconds();
    virtual BOOL HasSmartGPSPower();

    virtual VOID GetRange(FXRANGE *pRange);
    virtual VOID SetRangeFinder(BOOL OnOff);
    virtual VOID ResetRangeFinderTimeouts();

    virtual BOOL AppendRecord(CHAR *pFileName, VOID *pRecord, UINT RecordSize);
    virtual BOOL DeleteRecords(CHAR *pFileName);
    virtual BOOL EnumRecordInit(CHAR *pFileName);
    virtual BOOL EnumRecordSetIndex(UINT Index, UINT RecordSize);
    virtual BOOL EnumRecordNext(VOID *pRecord, UINT RecordSize);
    virtual BOOL EnumRecordDone();

    virtual CHAR *AllocPathApplication(const CHAR *pFileName);
    virtual CHAR *AllocPathSD();

    virtual BOOL IsUpdateSupported();
    virtual VOID CheckForUpdate(GUID AppId, GUID StampId, CHAR *pWebUpdateUrl, CHAR *pTargetPath);

    virtual VOID RegisterExportFile(CHAR *pExportFilePath, FXEXPORTFILEINFO* exportFileInfo);
};

