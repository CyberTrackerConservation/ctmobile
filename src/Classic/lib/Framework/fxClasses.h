#pragma once
#include "pch.h"
#include "fxTypes.h"
#include "fxLists.h"

class CfxFiler;
class CfxFilerUI;
class CfxFilerResource;

// Markers for filer streams: also defined on the server side
#define FILER_MARKER_DEFINEBEGIN -2
#define FILER_MARKER_LISTEND     -1

// CfxPersistent
class CfxPersistent: public QObject
{
    Q_OBJECT

public:
    CfxPersistent(CfxPersistent *pOwner);
    virtual ~CfxPersistent() {}

    virtual void Assign(CfxPersistent *pSource);

    virtual void DefineProperties(CfxFiler &F);
    virtual void DefineState(CfxFiler &F);
    virtual void DefinePropertiesUI(CfxFilerUI &F);
    virtual void DefineResources(CfxFilerResource &F);

    virtual void OnAlarm();
    virtual void OnTimer();
    virtual void OnTrackTimer();
    virtual void OnPortData(BYTE PortId, BYTE *pData, int Length);
    virtual void OnRecordingComplete(char *pFileName, int Duration);
    virtual void OnPictureTaken(char *pFileName, bool Success);
    virtual void OnBarcodeScan(char *pBarcode, bool Success);
    virtual void OnPhoneNumberTaken(char *pPhoneNumber, bool Success);
    virtual void OnUrlReceived(const char *pFileName, int ErrorCode);

    CfxPersistent *GetOwner();
    virtual void SetOwner(CfxPersistent *pOwner);
};

// CfxFiler
class CfxFiler
{
public:
    virtual void DefineValue(const QString& name, FXDATATYPE Type, void *pData, const char *pDefault = nullptr)=0;
    virtual void DefineFileName(const QString& name, char **ppFileName)=0;
    virtual bool DefineBegin(const QString& name)=0;
    virtual void DefineEnd()=0;
    virtual bool ListEnd()=0;
    virtual bool IsReader()=0;
    virtual bool IsWriter()=0;

    void DefineVariant(const QString& name, FXVARIANT **ppVariant);
    void DefineGPS(FXGPS_POSITION *pPosition);
    void DefineDateTime(FXDATETIME *pDateTime);
    void DefineGOTO(FXGOTO *pGoto);
    void DefineCOM(BYTE *pPortId, FX_COMPORT *pComPort);
    void DefineRange(FXRANGE_DATA *pRange);
    void DefineXFONT(const QString& name, XFONT *pFont, const char *pDefault = nullptr);
    void DefineXSOUND(const QString& name, XSOUND *pSound);
    void DefineXBITMAP(const QString& name, XBITMAP *pBitmap);
    void DefineXOBJECT(const QString& name, XGUID *pObject);
    void DefineXOBJECTLIST(const QString& name, XGUIDLIST *pObjectList);
    void DefineXBINARY(const QString& name, XBINARY *pBinary);
    void DefineXFILE(const QString& name, char **ppFileName);
};

// CfxFilerUI
class CfxFilerUI
{
public:
    virtual void DefineValue(char *pCaption, FXDATATYPE Type, void *pData, const char *pParams = "")=0;
    virtual void DefineStaticLink(char *pCaption, GUID *pScreenId)=0;
    virtual void DefineDynamicLink(GUID *pObjectId, GUID *pScreenId)=0;
    virtual void DefineBegin(const QString& name)=0;
    virtual void DefineEnd()=0;

    void DefineCOM(BYTE *pPortId, FX_COMPORT *pComPort);
};

// CfxFilerResource
class CfxFilerResource
{
public:
    virtual void DefineFont(FXFONT Font)=0;
    virtual void DefineBitmap(FXBITMAP Bitmap)=0;
    virtual void DefineSound(FXSOUND Sound)=0;
    virtual void DefineObject(GUID Guid, BYTE Attribute=0)=0;
    virtual void DefineBinary(FXBINARY Binary)=0;
    virtual void DefineFile(char *pFileName)=0;
    virtual void DefineField(GUID Guid, BYTE DataType)=0;
};

// CfxReader
class CfxReader: public CfxFiler
{
protected:
    CfxStream *_stream = nullptr;
    bool ReadMarker(const int Value);
public:
    CfxReader(CfxStream *pStream);
    void DefineValue(const QString& name, FXDATATYPE Type, void *pData, const char *pDefault = nullptr);
    void DefineFileName(const QString& name, char **ppFileName);
    bool DefineBegin(const QString& name);
    void DefineEnd();
    bool ListEnd();
    bool IsReader();
    bool IsWriter();
    bool IsEnd();

    static QStringList FindStrings(const void *pData, int size, int minLength);
    static QStringList FindFileNames(const void *pData, int size);
};

// CfxReader
class CfxWriter: public CfxFiler
{
protected:
    CfxStream *_stream = nullptr;
    void WriteMarker(const int Value);
public:
    CfxWriter(CfxStream *pStream);
    void DefineValue(const QString& name, FXDATATYPE Type, void *pData, const char *pDefault = nullptr);
    void DefineFileName(const QString& name, char **ppFileName);
    bool DefineBegin(const QString& name);
    void DefineEnd();
    bool ListEnd();
    bool IsReader();
    bool IsWriter();
};

#define F_NULL             ""
#define F_TRUE             "true"
#define F_FALSE            "false"
#define F_ZERO             "0"
#define F_ONE              "1"
#define F_TWO              "2"
#define F_GUID_ZERO        "{00000000-0000-0000-0000-000000000000}"
#define F_COLOR_BLACK      "00000000"
#define F_COLOR_WHITE      "FFFFFF00"
#define F_COLOR_LIME       "00FF0000"
#define F_COLOR_RED        "FF000000"
#define F_COLOR_GRAY       "80808000"
#define F_FONT_DEFAULT_S   "Arial,8,"
#define F_FONT_DEFAULT_M   "Arial,10,"
#define F_FONT_DEFAULT_L   "Arial,12,"
#define F_FONT_DEFAULT_XL  "Arial,14,"
#define F_FONT_DEFAULT_SB  "Arial,8,B"
#define F_FONT_DEFAULT_MB  "Arial,10,B"
#define F_FONT_DEFAULT_LB  "Arial,12,B"
#define F_FONT_DEFAULT_XLB "Arial,14,B"
#define F_FONT_DEFAULT_XXLB "Arial,18,B"
#define F_FONT_FIXED_S     "Courier New,8,"
#define F_FONT_FIXED_M     "Courier New,10,"
#define F_FONT_FIXED_L     "Courier New,12,"
#define F_FONT_FIXED_XL    "Courier New,14,"
#define F_FONT_FIXED_SB    "Courier New,8,B"
#define F_FONT_FIXED_MB    "Courier New,10,B"
#define F_FONT_FIXED_LB    "Courier New,12,B"
#define F_FONT_FIXED_XLB   "Courier New,14,B"
#define F_FONT_FIXED_XXLB  "Courier New,18,B"
// CfxEventManager
typedef void (CfxPersistent::*NotifyMethod)(CfxPersistent *pSender, void *pParams);

struct FXEVENT
{
    BYTE Type = 0;
    CfxPersistent *Object = nullptr;
    NotifyMethod Method = nullptr;
};

class CfxEventManager
{
protected:
    TList<FXEVENT> _events;
public:
    CfxEventManager();
    ~CfxEventManager();
    void Clear();
    void Register(FXEVENT *pEvent);
    void Unregister(BYTE Type, CfxPersistent *Object);
    void Execute(int EventId, CfxPersistent *pSender, void *pParams = nullptr);
    void ExecuteType(BYTE Type, CfxPersistent *pSender, void *pParams = nullptr);
};

extern void ExecuteEvent(FXEVENT *pEvent, CfxPersistent *pSender, void *pParams = nullptr);
