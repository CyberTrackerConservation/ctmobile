#include "fxApplication.h"
#include "Classic/ClassicView.h"
#include "ctRegister.h"
#include "fxUtils.h"
#include "fxMath.h"
#include "fxProjection.h"
#include "fxTitleBar.h"

#include "ctElement.h"
#include "ctSession.h"
#include "ctActions.h"

#include "Cab.h"
#include "App.h"

#define CTX_QUOTA_SIZE (16 * 1024 * 1024) // 16MB

using namespace CyberTracker;

//*************************************************************************************************
// CctJsonBuilder

CctJsonBuilder::CctJsonBuilder(CctSession *pSession, CfxFileStream *pFileStream, int Protocol)
{
    _session = pSession;
    _fileStream = pFileStream;
    _protocol = Protocol;
}

bool CctJsonBuilder::WriteHeader()
{
    switch (_protocol)
    {
    case FXSENDDATA_PROTOCOL_HTTP:
    case FXSENDDATA_PROTOCOL_HTTPS:
    case FXSENDDATA_PROTOCOL_HTTPZ:
        return _fileStream->Write("{ \"type\": \"FeatureCollection\", \"features\": [\r\n");

    case FXSENDDATA_PROTOCOL_ESRI:
        return _fileStream->Write("[\r\n");
    }

    return false;
}

void CctJsonBuilder::ResolveElement(CctSession *pSession, CfxResource *pResource, XGUID ElementId, QString &Name)
{
    FXJSONID *jsonId = (FXJSONID *)pResource->GetObject(pSession, ElementId, (BYTE)ElementAttribute::JsonId);
    if (jsonId != nullptr && !QString(jsonId->Text).startsWith("__")) // Skip generated jsonIds.
    {
        Name = QString::fromUtf8(&jsonId->Text[0]);
        pResource->ReleaseObject(jsonId);
        return;
    }

    FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)pResource->GetObject(pSession, ElementId, (BYTE)ElementAttribute::Name);
    if (element != nullptr)
    {
        CTRESOURCEHEADER *header = (CTRESOURCEHEADER *)pResource->GetHeader(pSession);
        bool decode = (strncmp(header->ServerFileName, "KEY:", 4) == 0) ||      // SMART
                      (header->SendData.Protocol == FXSENDDATA_PROTOCOL_ESRI);  // ESRI
        if (decode)
        {
            auto nameStr = &element->Text[0];
            Name = QString::fromUtf8(element->Text[0] == '#' ? nameStr + 1 : nameStr);
        }
        else
        {
            char s[39] = {0};
            GUID *g = &element->Guid;
            sprintf(s,
                    "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                    g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
                    g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

            Name = QString::fromUtf8(s);
        }

        pResource->ReleaseObject(element);
        return;
    }

    Name = QStringLiteral("Unknown");
}

bool CctJsonBuilder::WriteSighting(CctSighting *pSighting, FXALERT *pAlert, bool First, FXFILELIST *pMediaFileList)
{
    switch (_protocol)
    {
    case FXSENDDATA_PROTOCOL_HTTP:
    case FXSENDDATA_PROTOCOL_HTTPS:
    case FXSENDDATA_PROTOCOL_HTTPZ:
        return WriteSightingGeoJson(pSighting, pAlert, First, pMediaFileList);
        
    case FXSENDDATA_PROTOCOL_ESRI:
        return WriteSightingEsri(pSighting, First, pMediaFileList);

    default:
        return false;
    }
}

bool CctJsonBuilder::WriteSightingGeoJson(CctSighting *pSighting, FXALERT *pAlert, bool First, FXFILELIST *pMediaFileList)
{
    FXGPS_POSITION *gps = pSighting ? pSighting->GetGPS() : GetEngine(_session)->GetGPSLastFix();

    bool result = true;
    if (!First)
    {
        result = _fileStream->Write(",\r\n");
    }

    auto geometryJson = QJsonObject {
        { "type", "Point" },
        { "coordinates", QJsonArray { gps->Longitude, gps->Latitude } },
    };

    char temp[256];
    auto propertiesJson = QJsonObject {
        { "ctVersion", QString("3.%1").arg((int)RESOURCE_VERSION) },
        { "dbName", _session->GetResourceHeader()->ServerFileName },
        { "appName", _session->GetResourceHeader()->Name },
    };

    // DateTime
    auto dateTime = pSighting ? pSighting->GetDateTime() : GetEngine(_session)->GetGPSLastTimeStamp();
    auto dateTimeString = GetHost(_session)->GetDateStringUTC(dateTime);
    propertiesJson.insert("dateTime", dateTimeString);
    MEM_FREE(dateTimeString);

    // DeviceId
    GUID deviceId;
    GetHost(_session)->GetDeviceId(&deviceId);
    sprintf(temp, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            deviceId.Data1, deviceId.Data2, deviceId.Data3, deviceId.Data4[0], deviceId.Data4[1], deviceId.Data4[2], deviceId.Data4[3],
            deviceId.Data4[4], deviceId.Data4[5], deviceId.Data4[6], deviceId.Data4[7]);
    propertiesJson.insert("deviceId", temp);

    // SightingId
    if (pSighting)
    {
        GUID uniqueId = *pSighting->GetUniqueId();
        sprintf(temp, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                uniqueId.Data1, uniqueId.Data2, uniqueId.Data3, uniqueId.Data4[0], uniqueId.Data4[1], uniqueId.Data4[2], uniqueId.Data4[3],
                uniqueId.Data4[4], uniqueId.Data4[5], uniqueId.Data4[6], uniqueId.Data4[7]);
        propertiesJson.insert("id", temp);
    }

    // GPS
    if (gps->Quality > FXGPS_QUALITY::None)
    {
        propertiesJson.insert("latitude", gps->Latitude);
        propertiesJson.insert("longitude", gps->Longitude);

        if (gps->Quality >= FXGPS_QUALITY::Fix3D)
        {
            propertiesJson.insert("altitude", gps->Altitude);
        }

        if ((gps->Accuracy > 0.0) && (gps->Accuracy < 50.0))
        {
            propertiesJson.insert("accuracy", gps->Accuracy);
        }
    }

    // Alert
    if (pAlert)
    {
        propertiesJson.insert("caUuid", pAlert->CaId);
        propertiesJson.insert("level", (int)pAlert->Level);
        propertiesJson.insert("description", pAlert->Description);
        propertiesJson.insert("typeUuid", pAlert->Type);

        if (pAlert->PatrolId[0])
        {
            propertiesJson.insert("patrolId", pAlert->PatrolId);
        }
    }

    // Write all other attributes
    if (pSighting)
    {
        auto sightingJson = QJsonObject();

        CfxResource *resource = _session->GetResource();

        for (int i=0; i<pSighting->GetAttributes()->GetCount(); i++) 
        {
            ATTRIBUTE *attribute = pSighting->GetAttributes()->GetPtr(i);
            
            QString nameStr, valueStr;
            ResolveElement(_session, resource, attribute->ElementId, nameStr);

            if (!IsNullXGuid(&attribute->ValueId))
            {
                ResolveElement(_session, resource, attribute->ValueId, valueStr);
                sightingJson.insert(nameStr, valueStr);
                continue;
            }

            if (pMediaFileList && attribute->IsMedia())
            {
                char *mediaName = attribute->GetMediaName();
                char *fileName = nullptr;
                FXFILEMAP fileMap = {0};
                auto cleanup = qScopeGuard([&] { FxUnmapFile(&fileMap); MEM_FREE(fileName); });

                fileName = _session->AllocMediaFileName(mediaName);
                if (fileName == nullptr)
                {
                    continue;
                }

                if (!FxMapFile(fileName, &fileMap))
                {
                    Log("Failed to open source file %s", fileName);
                    continue;
                }

                if (fileMap.FileSize == 0)
                {
                    Log("Empty file: %s", fileName);
                    continue;
                }

                auto view = QByteArray::fromRawData((char *)fileMap.BasePointer, fileMap.FileSize).toBase64();

                sightingJson.insert(nameStr, QString::fromLatin1(view));

                FXFILE file = {0};
                file.Name = AllocString(attribute->GetMediaName());
                pMediaFileList->Add(file);

                continue;
            }

            switch (attribute->Value->Type)
            {
                case dtNull:
                    sightingJson.insert(nameStr, QJsonValue());
                    break;

                case dtBoolean:
                    sightingJson.insert(nameStr, (bool)(attribute->Value->Data[0]));
                    break;

                case dtByte:
                case dtInt8:
                case dtInt16:
                case dtInt32:
                    sightingJson.insert(nameStr, (int)Type_VariantToDouble(attribute->Value));
                    break;

                case dtDouble:
                    sightingJson.insert(nameStr, Type_VariantToDouble(attribute->Value));
                    break;

                default:
                    Type_VariantToText(attribute->Value, valueStr);
                    sightingJson.insert(nameStr, valueStr);
                    break;
            }
        }

        propertiesJson.insert("sighting", sightingJson);
    }

    auto jsonObject = QJsonObject {
        { "type", "Feature" },
        { "geometry", geometryJson },
        { "properties", propertiesJson },
    };

    auto jsonData = QJsonDocument(jsonObject).toJson(QJsonDocument::JsonFormat::Indented);

    result = _fileStream->Write(jsonData.constData());

    return result;
}

void CctJsonBuilder::WriteSightingToStream(CctSession *pSession, CctSighting *pSighting, CfxStream &Data)
{
    FXGPS_POSITION *gps = pSighting ? pSighting->GetGPS() : GetEngine(pSession)->GetGPSLastFix();

    char *dateTimeString = nullptr;
    char *temp = (char *)MEM_ALLOC(4096);

    // Write header.

    // Type
    Data.Write("{ \"type\": \"Feature\"");

    // Geometry
    sprintf(temp, ",\r\n  \"geometry\": { \"type\": \"Point\", \"coordinates\": [%f, %f] }", gps->Longitude, gps->Latitude);
    Data.Write(temp);

    // Properties
    Data.Write(",\r\n  \"properties\": {");

    // DateTime
    FXDATETIME *dateTime;
    if (pSighting)
    {
        dateTime = pSighting->GetDateTime();
    }
    else
    {
        dateTime = GetEngine(pSession)->GetGPSLastTimeStamp();
    }
    dateTimeString = GetHost(pSession)->GetDateStringUTC(dateTime);
    sprintf(temp,
            "\r\n    \"dateTime\": \"%s\"",
            dateTimeString);
    Data.Write(temp);

    // DeviceId
    GUID deviceId;
    GetHost(pSession)->GetDeviceId(&deviceId);
    sprintf(temp, 
            ",\r\n    \"deviceId\": \"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\"",
            deviceId.Data1, deviceId.Data2, deviceId.Data3, deviceId.Data4[0], deviceId.Data4[1], deviceId.Data4[2], deviceId.Data4[3],
            deviceId.Data4[4], deviceId.Data4[5], deviceId.Data4[6], deviceId.Data4[7]);
    Data.Write(temp);

    // DatabaseName + application name.
    QString value;
    sprintf(temp, "3.%ld", (int)RESOURCE_VERSION);
    value = QString::fromUtf8(temp);
    FixJsonString(value);
    sprintf(temp, ",\r\n    \"ctVersion\": \"%s\"", value.toUtf8().constData());
    Data.Write(temp);

    value = QString::fromUtf8(pSession->GetResourceHeader()->ServerFileName);
    FixJsonString(value);
    sprintf(temp, ",\r\n    \"dbName\": \"%s\"", value.toUtf8().constData());
    Data.Write(temp);

    value = QString::fromUtf8(pSession->GetResourceHeader()->Name);
    FixJsonString(value);
    sprintf(temp, ",\r\n    \"appName\": \"%s\"", value.toUtf8().constData());
    Data.Write(temp);

    // SightingId
    GUID uniqueId = *pSighting->GetUniqueId();
    sprintf(temp, 
            ",\r\n    \"id\": \"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\"",
            uniqueId.Data1, uniqueId.Data2, uniqueId.Data3, uniqueId.Data4[0], uniqueId.Data4[1], uniqueId.Data4[2], uniqueId.Data4[3],
            uniqueId.Data4[4], uniqueId.Data4[5], uniqueId.Data4[6], uniqueId.Data4[7]);
    Data.Write(temp);

    // GPS
    if (gps->Quality > FXGPS_QUALITY::None)
    {
        sprintf(temp, ",\r\n    \"latitude\": %f,\r\n    \"longitude\": %f", gps->Latitude, gps->Longitude);
        Data.Write(temp);

        if (gps->Quality >= FXGPS_QUALITY::Fix3D)
        {
            sprintf(temp, ",\r\n    \"altitude\": %f", gps->Altitude);
            Data.Write(temp);
        }

        if ((gps->Accuracy > 0.0) && (gps->Accuracy < 50.0))
        {
            sprintf(temp, ",\r\n    \"accuracy\": %f", gps->Accuracy);
            Data.Write(temp);
        }
    }

    // Write all other attributes
    Data.Write(",\r\n    \"sighting\": {");

    CfxResource *resource = pSession->GetResource();

    for (int i=0; i<pSighting->GetAttributes()->GetCount(); i++) 
    {
        QString name, value;

        ATTRIBUTE *attribute = pSighting->GetAttributes()->GetPtr(i);

        ResolveElement(pSession, resource, attribute->ElementId, name);

        if (!IsNullXGuid(&attribute->ValueId))
        {
            ResolveElement(pSession, resource, attribute->ValueId, value);
        }
        else
        {
            Type_VariantToText(attribute->Value, value);
        }

        bool quotify = false;

        if (attribute->Value->Type == dtNull)
        {
            value = QStringLiteral("null");
        }
        else
        {
            if (value.isNull())
            {
                value.clear();
            }

            quotify = (attribute->Value->Type >= dtDate);
        }

        FixJsonString(name);
        if (i > 0)
        {
            Data.Write(",");
        }
        sprintf(temp, "\r\n      \"%s\": ", name.toUtf8().constData());
        Data.Write(temp);
        if (quotify)
        {
            Data.Write("\"");
        }
        FixJsonString(value);

        if (attribute->IsMedia())
        {
            Data.Write("[MEDIA]");
        }
        else
        {
            Data.Write(value.toUtf8().constData());
        }

        if (quotify)
        {
            Data.Write("\"");
        }
    }

    Data.Write("\r\n    }");

    // Success
    Data.Write("\r\n  }\r\n}");

    // Null terminate.
    char n = '\0';
    Data.Write1(&n);

    MEM_FREE(temp);
    MEM_FREE(dateTimeString);
}

bool CctJsonBuilder::WriteWaypoint(WAYPOINT *pWaypoint, bool First)
{
    char temp[256];
    bool result = true;
    if (!First)
    {
        result = _fileStream->Write(",\r\n");
    }

    switch (_protocol)
    {
    case FXSENDDATA_PROTOCOL_HTTP:
    case FXSENDDATA_PROTOCOL_HTTPS:
    case FXSENDDATA_PROTOCOL_HTTPZ:
        // Type
        result = result && _fileStream->Write("{ \"type\": \"Feature\",\r\n");

        // Geometry
        sprintf(temp, "  \"geometry\": { \"type\": \"Point\", \"coordinates\": [%f, %f] },\r\n", pWaypoint->Longitude, pWaypoint->Latitude);
        result = result && _fileStream->Write(temp);

        // Properties
        result = result && _fileStream->Write("  \"properties\": {\r\n");
        break;

    case FXSENDDATA_PROTOCOL_ESRI:
        // Type
        result = result && _fileStream->Write("{\r\n");

        // Geometry
        sprintf(temp, "  \"geometry\": { \"x\": %f, \"y\": %f, \"z\": %f },\r\n", pWaypoint->Longitude, pWaypoint->Latitude, pWaypoint->Altitude);
        result = result && _fileStream->Write(temp);

        // Properties
        result = result && _fileStream->Write("  \"attributes\": {\r\n");
        break;

    default:
        result = false;
        break;
    }

    if (!result) return false;

    // DeviceId
    sprintf(temp, 
            "    \"deviceId\": \"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\",\r\n",
            pWaypoint->DeviceId.Data1, pWaypoint->DeviceId.Data2, pWaypoint->DeviceId.Data3, pWaypoint->DeviceId.Data4[0], pWaypoint->DeviceId.Data4[1], pWaypoint->DeviceId.Data4[2], pWaypoint->DeviceId.Data4[3],
            pWaypoint->DeviceId.Data4[4], pWaypoint->DeviceId.Data4[5], pWaypoint->DeviceId.Data4[6], pWaypoint->DeviceId.Data4[7]);
    result = result && _fileStream->Write(temp);

    // WaypointId
    sprintf(temp, 
            "    \"id\": \"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\",\r\n",
            pWaypoint->Id.Data1, pWaypoint->Id.Data2, pWaypoint->Id.Data3, pWaypoint->Id.Data4[0], pWaypoint->Id.Data4[1], pWaypoint->Id.Data4[2], pWaypoint->Id.Data4[3],
            pWaypoint->Id.Data4[4], pWaypoint->Id.Data4[5], pWaypoint->Id.Data4[6], pWaypoint->Id.Data4[7]);
    result = result && _fileStream->Write(temp);

    // DateTime
    FXDATETIME dateCurrent = {0};
    DecodeDateTime(pWaypoint->DateCurrent, &dateCurrent);
    
    char *dateTimeString = GetHost(_session)->GetDateStringUTC(&dateCurrent, (_protocol == FXSENDDATA_PROTOCOL_ESRI));
    sprintf(temp,
            "    \"dateTime\": \"%s\"",
            dateTimeString);
    result = result && _fileStream->Write(temp);
    MEM_FREE(dateTimeString);
    

    // GPS
    if (_protocol != FXSENDDATA_PROTOCOL_ESRI)
    {
        result = result && _fileStream->Write(",\r\n");
        sprintf(temp, "    \"altitude\": %f", pWaypoint->Altitude);
        result = result && _fileStream->Write(temp);
    }

    result = result && _fileStream->Write(",\r\n");
    sprintf(temp, "    \"accuracy\": %f", pWaypoint->Accuracy);
    result = result && _fileStream->Write(temp);

    result = result && _fileStream->Write("\r\n  }\r\n}");

    return result;
}

bool CctJsonBuilder::WriteSightingEsri(CctSighting *pSighting, bool First, FXFILELIST *pMediaFileList)
{
    FXGPS_POSITION *gps = pSighting ? pSighting->GetGPS() : GetEngine(_session)->GetGPSLastFix();
    char *dateTimeString = nullptr;

    char *temp = (char *)MEM_ALLOC(4096);
    bool result = true;
    if (!First)
    {
        result = _fileStream->Write(",\r\n");
    }

    // Write header.

    // Type
    result = result && _fileStream->Write("{\r\n");

    // Geometry
    sprintf(temp, "  \"geometry\": { \"x\": %f, \"y\": %f, \"z\": %f },\r\n", gps->Longitude, gps->Latitude, gps->Altitude);
    result = result && _fileStream->Write(temp);

    // Properties
    result = result && _fileStream->Write("  \"attributes\": {");

    // DateTime
    FXDATETIME *dateTime;
    if (pSighting)
    {
        dateTime = pSighting->GetDateTime();
    }
    else
    {
        dateTime = GetEngine(_session)->GetGPSLastTimeStamp();
    }
    dateTimeString = GetHost(_session)->GetDateStringUTC(dateTime, true);
    
    sprintf(temp,
            "\r\n    \"dateTime\": \"%s\"",
            dateTimeString);
    result = result && _fileStream->Write(temp);

    // DeviceId
    GUID deviceId;
    GetHost(_session)->GetDeviceId(&deviceId);
    sprintf(temp, 
            ",\r\n    \"deviceId\": \"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\"",
            deviceId.Data1, deviceId.Data2, deviceId.Data3, deviceId.Data4[0], deviceId.Data4[1], deviceId.Data4[2], deviceId.Data4[3],
            deviceId.Data4[4], deviceId.Data4[5], deviceId.Data4[6], deviceId.Data4[7]);
    result = result && _fileStream->Write(temp);

    // Write all other attributes

    CfxResource *resource = _session->GetResource();
    QString name, value;

    for (int i=0; i<pSighting->GetAttributes()->GetCount(); i++)
    {
        ATTRIBUTE *attribute = pSighting->GetAttributes()->GetPtr(i);

        ResolveElement(_session, resource, attribute->ElementId, name);

        // Hack to fix name, because the desktop is not emitting the json type properly
        // for some reason.
        for (auto i = 0; i < name.length(); i++)
        {
            auto c = name.at(i);
            if (!c.isLetter() && !c.isNumber())
            {
                name[i] = '_';
            }
        }

        if (!IsNullXGuid(&attribute->ValueId))
        {
            ResolveElement(_session, resource, attribute->ValueId, value);
        }
        else
        {
            Type_VariantToText(attribute->Value, value);
        }

        bool quotify = false;

        if (!Type_IsText((FXDATATYPE)(attribute->Value->Type)))
        {
            if (value.compare(QLatin1String(""), Qt::CaseInsensitive) == 0)
            {
                value = QStringLiteral("1");
            }
        }

        quotify = (attribute->Value->Type >= dtDate);

        FixJsonString(name);
        sprintf(temp, ",\r\n    \"%s\": ", name.toUtf8().constData());
        result = result && _fileStream->Write(temp);
        if (quotify)
        {
            result = result && _fileStream->Write("\"");
        }
        FixJsonString(value);

        if (attribute->IsMedia())
        {
            FXFILE file = {0};
            file.Name = AllocString(attribute->GetMediaName());
            pMediaFileList->Add(file);

            value = QStringLiteral("1");
        }

        result = result && _fileStream->Write(value.toUtf8().constData());

        if (quotify)
        {
            result = result && _fileStream->Write("\"");
        }
    }

    // Success
    result = result && _fileStream->Write("\r\n  }\r\n}");

    MEM_FREE(temp);
    MEM_FREE(dateTimeString);
    return result;
}

bool CctJsonBuilder::WriteFooter()
{
    switch (_protocol)
    {
    case FXSENDDATA_PROTOCOL_HTTP:
    case FXSENDDATA_PROTOCOL_HTTPS:
    case FXSENDDATA_PROTOCOL_HTTPZ:
        return _fileStream->Write("\r\n]}");

    case FXSENDDATA_PROTOCOL_ESRI:
        return _fileStream->Write("\r\n]");
    }

    return false;
}

//*************************************************************************************************
// CctAlert

CctAlert::CctAlert(CfxPersistent *pOwner): CfxPersistent(pOwner), _matchElements()
{
    _active = false;
    _url = _userName = _password = _caId = _description = _type = nullptr;
    _level = 0;
    _pingFrequency = 0;
    _pingOnly = false;
    _protocol = FXSENDDATA_PROTOCOL_HTTPS;
    InitXGuid(&_patrolElementId);
    _committed = false;
    _patrolValue = nullptr;
    memset(&_alert, 0, sizeof(_alert));
}

CctAlert::~CctAlert()
{
    MEM_FREE(_url);
    MEM_FREE(_userName);
    MEM_FREE(_password);
    MEM_FREE(_caId);
    MEM_FREE(_description);
    MEM_FREE(_type);
    MEM_FREE(_patrolValue);
    _url = _userName = _password = _caId = _description = _type = _patrolValue = nullptr;

    if (GetEngine(this))
    {
        GetEngine(this)->KillAlarm(this);
    }
}

bool CompareStrings(char *p1, char *p2, bool CaseSensitive = false)
{
    if (p1 == p2)
    {
        return true;
    }

    if (!p1 && !p2)
    {
        return true;
    }

    if ((p1 && !p2) || (!p1 && p2))
    {
        return false;
    }

    if (CaseSensitive)
    {
        return strcmp(p1, p2) == 0;
    }
    else
    {
        return stricmp(p1, p2) == 0;
    }
}

bool CctAlert::Compare(CctAlert *pAlert)
{
    bool result;

    if (_active != pAlert->_active) return false;

    result = CompareStrings(_url, pAlert->_url);
    if (!result) return false;

    result = CompareStrings(_userName, pAlert->_userName);
    if (!result) return false;

    result = CompareStrings(_password, pAlert->_password);
    if (!result) return false;

    result = CompareStrings(_caId, pAlert->_caId);
    if (!result) return false;

    result = CompareStrings(_description, pAlert->_description);
    if (!result) return false;

    result = CompareStrings(_type, pAlert->_type);
    if (!result) return false;

    if (_level != pAlert->_level) return false;
    if (_pingFrequency != pAlert->_pingFrequency) return false;

    result = CompareXGuid(&_patrolElementId, &pAlert->_patrolElementId);
    if (!result) return false;

    result = _matchElements.GetCount() == pAlert->_matchElements.GetCount();
    if (!result) return false;
   
    result = memcmp(_matchElements.GetMemory(), pAlert->_matchElements.GetMemory(), sizeof(XGUID) * _matchElements.GetCount()) == 0;
    if (!result) return false;

    return true;
}

bool CctAlert::Commit(CctSession *pSession, CctSighting *pSighting)
{
    if (!_active || !Match(pSighting)) return false;

    // Get the patrol id value from the sighting if available.
    MEM_FREE(_patrolValue);
    _patrolValue = nullptr;
    ATTRIBUTE *attribute = nullptr;
    for (int i = 0; pSighting->EnumAttributes(i, &attribute); i++)
    {
        QString valueText;
        if (!CompareXGuid(&attribute->ElementId, &_patrolElementId)) continue;

        Type_VariantToText(attribute->Value, valueText);
        if (!valueText.isEmpty())
        {
            _patrolValue = AllocString(valueText.toUtf8().constData());
            break;
        }
    }

    if (!Build(pSighting, &_alert))
    {
        Log("Failed to build alert");
        return false;
    }

    // Activate this ping/alert.
    _committed = _pingOnly ? pSession->GetTransferManager()->CreatePing(&_alert) :
                             pSession->GetTransferManager()->CreateAlert(pSighting, &_alert);

    return _committed;
}

bool CctAlert::Build(CctSighting *pSighting, FXALERT *pAlert)
{
    memset(pAlert, 0, sizeof(FXALERT));
    if (!_active) return false;
    if (!_url) return false;

    pAlert->Magic = MAGIC_ALERT;
    
    strncpy(pAlert->SendData.Url, _url, ARRAYSIZE(pAlert->SendData.Url));
    pAlert->SendData.Magic = MAGIC_SENDDATA;
    pAlert->SendData.Protocol = _protocol;

    if (_userName) strncpy(pAlert->SendData.UserName, _userName, ARRAYSIZE(pAlert->SendData.UserName));
    if (_password) strncpy(pAlert->SendData.Password, _password, ARRAYSIZE(pAlert->SendData.Password));

    if (_caId) strncpy(pAlert->CaId, _caId, ARRAYSIZE(pAlert->CaId));
    if (_description) strncpy(pAlert->Description, _description, ARRAYSIZE(pAlert->Description));
    if (_type) strncpy(pAlert->Type, _type, ARRAYSIZE(pAlert->Type));

    pAlert->Level = _level;
    pAlert->PingFrequency = _pingFrequency;

    // Add the sighting id to the end of the URL.
    char *url = pAlert->SendData.Url;
    int len = strlen(url);
    if ((len > 0) && (url[len-1] != '/')) 
    {
        strcat(url, "/");
    }

    if (!_pingOnly)
    {
        char *idString = AllocGuidName(pSighting->GetUniqueId(), nullptr);
        strcat(url, idString);
        MEM_FREE(idString);
    }
    else
    {
        if (_patrolValue && _patrolValue[0] && (strlen(_patrolValue) + strlen(url) < ARRAYSIZE(pAlert->SendData.Url)))
        {
            strcat(url, _patrolValue);
        }
        else
        {
            strcat(url, "ping");
        }
    }

    // Add the patrol value if it exists.
    if (_patrolValue && _patrolValue[0])
    {
        strncpy(pAlert->PatrolId, _patrolValue, ARRAYSIZE(pAlert->PatrolId));
    }

    return true;
}

bool CctAlert::Match(CctSighting *pSighting)
{
    if (_matchElements.GetCount() == 0) return true;

    ATTRIBUTE *attribute = nullptr;
    
    for (int i = 0; pSighting->EnumAttributes(i, &attribute); i++)
    {
        for (int j = 0; j < _matchElements.GetCount(); j++)
        {
            XGUID id = _matchElements.Get(j);

            if (IsNullXGuid(&id)) continue;

            if (CompareXGuid(&id, &attribute->ElementId) ||
                CompareXGuid(&id, &attribute->ValueId))
            {
                return true;
            }
        }
    }

    return false;
}

void CctAlert::DefineResources(CfxFilerResource &F)
{
}

void CctAlert::DefineState(CfxFiler &F)
{
    F.DefineValue("Committed", dtBoolean, &_committed);
    F.DefineValue("PatrolElementValue", dtPText, &_patrolValue);
}

void CctAlert::DefineProperties(CfxFiler &F)
{
    F.DefineValue("Active", dtBoolean, &_active, F_FALSE);
    F.DefineValue("Url", dtPText, &_url, F_NULL);
    F.DefineValue("UserName", dtPText, &_userName, F_NULL);
    F.DefineValue("Password", dtPText, &_password, F_NULL);
    F.DefineValue("CaId", dtPText, &_caId, F_NULL);
    F.DefineValue("Description", dtPText, &_description, F_NULL);
    F.DefineValue("AlertType", dtPText, &_type, F_NULL);
    F.DefineValue("Level", dtInt32, &_level, F_ZERO);
    F.DefineValue("PingFrequency", dtInt32, &_pingFrequency, F_ZERO);
    F.DefineValue("PingOnly", dtBoolean, &_pingOnly, F_FALSE);
    F.DefineValue("Protocol", dtInt32, &_protocol, F_ZERO);
    F.DefineXOBJECTLIST("Elements", &_matchElements);
    F.DefineXOBJECT("PatrolElementId", &_patrolElementId);

    if (F.IsReader())
    {
        if ((_protocol != FXSENDDATA_PROTOCOL_HTTPS) &&
            (_protocol != FXSENDDATA_PROTOCOL_HTTPZ) &&
            (_protocol != FXSENDDATA_PROTOCOL_ESRI))
        {
            _protocol = FXSENDDATA_PROTOCOL_HTTPS;
        }
    }
}

void CctAlert::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctAlert::ActivateAlarm()
{
    if (!_committed) return;

    CfxEngine *engine = GetEngine(this);
    if (engine == nullptr) return;
    engine->KillAlarm(this);

    /*if (GetHost(this)->GetBatteryState() == FXBATTERYSTATE::Critical)
    {
        Log("Failed to activate alarm: battery critical");
        return;
    }*/

    if (_pingFrequency != 0)
    {
        engine->SetAlarm(this, _pingFrequency * 60, "Ping1");
    }
    else
    {
        OnAlarm();
    }
}

void CctAlert::OnAlarm()
{
    CctTransferManager *transferManager = (CctTransferManager *)GetOwner();
    CctSession *session = (CctSession *)(transferManager->GetOwner());
    if (!session->GetConnected()) return;

    if (!_committed) return;

    CfxEngine *engine = GetEngine(this);
    engine->KillAlarm(this);

    /*if (GetHost(this)->GetBatteryState() == FXBATTERYSTATE::Critical)
    {
        Log("Failed to activate alert: battery critical");
        return;
    }*/

    if (_pingFrequency == 0)
    {
        if (transferManager->HasPendingAlert())
        {
            engine->SetAlarm(this, 60, "UnsentAlerts");                // Retry in 120 seconds
        }
    }
    else
    {
        engine->SetAlarm(this, _pingFrequency * 60, "Ping2");    // Retry
        transferManager->CreatePing(&_alert);
    }
}

//*************************************************************************************************
// CctRetainState

CctRetainState::CctRetainState(CfxPersistent *pOwner, CfxControl *pParent): CfxPersistent(pOwner)
{
    _parent = pParent;
    _retain = false;
    _globalValue = nullptr;
    _lastValue = 0;
}

CctRetainState::~CctRetainState()
{
    MEM_FREE(_globalValue);
    _globalValue = nullptr;
    _parent = nullptr;
}

bool CctRetainState::MustResetState()
{
    if (_retain && _globalValue)
    {
        GLOBALVALUE *globalValue = GetSession(_parent)->GetGlobalValue(_globalValue);
        return (globalValue == nullptr) || (globalValue->Value != _lastValue);
    }
    else
    {
        return false;
    }
}

void CctRetainState::DefineState(CfxFiler &F)
{
    if (!_globalValue || !_retain)
    {
        return;
    }

    if (F.IsWriter())
    {
        CctSession *session = GetSession(_parent);
        GLOBALVALUE *globalValue = session->GetGlobalValue(_globalValue);
        _lastValue = globalValue != nullptr ? globalValue->Value : 1;
        if ((globalValue == nullptr) || (_lastValue != globalValue->Value))
        {
            session->SetGlobalValue(this, _globalValue, _lastValue);
        }
    }

    F.DefineValue("RetainGlobalValue", dtDouble, &_lastValue);
}

void CctRetainState::DefineProperties(CfxFiler &F)
{
    F.DefineValue("RetainState", dtBoolean, &_retain, F_FALSE);
    F.DefineValue("RetainStateGlobal", dtPText, &_globalValue);
}

void CctRetainState::DefinePropertiesUI(CfxFilerUI &F)
{
}


//*************************************************************************************************
// CctTransferManager

void DrawWaypointIcon(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Value, COLOR BackColor, COLOR ForeColor, bool Fill, bool StrikeOut, bool Invert)
{
    pCanvas->State.BrushColor = pCanvas->InvertColor(BackColor, Invert);
    pCanvas->FillRect(pRect);
    pCanvas->State.LineColor = pCanvas->InvertColor(ForeColor, Invert);

    FXRECT rect = *pRect;
    InflateRect(&rect, -(int)(pCanvas->State.LineWidth-1), -(int)(pCanvas->State.LineWidth-1));

    int width = rect.Right - rect.Left + 1;
    int height = rect.Bottom - rect.Top + 1;
    int h = min(height / 2 - 2, width / 2 - 2);

    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{h, 0}, {h*2, h*2}, {0, h*2}};

    int x = rect.Left + (width - h*2) / 2;
    int y = rect.Top + height / 2 - h;

    if (Fill)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(ForeColor, Invert);
        FILLPOLYGON(pCanvas, triangle, x, y);
    }

    if (Value)
    {
        char buffer[10];
        if (Value >= 60)
        {
            itoa(Value / 60, buffer, 10);
            strcat(buffer, "m");
        }
        else
        {
            itoa(Value, buffer, 10);
            strcat(buffer, "s");
        }

        int offsetY;
        pCanvas->TextHeight(pFont, buffer, 0, &offsetY);

        if ((int)pCanvas->FontHeight(pFont) - offsetY < h * 2)
        {
            FXRECT rect = {x + 1, y, x + h*2, y + h*2 + offsetY - 2};

            pCanvas->State.TextColor = Fill ? pCanvas->InvertColor(BackColor, Invert) : pCanvas->InvertColor(ForeColor, Invert);
            pCanvas->DrawTextRect(pFont, &rect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_BOTTOM, buffer);
        }
    }

    if (!Fill)
    {
        pCanvas->State.LineColor = pCanvas->InvertColor(ForeColor, Invert);
        POLYGON(pCanvas, triangle, x, y);
    }

    if (StrikeOut)
    {
        pCanvas->State.LineColor = pCanvas->InvertColor(ForeColor, Invert);
        pCanvas->MoveTo(rect.Left, rect.Top + height / 2);
        pCanvas->LineTo(rect.Right, rect.Top + height / 2);
    }
}

//*************************************************************************************************
// CctTransferManager

CctTransferManager::CctTransferManager(CfxPersistent *pOwner): CfxPersistent(pOwner), _alerts()
{
    Reset();
}

CctTransferManager::~CctTransferManager()
{
    Reset();
}

void CctTransferManager::Reset()
{
    ClearAlerts();
}

void CctTransferManager::ClearAlerts()
{
    for (int i = 0; i < _alerts.GetCount(); i++)
    {
        delete _alerts.Get(i);
    }
    _alerts.Clear();
}

void CctTransferManager::AddAlert(CctAlert *pAlert)
{
    for (int i = 0; i < _alerts.GetCount(); i++)
    {
        CctAlert *alert = _alerts.Get(i);

        // Skip already committed alerts.
        if (alert->GetCommitted()) continue;

        // De-dupe alerts.
        if (alert->Compare(pAlert))
        {
            return;
        }
    }

    CctAlert *alert = new CctAlert(this);
    alert->Assign(pAlert);
    _alerts.Add(alert);
}

void CctTransferManager::RemoveAlert(CctAlert *pAlert)
{
    for (int i = 0; i < _alerts.GetCount(); i++)
    {
        CctAlert *alert = _alerts.Get(i);

        // Skip already committed alerts.
        if (alert->GetCommitted()) continue;

        if (alert->Compare(pAlert))
        {
            delete alert;
            _alerts.Delete(i);
            break;
        }
    }
}

void CctTransferManager::CommitAlerts(CctSighting *pSighting)
{
    // We intend to refresh the already committed alerts, so blow away the old ones.
    for (int i = (int)_alerts.GetCount() - 1; i >= 0; i--)
    {
        CctAlert *alert = _alerts.Get(i);
        if (alert->GetCommitted())
        {
            delete alert;
            _alerts.Delete(i);
        }
    }

    // Activate alerts.
    for (int j = 0; j < _alerts.GetCount(); j++)
    {
        CctAlert *alert = _alerts.Get(j);
        if (alert->Commit(GetSession(), pSighting))
        {
            alert->ActivateAlarm();
        }
    }
}

void CctTransferManager::DefineState(CfxFiler &F)
{
    // NOT USED: uniqueness number for sightings and waypoints
    int dataUniqueness = 0;
    F.DefineValue("DataUniqueness", dtInt32, &dataUniqueness);

    // Alerts
    if (F.IsReader())
    {
        ClearAlerts();
        while (!F.ListEnd())
        {
            CctAlert *alert = new CctAlert(this);
            alert->DefineProperties(F);
            alert->DefineState(F);
            _alerts.Add(alert);
            alert->ActivateAlarm();
        }
    }
    else
    {
        for (int i = 0; i < _alerts.GetCount(); i++)
        {
            CctAlert *alert = _alerts.Get(i);
            alert->DefineProperties(F);
            alert->DefineState(F);
        }
        F.ListEnd();
    }
}

bool CctTransferManager::HasOutgoingData()
{
    bool result = false;
    FXFILELIST fileInfoList;

    if (FxBuildFileList(GetSession()->GetOutgoingPath(), "*.OUT", &fileInfoList))
    {
        result = fileInfoList.GetCount() > 0;
    }

    FileListClear(&fileInfoList);

    return result;
}

bool CctTransferManager::HasTransferData()
{
    return (GetSession()->GetResourceHeader()->SendData.Protocol != FXSENDDATA_PROTOCOL_UNKNOWN) &&
           HasUnsentData();
}

bool CctTransferManager::HasExportData()
{
    return (GetSession()->GetResourceHeader()->SendData.Protocol == FXSENDDATA_PROTOCOL_UNKNOWN) &&
           HasUnsentData();
}

bool CctTransferManager::HasPendingAlert()
{
    bool result = false;
    FXFILELIST fileInfoList;

    if (FxBuildFileList(GetSession()->GetOutgoingPath(), "!*.OUT", &fileInfoList))
    {
        result = fileInfoList.GetCount() > 0;
    }

    FileListClear(&fileInfoList);

    return result;
}

void LocationToWaypoint(const QString& locationUid, Location* location, const GUID& deviceId, WAYPOINT* pWaypoint)
{
    *pWaypoint = {};
    pWaypoint->Magic = MAGIC_WAYPOINT;
    pWaypoint->DeviceId = deviceId;

    auto bytes = QByteArray::fromHex(locationUid.toLatin1());
    qFatalIf(bytes.size() != sizeof(GUID), "Bad location uid");
    memcpy(&pWaypoint->Id, bytes.constData(), sizeof(GUID));

    pWaypoint->Longitude = location->x();
    pWaypoint->Latitude = location->y();
    pWaypoint->Altitude = std::isnan(location->z()) ? 0 : location->z();

    auto accuracy = location->accuracy();
    pWaypoint->Accuracy = min(50, (std::isnan(accuracy) ? 100 : accuracy) / 10);

    auto dt = Utils::decodeTimestamp(location->timestamp());
    FXDATETIME dateTime = {};
    dateTime.Year = dt.date().year();
    dateTime.Month = dt.date().month();
    dateTime.Day = dt.date().day();
    dateTime.Hour = dt.time().hour();
    dateTime.Minute = dt.time().minute();
    dateTime.Second = dt.time().second();
    pWaypoint->DateCurrent = EncodeDateTime(&dateTime);
}

void CctTransferManager::MakeWAY(char *pFilePath, FXEXPORTFILEINFO* pExportFileInfo)
{
    auto *classicView = GetClassicView(this);

    CfxFileStream fileStream;
    qFatalIf(!fileStream.OpenForWrite(pFilePath), "Failed to open for write: " + QString(pFilePath));

    GUID deviceId = {};
    GetHost(this)->GetDeviceId(&deviceId);

    auto writeWaypoint = [&](const QString& locationUid, Location* location)
    {
        WAYPOINT waypoint;
        LocationToWaypoint(locationUid, location, deviceId, &waypoint);

        fileStream.Write(&waypoint, sizeof(WAYPOINT));

        auto startDt = EncodeDateTime(&pExportFileInfo->startTime);
        auto stopDt = EncodeDateTime(&pExportFileInfo->stopTime);

        if (pExportFileInfo->waypointCount == 0 || waypoint.DateCurrent < startDt)
        {
            DecodeDateTime(waypoint.DateCurrent, &pExportFileInfo->startTime);
        }
        if (pExportFileInfo->waypointCount == 0 || waypoint.DateCurrent > stopDt)
        {
            DecodeDateTime(waypoint.DateCurrent, &pExportFileInfo->stopTime);
        }

        pExportFileInfo->waypointCount++;
    };

    // Export all locations from the active track file.
    classicView->form()->trackFile()->enumLocations(writeWaypoint);

    fileStream.Close();

    if (pExportFileInfo->waypointCount == 0)
    {
        FxDeleteFile(pFilePath);
    }
}

bool CctTransferManager::CreateFileInQueue(const char *pTargetFileName, const char *pTargetFileExtension, BYTE *pData, int DataSize)
{
    bool result = false;
    HANDLE hFile = 0;

    char *targetPath = AllocMaxPath(GetSession()->GetOutgoingPath());
    if (targetPath == nullptr)
    {
        Log("Failed to allocate working space");
        goto cleanup;
    }

    AppendSlash(targetPath);
    strcat(targetPath, pTargetFileName);
    strcat(targetPath, pTargetFileExtension);

    hFile = FileOpen(targetPath, "wb");
    if (hFile == 0)
    {
        Log("Failed to create .out file");
        goto cleanup;
    }

    if (!FileWrite(hFile, pData, DataSize))
    {
        Log("Failed to write data: %08lx", FileGetLastError(hFile));
        goto cleanup;
    }

    result = true;

cleanup:

    if (hFile != 0)
    {
        FileClose(hFile);
    }

    if (result)
    {    
        GetHost(this)->RequestMediaScan(targetPath);
    }

    MEM_FREE(targetPath);

    return result;
}

bool CctTransferManager::CreateFileInQueue(const char *pTargetFileName, const char *pTargetFileExtension, const char *pSourceFileName, bool Compress, bool CopyNotMove)
{
    bool result = false;

    char *targetPath = AllocMaxPath(GetSession()->GetOutgoingPath());
    if (targetPath == nullptr)
    {
        Log("Failed to allocate working space");
        goto cleanup;
    }

    AppendSlash(targetPath);
    strcat(targetPath, pTargetFileName);
    strcat(targetPath, pTargetFileExtension);

    if (Compress)
    {
        if (!CompressFile(pSourceFileName, targetPath))
        {
            Log("Failed compress file from %s to %s", pSourceFileName, targetPath);
            if (!FxMoveFile(pSourceFileName, targetPath))
            {
                Log("Failed move file from %s to %s", pSourceFileName, targetPath);
            }
            goto cleanup;
        }
        else
        {
            FxDeleteFile(pSourceFileName);
        }
    }
    else
    {
        if (CopyNotMove)
        {
            if (!FxCopyFile(pSourceFileName, targetPath))
            {
                Log("Failed copy file from %s to %s", pSourceFileName, targetPath);
                goto cleanup;
            }
        }
        else
        {
            if (!FxMoveFile(pSourceFileName, targetPath))
            {
                Log("Failed move file from %s to %s", pSourceFileName, targetPath);
                goto cleanup;
            }
        }
    }

    GetHost(this)->RequestMediaScan(pSourceFileName);
    GetHost(this)->RequestMediaScan(targetPath);
    result = true;

cleanup:

    MEM_FREE(targetPath);

    return result;
}

bool CctTransferManager::PushToOutgoing(const char *pSourceFileName, const char *pTargetFileExtension, FXSENDDATA *pSendData)
{
    bool success = false;

    const char *targetFileName = strrchr(pSourceFileName, PATH_SLASH_CHAR);
    if (targetFileName == nullptr)
    {
        Log("Bad file name");
        goto cleanup;
    }

    targetFileName++;

    success = CreateFileInQueue(targetFileName, pTargetFileExtension, pSourceFileName, pSendData->Protocol == FXSENDDATA_PROTOCOL_HTTPZ, false);
    if (!success)
    {
        Log("Failed to create data file in queue.");
        goto cleanup;
    }
    
    success = CreateFileInQueue(targetFileName, ".OUT", (BYTE *)pSendData, sizeof(FXSENDDATA));
    if (!success)
    {
        Log("Failed to create OUT file in queue.");
        goto cleanup;
    }

cleanup:

    return success;
}

bool CctTransferManager::PushMediaToOutgoing(const char *pSourceMediaName, const char *pTargetFileName, int TargetId, FXSENDDATA *pSendData)
{
    bool success = false;
    char *targetFileName = AllocMaxPath("");
    char *sourceFileName = GetSession()->AllocMediaFileName(pSourceMediaName);
    const char *sourceExtension = nullptr;
    if (!FxDoesFileExist(sourceFileName))
    {
        //Testing: FxCopyFile("c:\\Users\\Builder\\Pictures\\Elephant.png", sourceFileName);
        Log("Media file does not exist");
        goto cleanup;
    }

    sourceExtension = GetFileExtension(pSourceMediaName);
    if (sourceExtension == nullptr)
    {
        Log("Bad source file name");
        goto cleanup;
    }

    if (targetFileName == nullptr)
    {
        Log("Out of memory");
        goto cleanup;
    }

    sprintf(targetFileName, "%s-%lu", strrchr(pTargetFileName, PATH_SLASH_CHAR) + 1, TargetId);

    success = CreateFileInQueue(targetFileName, sourceExtension, sourceFileName, false, true);
    if (!success)
    {
        Log("Failed to create data file in queue.");
        goto cleanup;
    }

cleanup:

    MEM_FREE(targetFileName);
    MEM_FREE(sourceFileName);
    return success;
}

char *CctTransferManager::BuildFileName(const char *pPrefix)
{
    CfxHost *host = GetHost(this);
    CctSession *session = GetSession();
    GUID outgoingId = {0};
    char *outgoingFileId = nullptr;
    char *outgoingFileName = nullptr;
    char *result = nullptr;

    host->CreateGuid(&outgoingId);

    outgoingFileId = AllocGuidName(&outgoingId, nullptr);
    outgoingFileName = AllocMaxPath("");

    char *dbFileName;
    dbFileName = session->GetResourceHeader()->ServerFileName;

    if (dbFileName)
    {
        char *p;
        p = strrchr(dbFileName, '%');
        if (p) 
        {
            dbFileName = p + 1;
        }

        p = strrchr(dbFileName, '\\');
        if (p) 
        {
            dbFileName = p + 1;
        }

        strcpy(outgoingFileName, "");

        for (int i = 0; i < 16; i++) 
        {
            char c = *dbFileName;
            if (c == '\0') break;
            if (c == '.') break;

            if (((c < 'A') || (c > 'Z')) &&
                ((c < 'a') || (c > 'z')) &&
                ((c < '0') || (c > '9')))
            {
                c = '_';
            }

            char sz[2];
            sz[0] = c;
            sz[1] = '\0';
            
            strncat(outgoingFileName, sz, 1);
            dbFileName++;
        }
        
        strcat(outgoingFileName, "_");
    }

    result = AllocMaxPath(nullptr);
    if (pPrefix)
    {
        strcat(result, pPrefix);
    }
    strcat(result, outgoingFileName);
    strcat(result, outgoingFileId);

    MEM_FREE(outgoingFileId);
    MEM_FREE(outgoingFileName);

    return result;
}

bool CctTransferManager::HasUnsentData()
{
    auto *classicView = GetClassicView(this);
    auto sightingDatabase = GetSession()->CreateSightingDatabasePtr();

    if (sightingDatabase->HasUnsentData())
    {
        return true;
    }

    return classicView->form()->trackFile()->hasLocations();
}

bool CctTransferManager::CreateCTX(char *pFileName, FXFILELIST *pOutMediaFileList, bool *pOutHasData, FXEXPORTFILEINFO* pExportFileInfo)
{
    bool success = false;

    FXEXPORTFILEINFO localExportFileInfo = {};
    if (!pExportFileInfo)
    {
        pExportFileInfo = &localExportFileInfo;
    }
    else
    {
        *pExportFileInfo = {};
    }

    CfxStream stream;
    CfxFileStream fileStream;
    CctSession *session = GetSession();
    auto sightingDatabase = session->CreateSightingDatabasePtr();

    char *tempPath = nullptr;
    char *workPath = nullptr;
	char *serverFileName = nullptr;

    workPath = AllocMaxPath(nullptr);
    qFatalIf(!workPath, "Failed to allocate working space");

    // Get the temporary directory and clear it
    tempPath = session->AllocTempMaxPath(true);
    qFatalIf(!tempPath, "Failed to create temp directory");

    // Sightings and Waypoints.
    strcpy(workPath, tempPath);
    strcat(workPath, "Sightings.dat");
    sightingDatabase->MakeExportDAT(workPath, pOutMediaFileList, pExportFileInfo);

    strcpy(workPath, tempPath);
    strcat(workPath, "Waypoints.dat");
    MakeWAY(workPath, pExportFileInfo);

    // Check if we have data.
    *pOutHasData = pExportFileInfo->sightingCount > 0 || pExportFileInfo->waypointCount > 0;
    if (!*pOutHasData)
    {
        success = true;
        goto cleanup;
    }

    // Elements.txt
    strcpy(workPath, tempPath);
    strcat(workPath, "Elements.txt");

    if (!session->WriteElementsFile(workPath))
    {
        Log("Failed to create elements file");

        // This should not be a fatal failure if someone deleted the Elements.txt file.
    }

    // Info.xml

    serverFileName = AllocMaxPath(session->GetResourceHeader()->ServerFileName);
    StringReplace(&serverFileName, "&", "&amp;");
    StringReplace(&serverFileName, ">", "&gt;");
    StringReplace(&serverFileName, "<", "&lt;");
    StringReplace(&serverFileName, "'", "&apos;");

    stream.Clear();
    stream.Write("<?xml version=\"1.0\"?>\r\n");
    stream.Write("<Info>\r\n");
    stream.Write("\t<CTXVersion>3295</CTXVersion>\r\n");
    stream.Write("\t<SourceDatabase>");
    stream.Write(serverFileName);
    stream.Write("</SourceDatabase>\r\n");
    stream.Write("</Info>\r\n");

    MEM_FREE(serverFileName);

    strcpy(workPath, tempPath);
    strcat(workPath, "Info.xml");

    qFatalIf(!fileStream.OpenForWrite(workPath), "Failed to create Info.xml");
    qFatalIf(!fileStream.Write(stream.GetMemory(), stream.GetSize()), "Failed to write Info.xml file");

    fileStream.Close();

    // Create the CAB file
    Log("CreateCAB %s, %s", tempPath, workPath);
    if (!CreateCAB(tempPath, pFileName))
    {
        Log("Failed to create CAB file.");
        goto cleanup;
    }

    success = true;

cleanup:

    fileStream.Close();

    if (!*pOutHasData)
    {
        FxDeleteFile(pFileName);
    }

    if (tempPath)
    {
        ClearDirectory(tempPath);
    }

    MEM_FREE(tempPath);
    MEM_FREE(workPath);
    
    return success;
}

bool CctTransferManager::CreateJSON(char *pFileName, int Protocol, const QByteArray& classicBlob, FXFILELIST *pOutMediaFileList)
{
    CfxFileStream fileStream;
    CctJsonBuilder jsonBuilder(GetSession(), &fileStream, Protocol);
    CctSighting sighting(this);
    char *workPath = AllocMaxPath("");

    sighting.LoadBlob(classicBlob);

    strcpy(workPath, pFileName);
    strcat(workPath, "-0");
    qFatalIf(!fileStream.OpenForWrite(workPath), "Failed to open stream for write");
    qFatalIf(!jsonBuilder.WriteHeader(), "Failed to write stream");
    qFatalIf(!jsonBuilder.WriteSighting(&sighting, nullptr, true, pOutMediaFileList), "Failed to export sighting to JSON");
    qFatalIf(!jsonBuilder.WriteFooter(), "Failed to write stream");
    fileStream.Close();

    return true;
}

bool CctTransferManager::CreateJSONWaypoints(char *pFileName, int Protocol)
{
    CfxFileStream fileStream;
    CctJsonBuilder jsonBuilder(GetSession(), &fileStream, Protocol);
    bool success = false;
    int waypointCount = 0;
    char *workPath = AllocMaxPath("");

    strcpy(workPath, pFileName);
    strcat(workPath, "-1");
    if (!fileStream.OpenForWrite(workPath))
    {
        Log("Failed to open stream for write %s:%d", workPath, fileStream.GetLastError());
        goto cleanup;
    }

    if (!jsonBuilder.WriteHeader())
    {
        Log("Failed to write stream %s:%d", workPath, fileStream.GetLastError());
        goto cleanup;
    }

    {
        GUID deviceId = {};
        GetHost(this)->GetDeviceId(&deviceId);

        auto *classicView = GetClassicView(this);
        classicView->form()->trackFile()->enumLocations([&](const QString& locationUid, Location* location)
        {
            WAYPOINT waypoint;
            LocationToWaypoint(locationUid, location, deviceId, &waypoint);

            if (!jsonBuilder.WriteWaypoint(&waypoint, waypointCount == 0))
            {
                Log("Failed to write waypoint to JSON");
                return;
            }

            waypointCount++;
        });
    }

    if (!jsonBuilder.WriteFooter())
    {
        Log("Failed to write stream %s:%d", workPath, fileStream.GetLastError());
        goto cleanup;
    }

    fileStream.Close();

    if (waypointCount == 0)
    {
        FxDeleteFile(workPath);
    }

    success = true;

cleanup:

    fileStream.Close();

    return success;
}

bool CctTransferManager::FlushData(const QString& projectUid)
{
    auto form = App::instance()->createFormPtr(projectUid, {}, true);
    auto classicView = std::unique_ptr<ClassicView>(new ClassicView(form.get()));
    classicView->componentComplete();
    classicView->setSize(QSize(256, 256));
    classicView->connectSession();
    classicView->session()->GetTransferManager()->CreateTransfer(true);

    return true;
}

void CctTransferManager::ExportLegacyFiles()
{
    auto database = GetSession()->CreateSightingDatabasePtr();
    database->MakeDAT();

    auto *session = GetSession();
    auto *host = GetHost(session);

    char datName[MAX_PATH];
    strcpy(datName, session->GetName());
    strcat(datName, ".DAT");
    char *filePath = host->AllocPathApplication(datName);

    FXEXPORTFILEINFO exportFileInfo = {};
    MakeWAY(filePath, &exportFileInfo);
}

bool CctTransferManager::CreateTransfer(bool ForceClear)
{
    bool success = false;
    CctSession *session = GetSession();
    char *workPath = nullptr;
    char *outgoingFileNameBase = nullptr;
    FXFILELIST mediaFileList;
    bool hasData = false;
    FXSENDDATA sendData = session->GetResourceHeader()->SendData;

    // Override the username and password from the session if they exist.
    if (session->GetUserName() && session->GetPassword())
    {
        strcpy(sendData.UserName, session->GetUserName());
        strcpy(sendData.Password, session->GetPassword());
    }

    workPath = AllocMaxPath(nullptr);
    qFatalIf(!workPath, "Failed to allocate working space");

    outgoingFileNameBase = BuildFileName();
    qFatalIf(!outgoingFileNameBase, "Failed to allocate working space");

    switch (sendData.Protocol)
    {
    case FXSENDDATA_PROTOCOL_UNC:
        qDebug() << "UNC is not supported";
        break;

    case FXSENDDATA_PROTOCOL_FTP:
    case FXSENDDATA_PROTOCOL_AZURE:
        {
            auto filename = QString("%1_0%2").arg(outgoingFileNameBase, CLIENT_EXPORT_EXT);
            auto filePath = App::instance()->getMediaFilePath(filename);
            auto filePath_str = filePath.toStdString();

            FXEXPORTFILEINFO exportFileInfo = {};
            success = CreateCTX(filePath_str.data(), &mediaFileList, &hasData, &exportFileInfo);

            if (FxDoesFileExist(filePath_str.data()))
            {
                GetHost(this)->AddToUploadQueue(QStringList { filename }, &sendData, &exportFileInfo);
            }
        }
        break;

    case FXSENDDATA_PROTOCOL_UNKNOWN:
        {
            ExportLegacyFiles();

            strcpy(workPath, session->GetOutgoingPath());
            AppendSlash(workPath);
            strcat(workPath, outgoingFileNameBase);

            auto filePath = (QString(workPath) + "_0" + QString(CLIENT_EXPORT_EXT)).toLatin1();
            auto filePath_str = filePath.data();

            FXEXPORTFILEINFO exportFileInfo = {};
            success = CreateCTX(filePath_str, &mediaFileList, &hasData, &exportFileInfo);
            if (FxDoesFileExist(filePath_str))
            {
                GetHost(this)->RegisterExportFile(filePath_str, &exportFileInfo);
                ForceClear = true;
            }
        }

        break;

    case FXSENDDATA_PROTOCOL_BACKUP:
        {
            ExportLegacyFiles();

            strcpy(workPath, session->GetBackupPath() ? session->GetBackupPath() : session->GetOutgoingPath());
            AppendSlash(workPath);
            strcat(workPath, outgoingFileNameBase);

            auto filePath = (QString(workPath) + "_0" + QString(CLIENT_EXPORT_EXT)).toLatin1();
            auto filePath_str = filePath.data();

            success = CreateCTX(filePath_str, &mediaFileList, &hasData);
        }
        break;

    case FXSENDDATA_PROTOCOL_HTTP:
    case FXSENDDATA_PROTOCOL_HTTPS:
    case FXSENDDATA_PROTOCOL_HTTPZ:
    case FXSENDDATA_PROTOCOL_ESRI:
        {
            auto *classicView = GetClassicView(this);

            // Collect unsent sighting UIDs and blobs upfront to avoid
            // iterating the database while performing callouts.
            struct SightingEntry { QString uid; QByteArray classicBlob; };
            auto sightingEntries = QList<SightingEntry>();
            App::instance()->database()->enumData(
                classicView->projectUid(), classicView->form()->stateSpace(),
                Database::DataFlag::DB_SIGHTING_FLAG, Database::DataFlag::DB_UPLOADED_FLAG,
                [&](const QString& /*projectUid*/, const QString& uid, const Database::DbData& dbData)
                {
                    auto classicBlob = dbData.data.value("classicBlob").toByteArray();
                    sightingEntries.append(SightingEntry { uid, classicBlob });
                    success = true;
                });

            // Send one sighting at a time, because sightings can include media and be very large.
            for (int i = 0; success && i < sightingEntries.size(); i++)
            {
                auto& entry = sightingEntries[i];

                outgoingFileNameBase = BuildFileName();
                qFatalIf(!outgoingFileNameBase, "Failed to allocate working space");

                auto jsonPath = App::instance()->getMediaFilePath(outgoingFileNameBase);
                auto jsonPath_str = jsonPath.toStdString();

                FileListClear(&mediaFileList);
                success = CreateJSON(jsonPath_str.data(), sendData.Protocol, entry.classicBlob, &mediaFileList);
                if (!success)
                {
                    break;
                }

                auto attachments = QStringList();

                auto filenameSightings = QString("%1-0").arg(outgoingFileNameBase);
                if (QFile::exists(App::instance()->getMediaFilePath(filenameSightings)))
                {
                    attachments.append(filenameSightings);
                }

                if (sendData.Protocol == FXSENDDATA_PROTOCOL_ESRI)
                {
                    for (int j = 0; j < mediaFileList.GetCount(); j++)
                    {
                        attachments.append(mediaFileList.GetPtr(j)->Name);
                    }
                }

                GetHost(this)->AddToUploadQueue(attachments, &sendData, nullptr);

                // Mark uploaded after adding to queue.
                App::instance()->database()->setDataFlags(classicView->projectUid(), entry.uid, Database::DataFlag::DB_UPLOADED_FLAG);
                emit App::instance()->sightingFlagsChanged(classicView->projectUid(), entry.uid);

                MEM_FREE(outgoingFileNameBase);
                outgoingFileNameBase = nullptr;
            }

            // Send unsent track points from the active track file.
            if (success && classicView->form()->trackFile()->hasLocations())
            {
                outgoingFileNameBase = BuildFileName();
                qFatalIf(!outgoingFileNameBase, "Failed to allocate working space");

                auto jsonPath = App::instance()->getMediaFilePath(outgoingFileNameBase);
                auto jsonPath_str = jsonPath.toStdString();

                success = CreateJSONWaypoints(jsonPath_str.data(), sendData.Protocol);
                if (success)
                {
                    auto filenameWaypoints = QString("%1-1").arg(outgoingFileNameBase);
                    if (QFile::exists(App::instance()->getMediaFilePath(filenameWaypoints)))
                    {
                        GetHost(this)->AddToUploadQueue(QStringList { filenameWaypoints }, &sendData, nullptr);
                    }

                    classicView->form()->trackFile()->reset();
                }

                MEM_FREE(outgoingFileNameBase);
                outgoingFileNameBase = nullptr;
            }
        }

        break;
    }

    //
    // Mark data as uploaded.
    //

    if (success)
    {
        auto *classicView = GetClassicView(this);
        auto *form = classicView->form();

        session->CreateSightingDatabasePtr()->MarkUploaded();
        form->trackFile()->reset();
    }

    //
    // Cleanup data if required.
    //

    if (success && (session->GetResourceHeader()->ClearDataOnSend || ForceClear))
    {
        auto *classicView = GetClassicView(this);

        App::instance()->deleteSightingsByFlag(
            classicView->projectUid(), classicView->form()->stateSpace(),
            Database::DataFlag::DB_UPLOADED_FLAG, false);
    }

    session->SaveState();

    MEM_FREE(outgoingFileNameBase);
    MEM_FREE(workPath);

    FileListClear(&mediaFileList);

    session->Fault(success, "Failed to package sightings");

    return success;
}

bool CctTransferManager::CreateAlert(CctSighting *pSighting, FXALERT *pAlert)
{
    bool result = false;
    CctSession *session = GetSession();

    char *tempPath = nullptr;
    char *outgoingFileNameBase = nullptr;
    int protocol = pAlert->SendData.Protocol;
    CfxFileStream fileStream;
    CctJsonBuilder jsonBuilder(session, &fileStream, protocol);
    const char *prefix = nullptr;
    if (pSighting)
    {
        prefix = "!";
    }

    //
    // Build the path.
    //

    tempPath = session->AllocTempMaxPath(true);
    if (tempPath == nullptr)
    {
        Log("Failed to create temp path.");
        goto cleanup;
    }

    outgoingFileNameBase = BuildFileName(prefix);
    if (outgoingFileNameBase == nullptr)
    {
        Log("Failed to create outgoing path.");
        goto cleanup;
    }

    //
    // Create the alert JSON.
    //

    char workPath[MAX_PATH];
    strcpy(workPath, tempPath);
    AppendSlash(workPath);
    strcat(workPath, outgoingFileNameBase);

    if (!fileStream.OpenForWrite(workPath))
    {
        Log("Failed to create file for alert %s: %d", workPath, fileStream.GetLastError());
        goto cleanup;
    }

    if (!jsonBuilder.WriteHeader())
    {
        Log("Failed to write file for alert %d", fileStream.GetLastError());
        goto cleanup;
    }

    if (!jsonBuilder.WriteSighting(pSighting, pAlert, true, nullptr))
    {
        Log("Failed to create JSON for sighting.");
        goto cleanup;
    }

    if (!jsonBuilder.WriteFooter())
    {
        Log("Failed to write file for alert %d", fileStream.GetLastError());
        goto cleanup;
    }

    fileStream.Close();

    //
    // Add to queue.
    //

    if (!PushToOutgoing(workPath, CLIENT_EXPORT_JSON_EXT, &pAlert->SendData))
    {
        Log("Failed to create JSON.");
        goto cleanup;
    }

    //
    // Success.
    //

    result = true;

  cleanup:

    MEM_FREE(outgoingFileNameBase);

    return result;
}

bool CctTransferManager::CreatePing(FXALERT *pAlert)
{
    CfxEngine *engine = GetEngine(this);
    FXGPS_POSITION *lastFix = engine->GetGPSLastFix();

    // Return true here means that the ping will activated.
    // We skip the ping if there is no GPS.
    if (!TestGPS(lastFix)) return true;

    return CreateAlert(nullptr, pAlert);
}

void CctTransferManager::Update()
{
    CfxEngine *engine = GetEngine(this);

    // No timeout or alarm already set.
    if ((_autoSendTimeout == 0) || engine->HasAlarm(this)) return;

    // Set the timer if required
    if (HasOutgoingData() || HasTransferData())
    {
        engine->SetAlarm(this, _autoSendTimeout, "TransferManagerAutoSend");
    }
}

void CctTransferManager::Disconnect()
{
    GetEngine(this)->KillAlarm(this);
    ClearAlerts();
}

void CctTransferManager::OnAlarm()
{
    if (!GetSession()->GetConnected()) return;

    if (HasTransferData())
    {
        CreateTransfer();
    }

    Update();
}

//*************************************************************************************************
// CctUpdateManager

CctUpdateManager::CctUpdateManager(CfxPersistent *pOwner): CfxPersistent(pOwner)
{
}

void CctUpdateManager::DefineState(CfxFiler &F)
{
    F.DefineValue("LastCheckTime", dtDouble, &_lastCheckTime);
    F.DefineValue("LastUpdateTime", dtDouble, &_lastUpdateTime);
}

void CctUpdateManager::Install(const char *pSrcPath, const char *pDstPath, const char *pAppName, const char *pTag, const char *pUrl)
{
    char *workPath = AllocMaxPath(pDstPath);
    AppendSlash(workPath);

    // Build a list of the files to copy.
    FXFILELIST fileList;
    FxBuildFileList(pSrcPath, "*", &fileList);
    char tempPath[MAX_PATH]; 

    for (int i = 0; i < fileList.GetCount(); i++)
    {
        strcpy(tempPath, workPath);

        FXFILE *file = fileList.GetPtr(i);
        if (stricmp(file->Name, "Application.CTS") == 0)
        {
            strcat(tempPath, pAppName);
            strcat(tempPath, ".CTS");
        }
        else if (stricmp(file->Name, "Application.TXT") == 0)
        {
            strcat(tempPath, pAppName);
            strcat(tempPath, ".TXT");
        }
        else if (stricmp(file->Name, "Application.HSI") == 0)
        {
            strcat(tempPath, pAppName);
            strcat(tempPath, ".HSI");
        }
        else if (stricmp(file->Name, "Application.HST") == 0)
        {
            strcat(tempPath, pAppName);
            strcat(tempPath, ".HST");
        }
        else
        {
            strcat(tempPath, file->Name);
        }

        if (!FxCopyFile(file->FullPath, tempPath))
        {
            Log("Copy failed: %s->%s", file->FullPath, tempPath);
        }
        else
        {
            Log("Successfully updated: %s", tempPath);
            FxDeleteFile(file->FullPath);
        }
    }

    FileListClear(&fileList);

    // Fix up the custom tag.
    if (pTag && *pTag)
    {
        strcpy(tempPath, workPath);
        strcat(tempPath, pAppName);
        strcat(tempPath, ".TAG");
        HANDLE fileHandle = FileOpen(tempPath, "wb");
        if (fileHandle)
        {
            FileWrite(fileHandle, (void *)pTag, strlen(pTag) + 1);
            FileClose(fileHandle);
        }
    }

    // Fix up the def url.
    if (pUrl && *pUrl)
    {
        strcpy(tempPath, workPath);
        strcat(tempPath, pAppName);
        strcat(tempPath, ".DEF");
        HANDLE fileHandle = FileOpen(tempPath, "wb");
        if (fileHandle)
        {
            FileWrite(fileHandle, (void *)pUrl, strlen(pUrl) + 1);
            FileClose(fileHandle);
        }
    }

    // Delete state file.
    strcpy(tempPath, workPath);
    strcat(tempPath, pAppName);
    strcat(tempPath, ".STA");
    FxDeleteFile(tempPath);

    MEM_FREE(workPath);
    workPath = nullptr;
}

//*************************************************************************************************
// CctSightingDatabase
//*************************************************************************************************

CctSightingDatabase::CctSightingDatabase(CfxPersistent *pOwner): CfxPersistent(pOwner)
{
}

CctSightingDatabase::~CctSightingDatabase()
{
}

void CctSightingDatabase::EnsureLoaded()
{
    if (_loaded)
    {
        return;
    }

    _loaded = true;

    // Enumerate all sightings from the main database.
    auto *classicView = GetClassicView(this);
    App::instance()->database()->enumData(
        classicView->projectUid(), classicView->form()->stateSpace(),
        Database::DataFlag::DB_SIGHTING_FLAG,
        [&](const QString& /*projectUid*/, const QString& uid, const Database::DbData& dbData)
        {
            auto classicBlob = dbData.data.value("classicBlob").toByteArray();
            if (classicBlob.isEmpty()) return;

            int index = _entries.size();
            _entries.append(Entry { uid, classicBlob });
            _uidToIndex.insert(uid, index);
        });
}

bool CctSightingDatabase::Load(CctSighting *pSighting, const QString &sightingUid)
{
    EnsureLoaded();

    auto it = _uidToIndex.find(sightingUid);
    if (it == _uidToIndex.end())
    {
        return false;
    }

    const auto &entry = _entries[it.value()];
    pSighting->LoadBlob(entry.classicBlob);
    return true;
}

bool CctSightingDatabase::Enum(int Index, QString *pSightingUid)
{
    EnsureLoaded();

    pSightingUid->clear();

    if (Index < 0 || Index >= _entries.size())
    {
        return false;
    }

    *pSightingUid = _entries[Index].sightingUid;
    return true;
}

int CctSightingDatabase::GetCount()
{
    EnsureLoaded();
    return _entries.size();
}

bool CctSightingDatabase::TestId(const QString &sightingUid)
{
    EnsureLoaded();
    return _uidToIndex.contains(sightingUid);
}

bool CctSightingDatabase::GetIndex(const QString &sightingUid, int *pIndex)
{
    EnsureLoaded();

    auto it = _uidToIndex.find(sightingUid);
    if (it == _uidToIndex.end())
    {
        return false;
    }

    *pIndex = it.value();
    return true;
}

void CctSightingDatabase::MakeExportDAT(char *pFilePath, FXFILELIST *pOutMediaFileList, FXEXPORTFILEINFO* pExportFileInfo)
{
    auto *classicView = GetClassicView(this);
    auto *session = classicView->session();

    CfxFileStream fileStream;
    qFatalIf(!fileStream.OpenForWrite(pFilePath), "Failed to open for write: " + QString(pFilePath));

    CctSighting sighting(this);

    App::instance()->database()->enumData(
        classicView->projectUid(), classicView->form()->stateSpace(),
        Database::DataFlag::DB_SIGHTING_FLAG, Database::DataFlag::DB_UPLOADED_FLAG,
        [&](const QString& /*projectUid*/, const QString& /*uid*/, const Database::DbData& dbData)
        {
            auto classicBlob = dbData.data.value("classicBlob").toByteArray();
            if (classicBlob.isEmpty()) return;

            sighting.LoadBlob(classicBlob);
            sighting.BuildExport(session, fileStream, pOutMediaFileList);

            auto dt = EncodeDateTime(sighting.GetDateTime());
            auto startDt = EncodeDateTime(&pExportFileInfo->startTime);
            auto stopDt = EncodeDateTime(&pExportFileInfo->stopTime);

            if (pExportFileInfo->sightingCount == 0 || dt < startDt)
            {
                pExportFileInfo->startTime = *sighting.GetDateTime();
            }
            if (pExportFileInfo->sightingCount == 0 || dt > stopDt)
            {
                pExportFileInfo->stopTime = *sighting.GetDateTime();
            }

            pExportFileInfo->sightingCount++;
        });

    fileStream.Close();

    if (pExportFileInfo->sightingCount == 0)
    {
        FxDeleteFile(pFilePath);
    }
}

void CctSightingDatabase::MakeDAT()
{
    auto *classicView = GetClassicView(this);
    auto *session = classicView->session();
    auto *host = GetHost(session);

    char datName[MAX_PATH];
    strcpy(datName, session->GetName());
    strcat(datName, ".DAT");

    auto database = std::unique_ptr<CfxDatabase>(host->CreateDatabase(datName));
    database->DeleteAll();

    FXDATABASEHEADER header;
    header.Magic = MAGIC_SESSION;
    host->GetDeviceId(&header.DeviceId);
    memcpy(&header.FileName, &session->GetResourceHeader()->ServerFileName, sizeof(header.FileName));
    DBID dbId = INVALID_DBID;
    database->Write(&dbId, &header, sizeof(header));

    App::instance()->database()->enumData(
        classicView->projectUid(), classicView->form()->stateSpace(),
        Database::DataFlag::DB_SIGHTING_FLAG, Database::DataFlag::DB_UPLOADED_FLAG,
        [&](const QString& /*projectUid*/, const QString& /*uid*/, const Database::DbData& dbData)
        {
            auto classicBlob = dbData.data.value("classicBlob").toByteArray();
            if (!classicBlob.isEmpty())
            {
                dbId = INVALID_DBID;
                database->Write(&dbId, (void *)classicBlob.constData(), classicBlob.size());
            }
        });
}

void CctSightingDatabase::MarkUploaded()
{
    auto *classicView = GetClassicView(this);
    App::instance()->database()->setDataFlags(
        classicView->projectUid(), classicView->form()->stateSpace(),
        Database::DataFlag::DB_SIGHTING_FLAG, Database::DataFlag::DB_UPLOADED_FLAG,
        Database::DataFlag::DB_UPLOADED_FLAG);
}

bool CctSightingDatabase::HasUnsentData()
{
    auto *classicView = GetClassicView(this);
    return App::instance()->database()->hasData(
        classicView->projectUid(), classicView->form()->stateSpace(),
        Database::DataFlag::DB_SIGHTING_FLAG,  // flagsOn
        Database::DataFlag::DB_UPLOADED_FLAG); // flagsOff
}

void CctSightingDatabase::DeleteAll()
{
    auto *classicView = GetClassicView(this);
    App::instance()->database()->deleteData(
        classicView->projectUid(), classicView->form()->stateSpace(),
        Database::DataFlag::DB_SIGHTING_FLAG);

    _entries.clear();
    _uidToIndex.clear();
    _loaded = false;
}

bool CctSightingDatabase::DeleteOne(const QString& sightingUid)
{
    auto *classicView = GetClassicView(this);
    auto *database = App::instance()->database();

    if (!database->testData(classicView->projectUid(), sightingUid))
    {
        return false;
    }

    database->deleteData(classicView->projectUid(), sightingUid);

    _entries.clear();
    _uidToIndex.clear();
    _loaded = false;

    return true;
}

//*************************************************************************************************
//
// CfxSessionScreen: Suppress the paint routine, since the parent screen would have painted already
//
class CfxSessionScreen: public CfxScreen
{
public:
    CfxSessionScreen(CfxPersistent *pOwner, CfxControl *pParent): CfxScreen(pOwner, pParent) {};
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) {}
};

//*************************************************************************************************

CfxControl *Create_Control_Session(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctSession(pOwner, pParent);
}

CctSession::CctSession(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent), _events(), _actions(0), _stateActions(0), _transferManager(0), _updateManager(0), _sighting(pOwner), _backupSighting(pOwner), _store(), _globalValues(), _retainStates(), _dialogStates(), _customGotos()
{
    InitControl(&GUID_CONTROL_SESSION);

    memset(&_messageText[0], 0, sizeof(_messageText));

    _screen = _screenRight = nullptr;
    _messageWindow = nullptr;

    _resource = 0;
    _name[0] = '\0';

    _sdPath = nullptr;
    _outgoingPath = nullptr;
    _mediaPath = nullptr;
    _applicationPath = nullptr;
    _tempPath = nullptr;

    _userName = _password = nullptr;

    _stateSlot = 0;
    InitXGuid(&_firstScreenId);

    _shareDataEnabled = false;

    _sightingId = INVALID_DBID;
    _resourceStampId = GUID_0;

    InitXGuid(&_defaultSave1Target);
    InitXGuid(&_defaultSave2Target);

    memset(&_lastGPS, 0, sizeof(_lastGPS));
    _saving = false;
    _transferOnSave = false;

    _pendingGotoTitle = nullptr;

    _actions.SetOwner(this);
    _stateActions.SetOwner(this);
    _transferManager.SetOwner(this);
    _updateManager.SetOwner(this);

    _faulted = false;
    _connected = false;

    _historyItemDef = _historyItem = nullptr;
    _historyItemCount = 0;
    _historyItemIndex = 0;
    _historyItemSize = 0;

    _sightingAccuracyName = nullptr;
    InitGpsAccuracy(&_sightingAccuracy);
    _sightingFixCount = SIGHTING_DEFAULT_FIX_COUNT;

    if (IsLive())
    {
        _screen = new CfxSessionScreen(GetOwner(), this);
        GetWindow()->SetDock(ControlDock::Fill);
        GetWindow()->SetComposite(true);

        _messageWindow = new CfxControl_Button(GetOwner(), this);
        GetMessageWindow()->SetVisible(false);
        GetMessageWindow()->SetOnClick(this, (NotifyMethod)&CctSession::OnMessageClick);
        GetMessageWindow()->SetOnPaint(this, (NotifyMethod)&CctSession::OnMessagePaint);

        CfxApplication *application = GetApplication(this);
        application->RegisterEvent(aeShowMenu, this, (NotifyMethod)&CctSession::OnShowMenu);
    }
}

CctSession::~CctSession()
{
    Disconnect();

    //
    // We have an event manager, so we have to clear our children before it gets destroyed, so they
    // get a chance to clean up any registered events, before the EventManager is freed
    //
    ClearControls();

    //
    // Clear events: we don't care if there are still some events registered - they can never be
    // fired at this point since the controls are gone and soon the session will be gone
    //
    _events.Clear();

    CfxApplication *application = ::GetApplication(this);
    if (application)
    {
        application->UnregisterEvent(aeShowMenu, this);
    }
}

CfxDatabase *CctSession::GetSightingDatabase(bool *pCreated)
{
    bool created = false;

    char name[MAX_PATH];
    strcpy(name, _name);
    strcat(name, ".DAT");
    CfxDatabase *database = GetHost(this)->CreateDatabase(name);

    Fault(database->Verify(), "Could not create Sighting Database");
    if (!_faulted && !ValidateSightingHeader(database))
    {
        created = true;

        // Setup header
        FXDATABASEHEADER header;
        header.Magic = MAGIC_SESSION;
        GetHost(this)->GetDeviceId(&header.DeviceId);
        memcpy(&header.FileName, &GetResourceHeader()->ServerFileName, sizeof(header.FileName));

        // Load the existing first sighting (if it exists)
        void *buffer = nullptr;
        DBID dbId = INVALID_DBID;
        if (database->GetCount() > 0)
        {
            if (database->Enum(0, &dbId))
            {
                database->Read(dbId, &buffer);
            }
        }

        // Write the sighting header to the first entry
        if (!database->Write(&dbId, &header, sizeof(header)))
        {
            Fault(false, "Error writing sighting header: pass 1");
        }

        // If there was already an entry: write it back
        if (buffer)
        {
            dbId = INVALID_DBID;
            if (!database->Write(&dbId, buffer, MEM_SIZE(buffer)))
            {
                Fault(false, "Error writing sighting header: pass 2");
            }
            MEM_FREE(buffer);
        }
    }

    if (pCreated)
    {
        *pCreated = created;
    }

    return database;
}

void CctSession::FreeSightingDatabase(CfxDatabase *pDatabase)
{
    delete pDatabase;
}

std::unique_ptr<CctSightingDatabase> CctSession::CreateSightingDatabasePtr()
{
    return std::make_unique<CctSightingDatabase>(this);
}

bool CctSession::ValidateSightingHeader(CfxDatabase *SightingDatabase)
{
    bool found = false;
    DBID dbId = 0;

    if (SightingDatabase->GetCount() == 0)
    {
        goto Done;
    }

    if (SightingDatabase->Enum(0, &dbId))
    {
        void *buffer = nullptr;
        if (SightingDatabase->Read(dbId, &buffer) && buffer)
        {
            found = ((FXDATABASEHEADER*)buffer)->Magic == MAGIC_SESSION;
        }
        MEM_FREE(buffer);
    }

Done:
    return found;
}

bool CctSession::UseSDStorage()                
{
    return (_sdPath != nullptr) && GetResourceHeader()->UseSD;
}

bool CctSession::SD_Initialize()
{
    if (!UseSDStorage()) return false;

    bool result = false;
    char *serverFileName = nullptr;

    CfxStream stream;
    CfxFileStream fileStream;

    // Create Info.XML file.
    char workPath[MAX_PATH];
    strcpy(workPath, _sdPath);
    strcat(workPath, _name);
    strcat(workPath, ".XML");

    stream.Clear();
    stream.Write("<?xml version=\"1.0\"?>\r\n");
    stream.Write("<Info>\r\n");
    stream.Write("\t<CTXVersion>3295</CTXVersion>\r\n");
    stream.Write("\t<SourceDatabase>");
    stream.Write(GetResourceHeader()->ServerFileName);
    stream.Write("</SourceDatabase>\r\n");
    stream.Write("</Info>\r\n");
    
    if (!fileStream.OpenForWrite(workPath))
    {
        Log("Failed to create Info.xml");
        goto done;
    }

    if (!fileStream.Write(stream.GetMemory(), stream.GetSize()))
    {
        Log("Failed to write Info.xml file: %08lx", fileStream.GetLastError());
        goto done;
    }

    // Write the elements file.
    strcpy(workPath, _sdPath);
    strcat(workPath, _name);
    strcat(workPath, ".TXT");
    result = WriteElementsFile(workPath);

done:

    fileStream.Close();

    return result;
}

void CctSession::SD_Finalize()
{
    if (!UseSDStorage()) return;

    bool hasSighting = false;
    bool hasWaypoint = false;

    char name[MAX_PATH];
    
    strcpy(name, _sdPath);
    strcat(name, _name);
    strcat(name, ".DAT");
    hasSighting = FxDoesFileExist(name);

    strcpy(name, _sdPath);
    strcat(name, _name);
    strcat(name, ".WAY");
    hasWaypoint = FxDoesFileExist(name);

    if (!hasSighting && !hasWaypoint)
    {
        strcpy(name, _sdPath);
        strcat(name, _name);
        strcat(name, ".TXT");
        FxDeleteFile(name);

        strcpy(name, _sdPath);
        strcat(name, _name);
        strcat(name, ".XML");
        FxDeleteFile(name);
    }
}

bool CctSession::EditSighting(const QString& sightingUid)
{
    if (!CreateSightingDatabasePtr()->TestId(sightingUid))
    {
        return false;
    }

    if (GetCurrentSightingUid() == sightingUid)
    {
        Reconnect(sightingUid);
    }
    else
    {
        DoStartEdit(sightingUid);
    }

    return true;
}

bool CctSession::DeleteSighting(const QString& sightingUid)
{
    return CreateSightingDatabasePtr()->DeleteOne(sightingUid);
}

bool CctSession::SD_WriteSighting(CctSighting *pSighting)
{
    if (!UseSDStorage()) return false;

    bool Result = false;

    char *tempFileName = AllocTempMaxPath(false);
    AppendSlash(tempFileName);
    strcat(tempFileName, "Sighting.dat");
    FxDeleteFile(tempFileName);

    CfxFileStream fileStream;
    if (!fileStream.OpenForWrite(tempFileName))
    {
        Log("Error writing to temp file");
        goto cleanup;
    }

    if (pSighting->BuildExport(this, fileStream, nullptr))
    {
        fileStream.Close();

        char name[MAX_PATH];
        strcpy(name, _sdPath);
        strcat(name, _name);
        strcat(name, ".DAT");

        FXFILEMAP fileMap = {0};
        if (FxMapFile(tempFileName, &fileMap))
        {
            Result = GetHost(this)->AppendRecord(name, fileMap.BasePointer, fileMap.FileSize);
            FxUnmapFile(&fileMap);
        }
    }

  cleanup:

    fileStream.Close();
    FxDeleteFile(tempFileName);
    
    MEM_FREE(tempFileName);
    tempFileName = nullptr;

    Fault(Result, "Error saving sighting to SD");

    return Result;
}


bool CctSession::WriteElementsFile(char *pFileName)
{
    bool result = false;
    char *srcName = AllocMaxPath(_name);
    strcat(srcName, ".TXT");
    char *srcPath = GetHost(this)->AllocPathApplication(srcName);

    result = FxCopyFile(srcPath, pFileName);

    MEM_FREE(srcPath);
    MEM_FREE(srcName);

    return result;
}

char *CctSession::AllocMediaFileName(const char *pMediaName)
{
    char *result = AllocMaxPath(_mediaPath);
    if (!result)
    {
        return nullptr;
    }

    if (pMediaName)
    {
        strcat(result, pMediaName);
    }
    else
    {
        GUID mediaId = {0};
        GetHost(this)->CreateGuid(&mediaId);
        auto mediaName = AllocGuidName(&mediaId, nullptr);
        strcat(result, mediaName);
        MEM_FREE(mediaName);
    }

    return result;
}

bool CctSession::DeleteMedia(char *pMediaName, bool KeepBackup)
{
    bool result = false;
    char *fileName = AllocMediaFileName(pMediaName);

    if (fileName)
    {
        result = FxDeleteFile(fileName);

        GetHost(this)->RequestMediaScan(fileName);

        MEM_FREE(fileName);
    }

    if (UseSDStorage() && !KeepBackup)
    {
        char backupPath[MAX_PATH];
        strcpy(backupPath, _sdPath);
        strcat(backupPath, "Media/");
        strcat(backupPath, pMediaName);
        FxDeleteFile(backupPath);
    }

    return result;
}

bool CctSession::BackupMedia(char *pMediaName)
{
    if (!UseSDStorage()) return false;

    char dstPath[MAX_PATH];
    strcpy(dstPath, _sdPath);
    strcat(dstPath, "Media");
    if (!FxCreateDirectory(dstPath))
    {
        return false;
    }

    AppendSlash(dstPath);
    strcat(dstPath, pMediaName);

    char *srcPath = AllocMediaFileName(pMediaName);
    bool Result = FxCopyFile(srcPath, dstPath);
    MEM_FREE(srcPath);

    return Result;
}

char *CctSession::GetMediaName(char *pFileName)
{
    if (pFileName == nullptr)
    {
        return nullptr;
    }

    int prefixLen = strlen(_mediaPath);
    int len = strlen(pFileName);

    if (len > prefixLen)
    {
        return pFileName + prefixLen;
    }
    else
    {
        return nullptr;
    }
}

char *CctSession::AllocTempMaxPath(bool Clear)
{
    char *path = AllocMaxPath(_tempPath);
    if (!path)
    {
        return nullptr;
    }

    if (Clear)
    {
        if (!ClearDirectory(path))
        {
            MEM_FREE(path);
            return nullptr;
        }
    }

    AppendSlash(path);
    return path;
}

bool CctSession::InitPaths()
{
    MEM_FREE(_applicationPath);
    _applicationPath = nullptr;
    MEM_FREE(_mediaPath);
    _mediaPath = nullptr;
    MEM_FREE(_outgoingPath);
    _outgoingPath = nullptr;
    MEM_FREE(_tempPath);
    _tempPath = nullptr;
    MEM_FREE(_sdPath);
    _sdPath = nullptr;

    bool result = false;
    char *path = nullptr;
    
    //
    // Application path.
    //

    path = GetHost(this)->AllocPathApplication(nullptr);
    if (path == nullptr)
    {
        goto cleanup;
    }


    _applicationPath = AllocString(path);
    MEM_FREE(path);
    path = nullptr;

    //
    // SD Path.
    // This is only active if UseSD or backup.
    // Android writes to this path for backup, but will not use it for anything else.
    // We are moving to a model where backup happens at regular intervals via a timer.
    //

    if (GetResourceHeader()->UseSD || (GetResourceHeader()->SendData.Protocol == FXSENDDATA_PROTOCOL_BACKUP))
    {
        path = GetHost(this)->AllocPathSD();
        if (path)
        {
            _sdPath = AllocMaxPath(path);
            AppendSlash(_sdPath);
            MEM_FREE(path);
        }
    }

    //
    // Media path: use the main media folder.
    //

    {
        auto mainMediaPath = App::instance()->mediaPath().toStdString() + "/";
        _mediaPath = AllocString(mainMediaPath.c_str());
    }

    //
    // Outgoing path.
    //


    if (_outgoingPath == nullptr)
    {
        path = AllocMaxPath(_applicationPath);
        if (!path)
        {
            Log("Bad outgoing path");
            goto cleanup;
        }
        strcat(path, "Outgoing");
        AppendSlash(path);
        _outgoingPath = AllocString(path);
    }
    MEM_FREE(path);
    path = nullptr;

    //
    // Temp path.
    //

    path = AllocMaxPath(_applicationPath);
    if (!path)
    {
        Log("Bad temp path");
        goto cleanup;
    }
    strcat(path, "Temp");
    AppendSlash(path);
    _tempPath = AllocString(path);
    MEM_FREE(path);
    path = nullptr;

    //
    // Create the directories.
    //

    if (!FxCreateDirectory(_applicationPath))
    {
        Log("Cannot create ApplicationPath %s", _applicationPath);
        goto cleanup;
    }

    if (!FxCreateDirectory(_outgoingPath))
    {
        Log("Cannot create OutgoingPath %s", _outgoingPath);
        goto cleanup;
    }

    if (!FxCreateDirectory(_tempPath))
    {
        Log("Cannot create TempPath %s", _tempPath);
        goto cleanup;
    }

    if (!ClearDirectory(_tempPath))
    {
        Log("Cannot clear TempPath");
        goto cleanup;
    }

    result = true;

cleanup:

    MEM_FREE(path);

    return result;
}

void CctSession::FreePaths()
{

    MEM_FREE(_tempPath);
    _tempPath = nullptr;

    MEM_FREE(_outgoingPath);
    _outgoingPath = nullptr;

    MEM_FREE(_mediaPath);
    _mediaPath = nullptr;

    MEM_FREE(_applicationPath);
    _applicationPath = nullptr;

    MEM_FREE(_sdPath);
    _sdPath = nullptr;
}

bool CctSession::InitSightingDatabase()
{
    bool created = false;

    CfxDatabase *database = GetSightingDatabase(&created);
    FreeSightingDatabase(database);

    return created;
}

GUID CctSession::GetCurrentSequenceId()
{
    if (GetConnected()) 
    {
        return _resource->GetHeader(this)->StampId;
    }
    else
    {
        return GUID_0;
    }
}

bool CctSession::MatchProfileExact(CTRESOURCEHEADER *pHeader, int Width, int Height, int DPI, FXPROFILE *pMatch)
{
    bool result = false;

    for (int i = 0; (i < pHeader->ProfileCount) && !result; i++)
    {
        FXPROFILE *p = _resource->GetProfile(this, i);

        double dh, ddpi;
        dh = (double)FxAbs((int)p->Height - (int)Height) / (double)Height;
        ddpi = (DPI != 0) ? (double)FxAbs((int)p->DPI - (int)DPI) / (double)DPI : 0.0;

        if ((p->Width == Width) && (dh < 0.1) && (ddpi < 0.1))
        {
            *pMatch = *p;
            result = true;
        }

        _resource->ReleaseProfile(p);
    }

    return result;
}

bool CctSession::MatchProfileFuzzy(CTRESOURCEHEADER *pHeader, int Width, int Height, int DPI, FXPROFILE *pMatch)
{
    int delta = 10000;
    int multiplier, t;

    for (int i = 0; i < pHeader->ProfileCount; i++)
    {
        FXPROFILE *p = _resource->GetProfile(this, i);
        double ddpi = (DPI != 0) ? (double)FxAbs((int)p->DPI - (int)DPI) / (double)DPI : 0.0;

        // Check DPI: 0 means don't care.
        if ((DPI != 0) && (ddpi > 0.1))
        {
            _resource->ReleaseProfile(p);
            continue;
        }

        // Width must be exactly a multiple.
        if (Width % p->Width != 0)
        {
            _resource->ReleaseProfile(p);
            continue;
        }
        
        multiplier = Width / p->Width;
        t = FxAbs((int)Height - ((int)p->Height * (int)multiplier));
        if (t < delta)
        {
            delta = t;
            *pMatch = *p;
            pMatch->Width *= multiplier;
            pMatch->Height *= multiplier;
            pMatch->DPI *= multiplier;
            pMatch->ScaleX *= multiplier;
            pMatch->ScaleY *= multiplier;
            pMatch->ScaleBorder *= multiplier;
            pMatch->ScaleFont *= multiplier;
        }

        _resource->ReleaseProfile(p);
    }

    return ((double)delta / (double)Height) < 0.15;
}

bool CctSession::Connect(char *pName, int StateSlot, XGUID FirstScreenId)
{
    Disconnect();

    CfxDatabase *database = nullptr;
    CfxEngine *engine = GetEngine(this);
    CTRESOURCEHEADER *header = nullptr;
    CfxHost *host = GetHost(this);
    bool sightingDbCreated = false;

    memset(&_lastWaypoint, 0, sizeof(_lastWaypoint));

    // Keep the name for database load/save
    FX_ASSERT(strlen(pName) + 1 < sizeof(_name));
    strcpy(_name, pName);
    _stateSlot = StateSlot;

    _firstScreenId = FirstScreenId;

    // Load the resource: fail over to "Tutorial" if no resource is found
    _resource = host->CreateResource(_name);
    Fault(_resource && _resource->Verify(), "No Applications");
    if (_faulted) goto Done;

    header = GetResourceHeader();
    Fault(header != nullptr, "Cannot read Application header");

    Fault(header->Version >= 510, "Version Error - Requires at least 3.510");
    if (_faulted) goto Done;

    if (host->GetUseResourceScale())
    {
        FXPROFILE *profile = host->GetProfile();
        FXPROFILE profileMatch = {0};
        int w = profile->Width;
        int h = profile->Height;
        int dpi = profile->DPI;//((profile->DPI + 49) / 100) * 100;

        if (MatchProfileExact(header, w, h, dpi, &profileMatch) ||
            MatchProfileExact(header, h, w, dpi, &profileMatch) ||
            MatchProfileFuzzy(header, w, h, dpi, &profileMatch) ||
            MatchProfileFuzzy(header, h, w, dpi, &profileMatch) ||
            MatchProfileFuzzy(header, w, h, 0, &profileMatch) ||
            MatchProfileFuzzy(header, h, w, 0, &profileMatch)) 
        {
            profile->ScaleX = max(100, profileMatch.ScaleX); 
            profile->ScaleY = max(100, profileMatch.ScaleY);
            profile->ScaleBorder = max(100, profileMatch.ScaleBorder);
            profile->ScaleFont = max(100, profileMatch.ScaleFont);
            profile->DPI = profileMatch.DPI;
        }
        else
        {
            Log("No profile match");
            profile->ScaleX = profile->ScaleY = profile->ScaleBorder = profile->ScaleFont = dpi;
            profile->DPI = dpi;
        }

        GetCanvas(this)->SetScaleFont(profile->ScaleFont);
        /*Log("Scaling: x=%d, y=%d, l=%d, f=%d, dpi=%d", 
                 profile->ScaleX, 
                 profile->ScaleY, 
                 profile->ScaleBorder,
                 profile->ScaleFont,
                 profile->DPI);*/
    }

    if (header->ScaleScroller != 100) 
    {
        engine->SetScaleScroller(header->ScaleScroller);
    }

    if (header->ScaleTab != 100) 
    {
        engine->SetScaleTab(header->ScaleTab);
    }

    if (header->ScaleTitle != 100) 
    {
        engine->SetScaleTitle(header->ScaleTitle);
    }

    // Check date/time: ignore for client to prevent weird time change bugs
    if (GetResourceHeader()->TestTime)
    {
        FXDATETIME currentTime;
        host->GetDateTime(&currentTime);
        double time1 = EncodeDateTime(&currentTime); 
        double time2 = EncodeDateTime(&_resource->GetHeader(this)->StampTime);
        double tdiff = EncodeTime(0, 10, 0, 0); // 10 minutes

/*        Fault((time1 > time2) || (fabs(time1 - time2) < tdiff), "Time is wrong");

        if (_faulted) goto Done;*/
    }

    // Initialize the sighting database
    //sightingDbCreated = InitSightingDatabase();
    //if (_faulted) goto Done;

    // Test write the state database
    // The state is now managed in the main database.
    // database = GetStateDatabase();

    // // Reset the state database if the sighting database was just created
    // if (sightingDbCreated && header->ResetStateOnSync)
    // {
    //     database->DeleteAll();
    // }
    // Fault(database->Verify(), "Could not create State Database");
    // FreeStateDatabase(database);
    // database = nullptr;

    if (_faulted) goto Done;

    // Init the default state from the header
    _sightingAccuracy = header->SightingAccuracy;
    _sightingFixCount = max(1, header->SightingFixCount);
    GetHost(this)->SetTrackTimer(header->WaypointTimer);
    GetEngine(this)->SetGPSTimeSync(header->GpsTimeSync, header->GpsTimeZone);

    // Set the projection params for text output of GPS co-ordinates.
    SetProjectionParams((FXPROJECTION)header->Projection, header->UTMZone);

    // This must happen before LoadState, which loads the latest passwords.
    SetCredentials(header->SendData.UserName, header->SendData.Password);

    // Load the session state
    LoadState();
    if (_faulted) goto Done;

    // 
    // If we're starting fresh, pick up the first screen from the resource. 
    // If there is no first screen do nothing.
    //

    // If we're on a device, then the profile is configured by the resource file
    _firstScreenId = _resource->GetHeader(this)->FirstObject;
    _sighting.Init(_firstScreenId);

    if (_faulted) goto Done;

    Fault(InitPaths(), "Init paths");

    SD_Initialize();

    // Configure the transfer manager
    _transferManager.SetAutoSendTimeout(header->AutoSendTimeout);
    _transferManager.Update();

    // Connect if nothing bad happened
    _connected = !_faulted;

Done:
    if (!_connected)
    {
        // Clean up
        Disconnect();
    }

    return _connected;
}

void CctSession::Disconnect()
{
    // State is automatically saved before entering a dialog
    if (_connected && !InDialog())
    {
        // Execute Leave event: we always fire this event before we close a screen
        _events.ExecuteType(seLeave, this);
        
        // Save the actions and the screen state
        SaveCurrentScreen();
        SaveState();    
    }

    // Clear out the sighting
    _sighting.Clear();

    // Disconnect the transfer manager
    SetCredentials(nullptr, nullptr);
    _transferManager.Disconnect();

    // Clear the store
    ClearStore();

    // Finalize the SD backup.
    SD_Finalize();

    _connected = false;
    _stateSlot = 0;
    InitXGuid(&_firstScreenId);

    delete _resource;
    _resource = 0;

    MEM_FREE(_pendingGotoTitle);
    _pendingGotoTitle = nullptr;

    MEM_FREE(_sightingAccuracyName);
    _sightingAccuracyName = nullptr;

    FreePaths();

    _name[0] = '\0';
}

void CctSession::Reconnect(const QString &sightingUid, bool DoRepaint)
{
    FX_ASSERT(_connected);

    if (_dialogStates.GetCount() <= 1)
    {
        ClearDialogs();

        if (!sightingUid.isEmpty() && sightingUid != GetCurrentSightingUid())
        {
            _sightingId = 1; // Mark as editing
            CreateSightingDatabasePtr()->Load(&_sighting, sightingUid);
            SaveState();
        }

        if (_faulted) return;

        // Load the current screen
        LoadCurrentScreenAndState();

        // Reverse the actions on the new screen
        _actions.Rollback(this);
        _actions.Clear();

        // Execute Enter event
        _events.ExecuteType(seEnter, this);

        // Process Element effects
        ProcessElementEffects();

        if (DoRepaint)
        {
            Repaint();
        }

        return;
    }

    FX_ASSERT(_dialogStates.GetCount() > 1);
    {
        MEM_FREE(_dialogStates.GetPtr(0)->StateData);
        _dialogStates.Delete(0);

        DIALOGSTATE *dialogState;
        dialogState = _dialogStates.GetPtr(0);

        FXDIALOG *dialog = (FXDIALOG *)_resource->GetDialog(this, &dialogState->Id);
        Fault((dialog != nullptr), "Attempt to load invalid Dialog");
        if (dialog)
        {
            CfxStream stream(&dialog->Data[0]);
            CfxReader reader(&stream);
           
            CfxScreen *screen = GetWindow();
            screen->ClearControls();
            Repaint();

            screen->DefineProperties(reader);

            CfxDialog *editor = (CfxDialog *)screen->FindControlByType(&dialogState->Id);
            if (editor)
            {
                editor->SetBounds(0, 0, GetClientWidth(), GetClientHeight());
                GetEngine(this)->AlignControls(screen);
            }
            
            CfxStream streamState(dialogState->StateData);
            CfxReader readerState(&streamState);
            screen->DefineState(readerState);

            _resource->ReleaseDialog(dialog);
        }

        if (DoRepaint)
        {
            Repaint();
        }

        MEM_FREE(dialogState->StateData);
        dialogState->StateData = nullptr;

        return;
    }
}

void CctSession::Run()
{
    if (!_faulted)
    {
        // Load the screen and it's state
        LoadCurrentScreenAndState();

        // Reverse the actions on this screen
        _actions.Rollback(this);
        _actions.Clear();

        // Execute Enter event
        _events.ExecuteType(seEnter, this);

        // Process Element effects
        ProcessElementEffects();
    }

    Repaint();
}

void CctSession::Fault(bool Condition, const char *pError)
{ 
    if (Condition) return;

    // We're in a bad state now
    _faulted = true;

    // Disable the screen
    GetWindow()->SetEnabled(false);

    // Force an align in case this happened on startup and we don't yet have a valid size
    GetEngine(this)->AlignControls(GetApplication(this)->GetScreen());

    // Setup fault window
    int h = (GetHost(this)->GetProfile()->ScaleY * 40) / 100;
    GetMessageWindow()->SetBounds(_left - 1, _top + (_height - h) / 2, _width + 2, h);
    GetMessageWindow()->SetVisible(true);

    sprintf(_messageText, "Press to %s: %s.", _connected ? "Reset" : "Exit", pError);

    // Repaint to show the fault message
    Repaint();
}

void CctSession::ShowMessage(const char *pMessage, COLOR BackColor)
{
    GetHost(this)->ShowToast(pMessage);
}

void CctSession::SetCredentials(char *pUserName, char *pPassword)
{
    MEM_FREE(_userName);
    _userName = nullptr;
    if (pUserName)
    {
        _userName = AllocString(pUserName);
    }

    MEM_FREE(_password);
    _password = nullptr;

    if (pPassword)
    {
        _password = AllocString(pPassword);
    }
}

void CctSession::OnMessageClick(CfxControl *pSender, void *pParams)
{
    // If we never connected then we cannot reset, because things aren't going to get better
    if (!_connected)
    {
        GetEngine(this)->Shutdown();
        return;
    }

    // Enable the screen
    GetWindow()->SetEnabled(true);

    // Hide the fault window
    GetMessageWindow()->SetVisible(false);

    // Put us back in a good state
    if (_faulted)
    {
        _faulted = false;

        // Clear the state screen actions
        _stateActions.Clear();

        // Clear out the Path
        _sighting.Clear();
        _sighting.Init(_firstScreenId);

        // Start again
        Run();
    }
    else
    {
        Repaint();
    }
}

void CctSession::OnMessagePaint(CfxControl *pSender, void *pParams)
{
    FXPAINTDATA *paintData = (FXPAINTDATA *)pParams; 
    CfxCanvas *canvas = paintData->Canvas;

    canvas->State.BrushColor = _faulted ? 0x000080 : _messageBackColor;
    canvas->FillRect(paintData->Rect);

    canvas->SystemDrawText(_messageText, paintData->Rect, 0xFFFFFF, canvas->State.BrushColor);
}

void CctSession::OnShowMenu(CfxPersistent *pSender, void *pParams)
{
    if (!IsEditing())
    {
        ShowSightingEditorPage();
    }
}

void CctSession::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
}

void CctSession::DefineState(CfxFiler &F)
{
    // Define header magic number
    int magic = MAGIC_SESSION;
    F.DefineValue("Magic", dtInt32, &magic);
    FX_ASSERT(magic == MAGIC_SESSION);

    // Define the Server database filename: perhaps should use connection string
    CTRESOURCEHEADER *header = GetResourceHeader();
    
    if (F.IsWriter()) 
    {
    	F.DefineValue("ServerFileName", dtText, &header->ServerFileName);
    }
    else
    {
        char ServerFileName[256];
    	F.DefineValue("ServerFileName", dtText, &ServerFileName);
    }

    // Active sighting Id
    F.DefineValue("SightingId", dtInt32, &_sightingId);

    // Stamp GUID so we stay in sync with the resource file
    _resourceStampId = header->StampId;
    F.DefineValue("StampId", dtGuid,  &_resourceStampId);

    // Bail out if the resource stamp is out of sync. Typically after an update.
    if (F.IsReader() && !CompareGuid(&_resource->GetHeader(this)->StampId, &_resourceStampId))
    {
        return;
    }
    
    //
    // We can change any of this at will, because the above check will always be in sync with the
    // resource file - which is versioned.
    //
    _stateActions.DefineProperties(F);
    _stateActions.DefineState(F);

    // Retained for state file backward compatibility (was CctWaypointManager::DefineState).
    int waypointTimeoutDummy = 0, waypointMinTimeoutDummy = 0;
    double waypointAccuracyDOPDummy = 0, waypointAccuracyMaxSpeedDummy = 0;
    F.DefineValue("WaypointTimeout", dtInt32, &waypointTimeoutDummy);
    F.DefineValue("WaypointMinTimeout", dtInt32, &waypointMinTimeoutDummy);
    F.DefineValue("WaypointAccuracyDOP", dtDouble, &waypointAccuracyDOPDummy);
    F.DefineValue("WaypointAccuracyMaxSpeed", dtDouble, &waypointAccuracyMaxSpeedDummy);

    F.DefineValue("SightingAccuracyDOP", dtDouble, &_sightingAccuracy.DilutionOfPrecision);
    F.DefineValue("SightingAccuracyMaxSpeed", dtDouble, &_sightingAccuracy.MaximumSpeed);
    F.DefineValue("SightingFixCount", dtInt32, &_sightingFixCount);

    // Retained for state file backward compatibility
    bool rangeFinderConnectedDummy = false;
    F.DefineValue("RangeFinderConnected", dtBoolean, &rangeFinderConnectedDummy);

    // Track default save targets
    F.DefineXOBJECT("DefaultSave1Target", &_defaultSave1Target);
    F.DefineXOBJECT("DefaultSave2Target", &_defaultSave2Target);

    _sighting.DefineProperties(F);

    // Validate the sightingId
    if (_sightingId != INVALID_DBID)
    {
        auto sightingUid = GetCurrentSightingUid();
        if (!sightingUid.isEmpty() && CreateSightingDatabasePtr()->TestId(sightingUid))
        {
            _backupSighting.DefineProperties(F);
        }
        else
        {
            _sightingId = INVALID_DBID;
        }
    }

    // Store the last known good GPS location
    F.DefineGPS(&_lastGPS);

    // Make the last GPS state persistent
    F.DefineGPS(GetEngine(this)->GetGPSLastFix());
    F.DefineDateTime(GetEngine(this)->GetGPSLastTimeStamp());

    // Item store
    if (F.IsReader())
    {
        ClearStore();
        while (!F.ListEnd())
        {
            STOREITEM storeItem;
            F.DefineValue("PrimaryId", dtGuid,  &storeItem.PrimaryId);
            F.DefineValue("ControlId", dtInt32, &storeItem.ControlId);
            storeItem.Data = nullptr;
            F.DefineValue("Data", dtPBinary, &storeItem.Data);
            _store.Add(storeItem);
        }
    }
    else
    {
        int i;
        for (i=0; i<_store.GetCount(); i++)
        {
            STOREITEM storeItem = _store.Get(i);
            F.DefineValue("PrimaryId", dtGuid,  &storeItem.PrimaryId);
            F.DefineValue("ControlId", dtInt32, &storeItem.ControlId);
            F.DefineValue("Data", dtPBinary, &storeItem.Data);
        }
        F.ListEnd();
    }

    // Global values
    F.DefineValue("GlobalValues", dtPBinary, _globalValues.GetMemoryPtr());

    // Gotos
    F.DefineValue("PendingGotoTitle", dtPText, &_pendingGotoTitle);
    F.DefineValue("CustomGotos", dtPBinary, _customGotos.GetMemoryPtr());

    // NOT USED: uniqueness number for sightings and waypoints
    int dataUniqueness = 0;
    F.DefineValue("DataUniqueness", dtInt32, &dataUniqueness);

    // Transfer manager
    _transferManager.DefineState(F);
    F.DefineValue("UserName", dtPText, &_userName);
    F.DefineValue("Password", dtPText, &_password);

    // Update manager
    _updateManager.DefineState(F);

    // Last waypoint
    F.DefineValue("LastWaypoint_DateCurrent", dtDouble, &_lastWaypoint.DateCurrent);
    F.DefineValue("LastWaypoint_Latitude", dtDouble, &_lastWaypoint.Latitude);
    F.DefineValue("LastWaypoint_Longitude", dtDouble, &_lastWaypoint.Longitude);

    // GPS time offset
    double gpsTimeOffset = GetHost(this)->GetGPSTimeOffset();
    F.DefineValue("GpsTimeOffset", dtDouble, &gpsTimeOffset);
    if (F.IsReader())
    {
        GetHost(this)->SetGPSTimeOffset(gpsTimeOffset);
    }

    // Share data support
    // Hack because we did not change the resource version number.
    auto reachedEnd = false;
    if (F.IsReader())
    {
        if (static_cast<CfxReader&>(F).IsEnd())
        {
            _shareDataEnabled = false;
            return;
        }
    }

    F.DefineValue("ShareDataEnabled", dtBoolean, &_shareDataEnabled);
}

void CctSession::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctSession::DefineResources(CfxFilerResource &F)
{
}

void CctSession::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    if (_screenRight)
    {
        int w = (Width - 4) / 2;
        GetWindow()->SetBounds(0, 0, w, Height);
        _screenRight->SetBounds(w + 4, 0, w, _height);
    }
    else
    {
        GetWindow()->SetBounds(0, 0, Width, Height);
    }
}

void CctSession::LoadState()
{
    GetClassicView(this)->loadState();
}

void CctSession::ForceTitleBar(CfxScreen *pScreen)
{
    CfxEngine *engine = GetEngine(this);
    engine->AlignControls(pScreen);

    CfxControl_Panel *topPanel = nullptr;

    CfxControl *topControl = engine->HitTest(pScreen, 0, 0);
    if (topControl && 
        (topControl->GetDock() == ControlDock::Top) &&
        (topControl->GetHeight() < (60 * GetHost(this)->GetProfile()->ScaleY) / 100) &&
        CompareGuid(topControl->GetTypeId(), &GUID_CONTROL_PANEL))
    {
        topPanel = (CfxControl_Panel *)topControl;
    }

    CfxControl_TitleBar *titleBar = new CfxControl_TitleBar(GetOwner(), pScreen);
    titleBar->SetDock(ControlDock::Top);
    titleBar->SetBounds(0, 0, 100, titleBar->GetHeight());
    titleBar->SetShowBattery(false);
    titleBar->SetShowTime(false);
    titleBar->SetShowMenu(true);
    titleBar->AssignFont(pScreen->GetFont());
    titleBar->SetShowBattery(true);
    titleBar->SetShowTime(true);
    titleBar->SetShowExit(true);
        
    if (topPanel)
    {
        titleBar->SetCaption(topPanel->GetVisibleCaption());
        titleBar->SetBorderStyle(ControlBorderStyle::None);
        titleBar->SetColor(topPanel->GetColor());
        titleBar->SetTextColor(topPanel->GetTextColor());

        topPanel->SetBounds(0, titleBar->GetHeight(), 100, 0);
        topPanel->SetCaption(nullptr);
    }
    else
    {
        if (pScreen->GetName())
        {
            titleBar->SetUseScreenName(true);
        }
        else
        {
            titleBar->SetCaption("Client");
        }
    }
}

void CctSession::LoadScreen(CfxScreen *pScreen, XGUID ScreenId, void *pState)
{
    CfxHost *host = GetApplication(this)->GetHost();

    Fault(host->GetFreeMemory() > 65536 * 2, "Memory Low");

    // Load the screen resource
    FXSCREENDATA *screenData = (FXSCREENDATA *)_resource->GetObject(this, ScreenId);
    Fault((screenData != nullptr), "Attempt to load invalid screen");
    if (screenData)
    {
        FX_ASSERT(screenData->Magic == MAGIC_SCREEN);

        CfxStream stream(&screenData->Data[0]);
        CfxReader reader(&stream);
        pScreen->ClearControls();
        pScreen->DefineProperties(reader);
        _resource->ReleaseObject(screenData);

        if (GetResourceHeader()->ForceTitleBar)
        {
            ForceTitleBar(pScreen);
        }
    }

    // Load the new screen state
    _actions.Clear();

    if (pState)
    {
        CfxStream stream(pState);
        CfxReader reader(&stream);
        pScreen->DefineState(reader);

        _actions.DefineProperties(reader);
        _actions.DefineState(reader);
    }

    GetEngine(this)->AlignControls(this);    

}

void CctSession::LoadCurrentScreen()
{
    LoadScreen(GetWindow(), *_sighting.GetCurrentScreenId(), nullptr);
}

void CctSession::LoadCurrentScreenAndState()
{
    XGUID screenId = *_sighting.GetCurrentScreenId();
    void *screenData = _sighting.GetScreenData(screenId, _sighting.GetPathIndex());
    LoadScreen(GetWindow(), screenId, screenData);
}

QByteArray CctSession::SaveStateBlob(QStringList* attachments)
{
    // Save the session state.
    CfxStream stream;
    CfxWriter writer(&stream);
    DefineState(writer);

    // Capture the attachments from the sightings.
    *attachments += _sighting.GetMediaFileList();
    *attachments += _backupSighting.GetMediaFileList();

    return QByteArray((const char *)stream.GetMemory(), stream.GetSize());
}

void CctSession::LoadStateBlob(const QByteArray& blob)
{
    if (blob.isEmpty())
    {
        return;
    }

    CfxStream stream((void *)blob.constData(), blob.size());
    CfxReader reader(&stream);
    DefineState(reader);

    // Check to see if something went out of sync
    if (!CompareGuid(&_resource->GetHeader(this)->StampId, &_resourceStampId))
    {
        _stateActions.Clear();
        _sighting.Clear();
        _sighting.Init(_firstScreenId);
    }
}

void CctSession::SaveState()
{
//     DBID dbId = INVALID_DBID;

//     CfxStream stream;
//     CfxWriter writer(&stream);

//     CfxDatabase *database = GetStateDatabase();
//     Fault(database->Verify(), "Could not create Database");
//     if (_faulted) goto Done;
//     Fault(!database->Full(), "Database Full");
//     if (_faulted) goto Done;

//     // Write out the session state
//     DefineState(writer);

//     // Save to the database
//     database->DeleteAll();
//     database->Write(&dbId, stream.GetMemory(), stream.GetSize());

// Done:

//     FreeStateDatabase(database);
//     database = nullptr;

    GetClassicView(this)->saveState();
}

void CctSession::SaveScreen(CfxScreen *pScreen, XGUID ScreenId, int PathIndex)
{
    // Save the screen state
    CfxStream stream;
    CfxWriter writer(&stream);
    pScreen->DefineState(writer);

    // Save the active actions
    _actions.DefineProperties(writer);
    _actions.DefineState(writer);

    // Clone the stream memory and hand it to the sighting to manage
    _sighting.SetScreenData(ScreenId, PathIndex, stream.CloneMemory());
}

void CctSession::SaveCurrentScreen()
{
    if (_sighting.GetPathIndex() < 0) return;

    SaveScreen(GetWindow(), *_sighting.GetCurrentScreenId(), _sighting.GetPathIndex());
}

void CctSession::RegisterEvent(SESSIONEVENT Event, CfxControl *pControl, NotifyMethod Method)
{
    FXEVENT event = {(BYTE)Event, pControl, Method};
    _events.Register(&event);
}

void CctSession::UnregisterEvent(SESSIONEVENT Event, CfxControl *pControl)
{
    _events.Unregister(Event, pControl);
}

void CctSession::RegisterRetainState(CfxControl *pControl, CctRetainState *pRetainState)
{
    RETAINSTATE retainState = { pControl, pRetainState };
    _retainStates.Add(retainState);
}

void CctSession::UnregisterRetainState(CfxControl *pControl)
{
    for (int i = 0; i < _retainStates.GetCount(); i++)
    {
        if (_retainStates.GetPtr(i)->Control == pControl)
        {
            _retainStates.Delete(i);
            break;
        }
    }
}

void CctSession::ProcessElementEffects()
{
    ATTRIBUTE *attribute = nullptr;
    CfxResource *resource = GetResource();

    MEM_FREE(_sightingAccuracyName);
    _sightingAccuracyName = nullptr;

    for (int i = 0; _sighting.EnumAttributes(i, &attribute); i++)
    {
        FXTRAVELMODE *travelMode = nullptr;
        FXTEXTRESOURCE *travelModeName = nullptr;

        if (!IsNullXGuid(&attribute->ElementId))
        {
            travelMode = (FXTRAVELMODE *)resource->GetObject(this, attribute->ElementId, (BYTE)ElementAttribute::TravelMode);
            if (travelMode != nullptr)
            {
                travelModeName = (FXTEXTRESOURCE *)resource->GetObject(this, attribute->ElementId, (BYTE)ElementAttribute::Name);
            }
        }

        if (!IsNullXGuid(&attribute->ValueId))
        {
            if (travelMode == nullptr)
            {
                travelMode = (FXTRAVELMODE *)resource->GetObject(this, attribute->ValueId, (BYTE)ElementAttribute::TravelMode);
                if (travelMode != nullptr)
                {
                    travelModeName = (FXTEXTRESOURCE *)resource->GetObject(this, attribute->ValueId, (BYTE)ElementAttribute::Name);
                }
            }
        }

        if (travelMode != nullptr)
        {
            FX_ASSERT(travelModeName->Magic == MAGIC_TEXT);
            FX_ASSERT(travelMode->Magic == MAGIC_TRAVELMODE);

            MEM_FREE(_sightingAccuracyName);
            _sightingAccuracyName = nullptr;
            _sightingAccuracyName = AllocString(travelModeName->Text);

            SetSightingAccuracy(&travelMode->SightingAccuracy);

            resource->ReleaseObject(travelModeName);
            resource->ReleaseObject(travelMode);
        }
    }
}

void CctSession::EnterScreen()
{
    // Execute Enter event
    _events.ExecuteType(seEnter, this);

    // Load control states
    LoadControlStates();

    // Execute Enter forward event
    _events.ExecuteType(seEnterNext, this);

    // Save state
    SaveState();

    // ProcessGPSFilter
    ProcessElementEffects();
}

void CctSession::LeaveScreen()
{
    // Execute BeforeNext event: used to add screens before traditional next has been processed 
    _events.ExecuteType(seBeforeNext, this);

    // Save control states
    SaveControlStates();

    // Execute Next event: gives controls a chance to add Next actions 
    _events.ExecuteType(seNext, this);

    // Execute Next event: gives controls a chance to add Next actions 
    _events.ExecuteType(seAfterNext, this);

    // Execute Leave event: we always fire this event before we close a screen
    _events.ExecuteType(seLeave, this);
}

bool CctSession::CanDoBack()
{
    if (_faulted || (_sighting.BOP())) return false;

    bool canBack = true;
    _events.ExecuteType(seCanBack, this, &canBack);

    return canBack;
}   

void CctSession::DoBack()
{
    // Give controls a chance to block Back
    if (!CanDoBack()) return;

    // Save control states
    SaveControlStates();

    // Execute Back event
    _events.ExecuteType(seBack, this);

    // Execute Leave event: we always fire this event before we close a screen
    _events.ExecuteType(seLeave, this);

    // Reverse the current actions on the screen we are about to leave
    _actions.Rollback(this);
    _actions.Clear();
    
    // Save the actions and the screen state
    SaveCurrentScreen();

    // Move back one screen
    _sighting.DecPathIndex();

    // Load the screen and it's state
    LoadCurrentScreenAndState();

    // Reverse the actions on the new screen
    _actions.Rollback(this);
    _actions.Clear();

    // Execute Enter event
    _events.ExecuteType(seEnter, this);

    // Execute Enter forward event
    _events.ExecuteType(seEnterBack, this);

    // ProcessGPSFilter
    ProcessElementEffects();

    Repaint();
}

bool CctSession::CanDoNext()
{
    if (_faulted) return false;

    bool canNext = true;
    _events.ExecuteType(seCanNext, this, &canNext);

    return canNext;
}   

void CctSession::DoNext()
{
    // Give controls a chance to block Next
    if (!CanDoNext()) return;

    XGUID currentScreenId = *_sighting.GetCurrentScreenId();

    // Execute leave screen events
    LeaveScreen();
    
    // Make sure we're not going off into the void
    if (_sighting.EOP())
    {
        XGUID nextScreenId;
        _resource->GetNextScreen(this, currentScreenId, &nextScreenId);

        if (IsNullXGuid(&nextScreenId))
        {
            Fault(false, "No Next Screen");
            return;
        }
        else
        {
            CctAction_InsertScreen action(this);
            action.Setup(nextScreenId);
            ExecuteAction(&action);
        }
    }

    // Save the actions and the screen state
    SaveCurrentScreen();

    // Move to the next screen
    _sighting.IncPathIndex();

    // Remove loops from the path
    bool looped = _sighting.FixPathLoop();

    // Load the screen - use state if editing or not ClearOnNext
    if (IsEditing() || !GetResourceHeader()->ClearOnNext || looped)
    {
        LoadCurrentScreenAndState();

        // Reverse the actions on the new screen
        _actions.Rollback(this);
        _actions.Clear();
    }
    else
    {
        LoadCurrentScreen();
    }

    // Start up the screen.
    EnterScreen();

    Repaint();
}

void CctSession::DoSkipToScreen(XGUID TargetScreenId)
{
    FX_ASSERT(!IsNullXGuid(&TargetScreenId));

    CctAction_InsertScreen action(this);
    action.Setup(TargetScreenId);
    ExecuteAction(&action);

    // Save control states
    SaveControlStates();

    // Execute Leave event: we always fire this event before we close a screen
    _events.ExecuteType(seLeave, this);

    // Save the actions and the screen state
    SaveCurrentScreen();

    // Move to the next screen
    _sighting.IncPathIndex();

    // Load the screen: no state
    LoadCurrentScreen();

    // Startup the screen
    EnterScreen();

    // Repaint
    Repaint();
}

void CctSession::DoJumpToScreen(XGUID BaseScreenId, XGUID JumpScreenId)
{
    // Early out - no jump target
    if (IsNullXGuid(&JumpScreenId)) return;

    // If the screen is the same, the user probably means "Home"
    if (CompareXGuid(&BaseScreenId, &JumpScreenId))
    {
        DoHome(BaseScreenId);
        return;
    }

    // Execute Leave event: we always fire this event before we close a screen
    _events.ExecuteType(seLeave, this);

    while (_sighting.GetPathIndex() > 0)
    {
        if (CompareXGuid(_sighting.GetCurrentScreenId(), &BaseScreenId)) break;

        LoadCurrentScreenAndState();
        _actions.Rollback(this);
        _actions.Clear();
        _sighting.DecPathIndex();
    }

    // This is the new screen we are starting with
    LoadCurrentScreenAndState();
    _actions.Rollback(this);
    _actions.Clear();

    //
    // Now restart the sequence in our new state
    //
 
    // Note: we are losing the state and actions from the current screen
    LoadCurrentScreen();

    // Startup the screen
    EnterScreen();

    // Execute Set Jump Target
    _events.ExecuteType(seSetJumpTarget, this, (void *)&JumpScreenId);

    // Controls should be set up so that Next will go to the jump target.
    if (CanDoNext())
    {
        // Advance
        DoNext();
    }
    else
    {
        Fault(false, "Jump screen not a Link");
    }
}

void CctSession::DoCommit(XGUID TargetScreenId, bool DoRepaint)
{
    if (!CanDoNext()) return;

    // Shut down the screen
    LeaveScreen();

    // Save the actions and the screen state
    _saving = true;

    SaveCurrentScreen();

    // Save the last GPS position if exists and we're saving a new sighting
    if ((_sightingId == INVALID_DBID) &&
        (_sighting.GetGPS()->Quality != FXGPS_QUALITY::None))
    {
        _lastGPS = *_sighting.GetGPS();

        // Add a custom goto point for the current title
        if (_pendingGotoTitle)
        {
            FXGOTO newGoto = {MAGIC_GOTO};
            strncpy(newGoto.Title, _pendingGotoTitle, ARRAYSIZE(newGoto.Title)-1);
            newGoto.X = _lastGPS.Longitude;
            newGoto.Y = _lastGPS.Latitude;
            AddCustomGoto(&newGoto);

            MEM_FREE(_pendingGotoTitle);
            _pendingGotoTitle = nullptr;
        }
    }

    // Save the current sighting
    // CfxDatabase *database = GetSightingDatabase();
    // Fault(_sighting.Save(database, &_sightingId), "Failed to save");
    // FreeSightingDatabase(database);
    // database = nullptr;

    GetClassicView(this)->saveSighting(&_sighting);

    SD_WriteSighting(&_sighting);

    //
    _transferManager.CommitAlerts(&_sighting);

    if (_transferOnSave)
    {
        _transferManager.OnAlarm();
    }

    _sightingId = INVALID_DBID;
    _sighting.Reset();

    // Save the state
    SaveState();
    if (_faulted) goto Done;

    //
    // Reset the system state
    //
    // Calculate the target screen
    int targetIndex;

    if (IsNullXGuid(&TargetScreenId))
    {
        targetIndex = 0;
    }
    else
    {
        int i;
        targetIndex = -1;
        for (i = _sighting.GetPathIndex(); i >= 0; i--)
        {
            if (CompareXGuid(_sighting.GetPathItem(i), &TargetScreenId))
            {
                targetIndex = i;
                break;
            }
        }
    }

    // Check for inplace
    if (targetIndex == -1)  // Save target is outside the current path
    {
        CctAction_InsertScreen action(this);
        action.Setup(TargetScreenId);
        ExecuteAction(&action);
        SaveCurrentScreen();
        if (_faulted) goto Done;
        SaveState();
        if (_faulted) goto Done;

        _sighting.IncPathIndex();
        LoadCurrentScreen();
    }
    else                    // Save target is inside the current path
    {
        while ((int)_sighting.GetPathIndex() >= targetIndex)
        {
            _actions.Rollback(this);
            _actions.Clear();
            if ((int)_sighting.GetPathIndex() == targetIndex) break;
            _sighting.DecPathIndex();
            LoadCurrentScreenAndState();
        }

        // Note: we are losing the state and actions from the current screen
        LoadCurrentScreen();
    }

    _sighting.ClearGPS();
    _sighting.ClearOldState();
    
    //
    // Now restart the sequence in our new state
    //

    // Startup the screen
    EnterScreen();

Done:
    _saving = false;

    if (DoRepaint)
    {
        Repaint();
    }

    _transferManager.Update();
}

void CctSession::DoCommitInplace()
{
    if (!CanDoNext()) return;

    // Shut down the screen
    LeaveScreen();

    // Save the current sighting
    // CfxDatabase *database = GetSightingDatabase();
    // Fault(_sighting.Save(database, &_sightingId), "Failed to save");
    // FreeSightingDatabase(database);
    // database = nullptr;

    GetClassicView(this)->saveSighting(&_sighting);

    SD_WriteSighting(&_sighting);

    _transferManager.CommitAlerts(&_sighting);

    _sightingId = INVALID_DBID;
    _sighting.Reset();

    // Save the state: BUGBUG
    //SaveState();
    if (_faulted) goto Done;

    _actions.Rollback(this);
    _actions.Clear();

    //
    // Now restart the sequence in our new state
    //

    // Startup the screen
    EnterScreen();

Done:
    _saving = false;

    _transferManager.Update();
}

void CctSession::DoStartEdit(const QString &sightingUid)
{
    FX_ASSERT(_connected);
    FX_ASSERT(!sightingUid.isEmpty());
    FX_ASSERT(sightingUid != GetCurrentSightingUid());

    ClearDialogs();

    SaveCurrentScreen();

    _backupSighting.Assign(&_sighting);

    _sightingId = 1; // Mark as editing

    CreateSightingDatabasePtr()->Load(&_sighting, sightingUid);

    SaveState();
    if (_faulted) return;

    // Load the current screen
    LoadCurrentScreenAndState();

    // Reverse the actions on the new screen
    _actions.Rollback(this);
    _actions.Clear();

    // Execute Enter event
    _events.ExecuteType(seEnter, this);

    // ProcessGPSFilter
    ProcessElementEffects();

    Repaint();
}

void CctSession::DoAcceptEdit()
{
    if (!CanDoNext()) return;

    // Shut down the screen
    LeaveScreen();

    // Save the actions and the screen state
    SaveCurrentScreen();

    // Save the sighting
    // CfxDatabase *database = GetSightingDatabase();
    // Fault(_sighting.Save(database, &_sightingId), "Failed to save");
    // FreeSightingDatabase(database);
    // database = nullptr;

    _sightingId = INVALID_DBID;

    GetClassicView(this)->saveSighting(&_sighting);

    SD_WriteSighting(&_sighting);

    // Assign the backup sighting
    _sighting.Assign(&_backupSighting);
    _backupSighting.Clear();

    // Save the state
    SaveState();
    if (_faulted) goto Done;

    // This is the new screen we are starting with
    LoadCurrentScreenAndState();
    _actions.Rollback(this);
    _actions.Clear();

    // Note we are losing the state and actions from the current screen
    //LoadCurrentScreen();

    // Execute Enter event
    _events.ExecuteType(seEnter, this);

    // ProcessGPSFilter
    ProcessElementEffects();

Done:

    Repaint();
}

void CctSession::DoCancelEdit()
{
    // Shut down the screen
    _events.ExecuteType(seLeave, this);

    // Save the actions and the screen state
    SaveCurrentScreen();

    // Assign the backup sighting
    _sightingId = INVALID_DBID;
    _sighting.Assign(&_backupSighting);
    _backupSighting.Clear();

    // Save the state
    SaveState();
    if (_faulted) goto Done;

    // This is the new screen we are starting with
    LoadCurrentScreenAndState();
    _actions.Rollback(this);
    _actions.Clear();

    // Note we are losing the state and actions from the current screen
    //LoadCurrentScreen();

    // Execute Enter event
    _events.ExecuteType(seEnter, this);

    // ProcessGPSFilter
    ProcessElementEffects();

Done:

    Repaint();
}

void CctSession::DoHome(XGUID TargetScreenId)
{
    // Shut down the screen: run the next events so the current screen gets to add any state it wants
    _events.ExecuteType(seLeave, this);

    _sightingId = INVALID_DBID;
    _sighting.Reset();

    while (_sighting.GetPathIndex() > 0)
    {
        if (CompareXGuid(_sighting.GetCurrentScreenId(), &TargetScreenId)) break;

        LoadCurrentScreenAndState();
        _actions.Rollback(this);
        _actions.Clear();

        _sighting.DecPathIndex();
    }

    // This is the new screen we are starting with
    LoadCurrentScreenAndState();
    _actions.Rollback(this);
    _actions.Clear();

    //
    // Now restart the sequence in our new state
    //
 
    // Note: we are losing the state and actions from the current screen
    LoadCurrentScreen();

    // Startup the screen
    EnterScreen();

    // Repaint
    Repaint();
}

void CctSession::ClearDialogs()
{
    int i;
    for (i = 0; i < _dialogStates.GetCount(); i++)
    {
        MEM_FREE(_dialogStates.GetPtr(i)->StateData);
    }
    _dialogStates.Clear();
}

void CctSession::ShowDialog(const GUID *pDialogId, CfxControl *pSender, void *pParam, int ParamSize)
{
    void *dialogParam = nullptr;
    if (pParam && ParamSize)
    {
        dialogParam = MEM_ALLOC(ParamSize);
        memcpy(dialogParam, pParam, ParamSize);
    }

    // Save the current state
    if (_dialogStates.GetCount() == 0)
    {
        // Execute Leave event: we always fire this event before we close a screen
        _events.ExecuteType(seLeave, this);

        // Save the actions and the screen state
        SaveCurrentScreen();

        // Save the state
        SaveState();
    }
    else
    {
        DIALOGSTATE *dialogState;
        dialogState = _dialogStates.GetPtr(0);
        FX_ASSERT(dialogState->StateData == nullptr);

        CfxStream stream;
        CfxWriter writer(&stream);

        GetWindow()->DefineState(writer);
        dialogState->StateData = stream.CloneMemory();
    }

    // Create a new dialog and add it to the stack.
    {
        DIALOGSTATE dialogState;
        dialogState.Id = *pDialogId;
        dialogState.StateData = nullptr;
        _dialogStates.Insert(0, dialogState);
    }

    if (!_faulted)
    {
        FXDIALOG *dialog = (FXDIALOG *)_resource->GetDialog(this, pDialogId);
        Fault((dialog != nullptr), "Attempt to load invalid Dialog");
        if (dialog)
        {
            CfxStream stream(&dialog->Data[0]);
            CfxReader reader(&stream);
           
            CfxScreen *screen = GetWindow();
            screen->ClearControls();
            Repaint();

            CfxEngine *engine = GetEngine(this);
            engine->BlockPaint();
            
            screen->DefineProperties(reader);
            CfxDialog *editor = (CfxDialog *)screen->FindControlByType(pDialogId);
            if (editor)
            {
                editor->Init(pSender == nullptr ? this : pSender, dialogParam, ParamSize);
                editor->SetBounds(0, 0, GetClientWidth(), GetClientHeight());
                engine->AlignControls(screen);
                editor->PostInit(this);
            }
            _resource->ReleaseDialog(dialog);

            engine->UnblockPaint();
        }
    }

    MEM_FREE(dialogParam);

    Repaint();
}

CfxScreen *CctSession::GetWindow()
{
    //FX_ASSERT(CompareGuid(&GUID_0, _controls.Get(1)->GetTypeId()));
    return _screen;//(CfxScreen *)_controls.Get(1);
}

CfxControl_Button *CctSession::GetMessageWindow()
{
    //FX_ASSERT(CompareGuid(&GUID_CONTROL_BUTTON, _controls.Get(0)->GetTypeId()));
    return _messageWindow;//(CfxControl_Button *)_controls.Get(0);
}

int CctSession::InsertScreen(XGUID ScreenId)
{
    return _sighting.InsertScreen(ScreenId);
}

int CctSession::AppendScreen(XGUID ScreenId)
{
    if (_sighting.GetPathCount() == 0)
    {
        _firstScreenId = ScreenId;
    }

    return _sighting.AppendScreen(ScreenId);
}

void CctSession::DeleteScreen(int Index)
{
    if ((Index >= _sighting.GetPathCount()))
    {
        // BUGBUG: How exactly does this happen?
        return;
    }
    
    Fault(Index < _sighting.GetPathCount(), "Attempt to Delete an invalid screen");
    if (_faulted) return;

    _sighting.DeleteScreen(Index);
}

XGUID *CctSession::GetCurrentScreenId()
{
    return _sighting.GetCurrentScreenId();
}

QString CctSession::GetCurrentSightingUid()
{
    if (_sightingId == INVALID_DBID)
    {
        return QString();
    }

    return GUIDToString(_sighting.GetUniqueId());
}

int CctSession::GetSightingCount()
{
    return CreateSightingDatabasePtr()->GetCount();
}

int CctSession::MergeAttributes(int ControlId, ATTRIBUTES *pNewAttributes, ATTRIBUTES *pOldAttributes)
{
    // Pick up the full element guid
    CfxResource *resource = GetResource();
    for (int i=0; i<pNewAttributes->GetCount(); i++)
    {
        ATTRIBUTE *attribute = pNewAttributes->GetPtr(i);

            FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, attribute->ElementId, (BYTE)ElementAttribute::Name);
            if (element)
            {
                attribute->ElementGuid = element->Guid;
                resource->ReleaseObject(element);
            }
    }

    // The sighting will now perform the merge
    return _sighting.MergeAttributes(ControlId, pNewAttributes, pOldAttributes);
}

void CctSession::ExecuteAction(CfxAction *pAction)
{
    pAction->SetSourceId(_sighting.GetCurrentScreenId());
    _actions.AddCloneAndExecute(pAction, this);
}

bool CctSession::EnumHistoryInit(HISTORY_ITEM **ppHistoryItemDef)
{
    FX_ASSERT(_name && (strlen(_name) > 0));
    FX_ASSERT(_historyItemDef == nullptr);
    FX_ASSERT(_historyItem == nullptr);

    CfxHost *host = GetHost(this);

    if (ppHistoryItemDef != nullptr)
    {
        *ppHistoryItemDef = nullptr;
    }
    _historyItemDef = _historyItemDef = nullptr;
    _historyItemCount = 0;
    _historyItemIndex = 0;
    _historyItemSize = 0;
    
    char name[MAX_PATH];
    strcpy(name, _name);
    strcat(name, ".HST");

    // Read the header item and get the count and size.
    HISTORY_ITEM historyItemFirst = {0};
    int result = 0;

    if (host->EnumRecordInit(name))
    {
        if (host->EnumRecordNext(&historyItemFirst, sizeof(HISTORY_ITEM)))
        {
            _historyItemCount = historyItemFirst.RecordCount;
            _historyItemIndex = 0;
            _historyItemSize = sizeof(HISTORY_ITEM) + historyItemFirst.ColumnCount * 32;
        }

        host->EnumRecordDone();
    }

    if (_historyItemCount > 0)
    {
        _historyItemDef = (HISTORY_ITEM *)MEM_ALLOC(_historyItemSize); 
        if (_historyItemDef == nullptr) return false;
        _historyItem = (HISTORY_ITEM *)MEM_ALLOC(_historyItemSize);
        if (_historyItem == nullptr) return false;

        if (host->EnumRecordInit(name))
        {
            if (host->EnumRecordNext(_historyItemDef, _historyItemSize))
            {
                _historyItemIndex = 1;
                if (ppHistoryItemDef != nullptr)
                {
                    *ppHistoryItemDef = _historyItemDef;
                }
                return true;
            }

            host->EnumRecordDone();
        }
    }

    return false;
}

bool CctSession::EnumHistorySetIndex(int Index)
{
    if (Index >= _historyItemCount) return false;
    _historyItemIndex = Index + 1;
    return GetHost(this)->EnumRecordSetIndex(_historyItemIndex, _historyItemSize);
}

bool CctSession::EnumHistoryNext(HISTORY_ITEM **ppHistoryItem)
{
    *ppHistoryItem = _historyItem;

    if ((_historyItemCount == 0) || (_historyItemSize == 0)) return false;
    if (_historyItem == nullptr) return false;
    if (_historyItemIndex > _historyItemCount) return false;

    bool result = GetHost(this)->EnumRecordNext(_historyItem, _historyItemSize);
    
    if (result) 
    {
        _historyItemIndex++;
    }

    return result;
}

void CctSession::EnumHistoryDone()
{
    GetHost(this)->EnumRecordDone();

    MEM_FREE(_historyItem);
    _historyItem = nullptr;

    MEM_FREE(_historyItemDef);
    _historyItemDef = nullptr;
    
    _historyItemCount = 0;
    _historyItemIndex = 0;
    _historyItemSize = 0;
}

void CctSession::EnumHistoryWaypoints(CfxControl *pControl, EnumHistoryWaypointCallback Callback)
{
    CfxHost *host = GetHost(this);
    HISTORY_ITEM *historyItemDef;
    HISTORY_WAYPOINT historyWaypoint;
    int historyItemSize;

    if (!EnumHistoryInit(&historyItemDef))
    {
        return;
    }

    historyItemSize = sizeof(HISTORY_ITEM) + historyItemDef->ColumnCount * 32;
    if (!host->EnumRecordSetIndex(historyItemDef->RecordCount + 1, historyItemSize))
    {
        EnumHistoryDone();
        return;
    }

    while (host->EnumRecordNext(&historyWaypoint, sizeof(historyWaypoint)))
    {
        (pControl->*Callback)(&historyWaypoint);
    }

    EnumHistoryDone();
}

HISTORY_ROW *CctSession::GetHistoryItemRow(HISTORY_ITEM *pHistoryItem, int Index)
{
    FX_ASSERT(Index < pHistoryItem->ColumnCount);
    HISTORY_ROW *row = (HISTORY_ROW *)(pHistoryItem + 1);
    return row + Index;
}

//*************************************************************************************************
// Store functions

void CctSession::ClearStore()
{
    for (int i=0; i<_store.GetCount(); i++)
    {
        MEM_FREE(_store.GetPtr(i)->Data);
    }
    _store.Clear();
}

STOREITEM *CctSession::GetStoreItem(GUID PrimaryId, int ControlId)
{
    for (int i=0; i<_store.GetCount(); i++)
    {
        STOREITEM *item = _store.GetPtr(i);
        if (CompareGuid(&PrimaryId, &item->PrimaryId) && (ControlId == item->ControlId))
        {
            return item;
        }
    }
    
    return nullptr;
}

STOREITEM *CctSession::SetStoreItem(GUID PrimaryId, int ControlId, void *pData)
{
    for (int i=0; i<_store.GetCount(); i++)
    {
        STOREITEM *item = _store.GetPtr(i);
        if (CompareGuid(&PrimaryId, &item->PrimaryId) && (ControlId == item->ControlId))
        {
            MEM_FREE(item->Data);
            item->Data = nullptr;
            _store.Delete(i);
            break;
        }
    }
    
    if (pData != nullptr)
    {
        STOREITEM item;
        item.PrimaryId = PrimaryId;
        item.ControlId = ControlId;
        item.Data = pData;
        return _store.GetPtr(_store.Add(item));
    }
    else
    {
        return nullptr;
    }
}

void CctSession::SaveControlState(CfxControl *pControl, bool Flush)
{
    CfxStream stream;
    CfxWriter writer(&stream);
    pControl->DefineState(writer);
    SetStoreItem(ConstructGuid(*GetCurrentScreenId()), pControl->GetId(), stream.CloneMemory());

    if (Flush)
    {
        SaveState();
    }
}

void CctSession::LoadControlState(CfxControl *pControl)
{
    STOREITEM *item = GetStoreItem(ConstructGuid(*GetCurrentScreenId()), pControl->GetId());
    if (item)
    {
        CfxStream stream(item->Data);
        CfxReader reader(&stream);
        pControl->DefineState(reader);
    }
}

void CctSession::LoadControlStates()
{
    if (IsEditing()) return;

    for (int i = 0; i < _retainStates.GetCount(); i++)
    {
        RETAINSTATE *retainState = _retainStates.GetPtr(i);
            
        if (retainState->RetainState->MustRetain())
        {
            LoadControlState(retainState->Control);

            if (retainState->RetainState->MustResetState())
            {
                retainState->Control->ResetState();
            }
        }
    }
}

void CctSession::SaveControlStates()
{
    if (IsEditing()) return;

    for (int i = 0; i < _retainStates.GetCount(); i++)
    {
        RETAINSTATE *retainState = _retainStates.GetPtr(i);
            
        if (retainState->RetainState->MustRetain())
        {
            SaveControlState(retainState->Control, false);
        }
    }
}

//*************************************************************************************************
// Global value functions

void CctSession::ClearGlobalValues()
{
    _globalValues.Clear();
}

GLOBALVALUE *CctSession::EnumGlobalValues(int Index)
{
    if (Index >= _globalValues.GetCount())
    {
        return nullptr;
    }
    else
    {
        return _globalValues.GetPtr(Index);
    }
}

GLOBALVALUE *CctSession::GetGlobalValue(const char *pName)
{
    int i = 0;
    GLOBALVALUE *globalValue = EnumGlobalValues(0);
    while (globalValue)
    {
        if (stricmp(pName, globalValue->Name) == 0)
        {
            return globalValue;
        }

        i++;
        globalValue = EnumGlobalValues(i);
    }

    return nullptr;
}

void CctSession::SetGlobalValue(CfxPersistent *pSender, const char *pName, double Value)
{
    if ((pName == nullptr) || (*pName == '\0')) return;

    double oldValue = 0;

    GLOBALVALUE *globalValue = GetGlobalValue(pName);
    if (globalValue)
    {
        oldValue = globalValue->Value;
        globalValue->Value = Value;
    }
    else
    {
        GLOBALVALUE newValue;
        strncpy(newValue.Name, pName, sizeof(newValue.Name)-1);
        newValue.Value = Value;
        _globalValues.Add(newValue);
    }

    //if (pSender)
    {
        GLOBAL_CHANGED_PARAMS eventParams = {0};
        eventParams.pSender = (pSender == nullptr) ? this : pSender;
        eventParams.Name = pName;
        eventParams.OldValue = oldValue;
        eventParams.NewValue = Value;

        CfxEngine *engine = GetEngine(this);
        engine->BlockPaint();
        _events.ExecuteType(seGlobalChanged, this, &eventParams);
        engine->UnblockPaint();
    }
}

char *CctSession::GetUpdateTag()
{
    char *result = nullptr;
    char *workPath = AllocMaxPath(GetApplicationPath());
    AppendSlash(workPath);
    strcat(workPath, _name);
    strcat(workPath, ".TAG");

    FXFILEMAP fileMap = {0};
    if (FxMapFile(workPath, &fileMap))
    {
        result = AllocString((char *)fileMap.BasePointer);
        FxUnmapFile(&fileMap);
    }

    MEM_FREE(workPath);
    workPath = nullptr;
       
    return result;
}

char *CctSession::GetUpdateUrl()
{
    char *result = nullptr;
    char *workPath = AllocMaxPath(GetApplicationPath());
    AppendSlash(workPath);
    strcat(workPath, _name);
    strcat(workPath, ".DEF");

    FXFILEMAP fileMap = {0};
    if (FxMapFile(workPath, &fileMap))
    {
        result = AllocString((char *)fileMap.BasePointer);
        FxUnmapFile(&fileMap);
    }
    else
    {
        result = AllocString(GetResourceHeader()->WebUpdateUrl);
    }

    MEM_FREE(workPath);
    workPath = nullptr;
       
    return result;
}

//*************************************************************************************************
// Utility functions

bool RegisterSessionEvent(SESSIONEVENT Event, CfxControl *pControl, NotifyMethod Method)
{
    CctSession *session = GetSession(pControl);
    if (session)
    {
        session->RegisterEvent(Event, pControl, Method);
        return true;
    }
    else
    {
        return false;
    }
}

bool UnregisterSessionEvent(SESSIONEVENT Event, CfxControl *pControl)
{
    CctSession *session = GetSession(pControl);
    if (session)
    {
        session->UnregisterEvent(Event, pControl);
        return true;
    }
    else
    {
        return false;
    }
}

bool RegisterRetainState(CfxControl *pControl, CctRetainState *pRetainState)
{
    CctSession *session = GetSession(pControl);
    if (session)
    {
        session->RegisterRetainState(pControl, pRetainState);
        return true;
    }
    else
    {
        return false;
    }
}

bool UnregisterRetainState(CfxControl *pControl)
{
    CctSession *session = GetSession(pControl);
    if (session)
    {
        session->UnregisterRetainState(pControl);
        return true;
    }
    else
    {
        return false;
    }
}

void CctSession::ShowExports()
{
    emit GetClassicView(this)->showExports();
}

void CctSession::ShowUploadQueue()
{
    emit GetClassicView(this)->showUploadQueue();
}

void CctSession::ShowMapPage()
{
    emit GetClassicView(this)->showMapPage();
}

void CctSession::ShowSightingEditorPage()
{
    auto protocol = GetResourceHeader()->SendData.Protocol;

    emit GetClassicView(this)->showSightingEditorPage(protocol == FXSENDDATA_PROTOCOL_UNKNOWN);
}

void CctSession::ShowTextEditorPage(int ControlId)
{
    emit GetClassicView(this)->showTextEditorPage(ControlId);
}

void CctSession::ShowGPSReaderPage(XGUID SaveTarget, int SkipTimeout, bool SkipAllowed)
{
    emit GetClassicView(this)->showGPSReaderPage(SaveTarget, SkipTimeout, SkipAllowed);
}

void CctSession::ShowConfirm(int ControlId, const char *pMessage)
{
    emit GetClassicView(this)->showConfirm(ControlId, QString::fromUtf8(pMessage ? pMessage : ""));
}

void CctSession::ShareData()
{
    emit GetClassicView(this)->shareData();
}

void CctSession::SubmitData()
{
    emit GetClassicView(this)->submitDataClicked();
}

CctSession *GetSession(CfxControl *pControl)
{
    CfxControl *control = pControl;

    while (control && !CompareGuid(control->GetTypeId(), &GUID_CONTROL_SESSION))
    {
        control = control->GetParent();
    }

    return (CctSession *)control;
}

bool IsSessionEditing(CfxControl *pControl)
{
    CctSession *session = GetSession(pControl);
    return session && session->IsEditing();
}

