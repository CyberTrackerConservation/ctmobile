#include "fxClasses.h"
#include "fxUtils.h"

//*************************************************************************************************
// CfxPersistent

CfxPersistent::CfxPersistent(CfxPersistent *pOwner): QObject(pOwner)
{
}

void CfxPersistent::Assign(CfxPersistent *pSource)
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

void CfxPersistent::DefineProperties(CfxFiler &F)
{
}

void CfxPersistent::DefineState(CfxFiler &F)
{
}

void CfxPersistent::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CfxPersistent::DefineResources(CfxFilerResource &F)
{
}

void CfxPersistent::OnAlarm()
{
}

void CfxPersistent::OnTimer()
{
}

void CfxPersistent::OnTrackTimer()
{
}

void CfxPersistent::OnPortData(BYTE PortId, BYTE *pData, int Length)
{
}

void CfxPersistent::OnRecordingComplete(char *pFileName, int Duration)
{
}

void CfxPersistent::OnPictureTaken(char *pFileName, bool Success)
{
}

void CfxPersistent::OnBarcodeScan(char *pBarcode, bool Success)
{
}

void CfxPersistent::OnPhoneNumberTaken(char *pPhoneNumber, bool Success)
{
}

void CfxPersistent::OnUrlReceived(const char *pFileName, int ErrorCode)
{
}

CfxPersistent *CfxPersistent::GetOwner()
{
    return qobject_cast<CfxPersistent*>(parent());
}

void CfxPersistent::SetOwner(CfxPersistent *pOwner)
{
    setParent(pOwner);
}

//*************************************************************************************************
// CfxFiler

void CfxFiler::DefineVariant(const QString& name, FXVARIANT **ppVariant)
{
    FXDATATYPE type = dtNull;
    void *data = nullptr;

    DefineBegin(name);
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

void CfxFiler::DefineGPS(FXGPS_POSITION *pPosition)
{
    DefineValue("Quality",   dtByte,   &pPosition->Quality);
    DefineValue("Latitude",  dtDouble, &pPosition->Latitude);
    DefineValue("Longitude", dtDouble, &pPosition->Longitude);
    DefineValue("Altitude",  dtDouble, &pPosition->Altitude);
    DefineValue("Accuracy",  dtDouble, &pPosition->Accuracy);
    DefineValue("Speed",     dtDouble, &pPosition->Speed);
}

void CfxFiler::DefineDateTime(FXDATETIME *pDateTime)
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

void CfxFiler::DefineGOTO(FXGOTO *pGoto)
{
    char *title = nullptr;
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
    title = nullptr;

    DefineValue("X", dtDouble, &pGoto->X);
    DefineValue("Y", dtDouble, &pGoto->Y);
}

void CfxFiler::DefineCOM(BYTE *pPortId, FX_COMPORT *pComPort)
{
    DefineValue("PortId", dtByte, pPortId, "1");
    DefineValue("Baud", dtInt32, &pComPort->Baud, "19200");
    DefineValue("Parity", dtByte, &pComPort->Parity, "0");
    DefineValue("Data", dtByte, &pComPort->Data, "8");
    DefineValue("Stop", dtByte, &pComPort->Stop, "0");
    DefineValue("FlowControl", dtBoolean, &pComPort->FlowControl, F_FALSE);
}

void CfxFiler::DefineRange(FXRANGE_DATA *pRange)
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


void CfxFiler::DefineXFONT(const QString& name, XFONT *pFont, const char *pDefault)
{
    DefineValue(name, dtInt32, pFont);
}

void CfxFiler::DefineXSOUND(const QString& name, XSOUND *pSound)
{
    DefineValue(name, dtInt32, pSound);
}

void CfxFiler::DefineXBITMAP(const QString& name, XBITMAP *pBitmap)
{
    DefineValue(name, dtInt32, pBitmap);
}

void CfxFiler::DefineXOBJECT(const QString& name, XGUID *pObject)
{
    DefineValue(name, dtInt32, pObject);
}

void CfxFiler::DefineXOBJECTLIST(const QString& name, XGUIDLIST *pObjectList)
{
    DefineValue(name, dtPBinary, pObjectList->GetMemoryPtr());
}

void CfxFiler::DefineXBINARY(const QString& name, XBINARY *pBinary)
{
    DefineValue(name, dtInt32, pBinary);
}

void CfxFiler::DefineXFILE(const QString& name, char **ppFileName)
{
    DefineFileName(name, ppFileName);
}

//*************************************************************************************************
// CfxReader

CfxReader::CfxReader(CfxStream *pStream)
{
    _stream = pStream;
}

bool CfxReader::ReadMarker(const int Value)
{
    int p;
    int i;

    p = _stream->GetPosition();
    _stream->Read(&i, sizeof(i));

    if (i == Value)
    {
        return true;
    }
    else
    {
        _stream->SetPosition(p);
        return false;
    }
}

void CfxReader::DefineValue(const QString& name, FXDATATYPE Type, void *pData, const char *pDefault)
{
    Type_FromStream(Type, pData, *_stream);
}

void CfxReader::DefineFileName(const QString& name, char **ppFileName)
{
    DefineValue(name, dtPTextAnsi, ppFileName);
}

bool CfxReader::DefineBegin(const QString& name)
{
    return ReadMarker(FILER_MARKER_DEFINEBEGIN);
}

void CfxReader::DefineEnd()
{
}

bool CfxReader::ListEnd()
{
    return ReadMarker(FILER_MARKER_LISTEND);
}

bool CfxReader::IsReader()
{
    return true;
}

bool CfxReader::IsWriter()
{
    return false;
}

bool CfxReader::IsEnd()
{
    return _stream->GetPosition() == _stream->GetSize();
}

//*************************************************************************************************
// CfxWriter

CfxWriter::CfxWriter(CfxStream *pStream)
{
    _stream = pStream;
}

void CfxWriter::WriteMarker(const int Value)
{
    int i = Value;
    _stream->Write(&i, sizeof(i));
}

void CfxWriter::DefineValue(const QString& name, FXDATATYPE Type, void *pData, const char *pDefault)
{
    Type_ToStream(Type, pData, *_stream);
}

void CfxWriter::DefineFileName(const QString& name, char **ppFileName)
{
    DefineValue(name, dtPTextAnsi, ppFileName);
}

bool CfxWriter::DefineBegin(const QString& name)
{
    WriteMarker(FILER_MARKER_DEFINEBEGIN);
    return true;
}

void CfxWriter::DefineEnd()
{
}

bool CfxWriter::ListEnd()
{
    WriteMarker(FILER_MARKER_LISTEND);
    return true;
}

bool CfxWriter::IsReader()
{
    return false;
}

bool CfxWriter::IsWriter()
{
    return true;
}


QStringList CfxReader::FindStrings(const void *pData, int size, int minLength)
{
    QStringList result;

    auto bytes = static_cast<const BYTE *>(pData);
    int pos = 0;

    while (pos < size)
    {
        BYTE tag = bytes[pos];

        // Marker: first byte is 0xFE (-2) or 0xFF (-1) on little-endian.
        if (tag >= 0xFE)
        {
            pos += sizeof(int);
            continue;
        }

        // Must be a valid type tag.
        if (tag >= dtMax)
        {
            break;
        }

        // Read the 4-byte size that follows the type tag.
        pos += 1;
        if (pos + (int)sizeof(int) > size)
        {
            break;
        }

        int dataSize = 0;
        memcpy(&dataSize, bytes + pos, sizeof(int));
        pos += sizeof(int);

        if (dataSize < 0 || pos + dataSize > size)
        {
            break;
        }

        // Null-terminated string data, so actual string length is dataSize - 1.
        auto dataType = static_cast<FXDATATYPE>(tag);
        if ((dataType == dtTextAnsi || dataType == dtTextUtf8) && dataSize - 1 >= minLength)
        {
            auto str = (dataType == dtTextAnsi)
                ? QString::fromLatin1((const char *)(bytes + pos), dataSize - 1)
                : QString::fromUtf8((const char *)(bytes + pos), dataSize - 1);

            if (!str.isEmpty())
            {
                result.append(str);
            }
        }

        pos += dataSize;
    }

    return result;
}


QStringList CfxReader::FindFileNames(const void *pData, int size)
{
    QStringList result;
    auto mediaPrefix = QLatin1String(MEDIA_PREFIX);

    for (auto& str : FindStrings(pData, size, 16))
    {
        if (str.startsWith(mediaPrefix))
        {
            str = str.mid(mediaPrefix.size());
        }

        auto dotIndex = str.lastIndexOf('.');
        if (dotIndex < 0)
        {
            continue;
        }

        auto extLength = str.length() - dotIndex - 1;
        if (extLength < 3 || extLength > 4)
        {
            continue;
        }

        result.append(str);
    }

    return result;
}

//*************************************************************************************************
// CfxEventManager

void ExecuteEvent(FXEVENT *pEvent, CfxPersistent *pSender, void *pParams)
{
    (pEvent->Object->*pEvent->Method)(pSender, pParams);
}

CfxEventManager::CfxEventManager(): _events()
{
}

CfxEventManager::~CfxEventManager()
{
    for (int i=0; i<_events.GetCount(); i++)
    {
        FX_ASSERT(!((FXEVENT *)_events.GetPtr(i))->Object); // Event not unregistered
    }
}

void CfxEventManager::Clear()
{
    _events.Clear();
}

void CfxEventManager::Register(FXEVENT *pEvent)
{
    for (int i=0; i<_events.GetCount(); i++)
    {
        if (!((FXEVENT *)_events.GetPtr(i))->Object)
        {
            _events.Put(i, *pEvent);
            return;
        }
    }

    _events.Add(*pEvent);
}

void CfxEventManager::Unregister(BYTE Type, CfxPersistent *Object)
{
    for (int i=0; i<_events.GetCount(); i++)
    {
        FXEVENT *event = _events.GetPtr(i);

        if ((event->Object == Object) && (event->Type == Type))
        {
            event->Object = nullptr;
            return;
        }
    }

    // Overactive?
    FX_ASSERT(false); // Invalid Event
}

void CfxEventManager::Execute(int EventId, CfxPersistent *pSender, void *pParams)
{
    FX_ASSERT((EventId >=0) && (EventId < _events.GetCount())); // Invalid Event Id
    
    ExecuteEvent(_events.GetPtr(EventId), pSender, pParams);
}

void CfxEventManager::ExecuteType(BYTE Type, CfxPersistent *pSender, void *pParams)
{
    for (int i=0; i<_events.GetCount(); i++)
    {
        FXEVENT *event = _events.GetPtr(i);
        if (event->Object && (event->Type == Type))
        {
            ExecuteEvent(event, pSender, pParams);
        }
    }
}
