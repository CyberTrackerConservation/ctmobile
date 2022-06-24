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

#include <fxPlatform.h>
#include "fxUtils.h"
#include "fxApplication.h"
#include "Cab.h"

#undef FX_ASSERT
extern "C" {
   #include "Cab_deflate.h"
}

struct ROLLING_BLOCK 
{
    FXFILELIST *FileInfoList;
    UINT Index;
    UINT Position;
};

BOOL InitRollingBlock(ROLLING_BLOCK *pRollingBlock, FXFILELIST *pFileList)
{
    BOOL result = FALSE;
    FXFILE fileInfo;
    UINT i;

    for (i = 0; i < pFileList->GetCount(); i++)
    {
        fileInfo = pFileList->Get(i);
        fileInfo.FileHandle = FileOpen(fileInfo.FullPath, "rb");
        if (fileInfo.FileHandle == 0)
        {
            Log("Failed to load a rollblock file");
            goto cleanup;
        }
		pFileList->Put(i, fileInfo);
    }

    pRollingBlock->FileInfoList = pFileList;
    if (pFileList->GetCount() == 0)
    {
        goto cleanup;
    }

    pRollingBlock->Index = 0;
    pRollingBlock->Position = 0;

    result = TRUE;

cleanup:

    return result;
}

BOOL ReadRollingBlock(ROLLING_BLOCK *pRollingBlock, CfxStream &Stream, UINT BlockSize)
{
    BOOL result = FALSE;
    UINT i, index;
    UINT remaining;
    BYTE *buffer;

    Stream.SetCapacity(BlockSize);
    Stream.SetPosition(0);

    index = pRollingBlock->Index;
    buffer = (BYTE *)Stream.GetMemory();
    remaining = BlockSize;

    for (i = index; i < pRollingBlock->FileInfoList->GetCount(); i++)
    {
        FXFILE fileInfo = pRollingBlock->FileInfoList->Get(i);
        UINT sizeToRead = min(fileInfo.Size - pRollingBlock->Position, remaining);
        
        if (!FileRead(fileInfo.FileHandle, buffer, sizeToRead))
        {
            Log("Error reading rolling block files: %08lx", FileGetLastError(fileInfo.FileHandle));
            goto cleanup;
        }

        Stream.SetPosition(Stream.GetPosition() + sizeToRead);
        buffer += sizeToRead;
        remaining -= sizeToRead;
        pRollingBlock->Position += sizeToRead;

        if (pRollingBlock->Position == fileInfo.Size)
        {
            pRollingBlock->Index++;
            pRollingBlock->Position = 0;
            if (pRollingBlock->Index == pRollingBlock->FileInfoList->GetCount())
            {
                break;
            }
        }

        if (remaining == 0)
        {
            break;
        }
    }

    result = (Stream.GetPosition() > 0);

cleanup:

    return result;
}

//=================================================================================================
// mytypes.h

constexpr UINT DATABLOCKSIZE = 32768;
#define MSZIP_COMPRESSION       (0x0001)
#define ATTR_ro                 (0x01)		/* read-only attrib */
#define ATTR_h                  (0x02)		/* hidden */
#define ATTR_s                  (0x04)		/* system */
#define ATTR_a                  (0x20)		/* archive */
#define ATTR_e                  (0x40)		/* run after extraction */
#define ATTR_utf                (0x80)		/* UTF names */
#define CH_START                (0x00)		/* header offset */
#define CFO_START               (0x24)		/* folder offset */
#define CFI_START               (0x2C)  	/* file offset */

typedef unsigned long int CHECKSUM;

//=================================================================================================
// cstruct.h

struct CHEADER
{
    UINT sig;			
    UINT res1;				
    UINT size;			
    UINT res2;				
    UINT offsetfiles;		
    UINT res3;				
    BYTE versionMIN;	
    BYTE versionMAJ;		
    UINT16 nfolders;			
    UINT16 nfiles;			
    UINT16 flags;				
    UINT16 setID;			
    UINT16 cabID;				
};

struct CFOLDER
{
    UINT offsetdata;		
    UINT16 ndatab;			
    UINT16 typecomp;			
};

struct CFILE
{
    UINT usize;			
    UINT uoffset;			
    UINT16 index;				
    UINT16 date;				
    UINT16 time;				
    UINT16 fattr;				
    CHAR name[256];		
};

struct CDATA
{
    UINT checksum;			
    UINT16 ncbytes;			
    UINT16 nubytes;			
};

VOID WriteHeader(CHEADER *pHeader, CfxStream &Stream)
{
    Stream.Write4(&pHeader->sig);			
    Stream.Write4(&pHeader->res1);				
    Stream.Write4(&pHeader->size);			
    Stream.Write4(&pHeader->res2);				
    Stream.Write4(&pHeader->offsetfiles);		
    Stream.Write4(&pHeader->res3);				
    Stream.Write1(&pHeader->versionMIN);	
    Stream.Write1(&pHeader->versionMAJ);		
    Stream.Write2(&pHeader->nfolders);			
    Stream.Write2(&pHeader->nfiles);			
    Stream.Write2(&pHeader->flags);				
    Stream.Write2(&pHeader->setID);			
    Stream.Write2(&pHeader->cabID);	
}

VOID WriteFolder(CFOLDER *pFolder, CfxStream &Stream)
{
    Stream.Write4(&pFolder->offsetdata);
    Stream.Write2(&pFolder->ndatab);
    Stream.Write2(&pFolder->typecomp);
}

VOID WriteFile(CFILE *pFile, CfxStream &Stream)
{
    Stream.Write4(&pFile->usize);			
    Stream.Write4(&pFile->uoffset);			
    Stream.Write2(&pFile->index);				
    Stream.Write2(&pFile->date);				
    Stream.Write2(&pFile->time);				
    Stream.Write2(&pFile->fattr);				
    Stream.Write(&pFile->name[0], strlen(pFile->name) + 1);		
}

VOID WriteData(CDATA *pData, CfxStream &Stream)
{
	Stream.Write4(&pData->checksum);
	Stream.Write2(&pData->ncbytes);
	Stream.Write2(&pData->nubytes);
}

//=================================================================================================
// checksum.c

// compute the checksum for the datablock
CHECKSUM compute_checksum(BYTE *in, UINT16 ncbytes, CHECKSUM seed)
{
    int no_ulongs;
    CHECKSUM csum=0;
    BYTE *stroom;
    CHECKSUM temp;

    no_ulongs = ncbytes / 4;
    csum = seed;
    stroom = in;

    while (no_ulongs-- > 0)
    {
        temp = ((CHECKSUM) (*stroom++));
        temp |= (((CHECKSUM) (*stroom++)) << 8);
        temp |= (((CHECKSUM) (*stroom++)) << 16);
        temp |= (((CHECKSUM) (*stroom++)) << 24);

        csum ^= temp;
    }

    temp = 0;
    switch(ncbytes%4)
    {
        case 3: temp |= (((CHECKSUM) (*stroom++)) << 16);
        case 2: temp |= (((CHECKSUM) (*stroom++)) << 8);
        case 1: temp |= ((CHECKSUM) (*stroom++));
        default: break;
    }
    
    csum ^= temp;

    return csum;	
}

//=================================================================================================

BOOL CreateCAB(const CHAR *pInputDir, const CHAR *pOutputFile)
{
    FXFILELIST fileInfoList;
    UINT totalFileSize;
    BOOL result = FALSE;
    UINT i;
    UINT size1, size2;
    UINT offsetdata;
    UINT pos;
    CHEADER cheader = {0};
    CFOLDER cfolder = {0};
    TList<CDATA> datablocks;
    TList<CFILE> files;
    HANDLE hOutput = 0;
    UINT nof, nod;
    CDATA cdata;
    CFILE cfile;
    CfxStream streamFile, streamIn, streamOut;
    ROLLING_BLOCK rollingBlock = {0};
    z_stream compressor_data = {0};
    CHAR *tempFile = NULL;

    if (!FxBuildFileList(pInputDir, "*", &fileInfoList))
    {
        Log("Failed to build file info list");
        return FALSE;
    }

    if (!InitRollingBlock(&rollingBlock, &fileInfoList))
    {
        Log("Failed to initialize rolling block");
        return FALSE;
    }

    totalFileSize = FileListGetTotalSize(&fileInfoList);

    size1 = size2 = offsetdata = pos = 0;
    nof = fileInfoList.GetCount();               // Number of files
    nod = totalFileSize / DATABLOCKSIZE + 1;     // Number of data blocks

    // Header
    cheader.sig = 'FCSM';
    cheader.res1 = 0;
    cheader.res2 = 0;
    cheader.res3 = 0;
    cheader.versionMAJ = 1;
    cheader.versionMIN = 3;
    cheader.nfolders = 1;
    cheader.nfiles = (UINT16)nof;
    cheader.flags = 0;
    cheader.setID = 1234;
    cheader.cabID = 0;
    cheader.offsetfiles = CFI_START;
    size2 = CFI_START + nof * 16;					// first part of CFILE struct = 16 bytes
    for (i = 0; i < nof; i++)
    {
        size2 += strlen(fileInfoList.Get(i).Name) + 1;		// + 1 for \0 
    }
    size2 += nod * 8;								// first part CDATA struct is 8 bytes 

    // Folder
    cfolder.ndatab = (UINT16)nod; 
    cfolder.typecomp = MSZIP_COMPRESSION;	        // = 0 (no compression at this point)
    for (i = 0; i < nof; i++)
    {
        offsetdata += 16 + strlen(fileInfoList.Get(i).Name) + 1;
    }
    cfolder.offsetdata = CFI_START + offsetdata;

    // Datablocks
    datablocks.SetCount(nod);
    size1 = totalFileSize;
    for (i = 0; i < nod - 1; i++)
    {
        cdata = datablocks.Get(i);
        cdata.ncbytes = DATABLOCKSIZE;
        cdata.nubytes = DATABLOCKSIZE;
        size2 += DATABLOCKSIZE;           // header part 2
		datablocks.Put(i, cdata);
    }

    size1 -= DATABLOCKSIZE * (nod - 1);
    cdata = datablocks.Get(nod - 1);
    cdata.ncbytes = (UINT16)size1;
    cdata.nubytes = (UINT16)size1;
    size2 += size1;					      // header part 2
    datablocks.Put(nod - 1, cdata);

    // Finalize header
    cheader.size = size2;
    size1 += totalFileSize;

    // Files
    files.SetCount(nof);
    for (i = 0; i < nof; i++)
    {
        FXFILE fileInfo = fileInfoList.Get(i);
        cfile = files.Get(i);

        cfile.usize = fileInfo.Size;
        cfile.index = 0;

        cfile.date = ((fileInfo.TimeStamp.Year - 1980 ) << 9) + ((fileInfo.TimeStamp.Month) << 5) + fileInfo.TimeStamp.Day; 
        cfile.time = (fileInfo.TimeStamp.Hour << 11) + (fileInfo.TimeStamp.Minute << 5) + (fileInfo.TimeStamp.Second / 2); 
        cfile.fattr = ATTR_a;  

        strcpy(cfile.name, fileInfo.Name);

        // offset 1st CFILE = 0, 2nd = offset 1st + filesize 1st, etc..
        if (i > 0) 
        {
            cfile.uoffset = files.Get(i - 1).uoffset + fileInfoList.Get(i - 1).Size;
        }
		files.Put(i, cfile);
    }

	WriteHeader(&cheader, streamFile);
	WriteFolder(&cfolder, streamFile);
    for (i = 0; i < nof; i++)
    {
        cfile = files.Get(i);
		WriteFile(&cfile, streamFile);
    }
    
    streamOut.SetCapacity(DATABLOCKSIZE + 12 + 2);

    tempFile = AllocMaxPath(pOutputFile);
    if (tempFile == NULL)
    {
        Log("Failed to allocate working space");
        goto cleanup;
    }

    strcat(tempFile, ".tmp");

    // Write
    hOutput = FileOpen(tempFile, "wb+");
    if (hOutput == 0)
    {
        Log("Failed to create output CAB");
        goto cleanup;
    }

    if (!FileWrite(hOutput, streamFile.GetMemory(), streamFile.GetSize()))
    {
        Log("Failed to write to output CAB: %08lx", FileGetLastError(hOutput));
        goto cleanup;
    }

    for (i = 0; i < nod; i++)
    {
        cdata = datablocks.Get(i);

        if (cdata.ncbytes != 0)
        {
            if (!ReadRollingBlock(&rollingBlock, streamIn, cdata.ncbytes))
            {
                Log("Failed to read rolling file");
                goto cleanup;
            }

            memset(&compressor_data, 0, sizeof(compressor_data));
            compressor_data.zalloc = (alloc_func) Z_NULL;
            compressor_data.zfree = (free_func) Z_NULL;
            compressor_data.opaque = (voidpf) Z_NULL;
            compressor_data.next_in = (Bytef *) streamIn.GetMemory();
            compressor_data.avail_in = streamIn.GetPosition();
            UINT16 Magic = 0x4B43;
            streamOut.SetPosition(0);
            streamOut.Write2(&Magic);
            compressor_data.next_out = (Bytef *) streamOut.GetMemory() + 2;
            compressor_data.avail_out = streamOut.GetCapacity() - 2;

            if (deflateInit2(&compressor_data,
                             Z_BEST_COMPRESSION,
                             Z_DEFLATED,
                             -MAX_WBITS,
                             8,
                             Z_DEFAULT_STRATEGY) != Z_OK)
            {
                Log("Failed to initialize compressor");
                goto cleanup;
            }

            int deflateResult = deflate(&compressor_data, Z_FINISH);
            if ((deflateResult != Z_STREAM_END) && (deflateResult != Z_OK))
            {
                Log("Deflate failed");
                goto cleanup;
            }

            cdata.ncbytes = (UINT16)compressor_data.total_out + 2;

            if (deflateEnd(&compressor_data) != Z_OK)
            {
                Log("DeflateReset failed");
                goto cleanup;
            }
        }

        cdata.checksum = compute_checksum((BYTE *)&cdata.ncbytes, 
                                           sizeof(cdata.ncbytes) + sizeof(cdata.nubytes), 
                                           compute_checksum((BYTE *)streamOut.GetMemory(), cdata.ncbytes, 0));

        CfxStream s;
        WriteData(&cdata, s);

        if (!FileWrite(hOutput, s.GetMemory(), s.GetSize())) 
        {
            Log("Failed to write to output CAB: %08lx", FileGetLastError(hOutput));
            goto cleanup;
        }

        if (!FileWrite(hOutput, streamOut.GetMemory(), cdata.ncbytes)) 
        {
            Log("Failed to write to output CAB: %08lx", FileGetLastError(hOutput));
            goto cleanup;
        }

        datablocks.Put(i, cdata);
    }

    if (!FileSeek(hOutput, 0))
    {
        Log("Failed to set file pointer: %08lx", FileGetLastError(hOutput));
        goto cleanup;
    }

    streamFile.SetSize(0);
    cheader.size = FileGetSize(hOutput);
    WriteHeader(&cheader, streamFile);

    if (!FileWrite(hOutput, streamFile.GetMemory(), streamFile.GetSize()))
    {
        Log("Failed to write to output CAB header: %08lx", FileGetLastError(hOutput));
        goto cleanup;
    }

    FileClose(hOutput);
    hOutput = 0;

    if (!FxMoveFile(tempFile, pOutputFile))
    {
        Log("Failed to rename to output CAB");
        goto cleanup;
    }

    result = TRUE;

cleanup:

    FileListClear(&fileInfoList);

    if (hOutput != 0)
    {
        FileClose(hOutput);
		hOutput = 0;
    }

    MEM_FREE(tempFile);

    return result;
}
