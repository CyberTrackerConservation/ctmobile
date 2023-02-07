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

#include "fxClasses.h"
#include "fxUtils.h"

//*************************************************************************************************
// CfxPersistent

CfxPersistent::CfxPersistent(CfxPersistent *pOwner)
{
    _owner = pOwner;
}

VOID CfxPersistent::Assign(CfxPersistent *pSource)
{
    CfxStream stream;
    CfxReader reader(&stream);
    CfxWriter writer(&stream);

    // Write out source
    pSource->DefineProperties(writer);
    pSource->DefineState(writer);

    // Read back
    stream.SetPosition(0);
    DefineProperties(reader);
    DefineState(reader);
}

VOID CfxPersistent::DefineProperties(CfxFiler &F)
{
}

VOID CfxPersistent::DefineState(CfxFiler &F)
{
}

VOID CfxPersistent::DefinePropertiesUI(CfxFilerUI &F)
{
}

VOID CfxPersistent::DefineResources(CfxFilerResource &F)
{
}

VOID CfxPersistent::OnAlarm()
{
}

VOID CfxPersistent::OnTimer()
{
}

VOID CfxPersistent::OnTrackTimer()
{
}

VOID CfxPersistent::OnPortData(BYTE PortId, BYTE *pData, UINT Length)
{
}

VOID CfxPersistent::OnTransfer(FXSENDSTATEMODE Mode)
{
}

VOID CfxPersistent::OnRecordingComplete(CHAR *pFileName, UINT Duration)
{
}

VOID CfxPersistent::OnPictureTaken(CHAR *pFileName, BOOL Success)
{
}

VOID CfxPersistent::OnBarcodeScan(CHAR *pBarcode, BOOL Success)
{
}

VOID CfxPersistent::OnPhoneNumberTaken(CHAR *pPhoneNumber, BOOL Success)
{
}

VOID CfxPersistent::OnUrlReceived(const CHAR *pFileName, UINT ErrorCode)
{
}

CfxPersistent *CfxPersistent::GetOwner()
{
    return _owner;
}

VOID CfxPersistent::SetOwner(CfxPersistent *pOwner)
{
    _owner = pOwner;
}

//*************************************************************************************************
// CfxFiler

VOID CfxFiler::DefineVariant(CHAR *pName, FXVARIANT **ppVariant)
{
    FXDATATYPE type = dtNull;
    VOID *data = NULL;

    DefineBegin(pName);
    if (IsReader())
    {
        Type_FreeVariant(ppVariant);
        DefineValue("Type", dtInt8, &type);
        Type_CreateEditor(&type, &data, 0);
        DefineValue("Data", type, data);
        *ppVariant = Type_CreateVariant(type, data);
        Type_FreeEditor(&type, &data);
    }
    else
    {
        type = (FXDATATYPE)((*ppVariant)->Type);
        data = &(*ppVariant)->Data;

        DefineValue("Type", dtInt8,  &type);
        DefineValue("Data", type,    data);
    }
    DefineEnd();
}

VOID CfxFiler::DefineGPS(FXGPS_POSITION *pPosition)
{
    DefineValue("Quality",   dtByte,   &pPosition->Quality);
    DefineValue("Latitude",  dtDouble, &pPosition->Latitude);
    DefineValue("Longitude", dtDouble, &pPosition->Longitude);
    DefineValue("Altitude",  dtDouble, &pPosition->Altitude);
    DefineValue("Accuracy",  dtDouble, &pPosition->Accuracy);
    DefineValue("Speed",     dtDouble, &pPosition->Speed);
}

VOID CfxFiler::DefineDateTime(FXDATETIME *pDateTime)
{
    DefineValue("Year",        dtInt16, &pDateTime->Year);
    DefineValue("Month",       dtInt16, &pDateTime->Month);
    DefineValue("Day",         dtInt16, &pDateTime->Day);
    DefineValue("Hour",        dtInt16, &pDateTime->Hour);
    DefineValue("Minute",      dtInt16, &pDateTime->Minute);
    DefineValue("Second",      dtInt16, &pDateTime->Second);
    DefineValue("MilliSecond", dtInt16, &pDateTime->Milliseconds);

    if (IsReader())
    {
        pDateTime->DayOfWeek = 0;
    }
}

VOID CfxFiler::DefineGOTO(FXGOTO *pGoto)
{
    CHAR *title = NULL;
    if (IsReader())
    {
        memset(pGoto, 0, sizeof(FXGOTO));
        DefineValue("Title", dtPText, &title);
        if (title)
        {
            strncpy(pGoto->Title, title, ARRAYSIZE(pGoto->Title) - 1);
        }
    }
    else
    {
        title = AllocString(pGoto->Title);
        DefineValue("Title", dtPText, &title);
    }
    MEM_FREE(title);
    title = NULL;

    DefineValue("X", dtDouble, &pGoto->X);
    DefineValue("Y", dtDouble, &pGoto->Y);
}

VOID CfxFiler::DefineCOM(BYTE *pPortId, FX_COMPORT *pComPort)
{
    DefineValue("PortId", dtByte, pPortId, "1");
    DefineValue("Baud", dtInt32, &pComPort->Baud, "19200");
    DefineValue("Parity", dtByte, &pComPort->Parity, "0");
    DefineValue("Data", dtByte, &pComPort->Data, "8");
    DefineValue("Stop", dtByte, &pComPort->Stop, "0");
    DefineValue("FlowControl", dtBoolean, &pComPort->FlowControl, F_FALSE);
}

VOID CfxFiler::DefineRange(FXRANGE_DATA *pRange)
{
    DefineValue("Flags",              dtInt32,   &pRange->Flags);
    DefineValue("Range",              dtDouble,  &pRange->Range);
    DefineValue("RangeUnits",         dtByte,    &pRange->RangeUnits);
    DefineValue("Speed",              dtDouble,  &pRange->Speed);
    DefineValue("SpeedUnits",         dtByte,    &pRange->SpeedUnits);

    DefineValue("PolarBearing",       dtDouble,  &pRange->PolarBearing);
    DefineValue("PolarBearingMode",   dtByte,    &pRange->PolarBearingMode);
    DefineValue("PolarInclination",   dtDouble,  &pRange->PolarInclination);
    DefineValue("PolarRoll",          dtDouble,  &pRange->PolarRoll);

    DefineValue("CartesianX",         dtDouble,  &pRange->CartesianX);
    DefineValue("CartesianY",         dtDouble,  &pRange->CartesianY);
    DefineValue("CartesianZ",         dtDouble,  &pRange->CartesianZ);
    DefineValue("CartesianH",         dtDouble,  &pRange->CartesianH);
    DefineValue("CartesianV",         dtDouble,  &pRange->CartesianV);

    DefineValue("GolfTournamentBearing", dtDouble, &pRange->GolfTournamentBearing);
    DefineValue("GolfTournamentHRange",  dtDouble, &pRange->GolfTournamentHRange);
    DefineValue("GolfTournamentVRange",  dtDouble, &pRange->GolfTournamentVRange);

    DefineValue("Azimuth",            dtDouble,  &pRange->Azimuth);
    DefineValue("Inclination",        dtDouble,  &pRange->Inclination);
    DefineValue("HorizontalDistance", dtDouble,  &pRange->HorizontalDistance);
    DefineValue("CalculatedHeight",   dtDouble,  &pRange->CalculatedHeight);
    
    DefineValue("HorizontalDistanceUnits", dtByte, &pRange->HorizontalDistanceUnits);
    DefineValue("CalculatedHeightUnits",   dtByte, &pRange->CalculatedHeightUnits);
}

#ifdef CLIENT_DLL
//
// Hack to workaround an old bad design decision to put attributes that require translation within
// a "Translate" DefineBegin/DefineEnd block. This causes problems with re-ordering among other things. 
// Now, we prepend "Translate__" to the property name and this tells the server to do the necessary 
// work.
//
VOID LegacyDefineValue(CfxFiler *pFiler, CHAR *pName, FXDATATYPE DataType, VOID *pData, const CHAR *pDefault = NULL)
{
    if (pFiler->IsReader() && pFiler->DefineBegin("Translate"))
    {
        pFiler->DefineValue(pName, DataType, pData);
        pFiler->DefineEnd();
    }
    else
    {
        CHAR name[256];
        sprintf(name, "Translate__%s", pName);
        pFiler->DefineValue(name, DataType, pData, pDefault);
    }

    // Hack to fix up uninitialized guids
    if (DataType == dtGuid)
    {
        const GUID GUID_BAD = {0xCDCDCDCD, 0xCDCD, 0xCDCD, {0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD}};
        if (memcmp(pData, &GUID_BAD, sizeof(GUID)) == 0)
        {
            *(GUID *)pData = GUID_0;
        }
    }
}

VOID CfxFilerUI::DefineCOM(BYTE *pPortId, FX_COMPORT *pComPort)
{
    CHAR items[256];
    sprintf(items, "LIST(2400=%d 4800=%d 9600=%d 19200=%d 38400=%d 57600=%d 115200=%d)", CBR_2400, CBR_4800, CBR_9600, CBR_19200, CBR_38400, CBR_57600, CBR_115200);
    DefineValue("COM - Baud rate", dtInt32, &pComPort->Baud, items);
    sprintf(items, "LIST(7=%d 8=%d)", DATABITS_7, DATABITS_8);
    DefineValue("COM - Data bits", dtByte, &pComPort->Data, items);
    DefineValue("COM - Flow control", dtBoolean, &pComPort->FlowControl);
    sprintf(items, "LIST(COM0=%d COM1=%d COM2=%d COM3=%d COM4=%d COM5=%d COM6=%d COM7=%d COM8=%d COM9=%d COM10=%d COM11=%d COM12=%d COM13=%d COM14=%d COM15=%d COM16=%d COM17=%d COM18=%d COM19=%d COM20=%d)", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
    DefineValue("COM - Port", dtByte, pPortId, items);
    sprintf(items, "LIST(None=%d Odd=%d Even=%d)", NOPARITY, ODDPARITY, EVENPARITY);
    DefineValue("COM - Parity", dtByte, &pComPort->Parity, items); 
    sprintf(items, "LIST(1=%d 2=%d)", ONESTOPBIT, TWOSTOPBITS);
    DefineValue("COM - Stop bits", dtByte, &pComPort->Stop, items);
}

#endif

VOID CfxFiler::DefineXFONT(CHAR *pName, XFONT *pFont, const CHAR *pDefault)
{
#ifdef CLIENT_DLL
    LegacyDefineValue(this, pName, dtFont, pFont, pDefault);
#else
    DefineValue(pName, dtInt32, pFont);
#endif
}

VOID CfxFiler::DefineXSOUND(CHAR *pName, XSOUND *pSound)
{
#ifdef CLIENT_DLL
    LegacyDefineValue(this, pName, dtPSound, pSound);
#else
    DefineValue(pName, dtInt32, pSound);
#endif
}

VOID CfxFiler::DefineXBITMAP(CHAR *pName, XBITMAP *pBitmap)
{
#ifdef CLIENT_DLL
    LegacyDefineValue(this, pName, dtPGraphic, pBitmap);
#else
    DefineValue(pName, dtInt32, pBitmap);
#endif
}

VOID CfxFiler::DefineXOBJECT(CHAR *pName, XGUID *pObject)
{
#ifdef CLIENT_DLL
    LegacyDefineValue(this, pName, dtGuid, pObject, F_GUID_ZERO);
#else
    DefineValue(pName, dtInt32, pObject);
#endif
}

VOID CfxFiler::DefineXOBJECTLIST(CHAR *pName, XGUIDLIST *pObjectList)
{
#ifdef CLIENT_DLL
    LegacyDefineValue(this, pName, dtPGuidList, pObjectList->GetMemoryPtr());
#else
    DefineValue(pName, dtPBinary, pObjectList->GetMemoryPtr());
#endif
}

VOID CfxFiler::DefineXBINARY(CHAR *pName, XBINARY *pBinary)
{
#ifdef CLIENT_DLL
    LegacyDefineValue(this, pName, dtPBinary, pBinary);
#else
    DefineValue(pName, dtInt32, pBinary);
#endif
}

VOID CfxFiler::DefineXFILE(CHAR *pName, CHAR **ppFileName)
{
#ifdef CLIENT_DLL
    CHAR name[256];
    sprintf(name, "Translate__%s", pName);
    DefineFileName(name, ppFileName);
#else
    DefineFileName(pName, ppFileName);
#endif
}

//*************************************************************************************************
// CfxReader

CfxReader::CfxReader(CfxStream *pStream)
{
    _stream = pStream;
}

BOOL CfxReader::ReadMarker(const UINT Value)
{
    UINT p;
    UINT i;

    p = _stream->GetPosition();
    _stream->Read(&i, sizeof(i));

    if (i == Value)
    {
        return TRUE;
    }
    else
    {
        _stream->SetPosition(p);
        return FALSE;
    }
}

VOID CfxReader::DefineValue(CHAR *pName, FXDATATYPE Type, VOID *pData, const CHAR *pDefault)
{
    Type_FromStream(Type, pData, *_stream);
}

VOID CfxReader::DefineFileName(CHAR *pName, CHAR **ppFileName)
{
    DefineValue(pName, dtPTextAnsi, ppFileName);
}

BOOL CfxReader::DefineBegin(CHAR *pName)
{
    return ReadMarker(FILER_MARKER_DEFINEBEGIN);
}

VOID CfxReader::DefineEnd()
{
}

BOOL CfxReader::ListEnd()
{
    return ReadMarker(FILER_MARKER_LISTEND);
}

BOOL CfxReader::IsReader()
{
    return TRUE;
}

BOOL CfxReader::IsWriter()
{
    return FALSE;
}

BOOL CfxReader::IsEnd()
{
    return _stream->GetPosition() == _stream->GetSize();
}

//*************************************************************************************************
// CfxWriter

CfxWriter::CfxWriter(CfxStream *pStream)
{
    _stream = pStream;
}

VOID CfxWriter::WriteMarker(const UINT Value)
{
    UINT i = Value;
    _stream->Write(&i, sizeof(i));
}

VOID CfxWriter::DefineValue(CHAR *pName, FXDATATYPE Type, VOID *pData, const CHAR *pDefault)
{
    Type_ToStream(Type, pData, *_stream);
}

VOID CfxWriter::DefineFileName(CHAR *pName, CHAR **ppFileName)
{
    DefineValue(pName, dtPTextAnsi, ppFileName);
}

BOOL CfxWriter::DefineBegin(CHAR *pName)
{
    WriteMarker(FILER_MARKER_DEFINEBEGIN);
    return TRUE;
}

VOID CfxWriter::DefineEnd()
{
}

BOOL CfxWriter::ListEnd()
{
    WriteMarker(FILER_MARKER_LISTEND);
    return TRUE;
}

BOOL CfxWriter::IsReader()
{
    return FALSE;
}

BOOL CfxWriter::IsWriter()
{
    return TRUE;
}


//*************************************************************************************************
// CfxEventManager

VOID ExecuteEvent(FXEVENT *pEvent, CfxPersistent *pSender, VOID *pParams)
{
    (pEvent->Object->*pEvent->Method)(pSender, pParams);
}

CfxEventManager::CfxEventManager(): _events()
{
}

CfxEventManager::~CfxEventManager()
{
    for (UINT i=0; i<_events.GetCount(); i++)
    {
        FX_ASSERT(!((FXEVENT *)_events.GetPtr(i))->Object); // Event not unregistered
    }
}

VOID CfxEventManager::Clear()
{
    _events.Clear();
}

VOID CfxEventManager::Register(FXEVENT *pEvent)
{
    for (UINT i=0; i<_events.GetCount(); i++)
    {
        if (!((FXEVENT *)_events.GetPtr(i))->Object)
        {
            _events.Put(i, *pEvent);
            return;
        }
    }

    _events.Add(*pEvent);
}

VOID CfxEventManager::Unregister(BYTE Type, CfxPersistent *Object)
{
    for (UINT i=0; i<_events.GetCount(); i++)
    {
        FXEVENT *event = _events.GetPtr(i);

        if ((event->Object == Object) && (event->Type == Type))
        {
            event->Object = NULL;
            return;
        }
    }

    // Overactive?
    FX_ASSERT(FALSE); // Invalid Event
}

VOID CfxEventManager::Execute(UINT EventId, CfxPersistent *pSender, VOID *pParams)
{
    FX_ASSERT((EventId >=0) && (EventId < _events.GetCount())); // Invalid Event Id
    
    ExecuteEvent(_events.GetPtr(EventId), pSender, pParams);
}

VOID CfxEventManager::ExecuteType(BYTE Type, CfxPersistent *pSender, VOID *pParams)
{
    for (UINT i=0; i<_events.GetCount(); i++)
    {
        FXEVENT *event = _events.GetPtr(i);
        if (event->Object && (event->Type == Type))
        {
            ExecuteEvent(event, pSender, pParams);
        }
    }
}
