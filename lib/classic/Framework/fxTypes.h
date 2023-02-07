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
#include "fxPlatform.h"

// Magic numbers for the different resources
const UINT MAGIC_RESOURCE	= 'RES2';
const UINT MAGIC_FONT		= 'FONT';
const UINT MAGIC_TEXT		= 'TEXT';
const UINT MAGIC_SCREEN		= 'SCRE';
const UINT MAGIC_BITMAP		= 'BITM';
const UINT MAGIC_DIALOG		= 'OBJN';
const UINT MAGIC_OBJECT		= 'OBJH';
const UINT MAGIC_BINARY		= 'BINA';
const UINT MAGIC_SOUND		= 'SOUN';
const UINT MAGIC_SENDDATA	= 'FTPD';
const UINT MAGIC_GOTO		= 'GOTO';
const UINT MAGIC_MOVINGMAP  = 'MPXX';
const UINT MAGIC_PROFILE	= 'PROF';
const UINT MAGIC_TRAVELMODE = 'TRAM';
const UINT MAGIC_JSONID     = 'JSON';
const UINT MAGIC_ALERT      = 'ALRT';
const UINT MAGIC_FIELD      = 'FILD';

#define MEDIA_PREFIX        "media://"

#define PI 	                3.1415926535898
#define	D2R	                (PI / 180.0)
#define	R2D	                (180.0 / PI)

#define DEGREE_SYMBOL_UTF8  "\xC2\xB0"

// FXDATATYPE
enum FXDATATYPE
{
    dtNull = 0,
    dtBoolean,
    dtByte,
    dtInt8, dtInt16, dtInt32, 
    dtDouble, 
    dtDate, dtTime, dtDateTime,
    dtColor,
    dtGuid,
    dtFont,

    dtTextAnsi, dtBinary, dtGraphic, dtSound, dtGuidList, dtTextUtf8,

    dtPTextAnsi, dtPBinary, dtPGraphic, dtPSound, dtPGuidList, dtPTextUtf8,

    dtMax
};

#define dtText  dtTextUtf8
#define dtPText dtPTextUtf8

// Define FXRECT, FXPOINT, FXLINE
struct FXRECT
{
    INT Left, Top, Right, Bottom;
};

struct FXEXTENT
{
    DOUBLE XMin, YMin, XMax, YMax;
};

struct FXPOINT
{
    INT X, Y;
};

struct FXPOINTLIST
{
    UINT Length;        
    FXPOINT *Points;   
};

struct FXLINE
{
    INT X1, Y1, X2, Y2;
};

struct FXDATETIME
{
   UINT16 Year;
   UINT16 Month;
   UINT16 DayOfWeek;
   UINT16 Day;
   UINT16 Hour;
   UINT16 Minute;
   UINT16 Second;
   UINT16 Milliseconds;
};

struct FX_COMPORT
{
    UINT Baud;
    BYTE Parity;
    BYTE Data;
    BYTE Stop;
    BOOL FlowControl;
};

struct FX_COMPORT_DATA
{
    BYTE PortId;
    BYTE *Data;
    UINT Length;
};

const UINT16 GPS_MAX_SIGNAL_QUALITY = 55;

struct FXGPS_SATELLITE
{
	BOOL UsedInSolution;			
	UINT16 PRN;					
	UINT16 SignalQuality;			
	UINT16 Azimuth;					
	UINT16 Elevation;				
};

enum FXDEVICE_STATE {dsNotFound=0, dsDetecting, dsDisconnected, dsConnected};

#define MAX_SATELLITES 36
const DOUBLE GPS_DEFAULT_ACCURACY = 49.0;
enum FXGPS_QUALITY {fqNone=0, fq2D, fq3D, fqDiff3D, fqUser2D, fqUser3D, fqSim3D};

struct FXGPS_ACCURACY
{
    DOUBLE DilutionOfPrecision;
    DOUBLE MaximumSpeed;
};

struct FXGPS_POSITION
{
    FXGPS_QUALITY Quality;
    DOUBLE Latitude, Longitude, Altitude, Accuracy;
    DOUBLE Speed;
};

struct FXGPS
{
    FXDEVICE_STATE State;
    UINT TimeStamp;
    FXGPS_POSITION Position;
    FXDATETIME DateTime;
    BOOL TimeInSync;
    UINT Heading;
    BYTE SatellitesInView;		
    FXGPS_SATELLITE Satellites[MAX_SATELLITES];
};

enum FXPROJECTION
{
    PR_DEGREES_MINUTES_SECONDS = 0,
    PR_DECIMAL_DEGREES = 1,
    PR_UTM = 2
};

#define RANGE_FLAG_RANGE                   0x00000001
#define RANGE_FLAG_POLAR_BEARING           0x00000002
#define RANGE_FLAG_POLAR_INCLINATION       0x00000004
#define RANGE_FLAG_POLAR_ROLL              0x00000008
#define RANGE_FLAG_CARTESIAN_X             0x00000010
#define RANGE_FLAG_CARTESIAN_Y             0x00000020
#define RANGE_FLAG_CARTESIAN_Z             0x00000040
#define RANGE_FLAG_CARTESIAN_HORIZONTAL    0x00000080
#define RANGE_FLAG_CARTESIAN_VERTICAL      0x00000100
#define RANGE_FLAG_GOLFTOURNAMENT_BEARING  0x00000200
#define RANGE_FLAG_GOLFTOURNAMENT_VRANGE   0x00000400
#define RANGE_FLAG_GOLFTOURNAMENT_HRANGE   0x00000800
#define RANGE_FLAG_HORIZONTAL_DISTANCE     0x00001000
#define RANGE_FLAG_AZIMUTH                 0x00002000
#define RANGE_FLAG_INCLINATION             0x00004000
#define RANGE_FLAG_CALCULATED_HEIGHT       0x00008000

struct FXRANGE_DATA
{
    UINT Flags;

    // Generic
    DOUBLE Range;
    CHAR RangeUnits;
    DOUBLE Speed;
    CHAR SpeedUnits;

    // LaserAtlanta: Polar, Cartesian, GolfTournament
    DOUBLE PolarBearing, PolarInclination, PolarRoll;
    CHAR PolarBearingMode;
    DOUBLE CartesianX, CartesianY, CartesianZ, CartesianH, CartesianV;
    DOUBLE GolfTournamentBearing, GolfTournamentHRange, GolfTournamentVRange;

    // TruePulse
    DOUBLE HorizontalDistance, Azimuth, Inclination, CalculatedHeight;
    CHAR HorizontalDistanceUnits, CalculatedHeightUnits;
};

struct FXRANGE
{
    FXDEVICE_STATE State;
    UINT TimeStamp;
    FXRANGE_DATA Range;
    inline BOOL HasRange()
    {
        return (Range.Flags & RANGE_FLAG_RANGE) != 0;
    }
};

enum FXIMAGE_QUALITY
{
    iqDefault = 0,
    iqVeryHigh,
    iqHigh,
    iqMedium,
    iqLow,
    iqVeryLow,
    iqUltraLow
};

// Must be packed for compatibility with the Server
#pragma pack(push, 1) 

// Define Profile
#define PROFILE_SCALE_HIT_SIZE 35
struct FXPROFILE
{
    UINT Magic;
    UINT Width, Height, BitDepth, DPI;
    UINT ScaleX, ScaleY, ScaleBorder, ScaleFont, ScaleHitSize;
    UINT Zoom, AliasIndex;
};

// Define text
struct FXTEXTRESOURCE
{
    UINT Magic;
    GUID Guid;
    CHAR Text[1];
};

// Define Sound
typedef BYTE * FXSOUND;
struct FXSOUNDRESOURCE
{
    UINT Magic;
    UINT Size;
    BYTE Data[1];
    // Note: Pixels is variable length
};

// Define Bitmap
typedef BYTE * FXBITMAP;
struct FXBITMAPRESOURCE
{
    UINT Magic;
    UINT16 Width, Height, BitDepth, RowBytes, Compress;
    UINT Size;
    BYTE Pixels[1];
    // Note: Pixels is variable length
};

// Define binary
typedef BYTE * FXBINARY;
struct FXBINARYRESOURCE
{
    UINT Magic;
    UINT BinaryMagic;
    BYTE Data[1];
};

// Define Font
typedef CHAR FXFONT[74];

// Font style
#define FXFONT_STYLE_NORMAL 0
#define FXFONT_STYLE_BOLD   1 
#define FXFONT_STYLE_ITALIC 2

struct FXFONTRESOURCE
{
    UINT Magic;
    CHAR Name[64];
    UINT16 Format;
    UINT16 Height;
    UINT16 Size;
    UINT16 Style;
};

struct FXFIELDRESOURCE
{
    UINT Magic;
    CHAR Name[256];
    GUID ElementId;
    UINT32 DataType;
};

// Define Screen
struct FXSCREENDATA
{
    UINT Magic;
    GUID Guid;
    BYTE Data[1];
    // Note: Data is variable length
};

// Define Screen
struct FXDIALOG
{
    UINT Magic;
    GUID Guid;
    BYTE Data[1];
    // Note: Data is variable length
};

// Define Goto
struct FXGOTO
{
    UINT Magic;
    CHAR Title[28];
    DOUBLE X, Y;
};

struct FXTRAVELMODE
{
    UINT Magic;
    FXGPS_ACCURACY SightingAccuracy, WaypointAccuracy;
};

struct FXJSONID
{
    UINT Magic;
    CHAR Text[1];
};

struct FXMOVINGMAP
{
    UINT Magic;
    CHAR FileName[256];
    CHAR Name[64];
    DOUBLE Xa, Ya, Xb, Yb;
    COLOR TrackColor, SightingColor, HistoryTrackColor, HistoryColor, GotoColor;
    BOOL Lock100;
    BYTE GroupId;
    BYTE Padding2;
    BYTE Padding3;
};


// Define FXVARIANT
struct FXVARIANT
{
    BYTE Type;  // FXDATATYPE
    UINT Size;
    BYTE Data[1];
    // Note: Data is variable length
};

// Define FXRESOURCEHEADER
struct FXRESOURCEHEADER
{
    UINT Magic;
	UINT Size;
    UINT Version;
    CHAR Name[64];
    GUID StampId;
    FXDATETIME StampTime;
    UINT ScaleScroller, ScaleTitle, ScaleTab;
    UINT Unused1, Unused2, Unused3;
    UINT FirstProfile;
    UINT ProfileCount;
    UINT FirstObject;
    UINT ObjectCount;
    UINT FirstDialog;
    UINT DialogCount;
    UINT FirstField;
    UINT FieldCount;
    UINT NextScreenList;
    UINT NextScreenCount;
    UINT GotosOffset;
    UINT GotosCount;
	UINT MovingMapsOffset;
	UINT MovingMapsCount;
};

// FXOBJECTHEADER
struct FXOBJECTHEADER
{
    UINT Magic;
    UINT Attribute;
    UINT Offset;
    UINT Size;
	//BYTE Data[1];
	// Note: Data is variable length
};

struct FXDATABASEHEADER
{
    UINT Magic;
    GUID DeviceId;
    CHAR FileName[256];
};

#define FXSENDDATA_PROTOCOL_UNKNOWN 0
#define FXSENDDATA_PROTOCOL_FTP     1
#define FXSENDDATA_PROTOCOL_UNC     2
#define FXSENDDATA_PROTOCOL_HTTP    3
#define FXSENDDATA_PROTOCOL_HTTPS   4
#define FXSENDDATA_PROTOCOL_BACKUP  5
#define FXSENDDATA_PROTOCOL_HTTPZ   6
#define FXSENDDATA_PROTOCOL_ESRI    7
#define FXSENDDATA_PROTOCOL_AZURE   8

struct FXEXPORTFILEINFO
{
    UINT sightingCount;
    UINT waypointCount;
    FXDATETIME startTime;
    FXDATETIME stopTime;
};

struct FXSENDDATA
{
    UINT Magic;
    UINT Protocol;
    CHAR Url[256];
    CHAR UserName[64];
    CHAR Password[64];
};

struct FXALERT
{
    UINT Magic;
    FXSENDDATA SendData;
    CHAR CaId[64];
    CHAR Description[64];
    CHAR Type[64];
    UINT Level;
    UINT PingFrequency;
    CHAR PatrolId[64];
};

enum FXSENDSTATEMODE
{
    SS_IDLE = 0,
    SS_BLOCKED,
    SS_STOPPED,
    SS_CONNECTING,
    SS_FAILED_CONNECT,
    SS_SENDING,
    SS_FAILED_SEND,
    SS_COMPLETED,
    SS_AUTH_START,
    SS_AUTH_SUCCESS,
    SS_AUTH_FAILURE,
    SS_NO_WIFI
};

struct FXSENDSTATE
{
    FXSENDSTATEMODE Mode;
    UINT Id;
    UINT Progress;
    CHAR LastError[256];
};

struct FXFILEMAP
{
    HANDLE FileHandle;
    HANDLE MapHandle;
    UINT FileSize;
    VOID *BasePointer;
};

struct FXFILE 
{
    CHAR *Name;
    CHAR *FullPath;
    UINT Size;
    FXDATETIME TimeStamp;
    HANDLE FileHandle;
};

// Back to whatever it was
#pragma pack(pop)

// CfxStream
class CfxStream
{
protected:
    BOOL _ownsMemory;
    UINT _capacity, _size;
    VOID *_memory;
    UINT _position;

    VOID *ReAlloc(UINT *pNewCapacity);
    VOID SetMemory(VOID *pMemory, UINT MemorySize);
public:
    CfxStream();
    CfxStream(VOID *pMemory, UINT MemorySize = 0xFFFFFF);
    ~CfxStream();

    VOID Clear();
    VOID Peek(VOID *pBuffer, UINT Count);

    VOID Read(VOID *pBuffer, UINT Count);
    VOID Write(VOID *pBuffer, UINT Count);
    
    VOID Read1(VOID *pBuffer);
    VOID Read2(VOID *pBuffer);
    VOID Read4(VOID *pBuffer);
    VOID Read8(VOID *pBuffer);
    VOID Write1(VOID *pBuffer);
    VOID Write2(VOID *pBuffer);
    VOID Write4(VOID *pBuffer);
    VOID Write8(VOID *pBuffer);
    
    VOID Write(const CHAR *pString);
    
    VOID LoadFromStream(CfxStream &Stream);
    VOID SaveToStream(CfxStream &Stream);

    VOID *GetMemory()  { return _memory;   }
    UINT GetSize()     { return _size;     }
    UINT GetCapacity() { return _capacity; }
    UINT GetPosition() { return _position; }

    VOID *CloneMemory();

    VOID SetPosition(UINT NewPosition);
    VOID SetSize(UINT NewSize);
    VOID SetCapacity(UINT NewCapacity);
};

// CfxFileStream
class CfxFileStream
{
protected:
    HANDLE _fileHandle;
    CHAR _fileName[MAX_PATH];
    UINT _position;
public:
    CfxFileStream();
    ~CfxFileStream();

    VOID Close();
    BOOL OpenForRead(CHAR *pFileName);
    BOOL OpenForWrite(CHAR *pFileName);

    BOOL Read(VOID *pBuffer, UINT Count);
    BOOL Write(VOID *pBuffer, UINT Count);
    BOOL Write(const CHAR *pString);
    BOOL WriteNullChar();

    BOOL Seek(UINT Position);
    
    UINT GetSize();
    UINT GetPosition()  { return _position; }
    CHAR *GetFileName() { return _fileName; }
    UINT GetLastError();
};

// CfxString
class CfxString
{
protected:
    CHAR *_string;
public:
    CfxString();
    CfxString(const CHAR *pValue, UINT Count=0);
    ~CfxString();
    CHAR *Get();
    VOID Set(const CHAR *pValue, UINT Count=0);
    BOOL Compare(const CHAR *pValue);
    UINT Length();
};

extern const GUID  GUID_0;
extern const XGUID XGUID_0;

extern VOID PrintF(CHAR* S, DOUBLE Value, UINT16 Size, UINT16 Precision);

extern VOID InitFont(FXFONT &F, const CHAR *Value);
extern VOID InitFont(UINT &F, const CHAR *Value);

// Type support
extern VOID WriteType(CfxStream &Stream, FXDATATYPE Type, UINT DataSize);
extern VOID WriteType(CfxFileStream &Stream, FXDATATYPE Type, UINT DataSize);

extern VOID Type_DataToText(FXDATATYPE Type, VOID *pData, CfxString &Value);
extern VOID Type_DataFromText(PCHAR InValue, FXDATATYPE Type, VOID *pData);
extern VOID Type_ToStream(FXDATATYPE Type, VOID *pData, CfxStream &Stream);
extern VOID Type_FromStream(FXDATATYPE Type, VOID *pData, CfxStream &Stream);
extern VOID * Type_Alloc(VOID *pData, UINT _size);
extern VOID Type_FreeAndNil(VOID **ppData);
extern UINT Type_Size(VOID *pData);
extern BOOL Type_IsText(FXDATATYPE Type);
extern INT Type_StaticSize(FXDATATYPE Type);
extern VOID Type_CreateEditor(FXDATATYPE *pType, VOID **ppData, UINT Size);
extern VOID Type_FreeEditor(FXDATATYPE *pType, VOID **ppData);

extern FXVARIANT *Type_CreateVariant(FXDATATYPE Type, VOID *pData);
extern VOID Type_FreeVariant(FXVARIANT **ppVariant);
extern VOID Type_VariantToText(FXVARIANT *pVariant, CfxString &Value);
extern FXVARIANT *Type_DuplicateVariant(FXVARIANT *pVariant);
extern DOUBLE Type_VariantToDouble(FXVARIANT *pVariant);
extern VOID Type_VariantToStream(FXVARIANT *pVariant, CfxStream &Stream);
extern VOID Type_VariantToStream(FXVARIANT *pVariant, CfxFileStream &Stream);

extern VOID Get16(BYTE *pDst, BYTE *pSrc);
extern VOID Get32(BYTE *pDst, BYTE *pSrc);
extern VOID Get64(BYTE *pDst, BYTE *pSrc);

extern VOID Swap16(BYTE *pDst, BYTE *pSrc);
extern VOID Swap32(BYTE *pDst, BYTE *pSrc);
extern VOID Swap64(BYTE *pDst, BYTE *pSrc);

extern CHAR *AllocString(const CHAR *pValue);
extern VOID TrimString(CHAR *pValue);

extern FXDATATYPE GetTypeFromString(const CHAR *pValue);
