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

#include "Win32_RangeFinder.h"
#include "SerialRangeFinder.h"

CRangeFinder_Win32 *g_RangeFinder;

//*************************************************************************************************

// Remove a specified registry value
VOID RemoveRegistryValue(HKEY RootKey, WCHAR *lpKeyName, WCHAR *lpValueName)
{
    HKEY key;
    if (ERROR_SUCCESS == ::RegOpenKeyExW(RootKey, lpKeyName, 0, MAXIMUM_ALLOWED, &key))
    {
        ::RegDeleteValueW(key, lpValueName);
    }
}

//*************************************************************************************************
// CRangeFinder_Win32

struct CRangeFinderThread
{
    HANDLE OnOffEvent;
    BOOL Terminate;
    CRITICAL_SECTION ThreadLock, StateLock;
    CfxRangeFinder *Device;
    FXRANGE Range;
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

        #if defined(_WIN32_WCE)
            // 
            // Garmin Bluetooth does not work by default without removing these registry value.
            // This fix is from Garmin Technical support.
            //
            HMODULE garminLib = ::LoadLibraryW(L"QUEAPI.DLL");
            if (garminLib)
            {
                ::FreeLibrary(garminLib);

                RemoveRegistryValue(HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\SS1VCOMM5", L"EventDLL");
                RemoveRegistryValue(HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\SS1VCOMM7", L"EventDLL");
                RemoveRegistryValue(HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\SS1VCOMM8", L"EventDLL");
            }
        #endif


        // Detect range finders
        Device = new CSerialRangeFinder(PortOverride, ParamsOverride);
        if (!Device->IsPresent())
        {
            delete Device;
            Device = NULL;
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

    VOID GetState(FXRANGE *pRange)
    {
        ::EnterCriticalSection(&StateLock);

        // Validate the state against a pending change
        if (PendingTurnOn)
        {
            if ((Range.State == dsNotFound) || (Range.State == dsDisconnected))
            {
                Range.State = dsDetecting;
            }
        }
        else
        {
            FX_ASSERT(!PendingTurnOn);
            Range.State = dsDisconnected;
        }

        // Populate the Range state
        memcpy(pRange, &Range, sizeof(FXRANGE));

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

CRangeFinderThread g_rangeFinderThread;

DWORD RangeFinderThreadProc(CRangeFinder_Win32 *pRange)
{
    CRangeFinderThread *threadData = &g_rangeFinderThread;

    while ((WaitForSingleObject(threadData->OnOffEvent, INFINITE) == WAIT_OBJECT_0) && !threadData->Terminate)
    {
        ::EnterCriticalSection(&threadData->ThreadLock);

        if (threadData->Device)
        {
            // Snap the state and get pendingTurnOn
            ::EnterCriticalSection(&threadData->StateLock);
            threadData->Device->GetState(&threadData->Range);
            FXDEVICE_STATE currentState = threadData->Range.State; 
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
            else
            {
                // Turn off pendingTurnOn: it didn't work
                ::EnterCriticalSection(&threadData->StateLock);
                threadData->PendingTurnOn = FALSE;
                ::LeaveCriticalSection(&threadData->StateLock);

                // If the device can't iterate, stop the thread
                ::ResetEvent(threadData->OnOffEvent);
            }
        }

        ::LeaveCriticalSection(&threadData->ThreadLock);
    }

    threadData->Destroy();
    delete pRange;

    return 1;
}

//*************************************************************************************************
// CRangeFinder_Win32

CRangeFinder_Win32::CRangeFinder_Win32()
{
    memset(&g_rangeFinderThread, 0, sizeof(g_rangeFinderThread));
    g_rangeFinderThread.Create();
    g_rangeFinderThread.Detect(0, 0);

    _thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RangeFinderThreadProc, NULL, CREATE_SUSPENDED, NULL);
    SetThreadPriority(_thread, THREAD_PRIORITY_BELOW_NORMAL);
    ResumeThread(_thread);
}

CRangeFinder_Win32::~CRangeFinder_Win32()
{
    if (_thread != NULL)
    {
        CloseHandle(_thread);
    }
}

VOID CRangeFinder_Win32::Terminate()
{
    g_rangeFinderThread.Terminate = TRUE;
    ::SetEvent(g_rangeFinderThread.OnOffEvent);

    UINT exitCode;
    if (GetExitCodeThread(_thread, &exitCode))
    {
        if ((exitCode != 1) && (exitCode != STILL_ACTIVE))
        {
            g_rangeFinderThread.Destroy();
            delete this;
        }
    }
}

BOOL CRangeFinder_Win32::Detect(BYTE PortOverride, BYTE ParamsOverride)
{
    if (PortOverride != 0)
    {
        g_rangeFinderThread.Detect(PortOverride, ParamsOverride);
    }

    return g_rangeFinderThread.Device != NULL;
}

VOID CRangeFinder_Win32::ResetTimeouts()
{
    g_rangeFinderThread.ResetTimeouts();
}

VOID CRangeFinder_Win32::GetState(FXRANGE *pRange)
{
    memset(pRange, 0, sizeof(FXRANGE));
    g_rangeFinderThread.GetState(pRange);
}

VOID CRangeFinder_Win32::SetState(BOOL TurnOn)
{
    g_rangeFinderThread.SetState(TurnOn);
}

//*************************************************************************************************
