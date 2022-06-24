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
#include "fxHost.h"
#include "Win32_RangeFinder.h"

#include "fxRangeFinderParse.h"
#include "SerialIO.h"

//*************************************************************************************************

#define SERIAL_RANGEFINDER_BUFFER_SIZE            256
#define SERIAL_RANGEFINDER_MAX_FIX_AGE_SECONDS    0xFFFFFFFF
#define SERIAL_RANGEFINDER_PING_TIME              2000
#define SERIAL_RANGEFINDER_DETECT_TIMEOUT_SECONDS 30

const FX_COMPORT g_rangeFinderDetectPortParams[] = {
    {CBR_4800,  NOPARITY,  DATABITS_8, ONESTOPBIT, FALSE },
    {CBR_4800,  NOPARITY,  DATABITS_8, ONESTOPBIT, FALSE },
    {CBR_9600,  NOPARITY,  DATABITS_8, ONESTOPBIT, FALSE },
    {CBR_19200, NOPARITY,  DATABITS_8, ONESTOPBIT, FALSE },
    {CBR_38400, NOPARITY,  DATABITS_8, ONESTOPBIT, FALSE },
    {CBR_57600, NOPARITY,  DATABITS_8, ONESTOPBIT, FALSE },
    {CBR_115200, NOPARITY, DATABITS_8, ONESTOPBIT, FALSE },
    {CBR_2400,  NOPARITY,  DATABITS_8, ONESTOPBIT, FALSE },
    };

#define RANGEFINDER_DEVICE_VALUE L"RangeDevice"
#define RANGEFINDER_PARAMS_VALUE L"RangeParams"

//
// CSerialRangeFinder
//
class CSerialRangeFinder: public CfxRangeFinder
{
protected:
    CSerialIO _serial;
    CSerialDevices _devices;
    CfxRangeFinderParser *_parser;
    CHAR *_buffer;

    UINT _lastPingTimeStamp;
    
    BOOL _pendingDisconnect;

    FXDEVICE_STATE _state;
    BOOL _detectLocked;
    WCHAR *_detectDevice;
    UINT _detectParamsIndex;
    INT _detectCounter;
    BOOL _detectHasData;
    UINT _detectStartTimeStamp;

    WCHAR *_foundDevice;
    UINT _foundParamsIndex;

    BOOL DetectParser();
    BOOL DetectDevice();
    VOID DetectIterate();
public:
    CSerialRangeFinder(BYTE PortOverride, BYTE ParamsOverride);
    ~CSerialRangeFinder();

    BOOL IsPresent();
    VOID ResetTimeouts();
    VOID GetState(FXRANGE *pRange);
    VOID SetState(BOOL TurnOn);
    BOOL CanIterate();
    VOID Iterate();
};

//*************************************************************************************************

