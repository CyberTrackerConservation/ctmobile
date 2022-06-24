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

#include "fxUtils.h"
#include "Win32_Utils.h"

//
// Platform APIs.
//

VOID FxHardFault(CHAR *pFile, int Line)
{
    Log("Fault in %S, Line %d", pFile, Line);

    WCHAR msg[MAX_PATH]; 
    wsprintfW(msg, L"Fault in File: %S, Line %d", pFile, Line); 
    MessageBoxW(NULL, msg, L"Error", MB_OK); 
    DebugBreak(); 
}

VOID FxOutputDebugString(CHAR *pMessage)
{
    WCHAR *messageW = ToUnicodeFromAnsi(pMessage);
    if (messageW) 
    {
        OutputDebugStringW(messageW);
        MEM_FREE(messageW);
    }
}

UINT FxGetTickCount()
{
    return GetTickCount();
}

UINT FxGetTicksPerSecond()
{
    return 1000;
}

VOID FxSleep(UINT Milliseconds)
{
    Sleep(Milliseconds);
}

BOOL FxCreateDirectory(const CHAR *pPath)
{
    BOOL success = FALSE;
    WCHAR *pPathW = ToUnicodeFromAnsi(pPath);

    TrimSlashW(pPathW);

    success = CreateDirectoryW(pPathW, NULL);
    if (!success && (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        success = TRUE;
    }

    MEM_FREE(pPathW);

    return success;
}

BOOL FxDeleteDirectory(const CHAR *pPath)
{
    WCHAR *pathW = ToUnicodeFromAnsi(pPath);
    TrimSlashW(pathW);
    BOOL result = RemoveDirectoryW(pathW);
    MEM_FREE(pathW);
    return result;
}

UINT FxGetFileSize(const CHAR *pFileName)
{
    UINT size = 0;
    HANDLE handle;
    WCHAR *pFileNameW = ToUnicodeFromAnsi(pFileName);
    if (pFileNameW) 
    {
        handle = CreateFileW(pFileNameW, 
                             GENERIC_READ, 
                             FILE_SHARE_READ, 
                             NULL, 
                             OPEN_EXISTING, 
                             FILE_ATTRIBUTE_NORMAL, 
                             NULL);

        if (handle != INVALID_HANDLE_VALUE)
        {
            size = GetFileSize(handle, NULL);
            CloseHandle(handle);
        }
    }

    MEM_FREE(pFileNameW);

    return size;
}

BOOL FxDeleteFile(const CHAR *pFilename)
{
    BOOL success = FALSE;
    WCHAR *pFilenameW = NULL;

    pFilenameW = ToUnicodeFromAnsi(pFilename);
    if (pFilenameW == NULL)
    {
        goto cleanup;
    }

    success = DeleteFileW(pFilenameW);

cleanup:

    MEM_FREE(pFilenameW);

    return success;
}

BOOL FxDoesFileExist(const CHAR *pFileName)
{
    BOOL exists = FALSE;
    WCHAR *pFileNameW = ToUnicodeFromAnsi(pFileName);
    if (pFileNameW) 
    {
        exists = GetFileAttributesW(pFileNameW) != 0xFFFFFFFF;
    }

    MEM_FREE(pFileNameW);

    return exists;
}

BOOL FxMoveFile(const CHAR *pExisting, const CHAR *pNew)
{
    BOOL result = FALSE;
    WCHAR *existingW = ToUnicodeFromAnsi(pExisting);
    WCHAR *newW = ToUnicodeFromAnsi(pNew);
    if (existingW && newW) 
    {
        result = MoveFileW(existingW, newW);
    }

    MEM_FREE(existingW);
    MEM_FREE(newW);

    return result;
}

BOOL FxCopyFile(const CHAR *pExisting, const CHAR *pNew)
{
    BOOL result = FALSE;
    WCHAR *existingW = ToUnicodeFromAnsi(pExisting);
    WCHAR *newW = ToUnicodeFromAnsi(pNew);
    if (existingW && newW) 
    {
        result = CopyFileW(existingW, newW, FALSE);
    }

    MEM_FREE(existingW);
    MEM_FREE(newW);

    return result;
}

//
// Win32_Utils APIs.
//

CHAR *ToAnsi(const WCHAR *pValue)
{
    if (pValue == NULL)
    {
        return NULL;
    }

    INT len = wcslen(pValue);
    CHAR *value = (CHAR *) MEM_ALLOC((len + 1) * sizeof(CHAR));

    if (!value)
    {
        return NULL;
    }

    WideCharToMultiByte(CP_ACP, 0, pValue, len, value, len, NULL, NULL);
    value[len] = 0;

    return value;
}

CHAR *ToUtf8(const WCHAR *pValue)
{
    if (pValue == NULL)
    {
        return NULL;
    }

    INT len = wcslen(pValue);
    CHAR *value = (CHAR *) MEM_ALLOC((len + 1) * sizeof(WCHAR));

    if (!value)
    {
        return NULL;
    }

    WideCharToMultiByte(CP_UTF8, 0, pValue, len, value, len * sizeof(WCHAR), NULL, NULL);
    //value[len] = 0;

    return value;
}

WCHAR *ToUnicodeFromAnsi(const CHAR *pValue)
{
    if (pValue == NULL)
    {
        return NULL;
    }

    INT len = strlen(pValue) + 1;
    WCHAR *value = (WCHAR *) MEM_ALLOC((len + 1) * sizeof(WCHAR));
	memset(value, 0, len * sizeof(WCHAR));

    if (!value)
    {
        return NULL;
    }

    MultiByteToWideChar(CP_ACP, 0, pValue, len, value, len);

    return value;
}

WCHAR *ToUnicodeFromUtf8(const CHAR *pValue)
{
    if (pValue == NULL)
    {
        return NULL;
    }

    INT len = strlen(pValue) + 1;
    WCHAR *value = (WCHAR *) MEM_ALLOC((len + 1) * sizeof(WCHAR));
	memset(value, 0, len * sizeof(WCHAR));

    if (!value)
    {
        return NULL;
    }

    MultiByteToWideChar(CP_UTF8, 0, pValue, len, value, len);

    return value;
}

WCHAR *DupStringW(const WCHAR *pValue)
{
    if (!pValue)
    {
        return NULL;
    }

    WCHAR *value = (WCHAR *) MEM_ALLOC((wcslen(pValue) + 1) * sizeof(WCHAR));
    if (value)
    {
        wcscpy(value, pValue);
    }

    return value;
}

WCHAR *AllocMaxPathW(const WCHAR *pSource)
{
	WCHAR *result;
	UINT len = MAX_PATH * sizeof(WCHAR);

	result = (WCHAR *)MEM_ALLOC(len);
	if (result != NULL)
	{
		memset(result, 0, len);
	}

	if (pSource != NULL)
	{
		wcscpy(result, pSource);
	}

	return result;
}

VOID AppendSlashW(WCHAR *pPath)
{
	if (pPath)
    {
	    UINT len = wcslen(pPath);
	    if ((len > 0) && (pPath[len-1] != L'\\')) 
	    {
		    wcscat(pPath, L"\\");
	    }
    }
}

VOID TrimSlashW(WCHAR *pPath)
{
	if (pPath)
    {
	    UINT len = wcslen(pPath);
	    if ((len > 0) && (pPath[len-1] == L'\\')) 
	    {
		    pPath[len-1] = '\0';
	    }
    }
}

WCHAR *ExtractFileNameW(WCHAR *pFullFileName)
{
    WCHAR *pTemp = wcsrchr(pFullFileName, L'\\');
    if (pTemp)
    {
        return pTemp + 1;
    }
    else
    {
        return NULL;
    }
}

UINT GetFileSizeW(WCHAR *pFileName)
{
    UINT result = 0;
    CHAR *fileName = ToAnsi(pFileName);
    if (fileName)
    {
        result = FxGetFileSize(fileName);
    }

    MEM_FREE(fileName);

    return result;
}

WCHAR *AllocMaxPathApplicationW()
{
    WCHAR *result = AllocMaxPathW(NULL);
    WCHAR *temp = NULL;

    if (result == NULL)
    {
        goto cleanup;
    }

    if (GetModuleFileNameW(NULL, result, MAX_PATH-1) == 0)
    {
        MEM_FREE(result);
        result = NULL;
        goto cleanup;
    }

    temp = wcsrchr(result, L'\\');
    if (temp == NULL)
    {
        MEM_FREE(result);
        result = NULL;
        goto cleanup;
    }

    *temp = L'\0';

    AppendSlashW(result);

cleanup:

    return result;
}

BOOL FxBuildFileList(const CHAR *pPath, const CHAR *pPattern, FXFILELIST *pFileList)
{
    FileListClear(pFileList);

    BOOL result = FALSE;
    WIN32_FIND_DATAW findData;
    HANDLE hFindFile = INVALID_HANDLE_VALUE;
    FXFILE inputFileInfo = {0};
    WCHAR *pathW = NULL;
    WCHAR *patternW = NULL;
    WCHAR *workPath = NULL;

    pathW = ToUnicodeFromAnsi(pPath);
    patternW = ToUnicodeFromAnsi(pPattern);
    workPath = AllocMaxPathW(pathW);

    if (!pathW || !patternW || !workPath)
    {
        goto done;
    }

    AppendSlashW(workPath);
    wcscat(workPath, patternW);

    hFindFile = FindFirstFileW(workPath, &findData);
    while (hFindFile != INVALID_HANDLE_VALUE)
    {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            inputFileInfo.Name = ToAnsi(findData.cFileName);

            wcscpy(workPath, pathW);
            AppendSlashW(workPath);
            wcscat(workPath, findData.cFileName);
            inputFileInfo.FullPath = ToAnsi(workPath);

            inputFileInfo.Size = findData.nFileSizeLow;

            SYSTEMTIME systemTime;
            FileTimeToSystemTime(&findData.ftLastWriteTime, &systemTime);
		    #ifndef _WIN32_WCE
			    SystemTimeToTzSpecificLocalTime(NULL, &systemTime, &systemTime);
		    #endif

            inputFileInfo.TimeStamp.Day = systemTime.wDay;
            inputFileInfo.TimeStamp.Hour = systemTime.wHour;
            inputFileInfo.TimeStamp.Milliseconds = systemTime.wMilliseconds;
            inputFileInfo.TimeStamp.Minute = systemTime.wMinute;
            inputFileInfo.TimeStamp.Month = systemTime.wMonth;
            inputFileInfo.TimeStamp.Second = systemTime.wSecond;
            inputFileInfo.TimeStamp.Year = systemTime.wYear;

            pFileList->Add(inputFileInfo);
        }
      
        if (!FindNextFileW(hFindFile, &findData))
        {
            FindClose(hFindFile);
            break;
        }
    }

    result = TRUE;

done:

    MEM_FREE(pathW);
    MEM_FREE(patternW);
    MEM_FREE(workPath);

    return result;
}

BOOL FxMapFile(const CHAR *pFileName, FXFILEMAP *pFileMap)
{
	BOOL Result = FALSE;
    WCHAR *fileName = ToUnicodeFromAnsi(pFileName);

    pFileMap->FileHandle = 
#ifdef _WIN32_WCE
                           CreateFileForMapping(
#else
                           CreateFileW(
#endif
                                       fileName,
                                       GENERIC_READ, 
                                       FILE_SHARE_READ, 
                                       NULL, 
                                       OPEN_EXISTING, 
                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, 
                                       NULL);
    if (pFileMap->FileHandle == INVALID_HANDLE_VALUE) 
    {
        pFileMap->FileHandle = 0;
        goto Exit;
    }

    pFileMap->FileSize = GetFileSize(pFileMap->FileHandle, NULL);

    pFileMap->MapHandle = CreateFileMapping(pFileMap->FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (pFileMap->MapHandle == 0)
    {
        CloseHandle(pFileMap->FileHandle);
        pFileMap->FileHandle = 0;
        goto Exit;
    }

    pFileMap->BasePointer = MapViewOfFile(pFileMap->MapHandle, FILE_MAP_READ, 0, 0, 0);

    if (pFileMap->BasePointer == NULL)
    {
        CloseHandle(pFileMap->FileHandle);
        pFileMap->FileHandle = 0;

        CloseHandle(pFileMap->MapHandle);
        pFileMap->MapHandle = 0;

        goto Exit;
    }

    Result = TRUE;

Exit:

    MEM_FREE(fileName);

    return Result;
}

VOID FxUnmapFile(FXFILEMAP *pFileMap)
{
    if (pFileMap->BasePointer)
    {
        UnmapViewOfFile(pFileMap->BasePointer);
	    pFileMap->BasePointer = NULL;
    }

    if (pFileMap->MapHandle)
    {
        CloseHandle(pFileMap->MapHandle);
        pFileMap->MapHandle = 0;
    }

    if (pFileMap->FileHandle != 0)
    {
        CloseHandle(pFileMap->FileHandle);
    	pFileMap->FileHandle = 0;
    }
}

BOOL GetRegValueString(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue, WCHAR **pValue)
{
    HKEY key;
    BOOL ret = FALSE;
    *pValue = NULL;
    if (ERROR_SUCCESS == RegOpenKeyExW(hKeyBase, lpKey, 0, MAXIMUM_ALLOWED, &key))
    {
        DWORD dataType;
        DWORD dataSize = sizeof(DWORD);
        if ((RegQueryValueExW(key, lpValue, NULL, &dataType, NULL, &dataSize) == ERROR_SUCCESS) &&
            (dataSize > 0))
        {
             *pValue = (WCHAR *)MEM_ALLOC(dataSize);
             if (*pValue)
             {
                ret = (RegQueryValueExW(key, lpValue, NULL, &dataType, (LPBYTE)(*pValue), &dataSize) == ERROR_SUCCESS);
             }
        }
        RegCloseKey(key);
    }
    return ret;
}

BOOL SetRegValueString(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue, const WCHAR *pValue)
{
    HKEY key;
    BOOL ret = FALSE;
    if (ERROR_SUCCESS == RegCreateKeyExW(hKeyBase, lpKey, 0, NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, &key, NULL))
    {
        ret = (RegSetValueExW(key, lpValue, 0, REG_SZ, (LPBYTE)pValue, (wcslen(pValue) + 1) * sizeof(WCHAR)) == ERROR_SUCCESS);
        RegCloseKey(key);
    }
    return ret;
}

BOOL GetRegValueDword(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue, DWORD *pValue)
{
    HKEY key;
    BOOL ret = FALSE;
    *pValue = 0;
    if (ERROR_SUCCESS == RegOpenKeyExW(hKeyBase, lpKey, 0, MAXIMUM_ALLOWED, &key))
    {
        DWORD dataType;
        DWORD dataSize = sizeof(DWORD);
        ret = (RegQueryValueExW(key, lpValue, NULL, &dataType, (LPBYTE)pValue, &dataSize) == ERROR_SUCCESS);
        RegCloseKey(key);
    }
    return ret;
}

BOOL SetRegValueDword(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue, const DWORD Value)
{
    HKEY key;
    BOOL ret = FALSE;
    if (ERROR_SUCCESS == RegCreateKeyExW(hKeyBase, lpKey, 0, NULL, REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, &key, NULL))
    {
        ret = (RegSetValueExW(key, lpValue, 0, REG_DWORD, (LPBYTE)&Value, sizeof(Value)) == ERROR_SUCCESS);
        RegCloseKey(key);
    }
    return ret;
}

BOOL GetRegValueBool(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue)
{
    DWORD dwValue;

    if (GetRegValueDword(hKeyBase, lpKey, lpValue, &dwValue))
    {
        return (BOOL)dwValue;
    }
    else
    {
        return FALSE;
    }
}

VOID SetRegValueBool(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue, const BOOL Value)
{
    DWORD dwValue = (DWORD)Value;
    SetRegValueDword(hKeyBase, lpKey, lpValue, dwValue);
}

CHAR *GetDateStringUTC(DOUBLE Value, BOOL FormatAsUTC)
{
    TIME_ZONE_INFORMATION timeZoneInfo = {0};
    DOUBLE delta = 0;
    switch (GetTimeZoneInformation(&timeZoneInfo))
    {
    case TIME_ZONE_ID_UNKNOWN:
        break;

    case TIME_ZONE_ID_STANDARD:
        delta = timeZoneInfo.StandardBias * 60;
        break;

    case TIME_ZONE_ID_DAYLIGHT:
        delta = timeZoneInfo.DaylightBias * 60;
        break;
    }

    delta += timeZoneInfo.Bias * 60;

    CHAR dateString[64];
    if ( FormatAsUTC == TRUE )
    {
        //Convert to UTC
        Value += (delta / 86400);

        FXDATETIME final;
        DecodeDateTime(Value, &final);
        sprintf(dateString,
                "%04d-%02d-%02dT%02d:%02d:%02d.%dZ", 
                final.Year, final.Month, final.Day,
                final.Hour, final.Minute, final.Second,
                final.Milliseconds);
        
    }
    else
    {
        delta /= 86400;
        delta *= 24;
        UINT deltaHour = (UINT)fabs(delta);
        UINT deltaMinute = (UINT)((fabs(delta) - deltaHour) * 60);

        FXDATETIME final;
        DecodeDateTime(Value, &final);

        sprintf(dateString, 
            "%04d-%02d-%02dT%02d:%02d:%02d.%d%c%02d:%02d",
            final.Year, final.Month, final.Day,
            final.Hour, final.Minute, final.Second,
            final.Milliseconds,
            delta > 0 ? '-' : '+',
            deltaHour, deltaMinute);

    }

    return AllocString(dateString);
}