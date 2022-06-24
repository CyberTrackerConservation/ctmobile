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
#include "ctElementSerial.h"
#include "ctDialog_ComPortSelect.h"
#include "ctSession.h"
#include "ctActions.h"
#include "fxNMEA.h"
#include "fxLaserAtlanta.h"

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

    _autoNext = FALSE;
    _clearOnEnter = FALSE;
    _sendOnConnect = TRUE;
    _sendData = NULL;
    _activeElementCount = 0;

    _keepConnected = FALSE;
    _hasData = FALSE;

    _portId = _portIdOverride = _portIdConnected = 0;
    InitCOM(&_portId, &_comPort);
    _connected = _tryConnect = FALSE;

    _separator = NULL;

    _showPortOverride = TRUE;
    _showPortState = FALSE;

    _portGlobalValue = NULL;

    _buttonPortSelect = new CfxControl_Button(pOwner, this);
    _buttonPortSelect->SetComposite(TRUE);
    _buttonPortSelect->SetVisible(_showPortOverride);
    _buttonPortSelect->SetBorder(bsNone, 0, 0, _borderColor);
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
    _sendData = NULL;

    MEM_FREE(_separator);
	_separator = NULL;

    MEM_FREE(_portGlobalValue);
    _portGlobalValue = NULL;

    SetCaption(NULL);

    for (INT i = 0; i < ARRAYSIZE(_values); i++)
    {
        MEM_FREE(_values[i]);
        _values[i] = 0;
    }
}

VOID CctControl_ElementSerialData::Reset(BOOL KeepConnected)
{
    if (_connected && IsLive())
    {
        GetEngine(this)->DisconnectPort(this, _portIdConnected, KeepConnected);
    }

    _portIdConnected = 0;
    _connected = _tryConnect = FALSE;

    _dataIndex = 0;
    memset(&_data[0], 0, sizeof(_data));

    _hasData = FALSE;
}

BOOL CctControl_ElementSerialData::ParseData(CHAR *pData)
{
    if (pData == NULL) return FALSE;
    if ((_separator == NULL) || (strlen(_separator) != 1)) return FALSE;
    if (strchr(pData, *_separator) == NULL) return FALSE;

    BOOL result = FALSE;
    CHAR *curr = pData;
    CHAR *last = pData + strlen(pData);

    for (INT i = 0; i < ARRAYSIZE(_values); i++)
    {
        MEM_FREE(_values[i]);
        _values[i] = NULL;

        CHAR *next = strchr(curr, *_separator);
        if (next == NULL)
        {
            next = last;
        }

        INT len = next - curr + 1;
        _values[i] = (CHAR *)MEM_ALLOC(len);
        memcpy(_values[i], curr, len);
        _values[i][len - 1] = '\0';
        result = TRUE;

        if (next == last)
        {
            break;
        }

        curr = next + 1;
    }

    return result;
}

VOID CctControl_ElementSerialData::OnPortSelectClick(CfxControl *pSender, VOID *pParams)
{
    PORTSELECT_PARAMS params;
    params.ControlId = GetId();
    params.PortId = _portId;
    if (_portIdOverride != 0)
    {
        params.PortId = _portIdOverride;
    }
    
    _portIdOverride = 0;

    Reset();

    GetSession(this)->ShowDialog(&GUID_DIALOG_PORTSELECT, NULL, &params, sizeof(PORTSELECT_PARAMS)); 
}

VOID CctControl_ElementSerialData::OnDialogResult(GUID *pDialogId, VOID *pParams)
{
    // Ignore overrides from the global value
    MEM_FREE(_portGlobalValue);
    _portGlobalValue = NULL;

    SetCaption(NULL);

    _portIdOverride = (BYTE)(UINT64)pParams;
    GetSession(this)->SaveControlState(this);

    Reset();
    _tryConnect = TRUE;
    Repaint();
}

VOID CctControl_ElementSerialData::OnPortSelectPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPort(pParams->Rect, pParams->Canvas->InvertColor(_color), pParams->Canvas->InvertColor(_borderColor), _buttonPortSelect->GetDown()); 
}

VOID CctControl_ElementSerialData::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    CctAction_MergeAttribute action(this);
    BOOL use = FALSE;

    if (!IsNullXGuid(&_resultElementId))
    {
        use = TRUE;
        action.SetupAdd(_id, _resultElementId, dtPText, &_caption);
    }

    for (INT i=0; i < ARRAYSIZE(_elements); i++)
    {
        if ((!IsNullXGuid(&_elements[i])) &&
            (_values[i] != NULL))
        {
            use = TRUE;
            action.SetupAdd(_id, _elements[i], dtPText, &_values[i]);
        }
    }

    if (use)
    {
        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

VOID CctControl_ElementSerialData::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl_Panel::SetBounds(Left, Top, Width, Height);

    UINT spacer = 4 * _borderLineWidth;
    UINT size = _borderLineWidth * 24;
    _buttonPortSelect->SetBounds(Width - size - spacer - _borderWidth, spacer + _borderWidth, size, size);
}

VOID CctControl_ElementSerialData::OnPortData(BYTE PortId, BYTE *pData, UINT Length)
{
    if (PortId != _portIdConnected) return;

    FxResetAutoOffTimer();

    UINT i;
    BOOL hasData = FALSE;

    for (i = 0; i < Length; i++, pData++)
    {
        if ((*pData == 10) || (*pData == 13))
        {
            if (_dataIndex > 0)
            {
                if ((_activeElementCount == 0) || (ParseData((char *)&_data[0])))
                {
                    _hasData = TRUE;
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

VOID CctControl_ElementSerialData::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Panel::DefineResources(F);
    F.DefineObject(_resultElementId);
    for (INT i=0; i<ARRAYSIZE(_elements); i++)
    {
        F.DefineObject(_elements[i]);
    }

    F.DefineField(_resultElementId, dtText);
#endif
}

VOID CctControl_ElementSerialData::DefineState(CfxFiler &F)
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
            SetCaption(NULL);
        }
    }
}

VOID CctControl_ElementSerialData::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    for (INT i=0; i<ARRAYSIZE(_elements); i++)
    {
        CHAR varName[32];
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
        for (UINT i = 0; i < ARRAYSIZE(_elements); i++)
        {
            if (IsNullXGuid(&_elements[i])) continue;
            _activeElementCount++;
        }

        SetBounds(_left, _top, _width, _height);
        _buttonPortSelect->SetVisible(_showPortOverride);

        if (!_connected)
        {
            _portIdOverride = _portIdConnected = 0;
            _tryConnect = TRUE;
        }
    }
}

VOID CctControl_ElementSerialData::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Auto next",     dtBoolean, &_autoNext);

    F.DefineValue("Border color",  dtColor,   &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style",  dtInt8,    &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width",  dtInt32,   &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Clear on enter", dtBoolean, &_clearOnEnter);
    F.DefineValue("Color",         dtColor,   &_color);

    F.DefineValue("COM - Global value", dtPText, &_portGlobalValue);
    F.DefineCOM(&_portId, &_comPort);

    F.DefineValue("Dock",         dtByte,    &_dock,        "LIST(None Top Bottom Left Right Fill)");

    for (INT i=0; i<ARRAYSIZE(_elements); i++)
    {
        CHAR items[32];
        sprintf(items, "Elements %d", i + 1);
        F.DefineValue(items, dtGuid, &_elements[i], "EDITOR(ScreenElementsEditor)");
    }

    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);

    F.DefineValue("Keep connected", dtBoolean, &_keepConnected);

    F.DefineValue("Result Element", dtGuid, &_resultElementId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Send data",    dtPText,  &_sendData);
    F.DefineValue("Send on connect", dtBoolean, &_sendOnConnect);
    F.DefineValue("Separator",     dtPText, &_separator);

    F.DefineValue("Show port select", dtBoolean, &_showPortOverride);
    F.DefineValue("Show port state", dtBoolean, &_showPortState);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_ElementSerialData::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
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

        if (_connected && _sendOnConnect && (_caption == NULL))
        {
            OnPenClick(0, 0);
        }
    }

    CfxResource *resource = GetResource(); 

    CHAR *caption = "Disconnected";

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
            pCanvas->State.LineWidth = (UINT)_borderLineWidth;
            UINT spacer = 4 * _borderLineWidth;
            UINT size = _borderLineWidth * 24;
            FXRECT r;
            r.Left = pRect->Left + spacer + _borderWidth;
            r.Top = pRect->Top + spacer + _borderWidth;
            r.Right = pRect->Left + size;
            r.Bottom = pRect->Top + size;
            pCanvas->DrawPortState(&r, font, _portIdConnected, _color, _textColor, _hasData, _transparent);
        }

        pCanvas->State.BrushColor = _color;
        pCanvas->State.TextColor = _textColor;
        if ((_activeElementCount == 0) || (_caption == NULL))
        {
            pCanvas->DrawTextRect(font, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, caption);
        }
        else
        {
            INT i;
            UINT fontHeight = pCanvas->FontHeight(font);
            INT h = fontHeight * _activeElementCount;
            INT w = GetClientWidth() / 2;
            INT t = pRect->Top + (GetClientHeight() - h) / 2;

            for (i = 0; i < ARRAYSIZE(_elements); i++)
            {
                if (IsNullXGuid(&_elements[i])) continue;

                FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, _elements[i], eaName);
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

VOID CctControl_ElementSerialData::OnPenClick(INT X, INT Y)
{
    if (!_connected) return; 

    if (!GetEngine(this)->IsPortConnected(_portIdConnected))
    {
        Reset();
        _tryConnect = TRUE;
        Repaint();
        FxSleep(1000);
        Repaint();
        return;
    }

    if ((_sendData != NULL) && (strlen(_sendData) != 0) && (strlen(_sendData) < 60))
    {
        CHAR sendData[64];
        sprintf(sendData, "\n\r%s\n\r", _sendData);
        GetEngine(this)->WritePortData(_portIdConnected, (BYTE *)sendData, strlen(sendData));
    }
}