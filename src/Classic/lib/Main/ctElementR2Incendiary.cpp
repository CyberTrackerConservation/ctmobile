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

    _onCapsuleAction = R2INCENDIARY_SAVE_ACTION::None;
    _onBlankAction = R2INCENDIARY_SAVE_ACTION::None;
    _onStatusAction = R2INCENDIARY_SAVE_ACTION::None;

    InitCOM(&_portId, &_comPort);

    _connected = false;

    _fastSave = false;

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
    _resultHeartbeatCount = nullptr;

    MEM_FREE(_resultDropRate);
    _resultDropRate = nullptr;

    MEM_FREE(_resultCapsuleCount);
    _resultCapsuleCount = nullptr;

    MEM_FREE(_resultCapsuleRollCount);
    _resultCapsuleRollCount = nullptr;

    MEM_FREE(_resultCapsuleFlightTotal);
    _resultCapsuleFlightTotal = nullptr;

    MEM_FREE(_resultBlankCount);
    _resultBlankCount = nullptr;

    MEM_FREE(_resultLostPackets);
    _resultLostPackets = nullptr;

    MEM_FREE(_resultBadPackets);
    _resultBadPackets = nullptr;

    MEM_FREE(_resultNoGPS);
    _resultNoGPS = nullptr;
}

void CctControl_ElementR2Incendiary::Reset()
{
    if (_connected && IsLive())
    {
        GetEngine(this)->DisconnectPort(this, _portId);
    }

    KillTimer();

    _connected = false;
    _lastConnected = false;
    SetCaption("Disconnected");

    _state = R2INCENDIARY_PARSE_STATE::Start;
    _counter = 0;
    _dataLength = 0;
    _dataIndex = 0;
    memset(&_data[0], 0, sizeof(_data));

    _hasLastCounter = false;
    _lastCounter = 0;
    _heartbeatCount = 0;
}

void CctControl_ElementR2Incendiary::OnSessionNext(CfxControl *pSender, void *pParams)
{
    if (!IsNullXGuid(&_statusElementId))
    {
        CctAction_MergeAttribute action(this);

        action.SetupAdd(_id, _statusElementId, dtPText, &_caption);

        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

void CctControl_ElementR2Incendiary::ExecuteAction(R2INCENDIARY_SAVE_ACTION Action)
{
    switch (Action) {
    case R2INCENDIARY_SAVE_ACTION::None:
        // Do nothing
        break;

    case R2INCENDIARY_SAVE_ACTION::Waypoint:
        break;

    case R2INCENDIARY_SAVE_ACTION::Sighting:
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

void CctControl_ElementR2Incendiary::IncGlobalValue(char *pName)
{
    if (!IsLive()) return;
    if (pName == nullptr) return;

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

void CctControl_ElementR2Incendiary::OnTimer()
{
    SetCaption("Disconnected");
    _lastConnected = false;
    Repaint();
}

void CctControl_ElementR2Incendiary::OnPortData(BYTE PortId, BYTE *pData, int Length)
{
    int i;

    for (i = 0; i < Length; i++, pData++)
    {
        switch (_state)
        {
        case R2INCENDIARY_PARSE_STATE::Start:
            if (*pData == 0xAC)   // 0xAC is message header
            {
                _expectedChecksum = *pData;
                _state = R2INCENDIARY_PARSE_STATE::Counter;
            }
            break;

        case R2INCENDIARY_PARSE_STATE::Counter:            // Counter should be 1+ the last counter mod 255
            _counter = *pData;
            _expectedChecksum += *pData;
            _state = R2INCENDIARY_PARSE_STATE::Length;

            if (!_hasLastCounter)
            {
                _lastCounter = (int)_counter - 1;
            }

            if ((int)_counter - _lastCounter != 1)
            {
                // Lost packets
                IncGlobalValue(_resultLostPackets);
            }
            
            _lastCounter = _counter;

            break;

        case R2INCENDIARY_PARSE_STATE::Length:
            _dataLength = *pData;
            _expectedChecksum += *pData;
            _state = R2INCENDIARY_PARSE_STATE::Data;
            _dataIndex = 0;
            memset(&_data[0], 0, sizeof(_data));

            if (_dataLength == 0)
            {
                // Bad length
                _state = R2INCENDIARY_PARSE_STATE::Start;
                IncGlobalValue(_resultBadPackets);
            }

            break;

        case R2INCENDIARY_PARSE_STATE::Data:
            _data[_dataIndex] = *pData;
            _expectedChecksum += *pData;
            _dataIndex++;

            if (_dataIndex == _dataLength)
            {
                _state = R2INCENDIARY_PARSE_STATE::Checksum;
            }
            break;

        case R2INCENDIARY_PARSE_STATE::Checksum:
            if (*pData == _expectedChecksum) 
            {
                ParsePacket(&_data[0], _dataLength);
            }
            else
            {
                // Bad packet
                IncGlobalValue(_resultBadPackets);
            }
            _state = R2INCENDIARY_PARSE_STATE::Start;
            break;
        }
    }
}

void CctControl_ElementR2Incendiary::ParsePacket(BYTE *pData, int Length)
{
    char outputText[64];

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
            if (_resultDropRate != nullptr)
            {
                GetSession(this)->SetGlobalValue(this, _resultDropRate, (double)pData[0]);
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
            if (_resultCapsuleRollCount != nullptr)
            {
                GetSession(this)->SetGlobalValue(this, _resultCapsuleRollCount, (double)((int)pData[0] * 256 + (int)pData[1]));
            }

            if (_resultCapsuleFlightTotal != nullptr)
            {
                GetSession(this)->SetGlobalValue(this, _resultCapsuleFlightTotal, (double)((int)pData[2] * 256 + (int)pData[3]));
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

    Repaint();
}

void CctControl_ElementR2Incendiary::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementR2Incendiary::DefineProperties(CfxFiler &F)
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

    if (IsLive() && F.IsReader())
    {
        Reset();
        _connected = GetEngine(this)->ConnectPort(this, _portId, &_comPort);
        SetTimer(1000);
    }
}

void CctControl_ElementR2Incendiary::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_ElementR2Incendiary::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
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

        int w = rect.Right - rect.Left + 1;
        int h = rect.Bottom - rect.Top + 1;
        int m = min(w, h) / 2;

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

        char *caption = GetVisibleCaption();
        if (caption && (strlen(caption) > 0))
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, pRect, _alignment | TEXT_ALIGN_BOTTOM, caption);
        }
        resource->ReleaseFont(font);
    }
}
