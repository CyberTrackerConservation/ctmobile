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

//
// CfxDatabase
//
class CfxDatabase: public CfxPersistent
{
public:
    CfxDatabase(CfxPersistent *pOwner, CHAR *pDatabaseName): CfxPersistent(pOwner) {}

    virtual CHAR *GetFileName()=0;

    virtual BOOL Verify()=0;

    virtual BOOL Read(DBID Id, VOID **ppBuffer)=0;
    virtual BOOL Write(DBID *pId, VOID *pBuffer, UINT Size)=0;

    virtual BOOL Delete(DBID Id)=0;
    virtual BOOL Enum(UINT Index, DBID *pId)=0;
    virtual BOOL GetIndex(DBID Id, INT *pIndex)=0;
    virtual BOOL TestId(DBID Id)=0;

    virtual UINT GetCount()=0;

    virtual BOOL DeleteAll();

    virtual BOOL Full() { return FALSE; }
};

