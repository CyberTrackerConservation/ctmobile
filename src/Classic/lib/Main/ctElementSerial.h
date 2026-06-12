#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "fxButton.h"
#include "ctElement.h"

//*************************************************************************************************
//
// CctControl_ElementSerialData
//

const GUID GUID_CONTROL_ELEMENTSERIALDATA = {0x10e96b73, 0x7774, 0x40ff, {0x8b, 0x6a, 0x12, 0x90, 0xd1, 0x93, 0x34, 0x1}};

class CctControl_ElementSerialData: public CfxControl_Panel
{
protected:
    XGUID _elements[30] = {};
    char *_values[30] = {};
    XGUID _resultElementId = 0;

    int _activeElementCount = 0;

    bool _autoNext = false;
    bool _clearOnEnter = false;

    bool _sendOnConnect = false;
    char *_sendData = nullptr;
    char *_separator = nullptr;

    bool _keepConnected = false;

    BYTE _portId = 0, _portIdOverride = 0, _portIdConnected = 0;
    FX_COMPORT _comPort;
    bool _connected = false, _tryConnect = false, _hasData = false, _showPortState = false;

    int _dataIndex = 0;
    BYTE _data[1024] = {};

    char *_portGlobalValue = nullptr;

    bool _showPortOverride = false;
    CfxControl_Button *_buttonPortSelect = nullptr;
    void OnPortSelectPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnPortSelectClick(CfxControl *pSender, void *pParams);

    void Reset(bool KeepConnected = false);
    void OnSessionNext(CfxControl *pSender, void *pParams);
    bool ParseData(char *pData);
public:
    CctControl_ElementSerialData(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementSerialData();

    void OnPortData(BYTE PortId, BYTE *pData, int Length) override;

    void SetBounds(int Left, int Top, int Width, int Height) override;

    void DefineState(CfxFiler &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnDialogResult(GUID *pDialogId, void *pParams) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    void OnPenClick(int X, int Y) override;
};

extern CfxControl *Create_Control_ElementSerialData(CfxPersistent *pOwner, CfxControl *pParent);

