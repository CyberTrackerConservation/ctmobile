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
    double ExpectedTime = 0.0;
    CfxPersistent *Object = nullptr;
    bool Active = false;
    const char *Name = nullptr;
};

struct PORT_ITEM
{
    BYTE PortId = 0;
    FX_COMPORT ComPort;
    CfxPersistent *Object = nullptr;
};

struct URL_REQUEST_ITEM
{
    int Id = 0;
    BYTE *Data = nullptr;
    int DataSize = 0;
    CfxPersistent *Object = nullptr;
};

class CfxEngine: public CfxPersistent
{
protected:
    TList<CfxPersistent *> _timers;
    TList<ALARM_ITEM> _alarms;
    CfxPersistent *_trackTimer = nullptr;
    int _portsUniqueness = 0;
    TList<PORT_ITEM> _ports;
    TList<URL_REQUEST_ITEM> _urlRequests;
    CfxPersistent *_recorder = nullptr;
    CfxControl *_captureControl = nullptr;
    int _penDownX = 0, _penDownY = 0;
    bool _penMoved = false;
    BYTE _pendingKeyCode = 0;
    bool _inputBlocked = false;
    QTimer *_keyPressTimer = nullptr;
    int _lastUserEventTime = 0;
    int _blockPaintCount = 0;
    bool _paintRequested = false;
    CfxCanvas *_paintCanvas = nullptr;
    CanvasState _paintCanvasState;
    bool _terminating = false;
    bool _inPaint = false;
    int _cornerWidth1 = 0, _cornerWidth2 = 0;
    bool _realignRequested = false;
    int _scaleScroller = 0, _scaleTitle = 0, _scaleTab = 0;

    FXGOTO *_currentGoto = nullptr;
    int _paintTimerElapse = 0;

    FXGPS _gps;
    FXGPS_POSITION _gpsLastFix;
    FXDATETIME _gpsLastTimeStamp;

    int _gpsLastHeading = 0;
    bool _gpsTimeFirst = false;
    bool _gpsTimeSync = false;
    int _gpsTimeZone = 0;
    bool _gpsTimeUsedOnce = false;

    bool _alarmSet = false;
    bool _alarmMutex = false;
    void SetNextAlarm();

    void OnTimer() override;

    void PreDecorate(CfxControl *pControl, FXRECT *pRect);
    void PostDecorate(CfxControl *pControl, FXRECT *pRect);
    void PaintControl(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, int OffsetX, int OffsetY);
    void PaintControlNoClip(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, int OffsetX, int OffsetY);
    CfxControl *HitControl(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, FXPOINT *pPoint);
    CfxControl *HitControlNoClip(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, FXPOINT *pPoint);
public:
    CfxEngine(CfxPersistent *pOwner);
    ~CfxEngine();

    CfxControl *GetCaptureControl()		       { return _captureControl;     }
    void SetCaptureControl(CfxControl *pValue) { _captureControl = pValue;   }
    int GetLastUserEventTime()                { return _lastUserEventTime;  }
    void SetLastUserEventTime(int Value)      { _lastUserEventTime = Value; } 
    bool GetTerminating()                      { return _terminating;        }

    int GetScaleScroller()                    { return _scaleScroller;      }
    void SetScaleScroller(int Value)          { _scaleScroller = Value;     }
    int GetScaleTitle()                       { return _scaleTitle;         }
    void SetScaleTitle(int Value)             { _scaleTitle = Value;        }
    int GetScaleTab()                         { return _scaleTab;           }
    void SetScaleTab(int Value)               { _scaleTab = Value;          }

    void Paint(CfxCanvas *pCanvas);
    void PaintNoClip(CfxCanvas *pCanvas);
    void BlockPaint();
    void UnblockPaint();
    bool RequestPaint();

    CfxControl *HitTest(CfxControl *pControl, int X, int Y);
    CfxControl *HitTestNoClip(CfxControl *pControl, int X, int Y);

    void RequestRealign()                      { _realignRequested = true;   }
    void AlignControls(CfxControl *pParent);

    void Startup(); 
    void Shutdown();
    void Repaint();

    void PenDown(int X, int Y);
    void PenUp(int X, int Y);
    void PenMove(int X, int Y);

    bool KeyDown(BYTE KeyCode);
    bool KeyUp(BYTE KeyCode);
    void KeyPress(BYTE KeyCode, int DelayMs = 0);

    bool CloseDialog();

    void SetPaintTimer(int Elapse);

    void DoTimer(int TimerId);
    void SetTimer(CfxPersistent *pObject, int Elapse);
    void KillTimer(CfxPersistent *pObject);
    bool HasTimer(CfxPersistent *pObject);

    void DoAlarm();
    void SetAlarm(CfxPersistent *pObject, int ElapseInSeconds, const char *pName = "Unnamed");
    void KillAlarm(CfxPersistent *pObject);
    bool HasAlarm(CfxPersistent *pObject);

    void DoTrackTimer();
    void SetTrackTimer(CfxPersistent *pObject, int ElapseInSeconds);
    void KillTrackTimer(CfxPersistent *pObject);

    bool StartRecording(CfxPersistent *pObject, char *pFileNameNoExt, int Duration, int Frequency);
    bool StopRecording();
    void CancelRecording();
    bool IsRecording();
    void DoRecordingComplete(char *pFileName, int Duration);

    void DoPictureTaken(CfxControl *pControl, char *pFileName, bool Success);
    void DoBarcodeScan(CfxControl *pControl, char *pBarcode, bool Success);
    void DoPhoneNumberTaken(CfxControl *pControl, char *pPhoneNumber, bool Success);

    void DoUrlRequestCompleted(int Id, const char *pFileName, int ErrorCode);
    void SetUrlRequest(CfxPersistent *pObject, char *pUrl, char *pUserName, char *pPassword, CfxStream &Data);
    void CancelUrlRequest(CfxPersistent *pObject);

    void DoPortData(FX_COMPORT_DATA *pComPortData);
    bool ConnectPort(CfxPersistent *pObject, BYTE PortId, FX_COMPORT *pComPort);
    bool IsPortConnected(BYTE PortId);
    void CyclePort(BYTE PortId, FX_COMPORT *pComPort);
    void DisconnectPort(CfxPersistent *pObject, BYTE PortId, bool KeepConnected = false);
    void WritePortData(BYTE PortId, BYTE *pData, int Length);


    bool GetGPSTimeSync()         { return _gpsTimeSync;     }
    void SetGPSTimeSync(bool Enabled, int Zone);
    int GetGPSTimeZone()          { return _gpsTimeZone;     }
    bool GetGPSTimeUsedOnce()     { return _gpsTimeUsedOnce; }
    FXGPS *GetGPS();
    FXGPS_POSITION *GetGPSLastFix()   { return &_gpsLastFix;       }
    int GetGPSLastHeading()           { return _gpsLastHeading;    }
    FXDATETIME *GetGPSLastTimeStamp() { return &_gpsLastTimeStamp; }

    void LockGPS();
    void UnlockGPS();
    void DoStartGPS();

    FXGOTO *GetCurrentGoto()            { return _currentGoto;  }
    void SetCurrentGoto(FXGOTO *pGoto)  { memcpy(_currentGoto, pGoto, sizeof(FXGOTO)); }

    void DoWakeup();
    void DoSleep();
};

