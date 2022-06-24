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

#include "Win32_Chunk.h"
#include "Win32_Utils.h"
#include "fxUtils.h"

CDatafile_Win32::CDatafile_Win32(CfxPersistent *pOwner, CHAR *pFileName): CfxDatafile(pOwner, pFileName)
{
    _handle = INVALID_HANDLE_VALUE;
    _fileName = NULL;
    _lastOffset = 0;
    _chunk = NULL;
    _chunkSize = 0;
    EnsureChunkSize(256);

#if defined(_WIN32_WCE)
    WCHAR *fullPath = GetApplicationPathW();
#else
    WCHAR *fullPath = GetTempPathW(FALSE);
#endif

    WCHAR *fileName = ToUnicode(pFileName);

    if ((fileName == NULL) || (fullPath == NULL))
    {
        goto Done;
    }

    if (wcschr(fileName, L'\\'))
    {
        wcscpy(fullPath, fileName);
    }
    else
    {
        wcscat(fullPath, fileName);
    }

    _fileName = fullPath;
    fullPath = NULL;

    OpenFile();

Done:

    free(fullPath);
    free(fileName);
}

CDatafile_Win32::~CDatafile_Win32()
{
    CloseFile();
    free(_fileName);
    _fileName = NULL;

    free(_chunk);
    _chunk = NULL;
    _chunkSize = 0;
}

BOOL CDatafile_Win32::OpenFile()
{
    CloseFile();

    _handle = CreateFileW(_fileName, 
                          GENERIC_READ | GENERIC_WRITE, 
                          FILE_SHARE_READ | FILE_SHARE_WRITE, 
                          NULL, 
                          OPEN_ALWAYS, 
                          FILE_ATTRIBUTE_NORMAL, 
                          NULL);
    return _handle != INVALID_HANDLE_VALUE;
}

VOID CDatafile_Win32::CloseFile()
{
    if (_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_handle);
        _handle = INVALID_HANDLE_VALUE;
    }
}

BOOL CDatafile_Win32::EnsureChunkSize(UINT32 Size)
{
    if (_chunkSize >= Size) return TRUE;

    DBCHUNK *newChunk;

    if (_chunk == NULL)
    {
        newChunk = (DBCHUNK *)malloc(Size);
    }
    else
    {
        newChunk = (DBCHUNK *)realloc(_chunk, Size);
    }

    if (newChunk != NULL) 
    {
        _chunk = newChunk;
        _chunkSize = Size;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL CDatafile_Win32::Verify()
{
    return (_handle != INVALID_HANDLE_VALUE);
}

BOOL CDatafile_Win32::IsFull()
{
    BOOL result = TRUE;
    ULARGE_INTEGER FreeBytesAvailable = {0};

#if defined(_WIN32_WCE)
    WCHAR *fullPath = GetApplicationPathW();
#else
    WCHAR *fullPath = GetTempPathW(FALSE);
#endif

    GetDiskFreeSpaceExW(fullPath, &FreeBytesAvailable, NULL, NULL);

    free(fullPath);

    return (FreeBytesAvailable.QuadPart < 64 * 1024);
}

BOOL CDatafile_Win32::Delete()
{
    SetFilePointer(_handle, 0, 0, FILE_BEGIN);
    return SetEndOfFile(_handle);
}

DBCHUNK *CDatafile_Win32::FindFirst()
{
    if (_handle == INVALID_HANDLE_VALUE) return NULL;
    if (SetFilePointer(_handle, 0, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER) return NULL;
    _lastOffset = _currOffset = 0;

    return FindNext();
}

DBCHUNK *CDatafile_Win32::FindNext()
{
    DBCHUNK *dbChunkOut = NULL;
    UINT32 numberToRead, numberRead;

    if (!ReadFile(_handle, _chunk, sizeof(DBCHUNK), &numberRead, NULL)) goto Done;
    if (numberRead != sizeof(DBCHUNK)) goto Done;

    if (!EnsureChunkSize(_chunk->Size)) goto Done;

    numberToRead = _chunk->Size - sizeof(DBCHUNK);
    numberToRead = (numberToRead + 3) & ~3;
    if (!ReadFile(_handle, (PBYTE)_chunk + sizeof(DBCHUNK), numberToRead, &numberRead, NULL)) goto Done;
    if (numberToRead != numberRead) goto Done;

    _lastOffset = _currOffset;
    _currOffset = _lastOffset + sizeof(DBCHUNK) + numberToRead;

    dbChunkOut = _chunk;

Done:

    return dbChunkOut;
}

UINT32 CDatafile_Win32::GetCurrentOffset()
{
    return _lastOffset;
}

DBCHUNK *CDatafile_Win32::GetAtOffset(UINT32 Offset)
{
    if (_handle == INVALID_HANDLE_VALUE) return NULL;
    if (SetFilePointer(_handle, Offset, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER) return NULL;

    memset(_chunk, 0, _chunkSize);
    return FindNext();
}

BOOL CDatafile_Win32::WriteRaw(UINT32 Offset, VOID *pData, UINT32 Size)
{
    BOOL success = FALSE;

    if (_handle == INVALID_HANDLE_VALUE) goto Done;
    
    if (Offset == (UINT32)-1)
    {
        if (SetFilePointer(_handle, 0, 0, FILE_END) == INVALID_SET_FILE_POINTER) 
        {
            goto Done;
        }
    }
    else
    {
        if (SetFilePointer(_handle, Offset, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER) 
        {
            goto Done;
        }
    }

    UINT32 numberWritten;
    Size = (Size + 3) & ~3;
    if (WriteFile(_handle, pData, Size, &numberWritten, NULL))
    {
        success = numberWritten == Size;
    }
    FlushFileBuffers(_handle);

Done:

    return success;
}

BOOL CDatafile_Win32::WriteChunk(UINT32 Offset, UINT32 Index, UINT16 Type, VOID *pData, UINT32 Size)
{
    if (_handle == INVALID_HANDLE_VALUE) return FALSE;

    if (!EnsureChunkSize(Size + sizeof(DBCHUNK))) return FALSE;

    _chunk->Magic = DBCHUNK_MAGIC;
    _chunk->Index = Index;
    _chunk->Type = Type;
    _chunk->Size = Size + sizeof(DBCHUNK);
    _chunk->Flags = 0;
    memcpy((PBYTE)_chunk + sizeof(DBCHUNK), pData, Size);

    return WriteRaw(Offset, _chunk, _chunk->Size);
}

