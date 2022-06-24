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

#include "fxTypes.h"
#include "fxList.h"
#include "SerialIO.h"

typedef VOID (*SerialDataCallback)(FX_COMPORT_DATA *pComPortData, PVOID pContext);

struct SERIAL_CALLBACK
{
    SerialDataCallback Callback;
    PVOID Context;
};

class CSerialPorts
{
private:
    CRITICAL_SECTION _callbackLock;
    TList<SERIAL_CALLBACK> _callbacks;
public:
    CSerialPorts();
    ~CSerialPorts();

    VOID Terminate();
    VOID Disconnect();

    VOID AddCallback(SerialDataCallback Callback, PVOID pContext);
    VOID RemoveCallback(SerialDataCallback Callback, PVOID pContext);
    VOID ExecuteCallbacks(FX_COMPORT_DATA *pComPortData);

    BOOL ConnectPort(BYTE PortId, FX_COMPORT *pComPort);
    VOID DisconnectPort(BYTE PortId);
    VOID WritePortData(BYTE PortId, BYTE *pData, UINT Length);

    BOOL IsConnected(BYTE PortId);
};

extern CSerialPorts *g_SerialPorts;
