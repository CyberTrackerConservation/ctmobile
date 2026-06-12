#include "fxApplication.h"
#include "fxUtils.h"
#include "ctElementSerial.h"
#include "ctSession.h"
#include "ctActions.h"

CfxControl *Create_Control_ElementSerialData(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementSerialData(pOwner, pParent);
}

CctControl_ElementSerialData::CctControl_ElementSerialData(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTSERIALDATA);
    InitLockProperties("COM - Baud rate;COM - Data bits;COM - Flow control;COM - Port;COM - Parity;COM - Stop bits;Result Element");

    InitXGuid(&_resultElementId);
    
    memset(&_elements[0], 0, sizeof(_elements));
    memset(&_values[0], 0, sizeof(_values));

    _autoNext = false;
    _clearOnEnter = false;
    _sendOnConnect = true;
    _sendData = nullptr;
    _activeElementCount = 0;

    _keepConnected = false;
    _hasData = false;

    _portId = _portIdOverride = _portIdConnected = 0;
    InitCOM(&_portId, &_comPort);
    _connected = _tryConnect = false;

    _separator = nullptr;

    _showPortOverride = true;
    _showPortState = false;

    _portGlobalValue = nullptr;

    _buttonPortSelect = new CfxControl_Button(pOwner, this);
    _buttonPortSelect->SetComposite(true);
    _buttonPortSelect->SetVisible(_showPortOverride);
    _buttonPortSelect->SetBorder(ControlBorderStyle::None, 0, 0, _borderColor);
    _buttonPortSelect->SetOnClick(this, (NotifyMethod)&CctControl_ElementSerialData::OnPortSelectClick);
    _buttonPortSelect->SetOnPaint(this, (NotifyMethod)&CctControl_ElementSerialData::OnPortSelectPaint);

    Reset();

    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementSerialData::OnSessionNext);
}

CctControl_ElementSerialData::~CctControl_ElementSerialData()
{
    UnregisterSessionEvent(seNext, this);

    Reset(_keepConnected && _hasData);

    MEM_FREE(_sendData);
    _sendData = nullptr;

    MEM_FREE(_separator);
	_separator = nullptr;

    MEM_FREE(_portGlobalValue);
    _portGlobalValue = nullptr;

    SetCaption(nullptr);

    for (int i = 0; i < ARRAYSIZE(_values); i++)
    {
        MEM_FREE(_values[i]);
        _values[i] = 0;
    }
}

void CctControl_ElementSerialData::Reset(bool KeepConnected)
{
    if (_connected && IsLive())
    {
        GetEngine(this)->DisconnectPort(this, _portIdConnected, KeepConnected);
    }

    _portIdConnected = 0;
    _connected = _tryConnect = false;

    _dataIndex = 0;
    memset(&_data[0], 0, sizeof(_data));

    _hasData = false;
}

bool CctControl_ElementSerialData::ParseData(char *pData)
{
    if (pData == nullptr) return false;
    if ((_separator == nullptr) || (strlen(_separator) != 1)) return false;
    if (strchr(pData, *_separator) == nullptr) return false;

    bool result = false;
    char *curr = pData;
    char *last = pData + strlen(pData);

    for (int i = 0; i < ARRAYSIZE(_values); i++)
    {
        MEM_FREE(_values[i]);
        _values[i] = nullptr;

        char *next = strchr(curr, *_separator);
        if (next == nullptr)
        {
            next = last;
        }

        int len = next - curr + 1;
        _values[i] = (char *)MEM_ALLOC(len);
        memcpy(_values[i], curr, len);
        _values[i][len - 1] = '\0';
        result = true;

        if (next == last)
        {
            break;
        }

        curr = next + 1;
    }

    return result;
}

void CctControl_ElementSerialData::OnPortSelectClick(CfxControl *pSender, void *pParams)
{
    //PORTSELECT_PARAMS params;
    //params.ControlId = GetId();
    //params.PortId = _portId;
    //if (_portIdOverride != 0)
    //{
    //    params.PortId = _portIdOverride;
    //}
    //
    //_portIdOverride = 0;

    //Reset();

    //GetSession(this)->ShowDialog(&GUID_DIALOG_PORTSELECT, nullptr, &params, sizeof(PORTSELECT_PARAMS));
}

void CctControl_ElementSerialData::OnDialogResult(GUID *pDialogId, void *pParams)
{
    // Ignore overrides from the global value
    MEM_FREE(_portGlobalValue);
    _portGlobalValue = nullptr;

    SetCaption(nullptr);

    _portIdOverride = (BYTE)(UINT64)pParams;
    GetSession(this)->SaveControlState(this);

    Reset();
    _tryConnect = true;
    Repaint();
}

void CctControl_ElementSerialData::OnPortSelectPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPort(pParams->Rect, pParams->Canvas->InvertColor(_color), pParams->Canvas->InvertColor(_borderColor), _buttonPortSelect->GetDown()); 
}

void CctControl_ElementSerialData::OnSessionNext(CfxControl *pSender, void *pParams)
{
    CctAction_MergeAttribute action(this);
    bool use = false;

    if (!IsNullXGuid(&_resultElementId))
    {
        use = true;
        action.SetupAdd(_id, _resultElementId, dtPText, &_caption);
    }

    for (int i=0; i < ARRAYSIZE(_elements); i++)
    {
        if ((!IsNullXGuid(&_elements[i])) &&
            (_values[i] != nullptr))
        {
            use = true;
            action.SetupAdd(_id, _elements[i], dtPText, &_values[i]);
        }
    }

    if (use)
    {
        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

void CctControl_ElementSerialData::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl_Panel::SetBounds(Left, Top, Width, Height);

    int spacer = 4 * _borderLineWidth;
    int size = _borderLineWidth * 24;
    _buttonPortSelect->SetBounds(Width - size - spacer - _borderWidth, spacer + _borderWidth, size, size);
}

void CctControl_ElementSerialData::OnPortData(BYTE PortId, BYTE *pData, int Length)
{
    if (PortId != _portIdConnected) return;

    int i;
    bool hasData = false;

    for (i = 0; i < Length; i++, pData++)
    {
        if ((*pData == 10) || (*pData == 13))
        {
            if (_dataIndex > 0)
            {
                if ((_activeElementCount == 0) || (ParseData((char *)&_data[0])))
                {
                    _hasData = true;
                    SetCaption((char *)&_data[0]);
                    Repaint();
                }
            }
            
            _dataIndex = 0;
            memset(&_data[0], 0, sizeof(_data));
            continue;
        }

        _data[_dataIndex] = *pData;
        _dataIndex++;

        if (_dataIndex >= sizeof(_data))
        {
            _dataIndex = 0;
            memset(&_data[0], 0, sizeof(_data));
        }

        // Null terminate
        _data[_dataIndex] = 0;
    }

    if (_connected && _autoNext && hasData && (strlen(_caption) > 0) && (!GetSession(this)->HasFaulted()))
    {
        GetEngine(this)->KeyPress(KEY_RIGHT);
    }
}

void CctControl_ElementSerialData::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementSerialData::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);

    F.DefineValue("Caption", dtPText, &_caption);
    F.DefineValue("PortIdOverride", dtByte, &_portIdOverride);
    F.DefineValue("PortGlobalValue", dtPText, &_portGlobalValue);

    if (F.IsReader())
    {
        ParseData(_caption);

        if (_clearOnEnter)
        {
            memset(&_data[0], 0, sizeof(_data));
            SetCaption(nullptr);
        }
    }
}

void CctControl_ElementSerialData::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    for (int i=0; i<ARRAYSIZE(_elements); i++)
    {
        char varName[32];
        sprintf(varName, "Elements%ld", i);
        F.DefineXOBJECT(varName, &_elements[i]);
    }

    F.DefineValue("AutoNext",      dtBoolean, &_autoNext, F_FALSE);
    F.DefineValue("ClearOnEnter",   dtBoolean, &_clearOnEnter, F_FALSE);
    F.DefineXOBJECT("ResultElement", &_resultElementId);
    F.DefineValue("SendOnConnect", dtBoolean, &_sendOnConnect, F_TRUE);
    F.DefineValue("SendData",      dtPText,  &_sendData);
    F.DefineValue("Separator",     dtPText,  &_separator);

    F.DefineCOM(&_portId, &_comPort);
    F.DefineValue("ShowPortSelect", dtBoolean, &_showPortOverride, F_TRUE);
    F.DefineValue("ShowPortState", dtBoolean, &_showPortState, F_FALSE);
    F.DefineValue("KeepConnected", dtBoolean, &_keepConnected, F_FALSE);

    F.DefineValue("PortGlobalValue", dtPText, &_portGlobalValue);

    if (F.IsReader())
    {
        _activeElementCount = 0;
        for (int i = 0; i < ARRAYSIZE(_elements); i++)
        {
            if (IsNullXGuid(&_elements[i])) continue;
            _activeElementCount++;
        }

        SetBounds(_left, _top, _width, _height);
        _buttonPortSelect->SetVisible(_showPortOverride);

        if (!_connected)
        {
            _portIdOverride = _portIdConnected = 0;
            _tryConnect = true;
        }
    }
}

void CctControl_ElementSerialData::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_ElementSerialData::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    SetPaintTimer(2000);

    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    if (_tryConnect && IsLive())
    {
        GetSession(this)->LoadControlState(this);

        Reset();

        if (_portGlobalValue)
        {
            GLOBALVALUE *globalValue = GetSession(this)->GetGlobalValue(_portGlobalValue);
            if (globalValue)
            {
                _portIdOverride = (BYTE)globalValue->Value;
            }
        }

        if (_portIdOverride != 0) 
        {
            _portIdConnected = _portIdOverride;
        }
        else
        {
            _portIdConnected = _portId;
        }

        _connected = GetEngine(this)->ConnectPort(this, _portIdConnected, &_comPort);

        if (_connected && _sendOnConnect && (_caption == nullptr))
        {
            OnPenClick(0, 0);
        }
    }

    CfxResource *resource = GetResource(); 

    const char *caption = "Disconnected";

    if (_connected)
    {
        caption = "Connecting";
    }

    if (_connected && _caption)
    {
        caption = _caption;
    }

    if (_connected && (!GetEngine(this)->IsPortConnected(_portIdConnected)))
    {
        caption = "Tap to retry";
    }

    FXFONTRESOURCE *font = resource->GetFont(this, _font);
    if (font)
    {
        if (_showPortState)
        {
            pCanvas->State.LineWidth = (int)_borderLineWidth;
            int spacer = 4 * _borderLineWidth;
            int size = _borderLineWidth * 24;
            FXRECT r;
            r.Left = pRect->Left + spacer + _borderWidth;
            r.Top = pRect->Top + spacer + _borderWidth;
            r.Right = pRect->Left + size;
            r.Bottom = pRect->Top + size;
            pCanvas->DrawPortState(&r, font, _portIdConnected, _color, _textColor, _hasData, _transparent);
        }

        pCanvas->State.BrushColor = _color;
        pCanvas->State.TextColor = _textColor;
        if ((_activeElementCount == 0) || (_caption == nullptr))
        {
            pCanvas->DrawTextRect(font, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, caption);
        }
        else
        {
            int i;
            int fontHeight = pCanvas->FontHeight(font);
            int h = fontHeight * _activeElementCount;
            int w = GetClientWidth() / 2;
            int t = pRect->Top + (GetClientHeight() - h) / 2;

            for (i = 0; i < ARRAYSIZE(_elements); i++)
            {
                if (IsNullXGuid(&_elements[i])) continue;

                FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, _elements[i], (BYTE)ElementAttribute::Name);
                if (element)
                {
                    FXRECT rect = *pRect;
                    rect.Top = t;
                    rect.Right = w - + _borderWidth * 2;
                    rect.Bottom = t + fontHeight;
                    FX_ASSERT(element->Magic == MAGIC_TEXT);
                    pCanvas->DrawTextRect(font, &rect, TEXT_ALIGN_RIGHT, element->Text);
                    resource->ReleaseObject(element);
                }
            
                if (_values[i])
                {
                    pCanvas->DrawText(font, pRect->Left + w + _borderWidth * 2, t, _values[i]);
                }

                t += fontHeight + _borderLineWidth * 4;
            }
        }
        resource->ReleaseFont(font);
    }
}

void CctControl_ElementSerialData::OnPenClick(int X, int Y)
{
    if (!_connected) return; 

    if (!GetEngine(this)->IsPortConnected(_portIdConnected))
    {
        Reset();
        _tryConnect = true;
        Repaint();
        FxSleep(1000);
        Repaint();
        return;
    }

    if ((_sendData != nullptr) && (strlen(_sendData) != 0) && (strlen(_sendData) < 60))
    {
        char sendData[64];
        sprintf(sendData, "\n\r%s\n\r", _sendData);
        GetEngine(this)->WritePortData(_portIdConnected, (BYTE *)sendData, strlen(sendData));
    }
}