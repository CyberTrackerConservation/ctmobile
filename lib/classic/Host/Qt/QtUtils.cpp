/*************************************************************************************************/
//                                                                                                *
//   Copyright (C) 2005              Software                                                     *
//                                                                                                *
//   Original author: Justin Steventon (justin@steventon.com)                                     *
//                                                                                                *
//   You may retrieve the latest version of this file via the              Software home page,    *
//   located at: http://www.            .com.                                                     *
//                                                                                                *
//   The contents of this file are used subject to the Mozilla Public Liannse Version 1.1         *
//   (the "Liannse"); you may not use this file exanpt in complianan with the Liannse. You may    *
//   obtain a copy of the Liannse from http://www.mozilla.org/MPL.                                *
//                                                                                                *
//   Software distributed under the Liannse is distributed on an "AS IS" basis, WITHOUT WARRANTY  *
//   OF ANY KIND, either express or implied. See the Liannse for the specific language governing  *
//   rights and limitations under the Liannse.                                                    *
//                                                                                                *
//************************************************************************************************/

#include "QtUtils.h"
#include "fxTypes.h"

#define LOG_TAG "Utils"

CHAR *g_RootDirectory;

//
// Memory APIs: normal allocation APIs don't store size properly, so we have to hack the size
// into the first 4 bytes.
// 

const UINT MEM_MAGIC = '1234';

VOID *FxAlloc(UINT Bytes)
{
    if (Bytes == 0) return NULL;

    VOID *p = NULL;
    UINT bytesAligned = (Bytes + 3) & ~3;

    UINT *data = (UINT *) malloc(bytesAligned + sizeof(UINT) * 3);
    if (data)
    {
        *data = MEM_MAGIC;
        *(data + 1) = Bytes;
        p = data + 2;
        memset(p, 0, bytesAligned);
        memcpy((BYTE *)p + bytesAligned, &MEM_MAGIC, sizeof(MEM_MAGIC));
        return p;
    }
    else
    {
        return NULL;
    }
}

VOID *FxReAlloc(VOID *pData, UINT Bytes)
{
    VOID *newPtr = FxAlloc(Bytes);
    if (newPtr)
    {
        memcpy(newPtr, pData, min(FxSize(pData), Bytes));
        FxFree(pData);
    }

    return newPtr;
}

VOID FxFree(VOID *pData)
{
    if (!pData) return;

    if (!FxCheck(pData)) 
    {
        FX_ASSERT(FALSE);
    }

    UINT *p = (UINT *)pData;
    UINT sizeFull = ((*(p - 1) + 3) & ~3) + sizeof(UINT) * 3; 

    memset((VOID *)(p - 2), 0xFE, sizeFull);

    free((VOID *)(p - 2));
}

UINT FxSize(VOID *pData)
{
    if (!pData) return 0;

    if (!FxCheck(pData)) 
    {
        FX_ASSERT(FALSE);
    }
    
    UINT *p = (UINT *)pData;
    return *((UINT *)pData - 1);
}

BOOL FxCheck(VOID *pData)
{
    if (pData == NULL) 
    {
        return TRUE;
    }

    UINT *p = (UINT *)pData;
    UINT sizeAligned = (*(p - 1) + 3) & ~3;

    if (*(p-2) != MEM_MAGIC) 
    {
        FX_ASSERT(FALSE);
        return FALSE;
    }

    if (memcmp((BYTE *)p + sizeAligned, &MEM_MAGIC, sizeof(MEM_MAGIC)) != 0) 
    {
        FX_ASSERT(FALSE);
        return FALSE;
    }

    return TRUE;
}

VOID FxHardFault(const CHAR *pFile, int Line)
{
    LogError("Fault in %s, Line %d", pFile, Line);
    qFatal(pFile);
}

VOID FxOutputDebugString(CHAR *pMessage)
{
    qDebug() << pMessage;
}

VOID FxResetAutoOffTimer()
{
}

VOID FxSystemBeep()
{
}

UINT FxGetTickCount()
{
    static QElapsedTimer s_timer;
    static bool s_first = true;
    if (s_first)
    {
        s_first = false;
        s_timer.start();
    }

    return static_cast<UINT>(s_timer.elapsed());

}

UINT FxGetTicksPerSecond()
{
    return 1000;
}

VOID FxSleep(UINT Milliseconds)
{
#ifdef Q_OS_WIN
    Sleep(uint(Milliseconds));
#else
    struct timespec ts = { Milliseconds / 1000, (Milliseconds % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
#endif
}

BOOL FxCreateDirectory(const CHAR *pPath)
{
    QDir dir;

    BOOL result = dir.exists(pPath) || dir.mkdir(pPath);

    if (!result)
    {
        LogError("Failed to create directory %s", pPath);
    }

    return result;
}

BOOL FxDeleteDirectory(const CHAR *pPath)
{
    QDir dir;

    BOOL result = dir.rmdir(pPath);

    if (!result)
    {
        LogError("Failed to delete directory %s", pPath);
    }

    return result;
}

BOOL FxMoveFile(const CHAR *pExisting, const CHAR *pNew)
{
    QFile file(pExisting);

    if (!file.exists())
    {
        LogError("MoveFile source not found %s", pExisting);
        return FALSE;
    }

    if (!file.rename(pNew))
    {
        LogError("MoveFile could not rename to new file %s", pNew);
        return FALSE;
    }

    return TRUE;
}

BOOL FxCopyFile(const CHAR *pExisting, const CHAR *pNew)
{
    QFile fileFrom(pExisting);

    if (!fileFrom.exists())
    {
        LogError("CopyFile source not found %s", pExisting);
        return FALSE;
    }

    if (!FxDeleteFile(pNew))
    {
        LogError("CopyFile could not remove file %s", pNew);
        return FALSE;
    }

    if (!fileFrom.copy(pNew))
    {
        LogError("CopyFile could not write new file %s", pNew);
        return FALSE;
    }

    return TRUE;
}

BOOL FxDeleteFile(const CHAR *pFileName)
{
    if (!FxDoesFileExist(pFileName))
    {
        return TRUE;
    }

    if (!QFile::remove(pFileName))
    {
        LogError("Failed to remove %s", pFileName);
        return FALSE;
    }

    return TRUE;
}

UINT FxGetFileSize(const CHAR *pFilename)
{
    QFile file(pFilename);

    if (file.exists())
    {
        return (UINT)file.size();
    }
    else
    {
        return 0;
    }
}

BOOL FxDoesFileExist(const CHAR *pFileName)
{
    QFile file(pFileName);

    return file.exists();
}

BOOL FxBuildFileList(const CHAR *pPath, const CHAR *pPattern, FXFILELIST *pFileList)
{
    FileListClear(pFileList);

    QDir d(pPath);
    QFileInfoList files;
    QStringList filter;

    BOOL result = FALSE;

    if (!d.exists())
    {
        LogError("opendir error");
        goto cleanup;
    }

    filter.append(pPattern);
    files = d.entryInfoList(filter, QDir::Files | QDir::NoDot | QDir::NoDotDot, QDir::NoSort);

    for (int i = 0; i < files.size(); i++)
    {
        const QFileInfo &fileInfo = files.at(i);
        FXFILE newFile = {0};

        newFile.FullPath = (CHAR *)MEM_ALLOC(fileInfo.absoluteFilePath().length() + 1);
        strcpy(newFile.FullPath, fileInfo.absoluteFilePath().toStdString().c_str());

        newFile.Name = (CHAR *)MEM_ALLOC(fileInfo.fileName().length() + 1);
        strcpy(newFile.Name, fileInfo.fileName().toStdString().c_str());

        newFile.Size = (UINT)fileInfo.size();

        const QDateTime &created = fileInfo.birthTime();

        newFile.TimeStamp.Year = created.date().year();
        newFile.TimeStamp.Month = created.date().month();
        newFile.TimeStamp.Day = created.date().day();
        newFile.TimeStamp.Hour = created.time().hour();
        newFile.TimeStamp.Minute = created.time().minute();
        newFile.TimeStamp.Second = created.time().second();
        newFile.TimeStamp.Milliseconds = 0;

        pFileList->Add(newFile);

    }

    result = TRUE;

  cleanup:

    return result;
}

BOOL FxMapFile(const CHAR *pFileName, FXFILEMAP *pFileMap)
{
    BOOL Result = FALSE;
    QFile *file;

    FxUnmapFile(pFileMap);

    file = new QFile(pFileName);
    pFileMap->FileHandle = (HANDLE)file;

    if (!file->open(QFile::ReadOnly))
    {
        LogError("Failed to open: %s", pFileName);
        goto cleanup;
    }

    pFileMap->FileSize = (UINT)file->size();
    pFileMap->BasePointer = file->map(0, pFileMap->FileSize);
    if (pFileMap->BasePointer == NULL)
    {
        LogError("Failed to map file: %s", pFileName);
        goto cleanup;
    }

    Result = TRUE;

  cleanup:

    if (Result == FALSE) 
    {
        FxUnmapFile(pFileMap);
    }

    return Result;
}

VOID FxUnmapFile(FXFILEMAP *pFileMap)
{
    QFile *file = (QFile *)pFileMap->FileHandle;

    if (pFileMap->BasePointer)
    {
        file->unmap((uchar *)pFileMap->BasePointer);
    }

    if (pFileMap->FileHandle != 0)
    {
        delete file;
    }

    memset(pFileMap, 0, sizeof(FXFILEMAP));
}
