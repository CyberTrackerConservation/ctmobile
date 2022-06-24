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
#include "fxPanel.h"
#include "fxButton.h"
#include "ctElement.h"
#include "fxGPSParse.h"
#include "fxRangeFinderParse.h"

//*************************************************************************************************
//
// CctControl_ElementSerialData
//

const GUID GUID_CONTROL_ELEMENTSERIALDATA = {0x10e96b73, 0x7774, 0x40ff, {0x8b, 0x6a, 0x12, 0x90, 0xd1, 0x93, 0x34, 0x1}};

class CctControl_ElementSerialData: public CfxControl_Panel
{
protected:
    XGUID _elements[30];
    CHAR *_values[30];
    XGUID _resultElementId;

    INT _activeElementCount;

    BOOL _autoNext;
    BOOL _clearOnEnter;

    BOOL _sendOnConnect;
    CHAR *_sendData;
    CHAR *_separator;

    BOOL _keepConnected;

    BYTE _portId, _portIdOverride, _portIdConnected;
    FX_COMPORT _comPort;
    BOOL _connected, _tryConnect, _hasData, _showPortState;

    UINT _dataIndex;
    BYTE _data[1024];
    
    CHAR *_portGlobalValue;

    BOOL _showPortOverride;
    CfxControl_Button *_buttonPortSelect;
    VOID OnPortSelectPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnPortSelectClick(CfxControl *pSender, VOID *pParams);

    VOID Reset(BOOL KeepConnected = FALSE);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
    BOOL ParseData(CHAR *pData);
public:
    CctControl_ElementSerialData(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementSerialData();

    VOID OnPortData(BYTE PortId, BYTE *pData, UINT Length);

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);

    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnDialogResult(GUID *pDialogId, VOID *pParams);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    VOID OnPenClick(INT X, INT Y);
};

extern CfxControl *Create_Control_ElementSerialData(CfxPersistent *pOwner, CfxControl *pParent);

