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

#include "Win32_GPS.h"
#include "SerialGPS.h"
#include "fxMath.h"
#include "fxUtils.h"

#if defined(_WIN32_WCE)
	#include "Mobile5.h"
    #include "GarminM5.h"
#endif

CGPS_Win32 *g_GPS;

//*************************************************************************************************
// CGPS_Win32

struct CGPSThread
{
    HANDLE OnOffEvent;
    BOOL Terminate;
    CRITICAL_SECTION ThreadLock, StateLock;
    CfxGPS *Device;
    FXGPS GPS;
    BOOL PendingTurnOn;

    VOID Create()
    {
        ::InitializeCriticalSection(&ThreadLock);
        ::InitializeCriticalSection(&StateLock);

        OnOffEvent = ::CreateEventW(NULL, TRUE, FALSE, NULL);
        ::ResetEvent(OnOffEvent);
    }

    VOID Destroy()
    {
        ::CloseHandle(OnOffEvent);

        ::DeleteCriticalSection(&ThreadLock);
        ::DeleteCriticalSection(&StateLock);

        delete Device;
        Device = NULL;
    }

    VOID Detect(BYTE PortOverride, BYTE ParamsOverride)
    {
        ::ResetEvent(OnOffEvent);
        ::EnterCriticalSection(&ThreadLock);

        delete Device;
        Device = NULL;

		if (PortOverride == 0)
		{
        #if defined(_WIN32_WCE)
        #if defined(_ARM_)
            if (!Device)
            {
                Device = new CGarminGPS();
                if (!Device->IsPresent())
                {
                    delete Device;
                    Device = NULL;
                }
            }

			if (!Device)
			{
				Device = new CMobile5GPS();
				if (!Device->IsPresent())
				{
					delete Device;
					Device = NULL;
				}
			}
        #endif
        #endif
		}

        if (!Device)
        {
            Device = new CSerialGPS(PortOverride, ParamsOverride);
            if (!Device->IsPresent())
            {
                delete Device;
                Device = NULL;
            }
        }

        ::LeaveCriticalSection(&ThreadLock);

        // Let the thread run if the device is good to go
        if (Device && Device->CanIterate())
        {
            ::SetEvent(OnOffEvent);
        }
    }

    VOID ResetTimeouts()
    {
        if (Device)
        {
            Device->ResetTimeouts();
        }
    }

    VOID GetState(FXGPS *pGPS)
    {
        ::EnterCriticalSection(&StateLock);
        
        // Validate the state against a pending change
        if (PendingTurnOn)
        {
            if ((GPS.State == dsNotFound) || (GPS.State == dsDisconnected))
            {
                GPS.State = dsDetecting;
            }
        }
        else
        {
            FX_ASSERT(!PendingTurnOn);
            GPS.State = dsDisconnected;
        }

        // Populate the GPS state
        memcpy(pGPS, &GPS, sizeof(FXGPS));

        ::LeaveCriticalSection(&StateLock);
    }

    VOID SetState(BOOL TurnOn)
    {
        if (Device)
        {
            // Set the PendingTurnOn state
            ::EnterCriticalSection(&StateLock);
            PendingTurnOn = TurnOn;
            ::LeaveCriticalSection(&StateLock);

            // Reset any timeouts
            Device->ResetTimeouts();

            // Let the thread run
            ::SetEvent(OnOffEvent);
        }
        else
        {
            // No need to run the thread
            ::ResetEvent(OnOffEvent);
        }
    }
};

CGPSThread g_gpsThread;

DWORD GPSThreadProc(CGPS_Win32 *pGPS)
{
    CGPSThread *threadData = &g_gpsThread;

    while ((WaitForSingleObject(threadData->OnOffEvent, INFINITE) == WAIT_OBJECT_0) && !threadData->Terminate)
    {
        ::EnterCriticalSection(&threadData->ThreadLock);

        if (threadData->Device)
        {
            // Snap the state and get pendingTurnOn
            ::EnterCriticalSection(&threadData->StateLock);
            threadData->Device->GetState(&threadData->GPS);
            FXDEVICE_STATE currentState = threadData->GPS.State; 
            BOOL pendingTurnOn = threadData->PendingTurnOn;
            ::LeaveCriticalSection(&threadData->StateLock);

            // Change state based on pendingTurnOn
            BOOL noChange1 = pendingTurnOn && ((currentState == dsConnected) || (currentState == dsDetecting));
            BOOL noChange2 = !pendingTurnOn && ((currentState == dsNotFound) || (currentState == dsDisconnected));
            if (!noChange1 && !noChange2)
            {
                threadData->Device->SetState(pendingTurnOn);
            }

            // Iterate if we can
            if (threadData->Device->CanIterate())
            {
                FxResetAutoOffTimer();
                threadData->Device->Iterate();
            }

            // 
            // Test again, because we called Iterate so the state may have changed
            // which means we need to turn off right away
            // 
            if (!threadData->Device->CanIterate())
            {
                // Turn off pendingTurnOn: it didn't work
                ::EnterCriticalSection(&threadData->StateLock);
                threadData->Device->GetState(&threadData->GPS);
                threadData->PendingTurnOn = FALSE;
                ::LeaveCriticalSection(&threadData->StateLock);
                
                // If the device can't iterate, stop the thread
                ::ResetEvent(threadData->OnOffEvent);
            }

            if (!threadData->Terminate && (threadData->GPS.State != dsDetecting))
            {
                Sleep(250);
            }
        }

        ::LeaveCriticalSection(&threadData->ThreadLock);
    }

    threadData->Destroy();
    delete pGPS;

    return 1;
}

//*************************************************************************************************
// CGPS_Win32

CGPS_Win32::CGPS_Win32()
{
#ifdef CLIENT_DLL
    _simulate = TRUE;
    _simulateCenterLat = 47.6408729;
    _simulateCenterLon = -122.1274567;
    _simulateRadius = 1000;
    memset(&_simulateLast, 0, sizeof(_simulateLast));
#endif

    memset(&g_gpsThread, 0, sizeof(g_gpsThread));
    g_gpsThread.Create();
    g_gpsThread.Detect(0, 0);

    _thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GPSThreadProc, NULL, CREATE_SUSPENDED, NULL);
    //SetThreadPriority(_thread, THREAD_PRIORITY_BELOW_NORMAL);
    ResumeThread(_thread);
}

CGPS_Win32::~CGPS_Win32()
{
    if (_thread != NULL)
    {
        CloseHandle(_thread);
    }
}

VOID CGPS_Win32::Terminate()
{
    g_gpsThread.Terminate = TRUE;
    ::SetEvent(g_gpsThread.OnOffEvent);

    UINT exitCode;
    if (GetExitCodeThread(_thread, &exitCode))
    {
        if ((exitCode != 1) && (exitCode != STILL_ACTIVE))
        {
            g_gpsThread.Destroy();
            delete this;
        }
    }
}

BOOL CGPS_Win32::Detect(BYTE PortOverride, BYTE ParamsOverride)
{
#ifdef CLIENT_DLL
    if (_simulate)
    {
        return TRUE;
    }
#endif

    if (PortOverride != 0)
    {
        g_gpsThread.Detect(PortOverride, ParamsOverride);
    }

    return g_gpsThread.Device != NULL;
}

VOID CGPS_Win32::ResetTimeouts()
{
    g_gpsThread.ResetTimeouts();
}

#ifdef CLIENT_DLL
VOID CGPS_Win32::SetupSimulate(DOUBLE CenterLat, DOUBLE CenterLon, UINT Radius)
{
    _simulateCenterLat = CenterLat;
    _simulateCenterLon = CenterLon;
    _simulateRadius = (Radius == (UINT)-1) ? _simulateRadius : Radius;
}

VOID SetupSat(FXGPS_SATELLITE *pSat, BOOL UsedInSolution, UINT16 PRN, UINT16 SignalQuality, UINT16 Azimuth, UINT16 Elevation)
{
    pSat->UsedInSolution = UsedInSolution;
    pSat->PRN = PRN;
    pSat->SignalQuality = SignalQuality;
    pSat->Azimuth = Azimuth;
    pSat->Elevation = Elevation;
}
#endif

VOID CGPS_Win32::GetState(FXGPS *pGPS)
{
    memset(pGPS, 0, sizeof(FXGPS));

#ifdef CLIENT_DLL
    if (_simulate)
    {
        pGPS->TimeStamp = FxGetTickCount();

        SYSTEMTIME localTime;
        GetLocalTime(&localTime);
        pGPS->DateTime.Year = localTime.wYear;
        pGPS->DateTime.Month = localTime.wMonth;
        pGPS->DateTime.Day = localTime.wDay;
        pGPS->DateTime.Hour = localTime.wHour;
        pGPS->DateTime.Minute = localTime.wMinute;
        pGPS->DateTime.Second = localTime.wSecond;
        pGPS->DateTime.Milliseconds = localTime.wMilliseconds;

        DOUBLE lat = _simulateCenterLat;
        DOUBLE lon = _simulateCenterLon;
        DOUBLE alt = 100;
        DOUBLE rad = _simulateRadius / 110895.9;

        DOUBLE i = (DOUBLE)pGPS->DateTime.Second * 6;

        lat += sin(DEG2RAD(i)) * rad;
        lon += cos(DEG2RAD(i)) * rad;
        alt += sin(DEG2RAD(i)) * 100 + 100;

        pGPS->Position.Accuracy = 10.0;
        pGPS->Position.Altitude = alt;
        pGPS->Position.Latitude = lat;
        pGPS->Position.Longitude = lon;
        pGPS->Position.Quality = fqSim3D;
        pGPS->Position.Speed = 0;

        if (_simulateLast.DateTime.Second == pGPS->DateTime.Second)
        {
            pGPS->Heading = _simulateLast.Heading;
            pGPS->Position.Speed = _simulateLast.Position.Speed;
        }
        else if (_simulateLast.State == dsConnected)
        {
            pGPS->Heading = CalcHeading(_simulateLast.Position.Latitude, _simulateLast.Position.Longitude,
                                        pGPS->Position.Latitude, pGPS->Position.Longitude);
            
            DOUBLE DeltaTime = 24 * (EncodeDateTime(&pGPS->DateTime) - EncodeDateTime(&_simulateLast.DateTime));
            DOUBLE DeltaDist = CalcDistanceKm(_simulateLast.Position.Latitude, _simulateLast.Position.Longitude,
                                              pGPS->Position.Latitude, pGPS->Position.Longitude);

            if (DeltaTime > 0)
            {
                pGPS->Position.Speed = DeltaDist / DeltaTime;
            }
        }

        SetupSat(&pGPS->Satellites[0], TRUE,   1, 50,  45,   5);
        SetupSat(&pGPS->Satellites[1], TRUE,   2, 30,  73,  45);
        SetupSat(&pGPS->Satellites[2], TRUE,  12, 25, 110,  75);
        SetupSat(&pGPS->Satellites[3], FALSE,  8, 15,  20,  15);
        SetupSat(&pGPS->Satellites[4], TRUE,   4, 40, 188,  35);
        SetupSat(&pGPS->Satellites[5], FALSE,  7,  5, 240,  65);
        SetupSat(&pGPS->Satellites[6], TRUE,   3, 35, 340,  38);
        SetupSat(&pGPS->Satellites[7], FALSE, 11, 10,  16,  25);

        pGPS->SatellitesInView = ARRAYSIZE(pGPS->Satellites);
        

        pGPS->State = dsConnected;
        _simulateLast = *pGPS;

        return;
    }
#endif

    g_gpsThread.GetState(pGPS);
}

VOID CGPS_Win32::SetState(BOOL TurnOn)
{
#ifdef CLIENT_DLL
    if (_simulate)
	{
		return;
	}
#endif

    g_gpsThread.SetState(TurnOn);
}

WCHAR *CGPS_Win32::GetDeviceName()
{
    if (g_gpsThread.Device != NULL)
    {
        return g_gpsThread.Device->GetDeviceName();
    }
    else
    {
        return NULL;
    }
}

//*************************************************************************************************
