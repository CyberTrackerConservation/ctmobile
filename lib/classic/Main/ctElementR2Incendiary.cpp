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

#include "fxApplication.h"
#include "fxUtils.h"
#include "ctElementR2Incendiary.h"
#include "ctSession.h"
#include "ctActions.h"

CfxControl *Create_Control_ElementR2Incendiary(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementR2Incendiary(pOwner, pParent);
}

CctControl_ElementR2Incendiary::CctControl_ElementR2Incendiary(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTR2INCENDIARY);
    InitLockProperties("Action capsule;Action blank;Action error;COM - Baud rate;COM - Data bits;COM - Flow control;COM - Port;COM - Parity;COM - Stop bits");

    _onCapsuleAction = rsaNone;
    _onBlankAction = rsaNone;
    _onStatusAction = rsaNone;

    InitCOM(&_portId, &_comPort);

    _connected = FALSE;

    _fastSave = FALSE;

    _resultHeartbeatCount = AllocString("R2HeartbeatCount");
    _resultDropRate = AllocString("R2DropRate");
    _resultCapsuleCount = AllocString("R2CapsuleCount");
    _resultCapsuleRollCount = AllocString("R2CapsuleRollCount");
    _resultCapsuleFlightTotal = AllocString("R2CapsuleFlightTotal");
    _resultBlankCount = AllocString("R2BlankCount");
    _resultLostPackets = AllocString("R2LostPackets");
    _resultBadPackets = AllocString("R2BadPackets");
    _resultNoGPS = AllocString("R2NoGPS");

    InitXGuid(&_statusElementId);

    Reset();

    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementR2Incendiary::OnSessionNext);
}

CctControl_ElementR2Incendiary::~CctControl_ElementR2Incendiary()
{
    UnregisterSessionEvent(seNext, this);

    Reset();

    MEM_FREE(_resultHeartbeatCount);
    _resultHeartbeatCount = NULL;

    MEM_FREE(_resultDropRate);
    _resultDropRate = NULL;

    MEM_FREE(_resultCapsuleCount);
    _resultCapsuleCount = NULL;

    MEM_FREE(_resultCapsuleRollCount);
    _resultCapsuleRollCount = NULL;

    MEM_FREE(_resultCapsuleFlightTotal);
    _resultCapsuleFlightTotal = NULL;

    MEM_FREE(_resultBlankCount);
    _resultBlankCount = NULL;

    MEM_FREE(_resultLostPackets);
    _resultLostPackets = NULL;

    MEM_FREE(_resultBadPackets);
    _resultBadPackets = NULL;

    MEM_FREE(_resultNoGPS);
    _resultNoGPS = NULL;
}

VOID CctControl_ElementR2Incendiary::Reset()
{
    if (_connected && IsLive())
    {
        GetEngine(this)->DisconnectPort(this, _portId);
        GetSession(this)->SetDatabaseCaching(FALSE);
    }

    KillTimer();

    _connected = FALSE;
    _lastConnected = FALSE;
    SetCaption("Disconnected");

    _state = rpsStart;
    _counter = 0;
    _dataLength = 0;
    _dataIndex = 0;
    memset(&_data[0], 0, sizeof(_data));

    _hasLastCounter = FALSE;
    _lastCounter = 0;
    _heartbeatCount = 0;
}

VOID CctControl_ElementR2Incendiary::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    if (!IsNullXGuid(&_statusElementId))
    {
        CctAction_MergeAttribute action(this);

        action.SetupAdd(_id, _statusElementId, dtPText, &_caption);

        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

VOID CctControl_ElementR2Incendiary::ExecuteAction(R2INCENDIARY_SAVE_ACTION Action)
{
    switch (Action) {
    case rsaNone:
        // Do nothing
        break;

    case rsaWaypoint:
        {
            FXGPS *gps = GetEngine(this)->GetGPS();
            if (!GetSession(this)->GetWaypointManager()->StoreRecord(gps, FALSE))
            {
                IncGlobalValue(_resultNoGPS);
            }
        }
        break;

    case rsaSighting:
        {
            CctSession *session = GetSession(this);
            FXGPS *gps = GetEngine(this)->GetGPS();
            if (!TestGPS(gps, session->GetSightingAccuracy()))
            {
                IncGlobalValue(_resultNoGPS);
            }
            
            session->DoCommitInplace();
        }
        break;
    }
}

VOID CctControl_ElementR2Incendiary::IncGlobalValue(CHAR *pName)
{
    if (!IsLive()) return;
    if (pName == NULL) return;

    GLOBALVALUE *globalValue;
    CctSession *session = GetSession(this);

    globalValue = session->GetGlobalValue(pName);
    if (globalValue)
    {
        session->SetGlobalValue(this, pName, globalValue->Value + 1);
    }
    else
    {
        session->SetGlobalValue(this, pName, 1);
    }
}

VOID CctControl_ElementR2Incendiary::OnTimer()
{
    if (!GetEngine(this)->IsPortConnected(_portId))
    {
        SetCaption("Disconnected");
        _lastConnected = FALSE;
        Repaint();
    }
    else if (!_lastConnected)
    {
        SetCaption("Connected");
        _lastConnected = TRUE;
        Repaint();
    }

    FxResetAutoOffTimer();
}

VOID CctControl_ElementR2Incendiary::OnPortData(BYTE PortId, BYTE *pData, UINT Length)
{
    UINT i;

    for (i = 0; i < Length; i++, pData++)
    {
        switch (_state)
        {
        case rpsStart:
            if (*pData == 0xAC)   // 0xAC is message header
            {
                _expectedChecksum = *pData;  
                _state = rpsCounter;
            }
            break;

        case rpsCounter:            // Counter should be 1+ the last counter mod 255
            _counter = *pData;
            _expectedChecksum += *pData;
            _state = rpsLength;

            if (!_hasLastCounter)
            {
                _lastCounter = (INT)_counter - 1;
            }

            if ((INT)_counter - _lastCounter != 1)
            {
                // Lost packets
                IncGlobalValue(_resultLostPackets);
            }
            
            _lastCounter = _counter;

            break;

        case rpsLength:
            _dataLength = *pData;
            _expectedChecksum += *pData;
            _state = rpsData;
            _dataIndex = 0;
            memset(&_data[0], 0, sizeof(_data));

            if (_dataLength == 0)
            {
                // Bad length
                _state = rpsStart;
                IncGlobalValue(_resultBadPackets);
            }

            break;

        case rpsData:
            _data[_dataIndex] = *pData;
            _expectedChecksum += *pData;
            _dataIndex++;

            if (_dataIndex == _dataLength)
            {
                _state = rpsChecksum;
            }
            break;

        case rpsChecksum:
            if (*pData == _expectedChecksum) 
            {
                ParsePacket(&_data[0], _dataLength);
            }
            else
            {
                // Bad packet
                IncGlobalValue(_resultBadPackets);
            }
            _state = rpsStart;
            break;
        }
    }
}

VOID CctControl_ElementR2Incendiary::ParsePacket(BYTE *pData, UINT Length)
{
    CHAR outputText[64];

    BYTE EventCode = *pData;
    pData++;
    Length--;
    
    switch (EventCode)
    {
    case 0x01:  // Power on
        SetCaption("Power on");
        ExecuteAction(_onStatusAction);
        break;

    case 0x02:  // Software version
        if (Length == 4) 
        {
            sprintf(outputText, "Version %d.%d build %d", pData[1], pData[2], pData[3]);
            SetCaption(outputText);
            ExecuteAction(_onStatusAction);
        } 
        break;

    case 0x03:  // Diagnostics result
        if (Length == 3) 
        {
            sprintf(outputText, "Diagnostics: %d, %d, %d", pData[0], pData[1], pData[2]);
            SetCaption(outputText);
            ExecuteAction(_onStatusAction);
        }
        break;

    case 0x04:  // Status
        if (Length == 1) 
        {
            sprintf(outputText, "Status: %d", pData[0]);
            SetCaption(outputText);
            ExecuteAction(_onStatusAction);
        }
        break;

    case 0x05:  // Drop rate
        if (Length == 1)
        {
            if (_resultDropRate != NULL)
            {
                GetSession(this)->SetGlobalValue(this, _resultDropRate, (DOUBLE)pData[0]);
            }

            sprintf(outputText, "Drop rate: %d", pData[0]);
            SetCaption(outputText);
            ExecuteAction(_onStatusAction);
        }
        break;

    case 0x06:  // Heartbeat
        IncGlobalValue(_resultHeartbeatCount);
        _heartbeatCount++;
        break;

    case 0x07:  // Capsule dropped
        IncGlobalValue(_resultCapsuleCount);
        if (Length == 4)
        {
            if (_resultCapsuleRollCount != NULL)
            {
                GetSession(this)->SetGlobalValue(this, _resultCapsuleRollCount, (DOUBLE)((UINT)pData[0] * 256 + (UINT)pData[1]));
            }

            if (_resultCapsuleFlightTotal != NULL)
            {
                GetSession(this)->SetGlobalValue(this, _resultCapsuleFlightTotal, (DOUBLE)((UINT)pData[2] * 256 + (UINT)pData[3]));
            }
        }
        SetCaption("Capsule");
        ExecuteAction(_onCapsuleAction);
        break;

    case 0x08:  // Fired blank
        IncGlobalValue(_resultBlankCount);
        SetCaption("Blank");
        ExecuteAction(_onBlankAction);
        break;

    case 0x09:  // Shutdown
        SetCaption("Shutdown");
        ExecuteAction(_onStatusAction);
        break;

    case 0x10:  // Saved state
        SetCaption("Save state");
        ExecuteAction(_onStatusAction);
        break;

    case 0x11:  // Restored state
        SetCaption("Restore state");
        ExecuteAction(_onStatusAction);
        break;

    default:
        SetCaption("Unknown event");
        break;
    }

    FxResetAutoOffTimer();
    Repaint();
}

VOID CctControl_ElementR2Incendiary::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Panel::DefineResources(F);
    F.DefineObject(_statusElementId);
#endif
}

VOID CctControl_ElementR2Incendiary::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineCOM(&_portId, &_comPort);

    F.DefineValue("R2HeartbeatCount", dtPText, &_resultHeartbeatCount);
    F.DefineValue("R2DropRate", dtPText, &_resultDropRate);
    F.DefineValue("R2CapsuleCount", dtPText, &_resultCapsuleCount);
    F.DefineValue("R2CapsuleRollCount", dtPText, &_resultCapsuleRollCount);
    F.DefineValue("R2CapsuleFlightTotal", dtPText, &_resultCapsuleFlightTotal);
    F.DefineValue("R2BlankCount", dtPText, &_resultBlankCount);
    F.DefineValue("R2LostPackets", dtPText, &_resultLostPackets);
    F.DefineValue("R2BadPackets", dtPText, &_resultBadPackets);
    F.DefineValue("R2NoGPS", dtPText, &_resultNoGPS);

    F.DefineValue("ActionCapsule", dtByte, &_onCapsuleAction, F_ZERO);
    F.DefineValue("ActionBlank", dtByte, &_onBlankAction, F_ZERO);
    F.DefineValue("ActionStatus", dtByte, &_onStatusAction, F_ZERO);

    F.DefineValue("FastSave", dtBoolean, &_fastSave, F_FALSE);

    F.DefineXOBJECT("StatusElementId", &_statusElementId);

    // Null id is automatically assigned to static
    #ifdef CLIENT_DLL
        if (IsNullXGuid(&_statusElementId))
        {
            _statusElementId = GUID_ELEMENT_STATUS;
        }
    #endif

    if (IsLive() && F.IsReader())
    {
        Reset();
        _connected = GetEngine(this)->ConnectPort(this, _portId, &_comPort);
        SetTimer(1000);

        if (_fastSave)
        {
            GetSession(this)->SetDatabaseCaching(TRUE);
        }
    }
}

VOID CctControl_ElementR2Incendiary::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    CHAR items[256];
    sprintf(items, "LIST(None=%d \"Store waypoint\"=%d \"Store sighting\"=%d)", rsaNone, rsaWaypoint, rsaSighting);
    F.DefineValue("Action blank", dtByte, &_onBlankAction, items);
    F.DefineValue("Action capsule", dtByte, &_onCapsuleAction, items);
    sprintf(items, "LIST(None=%d \"Store sighting\"=%d)", rsaNone, rsaSighting);
    F.DefineValue("Action status", dtByte, &_onStatusAction, items);

    F.DefineValue("Border color",  dtColor,   &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style",  dtInt8,    &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width",  dtInt32,   &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",         dtColor,   &_color);

    sprintf(items, "LIST(2400=%d 4800=%d 9600=%d 19200=%d 38400=%d 57600=%d 115200=%d)", CBR_2400, CBR_4800, CBR_9600, CBR_19200, CBR_38400, CBR_57600, CBR_115200);
    F.DefineValue("COM - Baud rate", dtInt32, &_comPort.Baud, items);

    sprintf(items, "LIST(7=%d 8=%d)", DATABITS_7, DATABITS_8);
    F.DefineValue("COM - Data bits", dtByte, &_comPort.Data, items);
    F.DefineValue("COM - Flow control", dtBoolean, &_comPort.FlowControl);
    sprintf(items, "LIST(COM0=%d COM1=%d COM2=%d COM3=%d COM4=%d COM5=%d COM6=%d COM7=%d COM8=%d COM9=%d)", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
    F.DefineValue("COM - Port", dtByte, &_portId, items);
    sprintf(items, "LIST(None=%d Odd=%d Even=%d)", NOPARITY, ODDPARITY, EVENPARITY);
    F.DefineValue("COM - Parity", dtByte, &_comPort.Parity, items); 
    sprintf(items, "LIST(1=%d 2=%d)", ONESTOPBIT, TWOSTOPBITS);
    F.DefineValue("COM - Stop bits", dtByte, &_comPort.Stop, items);

    F.DefineValue("Dock",         dtByte,    &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Fast save",    dtBoolean, &_fastSave);
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);

    F.DefineValue("Result bad packets", dtPText, &_resultBadPackets);
    F.DefineValue("Result blank count", dtPText, &_resultBlankCount);
    F.DefineValue("Result capsule count", dtPText, &_resultCapsuleCount);
    F.DefineValue("Result capsule flight total", dtPText, &_resultCapsuleFlightTotal);
    F.DefineValue("Result capsule roll count", dtPText, &_resultCapsuleRollCount);
    F.DefineValue("Result drop rate", dtPText, &_resultDropRate);
    F.DefineValue("Result heartbeats", dtPText, &_resultHeartbeatCount);
    F.DefineValue("Result lost packets", dtPText, &_resultLostPackets);
    F.DefineValue("Result no gps", dtPText, &_resultNoGPS);

    F.DefineValue("Status Result Element", dtGuid, &_statusElementId, "EDITOR(ScreenElementsEditor)");

    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_ElementR2Incendiary::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    CfxResource *resource = GetResource(); 

    FXFONTRESOURCE *font = resource->GetFont(this, _font);
    if (font)
    {
        FXRECT rect = *pRect;
        rect.Bottom -= pCanvas->FontHeight(font);
        rect.Bottom -= _borderLineWidth * 4;
        InflateRect(&rect, -4*_borderLineWidth, -4*_borderLineWidth);

        INT w = rect.Right - rect.Left + 1;
        INT h = rect.Bottom - rect.Top + 1;
        INT m = min(w, h) / 2;

        if (_heartbeatCount & 1)
        {
            pCanvas->State.BrushColor = _borderColor;
            pCanvas->FillEllipse(rect.Left + w / 2, rect.Top + h / 2, m, m);
        }
        else
        {
            pCanvas->State.LineColor = _borderColor;
            pCanvas->State.LineWidth = _borderLineWidth;
            pCanvas->Ellipse(rect.Left + w / 2, rect.Top + h / 2, m, m);
        }

        CHAR *caption = GetVisibleCaption();
        if (caption && (strlen(caption) > 0))
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, pRect, _alignment | TEXT_ALIGN_BOTTOM, caption);
        }
        resource->ReleaseFont(font);
    }
}
