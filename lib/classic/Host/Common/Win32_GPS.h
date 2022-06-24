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

class CGPS_Win32
{
protected:
    HANDLE _thread;

#ifdef CLIENT_DLL
    BOOL _simulate;
    DOUBLE _simulateCenterLat, _simulateCenterLon;
    UINT _simulateRadius;
    FXGPS _simulateLast;
#endif
public:
	CGPS_Win32();
	~CGPS_Win32();

    VOID Terminate();

    VOID ResetTimeouts();

    VOID GetState(FXGPS *pGPS);
    VOID SetState(BOOL TurnOn);
    WCHAR *GetDeviceName();

    BOOL Detect(BYTE PortOverride, BYTE ParamsOverride);

#ifdef CLIENT_DLL
    BOOL GetSimulate()           { return _simulate;  }
    VOID SetSimulate(BOOL Value) { _simulate = Value; }
    VOID SetupSimulate(DOUBLE CenterLat, DOUBLE CenterLon, UINT Radius);
#endif
};

extern CGPS_Win32 *g_GPS;
