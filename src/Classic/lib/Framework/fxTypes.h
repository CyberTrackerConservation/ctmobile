#pragma once
#include "pch.h"
#include "fxPlatform.h"

// Magic numbers for the different resources
const int MAGIC_RESOURCE	= 'RES2';
const int MAGIC_FONT		= 'FONT';
const int MAGIC_TEXT		= 'TEXT';
const int MAGIC_SCREEN		= 'SCRE';
const int MAGIC_BITMAP		= 'BITM';
const int MAGIC_DIALOG		= 'OBJN';
const int MAGIC_OBJECT		= 'OBJH';
const int MAGIC_BINARY		= 'BINA';
const int MAGIC_SOUND		= 'SOUN';
const int MAGIC_SENDDATA	= 'FTPD';
const int MAGIC_GOTO		= 'GOTO';
const int MAGIC_MOVINGMAP  = 'MPXX';
const int MAGIC_PROFILE	= 'PROF';
const int MAGIC_TRAVELMODE = 'TRAM';
const int MAGIC_JSONID     = 'JSON';
const int MAGIC_ALERT      = 'ALRT';
const int MAGIC_FIELD      = 'FILD';

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
    int Left = 0, Top = 0, Right = 0, Bottom = 0;
};

struct FXEXTENT
{
    double XMin = 0.0, YMin = 0.0, XMax = 0.0, YMax = 0.0;
};

struct FXPOINT
{
    int X = 0, Y = 0;
};

struct FXPOINTLIST
{
    int Length = 0;
    FXPOINT *Points = nullptr;
};

struct FXLINE
{
    int X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;
};

struct FXDATETIME
{
   UINT16 Year = 0;
   UINT16 Month = 0;
   UINT16 DayOfWeek = 0;
   UINT16 Day = 0;
   UINT16 Hour = 0;
   UINT16 Minute = 0;
   UINT16 Second = 0;
   UINT16 Milliseconds = 0;
};

struct FX_COMPORT
{
    int Baud = 0;
    BYTE Parity = 0;
    BYTE Data = 0;
    BYTE Stop = 0;
    bool FlowControl = false;
};

struct FX_COMPORT_DATA
{
    BYTE PortId = 0;
    BYTE *Data = nullptr;
    int Length = 0;
};

const UINT16 GPS_MAX_SIGNAL_QUALITY = 55;

struct FXGPS_SATELLITE
{
	bool UsedInSolution = false;
	UINT16 PRN = 0;
	UINT16 SignalQuality = 0;
	UINT16 Azimuth = 0;
	UINT16 Elevation = 0;
};

enum class FXDEVICE_STATE : int8_t {NotFound=0, Detecting, Disconnected, Connected};

#define MAX_SATELLITES 36
const double GPS_DEFAULT_ACCURACY = 49.0;
enum class FXGPS_QUALITY : int8_t {None=0, Fix2D, Fix3D, Diff3D, User2D, User3D, Sim3D};

struct FXGPS_ACCURACY
{
    double DilutionOfPrecision = 0.0;
    double MaximumSpeed = 0.0;
};

struct FXGPS_POSITION
{
    FXGPS_QUALITY Quality = FXGPS_QUALITY::None;
    double Latitude = 0.0, Longitude = 0.0, Altitude = 0.0, Accuracy = 0.0;
    double Speed = 0.0;
};

struct FXGPS
{
    FXDEVICE_STATE State = FXDEVICE_STATE::NotFound;
    int TimeStamp = 0;
    FXGPS_POSITION Position;
    FXDATETIME DateTime;
    bool TimeInSync = false;
    int Heading = 0;
    BYTE SatellitesInView = 0;
    FXGPS_SATELLITE Satellites[MAX_SATELLITES];
};

enum class FXPROJECTION : int8_t
{
    DegreesMinutesSeconds = 0,
    DecimalDegrees = 1,
    UTM = 2
};

// Retained for resource file backward compatibility
#define RANGE_FLAG_RANGE                   0x00000001

struct FXRANGE_DATA
{
    int Flags = 0;
    double Range = 0.0;
    char RangeUnits = 0;
    double Speed = 0.0;
    char SpeedUnits = 0;
    double PolarBearing = 0.0, PolarInclination = 0.0, PolarRoll = 0.0;
    char PolarBearingMode = 0;
    double CartesianX = 0.0, CartesianY = 0.0, CartesianZ = 0.0, CartesianH = 0.0, CartesianV = 0.0;
    double GolfTournamentBearing = 0.0, GolfTournamentHRange = 0.0, GolfTournamentVRange = 0.0;
    double HorizontalDistance = 0.0, Azimuth = 0.0, Inclination = 0.0, CalculatedHeight = 0.0;
    char HorizontalDistanceUnits = 0, CalculatedHeightUnits = 0;
};

struct FXRANGE
{
    FXDEVICE_STATE State = FXDEVICE_STATE::NotFound;
    int TimeStamp = 0;
    FXRANGE_DATA Range;
    inline bool HasRange()
    {
        return (Range.Flags & RANGE_FLAG_RANGE) != 0;
    }
};

enum class FXIMAGE_QUALITY : int8_t
{
    Default = 0,
    VeryHigh,
    High,
    Medium,
    Low,
    VeryLow,
    UltraLow
};

// Must be packed for compatibility with the Server
#pragma pack(push, 1) 

// Define Profile
#define PROFILE_SCALE_HIT_SIZE 35
struct FXPROFILE
{
    int Magic = 0;
    int Width = 0, Height = 0, BitDepth = 0, DPI = 0;
    int ScaleX = 0, ScaleY = 0, ScaleBorder = 0, ScaleFont = 0, ScaleHitSize = 0;
    int Zoom = 0, AliasIndex = 0;
};

// Define text
struct FXTEXTRESOURCE
{
    int Magic = 0;
    GUID Guid;
    char Text[1];
};

// Define Sound
typedef BYTE * FXSOUND;
struct FXSOUNDRESOURCE
{
    int Magic = 0;
    int Size = 0;
    BYTE Data[1];
    // Note: Pixels is variable length
};

// Define Bitmap
typedef BYTE * FXBITMAP;
struct FXBITMAPRESOURCE
{
    int Magic = 0;
    UINT16 Width = 0, Height = 0, BitDepth = 0, RowBytes = 0, Compress = 0;
    int Size = 0;
    BYTE Pixels[1];
    // Note: Pixels is variable length
};

// Define binary
typedef BYTE * FXBINARY;
struct FXBINARYRESOURCE
{
    int Magic = 0;
    int BinaryMagic = 0;
    BYTE Data[1];
};

// Define Font
typedef char FXFONT[74];

// Font style
#define FXFONT_STYLE_NORMAL 0
#define FXFONT_STYLE_BOLD   1 
#define FXFONT_STYLE_ITALIC 2

struct FXFONTRESOURCE
{
    int Magic = 0;
    char Name[64] = {};
    UINT16 Format = 0;
    UINT16 Height = 0;
    UINT16 Size = 0;
    UINT16 Style = 0;
};

struct FXFIELDRESOURCE
{
    int Magic = 0;
    char Name[256] = {};
    GUID ElementId;
    UINT32 DataType = 0;
};

// Define Screen
struct FXSCREENDATA
{
    int Magic = 0;
    GUID Guid;
    BYTE Data[1];
    // Note: Data is variable length
};

// Define Screen
struct FXDIALOG
{
    int Magic = 0;
    GUID Guid;
    BYTE Data[1];
    // Note: Data is variable length
};

// Define Goto
struct FXGOTO
{
    int Magic = 0;
    char Title[28] = {};
    double X = 0.0, Y = 0.0;
};

struct FXTRAVELMODE
{
    int Magic = 0;
    FXGPS_ACCURACY SightingAccuracy, WaypointAccuracy;
};

struct FXJSONID
{
    int Magic = 0;
    char Text[1];
};

struct FXMOVINGMAP
{
    int Magic = 0;
    char FileName[256] = {};
    char Name[64] = {};
    double Xa = 0.0, Ya = 0.0, Xb = 0.0, Yb = 0.0;
    COLOR TrackColor = 0, SightingColor = 0, HistoryTrackColor = 0, HistoryColor = 0, GotoColor = 0;
    uint8_t Lock100 = 0;
    BYTE GroupId = 0;
    BYTE Padding2 = 0;
    BYTE Padding3 = 0;
};


// Define FXVARIANT
struct FXVARIANT
{
    BYTE Type = 0;  // FXDATATYPE
    int Size = 0;
    BYTE Data[1];
    // Note: Data is variable length
};

// Define FXRESOURCEHEADER
struct FXRESOURCEHEADER
{
    int Magic = 0;
	int Size = 0;
    int Version = 0;
    char Name[64] = {};
    GUID StampId;
    FXDATETIME StampTime;
    int ScaleScroller = 0, ScaleTitle = 0, ScaleTab = 0;
    int Unused1 = 0, Unused2 = 0, Unused3 = 0;
    int FirstProfile = 0;
    int ProfileCount = 0;
    int FirstObject = 0;
    int ObjectCount = 0;
    int FirstDialog = 0;
    int DialogCount = 0;
    int FirstField = 0;
    int FieldCount = 0;
    int NextScreenList = 0;
    int NextScreenCount = 0;
    int GotosOffset = 0;
    int GotosCount = 0;
	int MovingMapsOffset = 0;
	int MovingMapsCount = 0;
};

// FXOBJECTHEADER
struct FXOBJECTHEADER
{
    int Magic = 0;
    int Attribute = 0;
    int Offset = 0;
    int Size = 0;
	//BYTE Data[1];
	// Note: Data is variable length
};

struct FXDATABASEHEADER
{
    int Magic = 0;
    GUID DeviceId;
    char FileName[256] = {};
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
    int sightingCount = 0;
    int waypointCount = 0;
    FXDATETIME startTime;
    FXDATETIME stopTime;
};

struct FXSENDDATA
{
    int Magic = 0;
    int Protocol = 0;
    char Url[256] = {};
    char UserName[64] = {};
    char Password[64] = {};
};

struct FXALERT
{
    int Magic = 0;
    FXSENDDATA SendData;
    char CaId[64] = {};
    char Description[64] = {};
    char Type[64] = {};
    int Level = 0;
    int PingFrequency = 0;
    char PatrolId[64] = {};
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
    FXSENDSTATEMODE Mode = {};
    int Id = 0;
    int Progress = 0;
    char LastError[256] = {};
};

struct FXFILEMAP
{
    HANDLE FileHandle = nullptr;
    HANDLE MapHandle = nullptr;
    int FileSize = 0;
    void *BasePointer = nullptr;
};

struct FXFILE
{
    char *Name = nullptr;
    char *FullPath = nullptr;
    int Size = 0;
    FXDATETIME TimeStamp;
    HANDLE FileHandle = nullptr;
};

// Back to whatever it was
#pragma pack(pop)

// CfxStream
class CfxStream
{
protected:
    bool _ownsMemory = false;
    int _capacity = 0, _size = 0;
    void *_memory = nullptr;
    int _position = 0;

    void *ReAlloc(int *pNewCapacity);
    void SetMemory(void *pMemory, int MemorySize);
public:
    CfxStream();
    CfxStream(void *pMemory, int MemorySize = 0xFFFFFF);
    ~CfxStream();

    void Clear();
    void Peek(void *pBuffer, int Count);

    void Read(void *pBuffer, int Count);
    void Write(void *pBuffer, int Count);
    
    void Read1(void *pBuffer);
    void Read2(void *pBuffer);
    void Read4(void *pBuffer);
    void Read8(void *pBuffer);
    void Write1(void *pBuffer);
    void Write2(void *pBuffer);
    void Write4(void *pBuffer);
    void Write8(void *pBuffer);
    
    void Write(const char *pString);
    
    void LoadFromStream(CfxStream &Stream);
    void SaveToStream(CfxStream &Stream);

    void *GetMemory()  { return _memory;   }
    int GetSize()     { return _size;     }
    int GetCapacity() { return _capacity; }
    int GetPosition() { return _position; }

    void *CloneMemory();

    void SetPosition(int NewPosition);
    void SetSize(int NewSize);
    void SetCapacity(int NewCapacity);
};

// CfxFileStream
class CfxFileStream
{
protected:
    HANDLE _fileHandle = nullptr;
    char _fileName[MAX_PATH] = {};
    int _position = 0;
public:
    CfxFileStream();
    ~CfxFileStream();

    void Close();
    bool OpenForRead(char *pFileName);
    bool OpenForWrite(char *pFileName);

    bool Read(void *pBuffer, int Count);
    bool Write(void *pBuffer, int Count);
    bool Write(const char *pString);
    bool WriteNullChar();

    bool Seek(int Position);
    
    int GetSize();
    int GetPosition()  { return _position; }
    char *GetFileName() { return _fileName; }
    int GetLastError();
};

extern const GUID  GUID_0;
extern const XGUID XGUID_0;

extern void PrintF(char* S, double Value, UINT16 Size, UINT16 Precision);

extern void InitFont(FXFONT &F, const char *Value);
extern void InitFont(int &F, const char *Value);

// Type support
extern void WriteType(CfxStream &Stream, FXDATATYPE Type, int DataSize);
extern void WriteType(CfxFileStream &Stream, FXDATATYPE Type, int DataSize);

extern void Type_DataToText(FXDATATYPE Type, void *pData, QString &Value);
extern void Type_DataFromText(PCHAR InValue, FXDATATYPE Type, void *pData);
extern void Type_ToStream(FXDATATYPE Type, void *pData, CfxStream &Stream);
extern void Type_FromStream(FXDATATYPE Type, void *pData, CfxStream &Stream);
extern void * Type_Alloc(void *pData, int _size);
extern void Type_FreeAndNil(void **ppData);
extern int Type_Size(void *pData);
extern bool Type_IsText(FXDATATYPE Type);
extern int Type_StaticSize(FXDATATYPE Type);
extern void Type_CreateEditor(FXDATATYPE *pType, void **ppData, int Size);
extern void Type_FreeEditor(FXDATATYPE *pType, void **ppData);

extern FXVARIANT *Type_CreateVariant(FXDATATYPE Type, void *pData);
extern void Type_FreeVariant(FXVARIANT **ppVariant);
extern void Type_VariantToText(FXVARIANT *pVariant, QString &Value);
extern FXVARIANT *Type_DuplicateVariant(FXVARIANT *pVariant);
extern double Type_VariantToDouble(FXVARIANT *pVariant);
extern void Type_VariantToStream(FXVARIANT *pVariant, CfxStream &Stream);
extern void Type_VariantToStream(FXVARIANT *pVariant, CfxFileStream &Stream);

extern void Get16(BYTE *pDst, BYTE *pSrc);
extern void Get32(BYTE *pDst, BYTE *pSrc);
extern void Get64(BYTE *pDst, BYTE *pSrc);

extern void Swap16(BYTE *pDst, BYTE *pSrc);
extern void Swap32(BYTE *pDst, BYTE *pSrc);
extern void Swap64(BYTE *pDst, BYTE *pSrc);

extern char *AllocString(const char *pValue);
extern void TrimString(char *pValue);

extern FXDATATYPE GetTypeFromString(const char *pValue);
