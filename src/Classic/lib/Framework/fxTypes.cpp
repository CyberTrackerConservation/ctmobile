#include "fxTypes.h"
#include "fxUtils.h"

#define MEMORY_DELTA 0x0100UL // must be a power of 2

const GUID  GUID_0  = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

    const XGUID XGUID_0 = 0;

//*************************************************************************************************

void PrintF(char* S, double Value, UINT16 Size, UINT16 Precision) 
{ 
    S[0] = 0; 
    
    // Get integer portion 
    sprintf(S, "%ld.", (int)Value); 

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

    int power = 1;
    for (INT16 k=0; k<i; k++) power*=10;
    
    int rem = (int)((Value - (int)Value) * power);
    if (rem < 0)
    {
        rem = -rem;
    } 

    // Determine length of Remainder and glue zeros in front if necessary 
    sprintf(S, "%ld", rem); 
    len = strlen(S); 


    sprintf(S, "%ld.", (int)Value); 
    for (INT16 j=0; j<i-len; j++) 
    {
        strcat(S, "0"); 
    }

    // Add in remainder 
    sprintf(&S[strlen(S)], "%lu", (int)rem); 
} 

//*************************************************************************************************
// CfxStream

void *CfxStream::ReAlloc(int *pNewCapacity)
{
    FX_ASSERT(_ownsMemory);

    void *pResult;

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
            pResult = nullptr;
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

            FX_ASSERT(pResult != nullptr);
        }
    }

    return pResult;
}

void CfxStream::SetMemory(void *pMemory, int MemorySize)
{
    _memory = pMemory;
    _size = MemorySize;
}

CfxStream::CfxStream()
{
    _ownsMemory = true;
    _memory = nullptr;
    _size = _position = _capacity = 0;
}

CfxStream::CfxStream(void *pMemory, int MemorySize)
{
    _ownsMemory = false;
    _memory = pMemory;
    _size = MemorySize;
    _capacity = MemorySize;
    _position = 0;
}
    
CfxStream::~CfxStream()
{
    Clear();
}
    
void CfxStream::Clear() 
{
    if (!_ownsMemory) return;

    SetCapacity(0);
    _size = _position = 0;
}
    
void CfxStream::Read(void *pBuffer, int Count)
{
    if (Count == 0) return;

    FX_ASSERT(_position + Count <= _size);
    memmove(pBuffer, (BYTE *) _memory + _position, Count);
    _position += Count;
}
            
void CfxStream::Write(void *pBuffer, int Count) 
{
    if (Count == 0) return;

    int pos = _position + Count;
 
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

void CfxStream::Read1(void *pBuffer)
{
    Read(pBuffer, 1);
}

void CfxStream::Read2(void *pBuffer)
{
    Read(pBuffer, 2);
    //GET16(pBuffer, (BYTE *)_memory + _position);
    //_position += 2;
}

void CfxStream::Read4(void *pBuffer)
{
    Read(pBuffer, 4);
    //GET32(pBuffer, (BYTE *)_memory + _position);
    //_position += 4;
}

void CfxStream::Read8(void *pBuffer)
{
    Read(pBuffer, 8);
    //GET64(pBuffer, (BYTE *)_memory + _position);
    //_position += 8;
}

void CfxStream::Write1(void *pBuffer)
{
    Write(pBuffer, 1);
}

void CfxStream::Write2(void *pBuffer)
{
    Write(pBuffer, 2);
    //BYTE value[2];
    //GET16(&value[0], pBuffer);
    //Write(&value[0], 2);
}

void CfxStream::Write4(void *pBuffer)
{
    Write(pBuffer, 4);
    //BYTE value[4];
    //GET32(&value[0], pBuffer);
    //Write(&value[0], 4);
}

void CfxStream::Write8(void *pBuffer)
{
    Write(pBuffer, 8);
    //BYTE value[8];
    //GET64(&value[0], pBuffer);
    //Write(&value[0], 8);
}

void CfxStream::Write(const char *pString)
{
    // Write the string and null terminator, then backup to before the terminator.
    // This means the raw buffer will always be a null terminated string.
    Write((void *)pString, strlen(pString) + 1);
    _position--;
}

void CfxStream::Peek(void *pBuffer, int Count)
{
    Read(pBuffer, Count);
    _position -= Count;
}

void CfxStream::LoadFromStream(CfxStream &Stream)
{
    int count;
    Stream._position = 0;
    count = Stream.GetSize();
    SetSize(count);
    if (count) 
    {
        Stream.Read(_memory, count);
    }
}

void CfxStream::SaveToStream(CfxStream &Stream)
{
    if (_size) 
    {
        Stream.Write(_memory, _size);
    }
}

void CfxStream::SetPosition(int NewPosition)
{
    _position = NewPosition;
}

void CfxStream::SetCapacity(int NewCapacity)
{
    SetMemory(ReAlloc(&NewCapacity), _size);
    _capacity = NewCapacity;
}

void CfxStream::SetSize(int NewSize)
{
    int oldPosition = _position;
    SetCapacity(NewSize);
    _size = NewSize;
    if (oldPosition > NewSize) 
    {
        _position = _size;
    }
}

void *CfxStream::CloneMemory()
{ 
    void *memory = MEM_ALLOC(_size); 
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

void CfxFileStream::Close()
{
    if (_fileHandle)
    {
        FileClose(_fileHandle);
        _fileHandle = 0;
    }

    _position = 0;
    memset(_fileName, 0, sizeof(_fileName));
}

bool CfxFileStream::OpenForRead(char *pFileName)
{
    Close();
    strncpy(_fileName, pFileName, ARRAYSIZE(_fileName));
    _fileHandle = FileOpen(_fileName, "rb");
    return _fileHandle != 0;
}

bool CfxFileStream::OpenForWrite(char *pFileName)
{
    Close();
    strncpy(_fileName, pFileName, ARRAYSIZE(_fileName));
    _fileHandle = FileOpen(_fileName, "wb");
    return _fileHandle != 0;
}

bool CfxFileStream::Read(void *pBuffer, int Count)
{
    bool result = _fileHandle && FileRead(_fileHandle, pBuffer, Count);
    if (result)
    {
        _position += Count;
    }

    return result;
}

bool CfxFileStream::Write(void *pBuffer, int Count)
{
    bool result = _fileHandle && FileWrite(_fileHandle, pBuffer, Count);
    if (result)
    {
        _position += Count;
    }

    return result;
}

bool CfxFileStream::Write(const char *pString)
{
    return Write((void *)pString, strlen(pString));
}

bool CfxFileStream::WriteNullChar()
{
    char nullChar = 0;
    return Write(&nullChar, 1);
}

bool CfxFileStream::Seek(int Position)
{
    bool result = _fileHandle && FileSeek(_fileHandle, Position);
    if (result)
    {
        _position = Position;
    }

    return result;
}

int CfxFileStream::GetSize()
{
    if (_fileHandle)
    {
        return FileGetSize(_fileHandle);
    }

    return 0;
}

int CfxFileStream::GetLastError()
{
    if (_fileHandle)
    {
        return FileGetLastError(_fileHandle);
    }
    else
    {
        return (int)-1;
    }
}


//*************************************************************************************************
// Endian operations

void Get16(BYTE *pDst, BYTE *pSrc)
{
    memcpy(pDst, pSrc, sizeof(INT16));
}

void Get32(BYTE *pDst, BYTE *pSrc)
{
    memcpy(pDst, pSrc, sizeof(int));
}

void Get64(BYTE *pDst, BYTE *pSrc)
{
    memcpy(pDst, pSrc, sizeof(double));
}

void Swap16(BYTE *pDst, BYTE *pSrc)
{
    BYTE *ptr = pDst + 1;
    *ptr-- = *pSrc++;
    *ptr = *pSrc;
}

void Swap32(BYTE *pDst, BYTE *pSrc)
{
    BYTE *ptr = pDst + 3;
    *ptr-- = *pSrc++;
    *ptr-- = *pSrc++;
    *ptr-- = *pSrc++;
    *ptr = *pSrc;
}

void Swap64(BYTE *pDst, BYTE *pSrc)
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

typedef void (*_pfn_ToText)(void *pData, QString &String);
typedef void (*_pfn_FromText)(char *pString, void *pData);
typedef void (*_pfn_ToStream)(void *pData, CfxStream &Stream);
typedef void (*_pfn_FromStream)(void *pData, CfxStream &Stream);
typedef double (*_pfn_ToDouble)(void *pData);

struct DATATYPEINFO
{
    FXDATATYPE Type = {};
    int StaticSize = 0;
    _pfn_ToText FnToText = nullptr;
    _pfn_FromText FnFromText = nullptr;
    _pfn_ToStream FnToStream = nullptr;
    _pfn_FromStream FnFromStream = nullptr;
    _pfn_ToDouble FnToDouble = nullptr;
};

void TypeNullToText(void *pData, QString &String);
void TypeNullFromText(char *pString, void *pData);
void TypeNullToStream(void *pData, CfxStream &Stream);
void TypeNullFromStream(void *pData, CfxStream &Stream);

void TypeBooleanToText(void *pData, QString &String);
void TypeBooleanFromText(char *pString, void *pData);
void TypeBooleanToStream(void *pData, CfxStream &Stream);
void TypeBooleanFromStream(void *pData, CfxStream &Stream);
double TypeBooleanToDouble(void *pData);

void TypeByteToText(void *pData, QString &String);
void TypeByteFromText(char *pString, void *pData);
void TypeByteToStream(void *pData, CfxStream &Stream);
void TypeByteFromStream(void *pData, CfxStream &Stream);
double TypeByteToDouble(void *pData);

void TypeInt8ToText(void *pData, QString &String);
void TypeInt8FromText(char *pString, void *pData);
void TypeInt8ToStream(void *pData, CfxStream &Stream);
void TypeInt8FromStream(void *pData, CfxStream &Stream);
double TypeInt8ToDouble(void *pData);

void TypeInt16ToText(void *pData, QString &String);
void TypeInt16FromText(char *pString, void *pData);
void TypeInt16ToStream(void *pData, CfxStream &Stream);
void TypeInt16FromStream(void *pData, CfxStream &Stream);
double TypeInt16ToDouble(void *pData);

void TypeInt32ToText(void *pData, QString &String);
void TypeInt32FromText(char *pString, void *pData);
void TypeInt32ToStream(void *pData, CfxStream &Stream);
void TypeInt32FromStream(void *pData, CfxStream &Stream);
double TypeInt32ToDouble(void *pData);

void TypeDoubleToText(void *pData, QString &String);
void TypeDoubleFromText(char *pString, void *pData);
void TypeDoubleToStream(void *pData, CfxStream &Stream);
void TypeDoubleFromStream(void *pData, CfxStream &Stream);
double TypeDoubleToDouble(void *pData);

void TypeDateToText(void *pData, QString &String);
void TypeDateFromText(char *pString, void *pData);
void TypeDateToStream(void *pData, CfxStream &Stream);
void TypeDateFromStream(void *pData, CfxStream &Stream);

void TypeTimeToText(void *pData, QString &String);
void TypeTimeFromText(char *pString, void *pData);
void TypeTimeToStream(void *pData, CfxStream &Stream);
void TypeTimeFromStream(void *pData, CfxStream &Stream);

void TypeDateTimeToText(void *pData, QString &String);
void TypeDateTimeFromText(char *pString, void *pData);
void TypeDateTimeToStream(void *pData, CfxStream &Stream);
void TypeDateTimeFromStream(void *pData, CfxStream &Stream);

void TypeColorToText(void *pData, QString &String);
void TypeColorFromText(char *pString, void *pData);
void TypeColorToStream(void *pData, CfxStream &Stream);
void TypeColorFromStream(void *pData, CfxStream &Stream);

void TypeGuidToText(void *pData, QString &String);
void TypeGuidFromText(char *pString, void *pData);
void TypeGuidToStream(void *pData, CfxStream &Stream);
void TypeGuidFromStream(void *pData, CfxStream &Stream);

void TypeFontToText(void *pData, QString &String);
void TypeFontFromText(char *pString, void *pData);
void TypeFontToStream(void *pData, CfxStream &Stream);
void TypeFontFromStream(void *pData, CfxStream &Stream);

void TypeTextAnsiToText(void *pData, QString &String);
void TypeTextAnsiFromText(char *pString, void *pData);
void TypeTextAnsiToStream(void *pData, CfxStream &Stream);
void TypeTextAnsiFromStream(void *pData, CfxStream &Stream);

void TypeTextUtf8ToText(void *pData, QString &String);
void TypeTextUtf8FromText(char *pString, void *pData);
void TypeTextUtf8ToStream(void *pData, CfxStream &Stream);
void TypeTextUtf8FromStream(void *pData, CfxStream &Stream);

void TypeBinaryToText(void *pData, QString &String);
void TypeBinaryFromText(char *pString, void *pData);
void TypeBinaryToStream(void *pData, CfxStream &Stream);
void TypeBinaryFromStream(void *pData, CfxStream &Stream);

void TypeGraphicToText(void *pData, QString &String);
void TypeGraphicFromText(char *pString, void *pData);
void TypeGraphicToStream(void *pData, CfxStream &Stream);
void TypeGraphicFromStream(void *pData, CfxStream &Stream);

void TypeSoundToText(void *pData, QString &String);
void TypeSoundFromText(char *pString, void *pData);
void TypeSoundToStream(void *pData, CfxStream &Stream);
void TypeSoundFromStream(void *pData, CfxStream &Stream);

void TypeGuidListToText(void *pData, QString &String);
void TypeGuidListFromText(char *pString, void *pData);
void TypeGuidListToStream(void *pData, CfxStream &Stream);
void TypeGuidListFromStream(void *pData, CfxStream &Stream);

void TypePTextAnsiToText(void *pData, QString &String);
void TypePTextAnsiFromText(char *pString, void *pData);
void TypePTextAnsiToStream(void *pData, CfxStream &Stream);
void TypePTextAnsiFromStream(void *pData, CfxStream &Stream);

void TypePTextUtf8ToText(void *pData, QString &String);
void TypePTextUtf8FromText(char *pString, void *pData);
void TypePTextUtf8ToStream(void *pData, CfxStream &Stream);
void TypePTextUtf8FromStream(void *pData, CfxStream &Stream);

void TypePBinaryToText(void *pData, QString &String);
void TypePBinaryFromText(char *pString, void *pData);
void TypePBinaryToStream(void *pData, CfxStream &Stream);
void TypePBinaryFromStream(void *pData, CfxStream &Stream);

void TypePGraphicToText(void *pData, QString &String);
void TypePGraphicFromText(char *pString, void *pData);
void TypePGraphicToStream(void *pData, CfxStream &Stream);
void TypePGraphicFromStream(void *pData, CfxStream &Stream);

void TypePSoundToText(void *pData, QString &String);
void TypePSoundFromText(char *pString, void *pData);
void TypePSoundToStream(void *pData, CfxStream &Stream);
void TypePSoundFromStream(void *pData, CfxStream &Stream);

void TypePGuidListToText(void *pData, QString &String);
void TypePGuidListFromText(char *pString, void *pData);
void TypePGuidListToStream(void *pData, CfxStream &Stream);
void TypePGuidListFromStream(void *pData, CfxStream &Stream);

DATATYPEINFO DataTypeInfo[dtMax] = 
{
    { dtNull,      0,              TypeNullToText,      TypeNullFromText,      TypeNullToStream,      TypeNullFromStream,        nullptr},
    { dtBoolean,   sizeof(bool),   TypeBooleanToText,   TypeBooleanFromText,   TypeBooleanToStream,   TypeBooleanFromStream,     TypeBooleanToDouble   },
    { dtByte,      sizeof(BYTE),   TypeByteToText,      TypeByteFromText,      TypeByteToStream,      TypeByteFromStream,        TypeByteToDouble   },
    { dtInt8,      sizeof(INT8),   TypeInt8ToText,      TypeInt8FromText,      TypeInt8ToStream,      TypeInt8FromStream,        TypeInt8ToDouble   },
    { dtInt16,     sizeof(INT16),  TypeInt16ToText,     TypeInt16FromText,     TypeInt16ToStream,     TypeInt16FromStream,       TypeInt16ToDouble  },
    { dtInt32,     sizeof(int),  TypeInt32ToText,     TypeInt32FromText,     TypeInt32ToStream,     TypeInt32FromStream,       TypeInt32ToDouble  },
    { dtDouble,    sizeof(double), TypeDoubleToText,    TypeDoubleFromText,    TypeDoubleToStream,    TypeDoubleFromStream,      TypeDoubleToDouble },
    { dtDate,      sizeof(double), TypeDateToText,      TypeDateFromText,      TypeDateToStream,      TypeDateFromStream,        nullptr},
    { dtTime,      sizeof(double), TypeTimeToText,      TypeTimeFromText,      TypeTimeToStream,      TypeTimeFromStream,        nullptr},
    { dtDateTime,  sizeof(double), TypeDateTimeToText,  TypeDateTimeFromText,  TypeDateTimeToStream,  TypeDateTimeFromStream,    nullptr},
    { dtColor,     sizeof(int), TypeColorToText,     TypeColorFromText,     TypeColorToStream,     TypeColorFromStream,       nullptr},
    { dtGuid,      sizeof(GUID),   TypeGuidToText,      TypeGuidFromText,      TypeGuidToStream,      TypeGuidFromStream,        nullptr},
    { dtFont,      sizeof(FXFONT), TypeFontToText,      TypeFontFromText,      TypeFontToStream,      TypeFontFromStream,        nullptr},
    { dtTextAnsi,  -1,             TypeTextAnsiToText,  TypeTextAnsiFromText,  TypeTextAnsiToStream,  TypeTextAnsiFromStream,    nullptr},
    { dtBinary,    -1,             TypeBinaryToText,    TypeBinaryFromText,    TypeBinaryToStream,    TypeBinaryFromStream,      nullptr},
    { dtGraphic,   -1,             TypeGraphicToText,   TypeGraphicFromText,   TypeGraphicToStream,   TypeGraphicFromStream,     nullptr},
    { dtSound,     -1,             TypeSoundToText,     TypeSoundFromText,     TypeSoundToStream,     TypeSoundFromStream,       nullptr},
    { dtGuidList,  -1,             TypeGuidListToText,  TypeGuidListFromText,  TypeGuidListToStream,  TypeGuidListFromStream,    nullptr},
    { dtTextUtf8,  -1,             TypeTextUtf8ToText,  TypeTextUtf8FromText,  TypeTextUtf8ToStream,  TypeTextUtf8FromStream,    nullptr},
    { dtPTextAnsi, -1,             TypePTextAnsiToText, TypePTextAnsiFromText, TypePTextAnsiToStream, TypePTextAnsiFromStream,   nullptr},
    { dtPBinary,   -1,             TypePBinaryToText,   TypePBinaryFromText,   TypePBinaryToStream,   TypePBinaryFromStream,     nullptr},
    { dtPGraphic,  -1,             TypePGraphicToText,  TypePGraphicFromText,  TypePGraphicToStream,  TypePGraphicFromStream,    nullptr},
    { dtPSound,    -1,             TypePSoundToText,    TypePSoundFromText,    TypePSoundToStream,    TypePSoundFromStream,      nullptr},
    { dtPGuidList, -1,             TypePGuidListToText, TypePGuidListFromText, TypePGuidListToStream, TypePGuidListFromStream,   nullptr},
    { dtPTextUtf8, -1,             TypePTextUtf8ToText, TypePTextUtf8FromText, TypePTextUtf8ToStream, TypePTextUtf8FromStream,   nullptr},
};

int ReadType(CfxStream &Stream, FXDATATYPE ExpectedType, int ExpectedSize)
{
    BYTE type;
    FXDATATYPE dataType;
    int result;

    Stream.Read1((void *)&type);
    dataType = (FXDATATYPE)type;

    FX_ASSERT(ExpectedType == dataType);

    Stream.Read4((void *)&result);
    FX_ASSERT((ExpectedSize < 0) || ((int)ExpectedSize == result));

    return result;
}

void WriteType(CfxStream &Stream, FXDATATYPE Type, int DataSize)
{
    BYTE type = Type;
    Stream.Write1((void *)&type);
    Stream.Write4((void *)&DataSize);
}

void WriteType(CfxFileStream &Stream, FXDATATYPE Type, int DataSize)
{
    BYTE type = Type;
    Stream.Write((void *)&type, 1);
    Stream.Write((void *)&DataSize, sizeof(int));
}

//*************************************************************************************************
// Null

void TypeNullToText(void *pData, QString &String)
{
    String.clear();
}

void TypeNullFromText(char *pString, void *pData)
{
}

void TypeNullToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtNull, 0);
}

void TypeNullFromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtNull, 0);
}

//*************************************************************************************************
// Boolean

void TypeBooleanToText(void *pData, QString &String)
{
    String = *(bool *)pData == true ? "true" : "false";
}

void TypeBooleanFromText(char *pString, void *pData)
{
    *(bool *)pData = stricmp(pString, "true") == 0 ? true : false; 
}

void TypeBooleanToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtBoolean, sizeof(bool));
    Stream.Write1(pData);
}

void TypeBooleanFromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtBoolean, sizeof(bool));
    Stream.Read1(pData);
}

double TypeBooleanToDouble(void *pData)
{
    return (*(bool *)pData) == true ? 1 : 0;
}

//*************************************************************************************************
// Byte

void TypeByteToText(void *pData, QString &String)
{
    uint8_t value;
    memcpy(&value, pData, sizeof(uint8_t));
    String = QString::number(value);
}

void TypeByteFromText(char *pString, void *pData)
{
    *(BYTE *)pData = (BYTE)atoi(pString);
}

void TypeByteToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtByte, sizeof(BYTE));
    Stream.Write1(pData);
}

void TypeByteFromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtByte, sizeof(BYTE));
    Stream.Read1(pData);
}

double TypeByteToDouble(void *pData)
{
    return (double)(*(BYTE *)pData);
}

//*************************************************************************************************
// Int8

void TypeInt8ToText(void *pData, QString &String)
{
    int8_t value;
    memcpy(&value, pData, sizeof(int8_t));
    String = QString::number(value);
}

void TypeInt8FromText(char *pString, void *pData)
{
    *((INT8 *)pData) = (INT8)atoi(pString);
}

void TypeInt8ToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtInt8, sizeof(INT8));
    Stream.Write1(pData);
}

void TypeInt8FromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtInt8, sizeof(INT8));
    Stream.Read1(pData);
}

double TypeInt8ToDouble(void *pData)
{
    return (double)(*(INT8 *)pData);
}

//*************************************************************************************************
// Int16

void TypeInt16ToText(void *pData, QString &String)
{
    int16_t value;
    memcpy(&value, pData, sizeof(int16_t));
    String = QString::number(value);
}

void TypeInt16FromText(char *pString, void *pData)
{
    INT16 value = (INT16)atoi(pString);
    memcpy(pData, &value, sizeof(value));
}

void TypeInt16ToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtInt16, sizeof(INT16));
    Stream.Write2(pData);
}

void TypeInt16FromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtInt16, sizeof(INT16));
    Stream.Read2(pData);
}

double TypeInt16ToDouble(void *pData)
{
    INT16 value;
    memcpy(&value, pData, sizeof(INT16));
    return (double)value;
}

//*************************************************************************************************
// Int32

void TypeInt32ToText(void *pData, QString &String)
{
    int32_t value;
    memcpy(&value, pData, sizeof(int32_t));
    String = QString::number(value);
}

void TypeInt32FromText(char *pString, void *pData)
{
    int value = (int)atoi(pString);
    memcpy(pData, &value, sizeof(value));
}

void TypeInt32ToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtInt32, sizeof(int));
    Stream.Write4(pData);
}

void TypeInt32FromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtInt32, sizeof(int));
    Stream.Read4(pData);
}

double TypeInt32ToDouble(void *pData)
{
    int value;
    memcpy(&value, pData, sizeof(int));
    return (double)value;
}

//*************************************************************************************************
// Double

void TypeDoubleToText(void *pData, QString &String)
{
    double value;
    memcpy(&value, pData, sizeof(double));
    String = QString::number(value);
}

void TypeDoubleFromText(char *pString, void *pData)
{
    double value = (double)atof(pString);
    memcpy(pData, &value, sizeof(value));
}

void TypeDoubleToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtDouble, sizeof(double));
    Stream.Write8(pData);
}

void TypeDoubleFromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtDouble, sizeof(double));
    Stream.Read8(pData);
}

double TypeDoubleToDouble(void *pData)
{
    double value;
    memcpy(&value, pData, sizeof(double));
    return (double)value;
}

//*************************************************************************************************
// Date

void TypeDateToText(void *pData, QString &String)
{
    char valueText[32];
    double dateDouble;

    FXDATETIME dateTime;
    memcpy(&dateDouble, pData, sizeof(dateDouble));
    DecodeDate(dateDouble, &dateTime);
    sprintf(valueText, "%04d/%02d/%02d", dateTime.Year, dateTime.Month, dateTime.Day);

    String = QString::fromUtf8(valueText);
}

void TypeDateFromText(char *pString, void *pData)
{
}

void TypeDateToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtDate, sizeof(DATE));
    Stream.Write8(pData);
}

void TypeDateFromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtDate, sizeof(DATE));
    Stream.Read8(pData);
}

//*************************************************************************************************
// Time

void TypeTimeToText(void *pData, QString &String)
{
    char valueText[32];

    FXDATETIME dateTime;
    double timeDouble;
    memcpy(&timeDouble, pData, sizeof(timeDouble));
    DecodeDateTime(timeDouble, &dateTime);
    sprintf(valueText, "%02d:%02d:%02d", dateTime.Hour, dateTime.Minute, dateTime.Second);

    String = QString::fromUtf8(valueText);
}

void TypeTimeFromText(char *pString, void *pData)
{
}

void TypeTimeToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtTime, sizeof(TIME));
    Stream.Write8(pData);
}

void TypeTimeFromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtTime, sizeof(TIME));
    Stream.Read8(pData);
}

//*************************************************************************************************
// DateTime

void TypeDateTimeToText(void *pData, QString &String)
{
    String = QStringLiteral("Not yet implemented");
}

void TypeDateTimeFromText(char *pString, void *pData)
{
}

void TypeDateTimeToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtDateTime, sizeof(DATETIME));
    Stream.Write8(pData);
}

void TypeDateTimeFromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtDateTime, sizeof(DATETIME));
    Stream.Read8(pData);
}

//*************************************************************************************************
// Color

void TypeColorToText(void *pData, QString &String)
{
    COLOR value;
    memcpy(&value, pData, sizeof(COLOR));
    String = QString::number(value);
}

void TypeColorFromText(char *pString, void *pData)
{
    COLOR value = (COLOR)atoi(pString);
    memcpy(pData, &value, sizeof(value));
}

void TypeColorToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtColor, sizeof(COLOR));
    Stream.Write4(pData);
}

void TypeColorFromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtColor, sizeof(COLOR));
    Stream.Read4(pData);
}

//*************************************************************************************************
// Guid

void TypeGuidToText(void *pData, QString &String)
{
    char s[39] = {0};
    GUID *g = (GUID *)pData;
    sprintf(s, 
            "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
            g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

    String = QString::fromUtf8(s);
}

void TypeGuidFromText(char *pString, void *pData)
{
}

void TypeGuidToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtGuid, sizeof(GUID));

    GUID *pGuid;
    memcpy(&pGuid, &pData, sizeof(GUID *));
    Stream.Write4(&pGuid->Data1);
    Stream.Write2(&pGuid->Data2);
    Stream.Write2(&pGuid->Data3);
    Stream.Write(&pGuid->Data4[0], sizeof(pGuid->Data4));
}

void TypeGuidFromStream(void *pData, CfxStream &Stream)
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

void TypeFontToText(void *pData, QString &String)
{
    String = QString::fromUtf8((char *)pData);
}

void TypeFontFromText(char *pString, void *pData)
{
}

void TypeFontToStream(void *pData, CfxStream &Stream)
{
    WriteType(Stream, dtFont, sizeof(FXFONT));
    Stream.Write(pData, sizeof(FXFONT));
}

void TypeFontFromStream(void *pData, CfxStream &Stream)
{
    ReadType(Stream, dtFont, sizeof(FXFONT));
    Stream.Read(pData, sizeof(FXFONT));
}

//*************************************************************************************************
// TextAnsi

void TypeTextAnsiToText(void *pData, QString &String)
{
    String = QString::fromUtf8((char *)pData);
}

void TypeTextAnsiFromText(char *pString, void *pData)
{
    FX_ASSERT(false);
}

void TypeTextAnsiToStream(void *pData, CfxStream &Stream)
{
    int size = (int) strlen((char *)pData) + 1;

    WriteType(Stream, dtTextAnsi, size);
    Stream.Write(pData, size);
}

void TypeTextAnsiFromStream(void *pData, CfxStream &Stream)
{
    int size = ReadType(Stream, dtTextAnsi, -1);

    Stream.Read(pData, size);
}

//*************************************************************************************************
// TextUtf8

void TypeTextUtf8ToText(void *pData, QString &String)
{
    String = QString::fromUtf8((char *)pData);
}

void TypeTextUtf8FromText(char *pString, void *pData)
{
    FX_ASSERT(false);
}

void TypeTextUtf8ToStream(void *pData, CfxStream &Stream)
{
    int size = (int) strlen((char *)pData) + 1;
    WriteType(Stream, dtTextUtf8, size);
    Stream.Write(pData, size);
}

void TypeTextUtf8FromStream(void *pData, CfxStream &Stream)
{
    int size = ReadType(Stream, dtTextUtf8, -1);
    Stream.Read(pData, size);
}

//*************************************************************************************************
// Binary

void TypeBinaryToText(void *pData, QString &String)
{
    String = QStringLiteral("[Binary]");
}

void TypeBinaryFromText(char *pString, void *pData)
{
    FX_ASSERT(false);
}

void TypeBinaryToStream(void *pData, CfxStream &Stream)
{
    FX_ASSERT(false);
}

void TypeBinaryFromStream(void *pData, CfxStream &Stream)
{
    FX_ASSERT(false);
}

//*************************************************************************************************
// Graphic

void TypeGraphicToText(void *pData, QString &String)
{
    String = QStringLiteral("[Graphic]");
}

void TypeGraphicFromText(char *pString, void *pData)
{
    FX_ASSERT(false);
}

void TypeGraphicToStream(void *pData, CfxStream &Stream)
{
    FX_ASSERT(false);
}

void TypeGraphicFromStream(void *pData, CfxStream &Stream)
{
    FX_ASSERT(false);
}

//*************************************************************************************************
// Sound

void TypeSoundToText(void *pData, QString &String)
{
    String = QStringLiteral("[Sound]");
}

void TypeSoundFromText(char *pString, void *pData)
{
    FX_ASSERT(false);
}

void TypeSoundToStream(void *pData, CfxStream &Stream)
{
    FX_ASSERT(false);
}

void TypeSoundFromStream(void *pData, CfxStream &Stream)
{
    FX_ASSERT(false);
}

//*************************************************************************************************
// GuidList

void TypeGuidListToText(void *pData, QString &String)
{
    String = QStringLiteral("[GuidList]");
}

void TypeGuidListFromText(char *pString, void *pData)
{
    FX_ASSERT(false);
}

void TypeGuidListToStream(void *pData, CfxStream &Stream)
{
    FX_ASSERT(false);
}

void TypeGuidListFromStream(void *pData, CfxStream &Stream)
{
    FX_ASSERT(false);
}

//*************************************************************************************************

void BinaryToStream(FXDATATYPE Type, void *pData, CfxStream &Stream)
{
    int size = Type_Size(*(void **)pData);

    WriteType(Stream, Type, size);
    if (size > 0)
    {
        Stream.Write(*(void **)pData, size);
    }
}

void BinaryFromStream(FXDATATYPE Type, void *pData, CfxStream &Stream)
{
    int size = ReadType(Stream, Type, -1);

    *(void **)pData = Type_Alloc(*(void **)pData, size);
    if (size > 0)
    {
        Stream.Read(*(void **)pData, size);
    }
}

//*************************************************************************************************
// PTextAnsi

void TypePTextAnsiToText(void *pData, QString &String)
{
    String = QString::fromUtf8(*(char **)pData);
}

void TypePTextAnsiFromText(char *pString, void *pData)
{
    *(void **)pData = Type_Alloc(*(void **)pData, (int) strlen(pString) + 1);
    strcpy(*(char **)pData, pString);
}

void TypePTextAnsiToStream(void *pData, CfxStream &Stream)
{
    int size = *(void **)pData == nullptr ? 0 : (int) strlen(*(char **)pData) + 1;

    WriteType(Stream, dtTextAnsi, size);

    if (size > 0) 
    {
        Stream.Write(*(void **)pData, size);
    }
}

void TypePTextAnsiFromStream(void *pData, CfxStream &Stream)
{
    int size = ReadType(Stream, dtTextAnsi, -1);
    *(void **)pData = Type_Alloc(*(void **)pData, size);
    Stream.Read(*(void **)pData, size);
}

//*************************************************************************************************
// PTextUtf8

void TypePTextUtf8ToText(void *pData, QString &String)
{
    FX_ASSERT(false);
    String = QString::fromUtf8(*(char **)pData);
}

void TypePTextUtf8FromText(char *pString, void *pData)
{
    *(void **)pData = Type_Alloc(*(void **)pData, (int) strlen(pString) + 1);
    strcpy(*(char **)pData, pString);
}

void TypePTextUtf8ToStream(void *pData, CfxStream &Stream)
{
    int size = *(void **)pData == nullptr ? 0 : (int) strlen(*(char **)pData) + 1;

    WriteType(Stream, dtTextUtf8, size);

    if (size > 0) 
    {
        Stream.Write(*(void **)pData, size);
    }
}

void TypePTextUtf8FromStream(void *pData, CfxStream &Stream)
{
    int size = ReadType(Stream, dtTextUtf8, -1);
    *(void **)pData = Type_Alloc(*(void **)pData, size);
    Stream.Read(*(void **)pData, size);
}

//*************************************************************************************************
// PBinary

void TypePBinaryToText(void *pData, QString &String)
{
    FX_ASSERT(false);
}

void TypePBinaryFromText(char *pString, void *pData)
{
    FX_ASSERT(false);
}

void TypePBinaryToStream(void *pData, CfxStream &Stream)
{
    BinaryToStream(dtBinary, pData, Stream);
}

void TypePBinaryFromStream(void *pData, CfxStream &Stream)
{
    BinaryFromStream(dtBinary, pData, Stream);
}

//*************************************************************************************************
// PGraphic

void TypePGraphicToText(void *pData, QString &String)
{
    TypePBinaryToText(pData, String);
}

void TypePGraphicFromText(char *pString, void *pData)
{
    TypePBinaryFromText(pString, pData);
}

void TypePGraphicToStream(void *pData, CfxStream &Stream)
{
    BinaryToStream(dtGraphic, pData, Stream);
}

void TypePGraphicFromStream(void *pData, CfxStream &Stream)
{
    BinaryFromStream(dtGraphic, pData, Stream);
}

//*************************************************************************************************
// PSound

void TypePSoundToText(void *pData, QString &String)
{
    TypePBinaryToText(pData, String);
}

void TypePSoundFromText(char *pString, void *pData)
{
    TypePBinaryFromText(pString, pData);
}

void TypePSoundToStream(void *pData, CfxStream &Stream)
{
    BinaryToStream(dtSound, pData, Stream);
}

void TypePSoundFromStream(void *pData, CfxStream &Stream)
{
    BinaryFromStream(dtSound, pData, Stream);
}

//*************************************************************************************************
// PGuidList

void TypePGuidListToText(void *pData, QString &String)
{
    TypePBinaryToText(pData, String);
}

void TypePGuidListFromText(char *pString, void *pData)
{
    TypePBinaryFromText(pString, pData);
}

void TypePGuidListToStream(void *pData, CfxStream &Stream)
{
    BinaryToStream(dtGuidList, pData, Stream);
}

void TypePGuidListFromStream(void *pData, CfxStream &Stream)
{
    BinaryFromStream(dtGuidList, pData, Stream);
}

//*************************************************************************************************
// Type support

void Type_DataToText(FXDATATYPE Type, void *pData, QString &Value)
{
    DataTypeInfo[Type].FnToText(pData, Value);
}

void Type_DataFromText(char *InValue, FXDATATYPE Type, void *pData)
{
    DataTypeInfo[Type].FnFromText(InValue, pData);
}

void Type_ToStream(FXDATATYPE Type, void *pData, CfxStream &Stream)
{
    DataTypeInfo[Type].FnToStream(pData, Stream);
}

void Type_FromStream(FXDATATYPE Type, void *pData, CfxStream &Stream)
{
    DataTypeInfo[Type].FnFromStream(pData, Stream);
}

void *Type_Alloc(void *pData, int _size)
{
    if (_size == 0) 
    {
        Type_FreeAndNil(&pData);
        return nullptr;
    }

    void *pResult = pData ? MEM_REALLOC(pData, _size) : MEM_ALLOC(_size);

    memset(pResult, 0, _size);

    return pResult;
}

void Type_FreeAndNil(void **ppData)
{
    MEM_FREE(*ppData);
    *ppData = nullptr;
}

int Type_Size(void *pData)
{
    if (pData != nullptr)
    {
        return (int) MEM_SIZE(pData);
    }
    else
    {
        return 0;
    }
}

bool Type_IsText(FXDATATYPE Type)
{
    return (Type == dtTextAnsi) ||
           (Type == dtTextUtf8) ||
           (Type == dtPTextAnsi) ||
           (Type == dtPTextUtf8);
}

int Type_StaticSize(FXDATATYPE Type)
{
    return DataTypeInfo[Type].StaticSize;
}

void Type_CreateEditor(FXDATATYPE *pType, void **ppData, int Size)
{
    FX_ASSERT(!(*pType == dtPTextAnsi));
    FX_ASSERT(!(*pType == dtPTextUtf8));
    FX_ASSERT(!(*pType == dtPBinary));
    FX_ASSERT(!(*pType == dtPGraphic));
    FX_ASSERT(!(*pType == dtPSound));
    FX_ASSERT(!(*pType == dtPGuidList));

    void *outData = nullptr;
    FXDATATYPE outType = *pType;

    switch (*pType)
    {
    case dtTextAnsi:
    case dtTextUtf8:
    case dtBinary:
    case dtGraphic:
    case dtSound:
    case dtGuidList:
        outData = Type_Alloc(nullptr, sizeof(void *));
        
        *(void **)outData = nullptr;
        if (Size > 0)
        {
            *(void **)outData = Type_Alloc(nullptr, Size);
            memmove(*(void **)outData, *ppData, Size);
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

        outData = Type_Alloc(nullptr, Size);
        if ((Size > 0) && outData && *ppData)
        {
            memmove(outData, *ppData, Size);
        }
    }
    
    *ppData = outData;
    *pType = outType;
}

void Type_FreeEditor(FXDATATYPE *pType, void **ppData)
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
        void *data = *ppData;
        MEM_FREE(*(void **)data);
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

FXVARIANT *Type_CreateVariant(FXDATATYPE Type, void *pData)
{
    CfxStream stream;
    Type_ToStream(Type, pData, stream);
    return (FXVARIANT *)stream.CloneMemory();
}

void Type_FreeVariant(FXVARIANT **ppVariant)
{
    MEM_FREE(*ppVariant);
    *ppVariant = nullptr;
}

void Type_VariantToText(FXVARIANT *pVariant, QString &Value)
{
    Type_DataToText((FXDATATYPE)(pVariant->Type), &pVariant->Data[0], Value);
}

FXVARIANT *Type_DuplicateVariant(FXVARIANT *pVariant)
{
    return Type_CreateVariant((FXDATATYPE)(pVariant->Type), &pVariant->Data[0]);
}

double Type_VariantToDouble(FXVARIANT *pVariant)
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

void Type_VariantToStream(FXVARIANT *pVariant, CfxStream &Stream)
{
    WriteType(Stream, (FXDATATYPE)pVariant->Type, pVariant->Size);
    Stream.Write(&pVariant->Data[0], pVariant->Size);
}

void Type_VariantToStream(FXVARIANT *pVariant, CfxFileStream &Stream)
{
    WriteType(Stream, (FXDATATYPE)pVariant->Type, pVariant->Size);
    Stream.Write(&pVariant->Data[0], pVariant->Size);
}

FXDATATYPE GetTypeFromString(const char *pValue)
{
    if (pValue == nullptr)
    {
        return dtNull;
    }

    if (strncmp(pValue, MEDIA_PREFIX, strlen(MEDIA_PREFIX)) != 0)
    {
        return dtTextAnsi;
    }

    if ((strstr(pValue, ".wav") != nullptr) || 
        (strstr(pValue, ".WAV") != nullptr) ||
        (strstr(pValue, ".mp3") != nullptr) ||
        (strstr(pValue, ".MP3") != nullptr))
    {
        return dtSound;
    }

    return dtGraphic;
}