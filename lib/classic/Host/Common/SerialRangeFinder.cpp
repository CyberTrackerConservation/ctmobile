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

#include "SerialRangeFinder.h"
#include "fxLaserAtlanta.h"

//*************************************************************************************************

//#define RF_EMULATION

//*************************************************************************************************
// CSerialRangeFinder

CSerialRangeFinder::CSerialRangeFinder(BYTE PortOverride, BYTE ParamsOverride): CfxRangeFinder(), _serial(), _devices()
{
    _buffer = (CHAR *)MEM_ALLOC(SERIAL_RANGEFINDER_BUFFER_SIZE);
    _parser = NULL;

    _lastPingTimeStamp = 0;

    _foundDevice = NULL;
    _foundParamsIndex = 0;

    _pendingDisconnect = FALSE;

    _state = dsNotFound;
    _detectLocked = FALSE;
    _detectDevice = NULL;
    _detectParamsIndex = 0;
    _detectHasData = FALSE;
    _detectStartTimeStamp = 0;

    if (PortOverride != 0)
	{
        WCHAR deviceName[MAX_PATH];
        #if defined(_WIN32_WCE)
            swprintf(deviceName, L"COM%d:", PortOverride);
        #else
            swprintf(deviceName, L"\\\\.\\COM%d", PortOverride);
        #endif
        _detectDevice = _devices.FindDeviceInList(deviceName);
        _detectParamsIndex = ParamsOverride;
        _detectLocked = (_detectDevice != NULL);
	}
    else // Pick up the last detected RangeFinder
    {
        HKEY key;
        if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, CLIENT_REGISTRY_KEY, 0, MAXIMUM_ALLOWED, &key))
        {
            WCHAR deviceName[MAX_PATH];
            UINT dataType;
            UINT dataSize = sizeof(deviceName);
            if (ERROR_SUCCESS == RegQueryValueExW(key, RANGEFINDER_DEVICE_VALUE, NULL, &dataType, (LPBYTE)&deviceName, &dataSize))
            {
                UINT deviceIndex;
                dataSize = sizeof(deviceIndex);
                if (ERROR_SUCCESS == RegQueryValueExW(key, RANGEFINDER_PARAMS_VALUE, NULL, &dataType, (LPBYTE)&deviceIndex, &dataSize))
                {
                    _detectDevice = _devices.FindDeviceInList(deviceName);
                    _detectParamsIndex = deviceIndex;
                    #if defined(_WIN32_WCE)
                        _detectLocked = _detectDevice != NULL;
                    #endif
                }
            }
            RegCloseKey(key);
        }
    }

    if (_detectLocked == FALSE)
    {
        DisableBluetoothBrowserAllPorts();
    }
}

CSerialRangeFinder::~CSerialRangeFinder()
{
    delete _parser;
    MEM_FREE(_buffer);
}

BOOL CSerialRangeFinder::DetectParser()
{
    _detectHasData = FALSE;

    delete _parser;
    _parser = NULL;

    // loopCount makes sure we don't get stuck here forever for a high frequency sender
    UINT loopCount = 0;
    UINT bytesRead = 0;
    while ((loopCount < 8) && !_parser)
    {
        Sleep(100);
        bytesRead = _serial.Read((BYTE *)_buffer, SERIAL_RANGEFINDER_BUFFER_SIZE-1);
        if (bytesRead == 0) break; 

        _detectHasData = TRUE;
        _buffer[bytesRead] = 0;
        loopCount++;

        if (IsValidLaserAtlantaStream((BYTE *)_buffer, bytesRead))
        {
            _parser = new CfxLaserAtlantaParser();
            _serial.Purge();
        }
    }

    return _parser != NULL;
}

BOOL CSerialRangeFinder::DetectDevice()
{
    _detectHasData = FALSE;

    if (_serial.Connect(_detectDevice, &g_rangeFinderDetectPortParams[_detectParamsIndex], FALSE, FALSE))
    {
        Sleep(_detectLocked ? 1000 : 500);

        if (DetectParser())
        {
            _foundDevice = _detectDevice;
            _foundParamsIndex = _detectParamsIndex;
            _state = dsConnected;

            // Cache the successful device find
            HKEY key;
            if (ERROR_SUCCESS == RegCreateKeyExW(HKEY_CURRENT_USER, CLIENT_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, &key, NULL))
            {
                RegSetValueExW(key, RANGEFINDER_DEVICE_VALUE, 0, REG_SZ, (LPBYTE)_foundDevice, wcslen(_foundDevice) * sizeof(WCHAR));
                RegSetValueExW(key, RANGEFINDER_PARAMS_VALUE, 0, REG_DWORD, (LPBYTE)&_foundParamsIndex, sizeof(_foundParamsIndex));
                RegCloseKey(key);
            }

            #if defined(_WIN32_WCE)
                // Disable annoying Bluetooth browser
                DisableBluetoothBrowser(_foundDevice);

                // Lock the detection so we don't scan after having found it once.
                _detectLocked = TRUE;
            #endif
 
            return TRUE;
        }
        _serial.Disconnect();
    }

    return FALSE;
}

VOID CSerialRangeFinder::DetectIterate()
{
    FX_ASSERT(_state == dsDetecting);
    if (_detectDevice == NULL)
    {
        FX_ASSERT(!_detectLocked);
        _detectDevice = _devices.FindNextDevice(NULL);
    }

    if (DetectDevice()) return;

    if (_detectLocked)
    {
        // Detect is locked, so use the detectCounter as an on/off switch
        _detectCounter = (FxGetTickCount() - _detectStartTimeStamp > SERIAL_RANGEFINDER_DETECT_TIMEOUT_SECONDS * FxGetTicksPerSecond()) ? -1 : 1;
    }
    else
    {
        // Iterate to the next available port/baud rate

        if (_detectHasData)
        {
            // Unrecognized data: advance port settings
            _detectParamsIndex++;
        }
        else
        {
            // No data on this port: force next port
            _detectParamsIndex = ARRAYSIZE(g_rangeFinderDetectPortParams);
        }

        // If we're at the end of the params list, go to the next device
        if (_detectParamsIndex >= ARRAYSIZE(g_rangeFinderDetectPortParams))
        {
            _detectDevice = _devices.FindNextDevice(_detectDevice);
            _detectParamsIndex = 0;
            _detectCounter--;
        }
    }
}

BOOL CSerialRangeFinder::IsPresent()
{
    return TRUE;
}

VOID CSerialRangeFinder::ResetTimeouts()
{
    switch (_state)
    {
    case dsDetecting: _detectStartTimeStamp = FxGetTickCount(); break;
    case dsConnected: _parser->ResetTimeOut(); break;
    }
}

VOID CSerialRangeFinder::GetState(FXRANGE *pRange)
{
#ifdef RF_EMULATION
    memset(pRange, 0, sizeof(FXRANGE));
    pRange->Range.Range = 100;
    pRange->Range.RangeValid = TRUE;
    pRange->Range.RangeUnits = 'M';
    pRange->State = dsConnected;
    pRange->TimeStamp = FxGetTickCount();
    return;
#endif

    if (_state == dsConnected)
    {
        if (!_parser->GetRange(pRange, SERIAL_RANGEFINDER_MAX_FIX_AGE_SECONDS))
        {
            SetState(FALSE);
        }
    }
    else
    {
        memset(pRange, 0, sizeof(FXRANGE));
    }

    pRange->State = _state;
}

VOID CSerialRangeFinder::SetState(BOOL TurnOn)
{
    if (TurnOn)
    {
        _pendingDisconnect = FALSE;

		if (_state != dsConnected)
        {
            //
            // While the application is running, changes to the device list can happen:
            // e.g. plugging in a CF GPS. To fix this we rescan the device list whenever we
            // go from not connected (and not detecting) to detecting.
            //
            if ((_state != dsDetecting) && !_detectLocked)
            {
                _devices.Rescan();
                _detectDevice = NULL;
            }

            _state = dsDetecting;
            _detectCounter = _devices.GetCount();
        }
        ResetTimeouts();
    }
    else
    {
        _pendingDisconnect = TRUE;
        _state = _foundDevice == NULL ? dsNotFound : dsDisconnected;
    }
}

BOOL CSerialRangeFinder::CanIterate()
{
#ifdef RF_EMULATION
    return TRUE;
#else
    return (_pendingDisconnect || (_state == dsDetecting) || (_state == dsConnected));
#endif
}

VOID CSerialRangeFinder::Iterate()
{
#ifdef RF_EMULATION
    return;
#else
    FX_ASSERT(CanIterate());

    if (_state == dsDetecting)
    {
        // Detect if we are less than detect time
        if (_detectCounter >= 0)
        {
            DetectIterate();
        }
        else
        {
            SetState(FALSE);
        }
    }

    if (_state == dsConnected)
    {
        FX_ASSERT(_parser != NULL);
        FX_ASSERT(_serial.GetConnected());

        UINT currentTime = FxGetTickCount();

        // Ping the serial port if required
        if (currentTime - _lastPingTimeStamp > SERIAL_RANGEFINDER_PING_TIME)
        {
            BYTE *buffer = NULL;
            UINT length = 0;
            if (_parser->GetPingStream(&buffer, &length))
            {
                _serial.Write(buffer, length);
                MEM_FREE(buffer);
            }
            _lastPingTimeStamp = currentTime;
        }

        // Read from the device
        UINT bytesRead = _serial.Read((BYTE *)_buffer, SERIAL_RANGEFINDER_BUFFER_SIZE-1);
        _buffer[bytesRead] = 0;
        if (bytesRead > 0)
        {
            _parser->ParseBuffer(_buffer, bytesRead);
        }
        else
        {
            Sleep(100);
        }
    }

    // Disconnect if someone called SetState(FALSE);
    if (_pendingDisconnect)
    {
        _pendingDisconnect = FALSE;
        _serial.Disconnect();
    }
#endif
}

