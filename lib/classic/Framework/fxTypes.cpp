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

#include "fxTypes.h"
#include "fxUtils.h"

#define MEMORY_DELTA 0x0100UL // must be a power of 2

const GUID  GUID_0  = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

#ifdef CLIENT_DLL
    const XGUID XGUID_0 = GUID_0;
#else
    const XGUID XGUID_0 = 0;
#endif

//*************************************************************************************************

VOID PrintF(CHAR* S, DOUBLE Value, UINT16 Size, UINT16 Precision) 
{ 
    S[0] = 0; 
    
    // Get integer portion 
    sprintf(S, "%ld.", (INT)Value); 

    // Check the length of the integer portion, including dot 
    UINT16 len = strlen(S); 
    if (len > Size - 1)
    {
        return;
    }

    // Make sure we don't over run iSize characters 
    INT16 i = Size - len; 
    if (i > Precision) 
    {
        i = Precision; 
    }

    INT power = 1;
    for (INT16 k=0; k<i; k++) power*=10;
    
    INT rem = (INT)((Value - (INT)Value) * power);
    if (rem < 0)
    {
        rem = -rem;
    } 

    // Determine length of Remainder and glue zeros in front if necessary 
    sprintf(S, "%ld", rem); 
    len = strlen(S); 


    sprintf(S, "%ld.", (INT)Value); 
    for (INT16 j=0; j<i-len; j++) 
    {
        strcat(S, "0"); 
    }

    // Add in remainder 
    sprintf(&S[strlen(S)], "%lu", (INT)rem); 
} 

//*************************************************************************************************
// CfxStream

VOID *CfxStream::ReAlloc(UINT *pNewCapacity)
{
    FX_ASSERT(_ownsMemory);

    VOID *pResult;

    if ((*pNewCapacity > 0) && (*pNewCapacity != _size)) 
    {
        *pNewCapacity = (*pNewCapacity + (MEMORY_DELTA - 1)) & ~(MEMORY_DELTA - 1);
    }

    pResult = _memory;

    if (*pNewCapacity != _capacity) 
    {
        if (*pNewCapacity == 0) 
        {
            MEM_FREE(_memory);
            pResult = NULL;
        } 
        else 
        {
            if (_capacity == 0) 
            {
                pResult = MEM_ALLOC(*pNewCapacity);
            } 
            else 
            {
                pResult = MEM_REALLOC(pResult, *pNewCapacity);
            }

            FX_ASSERT(pResult != NULL);
        }
    }

    return pResult;
}

VOID CfxStream::SetMemory(VOID *pMemory, UINT MemorySize)
{
    _memory = pMemory;
    _size = MemorySize;
}

CfxStream::CfxStream()
{
    _ownsMemory = TRUE;
    _memory = NULL;
    _size = _position = _capacity = 0;
}

CfxStream::CfxStream(VOID *pMemory, UINT MemorySize)
{
    _ownsMemory = FALSE;
    _memory = pMemory;
    _size = MemorySize;
    _capacity = MemorySize;
    _position = 0;
}
    
CfxStream::~CfxStream()
{
    Clear();
}
    
VOID CfxStream::Clear() 
{
    if (!_ownsMemory) return;

    SetCapacity(0);
    _size = _position = 0;
}
    
VOID CfxStream::Read(VOID *pBuffer, UINT Count)
{
    if (Count == 0) return;

    FX_ASSERT(_position + Count <= _size);
    memmove(pBuffer, (BYTE *) _memory + _position, Count);
    _position += Count;
}
            
VOID CfxStream::Write(VOID *pBuffer, UINT Count) 
{
    if (Count == 0) return;

    UINT pos = _position + Count;
 
    if (pos > _size) 
    {
        if (pos > _capacity) 
        {
            SetCapacity(pos);
        }
        _size = pos;
    }
    memmove((BYTE *)_memory + _position, pBuffer, Count);
    _position = pos;
}

VOID CfxStream::Read1(VOID *pBuffer)
{
    Read(pBuffer, 1);
}

VOID CfxStream::Read2(VOID *pBuffer)
{
    Read(pBuffer, 2);
    //GET16(pBuffer, (BYTE *)_memory + _position);
    //_position += 2;
}

VOID CfxStream::Read4(VOID *pBuffer)
{
    Read(pBuffer, 4);
    //GET32(pBuffer, (BYTE *)_memory + _position);
    //_position += 4;
}

VOID CfxStream::Read8(VOID *pBuffer)
{
    Read(pBuffer, 8);
    //GET64(pBuffer, (BYTE *)_memory + _position);
    //_position += 8;
}

VOID CfxStream::Write1(VOID *pBuffer)
{
    Write(pBuffer, 1);
}

VOID CfxStream::Write2(VOID *pBuffer)
{
    Write(pBuffer, 2);
    //BYTE value[2];
    //GET16(&value[0], pBuffer);
    //Write(&value[0], 2);
}

VOID CfxStream::Write4(VOID *pBuffer)
{
    Write(pBuffer, 4);
    //BYTE value[4];
    //GET32(&value[0], pBuffer);
    //Write(&value[0], 4);
}

VOID CfxStream::Write8(VOID *pBuffer)
{
    Write(pBuffer, 8);
    //BYTE value[8];
    //GET64(&value[0], pBuffer);
    //Write(&value[0], 8);
}

VOID CfxStream::Write(const CHAR *pString)
{
    // Write the string and null terminator, then backup to before the terminator.
    // This means the raw buffer will always be a null terminated string.
    Write((VOID *)pString, strlen(pString) + 1);
    _position--;
}

VOID CfxStream::Peek(VOID *pBuffer, UINT Count)
{
    Read(pBuffer, Count);
    _position -= Count;
}

VOID CfxStream::LoadFromStream(CfxStream &Stream)
{
    UINT count;
    Stream._position = 0;
    count = Stream.GetSize();
    SetSize(count);
    if (count) 
    {
        Stream.Read(_memory, count);
    }
}

VOID CfxStream::SaveToStream(CfxStream &Stream)
{
    if (_size) 
    {
        Stream.Write(_memory, _size);
    }
}

VOID CfxStream::SetPosition(UINT NewPosition)
{
    _position = NewPosition;
}

VOID CfxStream::SetCapacity(UINT NewCapacity)
{
    SetMemory(ReAlloc(&NewCapacity), _size);
    _capacity = NewCapacity;
}

VOID CfxStream::SetSize(UINT NewSize)
{
    UINT oldPosition = _position;
    SetCapacity(NewSize);
    _size = NewSize;
    if (oldPosition > NewSize) 
    {
        _position = _size;
    }
}

VOID *CfxStream::CloneMemory()
{ 
    VOID *memory = MEM_ALLOC(_size); 
    if (memory)
    {
        memcpy(memory, _memory, _size); 
    }
    
    return memory; 
}

//*************************************************************************************************
// CfxFileStream

CfxFileStream::CfxFileStream()
{
    _fileHandle = 0;
    _position = 0;
    memset(_fileName, 0, sizeof(_fileName));
}

CfxFileStream::~CfxFileStream()
{
    Close();
}

VOID CfxFileStream::Close()
{
    if (_fileHandle)
    {
        FileClose(_fileHandle);
        _fileHandle = 0;
    }

    _position = 0;
    memset(_fileName, 0, sizeof(_fileName));
}

BOOL CfxFileStream::OpenForRead(CHAR *pFileName)
{
    Close();
    strncpy(_fileName, pFileName, ARRAYSIZE(_fileName));
    _fileHandle = FileOpen(_fileName, "rb");
    return _fileHandle != 0;
}

BOOL CfxFileStream::OpenForWrite(CHAR *pFileName)
{
    Close();
    strncpy(_fileName, pFileName, ARRAYSIZE(_fileName));
    _fileHandle = FileOpen(_fileName, "wb");
    return _fileHandle != 0;
}

BOOL CfxFileStream::Read(VOID *pBuffer, UINT Count)
{
    BOOL result = _fileHandle && FileRead(_fileHandle, pBuffer, Count);
    if (result)
    {
        _position += Count;
    }

    return result;
}

BOOL CfxFileStream::Write(VOID *pBuffer, UINT Count)
{
    BOOL result = _fileHandle && FileWrite(_fileHandle, pBuffer, Count);
    if (result)
    {
        _position += Count;
    }

    return result;
}

BOOL CfxFileStream::Write(const CHAR *pString)
{
    return Write((VOID *)pString, strlen(pString));
}

BOOL CfxFileStream::WriteNullChar()
{
    CHAR nullChar = 0;
    return Write(&nullChar, 1);
}

BOOL CfxFileStream::Seek(UINT Position)
{
    BOOL result = _fileHandle && FileSeek(_fileHandle, Position);
    if (result)
    {
        _position = Position;
    }

    return result;
}

UINT CfxFileStream::GetSize()
{
    if (_fileHandle)
    {
        return FileGetSize(_fileHandle);
    }

    return 0;
}

UINT CfxFileStream::GetLastError()
{
    if (_fileHandle)
    {
        return FileGetLastError(_fileHandle);
    }
    else
    {
        return (UINT)-1;
    }
}

//*************************************************************************************************
// CfxString

CfxString::CfxString()
{
    _string = NULL;
}

CfxString::CfxString(const CHAR *pValue, UINT Count)
{
    if (pValue)
    {
        UINT size = ((Count == 0) ? (UINT) strlen(pValue) : Count) + 1;
        _string = (CHAR *) MEM_ALLOC(size);
        memset(_string, 0, size);
        memcpy(_string, pValue, size-1);
    }
    else
    {
        _string = AllocString("");
    }
}

CfxString::~CfxString()
{
    MEM_FREE(_string);
    _string = NULL;
}

CHAR *CfxString::Get()
{
    return _string;          
}

VOID CfxString::Set(const CHAR *pValue, UINT Count)
{
    MEM_FREE(_string);
    _string = NULL;

    if (pValue)
    {
        UINT size = ((Count == 0) ? (UINT) strlen(pValue) : Count) + 1;
        _string = (CHAR *) MEM_ALLOC(size);
        memset(_string, 0, size);
        memcpy(_string, pValue, size-1);
    }
    else
    {
        _string = AllocString("");
    }
}

BOOL CfxString::Compare(const CHAR *pValue)
{
    return stricmp(_string, pValue) == 0;
}

UINT CfxString::Length()
{
    return (UINT) strlen(_string);
}

//*************************************************************************************************
// Endian operations

VOID Get16(BYTE *pDst, BYTE *pSrc)
{
    memcpy(pDst, pSrc, sizeof(INT16));
}

VOID Get32(BYTE *pDst, BYTE *pSrc)
{
    memcpy(pDst, pSrc, sizeof(INT));
}

VOID Get64(BYTE *pDst, BYTE *pSrc)
{
    memcpy(pDst, pSrc, sizeof(DOUBLE));
}

VOID Swap16(BYTE *pDst, BYTE *pSrc)
{
    BYTE *ptr = pDst + 1;
    *ptr-- = *pSrc++;
    *ptr = *pSrc;
}

VOID Swap32(BYTE *pDst, BYTE *pSrc)
{
    BYTE *ptr = pDst + 3;
    *ptr-- = *pSrc++;
    *ptr-- = *pSrc++;
    *ptr-- = *pSrc++;
    *ptr = *pSrc;
}

VOID Swap64(BYTE *pDst, BYTE *pSrc)
{
    BYTE *ptr = pDst + 7;
    *ptr-- = *pSrc++;
    *ptr-- = *pSrc++;
    *ptr-- = *pSrc++;
    *ptr-- = *pSrc++;
    *ptr-- = *pSrc++;
    *ptr-- = *pSrc++;
    *ptr-- = *pSrc++;
    *ptr = *pSrc;
}

//*************************************************************************************************
// Types

typedef VOID (*_pfn_ToText)(VOID *pData, CfxString &String);
typedef VOID (*_pfn_FromText)(CHAR *pString, VOID *pData);
typedef VOID (*_pfn_ToStream)(VOID *pData, CfxStream &Stream);
typedef VOID (*_pfn_FromStream)(VOID *pData, CfxStream &Stream);
typedef DOUBLE (*_pfn_ToDouble)(VOID *pData);

struct DATATYPEINFO
{
    FXDATATYPE Type;
    INT StaticSize;
    _pfn_ToText FnToText;
    _pfn_FromText FnFromText;
    _pfn_ToStream FnToStream;
    _pfn_FromStream FnFromStream;
    _pfn_ToDouble FnToDouble;
};

VOID TypeNullToText(VOID *pData, CfxString &String);
VOID TypeNullFromText(CHAR *pString, VOID *pData);
VOID TypeNullToStream(VOID *pData, CfxStream &Stream);
VOID TypeNullFromStream(VOID *pData, CfxStream &Stream);

VOID TypeBooleanToText(VOID *pData, CfxString &String);
VOID TypeBooleanFromText(CHAR *pString, VOID *pData);
VOID TypeBooleanToStream(VOID *pData, CfxStream &Stream);
VOID TypeBooleanFromStream(VOID *pData, CfxStream &Stream);
DOUBLE TypeBooleanToDouble(VOID *pData);

VOID TypeByteToText(VOID *pData, CfxString &String);
VOID TypeByteFromText(CHAR *pString, VOID *pData);
VOID TypeByteToStream(VOID *pData, CfxStream &Stream);
VOID TypeByteFromStream(VOID *pData, CfxStream &Stream);
DOUBLE TypeByteToDouble(VOID *pData);

VOID TypeInt8ToText(VOID *pData, CfxString &String);
VOID TypeInt8FromText(CHAR *pString, VOID *pData);
VOID TypeInt8ToStream(VOID *pData, CfxStream &Stream);
VOID TypeInt8FromStream(VOID *pData, CfxStream &Stream);
DOUBLE TypeInt8ToDouble(VOID *pData);

VOID TypeInt16ToText(VOID *pData, CfxString &String);
VOID TypeInt16FromText(CHAR *pString, VOID *pData);
VOID TypeInt16ToStream(VOID *pData, CfxStream &Stream);
VOID TypeInt16FromStream(VOID *pData, CfxStream &Stream);
DOUBLE TypeInt16ToDouble(VOID *pData);

VOID TypeInt32ToText(VOID *pData, CfxString &String);
VOID TypeInt32FromText(CHAR *pString, VOID *pData);
VOID TypeInt32ToStream(VOID *pData, CfxStream &Stream);
VOID TypeInt32FromStream(VOID *pData, CfxStream &Stream);
DOUBLE TypeInt32ToDouble(VOID *pData);

VOID TypeDoubleToText(VOID *pData, CfxString &String);
VOID TypeDoubleFromText(CHAR *pString, VOID *pData);
VOID TypeDoubleToStream(VOID *pData, CfxStream &Stream);
VOID TypeDoubleFromStream(VOID *pData, CfxStream &Stream);
DOUBLE TypeDoubleToDouble(VOID *pData);

VOID TypeDateToText(VOID *pData, CfxString &String);
VOID TypeDateFromText(CHAR *pString, VOID *pData);
VOID TypeDateToStream(VOID *pData, CfxStream &Stream);
VOID TypeDateFromStream(VOID *pData, CfxStream &Stream);

VOID TypeTimeToText(VOID *pData, CfxString &String);
VOID TypeTimeFromText(CHAR *pString, VOID *pData);
VOID TypeTimeToStream(VOID *pData, CfxStream &Stream);
VOID TypeTimeFromStream(VOID *pData, CfxStream &Stream);

VOID TypeDateTimeToText(VOID *pData, CfxString &String);
VOID TypeDateTimeFromText(CHAR *pString, VOID *pData);
VOID TypeDateTimeToStream(VOID *pData, CfxStream &Stream);
VOID TypeDateTimeFromStream(VOID *pData, CfxStream &Stream);

VOID TypeColorToText(VOID *pData, CfxString &String);
VOID TypeColorFromText(CHAR *pString, VOID *pData);
VOID TypeColorToStream(VOID *pData, CfxStream &Stream);
VOID TypeColorFromStream(VOID *pData, CfxStream &Stream);

VOID TypeGuidToText(VOID *pData, CfxString &String);
VOID TypeGuidFromText(CHAR *pString, VOID *pData);
VOID TypeGuidToStream(VOID *pData, CfxStream &Stream);
VOID TypeGuidFromStream(VOID *pData, CfxStream &Stream);

VOID TypeFontToText(VOID *pData, CfxString &String);
VOID TypeFontFromText(CHAR *pString, VOID *pData);
VOID TypeFontToStream(VOID *pData, CfxStream &Stream);
VOID TypeFontFromStream(VOID *pData, CfxStream &Stream);

VOID TypeTextAnsiToText(VOID *pData, CfxString &String);
VOID TypeTextAnsiFromText(CHAR *pString, VOID *pData);
VOID TypeTextAnsiToStream(VOID *pData, CfxStream &Stream);
VOID TypeTextAnsiFromStream(VOID *pData, CfxStream &Stream);

VOID TypeTextUtf8ToText(VOID *pData, CfxString &String);
VOID TypeTextUtf8FromText(CHAR *pString, VOID *pData);
VOID TypeTextUtf8ToStream(VOID *pData, CfxStream &Stream);
VOID TypeTextUtf8FromStream(VOID *pData, CfxStream &Stream);

VOID TypeBinaryToText(VOID *pData, CfxString &String);
VOID TypeBinaryFromText(CHAR *pString, VOID *pData);
VOID TypeBinaryToStream(VOID *pData, CfxStream &Stream);
VOID TypeBinaryFromStream(VOID *pData, CfxStream &Stream);

VOID TypeGraphicToText(VOID *pData, CfxString &String);
VOID TypeGraphicFromText(CHAR *pString, VOID *pData);
VOID TypeGraphicToStream(VOID *pData, CfxStream &Stream);
VOID TypeGraphicFromStream(VOID *pData, CfxStream &Stream);

VOID TypeSoundToText(VOID *pData, CfxString &String);
VOID TypeSoundFromText(CHAR *pString, VOID *pData);
VOID TypeSoundToStream(VOID *pData, CfxStream &Stream);
VOID TypeSoundFromStream(VOID *pData, CfxStream &Stream);

VOID TypeGuidListToText(VOID *pData, CfxString &String);
VOID TypeGuidListFromText(CHAR *pString, VOID *pData);
VOID TypeGuidListToStream(VOID *pData, CfxStream &Stream);
VOID TypeGuidListFromStream(VOID *pData, CfxStream &Stream);

VOID TypePTextAnsiToText(VOID *pData, CfxString &String);
VOID TypePTextAnsiFromText(CHAR *pString, VOID *pData);
VOID TypePTextAnsiToStream(VOID *pData, CfxStream &Stream);
VOID TypePTextAnsiFromStream(VOID *pData, CfxStream &Stream);

VOID TypePTextUtf8ToText(VOID *pData, CfxString &String);
VOID TypePTextUtf8FromText(CHAR *pString, VOID *pData);
VOID TypePTextUtf8ToStream(VOID *pData, CfxStream &Stream);
VOID TypePTextUtf8FromStream(VOID *pData, CfxStream &Stream);

VOID TypePBinaryToText(VOID *pData, CfxString &String);
VOID TypePBinaryFromText(CHAR *pString, VOID *pData);
VOID TypePBinaryToStream(VOID *pData, CfxStream &Stream);
VOID TypePBinaryFromStream(VOID *pData, CfxStream &Stream);

VOID TypePGraphicToText(VOID *pData, CfxString &String);
VOID TypePGraphicFromText(CHAR *pString, VOID *pData);
VOID TypePGraphicToStream(VOID *pData, CfxStream &Stream);
VOID TypePGraphicFromStream(VOID *pData, CfxStream &Stream);

VOID TypePSoundToText(VOID *pData, CfxString &String);
VOID TypePSoundFromText(CHAR *pString, VOID *pData);
VOID TypePSoundToStream(VOID *pData, CfxStream &Stream);
VOID TypePSoundFromStream(VOID *pData, CfxStream &Stream);

VOID TypePGuidListToText(VOID *pData, CfxString &String);
VOID TypePGuidListFromText(CHAR *pString, VOID *pData);
VOID TypePGuidListToStream(VOID *pData, CfxStream &Stream);
VOID TypePGuidListFromStream(VOID *pData, CfxStream &Stream);

DATATYPEINFO DataTypeInfo[dtMax] = 
{
    { dtNull,      0,              TypeNullToText,      TypeNullFromText,      TypeNullToStream,      TypeNullFromStream,        NULL},
    { dtBoolean,   sizeof(BOOL),   TypeBooleanToText,   TypeBooleanFromText,   TypeBooleanToStream,   TypeBooleanFromStream,     TypeBooleanToDouble   },
    { dtByte,      sizeof(BYTE),   TypeByteToText,      TypeByteFromText,      TypeByteToStream,      TypeByteFromStream,        TypeByteToDouble   },
    { dtInt8,      sizeof(INT8),   TypeInt8ToText,      TypeInt8FromText,      TypeInt8ToStream,      TypeInt8FromStream,        TypeInt8ToDouble   },
    { dtInt16,     sizeof(INT16),  TypeInt16ToText,     TypeInt16FromText,     TypeInt16ToStream,     TypeInt16FromStream,       TypeInt16ToDouble  },
    { dtInt32,     sizeof(INT),  TypeInt32ToText,     TypeInt32FromText,     TypeInt32ToStream,     TypeInt32FromStream,       TypeInt32ToDouble  },
    { dtDouble,    sizeof(double), TypeDoubleToText,    TypeDoubleFromText,    TypeDoubleToStream,    TypeDoubleFromStream,      TypeDoubleToDouble },
    { dtDate,      sizeof(double), TypeDateToText,      TypeDateFromText,      TypeDateToStream,      TypeDateFromStream,        NULL},
    { dtTime,      sizeof(double), TypeTimeToText,      TypeTimeFromText,      TypeTimeToStream,      TypeTimeFromStream,        NULL},
    { dtDateTime,  sizeof(double), TypeDateTimeToText,  TypeDateTimeFromText,  TypeDateTimeToStream,  TypeDateTimeFromStream,    NULL},
    { dtColor,     sizeof(UINT), TypeColorToText,     TypeColorFromText,     TypeColorToStream,     TypeColorFromStream,       NULL},
    { dtGuid,      sizeof(GUID),   TypeGuidToText,      TypeGuidFromText,      TypeGuidToStream,      TypeGuidFromStream,        NULL},
    { dtFont,      sizeof(FXFONT), TypeFontToText,      TypeFontFromText,      TypeFontToStream,      TypeFontFromStream,        NULL},
    { dtTextAnsi,  -1,             TypeTextAnsiToText,  TypeTextAnsiFromText,  TypeTextAnsiToStream,  TypeTextAnsiFromStream,    NULL},
    { dtBinary,    -1,             TypeBinaryToText,    TypeBinaryFromText,    TypeBinaryToStream,    TypeBinaryFromStream,      NULL},
    { dtGraphic,   -1,             TypeGraphicToText,   TypeGraphicFromText,   TypeGraphicToStream,   TypeGraphicFromStream,     NULL},
    { dtSound,     -1,             TypeSoundToText,     TypeSoundFromText,     TypeSoundToStream,     TypeSoundFromStream,       NULL},
    { dtGuidList,  -1,             TypeGuidListToText,  TypeGuidListFromText,  TypeGuidListToStream,  TypeGuidListFromStream,    NULL},
    { dtTextUtf8,  -1,             TypeTextUtf8ToText,  TypeTextUtf8FromText,  TypeTextUtf8ToStream,  TypeTextUtf8FromStream,    NULL},
    { dtPTextAnsi, -1,             TypePTextAnsiToText, TypePTextAnsiFromText, TypePTextAnsiToStream, TypePTextAnsiFromStream,   NULL},
    { dtPBinary,   -1,             TypePBinaryToText,   TypePBinaryFromText,   TypePBinaryToStream,   TypePBinaryFromStream,     NULL},
    { dtPGraphic,  -1,             TypePGraphicToText,  TypePGraphicFromText,  TypePGraphicToStream,  TypePGraphicFromStream,    NULL},
    { dtPSound,    -1,             TypePSoundToText,    TypePSoundFromText,    TypePSoundToStream,    TypePSoundFromStream,      NULL},
    { dtPGuidList, -1,             TypePGuidListToText, TypePGuidListFromText, TypePGuidListToStream, TypePGuidListFromStream,   NULL},
    { dtPTextUtf8, -1,             TypePTextUtf8ToText, TypePTextUtf8FromText, TypePTextUtf8ToStream, TypePTextUtf8FromStream,   NULL},
};

UINT ReadType(CfxStream &Stream, FXDATATYPE ExpectedType, INT ExpectedSize)
{
    BYTE type;
    FXDATATYPE dataType;
    UINT result;

    Stream.Read1((VOID *)&type);
    dataType = (FXDATATYPE)type;

    FX_ASSERT(ExpectedType == dataType);

    Stream.Read4((VOID *)&result);
    FX_ASSERT((ExpectedSize < 0) || ((UINT)ExpectedSize == result));

    return result;
}

VOID WriteType(CfxStream &Stream, FXDATATYPE Type, UINT DataSize)
{
    BYTE type = Type;
    Stream.Write1((VOID *)&type);
    Stream.Write4((VOID *)&DataSize);
}

VOID WriteType(CfxFileStream &Stream, FXDATATYPE Type, UINT DataSize)
{
    BYTE type = Type;
    Stream.Write((VOID *)&type, 1);
    Stream.Write((VOID *)&DataSize, sizeof(UINT));
}

//*************************************************************************************************
// Null

VOID TypeNullToText(VOID *pData, CfxString &String)
{
    String.Set("");
}

VOID TypeNullFromText(CHAR *pString, VOID *pData)
{
}

VOID TypeNullToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtNull, 0);
}

VOID TypeNullFromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtNull, 0);
}

//*************************************************************************************************
// Boolean

VOID TypeBooleanToText(VOID *pData, CfxString &String)
{
    *(BOOL *)pData == TRUE ? String.Set("true") : String.Set("false");
}

VOID TypeBooleanFromText(CHAR *pString, VOID *pData)
{
    *(BOOL *)pData = stricmp(pString, "TRUE") == 0 ? TRUE : FALSE; 
}

VOID TypeBooleanToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtBoolean, sizeof(BOOL));
    Stream.Write1(pData);
}

VOID TypeBooleanFromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtBoolean, sizeof(BOOL));
    Stream.Read1(pData);
}

DOUBLE TypeBooleanToDouble(VOID *pData)
{
    return (*(BOOL *)pData) == TRUE ? 1 : 0;
}

//*************************************************************************************************
// Byte

VOID TypeByteToText(VOID *pData, CfxString &String)
{
    CHAR buffer[20];
    itoa(*(BYTE *)pData, buffer, 10);
    String.Set(buffer);
}

VOID TypeByteFromText(CHAR *pString, VOID *pData)
{
    *(BYTE *)pData = (BYTE)atoi(pString);
}

VOID TypeByteToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtByte, sizeof(BYTE));
    Stream.Write1(pData);
}

VOID TypeByteFromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtByte, sizeof(BYTE));
    Stream.Read1(pData);
}

DOUBLE TypeByteToDouble(VOID *pData)
{
    return (DOUBLE)(*(BYTE *)pData);
}

//*************************************************************************************************
// Int8

VOID TypeInt8ToText(VOID *pData, CfxString &String)
{
    CHAR buffer[10];
    itoa(*(INT8 *)pData, buffer, 10);
    String.Set(buffer);
}

VOID TypeInt8FromText(CHAR *pString, VOID *pData)
{
    *((INT8 *)pData) = (INT8)atoi(pString);
}

VOID TypeInt8ToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtInt8, sizeof(INT8));
    Stream.Write1(pData);
}

VOID TypeInt8FromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtInt8, sizeof(INT8));
    Stream.Read1(pData);
}

DOUBLE TypeInt8ToDouble(VOID *pData)
{
    return (DOUBLE)(*(INT8 *)pData);
}

//*************************************************************************************************
// Int16

VOID TypeInt16ToText(VOID *pData, CfxString &String)
{
    CHAR buffer[20];
    INT16 value;
    memcpy(&value, pData, sizeof(INT16));
    itoa(value, buffer, 10);
    String.Set(buffer);
}

VOID TypeInt16FromText(CHAR *pString, VOID *pData)
{
    INT16 value = (INT16)atoi(pString);
    memcpy(pData, &value, sizeof(value));
}

VOID TypeInt16ToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtInt16, sizeof(INT16));
    Stream.Write2(pData);
}

VOID TypeInt16FromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtInt16, sizeof(INT16));
    Stream.Read2(pData);
}

DOUBLE TypeInt16ToDouble(VOID *pData)
{
    INT16 value;
    memcpy(&value, pData, sizeof(INT16));
    return (DOUBLE)value;
}

//*************************************************************************************************
// Int32

VOID TypeInt32ToText(VOID *pData, CfxString &String)
{
    CHAR buffer[20];
    INT value = 0;
    memcpy(&value, pData, sizeof(value));
    itoa(value, buffer, 10);
    String.Set(buffer);
}

VOID TypeInt32FromText(CHAR *pString, VOID *pData)
{
    INT value = (INT)atoi(pString);
    memcpy(pData, &value, sizeof(value));
}

VOID TypeInt32ToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtInt32, sizeof(INT));
    Stream.Write4(pData);
}

VOID TypeInt32FromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtInt32, sizeof(INT));
    Stream.Read4(pData);
}

DOUBLE TypeInt32ToDouble(VOID *pData)
{
    INT value;
    memcpy(&value, pData, sizeof(INT));
    return (DOUBLE)value;
}

//*************************************************************************************************
// Double

VOID TypeDoubleToText(VOID *pData, CfxString &String)
{
    CHAR buffer[20];
    DOUBLE d;
    memcpy(&d, pData, sizeof(d));
    PrintF(buffer, d, 8, 6);
    String.Set(buffer);
}

VOID TypeDoubleFromText(CHAR *pString, VOID *pData)
{
    DOUBLE value = (DOUBLE)atof(pString);
    memcpy(pData, &value, sizeof(value));
}

VOID TypeDoubleToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtDouble, sizeof(DOUBLE));
    Stream.Write8(pData);
}

VOID TypeDoubleFromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtDouble, sizeof(DOUBLE));
    Stream.Read8(pData);
}

DOUBLE TypeDoubleToDouble(VOID *pData)
{
    DOUBLE value;
    memcpy(&value, pData, sizeof(DOUBLE));
    return (DOUBLE)value;
}

//*************************************************************************************************
// Date

VOID TypeDateToText(VOID *pData, CfxString &String)
{
    CHAR valueText[32];
    DOUBLE dateDouble;

    FXDATETIME dateTime;
    memcpy(&dateDouble, pData, sizeof(dateDouble));
    DecodeDate(dateDouble, &dateTime);
    sprintf(valueText, "%04d/%02d/%02d", dateTime.Year, dateTime.Month, dateTime.Day);

    String.Set(valueText);
}

VOID TypeDateFromText(CHAR *pString, VOID *pData)
{
}

VOID TypeDateToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtDate, sizeof(DATE));
    Stream.Write8(pData);
}

VOID TypeDateFromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtDate, sizeof(DATE));
    Stream.Read8(pData);
}

//*************************************************************************************************
// Time

VOID TypeTimeToText(VOID *pData, CfxString &String)
{
    CHAR valueText[32];

    FXDATETIME dateTime;
    DOUBLE timeDouble;
    memcpy(&timeDouble, pData, sizeof(timeDouble));
    DecodeDateTime(timeDouble, &dateTime);
    sprintf(valueText, "%02d:%02d:%02d", dateTime.Hour, dateTime.Minute, dateTime.Second);

    String.Set(valueText);
}

VOID TypeTimeFromText(CHAR *pString, VOID *pData)
{
}

VOID TypeTimeToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtTime, sizeof(TIME));
    Stream.Write8(pData);
}

VOID TypeTimeFromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtTime, sizeof(TIME));
    Stream.Read8(pData);
}

//*************************************************************************************************
// DateTime

VOID TypeDateTimeToText(VOID *pData, CfxString &String)
{
    String.Set("Not yet implemented");
}

VOID TypeDateTimeFromText(CHAR *pString, VOID *pData)
{
}

VOID TypeDateTimeToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtDateTime, sizeof(DATETIME));
    Stream.Write8(pData);
}

VOID TypeDateTimeFromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtDateTime, sizeof(DATETIME));
    Stream.Read8(pData);
}

//*************************************************************************************************
// Color

VOID TypeColorToText(VOID *pData, CfxString &String)
{
    CHAR buffer[20];
    COLOR value = 0;
    memcpy(&value, pData, sizeof(value));
    itoa(value, buffer, 10);
    String.Set(buffer);
}

VOID TypeColorFromText(CHAR *pString, VOID *pData)
{
    COLOR value = (COLOR)atoi(pString);
    memcpy(pData, &value, sizeof(value));
}

VOID TypeColorToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtColor, sizeof(COLOR));
    Stream.Write4(pData);
}

VOID TypeColorFromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtColor, sizeof(COLOR));
    Stream.Read4(pData);
}

//*************************************************************************************************
// Guid

VOID TypeGuidToText(VOID *pData, CfxString &String)
{
    CHAR s[39] = {0};
    GUID *g = (GUID *)pData;
    sprintf(s, 
            "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
            g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

    String.Set(s);
}

VOID TypeGuidFromText(CHAR *pString, VOID *pData)
{
}

VOID TypeGuidToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtGuid, sizeof(GUID));

    GUID *pGuid;
    memcpy(&pGuid, &pData, sizeof(GUID *));
    Stream.Write4(&pGuid->Data1);
    Stream.Write2(&pGuid->Data2);
    Stream.Write2(&pGuid->Data3);
    Stream.Write(&pGuid->Data4[0], sizeof(pGuid->Data4));
}

VOID TypeGuidFromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtGuid, sizeof(GUID));

    GUID *pGuid;
    memcpy(&pGuid, &pData, sizeof(GUID *));
    Stream.Read4(&pGuid->Data1);
    Stream.Read2(&pGuid->Data2);
    Stream.Read2(&pGuid->Data3);
    Stream.Read(&pGuid->Data4[0], sizeof(pGuid->Data4));
}

//*************************************************************************************************
// Font

VOID TypeFontToText(VOID *pData, CfxString &String)
{
    String.Set((CHAR *)pData);
}

VOID TypeFontFromText(CHAR *pString, VOID *pData)
{
}

VOID TypeFontToStream(VOID *pData, CfxStream &Stream)
{
    WriteType(Stream, dtFont, sizeof(FXFONT));
    Stream.Write(pData, sizeof(FXFONT));
}

VOID TypeFontFromStream(VOID *pData, CfxStream &Stream)
{
    ReadType(Stream, dtFont, sizeof(FXFONT));
    Stream.Read(pData, sizeof(FXFONT));
}

//*************************************************************************************************
// TextAnsi

VOID TypeTextAnsiToText(VOID *pData, CfxString &String)
{
    String.Set((CHAR *)pData);
}

VOID TypeTextAnsiFromText(CHAR *pString, VOID *pData)
{
    FX_ASSERT(FALSE);
}

VOID TypeTextAnsiToStream(VOID *pData, CfxStream &Stream)
{
    UINT size = (UINT) strlen((CHAR *)pData) + 1;

    WriteType(Stream, dtTextAnsi, size);
    Stream.Write(pData, size);
}

VOID TypeTextAnsiFromStream(VOID *pData, CfxStream &Stream)
{
    UINT size = ReadType(Stream, dtTextAnsi, -1);

    Stream.Read(pData, size);
}

//*************************************************************************************************
// TextUtf8

VOID TypeTextUtf8ToText(VOID *pData, CfxString &String)
{
    //FX_ASSERT(FALSE);
    String.Set((CHAR *)pData);
}

VOID TypeTextUtf8FromText(CHAR *pString, VOID *pData)
{
    FX_ASSERT(FALSE);
}

VOID TypeTextUtf8ToStream(VOID *pData, CfxStream &Stream)
{
    UINT size = (UINT) strlen((CHAR *)pData) + 1;
    WriteType(Stream, dtTextUtf8, size);
    Stream.Write(pData, size);
}

VOID TypeTextUtf8FromStream(VOID *pData, CfxStream &Stream)
{
    UINT size = ReadType(Stream, dtTextUtf8, -1);
    Stream.Read(pData, size);
}

//*************************************************************************************************
// Binary

VOID TypeBinaryToText(VOID *pData, CfxString &String)
{
    String.Set("[Binary]");
}

VOID TypeBinaryFromText(CHAR *pString, VOID *pData)
{
    FX_ASSERT(FALSE);
}

VOID TypeBinaryToStream(VOID *pData, CfxStream &Stream)
{
    FX_ASSERT(FALSE);
}

VOID TypeBinaryFromStream(VOID *pData, CfxStream &Stream)
{
    FX_ASSERT(FALSE);
}

//*************************************************************************************************
// Graphic

VOID TypeGraphicToText(VOID *pData, CfxString &String)
{
    String.Set("[Graphic]");
}

VOID TypeGraphicFromText(CHAR *pString, VOID *pData)
{
    FX_ASSERT(FALSE);
}

VOID TypeGraphicToStream(VOID *pData, CfxStream &Stream)
{
    FX_ASSERT(FALSE);
}

VOID TypeGraphicFromStream(VOID *pData, CfxStream &Stream)
{
    FX_ASSERT(FALSE);
}

//*************************************************************************************************
// Sound

VOID TypeSoundToText(VOID *pData, CfxString &String)
{
    String.Set("[Sound]");
}

VOID TypeSoundFromText(CHAR *pString, VOID *pData)
{
    FX_ASSERT(FALSE);
}

VOID TypeSoundToStream(VOID *pData, CfxStream &Stream)
{
    FX_ASSERT(FALSE);
}

VOID TypeSoundFromStream(VOID *pData, CfxStream &Stream)
{
    FX_ASSERT(FALSE);
}

//*************************************************************************************************
// GuidList

VOID TypeGuidListToText(VOID *pData, CfxString &String)
{
    String.Set("[GuidList]");
}

VOID TypeGuidListFromText(CHAR *pString, VOID *pData)
{
    FX_ASSERT(FALSE);
}

VOID TypeGuidListToStream(VOID *pData, CfxStream &Stream)
{
    FX_ASSERT(FALSE);
}

VOID TypeGuidListFromStream(VOID *pData, CfxStream &Stream)
{
    FX_ASSERT(FALSE);
}

//*************************************************************************************************

VOID BinaryToStream(FXDATATYPE Type, VOID *pData, CfxStream &Stream)
{
    UINT size = Type_Size(*(VOID **)pData);

    WriteType(Stream, Type, size);
    if (size > 0)
    {
        Stream.Write(*(VOID **)pData, size);
    }
}

VOID BinaryFromStream(FXDATATYPE Type, VOID *pData, CfxStream &Stream)
{
    UINT size = ReadType(Stream, Type, -1);

    *(VOID **)pData = Type_Alloc(*(VOID **)pData, size);
    if (size > 0)
    {
        Stream.Read(*(VOID **)pData, size);
    }
}

//*************************************************************************************************
// PTextAnsi

VOID TypePTextAnsiToText(VOID *pData, CfxString &String)
{
    String.Set(*(CHAR **)pData);
}

VOID TypePTextAnsiFromText(CHAR *pString, VOID *pData)
{
    *(VOID **)pData = Type_Alloc(*(VOID **)pData, (UINT) strlen(pString) + 1);
    strcpy(*(CHAR **)pData, pString);
}

VOID TypePTextAnsiToStream(VOID *pData, CfxStream &Stream)
{
    UINT size = *(VOID **)pData == NULL ? 0 : (UINT) strlen(*(CHAR **)pData) + 1;

    WriteType(Stream, dtTextAnsi, size);

    if (size > 0) 
    {
        Stream.Write(*(VOID **)pData, size);
    }
}

VOID TypePTextAnsiFromStream(VOID *pData, CfxStream &Stream)
{
    UINT size = ReadType(Stream, dtTextAnsi, -1);
    *(VOID **)pData = Type_Alloc(*(VOID **)pData, size);
    Stream.Read(*(VOID **)pData, size);
}

//*************************************************************************************************
// PTextUtf8

VOID TypePTextUtf8ToText(VOID *pData, CfxString &String)
{
    FX_ASSERT(FALSE);
    String.Set(*(CHAR **)pData);
}

VOID TypePTextUtf8FromText(CHAR *pString, VOID *pData)
{
    *(VOID **)pData = Type_Alloc(*(VOID **)pData, (UINT) strlen(pString) + 1);
    strcpy(*(CHAR **)pData, pString);
}

VOID TypePTextUtf8ToStream(VOID *pData, CfxStream &Stream)
{
    UINT size = *(VOID **)pData == NULL ? 0 : (UINT) strlen(*(CHAR **)pData) + 1;

    WriteType(Stream, dtTextUtf8, size);

    if (size > 0) 
    {
        Stream.Write(*(VOID **)pData, size);
    }
}

VOID TypePTextUtf8FromStream(VOID *pData, CfxStream &Stream)
{
    UINT size = ReadType(Stream, dtTextUtf8, -1);
    *(VOID **)pData = Type_Alloc(*(VOID **)pData, size);
    Stream.Read(*(VOID **)pData, size);
}

//*************************************************************************************************
// Support functions

VOID BinToHex(VOID *pData, UINT _size, CfxString &String)
{
    //String.SetSize(_size * 2);
    FX_ASSERT(FALSE);
}

VOID HexToBin(CHAR *pString, VOID *pData, UINT _size)
{
    FX_ASSERT(FALSE);
}

//*************************************************************************************************
// PBinary

VOID TypePBinaryToText(VOID *pData, CfxString &String)
{
    UINT size = Type_Size(*(VOID **)pData);
    BinToHex(*(VOID **)pData, size, String);
}

VOID TypePBinaryFromText(CHAR *pString, VOID *pData)
{
    UINT size = (UINT) strlen(pString) / 2;
    *(VOID **)pData = Type_Alloc(*(VOID **)pData, size);
    HexToBin(pString, *(VOID **)pData, size);
}

VOID TypePBinaryToStream(VOID *pData, CfxStream &Stream)
{
    BinaryToStream(dtBinary, pData, Stream);
}

VOID TypePBinaryFromStream(VOID *pData, CfxStream &Stream)
{
    BinaryFromStream(dtBinary, pData, Stream);
}

//*************************************************************************************************
// PGraphic

VOID TypePGraphicToText(VOID *pData, CfxString &String)
{
    TypePBinaryToText(pData, String);
}

VOID TypePGraphicFromText(CHAR *pString, VOID *pData)
{
    TypePBinaryFromText(pString, pData);
}

VOID TypePGraphicToStream(VOID *pData, CfxStream &Stream)
{
    BinaryToStream(dtGraphic, pData, Stream);
}

VOID TypePGraphicFromStream(VOID *pData, CfxStream &Stream)
{
    BinaryFromStream(dtGraphic, pData, Stream);
}

//*************************************************************************************************
// PSound

VOID TypePSoundToText(VOID *pData, CfxString &String)
{
    TypePBinaryToText(pData, String);
}

VOID TypePSoundFromText(CHAR *pString, VOID *pData)
{
    TypePBinaryFromText(pString, pData);
}

VOID TypePSoundToStream(VOID *pData, CfxStream &Stream)
{
    BinaryToStream(dtSound, pData, Stream);
}

VOID TypePSoundFromStream(VOID *pData, CfxStream &Stream)
{
    BinaryFromStream(dtSound, pData, Stream);
}

//*************************************************************************************************
// PGuidList

VOID TypePGuidListToText(VOID *pData, CfxString &String)
{
    TypePBinaryToText(pData, String);
}

VOID TypePGuidListFromText(CHAR *pString, VOID *pData)
{
    TypePBinaryFromText(pString, pData);
}

VOID TypePGuidListToStream(VOID *pData, CfxStream &Stream)
{
    BinaryToStream(dtGuidList, pData, Stream);
}

VOID TypePGuidListFromStream(VOID *pData, CfxStream &Stream)
{
    BinaryFromStream(dtGuidList, pData, Stream);
}

//*************************************************************************************************
// Type support

VOID Type_DataToText(FXDATATYPE Type, VOID *pData, CfxString &Value)
{
    DataTypeInfo[Type].FnToText(pData, Value);
}

VOID Type_DataFromText(CHAR *InValue, FXDATATYPE Type, VOID *pData)
{
    DataTypeInfo[Type].FnFromText(InValue, pData);
}

VOID Type_ToStream(FXDATATYPE Type, VOID *pData, CfxStream &Stream)
{
    DataTypeInfo[Type].FnToStream(pData, Stream);
}

VOID Type_FromStream(FXDATATYPE Type, VOID *pData, CfxStream &Stream)
{
    DataTypeInfo[Type].FnFromStream(pData, Stream);
}

VOID *Type_Alloc(VOID *pData, UINT _size)
{
    if (_size == 0) 
    {
        Type_FreeAndNil(&pData);
        return NULL;
    }

    VOID *pResult = pData ? MEM_REALLOC(pData, _size) : MEM_ALLOC(_size);

    memset(pResult, 0, _size);

    return pResult;
}

VOID Type_FreeAndNil(VOID **ppData)
{
    MEM_FREE(*ppData);
    *ppData = NULL;
}

UINT Type_Size(VOID *pData)
{
    if (pData != NULL)
    {
        return (UINT) MEM_SIZE(pData);
    }
    else
    {
        return 0;
    }
}

BOOL Type_IsText(FXDATATYPE Type)
{
    return (Type == dtTextAnsi) ||
           (Type == dtTextUtf8) ||
           (Type == dtPTextAnsi) ||
           (Type == dtPTextUtf8);
}

INT Type_StaticSize(FXDATATYPE Type)
{
    return DataTypeInfo[Type].StaticSize;
}

VOID Type_CreateEditor(FXDATATYPE *pType, VOID **ppData, UINT Size)
{
    FX_ASSERT(!(*pType == dtPTextAnsi));
    FX_ASSERT(!(*pType == dtPTextUtf8));
    FX_ASSERT(!(*pType == dtPBinary));
    FX_ASSERT(!(*pType == dtPGraphic));
    FX_ASSERT(!(*pType == dtPSound));
    FX_ASSERT(!(*pType == dtPGuidList));

    VOID *outData = NULL;
    FXDATATYPE outType = *pType;

    switch (*pType)
    {
    case dtTextAnsi:
    case dtTextUtf8:
    case dtBinary:
    case dtGraphic:
    case dtSound:
    case dtGuidList:
        outData = Type_Alloc(NULL, sizeof(VOID *));
        
        *(VOID **)outData = NULL;
        if (Size > 0)
        {
            *(VOID **)outData = Type_Alloc(NULL, Size);
            memmove(*(VOID **)outData, *ppData, Size);
        }

        switch (*pType)
        {
        case dtTextAnsi: outType = dtPTextAnsi; break;
        case dtTextUtf8: outType = dtPTextUtf8; break;
        case dtBinary:   outType = dtPBinary;   break;
        case dtGraphic:  outType = dtPGraphic;  break;
        case dtSound:    outType = dtPSound;    break;
        case dtGuidList: outType = dtPGuidList; break;
        }
        break;
    default:
        if (Size == 0)
        {
            Size = DataTypeInfo[*pType].StaticSize;
            FX_ASSERT(Size >= 0);
        }

        outData = Type_Alloc(NULL, Size);
        if ((Size > 0) && outData && *ppData)
        {
            memmove(outData, *ppData, Size);
        }
    }
    
    *ppData = outData;
    *pType = outType;
}

VOID Type_FreeEditor(FXDATATYPE *pType, VOID **ppData)
{
    FX_ASSERT(!(*pType == dtTextAnsi));
    FX_ASSERT(!(*pType == dtTextUtf8));
    FX_ASSERT(!(*pType == dtBinary));
    FX_ASSERT(!(*pType == dtGraphic));
    FX_ASSERT(!(*pType == dtSound));
    FX_ASSERT(!(*pType == dtGuidList));    

    if ((*pType == dtPTextAnsi) || (*pType == dtPTextUtf8) || 
        (*pType == dtPBinary) || (*pType == dtPGraphic) || 
        (*pType == dtPSound) || (*pType == dtPGuidList))
    {
        VOID *data = *ppData;
        MEM_FREE(*(VOID **)data);
    }

    Type_FreeAndNil(ppData);

    FXDATATYPE outType = *pType;
    switch (*pType)
    {
    case dtPTextAnsi:  outType = dtTextAnsi; break;
    case dtPTextUtf8:  outType = dtTextUtf8; break;
    case dtPBinary:    outType = dtBinary;   break;
    case dtPGraphic:   outType = dtGraphic;  break;
    case dtPSound:     outType = dtSound;    break;
    case dtPGuidList:  outType = dtGuidList; break;
    }
}

FXVARIANT *Type_CreateVariant(FXDATATYPE Type, VOID *pData)
{
    CfxStream stream;
    Type_ToStream(Type, pData, stream);
    return (FXVARIANT *)stream.CloneMemory();
}

VOID Type_FreeVariant(FXVARIANT **ppVariant)
{
    MEM_FREE(*ppVariant);
    *ppVariant = NULL;
}

VOID Type_VariantToText(FXVARIANT *pVariant, CfxString &Value)
{
    Type_DataToText((FXDATATYPE)(pVariant->Type), &pVariant->Data[0], Value);
}

FXVARIANT *Type_DuplicateVariant(FXVARIANT *pVariant)
{
    return Type_CreateVariant((FXDATATYPE)(pVariant->Type), &pVariant->Data[0]);
}

DOUBLE Type_VariantToDouble(FXVARIANT *pVariant)
{
    if (DataTypeInfo[pVariant->Type].FnToDouble)
    {
        return DataTypeInfo[pVariant->Type].FnToDouble(pVariant->Data);
    }
    else
    {
        return 0;
    }
}

VOID Type_VariantToStream(FXVARIANT *pVariant, CfxStream &Stream)
{
    WriteType(Stream, (FXDATATYPE)pVariant->Type, pVariant->Size);
    Stream.Write(&pVariant->Data[0], pVariant->Size);
}

VOID Type_VariantToStream(FXVARIANT *pVariant, CfxFileStream &Stream)
{
    WriteType(Stream, (FXDATATYPE)pVariant->Type, pVariant->Size);
    Stream.Write(&pVariant->Data[0], pVariant->Size);
}

FXDATATYPE GetTypeFromString(const CHAR *pValue)
{
    if (pValue == NULL)
    {
        return dtNull;
    }

    if (strncmp(pValue, MEDIA_PREFIX, strlen(MEDIA_PREFIX)) != 0)
    {
        return dtTextAnsi;
    }

    if ((strstr(pValue, ".wav") != NULL) || 
        (strstr(pValue, ".WAV") != NULL) ||
        (strstr(pValue, ".mp3") != NULL) ||
        (strstr(pValue, ".MP3") != NULL))
    {
        return dtSound;
    }

    return dtGraphic;
}