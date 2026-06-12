#pragma once
#include "pch.h"
#include "fxTypes.h"
#include "fxClasses.h"

#define DBCHUNK_MAGIC    'dbct'
#define DB_FLAG_DELETED  0x00000001

struct DBCHUNK
{
    int Magic = 0;
    int Size = 0;
    int Flags = 0;
    int Padding = 0;
};

//
// CfxDatabase
//
class CfxDatabase: public CfxPersistent
{
protected:
    char *_fileName = nullptr;
    FXFILEMAP _fileMap;
    int _index = 0;
    DBID _currId = 0, _nextId = 0;
    int _cacheCount = 0;
    bool Map();
    void Unmap();
    bool DeleteDB();
    DBCHUNK *FindFirst();
    DBCHUNK *FindNext();
public:
    CfxDatabase(CfxPersistent *pOwner, char *pFileName);
    virtual ~CfxDatabase();

    char *GetFileName();

    bool Verify();

    bool Read(DBID Id, void **ppBuffer);
    bool Write(DBID *pId, void *pBuffer, int Size);

    bool Delete(DBID Id);
    bool Enum(int Index, DBID *pId);
    bool TestId(DBID Id);

    bool GetIndex(DBID Id, int *pIndex);
    int GetCount();

    bool DeleteAll();

    bool Full()    { return false; }
};
