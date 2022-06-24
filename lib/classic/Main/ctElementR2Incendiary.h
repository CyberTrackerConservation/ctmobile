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
#include "ctElement.h"

//*************************************************************************************************
//
// CctControl_ElementR2Incendiary
//

const GUID GUID_CONTROL_ELEMENTR2INCENDIARY = {0xcf2a83ab, 0x6b19, 0x4932, {0xa9, 0xdb, 0x10, 0x10, 0x8c, 0x78, 0x6a, 0x45}};

enum R2INCENDIARY_PARSE_STATE {rpsStart, rpsCounter, rpsLength, rpsData, rpsChecksum};
enum R2INCENDIARY_SAVE_ACTION {rsaNone, rsaWaypoint, rsaSighting};

class CctControl_ElementR2Incendiary: public CfxControl_Panel
{
protected:
    BYTE _portId;
    FX_COMPORT _comPort;
    BOOL _connected, _lastConnected;
    BOOL _fastSave;

    R2INCENDIARY_PARSE_STATE _state;
    R2INCENDIARY_SAVE_ACTION _onCapsuleAction;
    R2INCENDIARY_SAVE_ACTION _onBlankAction;
    R2INCENDIARY_SAVE_ACTION _onStatusAction;

    BYTE _counter;
    UINT _dataIndex;
    BYTE _dataLength;
    BYTE _data[256];
    BYTE _expectedChecksum;

    BOOL _hasLastCounter;
    INT _lastCounter;
    UINT _heartbeatCount;

    CHAR *_resultHeartbeatCount;
    CHAR *_resultDropRate;
    CHAR *_resultCapsuleCount;
    CHAR *_resultCapsuleRollCount;
    CHAR *_resultCapsuleFlightTotal;
    CHAR *_resultBlankCount;
    CHAR *_resultLostPackets;
    CHAR *_resultBadPackets;
    CHAR *_resultNoGPS;

    XGUID _statusElementId;

    VOID Reset();
    VOID IncGlobalValue(CHAR *pName);
    VOID ParsePacket(BYTE *pData, UINT Length);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
    VOID ExecuteAction(R2INCENDIARY_SAVE_ACTION Action);
public:
    CctControl_ElementR2Incendiary(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementR2Incendiary();

    VOID OnPortData(BYTE PortId, BYTE *pData, UINT Length);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID OnTimer();
};

extern CfxControl *Create_Control_ElementR2Incendiary(CfxPersistent *pOwner, CfxControl *pParent);

