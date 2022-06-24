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

#include "Win32_GetUrl.h"
#include "Win32_Utils.h"
#include "winnetwk.h"
#include "wininet.h"
#include "fxBrand.h"

#ifdef CLIENT_DLL
#include "Client_Host.h"
#else
#include "WinCE_Host.h"
#endif

CGetUrlManager *g_GetUrlManager;

//*************************************************************************************************
// CURLThread

struct GETURL_THREAD
{
    CRITICAL_SECTION Lock;
    HANDLE Handle;
    DWORD LastError;
    HANDLE Event;
    BOOL Terminated, Cancelled;
    CGetUrlManager *Manager;
    GETURL_ITEM Item;

    VOID Initialize(CGetUrlManager *Owner)
    {
        Terminated = Cancelled = FALSE;
        Manager = Owner;
        ::InitializeCriticalSection(&Lock);
        Event = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    }

    VOID Free()
    {
        Shutdown();

        ::CloseHandle(Handle);
        ::CloseHandle(Event);
        ::DeleteCriticalSection(&Lock);
        Handle = NULL;
    }

    BOOL Alive()
    {
        return !Terminated && !Cancelled;
    }

    VOID Unblock()
    {
        Cancelled = FALSE;
        ::SetEvent(Event);
        ResumeThread(Handle);
    }

    VOID Terminate()
    {
        Terminated = TRUE;
    }

    VOID Shutdown()
    {
        ::EnterCriticalSection(&Lock);
        Manager = NULL;
        ::LeaveCriticalSection(&Lock);
        Terminate();
        Unblock();
    }

    VOID Stop()
    {
        ::EnterCriticalSection(&Lock);
        Cancelled = TRUE;
        ::LeaveCriticalSection(&Lock);
    }

    BOOL GetHTTP(WCHAR *pUrl, WCHAR *pUserName, WCHAR *pPassword, BYTE *pData, UINT DataSize, CHAR *pFileName, UINT *pErrorCode)
    {
        BOOL success = FALSE;
        HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
        URL_COMPONENTSW urlComponents = {0};
        DWORD dwFlags = 0;
        CHAR *workBufferA1 = NULL, *workBufferA2 = NULL;
        WCHAR *workBufferU = NULL;
        CfxStream headerStream;
        BOOL useHTTPS = (_wcsnicmp(pUrl, L"https:", 6) == 0);
        DWORD statusCode;
        DWORD statusCodeLen = sizeof(statusCode);
        CfxFileStream fileStream;
        *pErrorCode = (UINT)-1;

        // Split the URL
        urlComponents.dwStructSize = sizeof(urlComponents);
        urlComponents.dwHostNameLength = 1;
        urlComponents.dwUrlPathLength = 1;

        if (!InternetCrackUrlW(pUrl, 0, 0, &urlComponents))
        {
            Log("InternetCrackUrl failed: %d", GetLastError());
            goto cleanup;
        }

        // Create a connection
        hSession = InternetOpenW(CLIENT_BRAND_PRODUCTW, 
                                 INTERNET_OPEN_TYPE_PROXY, 
                                 0, 0, 0);
        if (hSession == NULL) 
        {
            hSession = InternetOpenW(CLIENT_BRAND_PRODUCTW, 
                                     INTERNET_OPEN_TYPE_PRECONFIG, 
                                     0, 0, 0);
        }

        if (hSession == NULL)
        {
            Log("InternetOpen failed: %d", GetLastError());
            goto cleanup;
        }

        if (!Alive()) goto cleanup;

        if (urlComponents.lpszUrlPath && *urlComponents.lpszUrlPath)
        {
            *urlComponents.lpszUrlPath = '\0';
            urlComponents.lpszUrlPath++;
        }

        // Strip off extra port numbers.
        {
            WCHAR *p = wcschr(urlComponents.lpszHostName, L':');
            if (p)
            {
                *p = L'\0';
            }
        }

        if (urlComponents.nPort == 0)
        {
            urlComponents.nPort = useHTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
        }

        // Set the timeouts
        DWORD timeout;
        timeout = 60000;
        InternetSetOption(NULL, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        timeout = (DWORD)-1;
        InternetSetOption(NULL, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOption(NULL, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

        // Connect
        hConnect = InternetConnectW(hSession, 
                                    urlComponents.lpszHostName,
                                    urlComponents.nPort, 
                                    *pUserName ? pUserName : NULL, 
                                    *pPassword ? pPassword : NULL,
                                    INTERNET_SERVICE_HTTP,
                                    0, 
                                    0);
        if (hConnect == NULL)
        {
            Log("InternetConnect failed: %d", GetLastError());
            goto cleanup;
        }

        if (!Alive()) goto cleanup;

        if (useHTTPS)
        {
            dwFlags |= INTERNET_FLAG_SECURE;
        }

        dwFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
        dwFlags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP;
        dwFlags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS;
        dwFlags |= INTERNET_FLAG_KEEP_CONNECTION;
        dwFlags |= INTERNET_FLAG_NO_AUTO_REDIRECT;
        dwFlags |= INTERNET_FLAG_NO_COOKIES;
        dwFlags |= INTERNET_FLAG_NO_CACHE_WRITE;
        dwFlags |= INTERNET_FLAG_NO_UI;
        dwFlags |= INTERNET_FLAG_RELOAD;

        hRequest = HttpOpenRequestW(hConnect,
                                    L"POST", 
                                    urlComponents.lpszUrlPath,
                                    NULL, 
                                    NULL, 
                                    NULL,
                                    dwFlags,
                                    0);
        if (hRequest == NULL)
        {
            Log("Cannot create request: %d", GetLastError());
            goto cleanup;
        }

        // Allow invalid certs.
        {
            DWORD dwFlags; 
            DWORD dwBuffLen = sizeof(dwFlags); 
            InternetQueryOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID) &dwFlags, &dwBuffLen); 
            dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA; 
            InternetSetOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
        }

        if (*pUserName)
        {
            InternetSetOptionW(hRequest, INTERNET_OPTION_USERNAME, pUserName, wcslen(pUserName));
        }

        if (*pPassword)
        {
            InternetSetOptionW(hRequest, INTERNET_OPTION_PASSWORD, pPassword, wcslen(pPassword));
        }

        // Construct the headers
        headerStream.Write("Content-Type: application/json\r\n");

        if (*pUserName && *pPassword && useHTTPS)
        {
            headerStream.Write("Authorization: Basic ");

            workBufferA1 = (CHAR *)MEM_ALLOC(1024);
            sprintf(workBufferA1, "%S:%S", pUserName, pPassword);
            workBufferA2 = AsciiToBase64(workBufferA1);

            headerStream.Write(workBufferA2);
            headerStream.Write("\r\n");
        }

        CHAR b;
        b = L'\0';
        headerStream.Write(&b, 1);
        workBufferU = ToUnicodeFromAnsi((CHAR *)headerStream.GetMemory());

        if (!HttpSendRequestW(hRequest, workBufferU, wcslen(workBufferU), pData, DataSize))
        {
            Log("Failed to POST data: %d", GetLastError());
            goto cleanup;
        }
        
        if (!fileStream.OpenForWrite(pFileName))
        {
            Log("Failed to create file %s", pFileName);
            goto cleanup;
        }

        DWORD numRead;
        BYTE buffer[4096];
        while (InternetReadFile(hRequest, buffer, sizeof(buffer), &numRead))
        { 
            if (numRead == 0) 
            { 
                break; 
            }

            if (!fileStream.Write(buffer, numRead))
            {
                Log("Failed to write to file");
                goto cleanup;
            }
        }

        if (HttpQueryInfoW(hRequest, 
                           HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
                           &statusCode, 
                           &statusCodeLen, 
                           NULL))
        {
            *pErrorCode = statusCode;
        }
        else
        {
            Log("HttpQueryInfoW failed: %d", GetLastError());
            goto cleanup;
        }

        Log("HTTP: Success (%d)", statusCode);

        success = TRUE;

    cleanup:

        if (hRequest != NULL)
        {
            InternetCloseHandle(hRequest);
            hRequest = NULL;
        }

        if (hConnect != NULL)
        {
            InternetCloseHandle(hConnect);
            hConnect = NULL;
        }

        if (hSession != NULL)
        {
            InternetCloseHandle(hSession);
            hSession = NULL;
        }

        if (workBufferA1 != NULL)
        {
            MEM_FREE(workBufferA1);
            workBufferA1 = NULL;
        }

        if (workBufferA2 != NULL)
        {
            MEM_FREE(workBufferA1);
            workBufferA2 = NULL;
        }

        if (workBufferU != NULL)
        {
            MEM_FREE(workBufferU);
            workBufferU = NULL;
        }

        fileStream.Close();

        return success;
    }

    static DWORD ThreadProc(GETURL_THREAD *pGetUrlThread)
    {
        BOOL success = FALSE;
        WCHAR *url = NULL, *userName = NULL, *password = NULL;
        WCHAR *sourceFileName = NULL;
        UINT errorCode = 0;
        CfxString data;

        while (!pGetUrlThread->Terminated)
        {
            GETURL_ITEM currentItem = {0};
            if (WaitForSingleObject(pGetUrlThread->Event, INFINITE) != WAIT_OBJECT_0)
            {
                break;
            }
            if (!pGetUrlThread->Alive())
            {
                break;
            }

            if (!pGetUrlThread->Manager->UrlRequested(&currentItem))
            {
                ResetEvent(pGetUrlThread->Event);
                continue;
            }

            SetEvent(pGetUrlThread->Event);
            success = FALSE;

            // Get local variables
            url = ToUnicodeFromAnsi(currentItem.Url);
            userName = ToUnicodeFromAnsi(currentItem.UserName);
            password = ToUnicodeFromAnsi(currentItem.Password);
            if (pGetUrlThread->GetHTTP(url, userName, password, currentItem.Data, currentItem.DataSize, currentItem.FileName, &currentItem.ErrorCode))
            {
                pGetUrlThread->Manager->UrlCompleted(&currentItem);
            }

            MEM_FREE(currentItem.Data);
            MEM_FREE(url);
            url = NULL;
            MEM_FREE(userName);
            userName = NULL;
            MEM_FREE(password);
            password = NULL;
            pGetUrlThread->Cancelled = FALSE;
        }

        pGetUrlThread->Free();
        return 0;
    }
};

//*************************************************************************************************

static GETURL_THREAD g_GetUrlThread = {0};

CGetUrlManager::CGetUrlManager(): _pending()
{
    ::InitializeCriticalSection(&_lock);
    g_GetUrlThread.Initialize(this);
    g_GetUrlThread.Handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)g_GetUrlThread.ThreadProc, &g_GetUrlThread, CREATE_SUSPENDED, NULL);
    _idMax = 1;
}

CGetUrlManager::~CGetUrlManager()
{
    if (g_GetUrlThread.Handle != NULL)
    {
        g_GetUrlThread.Terminate();
    }

    DeleteCriticalSection(&_lock);
}

BOOL CGetUrlManager::UrlRequested(GETURL_ITEM *pItem)
{
    BOOL result = FALSE;

    ::EnterCriticalSection(&_lock);
    if (_pending.GetCount() > 0)
    {
        *pItem = _pending.Get(0);
        _pending.Delete(0);
        result = TRUE;
    }
    ::LeaveCriticalSection(&_lock);

    return result;
}

VOID CGetUrlManager::UrlCompleted(GETURL_ITEM *pItem)
{
    SendMessageW(pItem->hWnd, WM_URL_COMPLETED, 0, (LPARAM)pItem);
}

VOID CGetUrlManager::ClearPending()
{
    ::EnterCriticalSection(&_lock);
    for (UINT i = 0; i < _pending.GetCount(); i++)
    {
        MEM_FREE(_pending.GetPtr(i)->Data);
    }
    _pending.Clear();
    ::LeaveCriticalSection(&_lock);
}

BOOL CGetUrlManager::GetFile(HWND hWnd, CHAR *pUrl, CHAR *pUserName, CHAR *pPassword, CfxStream &Data, CHAR *pFileName, UINT *pId)
{
    *pId = 0;
    if (g_GetUrlThread.Handle == NULL) return FALSE;

    ::EnterCriticalSection(&_lock);
    GETURL_ITEM item = {0};
    item.hWnd = hWnd;
    item.Id = _idMax++;
    item.Data = (BYTE *)Data.CloneMemory();
    item.DataSize = Data.GetSize() - 1; // Strip off the trailing null.

    *pId = item.Id;
    strcpy(item.Url, pUrl);
    strcpy(item.UserName, pUserName);
    strcpy(item.Password, pPassword);
    sprintf(item.FileName, "%s-%u.tmp", pFileName, *pId);
   
    _pending.Add(item);
    ::LeaveCriticalSection(&_lock);
   
    g_GetUrlThread.Unblock();

    return TRUE;
}

VOID CGetUrlManager::CancelPending(UINT Id)
{
    UINT i;

    ::EnterCriticalSection(&_lock);
    for (i = 0; i < _pending.GetCount(); i++)
    {
        if (_pending.GetPtr(i)->Id == Id)
        {
            MEM_FREE(_pending.GetPtr(i)->Data);
            _pending.Delete(i);
            break;
        }
    }
    ::LeaveCriticalSection(&_lock);
}

VOID CGetUrlManager::Stop()
{
    if (g_GetUrlThread.Handle == NULL) return;

    ClearPending();
    g_GetUrlThread.Stop();
}

VOID CGetUrlManager::Terminate()
{
    if (g_GetUrlThread.Handle == NULL) return;

    ClearPending();
    g_GetUrlThread.Terminate();
    DeleteCriticalSection(&_lock);
}

BOOL CGetUrlManager::IsIdle()
{
    return _pending.GetCount() == 0;
}
