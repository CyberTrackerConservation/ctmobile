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

#include "Win32_Transfer.h"
#include "Win32_Utils.h"
#include "winnetwk.h"
#include "wininet.h"
#include "fxBrand.h"

#ifdef CLIENT_DLL
#include "Client_Host.h"
#else
#include "WinCE_Host.h"
#endif

CTransferManager *g_TransferManager;

typedef DWORD (__stdcall * PFN_WNetAddConnection3W)(HWND, LPNETRESOURCEW, LPCWSTR, LPCWSTR, DWORD);
PFN_WNetAddConnection3W g_WNetAddConnection3W;

//*************************************************************************************************
// CURLThread

struct URL_THREAD
{
    CRITICAL_SECTION Lock;
    HANDLE Handle;
    DWORD LastError;
    HANDLE Event;
    HANDLE Mutex;
    BOOL Terminated, Cancelled;
    CTransferManager *Manager;
    TRANSFER_ITEM Item;
    FXSENDSTATE State;
    WCHAR *OutgoingMaxPath;

    VOID Initialize(CTransferManager *Owner)
    {
        Terminated = Cancelled = FALSE;
        Manager = Owner;
        OutgoingMaxPath = NULL;
        ::InitializeCriticalSection(&Lock);
        Event = ::CreateEventW(NULL, FALSE, FALSE, NULL);

        Mutex = ::CreateMutexW(NULL, FALSE, CLIENT_UPLOAD_MUTEX_NAMEW);
        if (Mutex == NULL)
        {
            Log("Failed to acquire transfer mutex");
        }
    }

    VOID Free()
    {
        Shutdown();

        ::CloseHandle(Handle);
        ::ReleaseMutex(Mutex);
        ::CloseHandle(Mutex);
        ::CloseHandle(Event);
        ::DeleteCriticalSection(&Lock);
        Handle = NULL;
    }

    BOOL Alive()
    {
        return !Terminated && !Cancelled;
    }

    VOID FileRejected()
    {
        ::EnterCriticalSection(&Lock);
        if (Alive() && Manager)
        {
            Manager->FileRejected(&Item);
        }
        ::LeaveCriticalSection(&Lock);
    }

    VOID FileCompleted()
    {
        ::EnterCriticalSection(&Lock);
        if (Alive() && Manager)
        {
            Manager->FileCompleted(&Item);
        }
        ::LeaveCriticalSection(&Lock);
    }

    VOID FileRequest()
    {
        memset(&Item.FileName, 0, sizeof(Item.FileName));
        ::EnterCriticalSection(&Lock);
        if (Alive() && Manager)
        {
            Manager->FileRequested(&Item);
        }
        ::LeaveCriticalSection(&Lock);
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
        SetState(SS_STOPPED, "Stopped");
        ::LeaveCriticalSection(&Lock);
    }

    VOID GetState(FXSENDSTATE *pState)
    {
        ::EnterCriticalSection(&Lock);
        memcpy(pState, &State, sizeof(FXSENDSTATE));
        ::LeaveCriticalSection(&Lock);
    }

    VOID SetState(FXSENDSTATEMODE Mode, CHAR *pLastError = "")
    {
        ::EnterCriticalSection(&Lock);
        State.Mode = Mode;
        State.Id = Item.Id;
        if (strlen(pLastError) > 0)
        {
            strcpy(State.LastError, pLastError);
        }

        if (Item.hWnd != NULL)
        {
            PostMessageW(Item.hWnd, WM_TRANSFER_CHANGE, State.Mode, NULL);
        }

        ::LeaveCriticalSection(&Lock);
    }

    BOOL AcquireLock()
    {
        return (Mutex != NULL) && (::WaitForSingleObject(Mutex, 5000) == WAIT_OBJECT_0);
    }

    VOID ReleaseLock()
    {
        ::ReleaseMutex(Mutex);
    }

    BOOL SendUNC(WCHAR *pUrl, WCHAR *pUserName, WCHAR *pPassword, WCHAR *pSourceFileName)
    {
        BOOL success = FALSE;
        WCHAR *targetFileName = NULL;
        WCHAR *fileName = NULL;
        NETRESOURCEW nr = {0};
        DWORD errorCode;
        nr.dwType = RESOURCETYPE_DISK;
        nr.lpRemoteName = pUrl;

        // Get buffer for target filename
        targetFileName = AllocMaxPathW(NULL);
        if (targetFileName == NULL)
        {
            Log("Out of memory");
            SetState(SS_FAILED_SEND, "Out of memory");
            goto cleanup;
        }

        // Get the filename
        fileName = wcsrchr(pSourceFileName, L'\\');
        if (fileName == NULL)
        {
            fileName = pSourceFileName;
        }
        else
        {
            fileName++;
        }

        // Just try to copy the file first
        wcscpy(targetFileName, pUrl);
        AppendSlashW(targetFileName);
        wcscat(targetFileName, fileName);
        success = CopyFileW(pSourceFileName, targetFileName, FALSE);
        
        // Done if succeeded
        if (success)
        {
            goto cleanup;
        }

        // Done if no way to get a connection
        if (g_WNetAddConnection3W == NULL)
        {
            Log("WNetAddConnection3W API not found");
            SetState(SS_FAILED_SEND, "API not found");
            goto cleanup;
        }

        // Try connecting
        if (*pUserName == L'\0') 
        {
            if (*pPassword == L'\0')
            {
                errorCode = (*g_WNetAddConnection3W)(NULL, &nr, NULL, NULL, 0); 
            }
            else 
            {
                errorCode = (*g_WNetAddConnection3W)(NULL, &nr, pPassword, NULL, 0); 
            }
        }
        else 
        {
            errorCode = (*g_WNetAddConnection3W)(NULL, &nr, pPassword, pUserName, 0); 
        }

        if (errorCode != 0)
        {
            Log("Cannot log in: %d", errorCode);
            SetState(SS_FAILED_SEND, "Cannot log in");
            goto cleanup;
        }

        success = CopyFileW(pSourceFileName, targetFileName, FALSE);

    cleanup:

        MEM_FREE(targetFileName);

        return success;
    }

    BOOL SendFTP(WCHAR *pUrl, WCHAR *pUserName, WCHAR *pPassword, WCHAR *pSourceFileName)
    {
        HINTERNET hSession, hConnect, hRequest;
        URL_COMPONENTSW urlComponents;
        WCHAR *targetFileName;
        UINT sourceFileSize;
        CfxStream stream;
        HANDLE hFile;
        UINT bytesRead, bytesWritten;
        BOOL success;
        BOOL directoryOk;

        success = FALSE;
        targetFileName = NULL;
        hSession = hConnect = hRequest = NULL;
        hFile = INVALID_HANDLE_VALUE;
        directoryOk = FALSE;

        // Split the URL
        memset(&urlComponents, 0, sizeof(urlComponents));
        urlComponents.dwStructSize = sizeof(urlComponents);
        urlComponents.dwHostNameLength = 1;
        urlComponents.dwUrlPathLength = 1;

        Log("FTP: %S -> %S", pUrl, pSourceFileName);

        if (!InternetCrackUrlW(pUrl, 0, 0, &urlComponents))
        {
            Log("InternetCrackUrl failed: %d", GetLastError());
            SetState(SS_FAILED_CONNECT, "Bad URL");
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
            SetState(SS_FAILED_CONNECT, "No internet connection");
            goto cleanup;
        }

        if (!Alive()) goto cleanup;

        if (urlComponents.lpszUrlPath && *urlComponents.lpszUrlPath)
        {
            *urlComponents.lpszUrlPath = '\0';
            urlComponents.lpszUrlPath++;
        }

        hConnect = InternetConnectW(hSession, 
                                    urlComponents.lpszHostName,
                                    urlComponents.nPort,
                                    *pUserName ? pUserName : NULL, 
                                    *pPassword ? pPassword : NULL,
                                    INTERNET_SERVICE_FTP, 
                                    INTERNET_FLAG_PASSIVE, 
                                    0);
        if (hConnect == NULL)
        {
            Log("InternetConnect failed: %d", GetLastError());
            SetState(SS_FAILED_CONNECT, "Login failed");
            goto cleanup;
        }

        if (!Alive()) goto cleanup;

        SetState(SS_SENDING);

        directoryOk = FtpSetCurrentDirectoryW(hConnect, urlComponents.lpszUrlPath);
        if (!directoryOk)
        {
            targetFileName = AllocMaxPathW(NULL);
            if (targetFileName == NULL)
            {
                Log("Out of memory");
                SetState(SS_FAILED_SEND, "Out of memory");
                goto cleanup;
            }

            wcscpy(targetFileName, L"/");
            wcscat(targetFileName, urlComponents.lpszUrlPath);
            directoryOk = FtpSetCurrentDirectoryW(hConnect, targetFileName);
        }

        if (directoryOk)
        {
            hRequest = FtpOpenFileW(hConnect,
                                    Item.FileName,
                                    GENERIC_WRITE,
                                    FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_TRANSFER_BINARY,
                                    0);
        }

        if (hRequest == NULL)
        {
            Log("Cannot create request: %d", GetLastError());
            SetState(SS_FAILED_SEND, "Could not create file on host");
            goto cleanup;
        }

        if (!Alive()) goto cleanup;

        // Get source file size
        sourceFileSize = GetFileSizeW(pSourceFileName);
        if (sourceFileSize == 0)
        {
            Log("Bad file size");
            SetState(SS_FAILED_SEND, "Bad file size");
            goto cleanup;
        }

        // Open the source file
        hFile = CreateFileW(pSourceFileName,
                            GENERIC_READ, 
                            0, 
                            NULL, 
                            OPEN_EXISTING, 
                            FILE_ATTRIBUTE_NORMAL, 
                            NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            Log("Failed to open source file: %d", GetLastError());
            SetState(SS_FAILED_SEND, "Failed to open source file");
            goto cleanup;
        }

        stream.SetCapacity(65536);
        bytesRead = bytesWritten = 0;
        while (sourceFileSize > 0)
        {
            UINT bytesToRead = min(sourceFileSize, stream.GetCapacity());

            if (!ReadFile(hFile, stream.GetMemory(), bytesToRead, &bytesRead, NULL) ||
                (bytesRead != bytesToRead))
            {
                Log("Failed to read source file: %d", GetLastError());
                SetState(SS_FAILED_SEND, "Failed to read source file");
                goto cleanup;
            }

            if (!InternetWriteFile(hRequest, stream.GetMemory(), bytesRead, &bytesWritten) ||
                (bytesRead != bytesWritten))
            {
                Log("Failed to write to host: %d", GetLastError());
                SetState(SS_FAILED_SEND, "Failed to write to host");
                goto cleanup;
            }

            sourceFileSize -= bytesRead;
        }

        SetState(SS_COMPLETED, "Success");

        Log("FTP: Success");

        success = TRUE;

    cleanup:

        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
        }

        if (targetFileName)
        {
            MEM_FREE(targetFileName);
            targetFileName = NULL;
        }

        if (hRequest) 
        {
            InternetCloseHandle(hRequest);
            hRequest = NULL;
        }

        if (hConnect) 
        {
            InternetCloseHandle(hConnect);
            hConnect = NULL;
        }

        if (hSession) 
        {
            InternetCloseHandle(hSession);
            hSession = NULL;
        }

        return success;
    }

    BOOL SendHTTP(WCHAR *pUrl, WCHAR *pUserName, WCHAR *pPassword, WCHAR *pSourceFileName, BOOL Compressed)
    {
        BOOL success = FALSE;
        HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
        URL_COMPONENTSW urlComponents = {0};
        DWORD dwFlags = 0;
        FXFILEMAP fileMap = {0};
        CHAR *sourceFileNameA = NULL;
        CHAR *workBufferA1 = NULL, *workBufferA2 = NULL;
        WCHAR *workBufferU = NULL;
        CfxStream headerStream;
        BOOL useHTTPS = (_wcsnicmp(pUrl, L"https:", 6) == 0);
        DWORD statusCode;
        DWORD statusCodeLen = sizeof(statusCode);

        Log("HTTP: %S -> %S", pUrl, pSourceFileName);

        // Split the URL
        urlComponents.dwStructSize = sizeof(urlComponents);
        urlComponents.dwHostNameLength = 1;
        urlComponents.dwUrlPathLength = 1;

        if (!InternetCrackUrlW(pUrl, 0, 0, &urlComponents))
        {
            Log("InternetCrackUrl failed: %d", GetLastError());
            SetState(SS_FAILED_CONNECT, "Bad URL");
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
            SetState(SS_FAILED_CONNECT, "No internet connection");
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
            SetState(SS_FAILED_CONNECT, "Login failed");
            goto cleanup;
        }

        if (!Alive()) goto cleanup;

        SetState(SS_SENDING);

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
            SetState(SS_FAILED_SEND, "Could not connect");
            goto cleanup;
        }

        // Allow invalid certs.
        {
            DWORD dwFlags; 
            DWORD dwBuffLen = sizeof(dwFlags); 
            InternetQueryOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID) &dwFlags, &dwBuffLen); 
            dwFlags |=  SECURITY_FLAG_IGNORE_UNKNOWN_CA; 
            InternetSetOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof (dwFlags));
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

        if (Compressed)
        {
            headerStream.Write("Content-Encoding: deflate\r\n");
        }

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

        sourceFileNameA = ToAnsi(pSourceFileName);
        if (sourceFileNameA == NULL)
        {
            Log("Out of memory");
            SetState(SS_FAILED_SEND, "Out of memory");
            goto cleanup;
        }

        if (!FxMapFile(sourceFileNameA, &fileMap))
        {
            Log("Failed to read file: %d", GetLastError());
            SetState(SS_FAILED_SEND, "Could not read data file");
            goto cleanup;
        }

        if (!HttpSendRequestW(hRequest, workBufferU, wcslen(workBufferU), fileMap.BasePointer, fileMap.FileSize))
        {
            Log("Failed to POST data: %d", GetLastError());
            SetState(SS_FAILED_SEND, "Could not send data");
            goto cleanup;
        }
            
        if (HttpQueryInfoW(hRequest, 
                           HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
                           &statusCode, 
                           &statusCodeLen, 
                           NULL))
        {
            LONG errorCode = statusCode;
            if ((errorCode != 200) && (errorCode != 201) && (errorCode != 202))
            {
                Log("HttpQueryInfo returned: %d", errorCode);
                SetState(SS_FAILED_SEND, "Upload failed");
                goto cleanup;
            }
        }
        else
        {
            Log("HttpQueryInfoW failed: %d", GetLastError());
            SetState(SS_FAILED_SEND, "Failed to complete write");
            goto cleanup;
        }

        Log("HTTP: Success (%d)", statusCode);

        success = TRUE;

    cleanup:

        FxUnmapFile(&fileMap);
        memset(&fileMap, 0, sizeof(fileMap));

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

        if (sourceFileNameA != NULL)
        {
            MEM_FREE(sourceFileNameA);
            sourceFileNameA = NULL;
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
                
        return success;
    }

    BOOL SendESRI(WCHAR *pUrl, WCHAR *pUserName, WCHAR *pPassword, WCHAR *pSourceFileName)
    {
        #ifdef CLIENT_DLL
            typedef BOOL (APIENTRY *FN_Server_SendESRI)(CHAR *pUrl, CHAR *pUserName, CHAR *pPassword, CHAR *pFileName);
            static FN_Server_SendESRI _fn_Server_SendESRI;

            if (_fn_Server_SendESRI == NULL)
            {
                ::HMODULE handle = ::GetModuleHandle(NULL); 
                _fn_Server_SendESRI = (FN_Server_SendESRI) ::GetProcAddress(handle, "Server_SendESRI");
            }
            FX_ASSERT(_fn_Server_SendESRI != NULL);

            BOOL result = FALSE;
            CHAR *url = ToAnsi(pUrl);
            CHAR *userName = ToAnsi(pUserName);
            CHAR *password = ToAnsi(pPassword);
            CHAR *fileName = ToAnsi(pSourceFileName);

            result = _fn_Server_SendESRI(url, userName, password, fileName);

            MEM_FREE(url);
            MEM_FREE(userName);
            MEM_FREE(password);
            MEM_FREE(fileName);

            if (!result)
            {
                Log("Failed to send data to ESRI");
            }

            return result;
        #else
            return FALSE;
        #endif
    }

    static DWORD ThreadProc(URL_THREAD *pUrlThread)
    {
        BOOL success = FALSE;
        WCHAR *url = NULL, *userName = NULL, *password = NULL;
        WCHAR *sourceFileName = NULL;

        pUrlThread->SetState(SS_IDLE);

        while (!pUrlThread->Terminated)
        {
            if (WaitForSingleObject(pUrlThread->Event, INFINITE) != WAIT_OBJECT_0)
            {
                break;
            }
            if (!pUrlThread->Alive()) goto cleanup;

            if (!pUrlThread->AcquireLock())
            {
                ResetEvent(pUrlThread->Event);
                pUrlThread->SetState(SS_BLOCKED);
                continue;
            } 

            pUrlThread->FileRequest();
            if (pUrlThread->Item.FileName[0] == L'\0')
            {
                ResetEvent(pUrlThread->Event);
                pUrlThread->SetState(SS_IDLE);
                pUrlThread->ReleaseLock();
                continue;
            }
            else
            {
                SetEvent(pUrlThread->Event);
            }
            success = FALSE;

            pUrlThread->SetState(SS_CONNECTING);

            // Get local variables
            url = ToUnicodeFromAnsi(pUrlThread->Item.SendData.Url);
            userName = ToUnicodeFromAnsi(pUrlThread->Item.SendData.UserName);
            password = ToUnicodeFromAnsi(pUrlThread->Item.SendData.Password);
            if (!url)
            {
                Log("No URL specified");
                pUrlThread->SetState(SS_FAILED_CONNECT, "Bad url");
                goto cleanup;
            }

            // Build source filename
            sourceFileName = AllocMaxPathW(pUrlThread->Item.SourcePath);
            if (sourceFileName == NULL)
            {
                Log("Out of memory");
                pUrlThread->SetState(SS_FAILED_SEND, "Out of memory");
                goto cleanup;
            }
            wcscat(sourceFileName, pUrlThread->Item.FileName);

            //
            // Execute.
            //

            switch (pUrlThread->Item.SendData.Protocol)
            {
            case FXSENDDATA_PROTOCOL_UNC:
                success = pUrlThread->SendUNC(url, userName, password, sourceFileName);
                break;

            case FXSENDDATA_PROTOCOL_FTP:
                success = pUrlThread->SendFTP(url, userName, password, sourceFileName);
                break;

            case FXSENDDATA_PROTOCOL_HTTP:
            case FXSENDDATA_PROTOCOL_HTTPS:
            case FXSENDDATA_PROTOCOL_HTTPZ:
                success = pUrlThread->SendHTTP(url, userName, password, sourceFileName, pUrlThread->Item.SendData.Protocol == FXSENDDATA_PROTOCOL_HTTPZ);
                break;

            case FXSENDDATA_PROTOCOL_ESRI:
                success = pUrlThread->SendESRI(url, userName, password, sourceFileName);
                break;

            default:
                Log("Bad protocol specified");
                pUrlThread->SetState(SS_FAILED_SEND, "Bad protocol");
            }

        cleanup:

            // Now that the files have been sent, delete from the client
            if (success == TRUE)
            {
                DeleteFileW(sourceFileName);

                WCHAR *pExt = wcsrchr(sourceFileName, L'.');
                if (pExt)
                {
                    *pExt = L'\0';
                    wcscat(sourceFileName, L".OUT");
                    DeleteFileW(sourceFileName);
                }

                pUrlThread->SetState(SS_COMPLETED, "Success");
            }

            if (url != NULL)
            {
                MEM_FREE(url);
                url = NULL;
            }

            if (userName != NULL)
            {
                MEM_FREE(userName);
                userName = NULL;
            }

            if (password != NULL)
            {
                MEM_FREE(password);
                password = NULL;
            }

            if (sourceFileName != NULL)
            {
                MEM_FREE(sourceFileName);
                sourceFileName = NULL;
            }

            if (!pUrlThread->Terminated)
            {
                if (success)
                {
                    pUrlThread->FileCompleted();
                }
                else
                {
                    pUrlThread->FileRejected();
                }
            }

            pUrlThread->Cancelled = FALSE;
        }

        pUrlThread->Free();
        return 0;
    }
};

//*************************************************************************************************

static URL_THREAD g_UrlThread = {0};

CTransferManager::CTransferManager(): _pending()
{
    ::InitializeCriticalSection(&_lock);
    _idMax = 1;
    
#ifdef CLIENT_DLL
    g_WNetAddConnection3W = &WNetAddConnection3W;
#else
    HMODULE coreLib = LoadLibrary(L"coredll");
    if (coreLib != NULL) 
    { 
        g_WNetAddConnection3W = (PFN_WNetAddConnection3W)GetProcAddress(coreLib, L"WNetAddConnection3W"); 

        FreeLibrary(coreLib);
    }
#endif
    
    g_UrlThread.Initialize(this);
    g_UrlThread.Handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)g_UrlThread.ThreadProc, &g_UrlThread, CREATE_SUSPENDED, NULL);
    //SetThreadPriority(g_UrlThread.Handle, THREAD_PRIORITY_BELOW_NORMAL);
}

CTransferManager::~CTransferManager()
{
    if (g_UrlThread.Handle != NULL)
    {
        g_UrlThread.Terminate();
    }

    DeleteCriticalSection(&_lock);
}

BOOL CTransferManager::FileRequested(TRANSFER_ITEM *pItem)
{
    UINT i;
    BOOL result = FALSE;

    ::EnterCriticalSection(&_lock);
    for (i = 0; i < _pending.GetCount(); i++)
    {
        TRANSFER_ITEM *item = _pending.GetPtr(i);
        if (item->Id == 0)
        {
            item->Id = _idMax++;
            memcpy(pItem, item, sizeof(TRANSFER_ITEM));
            result = TRUE;
            break;
        }
    }
    ::LeaveCriticalSection(&_lock);

    return result;
}

BOOL CTransferManager::Complete(TRANSFER_ITEM *pItem)
{
    UINT i;
    BOOL result = FALSE;

    ::EnterCriticalSection(&_lock);
    for (i = 0; i < _pending.GetCount(); i++)
    {
        TRANSFER_ITEM *item = _pending.GetPtr(i);
        if (item->Id == pItem->Id)
        {
            _pending.Delete(i);

            result = TRUE;
            break;
        }
    }
    ::LeaveCriticalSection(&_lock);

    return result;
}

BOOL CTransferManager::FileRejected(TRANSFER_ITEM *pItem)
{
    return Complete(pItem);
}

BOOL CTransferManager::FileCompleted(TRANSFER_ITEM *pItem)
{
    return Complete(pItem);
}

VOID CTransferManager::ClearPending()
{
    ::EnterCriticalSection(&_lock);
    _pending.Clear();
    ::LeaveCriticalSection(&_lock);
}

BOOL CTransferManager::InternalSendFile(HWND hWnd, FXSENDDATA *pSendData, WCHAR *pSourcePath, WCHAR *pFileName)
{
    BOOL result = FALSE;
    TRANSFER_ITEM item = {0};
    UINT i;
    WCHAR *fullFileName = NULL;

    for (i = 0; i < _pending.GetCount(); i++)
    {
        if (wcsicmp(_pending.GetPtr(i)->FileName, pFileName) == 0)
        {
            goto cleanup;
        }
    }

    fullFileName = AllocMaxPathW(pSourcePath);
    wcscat(fullFileName, pFileName);
    if (GetFileAttributesW(fullFileName) == 0xFFFFFFFF)
    {
        goto cleanup;
    }

    item.hWnd = hWnd;
    item.Id = 0;
    item.SendData = *pSendData;
    wcscpy(item.SourcePath, pSourcePath);
    wcscpy(item.FileName, pFileName);

    if (item.FileName[0] == L'!')
    {
        _pending.Insert(0, item);
    }
    else
    {
        _pending.Add(item);
    }

    result = TRUE;

cleanup:

    if (fullFileName != NULL)
    {
        MEM_FREE(fullFileName);
        fullFileName = NULL;
    }

    return result;
}

BOOL CTransferManager::SendFiles(HWND hWnd, WCHAR *pSourcePath)
{
    WCHAR fileSpec[MAX_PATH];
    WIN32_FIND_DATAW findData;
    HANDLE hFindFile = INVALID_HANDLE_VALUE;
    WCHAR fullFileName[MAX_PATH];
    
    if (g_UrlThread.Handle == NULL) return FALSE;

    wcscpy(fileSpec, pSourcePath);
    wcscat(fileSpec, L"*.OUT");

    ::EnterCriticalSection(&_lock);

    hFindFile = FindFirstFileW(fileSpec, &findData);
    while (hFindFile != INVALID_HANDLE_VALUE)
    {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            HANDLE hFile;

            wcscpy(fullFileName, pSourcePath);
            wcscat(fullFileName, findData.cFileName);

            hFile = CreateFileW(fullFileName,
                                GENERIC_READ, 
                                0, 
                                NULL, 
                                OPEN_EXISTING, 
                                FILE_ATTRIBUTE_NORMAL, 
                                NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                FXSENDDATA sendData;
                DWORD numRead;
                if (ReadFile(hFile, &sendData, sizeof(sendData), &numRead, NULL) &&
                    (numRead == sizeof(FXSENDDATA)))
                {
                    UINT len = wcslen(findData.cFileName);
                    if (len > 4)
                    {
                        if (sendData.Protocol <= FXSENDDATA_PROTOCOL_UNC)
                        {
                            wcscpy(findData.cFileName + len - 4, CLIENT_EXPORT_EXTW);
                        }
                        else
                        {
                            wcscpy(findData.cFileName + len - 4, CLIENT_EXPORT_JSON_EXTW);
                        }

                        InternalSendFile(hWnd, &sendData, pSourcePath, findData.cFileName);                        
                    }
                }

                CloseHandle(hFile);
            }
        }
      
        if (!FindNextFileW(hFindFile, &findData))
        {
            FindClose(hFindFile);
            break;
        }
    }

    ::LeaveCriticalSection(&_lock);

    g_UrlThread.Unblock();

    return TRUE;
}

BOOL CTransferManager::SendFile(HWND hWnd, FXSENDDATA *pSendData, WCHAR *pSourcePath, WCHAR *pFileName)
{
    if (g_UrlThread.Handle == NULL) return FALSE;

    ::EnterCriticalSection(&_lock);
    BOOL result = InternalSendFile(hWnd, pSendData, pSourcePath, pFileName);
    ::LeaveCriticalSection(&_lock);
   
    g_UrlThread.Unblock();

    return result;
}

VOID CTransferManager::Stop()
{
    if (g_UrlThread.Handle == NULL) return;

    ClearPending();
    g_UrlThread.Stop();
}

VOID CTransferManager::Terminate()
{
    if (g_UrlThread.Handle == NULL) return;

    ClearPending();
    g_UrlThread.Terminate();
    DeleteCriticalSection(&_lock);
}

VOID CTransferManager::GetState(FXSENDSTATE *pState)
{
    g_UrlThread.GetState(pState);
}

BOOL CTransferManager::IsIdle()
{
    return _pending.GetCount() == 0;
}
