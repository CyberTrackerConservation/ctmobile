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

#include "ChunkDatabase.h"
#include "fxUtils.h"
#include "fxHost.h"

//
// CChunkDatabase
//

__inline int PadSize(int Value)
{
	return (Value + 3) & ~3;
}

CChunkDatabase::CChunkDatabase(CfxPersistent *pOwner, CHAR *pDatabaseName): CfxDatabase(pOwner, pDatabaseName)
{
	memset(&_fileMap, 0, sizeof(FXFILEMAP));
	Unmap();
    CfxHost *host = (CfxHost *)pOwner;
    _fileName = host->AllocPathApplication(pDatabaseName);
}

CChunkDatabase::~CChunkDatabase()
{
	Unmap();

    MEM_FREE(_fileName);
    _fileName = NULL;
}

CHAR *CChunkDatabase::GetFileName()
{
    return _fileName;
}

BOOL CChunkDatabase::Map()
{
    Unmap();
    return FxMapFile(_fileName, &_fileMap);
}

VOID CChunkDatabase::Unmap()
{
	_currId = _nextId = 0;
	_index = -2;
	_cacheCount = -1;
    FxUnmapFile(&_fileMap);
}

BOOL CChunkDatabase::DeleteDB()
{
	Unmap();
    return FxDeleteFile(_fileName);
}

BOOL CChunkDatabase::Verify()
{
	return _fileName != NULL;
}

BOOL CChunkDatabase::Read(DBID Id, VOID **ppBuffer)
{
	VOID *pBuffer = NULL;
	*ppBuffer = NULL;
	UINT Size = 0;

	if (_fileMap.BasePointer == NULL) {
		if (!Map()) {
			return FALSE;
		}
	}

	DBCHUNK *dbChunk = (DBCHUNK *)((BYTE *)_fileMap.BasePointer + Id);
	if (dbChunk->Magic != DBCHUNK_MAGIC) {
		return FALSE;
	}

	if (dbChunk->Size < sizeof(DBCHUNK)) {
		return FALSE;
	}

	Size = dbChunk->Size - sizeof(DBCHUNK);

	pBuffer = MEM_ALLOC(Size);
	if (pBuffer == NULL) {
		return FALSE;
	}

	memcpy(pBuffer, (BYTE *)dbChunk + sizeof(DBCHUNK), Size);
	*ppBuffer = pBuffer;
	return TRUE;
}

BOOL CChunkDatabase::Write(DBID *pId, VOID *pBuffer, UINT Size)
{
	BOOL Result = FALSE;
	FILE *f = NULL;

    Unmap();

    if (*pId != INVALID_DBID)
    {
        Delete(*pId);
    }

	DBCHUNK *dbChunk = NULL;

	dbChunk = (DBCHUNK *)MEM_ALLOC(PadSize(Size + sizeof(DBCHUNK)));
	if (dbChunk == NULL) {
		goto cleanup;
	}

	dbChunk->Magic = DBCHUNK_MAGIC;
	dbChunk->Size = sizeof(DBCHUNK) + Size;
	dbChunk->Flags = 0;
	dbChunk->Padding = 0;
	memcpy((BYTE *)dbChunk + sizeof(DBCHUNK), pBuffer, Size);

	f = fopen(_fileName, "ab");
	if (f == NULL) {
		goto cleanup;
	}

	if (fwrite(dbChunk, PadSize(dbChunk->Size), 1, f) != 1) {
		goto cleanup;
	}

    if (fflush(f) != 0) {
        goto cleanup;
    }

	Result = TRUE;

  cleanup:

    if (dbChunk != NULL) {
	    MEM_FREE(dbChunk);
    }

    if (f != NULL) {
    	fclose(f);
    }

    //Updated();

	return Result;
}

BOOL CChunkDatabase::Delete(DBID Id)
{
	BOOL Result = FALSE;
	DBCHUNK dbChunk = {0};
	FILE *f = NULL;

	Unmap();

    f = fopen(_fileName, "rb+");
    if (f == NULL) {
    	goto cleanup;
    }

    if (fseek(f, Id, SEEK_SET) != 0) {
    	goto cleanup;
    }

    if (fread(&dbChunk, sizeof(DBCHUNK), 1, f) != 1) {
    	goto cleanup;
    }

    dbChunk.Flags |= DB_FLAG_DELETED;

    if (fseek(f, Id, SEEK_SET) != 0) {
    	goto cleanup;
    }

    if (fwrite(&dbChunk, sizeof(DBCHUNK), 1, f) != 1) {
    	goto cleanup;
    }

    Result = TRUE;

  cleanup:

    if (f != NULL) {
    	fclose(f);
    }

    //Updated();

	return Result;
}

DBCHUNK *CChunkDatabase::FindFirst()
{
	_currId = _nextId = 0;
	_index = 0;
	return FindNext();
}

DBCHUNK *CChunkDatabase::FindNext()
{
	DBCHUNK *dbChunk;

	if (_nextId + sizeof(DBCHUNK) > _fileMap.FileSize) {
		return NULL;
	}

	dbChunk = (DBCHUNK *)((BYTE *)_fileMap.BasePointer + _nextId);
	if (dbChunk->Magic != DBCHUNK_MAGIC) {
		return NULL;
	}

	_currId = _nextId;
	_nextId = _nextId + PadSize(dbChunk->Size);

	return dbChunk;
}

BOOL CChunkDatabase::Enum(UINT Index, DBID *pId)
{
	*pId = INVALID_DBID;

	if (_fileMap.BasePointer == NULL) {
		if (!Map()) {
			return FALSE;
		}
	}

	DBCHUNK *dbChunk;

	if (Index == _index + 1) {
		if (_currId + sizeof(DBCHUNK) > _fileMap.FileSize) {
			return FALSE;
		}
		dbChunk = (DBCHUNK *)((BYTE *)_fileMap.BasePointer + _currId);
	} else {
		dbChunk = FindFirst();
	}

	while (dbChunk != NULL) {
		if ((dbChunk->Flags & DB_FLAG_DELETED) == 0) {
			if (Index == _index) {
				*pId = _currId;
				return TRUE;
			}
			_index++;
		}

		dbChunk = FindNext();
	}

	return FALSE;
}

BOOL CChunkDatabase::GetIndex(DBID Id, INT *pIndex)
{
	UINT index = 0;
	DBID dbId = INVALID_DBID;

	while (Enum(index, &dbId)) {
		if (Id == dbId) {
			*pIndex = index;
			return TRUE;
		}

		index++;
	}

	return FALSE;
}

BOOL CChunkDatabase::TestId(DBID Id)
{
	if (_fileMap.BasePointer == NULL) {
		if (!Map()) {
			return FALSE;
		}
	}

	return (Id + sizeof(DBCHUNK) < _fileMap.FileSize) &&
		   (((DBCHUNK *)((BYTE *)_fileMap.BasePointer + Id))->Magic == DBCHUNK_MAGIC);
}

UINT CChunkDatabase::GetCount()
{
	if (_cacheCount != -1) {
		return _cacheCount;
	}

	INT count = 0;
	DBID dbId = INVALID_DBID;

	while (Enum(count, &dbId)) {
		count++;
	}

    _cacheCount = count;

	return count;
}

BOOL CChunkDatabase::DeleteAll()
{
    Unmap();

    BOOL result = DeleteDB();

    //Updated();

    return result;
}

