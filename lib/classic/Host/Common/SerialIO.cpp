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

#include "SerialIO.h"
#include "fxUtils.h"

//*************************************************************************************************

BOOL DisableBluetoothBrowser(WCHAR *pDeviceName)
{
#if defined(_WIN32_WCE)
    if (pDeviceName == NULL) return FALSE;

    // Disable bluetooth browser
    if (wcsncmp(pDeviceName, L"COM", 3) == 0)
    {
        WCHAR bluetoothKey[MAX_PATH];
        wsprintfW(bluetoothKey, L"Software\\WidComm\\BtConfig\\AutoConnect\\000%c", pDeviceName[3]);

        HKEY key;
        if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, bluetoothKey, 0, MAXIMUM_ALLOWED, &key))
        {
            UINT data = 0;
            RegSetValueExW(key, L"BtBrowserEnabled", 0, REG_DWORD, (LPBYTE)&data, sizeof(data));
            RegCloseKey(key);
            return TRUE;
        }
    }
#endif
    return FALSE;
}

BOOL DisableBluetoothBrowserAllPorts()
{
#if defined(_WIN32_WCE)
    UINT i;

    for (i=0; i < 10; i++)
    {
        WCHAR bluetoothKey[MAX_PATH];
        wsprintfW(bluetoothKey, L"Software\\WidComm\\BtConfig\\AutoConnect\\000%d", i);

        HKEY key;
        if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_LOCAL_MACHINE, bluetoothKey, 0, MAXIMUM_ALLOWED, &key))
        {
            UINT data = 0;
            RegSetValueExW(key, L"BtBrowserEnabled", 0, REG_DWORD, (LPBYTE)&data, sizeof(data));
            RegCloseKey(key);
            return TRUE;
        }
    }
#endif
    return FALSE;
}

//*************************************************************************************************
// CSerialIO

CSerialIO::CSerialIO()
{
    _handle = INVALID_HANDLE_VALUE;
    _lastError = 0;
}

CSerialIO::~CSerialIO()
{
    Disconnect();
}

BOOL CSerialIO::Connect(WCHAR *pDevice, const FX_COMPORT *pParams, BOOL PurgeOnConnect, BOOL ReadOnly)
{
    UINT retryCount = 0;

    Disconnect();

    // Must be a COM port
    if (wcsstr(pDevice, L"COM") == NULL)
    {
        return FALSE;
    }

Retry:
    #if defined(_WIN32_WCE)
        _handle = ::CreateFileW(pDevice, 
                                ReadOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,  
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                0,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,     
                                NULL);
    #else
        {
            INT len = wcslen(pDevice);
            CHAR *value = (CHAR *) MEM_ALLOC((len + 1) * sizeof(CHAR));

            if (!value)
            {
                return NULL;
            }

            WideCharToMultiByte(CP_ACP, 0, pDevice, len, value, len, NULL, NULL);
            value[len] = 0;

            _handle = ::CreateFileA(value, 
                                    ReadOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,  
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,     
                                    NULL);

            MEM_FREE(value);
        }
    #endif

    if (_handle == INVALID_HANDLE_VALUE)
    {
        // Hack for cases where the port is correct but is in some sort of weird timed 
        // out state. Retry after a while seems to work.
        DWORD ErrorCode = GetLastError();
        if ((ErrorCode == 0) && (retryCount == 0))
        {
            Sleep(500);
            retryCount++;
            goto Retry;
        }

        goto Done;
    }

    ::SetupComm(_handle, 4096, 4096);

    if (PurgeOnConnect)
    {
        ::PurgeComm(_handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
    }

    // Setup timeouts
    COMMTIMEOUTS timeOuts;
    timeOuts.ReadIntervalTimeout = MAXDWORD; 
    timeOuts.ReadTotalTimeoutMultiplier = 0;
    timeOuts.ReadTotalTimeoutConstant = 0;
    timeOuts.WriteTotalTimeoutMultiplier = 10;
    timeOuts.WriteTotalTimeoutConstant = 1000;
    /*timeOuts.ReadIntervalTimeout = 10; 
    timeOuts.ReadTotalTimeoutMultiplier = 10;
    timeOuts.ReadTotalTimeoutConstant = 100;
    timeOuts.WriteTotalTimeoutMultiplier = 10;
    timeOuts.WriteTotalTimeoutConstant = 100;*/
    


    ::SetCommTimeouts(_handle, &timeOuts);

    // Get COM parameters
    DCB dcb;
    dcb.DCBlength = sizeof(DCB);
    ::GetCommState(_handle, &dcb);

    // Setup COM port
    dcb.BaudRate = pParams->Baud;
    dcb.Parity = pParams->Parity;
    dcb.ByteSize = pParams->Data;
    dcb.StopBits = pParams->Stop;
    dcb.fParity = (pParams->Parity != NOPARITY);
    dcb.fBinary = TRUE;                     

    dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = dcb.fDsrSensitivity = 0;        
    dcb.fOutX = dcb.fInX = dcb.fNull = 0;                  
    dcb.fErrorChar = 0;
    dcb.fAbortOnError = FALSE;

    if (pParams->FlowControl)
    {
        dcb.fDtrControl = DTR_CONTROL_ENABLE;  
        dcb.fRtsControl = RTS_CONTROL_ENABLE;   
        dcb.XonLim = dcb.XoffLim = 100;
        dcb.fTXContinueOnXoff = TRUE;
        dcb.Parity = 0;
    }
    else
    {
        dcb.fDtrControl = DTR_CONTROL_DISABLE;  
        dcb.fRtsControl = RTS_CONTROL_DISABLE;   
    }

    ::SetCommState(_handle, &dcb);

Done:
    if (_handle != INVALID_HANDLE_VALUE)
    {
        ClearError();
    }

    return GetConnected();
}

VOID CSerialIO::Disconnect()
{
    ClearError();
    if (GetConnected())
    {
        ::PurgeComm(_handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
        ::CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
    }
}

VOID CSerialIO::ClearError()
{
    if (GetConnected())
    {
        UINT errors;
        COMSTAT comStat;
        ::ClearCommError(_handle, &errors, &comStat);
    }
    SetLastError(0);
}

VOID CSerialIO::Purge()
{
    if (GetConnected())
    {
        ::PurgeComm(_handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
    }
}

VOID CSerialIO::SetBreak()
{
    if (GetConnected())
    {
        SetCommBreak(_handle);
    }
}

VOID CSerialIO::ClearBreak()
{
    if (GetConnected())
    {
        ClearCommBreak(_handle);
    }
}

UINT CSerialIO::Read(BYTE *pData, UINT Length)
{
    if (!GetConnected())
    {
        return 0;
    }

    ClearError();

    UINT bytesRead = 0;
    if (!ReadFile(_handle, pData, Length, &bytesRead, NULL))
    {
        _lastError = GetLastError();
        bytesRead = 0;
    }

    return bytesRead;
}

UINT CSerialIO::Write(BYTE *pData, UINT Length)
{
    if (!GetConnected())
    {
        return 0;
    }

    UINT bytesWritten;
    BOOL success = WriteFile(_handle, 
                             pData,
                             Length,
                             &bytesWritten,
                             NULL);
    if (!success)
    {
        _lastError = GetLastError();
        return 0;
    }

    return bytesWritten;
}

//*************************************************************************************************
// CSerialDevices

CSerialDevices::CSerialDevices()
{
    _deviceList = NULL;
    _deviceListLen = 0;
    Rescan();
}

CSerialDevices::~CSerialDevices()
{
    MEM_FREE(_deviceList);
}

VOID CSerialDevices::Rescan()
{
    #if defined(_WIN32_WCE)
    {
        HMODULE coreLib = LoadLibrary(L"coredll");
        if (coreLib != NULL) 
        { 
            //
            // BUGBUG: EnumDevices doesn't return newly created Bluetooth ports
            //
            FARPROC enumDevices = NULL;//GetProcAddress(coreLib, L"EnumDevices"); 
            if (enumDevices)
            {
                if (((DWORD(WINAPI *)(LPWSTR, PDWORD))enumDevices)(_deviceList, &_deviceListLen) != ERROR_SUCCESS) 
                {
                    _deviceList = (WCHAR *) MEM_ALLOC(_deviceListLen);
                    if (((DWORD(WINAPI *)(LPWSTR, PDWORD))enumDevices)(_deviceList, &_deviceListLen) != ERROR_SUCCESS)
                    {
                        MEM_FREE(_deviceList);
                        _deviceList = NULL;
                        return;
                    }
                }
            }
            else
            {
                const WCHAR comPorts[] = L"COM9:\0COM8:\0COM7:\0COM6:\0COM5:\0COM4:\0COM3:\0COM2:\0COM1:\0COM0:\0";
                _deviceList = (WCHAR *) MEM_ALLOC(sizeof(comPorts));
                if (_deviceList)
                {
                    memcpy(_deviceList, comPorts, sizeof(comPorts));
                    _deviceListLen = sizeof(comPorts);
                }
            }
            FreeLibrary(coreLib);
        }
    }
    #else
        const WCHAR comPorts[] = L"\\\\.\\COM20\0\\\\.\\COM19\0\\\\.\\COM18\0\\\\.\\COM17\0\\\\.\\COM16\0\\\\.\\COM15\0\\\\.\\COM14\0\\\\.\\COM13\0\\\\.\\COM12\0\\\\.\\COM11\0\\\\.\\COM10\0\\\\.\\COM9\0\\\\.\\COM8\0\\\\.\\COM7\0\\\\.\\COM6\0\\\\.\\COM5\0\\\\.\\COM4\0\\\\.\\COM3\0\\\\.\\COM2\0\\\\.\\COM1\0";
        _deviceList = (WCHAR *) MEM_ALLOC(sizeof(comPorts));
        if (_deviceList)
        {
            memcpy(_deviceList, comPorts, sizeof(comPorts));
            _deviceListLen = sizeof(comPorts);
        }
    #endif
}

WCHAR *CSerialDevices::FindDeviceInList(WCHAR *pDevice)
{
    WCHAR *current = _deviceList;
    while (current[0])
    {
        if (wcscmp(pDevice, current) == 0)
        {
            return current;
        }
        current += wcslen(current) + 1;
    }
    return NULL;
}

WCHAR *CSerialDevices::FindNextDevice(WCHAR *pCurrentDevice)
{
    WCHAR *next = pCurrentDevice;

    while (next && next[0])
    {
        next += wcslen(next) + 1;

        if (next && (wcsncmp(next, L"\\\\.\\COM", 7) == 0))
        {
            break;
        }

        if (next && (wcsncmp(next, L"COM", 3) == 0))
        {
            break;
        }
    }

    if (next && next[0])
    {
        return next;
    }
    else
    {
        return _deviceList;
    }
}

UINT CSerialDevices::GetCount()
{
    UINT count = 0;
    WCHAR *current = _deviceList;
    while (current[0])
    {
        if ((wcsncmp(current, L"\\\\.\\COM", 7) == 0) ||
            (wcsncmp(current, L"COM", 3) == 0))
        {
            count++;
        }

        current += wcslen(current) + 1;
    }
    return count;
}

//*************************************************************************************************

