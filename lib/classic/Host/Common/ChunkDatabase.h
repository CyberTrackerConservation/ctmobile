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
#include "pch.h"
#include "fxTypes.h"
#include "fxClasses.h"
#include "fxDatabase.h"

//
// CfxChunkDatabase
//

#define DBCHUNK_MAGIC    'dbct'
#define DB_FLAG_DELETED  0x00000001

struct DBCHUNK
{
    UINT Magic;
    UINT Size;
    UINT Flags;
    UINT Padding;
};

class CChunkDatabase: public CfxDatabase
{
protected:
    CHAR *_fileName;
    FXFILEMAP _fileMap;
    UINT _index;
    DBID _currId, _nextId;
    INT _cacheCount;
    BOOL Map();
    VOID Unmap();
    BOOL DeleteDB();
    DBCHUNK *FindFirst();
    DBCHUNK *FindNext();
public:
    CChunkDatabase(CfxPersistent *pOwner, CHAR *pDatabaseName);
    virtual ~CChunkDatabase();

    CHAR *GetFileName();

    BOOL Verify();

    BOOL Read(DBID Id, VOID **ppBuffer);
    BOOL Write(DBID *pId, VOID *pBuffer, UINT Size);

    BOOL Delete(DBID Id);
    BOOL Enum(UINT Index, DBID *pId);
    BOOL TestId(DBID Id);

    BOOL GetIndex(DBID Id, INT *pIndex);
    UINT GetCount();

    BOOL DeleteAll();

    BOOL Full()    { return FALSE; }
};
