#pragma once
#include "pch.h"
#include "fxBrand.h"

#define LogError(...)

#ifdef Q_OS_WIN
#pragma warning( disable : 4100)    // unreferenced parameters
#pragma warning( disable : 4189)    // variables set but not used
#pragma warning( disable : 4267)    // size_t -> ulong
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unordered_map>

#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>

#undef UNALIGNED
#define UNALIGNED

#define INT64               qint64
#define UINT64              quint64

#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define max(a,b)            (((a) > (b)) ? (a) : (b))

#define PATH_SLASH_CHAR     '/'

#if !defined(Q_OS_WIN)
    #define stricmp(a, b)       strcasecmp(a, b)
    #define itoa(a, b, c)       sprintf(b, "%d" , (int)a)
#else
    #define stricmp(a, b)       _stricmp(a, b)
    #define itoa(a, b, c)       sprintf(b, "%d" , (int)a)
#endif

#define BYTE     unsigned char
#define UINT16   quint16
#define UINT32   quint32
#define INT8     qint8
#define INT16    qint16
#define INT32    qint32
#define LONG     qint32
#define PCHAR    char *
#define INSTANCE void *
#define DATE     double
#define TIME     double
#define DATETIME double
#define COLOR    quint32
#define IEEE4    float
#define IEEE8    double

#define DBID     quint32

#define MAX_INT32           (2147483648.)
#define INVALID_DBID        (0xFFFFFFFF)
#define INVALID_INDEX       (0xFFFFFFFF)
#define SIGN(X)             (((X) == 0) ? 0 : ((X) > 0) ? 1 : -1)

#ifndef MAX_PATH
    #define MAX_PATH 256
#endif

#ifndef GUID_DEFINED
    #define GUID_DEFINED
    struct GUID
    {
        UINT32 Data1 = 0;
        UINT16 Data2 = 0;
        UINT16 Data3 = 0;
        BYTE   Data4[8] = {};
    };
#endif

#ifndef ARRAYSIZE
    #define ARRAYSIZE(X) (sizeof(X)/sizeof(X[0]))
#endif

#define FX_ASSERT(Condition)    if (!(Condition)) FxHardFault(__FILE__, __LINE__)

#define MEM_ALLOC(Size)         FxAlloc(Size)
#define MEM_REALLOC(Data, Size) FxReAlloc(Data, Size)
#undef MEM_FREE
#define MEM_FREE(Data)          if (Data) FxFree(Data)
#define MEM_SIZE(Data)          FxSize(Data)
#define MEM_CHECK(Data)         FxCheck(Data)

#define HANDLE				void *

#define CBR_110             110
#define CBR_300             300
#define CBR_600             600
#define CBR_1200            1200
#define CBR_2400            2400
#define CBR_4800            4800
#define CBR_9600            9600
#define CBR_14400           14400
#define CBR_19200           19200
#define CBR_38400           38400
#define CBR_56000           56000
#define CBR_57600           57600
#define CBR_115200          115200
#define CBR_128000          128000
#define CBR_256000          256000

#define NOPARITY            0
#define ODDPARITY           1
#define EVENPARITY          2
#define MARKPARITY          3
#define SPACEPARITY         4

#define ONESTOPBIT          0
#define ONE5STOPBITS        1
#define TWOSTOPBITS         2

#if !defined(DATABITS_5)
    #define DATABITS_5          0x0001
    #define DATABITS_6          0x0002
    #define DATABITS_7          0x0004
    #define DATABITS_8          0x0008
    #define DATABITS_16         0x0010
    #define DATABITS_16X        0x0020
#endif

#define XGUID     int
#define XFONT     int
#define XSOUND    int
#define XBITMAP   int
#define XBINARY   int
#define freeX(X)

#define ICON(x)   0

void FxHardFault(const char *pFile, int Line);

void *FxAlloc(int Bytes);
void *FxReAlloc(void *pData, int Bytes);
void FxFree(void *pData);
int FxSize(void *pData);
bool FxCheck(void *pData);

#undef _WIN32
#undef WIN32
