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

protected:
    CfxPersistent *_owner;
public:
    CfxPersistent(CfxPersistent *pOwner);
    virtual ~CfxPersistent() {}

    virtual VOID Assign(CfxPersistent *pSource);

    virtual VOID DefineProperties(CfxFiler &F);
    virtual VOID DefineState(CfxFiler &F);
    virtual VOID DefinePropertiesUI(CfxFilerUI &F);
    virtual VOID DefineResources(CfxFilerResource &F);

    virtual VOID OnAlarm();
    virtual VOID OnTimer();
    virtual VOID OnTrackTimer();
    virtual VOID OnPortData(BYTE PortId, BYTE *pData, UINT Length);
    virtual VOID OnTransfer(FXSENDSTATEMODE Mode);
    virtual VOID OnRecordingComplete(CHAR *pFileName, UINT Duration);
    virtual VOID OnPictureTaken(CHAR *pFileName, BOOL Success);
    virtual VOID OnBarcodeScan(CHAR *pBarcode, BOOL Success);
    virtual VOID OnPhoneNumberTaken(CHAR *pPhoneNumber, BOOL Success);
    virtual VOID OnUrlReceived(const CHAR *pFileName, UINT ErrorCode);

    CfxPersistent *GetOwner();
    virtual VOID SetOwner(CfxPersistent *pOwner);
};

// CfxFiler
class CfxFiler
{
public:
    virtual VOID DefineValue(CHAR *pName, FXDATATYPE Type, VOID *pData, const CHAR *pDefault = NULL)=0;
    virtual VOID DefineFileName(CHAR *pName, CHAR **ppFileName)=0;
    virtual BOOL DefineBegin(CHAR *pName)=0;
    virtual VOID DefineEnd()=0;
    virtual BOOL ListEnd()=0;
    virtual BOOL IsReader()=0;
    virtual BOOL IsWriter()=0;

    VOID DefineVariant(CHAR *pName, FXVARIANT **ppVariant);
    VOID DefineGPS(FXGPS_POSITION *pPosition);
    VOID DefineDateTime(FXDATETIME *pDateTime);
    VOID DefineGOTO(FXGOTO *pGoto);
    VOID DefineCOM(BYTE *pPortId, FX_COMPORT *pComPort);
    VOID DefineRange(FXRANGE_DATA *pRange);
    VOID DefineXFONT(CHAR *pName, XFONT *pFont, const CHAR *pDefault = NULL);
    VOID DefineXSOUND(CHAR *pName, XSOUND *pSound);
    VOID DefineXBITMAP(CHAR *pName, XBITMAP *pBitmap);
    VOID DefineXOBJECT(CHAR *pName, XGUID *pObject);
    VOID DefineXOBJECTLIST(CHAR *pName, XGUIDLIST *pObjectList);
    VOID DefineXBINARY(CHAR *pName, XBINARY *pBinary);
    VOID DefineXFILE(CHAR *pName, CHAR **ppFileName);
};

// CfxFilerUI
class CfxFilerUI
{
public:
    virtual VOID DefineValue(CHAR *pCaption, FXDATATYPE Type, VOID *pData, const CHAR *pParams = "")=0;
    virtual VOID DefineStaticLink(CHAR *pCaption, GUID *pScreenId)=0;
    virtual VOID DefineDynamicLink(GUID *pObjectId, GUID *pScreenId)=0;
    virtual VOID DefineBegin(CHAR *pName)=0;
    virtual VOID DefineEnd()=0;

    VOID DefineCOM(BYTE *pPortId, FX_COMPORT *pComPort);
};

// CfxFilerResource
class CfxFilerResource
{
public:
    virtual VOID DefineFont(FXFONT Font)=0;
    virtual VOID DefineBitmap(FXBITMAP Bitmap)=0;
    virtual VOID DefineSound(FXSOUND Sound)=0;
    virtual VOID DefineObject(GUID Guid, BYTE Attribute=0)=0;
    virtual VOID DefineBinary(FXBINARY Binary)=0;
    virtual VOID DefineFile(CHAR *pFileName)=0;
    virtual VOID DefineField(GUID Guid, BYTE DataType)=0;
};

// CfxReader
class CfxReader: public CfxFiler
{
protected:
    CfxStream *_stream;
    BOOL ReadMarker(const UINT Value);
public:
    CfxReader(CfxStream *pStream);
    VOID DefineValue(CHAR *pName, FXDATATYPE Type, VOID *pData, const CHAR *pDefault = NULL);
    VOID DefineFileName(CHAR *pName, CHAR **ppFileName);
    BOOL DefineBegin(CHAR *pName);
    VOID DefineEnd();
    BOOL ListEnd();
    BOOL IsReader();
    BOOL IsWriter();
    BOOL IsEnd();
};

// CfxReader
class CfxWriter: public CfxFiler
{
protected:
    CfxStream *_stream;
    VOID WriteMarker(const UINT Value);
public:
    CfxWriter(CfxStream *pStream);
    VOID DefineValue(CHAR *pName, FXDATATYPE Type, VOID *pData, const CHAR *pDefault = NULL);
    VOID DefineFileName(CHAR *pName, CHAR **ppFileName);
    BOOL DefineBegin(CHAR *pName);
    VOID DefineEnd();
    BOOL ListEnd();
    BOOL IsReader();
    BOOL IsWriter();
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
typedef VOID (CfxPersistent::*NotifyMethod)(CfxPersistent *pSender, VOID *pParams);

struct FXEVENT
{
    BYTE Type;
    CfxPersistent *Object;
    NotifyMethod Method;
};

class CfxEventManager
{
protected:
    TList<FXEVENT> _events;
public:
    CfxEventManager();
    ~CfxEventManager();
    VOID Clear();
    VOID Register(FXEVENT *pEvent);
    VOID Unregister(BYTE Type, CfxPersistent *Object);
    VOID Execute(UINT EventId, CfxPersistent *pSender, VOID *pParams = NULL);
    VOID ExecuteType(BYTE Type, CfxPersistent *pSender, VOID *pParams = NULL);
};

extern VOID ExecuteEvent(FXEVENT *pEvent, CfxPersistent *pSender, VOID *pParams = NULL);
