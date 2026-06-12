#include "fxDatabase.h"
#include "fxUtils.h"

__inline int PadSize(int Value)
{
	return (Value + 3) & ~3;
}

CfxDatabase::CfxDatabase(CfxPersistent *pOwner, char *pFileName): CfxPersistent(pOwner)
{
	memset(&_fileMap, 0, sizeof(FXFILEMAP));
	Unmap();
    _fileName = AllocString(pFileName);
}

CfxDatabase::~CfxDatabase()
{
	Unmap();

    MEM_FREE(_fileName);
    _fileName = nullptr;
}

char *CfxDatabase::GetFileName()
{
    return _fileName;
}

bool CfxDatabase::Map()
{
    Unmap();
    return FxMapFile(_fileName, &_fileMap);
}

void CfxDatabase::Unmap()
{
	_currId = _nextId = 0;
	_index = -2;
	_cacheCount = -1;
    FxUnmapFile(&_fileMap);
}

bool CfxDatabase::DeleteDB()
{
	Unmap();
    return FxDeleteFile(_fileName);
}

bool CfxDatabase::Verify()
{
	return _fileName != nullptr;
}

bool CfxDatabase::Read(DBID Id, void **ppBuffer)
{
	void *pBuffer = nullptr;
	*ppBuffer = nullptr;
	int Size = 0;

	if (_fileMap.BasePointer == nullptr) {
		if (!Map()) {
			return false;
		}
	}

	DBCHUNK *dbChunk = (DBCHUNK *)((BYTE *)_fileMap.BasePointer + Id);
	if (dbChunk->Magic != DBCHUNK_MAGIC) {
		return false;
	}

	if (dbChunk->Size < sizeof(DBCHUNK)) {
		return false;
	}

	Size = dbChunk->Size - sizeof(DBCHUNK);

	pBuffer = MEM_ALLOC(Size);
	if (pBuffer == nullptr) {
		return false;
	}

	memcpy(pBuffer, (BYTE *)dbChunk + sizeof(DBCHUNK), Size);
	*ppBuffer = pBuffer;
	return true;
}

bool CfxDatabase::Write(DBID *pId, void *pBuffer, int Size)
{
	bool Result = false;
	FILE *f = nullptr;

    Unmap();

    if (*pId != INVALID_DBID)
    {
        Delete(*pId);
    }

	DBCHUNK *dbChunk = nullptr;

	dbChunk = (DBCHUNK *)MEM_ALLOC(PadSize(Size + sizeof(DBCHUNK)));
	if (dbChunk == nullptr) {
		goto cleanup;
	}

	dbChunk->Magic = DBCHUNK_MAGIC;
	dbChunk->Size = sizeof(DBCHUNK) + Size;
	dbChunk->Flags = 0;
	dbChunk->Padding = 0;
	memcpy((BYTE *)dbChunk + sizeof(DBCHUNK), pBuffer, Size);

	f = fopen(_fileName, "ab");
	if (f == nullptr) {
		goto cleanup;
	}

	if (fwrite(dbChunk, PadSize(dbChunk->Size), 1, f) != 1) {
		goto cleanup;
	}

    if (fflush(f) != 0) {
        goto cleanup;
    }

	Result = true;

  cleanup:

    if (dbChunk != nullptr) {
	    MEM_FREE(dbChunk);
    }

    if (f != nullptr) {
    	fclose(f);
    }

    //Updated();

	return Result;
}

bool CfxDatabase::Delete(DBID Id)
{
	bool Result = false;
	DBCHUNK dbChunk = {0};
	FILE *f = nullptr;

	Unmap();

    f = fopen(_fileName, "rb+");
    if (f == nullptr) {
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

    Result = true;

  cleanup:

    if (f != nullptr) {
    	fclose(f);
    }

    //Updated();

	return Result;
}

DBCHUNK *CfxDatabase::FindFirst()
{
	_currId = _nextId = 0;
	_index = 0;
	return FindNext();
}

DBCHUNK *CfxDatabase::FindNext()
{
	DBCHUNK *dbChunk;

	if (_nextId + sizeof(DBCHUNK) > _fileMap.FileSize) {
		return nullptr;
	}

	dbChunk = (DBCHUNK *)((BYTE *)_fileMap.BasePointer + _nextId);
	if (dbChunk->Magic != DBCHUNK_MAGIC) {
		return nullptr;
	}

	_currId = _nextId;
	_nextId = _nextId + PadSize(dbChunk->Size);

	return dbChunk;
}

bool CfxDatabase::Enum(int Index, DBID *pId)
{
	*pId = INVALID_DBID;

	if (_fileMap.BasePointer == nullptr) {
		if (!Map()) {
			return false;
		}
	}

	DBCHUNK *dbChunk;

	if (Index == _index + 1) {
		if (_currId + sizeof(DBCHUNK) > _fileMap.FileSize) {
			return false;
		}
		dbChunk = (DBCHUNK *)((BYTE *)_fileMap.BasePointer + _currId);
	} else {
		dbChunk = FindFirst();
	}

	while (dbChunk != nullptr) {
		if ((dbChunk->Flags & DB_FLAG_DELETED) == 0) {
			if (Index == _index) {
				*pId = _currId;
				return true;
			}
			_index++;
		}

		dbChunk = FindNext();
	}

	return false;
}

bool CfxDatabase::GetIndex(DBID Id, int *pIndex)
{
	int index = 0;
	DBID dbId = INVALID_DBID;

	while (Enum(index, &dbId)) {
		if (Id == dbId) {
			*pIndex = index;
			return true;
		}

		index++;
	}

	return false;
}

bool CfxDatabase::TestId(DBID Id)
{
	if (_fileMap.BasePointer == nullptr) {
		if (!Map()) {
			return false;
		}
	}

	return (Id + sizeof(DBCHUNK) < _fileMap.FileSize) &&
		   (((DBCHUNK *)((BYTE *)_fileMap.BasePointer + Id))->Magic == DBCHUNK_MAGIC);
}

int CfxDatabase::GetCount()
{
	if (_cacheCount != -1) {
		return _cacheCount;
	}

	int count = 0;
	DBID dbId = INVALID_DBID;

	while (Enum(count, &dbId)) {
		count++;
	}

    _cacheCount = count;

	return count;
}

bool CfxDatabase::DeleteAll()
{
    Unmap();

    bool result = DeleteDB();

    //Updated();

    return result;
}
