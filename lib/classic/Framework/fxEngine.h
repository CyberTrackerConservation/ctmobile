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
#include "fxClasses.h"
#include "fxControl.h"

#define KEY_UP      1
#define KEY_DOWN    2
#define KEY_LEFT    3
#define KEY_RIGHT   4
#define KEY_ENTER   5

struct ALARM_ITEM 
{
    DOUBLE ExpectedTime;
    CfxPersistent *Object;
    BOOL Active;
    const CHAR *Name;
};

struct PORT_ITEM
{
    BYTE PortId;
    FX_COMPORT ComPort;
    CfxPersistent *Object;
};

struct URL_REQUEST_ITEM
{
    UINT Id;
    BYTE *Data;
    UINT DataSize;
    CfxPersistent *Object;
};

class CfxEngine: public CfxPersistent
{
protected:
    TList<CfxPersistent *> _timers;
    TList<ALARM_ITEM> _alarms;
    CfxPersistent *_trackTimer;
    UINT _portsUniqueness;
    TList<PORT_ITEM> _ports;
    TList<CfxPersistent *> _transfers;
    TList<URL_REQUEST_ITEM> _urlRequests;
    CfxPersistent *_recorder;
    CfxControl *_captureControl;
    UINT _lastUserEventTime;
    INT _blockPaintCount;
    BOOL _paintRequested;
    CfxCanvas *_paintCanvas;
    CanvasState _paintCanvasState;
    BOOL _terminating;
    BOOL _inPaint;
    UINT _cornerWidth1, _cornerWidth2;
    BOOL _realignRequested;
    BOOL _useRangeFinderForAltitude;

    UINT _scaleScroller, _scaleTitle, _scaleTab;

    FXGOTO *_currentGoto;
    UINT _paintTimerElapse;

    FXGPS _gps;
    FXGPS_POSITION _gpsLastFix;
    FXDATETIME _gpsLastTimeStamp;

    INT _gpsLastHeading;
    BOOL _gpsTimeFirst;
    BOOL _gpsTimeSync;
    INT _gpsTimeZone;
    BOOL _gpsTimeUsedOnce;

    FXRANGE _range;

    BOOL _alarmSet;
    VOID SetNextAlarm();

    VOID OnTimer();

    VOID PreDecorate(CfxControl *pControl, FXRECT *pRect);
    VOID PostDecorate(CfxControl *pControl, FXRECT *pRect);
    VOID PaintControl(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, INT OffsetX, INT OffsetY);
    VOID PaintControlNoClip(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, INT OffsetX, INT OffsetY);
    CfxControl *HitControl(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, FXPOINT *pPoint);
    CfxControl *HitControlNoClip(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, FXPOINT *pPoint);
public:
    CfxEngine(CfxPersistent *pOwner);
    ~CfxEngine();

    CfxControl *GetCaptureControl()		       { return _captureControl;     }
    VOID SetCaptureControl(CfxControl *pValue) { _captureControl = pValue;   }
    UINT GetLastUserEventTime()                { return _lastUserEventTime;  }
    VOID SetLastUserEventTime(UINT Value)      { _lastUserEventTime = Value; } 
    BOOL GetTerminating()                      { return _terminating;        }

    UINT GetScaleScroller()                    { return _scaleScroller;      }
    VOID SetScaleScroller(UINT Value)          { _scaleScroller = Value;     }
    UINT GetScaleTitle()                       { return _scaleTitle;         }
    VOID SetScaleTitle(UINT Value)             { _scaleTitle = Value;        }
    UINT GetScaleTab()                         { return _scaleTab;           }
    VOID SetScaleTab(UINT Value)               { _scaleTab = Value;          }

    VOID Paint(CfxCanvas *pCanvas);
    VOID PaintNoClip(CfxCanvas *pCanvas);
    VOID BlockPaint();
    VOID UnblockPaint();
    BOOL RequestPaint();

    CfxControl *HitTest(CfxControl *pControl, INT X, INT Y);
    CfxControl *HitTestNoClip(CfxControl *pControl, INT X, INT Y);

    VOID RequestRealign()                      { _realignRequested = TRUE;   }
    VOID AlignControls(CfxControl *pParent);

    VOID Startup(); 
    VOID Shutdown();
    VOID Repaint();

    VOID PenDown(INT X, INT Y);
    VOID PenUp(INT X, INT Y);
    VOID PenMove(INT X, INT Y);

    BOOL KeyDown(BYTE KeyCode);
    BOOL KeyUp(BYTE KeyCode);
    VOID KeyPress(BYTE KeyCode);

    BOOL CloseDialog();

    VOID SetPaintTimer(UINT Elapse);

    VOID DoTimer(UINT TimerId);
    VOID SetTimer(CfxPersistent *pObject, UINT Elapse);
    VOID KillTimer(CfxPersistent *pObject);
    BOOL HasTimer(CfxPersistent *pObject);

    VOID DoAlarm();
    VOID SetAlarm(CfxPersistent *pObject, UINT ElapseInSeconds, const CHAR *pName = "Unnamed");
    VOID KillAlarm(CfxPersistent *pObject);
    BOOL HasAlarm(CfxPersistent *pObject);

    VOID DoTrackTimer();
    VOID SetTrackTimer(CfxPersistent *pObject, UINT ElapseInSeconds);
    VOID KillTrackTimer(CfxPersistent *pObject);

    BOOL StartRecording(CfxPersistent *pObject, CHAR *pFileNameNoExt, UINT Duration, UINT Frequency);
    BOOL StopRecording();
    VOID CancelRecording();
    BOOL IsRecording();
    VOID DoRecordingComplete(CHAR *pFileName, UINT Duration);

    VOID DoPictureTaken(CfxControl *pControl, CHAR *pFileName, BOOL Success);
    VOID DoBarcodeScan(CfxControl *pControl, CHAR *pBarcode, BOOL Success);
    VOID DoPhoneNumberTaken(CfxControl *pControl, CHAR *pPhoneNumber, BOOL Success);

    VOID DoUrlRequestCompleted(UINT Id, const CHAR *pFileName, UINT ErrorCode);
    VOID SetUrlRequest(CfxPersistent *pObject, CHAR *pUrl, CHAR *pUserName, CHAR *pPassword, CfxStream &Data);
    VOID CancelUrlRequest(CfxPersistent *pObject);

    VOID DoPortData(FX_COMPORT_DATA *pComPortData);
    BOOL ConnectPort(CfxPersistent *pObject, BYTE PortId, FX_COMPORT *pComPort);
    BOOL IsPortConnected(BYTE PortId);
    VOID CyclePort(BYTE PortId, FX_COMPORT *pComPort);
    VOID DisconnectPort(CfxPersistent *pObject, BYTE PortId, BOOL KeepConnected = FALSE);
    VOID WritePortData(BYTE PortId, BYTE *pData, UINT Length);

    VOID DoTransferEvent(FXSENDSTATEMODE Mode);
    VOID GetTransferState(FXSENDSTATE *pState);
    VOID StartTransfer(CfxPersistent *pObject, CHAR *pOutgoingPath);
    VOID AddTransferListener(CfxPersistent *pObject);
    VOID RemoveTransferListener(CfxPersistent *pObject);
    VOID CancelTransfers();

    BOOL GetGPSTimeSync()         { return _gpsTimeSync;     }
    VOID SetGPSTimeSync(BOOL Enabled, INT Zone);
    INT GetGPSTimeZone()          { return _gpsTimeZone;     }
    BOOL GetGPSTimeUsedOnce()     { return _gpsTimeUsedOnce; }
    FXGPS *GetGPS();
    FXGPS_POSITION *GetGPSLastFix()   { return &_gpsLastFix;       }
    INT GetGPSLastHeading()           { return _gpsLastHeading;    }
    FXDATETIME *GetGPSLastTimeStamp() { return &_gpsLastTimeStamp; }

    VOID LockGPS();
    VOID UnlockGPS();
    VOID DoStartGPS();

    FXRANGE *GetRange();
    VOID LockRangeFinder();
    VOID UnlockRangeFinder();
    VOID DoStartRangeFinder();
    VOID ClearLastRange();
    VOID SetLastRange(FXRANGE *pRange)  { _range = *pRange; }
    
    BOOL GetUseRangeFinderForAltitude()           { return _useRangeFinderForAltitude;  }
    VOID SetUseRangeFinderForAltitude(BOOL Value) { _useRangeFinderForAltitude = Value; }

    FXGOTO *GetCurrentGoto()            { return _currentGoto;  }
    VOID SetCurrentGoto(FXGOTO *pGoto)  { memcpy(_currentGoto, pGoto, sizeof(FXGOTO)); }

    VOID DoWakeup();
    VOID DoSleep();
};

