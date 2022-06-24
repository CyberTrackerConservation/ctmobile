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

#include "fxDatabase.h"

class CDatafile_Win32: public CfxDatafile
{
protected:
    HANDLE _handle;
    WCHAR *_fileName;
    DBCHUNK *_chunk;
    UINT32 _chunkSize;
    UINT32 _lastOffset, _currOffset;
    BOOL OpenFile();
    VOID CloseFile();
    BOOL EnsureChunkSize(UINT32 Size);
public:
    CDatafile_Win32(CfxPersistent *pOwner, CHAR *pFileName);
    ~CDatafile_Win32();

    BOOL Verify();
    BOOL IsFull();

    BOOL Delete();

    DBCHUNK *FindFirst();
    DBCHUNK *FindNext();
    UINT32 GetCurrentOffset();
    DBCHUNK *GetAtOffset(UINT32 Offset);
  
    BOOL WriteRaw(UINT32 Offset, VOID *pData, UINT32 Size);
    BOOL WriteChunk(UINT32 Offset, UINT32 Index, UINT16 Type, VOID *pData, UINT32 Size);
};
