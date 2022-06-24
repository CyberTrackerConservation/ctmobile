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

#include "Win32_SerialPorts.h"
#include "fxUtils.h"

SerialDataCallback g_DataCallback;
PVOID g_DataCallbackContext;

struct SERIAL_PORT_STATE
{
    BYTE PortId;
    CRITICAL_SECTION Lock;
    FX_COMPORT ComPort;
    BOOL NextState;
    BOOL Terminated;
    BOOL Connected;
    BOOL ConnectPending;
    UINT Uniqueness;
    HANDLE Thread;
    HANDLE Event;
    BYTE *WriteData;
    UINT WriteLength;
};

CSerialPorts *g_SerialPorts;
SERIAL_PORT_STATE g_SerialThreads[21];
BOOL g_SerialThreadsInitialized = FALSE;

DWORD SerialThreadProc(SERIAL_PORT_STATE *State)
{
    CSerialIO serialIO;
    UINT uniqueness;

    WCHAR comName[16];
    #if defined(_WIN32_WCE)
        wsprintfW(comName, L"COM%d:", State->PortId);
    #else
        wsprintfW(comName, L"\\\\.\\COM%d", State->PortId);
    #endif

    // Enable the loop
    ::SetEvent(State->Event);

    uniqueness = State->Uniqueness;

    // Read from the port while it is active
    while (::WaitForSingleObject(State->Event, INFINITE) == WAIT_OBJECT_0)
    {
        if (State->Terminated) break;

        if (uniqueness != State->Uniqueness)
        {
            serialIO.Disconnect();
            uniqueness = State->Uniqueness;
        }

        switch (State->NextState)
        {
        case TRUE:
            if (!serialIO.GetConnected())
            {
                serialIO.Connect(comName, &State->ComPort, TRUE, FALSE);
            }
            break;

        case FALSE:
            serialIO.Disconnect();
            break;
        }

        State->Connected = serialIO.GetConnected();
        State->ConnectPending = FALSE;

        if (State->Connected)
        {
            // Writes
            if (State->WriteData != NULL)
            {
                ::EnterCriticalSection(&State->Lock);
                if (State->WriteData != NULL)
                {
                    serialIO.Write(State->WriteData, State->WriteLength);
                    MEM_FREE(State->WriteData);
                    State->WriteData = NULL;
                    State->WriteLength = 0;
                }
                ::LeaveCriticalSection(&State->Lock);
            }

            // Reads                    
            BYTE buffer[4096];
            UINT bytesRead = serialIO.Read(&buffer[0], sizeof(buffer));
            if (bytesRead > 0)
            {
                FX_COMPORT_DATA Message;
                Message.PortId = State->PortId;
                Message.Data = &buffer[0];
                Message.Length = bytesRead;
                g_SerialPorts->ExecuteCallbacks(&Message);
                Sleep(50);
            }
            else
            {
                Sleep(50);
            }

            ::SetEvent(State->Event);
        }
        else
        {
            ::ResetEvent(State->Event);
        }
    }

    // Cleanup the thread state
    ::EnterCriticalSection(&State->Lock);
    
    ::CloseHandle(State->Event);
    State->Event = INVALID_HANDLE_VALUE;

    ::CloseHandle(State->Thread);
    State->Thread = INVALID_HANDLE_VALUE;

    MEM_FREE(State->WriteData);
    State->WriteData = NULL;
    State->WriteLength = 0;

    ::LeaveCriticalSection(&State->Lock);

    return 1;
}

CSerialPorts::CSerialPorts(): _callbacks()
{
    InitializeCriticalSection(&_callbackLock);

    UINT i;
    for (i = 0; i < ARRAYSIZE(g_SerialThreads); i++)
    {
        SERIAL_PORT_STATE *state = &g_SerialThreads[i];

        ::InitializeCriticalSection(&state->Lock);
        state->PortId = (BYTE)i;
        state->Terminated = FALSE;
        state->NextState = FALSE;
        state->Event = INVALID_HANDLE_VALUE;
        state->Thread = INVALID_HANDLE_VALUE;
        state->WriteData = 0;
        state->WriteLength = 0;
        state->Uniqueness = 1;
    }

    g_SerialThreadsInitialized = TRUE;
}

CSerialPorts::~CSerialPorts()
{
    g_SerialThreadsInitialized = FALSE;

    UINT i;
    for (i = 0; i < ARRAYSIZE(g_SerialThreads); i++)
    {
        if (g_SerialThreads[i].Thread != INVALID_HANDLE_VALUE)
        {
            SERIAL_PORT_STATE *state = &g_SerialThreads[i];
            
            // Thread will do the cleanup
            state->Terminated = TRUE;
            ::ResetEvent(state->Event);
        }
    }

    ::DeleteCriticalSection(&_callbackLock);
}

VOID CSerialPorts::Terminate()
{
    Disconnect();
    g_SerialThreadsInitialized = FALSE;
}

VOID CSerialPorts::Disconnect()
{
    UINT i;
    for (i = 0; i < ARRAYSIZE(g_SerialThreads); i++)
    {
        DisconnectPort((BYTE)i);
    }
}

VOID CSerialPorts::AddCallback(SerialDataCallback Callback, PVOID pContext)
{
    SERIAL_CALLBACK item;
    item.Callback = Callback;
    item.Context = pContext;
    
    EnterCriticalSection(&_callbackLock);
    _callbacks.Add(item);
    LeaveCriticalSection(&_callbackLock);
}

VOID CSerialPorts::RemoveCallback(SerialDataCallback Callback, PVOID pContext)
{
    SERIAL_CALLBACK item;
    item.Callback = Callback;
    item.Context = pContext;

    EnterCriticalSection(&_callbackLock);
    _callbacks.Remove(item);
    LeaveCriticalSection(&_callbackLock);
}

VOID CSerialPorts::ExecuteCallbacks(FX_COMPORT_DATA *pComPortData)
{
    if (!g_SerialThreadsInitialized) return;

    EnterCriticalSection(&_callbackLock);

    UINT i;
    for (i = 0; i < _callbacks.GetCount(); i++)
    {
        SERIAL_CALLBACK *item;
        item = _callbacks.GetPtr(i);
        item->Callback(pComPortData, item->Context);
    }

    LeaveCriticalSection(&_callbackLock);
}

BOOL CSerialPorts::ConnectPort(BYTE PortId, FX_COMPORT *pComPort)
{
    FX_ASSERT(PortId < ARRAYSIZE(g_SerialThreads));

    SERIAL_PORT_STATE *state = &g_SerialThreads[PortId];

    // Check if the port is already connected with the same parameters
    if (state->NextState &&
        (memcmp(pComPort, &state->ComPort, sizeof(FX_COMPORT)) == 0))
    {
        return TRUE;
    }

    // Disconnect the port
    DisconnectPort(PortId);

    // Spin up a new thread for this port
    EnterCriticalSection(&state->Lock);
    state->ComPort = *pComPort;
    state->Terminated = FALSE;
    state->NextState = TRUE;
    state->ConnectPending = TRUE;
    state->Uniqueness++;
    if (state->Event == INVALID_HANDLE_VALUE)
    {
        state->Event = ::CreateEvent(NULL, FALSE, TRUE, NULL);
    }
    ::SetEvent(g_SerialThreads[PortId].Event);

    if (state->Thread == INVALID_HANDLE_VALUE)
    {
        state->Thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SerialThreadProc, (PVOID)state, 0, NULL);
    }
    LeaveCriticalSection(&state->Lock);

    return TRUE;
}

VOID CSerialPorts::DisconnectPort(BYTE PortId)
{
    EnterCriticalSection(&g_SerialThreads[PortId].Lock);
    if (g_SerialThreads[PortId].NextState)
    {
        g_SerialThreads[PortId].Uniqueness++;
        g_SerialThreads[PortId].NextState = FALSE;
        g_SerialThreads[PortId].ConnectPending = FALSE;
        ::SetEvent(g_SerialThreads[PortId].Event);
    }
    LeaveCriticalSection(&g_SerialThreads[PortId].Lock);
}

VOID CSerialPorts::WritePortData(BYTE PortId, BYTE *pData, UINT Length)
{
    FX_ASSERT(PortId < ARRAYSIZE(g_SerialThreads));
    FX_ASSERT(Length > 0);

    EnterCriticalSection(&g_SerialThreads[PortId].Lock);
    
    if (g_SerialThreads[PortId].WriteData != NULL)
    {
        MEM_FREE(g_SerialThreads[PortId].WriteData);
        g_SerialThreads[PortId].WriteData = NULL;
        g_SerialThreads[PortId].WriteLength = 0;
    }

    if (g_SerialThreads[PortId].NextState)
    {
        g_SerialThreads[PortId].WriteData = (BYTE *)MEM_ALLOC(Length);
        memcpy(g_SerialThreads[PortId].WriteData, pData, Length);
        g_SerialThreads[PortId].WriteLength = Length;

        ::SetEvent(g_SerialThreads[PortId].Event);
    }

    LeaveCriticalSection(&g_SerialThreads[PortId].Lock);
}

BOOL CSerialPorts::IsConnected(BYTE PortId)
{
    FX_ASSERT(PortId < ARRAYSIZE(g_SerialThreads));

    return g_SerialThreads[PortId].Connected ||
           g_SerialThreads[PortId].ConnectPending;
}