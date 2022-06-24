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

// 
// CSerialIO
//
class CSerialIO
{
protected:
	HANDLE _handle;
    UINT _lastError;
public:
	CSerialIO();
	~CSerialIO();

    BOOL Connect(WCHAR *pDevice, const FX_COMPORT *pParams, BOOL PurgeOnConnect, BOOL ReadOnly);
    VOID Disconnect();
    BOOL GetConnected()   { return _handle != INVALID_HANDLE_VALUE; }
    VOID Purge();

    UINT GetLastError() { return _lastError; }
    VOID ClearError();

    VOID SetBreak();
    VOID ClearBreak();

	UINT Write(BYTE *pData, UINT Length);
	UINT Read(BYTE *pData, UINT Length);
};

//
// CSerialDevices
//
class CSerialDevices
{
protected:
    WCHAR *_deviceList;
    UINT _deviceListLen;
public:
    CSerialDevices();
    ~CSerialDevices();

    WCHAR *FindDeviceInList(WCHAR *pDevice);
    WCHAR *FindNextDevice(WCHAR *pCurrentDevice);

    VOID Rescan();
    UINT GetCount();
};

extern BOOL DisableBluetoothBrowser(WCHAR *pDeviceName);
extern BOOL DisableBluetoothBrowserAllPorts();