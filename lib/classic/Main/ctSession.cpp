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

#include "ctMain.h"
#include "fxDialog.h"
#include "fxUtils.h"
#include "fxMath.h"

#include "ctElement.h"
#include "ctSession.h"
#include "ctActions.h"

#include "ctDialog_SightingEditor.h"
#include "ctDialog_GPSReader.h"
#include "ctDialog_GPSViewer.h"
#include "ctDialog_Confirm.h"
#include "ctDialog_Password.h"

#include "Cab.h"

#define CTX_QUOTA_SIZE (16 * 1024 * 1024) // 16MB

//*************************************************************************************************
// CctJsonBuilder

CctJsonBuilder::CctJsonBuilder(CctSession *pSession, CfxFileStream *pFileStream, UINT Protocol)
{
    _session = pSession;
    _fileStream = pFileStream;
    _protocol = Protocol;
}

BOOL CctJsonBuilder::WriteHeader()
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

    return FALSE;
}

VOID CctJsonBuilder::ResolveElement(CctSession *pSession, CfxResource *pResource, XGUID ElementId, CfxString &Name)
{
    FXJSONID *jsonId = (FXJSONID *)pResource->GetObject(pSession, ElementId, eaJsonId);
    if (jsonId != NULL)
    {
        Name.Set(jsonId->Text);
        pResource->ReleaseObject(jsonId);
        return;
    }

    FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)pResource->GetObject(pSession, ElementId, eaName);
    if (element != NULL)
    {
        CTRESOURCEHEADER *header = (CTRESOURCEHEADER *)pResource->GetHeader(pSession);
        BOOL decode = (strncmp(header->ServerFileName, "KEY:", 4) == 0) ||      // SMART
                      (header->SendData.Protocol == FXSENDDATA_PROTOCOL_ESRI);  // ESRI
        if (decode)
        {
            Name.Set(element->Text[0] == '#' ? element->Text + 1 : element->Text);
        }
        else
        {
            CHAR s[39] = {0};
            GUID *g = &element->Guid;
            sprintf(s, 
                    "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                    g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
                    g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

            Name.Set(s);
        }

        pResource->ReleaseObject(element);
        return;
    }

    Name.Set("Unknown");
}

BOOL CctJsonBuilder::WriteMediaAttribute(ATTRIBUTE *pAttribute)
{
    BOOL result = FALSE;
    CHAR *mediaName = pAttribute->GetMediaName();
    CHAR *fileName = NULL;
    FXFILEMAP fileMap = {0};

    fileName = _session->AllocMediaFileName(mediaName);
    if (fileName == NULL)
    {
        goto cleanup;
    }

    if (!FxMapFile(fileName, &fileMap))
    {
        Log("Failed to open source file %s", fileName);
        goto cleanup;
    }

    if (fileMap.FileSize == 0)
    {
        Log("Empty file: %s", fileName);
        goto cleanup;
    }

    //
    // Must succeed now, since we're writing to the stream.
    //

    if (!Base64Encode((char *)fileMap.BasePointer, fileMap.FileSize, *_fileStream))
    {
        Log("Failed to base64 encode file to stream: %s", fileName);
        goto cleanup;
    }

    result = TRUE;

cleanup:

    FxUnmapFile(&fileMap);

    MEM_FREE(fileName);

    return result;
}

BOOL CctJsonBuilder::WriteSighting(CctSighting *pSighting, FXALERT *pAlert, BOOL First, FXFILELIST *pMediaFileList)
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
        return FALSE;
    }
}

BOOL CctJsonBuilder::WriteSightingGeoJson(CctSighting *pSighting, FXALERT *pAlert, BOOL First, FXFILELIST *pMediaFileList)
{
    FXGPS_POSITION *gps = pSighting ? pSighting->GetGPS() : GetEngine(_session)->GetGPSLastFix();

    CHAR *dateTimeString = NULL;
    CHAR *temp = (CHAR *)MEM_ALLOC(4096);
    BOOL result = TRUE;
    if (!First)
    {
        result = _fileStream->Write(",\r\n");
    }

    // Write header.

    // Type
    result = result && _fileStream->Write("{ \"type\": \"Feature\"");

    // Geometry
    sprintf(temp, ",\r\n  \"geometry\": { \"type\": \"Point\", \"coordinates\": [%f, %f] }", gps->Longitude, gps->Latitude);
    result = result && _fileStream->Write(temp);

    // Properties
    result = result && _fileStream->Write(",\r\n  \"properties\": {");

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
    dateTimeString = GetHost(_session)->GetDateStringUTC(dateTime);
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

    // DatabaseName + application name.
    CfxString value;
    sprintf(temp, "3.%ld", (UINT)RESOURCE_VERSION);
    value.Set(temp);
    FixJsonString(value);
    sprintf(temp, ",\r\n    \"ctVersion\": \"%s\"", value.Get());
    result = result && _fileStream->Write(temp);

    value.Set(_session->GetResourceHeader()->ServerFileName);
    FixJsonString(value);
    sprintf(temp, ",\r\n    \"dbName\": \"%s\"", value.Get());
    result = result && _fileStream->Write(temp);

    value.Set(_session->GetResourceHeader()->Name);
    FixJsonString(value);
    sprintf(temp, ",\r\n    \"appName\": \"%s\"", value.Get());
    result = result && _fileStream->Write(temp);

    // SightingId
    if (pSighting)
    {
        GUID uniqueId = *pSighting->GetUniqueId();
        sprintf(temp, 
                ",\r\n    \"id\": \"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\"",
                uniqueId.Data1, uniqueId.Data2, uniqueId.Data3, uniqueId.Data4[0], uniqueId.Data4[1], uniqueId.Data4[2], uniqueId.Data4[3],
                uniqueId.Data4[4], uniqueId.Data4[5], uniqueId.Data4[6], uniqueId.Data4[7]);
        result = result && _fileStream->Write(temp);
    }

    // GPS
    if (gps->Quality > fqNone)
    {
        sprintf(temp, ",\r\n    \"latitude\": %f,\r\n    \"longitude\": %f", gps->Latitude, gps->Longitude);
        result = result && _fileStream->Write(temp);

        if (gps->Quality >= fq3D)
        {
            sprintf(temp, ",\r\n    \"altitude\": %f", gps->Altitude);
            result = result && _fileStream->Write(temp);
        }

        if ((gps->Accuracy > 0.0) && (gps->Accuracy < 50.0))
        {
            sprintf(temp, ",\r\n    \"accuracy\": %f", gps->Accuracy);
            result = result && _fileStream->Write(temp);
        }
    }

    // Alert
    if (pAlert)
    {
        sprintf(temp, ",\r\n    \"caUuid\": \"%s\"", pAlert->CaId);
        result = result && _fileStream->Write(temp);
        sprintf(temp, ",\r\n    \"level\": %ld", pAlert->Level);
        result = result && _fileStream->Write(temp);
        sprintf(temp, ",\r\n    \"description\": \"%s\"", pAlert->Description);
        result = result && _fileStream->Write(temp);
        sprintf(temp, ",\r\n    \"typeUuid\": \"%s\"", pAlert->Type);
        result = result && _fileStream->Write(temp);

        if (pAlert->PatrolId[0])
        {
            CfxString value;
            value.Set(pAlert->PatrolId);
            FixJsonString(value);
            sprintf(temp, ",\r\n    \"patrolId\": \"%s\"", value.Get());
            result = result && _fileStream->Write(temp);
        }
    }

    // Write all other attributes
    if (pSighting)
    {
        result = result && _fileStream->Write(",\r\n    \"sighting\": {");

        CfxResource *resource = _session->GetResource();
        CfxString name, value;

        for (UINT i=0; i<pSighting->GetAttributes()->GetCount(); i++) 
        {
            ATTRIBUTE *attribute = pSighting->GetAttributes()->GetPtr(i);
            
            ResolveElement(_session, resource, attribute->ElementId, name);

            if (!IsNullXGuid(&attribute->ValueId))
            {
                ResolveElement(_session, resource, attribute->ValueId, value);
            }
            else
            {
                Type_VariantToText(attribute->Value, value);
            }

            BOOL quotify = FALSE;

            if (attribute->Value->Type == dtNull)
            {
                value.Set("null");
            }
            else
            {
                if (value.Get() == NULL)
                {
                    value.Set("");
                }

                quotify = (attribute->Value->Type >= dtDate);
            }

            FixJsonString(name);
            if (i > 0)
            {
                result = result && _fileStream->Write(",");
            }
            sprintf(temp, "\r\n      \"%s\": ", name.Get());
            result = result && _fileStream->Write(temp);
            if (quotify)
            {
                result = result && _fileStream->Write("\"");
            }
            FixJsonString(value);

            if (pMediaFileList && attribute->IsMedia() && WriteMediaAttribute(attribute))
            {
                FXFILE file = {0};
                file.Name = AllocString(attribute->GetMediaName());
                pMediaFileList->Add(file);
            }
            else
            {
                result = result && _fileStream->Write(value.Get());
            }

            if (quotify)
            {
                result = result && _fileStream->Write("\"");
            }
        }

        result = result && _fileStream->Write("\r\n    }");
    }

    // Success
    result = result && _fileStream->Write("\r\n  }\r\n}");

    MEM_FREE(temp);
    MEM_FREE(dateTimeString);
    return result;
}

VOID CctJsonBuilder::WriteSightingToStream(CctSession *pSession, CctSighting *pSighting, CfxStream &Data)
{
    FXGPS_POSITION *gps = pSighting ? pSighting->GetGPS() : GetEngine(pSession)->GetGPSLastFix();

    CHAR *dateTimeString = NULL;
    CHAR *temp = (CHAR *)MEM_ALLOC(4096);

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
    CfxString value;
    sprintf(temp, "3.%ld", (UINT)RESOURCE_VERSION);
    value.Set(temp);
    FixJsonString(value);
    sprintf(temp, ",\r\n    \"ctVersion\": \"%s\"", value.Get());
    Data.Write(temp);

    value.Set(pSession->GetResourceHeader()->ServerFileName);
    FixJsonString(value);
    sprintf(temp, ",\r\n    \"dbName\": \"%s\"", value.Get());
    Data.Write(temp);

    value.Set(pSession->GetResourceHeader()->Name);
    FixJsonString(value);
    sprintf(temp, ",\r\n    \"appName\": \"%s\"", value.Get());
    Data.Write(temp);

    // SightingId
    GUID uniqueId = *pSighting->GetUniqueId();
    sprintf(temp, 
            ",\r\n    \"id\": \"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\"",
            uniqueId.Data1, uniqueId.Data2, uniqueId.Data3, uniqueId.Data4[0], uniqueId.Data4[1], uniqueId.Data4[2], uniqueId.Data4[3],
            uniqueId.Data4[4], uniqueId.Data4[5], uniqueId.Data4[6], uniqueId.Data4[7]);
    Data.Write(temp);

    // GPS
    if (gps->Quality > fqNone)
    {
        sprintf(temp, ",\r\n    \"latitude\": %f,\r\n    \"longitude\": %f", gps->Latitude, gps->Longitude);
        Data.Write(temp);

        if (gps->Quality >= fq3D)
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

    for (UINT i=0; i<pSighting->GetAttributes()->GetCount(); i++) 
    {
        CfxString name, value;

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

        BOOL quotify = FALSE;

        if (attribute->Value->Type == dtNull)
        {
            value.Set("null");
        }
        else
        {
            if (value.Get() == NULL)
            {
                value.Set("");
            }

            quotify = (attribute->Value->Type >= dtDate);
        }

        FixJsonString(name);
        if (i > 0)
        {
            Data.Write(",");
        }
        sprintf(temp, "\r\n      \"%s\": ", name.Get());
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
            Data.Write(value.Get());
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
    CHAR n = '\0';
    Data.Write1(&n);

    MEM_FREE(temp);
    MEM_FREE(dateTimeString);
}

BOOL CctJsonBuilder::WriteWaypoint(WAYPOINT *pWaypoint, BOOL First)
{
    CHAR temp[256];
    BOOL result = TRUE;
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
        result = FALSE;
        break;
    }

    if (!result) return FALSE;

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
    
    CHAR *dateTimeString = GetHost(_session)->GetDateStringUTC(&dateCurrent, (_protocol == FXSENDDATA_PROTOCOL_ESRI));
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

BOOL CctJsonBuilder::WriteSightingEsri(CctSighting *pSighting, BOOL First, FXFILELIST *pMediaFileList)
{
    FXGPS_POSITION *gps = pSighting ? pSighting->GetGPS() : GetEngine(_session)->GetGPSLastFix();
    CHAR *dateTimeString = NULL;

    CHAR *temp = (CHAR *)MEM_ALLOC(4096);
    BOOL result = TRUE;
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
    dateTimeString = GetHost(_session)->GetDateStringUTC(dateTime, TRUE);
    
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
    CfxString name, value;

    for (UINT i=0; i<pSighting->GetAttributes()->GetCount(); i++) 
    {
        ATTRIBUTE *attribute = pSighting->GetAttributes()->GetPtr(i);
        
        ResolveElement(_session, resource, attribute->ElementId, name);

        if (!IsNullXGuid(&attribute->ValueId))
        {
            ResolveElement(_session, resource, attribute->ValueId, value);
        }
        else
        {
            Type_VariantToText(attribute->Value, value);
        }

        BOOL quotify = FALSE;

        if (!Type_IsText((FXDATATYPE)(attribute->Value->Type)))
        {
            if (value.Compare(""))
            {
                value.Set("1");
            }
        }

        quotify = (attribute->Value->Type >= dtDate);

        FixJsonString(name);
        sprintf(temp, ",\r\n    \"%s\": ", name.Get());
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

            value.Set("1");
        }

        result = result && _fileStream->Write(value.Get());

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

BOOL CctJsonBuilder::WriteFooter()
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

    return FALSE;
}

//*************************************************************************************************
// CctAlert

CctAlert::CctAlert(CfxPersistent *pOwner): CfxPersistent(pOwner), _matchElements()
{
    _active = FALSE;
    _url = _userName = _password = _caId = _description = _type = NULL;
    _level = 0;
    _pingFrequency = 0;
    _pingOnly = FALSE;
    _protocol = FXSENDDATA_PROTOCOL_HTTPS;
    InitXGuid(&_patrolElementId);
    _committed = FALSE;
    _patrolValue = NULL;
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
    _url = _userName = _password = _caId = _description = _type = _patrolValue = NULL;

    if (GetEngine(this))
    {
        GetEngine(this)->KillAlarm(this);
    }
}

BOOL CompareStrings(CHAR *p1, CHAR *p2, BOOL CaseSensitive = FALSE)
{
    if (p1 == p2)
    {
        return TRUE;
    }

    if (!p1 && !p2)
    {
        return TRUE;
    }

    if ((p1 && !p2) || (!p1 && p2))
    {
        return FALSE;
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

BOOL CctAlert::Compare(CctAlert *pAlert)
{
    BOOL result;

    if (_active != pAlert->_active) return FALSE;

    result = CompareStrings(_url, pAlert->_url);
    if (!result) return FALSE;

    result = CompareStrings(_userName, pAlert->_userName);
    if (!result) return FALSE;

    result = CompareStrings(_password, pAlert->_password);
    if (!result) return FALSE;

    result = CompareStrings(_caId, pAlert->_caId);
    if (!result) return FALSE;

    result = CompareStrings(_description, pAlert->_description);
    if (!result) return FALSE;

    result = CompareStrings(_type, pAlert->_type);
    if (!result) return FALSE;

    if (_level != pAlert->_level) return FALSE;
    if (_pingFrequency != pAlert->_pingFrequency) return FALSE;

    result = CompareXGuid(&_patrolElementId, &pAlert->_patrolElementId);
    if (!result) return FALSE;

    result = _matchElements.GetCount() == pAlert->_matchElements.GetCount();
    if (!result) return FALSE;
   
    result = memcmp(_matchElements.GetMemory(), pAlert->_matchElements.GetMemory(), sizeof(XGUID) * _matchElements.GetCount()) == 0;
    if (!result) return FALSE;

    return TRUE;
}

BOOL CctAlert::Commit(CctSession *pSession, CctSighting *pSighting)
{
    if (!_active || !Match(pSighting)) return FALSE;

    // Get the patrol id value from the sighting if available.
    MEM_FREE(_patrolValue);
    _patrolValue = NULL;
    ATTRIBUTE *attribute = NULL;
    for (UINT i = 0; pSighting->EnumAttributes(i, &attribute); i++)
    {
        CfxString valueText;
        if (!CompareXGuid(&attribute->ElementId, &_patrolElementId)) continue;

        Type_VariantToText(attribute->Value, valueText);
        if (valueText.Length() > 0)
        {
            _patrolValue = AllocString(valueText.Get());
            break;
        }
    }

    if (!Build(pSighting, &_alert))
    {
        Log("Failed to build alert");
        return FALSE;
    }

    // Activate this ping/alert.
    _committed = _pingOnly ? pSession->GetTransferManager()->CreatePing(&_alert) :
                             pSession->GetTransferManager()->CreateAlert(pSighting, &_alert);

    return _committed;
}

BOOL CctAlert::Build(CctSighting *pSighting, FXALERT *pAlert)
{
    memset(pAlert, 0, sizeof(FXALERT));
    if (!_active) return FALSE;
    if (!_url) return FALSE;

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
    CHAR *url = pAlert->SendData.Url;
    UINT len = strlen(url);
    if ((len > 0) && (url[len-1] != '/')) 
    {
        strcat(url, "/");
    }

    if (!_pingOnly)
    {
        CHAR *idString = AllocGuidName(pSighting->GetUniqueId(), NULL);
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

    return TRUE;
}

BOOL CctAlert::Match(CctSighting *pSighting)
{
    if (_matchElements.GetCount() == 0) return TRUE;

    ATTRIBUTE *attribute = NULL;
    
    for (UINT i = 0; pSighting->EnumAttributes(i, &attribute); i++)
    {
        for (UINT j = 0; j < _matchElements.GetCount(); j++)
        {
            XGUID id = _matchElements.Get(j);

            if (IsNullXGuid(&id)) continue;

            if (CompareXGuid(&id, &attribute->ElementId) ||
                CompareXGuid(&id, &attribute->ValueId))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

VOID CctAlert::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    for (UINT i = 0; i < _matchElements.GetCount(); i++)
    {
        F.DefineObject(_matchElements.Get(i));
    }

    F.DefineObject(_patrolElementId);
#endif
}

VOID CctAlert::DefineState(CfxFiler &F)
{
    F.DefineValue("Committed", dtBoolean, &_committed);
    F.DefineValue("PatrolElementValue", dtPText, &_patrolValue);
}

VOID CctAlert::DefineProperties(CfxFiler &F)
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

VOID CctAlert::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Active", dtBoolean, &_active, F_FALSE);
    F.DefineValue("Activate Element(s)", dtPGuidList, &_matchElements, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("CaId", dtPText, &_caId);
    F.DefineValue("Description", dtPText, &_description);
    F.DefineValue("Level", dtInt32, &_level);
    F.DefineValue("Password", dtPText, &_password, "PASSWORD");
    F.DefineValue("Patrol Element", dtGuid, &_patrolElementId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Ping frequency (in minutes)", dtInt32, &_pingFrequency, "MIN(0)");
    F.DefineValue("Ping only", dtBoolean, &_pingOnly);
    F.DefineValue("Protocol", dtInt32, &_protocol, "LIST(\"HTTP/HTTPS (GeoJSON)\"=4 \"HTTP/HTTPS (GeoJSON compressed)\"=6)");
    F.DefineValue("Type", dtPText, &_type);
    F.DefineValue("Url", dtPText, &_url);
    F.DefineValue("User name", dtPText, &_userName);
#endif
}

VOID CctAlert::ActivateAlarm()
{
    if (!_committed) return;

    CfxEngine *engine = GetEngine(this);
    if (engine == NULL) return;
    engine->KillAlarm(this);

    /*if (GetHost(this)->GetBatteryState() == batCritical)
    {
        Log("Failed to activate alarm: battery critical");
        return;
    }*/

    if (_pingFrequency != 0)
    {
        engine->SetAlarm(this, _pingFrequency * 60, "Ping1");
        CctTransferManager *transferManager = (CctTransferManager *)_owner;
        CctSession *session = (CctSession *)(transferManager->GetOwner());
        session->GetWaypointManager()->SetMinTimeout(_pingFrequency * 60);
    }
    else
    {
        OnAlarm();
    }
}

VOID CctAlert::OnAlarm()
{
    CctTransferManager *transferManager = (CctTransferManager *)_owner;
    CctSession *session = (CctSession *)(transferManager->GetOwner());
    if (!session->GetConnected()) return;

    if (!_committed) return;

    CfxEngine *engine = GetEngine(this);
    engine->KillAlarm(this);

    /*if (GetHost(this)->GetBatteryState() == batCritical)
    {
        Log("Failed to activate alert: battery critical");
        return;
    }*/

    if (_pingFrequency == 0)
    {
        if (transferManager->HasPendingAlert())
        {
            engine->SetAlarm(this, 60, "UnsentAlerts");                // Retry in 120 seconds
            engine->StartTransfer(session, session->GetOutgoingPath());
        }
    }
    else
    {
        engine->SetAlarm(this, _pingFrequency * 60, "Ping2");    // Retry
        session->GetWaypointManager()->SetMinTimeout(_pingFrequency * 60);
        transferManager->CreatePing(&_alert);
        engine->StartTransfer(session, session->GetOutgoingPath());
    }
}

//*************************************************************************************************
// CctRetainState

CctRetainState::CctRetainState(CfxPersistent *pOwner, CfxControl *pParent): CfxPersistent(pOwner)
{
    _parent = pParent;
    _retain = FALSE;
    _globalValue = NULL;
    _lastValue = 0;
}

CctRetainState::~CctRetainState()
{
    MEM_FREE(_globalValue);
    _globalValue = NULL;
    _parent = NULL;
}

BOOL CctRetainState::MustResetState()
{
    if (_retain && _globalValue)
    {
        GLOBALVALUE *globalValue = GetSession(_parent)->GetGlobalValue(_globalValue);
        return (globalValue == NULL) || (globalValue->Value != _lastValue);
    }
    else
    {
        return FALSE;
    }
}

VOID CctRetainState::DefineState(CfxFiler &F)
{
    if (!_globalValue || !_retain)
    {
        return;
    }

    if (F.IsWriter())
    {
        CctSession *session = GetSession(_parent);
        GLOBALVALUE *globalValue = session->GetGlobalValue(_globalValue);
        _lastValue = globalValue != NULL ? globalValue->Value : 1;
        if ((globalValue == NULL) || (_lastValue != globalValue->Value))
        {
            session->SetGlobalValue(this, _globalValue, _lastValue);
        }
    }

    F.DefineValue("RetainGlobalValue", dtDouble, &_lastValue);
}

VOID CctRetainState::DefineProperties(CfxFiler &F)
{
    F.DefineValue("RetainState", dtBoolean, &_retain, F_FALSE);
    F.DefineValue("RetainStateGlobal", dtPText, &_globalValue);
}

VOID CctRetainState::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Retain state", dtBoolean,  &_retain);
    F.DefineValue("Retain state reset key", dtPText, &_globalValue);
#endif
}

//*************************************************************************************************
// CctWaypointManager

#define WAYPOINT_ALARM_THRESHOLD 60

CctWaypointManager::CctWaypointManager(CfxPersistent *pOwner): CfxPersistent(pOwner)
{
    _alive = FALSE;
    _connected = FALSE;
    _fixTryCount = 0;
    _lastUserEventTime = 0;
    _lastSuccess = FALSE;
    _gpsWasConnected = FALSE;
    _gpsLocked = FALSE;
    _timeout = _minTimeout = 0;
    InitGpsAccuracy(&_accuracy);
    
    _pendingEvent = FALSE;
    _lockCount = 0;
    _uniqueness = 0;
    InitGuid(&_alarmInstanceId);
}

CctWaypointManager::~CctWaypointManager()
{
    Disconnect();
}

VOID CctWaypointManager::BeginUpdate()
{
    _lockCount++;
}

VOID CctWaypointManager::EndUpdate()
{
    FX_ASSERT(_lockCount > 0);
    _lockCount--;

    if ((_lockCount == 0) && _pendingEvent)
    {
        _pendingEvent = FALSE;
        #if defined(QTBUILD)
            OnTrackTimer();
        #else
            OnTimer();
        #endif
    }
}

BOOL CctWaypointManager::InUpdate()
{
    return _lockCount > 0;
}

VOID CctWaypointManager::Connect()
{
    FX_ASSERT(!_connected);

    if (IsNullGuid(&_alarmInstanceId))
    {
        GetHost(this)->CreateGuid(&_alarmInstanceId);
    }

    _connected = TRUE;
    SetupTimer(0);
}

VOID CctWaypointManager::Disconnect()
{
    SetupTimer(0);
    _connected = FALSE;
}

UINT CctWaypointManager::GetFinalTimeout()
{
    if ((_timeout != 0) && (_minTimeout != 0))
    {
        return min(_timeout, _minTimeout);
    }
    else if (_timeout == 0)
    {
        return _minTimeout;
    }
    else
    {
        return _timeout;
    }
}

VOID CctWaypointManager::Restart()
{
    if ((GetFinalTimeout() == 0) || (GetEngine(this)->GetLastUserEventTime() == 0))
    {
        SetupTimer(0);
    }
    else if (!_alive)
    {
        SetupTimer(5);
    }
}

VOID CctWaypointManager::LockGPS()
{
    if (!_gpsLocked) 
    {
        GetEngine(this)->LockGPS();
        _gpsLocked = TRUE;
    }
}

VOID CctWaypointManager::UnlockGPS()
{
    if (_gpsLocked)
    {
        GetEngine(this)->UnlockGPS();
        _gpsLocked = FALSE;
    }
}

VOID CctWaypointManager::SaveAlarmInstanceId()
{
    HANDLE hFile = 0;
    CHAR *workPath = NULL;
    CctSession *session = GetSession();
    if (!session->GetConnected()) return;

    workPath = AllocMaxPath(session->GetApplicationPath());
    AppendSlash(workPath);
    strcat(workPath, "TransferInstanceId");

    hFile = FileOpen(workPath, "wb");
    if (hFile == 0)
    {
        Log("Failed to create InstanceId file");
        goto cleanup;
    }

    if (!FileWrite(hFile, &_alarmInstanceId, sizeof(_alarmInstanceId)))
    {
        Log("Failed to write data: %08lx", FileGetLastError(hFile));
        goto cleanup;
    }

cleanup:

    MEM_FREE(workPath);

    if (hFile != 0)
    {
        FileClose(hFile);
    }
}

BOOL CctWaypointManager::SameAlarmInstanceId()
{
    // Check if this alarm was sent by this instance of the app.
    // If the app crashes, we don't want to restart the timer.
    BOOL result = TRUE;
    HANDLE hFile = 0;
    CHAR *workPath = NULL;
    GUID alarmId;
    CctSession *session = GetSession();
    if (!session->GetConnected()) return TRUE;

    workPath = AllocMaxPath(session->GetApplicationPath());
    AppendSlash(workPath);
    strcat(workPath, "TransferInstanceId");

    hFile = FileOpen(workPath, "rb");
    if (hFile == 0)
    {
        Log("Failed to read InstanceId file");
        goto cleanup;
    }

    if (!FileRead(hFile, &alarmId, sizeof(alarmId)))
    {
        Log("Failed to read data: %08lx", FileGetLastError(hFile));
        goto cleanup;
    }

    result = CompareGuid(alarmId, _alarmInstanceId);

cleanup:

    MEM_FREE(workPath);

    if (hFile != 0)
    {
        FileClose(hFile);
        hFile = 0;
    }

    return result;
}

VOID CctWaypointManager::SetupTimer(UINT ElapseInSeconds)
{
    if (InUpdate())
    {
        return;
    }
    
#if defined(QTBUILD)
    CfxEngine *engine = GetEngine(this);

    if (ElapseInSeconds == 0)
    {
        _lastSuccess = _alive = FALSE;
        UnlockGPS();
        engine->KillTrackTimer(this);
        return;
    }

    engine->SetTrackTimer(this, ElapseInSeconds);

    if (ElapseInSeconds <= WAYPOINT_ALARM_THRESHOLD)
    {
        LockGPS();
    }
    else
    {
        UnlockGPS();
    }

    _alive = TRUE;

#else

    _alive = FALSE;
    
    CfxEngine *engine = GetEngine(this);
    CfxHost *host = GetHost(this);

    // Turn off all timers and Alarms
    engine->KillAlarm(this);
    engine->KillTimer(this);

    // Check for termination and normal failure cases
    if (ElapseInSeconds == 0)// || (host->GetBatteryState() == batCritical))
    {
        _lastSuccess = FALSE;
        UnlockGPS();
        return;
    }

    // Keep our GPS state up to date
    if (ElapseInSeconds <= WAYPOINT_ALARM_THRESHOLD)
    {
        LockGPS();
    }
    else
    {
        UnlockGPS();
    }

    if (ElapseInSeconds <= WAYPOINT_ALARM_THRESHOLD)
    {
        // High resolution timer
        engine->SetTimer(this, ElapseInSeconds * 1000);

        // Wake up alarm so we can be resistent to user manual turn-off
        engine->SetAlarm(this, 60, "WaypointManagerWakeup");
    }
    else
    {
        // Low resolution timer
        engine->SetAlarm(this, ElapseInSeconds, "WaypointManagerLowRes");
    }

    SaveAlarmInstanceId();

    _alive = TRUE;
#endif
}

VOID CctWaypointManager::OnAlarm()
{
#if defined(QTBUILD)
    Log("TrackTimer: ERROR - should not fire alarm on Android");
#endif

    if (!_connected || !GetSession()->GetConnected() || (GetFinalTimeout() == 0) || !SameAlarmInstanceId()) return;

    _alive = TRUE;

    OnTimer();
}

VOID CctWaypointManager::OnTrackTimer()
{
    if (!_connected) return;
    _pendingEvent = _pendingEvent || InUpdate();
    if (InUpdate()) return;

    CfxEngine *engine = GetEngine(this);
    FXGPS *gps = engine->GetGPS();
    if (TestGPS(gps, &_accuracy) && (_timeout != 0))
    {
        _lastSuccess = TRUE;
        StoreRecord(gps);
    }
    else
    {
        _lastSuccess = FALSE;
    }

    engine->Repaint();
    engine->SetTrackTimer(this, GetFinalTimeout());
}

VOID CctWaypointManager::OnTimer()
{
#if defined(QTBUILD)
    Log("TrackTimer: ERROR - should not fire timer on Android");
#endif

    if (!_connected) return;

    _pendingEvent = _pendingEvent || InUpdate();
    if (InUpdate()) return;
    
    CfxEngine *engine = GetEngine(this);
    CfxHost *host = GetHost(this);

    UINT finalTimeout = GetFinalTimeout();

    // If the power is off, then turn it on and try again in 10 seconds
    if (host->GetPowerState() == powOff)
    {
        _fixTryCount = 0;
        _lastUserEventTime =  engine->GetLastUserEventTime();
        host->SetPowerState(powIdle);

        // Try again in a few seconds: don't use SetupTimer because we don't want to touch the GPS yet
        engine->SetTimer(this, max(1, host->GetGPSPowerupTimeSeconds()) * 1000);
        
        engine->Repaint();
        
        if (!host->HasSmartGPSPower())
        {
            FxResetAutoOffTimer();
        }
       
        return;
    }    
    
    // If the GPS is valid or we reached our fixTryCount or we have a really fast timer 
    FXGPS *gps = engine->GetGPS();
    if (TestGPS(gps, &_accuracy) || (_fixTryCount == 6) || (finalTimeout <= 10))
    {
        _fixTryCount = 0;
        _gpsWasConnected = (gps->State == dsConnected);
        
        // Only store a record if the GPS timer is enabled.
        // Otherwise, we're just keeping the GPS on to keep the GPS active for pings.
        if (_timeout != 0)
        {
            StoreRecord(gps);
        }
                
        // If we were connected, then try again in _timeout seconds
        switch (gps->State)
        {
        case dsDetecting:
        case dsConnected:
            SetupTimer(finalTimeout);
            break;

        case dsNotFound:
            break;

        case dsDisconnected: 
            {
                _lastSuccess = FALSE;
                SetupTimer(0);
                for (UINT i=0; i<10; i++)
                {
                    FxSystemBeep();
                    FxSleep(100);
                }
            }
            break;
        
        default:
            FX_ASSERT(FALSE);
            break;    
        }
        
        // Repaint
        engine->Repaint();

        // If we powered it on, then we must power down
        if (_timeout > WAYPOINT_ALARM_THRESHOLD)
        {
            if (engine->GetLastUserEventTime() == _lastUserEventTime)
            {
                _lastUserEventTime = 0;
                host->SetPowerState(powOff);
            }
        }
        else if (gps->State != dsDisconnected)
        {
            // Make sure we don't turn off accidentally
            if (!host->HasSmartGPSPower())
            {
                FxResetAutoOffTimer();
            }
        }
       
        return;
    }
    
    // The power is on and we are waiting for a connection
    _fixTryCount++;
    SetupTimer(10);

    // For fast timers (between 10 and WAYPOINT_ALARM_THRESHOLD seconds),
    // we want the _lastSuccess to accurately reflect the status
    if (_timeout < WAYPOINT_ALARM_THRESHOLD)
    {
        _lastSuccess = FALSE;
        engine->Repaint();
    }

    // Make sure we don't turn off accidentally
    if (!host->HasSmartGPSPower())
    {
        FxResetAutoOffTimer();
    }
}

VOID CctWaypointManager::DefineState(CfxFiler &F)
{
    F.DefineValue("WaypointTimeout", dtInt32, &_timeout);
    F.DefineValue("WaypointMinTimeout", dtInt32, &_minTimeout);
    F.DefineValue("WaypointAccuracyDOP", dtDouble, &_accuracy.DilutionOfPrecision);
    F.DefineValue("WaypointAccuracyMaxSpeed", dtDouble, &_accuracy.MaximumSpeed);
}

BOOL CctWaypointManager::StoreRecord(FXGPS *pGPS, BOOL CheckTime)
{
    FX_ASSERT(_connected);
    _lastSuccess = TestGPS(pGPS, &_accuracy);
    if (!_lastSuccess) return FALSE;

    CfxHost *host = GetHost(this);

    WAYPOINT waypoint = {0};
    waypoint.Magic = MAGIC_WAYPOINT;

    // DateTime and Ids
    FXDATETIME dateTime;
    host->GetDateTime(&dateTime);
    waypoint.DateCurrent = EncodeDateTime(&dateTime);
    host->GetDeviceId(&waypoint.DeviceId);
    host->CreateGuid(&waypoint.Id);

    // These should all be aligned
    waypoint.Latitude  = pGPS->Position.Latitude;
    waypoint.Longitude = pGPS->Position.Longitude;
    waypoint.Altitude  = pGPS->Position.Altitude;
    waypoint.Accuracy  = pGPS->Position.Accuracy;

    return GetSession()->StoreWaypoint(&waypoint, CheckTime);
}

VOID CctWaypointManager::StoreRecordFromSighting(CctSighting *pSighting)
{
    FX_ASSERT(_connected);

    FXGPS_QUALITY fixQuality = pSighting->GetGPS()->Quality;

    if (fixQuality != fqNone)
    {
        CfxHost *host = GetHost(this);

        WAYPOINT waypoint = {0};
        waypoint.Magic = MAGIC_WAYPOINT;
        waypoint.DateCurrent = EncodeDateTime(pSighting->GetDateTime());
        host->GetDeviceId(&waypoint.DeviceId);
        host->CreateGuid(&waypoint.Id);
            
        FXGPS_POSITION *gps = pSighting->GetGPS();
        waypoint.Latitude = gps->Latitude;
        waypoint.Longitude = gps->Longitude;
        waypoint.Altitude = gps->Altitude;
        waypoint.Accuracy = gps->Accuracy;

        GetSession()->StoreWaypoint(&waypoint, TRUE);

        //
        // _lastSuccess measures GPS quality, so a fqUser* fix would not be representative
        //
        _lastSuccess = (fixQuality >= fq2D) || (fixQuality == fq3D) || (fixQuality == fqDiff3D) || (fixQuality == fqSim3D);
    }
    else
    {
        _lastSuccess = FALSE;
    }
}

VOID CctWaypointManager::SetTimeout(UINT Value)
{
    if (Value == _timeout) return;

    _timeout = Value;
    SetupTimer(GetFinalTimeout());
}

VOID CctWaypointManager::SetMinTimeout(UINT Value)
{
    if (Value == _minTimeout) return;

    if (_minTimeout > 0)
    {
        _minTimeout = min(Value, _minTimeout);
    }
    else
    {
        _minTimeout = Value;
    }

    SetupTimer(GetFinalTimeout());
}

VOID CctWaypointManager::DrawStatusIcon(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, COLOR BackColor, COLOR ForeColor, BOOL Invert)
{
    ::DrawWaypointIcon(pCanvas, pRect, pFont, _timeout, BackColor, ForeColor, _lastSuccess, !_alive, Invert);
}

//*************************************************************************************************
// Draw the waypoint timer triangle

VOID DrawWaypointIcon(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Value, COLOR BackColor, COLOR ForeColor, BOOL Fill, BOOL StrikeOut, BOOL Invert)
{
    pCanvas->State.BrushColor = pCanvas->InvertColor(BackColor, Invert);
    pCanvas->FillRect(pRect);
    pCanvas->State.LineColor = pCanvas->InvertColor(ForeColor, Invert);

    FXRECT rect = *pRect;
    InflateRect(&rect, -(INT)(pCanvas->State.LineWidth-1), -(INT)(pCanvas->State.LineWidth-1));

    INT width = rect.Right - rect.Left + 1;
    INT height = rect.Bottom - rect.Top + 1;
    INT h = min(height / 2 - 2, width / 2 - 2);

    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{h, 0}, {h*2, h*2}, {0, h*2}};

    INT x = rect.Left + (width - h*2) / 2;
    INT y = rect.Top + height / 2 - h;

    if (Fill)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(ForeColor, Invert);
        FILLPOLYGON(pCanvas, triangle, x, y);
    }

    if (Value)
    {
        CHAR buffer[10];
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
        
        INT offsetY;
        pCanvas->TextHeight(pFont, buffer, 0, &offsetY);
        
        if ((INT)pCanvas->FontHeight(pFont) - offsetY < h * 2)
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

VOID CctTransferManager::Reset()
{
    _dataUniqueness = 0;
    ResetSentHistory();
    ClearAlerts();
}

CHAR *CctTransferManager::AllocSentHistoryFileName(const CHAR *pExtension)
{
    CHAR workPath[MAX_PATH];
    strcpy(workPath, GetSession()->GetApplicationPath());
    AppendSlash(workPath);
    strcat(workPath, "SentHistory");
    strcat(workPath, pExtension);

    return AllocString(workPath);
}

BOOL CctTransferManager::LoadSentHistory(const CHAR *pExtension)
{
    Reset();

    BOOL result = TRUE;
    CHAR *fileName = AllocSentHistoryFileName(pExtension);

    auto fileMap = FXFILEMAP();
    if (FxMapFile(fileName, &fileMap))
    {
        GUID *id = (GUID *)fileMap.BasePointer;
        for (UINT i = 0; i < fileMap.FileSize / sizeof(GUID); i++, id++)
        {
            _sentHistoryHash.insert({ makeHashGuid(id), true });
        }

        FxUnmapFile(&fileMap);
    }

    MEM_FREE(fileName);

    return result;
}

BOOL CctTransferManager::SaveSentHistory(const CHAR *pExtension)
{
    CHAR *fileName = AllocSentHistoryFileName(pExtension);
    BOOL result = TRUE;

    CfxFileStream fileStream;
    if (fileStream.OpenForWrite(fileName))
    {
        if (_sentHistoryHash.size() != 0)
        {
            CfxStream stream;
            stream.SetSize(_sentHistoryHash.size() * sizeof(GUID));
            for (auto it = _sentHistoryHash.cbegin(); it != _sentHistoryHash.cend(); it++)
            {
                auto id = *it;
                stream.Write(&id, sizeof(GUID));
            }

            result = fileStream.Write(stream.GetMemory(), stream.GetSize());
            if (!result)
            {
                Log("Failed to save sent history file %s", fileName);
            }
        }

        fileStream.Close();
    }

    MEM_FREE(fileName);

    return result;
}

VOID CctTransferManager::DeleteSentHistory(const CHAR *pExtension)
{
    CHAR *fileName = AllocSentHistoryFileName(pExtension);
    FxDeleteFile(fileName);
    MEM_FREE(fileName);
}

BOOL CctTransferManager::HasSentHistoryId(GUID *pId)
{
    return _sentHistoryHash.find(makeHashGuid(pId)) != _sentHistoryHash.end();
}

VOID CctTransferManager::AddSentHistoryId(GUID *pId)
{
    _sentHistoryHash.insert({ makeHashGuid(pId), true });
}

VOID CctTransferManager::RemoveSentHistoryId(const CHAR *pExtension, GUID *pId)
{
    LoadSentHistory(pExtension);
    _sentHistoryHash.erase(makeHashGuid(pId));
    SaveSentHistory(pExtension);
    ResetSentHistory();
}

VOID CctTransferManager::ResetSentHistory()
{
    _sentHistoryHash.clear();
}

VOID CctTransferManager::ClearAlerts()
{
    for (UINT i = 0; i < _alerts.GetCount(); i++)
    {
        delete _alerts.Get(i);
    }
    _alerts.Clear();
}

VOID CctTransferManager::AddAlert(CctAlert *pAlert)
{
    for (UINT i = 0; i < _alerts.GetCount(); i++)
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

VOID CctTransferManager::RemoveAlert(CctAlert *pAlert)
{
    for (UINT i = 0; i < _alerts.GetCount(); i++)
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

VOID CctTransferManager::CommitAlerts(CctSighting *pSighting)
{
    GetSession()->GetWaypointManager()->SetMinTimeout(0);

    // We intend to refresh the already committed alerts, so blow away the old ones.
    for (INT i = (INT)_alerts.GetCount() - 1; i >= 0; i--)
    {
        CctAlert *alert = _alerts.Get(i);
        if (alert->GetCommitted())
        {
            delete alert;
            _alerts.Delete(i);
        }
    }

    // Activate alerts.
    for (UINT j = 0; j < _alerts.GetCount(); j++)
    {
        CctAlert *alert = _alerts.Get(j);
        if (alert->Commit(GetSession(), pSighting))
        {
            alert->ActivateAlarm();
        }
    }
}

VOID CctTransferManager::DefineState(CfxFiler &F)
{
    F.DefineValue("DataUniqueness", dtInt32, &_dataUniqueness);

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
        for (UINT i = 0; i < _alerts.GetCount(); i++)
        {
            CctAlert *alert = _alerts.Get(i);
            alert->DefineProperties(F);
            alert->DefineState(F);
        }
        F.ListEnd();
    }
}

BOOL CctTransferManager::HasOutgoingData()
{
    BOOL result = FALSE;
    FXFILELIST fileInfoList;

    if (FxBuildFileList(GetSession()->GetOutgoingPath(), "*.OUT", &fileInfoList))
    {
        result = fileInfoList.GetCount() > 0;
    }

    FileListClear(&fileInfoList);

    return result;
}

BOOL CctTransferManager::HasTransferData()
{
    return (GetSession()->GetResourceHeader()->SendData.Protocol != FXSENDDATA_PROTOCOL_UNKNOWN) &&
           (_dataUniqueness != GetSession()->GetDataUniqueness());
}

BOOL CctTransferManager::HasExportData()
{
    return (GetSession()->GetResourceHeader()->SendData.Protocol == FXSENDDATA_PROTOCOL_UNKNOWN) &&
           (_dataUniqueness != GetSession()->GetDataUniqueness());
}

BOOL CctTransferManager::HasPendingAlert()
{
    BOOL result = FALSE;
    FXFILELIST fileInfoList;

    if (FxBuildFileList(GetSession()->GetOutgoingPath(), "!*.OUT", &fileInfoList))
    {
        result = fileInfoList.GetCount() > 0;
    }

    FileListClear(&fileInfoList);

    return result;
}

BOOL CctTransferManager::CreateFileInQueue(CHAR *pTargetFileName, CHAR *pTargetFileExtension, BYTE *pData, UINT DataSize)
{
    BOOL result = FALSE;
    HANDLE hFile = 0;

    CHAR *targetPath = AllocMaxPath(GetSession()->GetOutgoingPath());
    if (targetPath == NULL)
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

    result = TRUE;

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

BOOL CctTransferManager::CreateFileInQueue(CHAR *pTargetFileName, CHAR *pTargetFileExtension, CHAR *pSourceFileName, BOOL Compress, BOOL CopyNotMove)
{
    BOOL result = FALSE;

    CHAR *targetPath = AllocMaxPath(GetSession()->GetOutgoingPath());
    if (targetPath == NULL)
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
    result = TRUE;

cleanup:

    MEM_FREE(targetPath);

    return result;
}

BOOL CctTransferManager::PushToOutgoing(CHAR *pSourceFileName, CHAR *pTargetFileExtension, FXSENDDATA *pSendData)
{
    BOOL success = FALSE;

    CHAR *targetFileName = strrchr(pSourceFileName, PATH_SLASH_CHAR);
    if (targetFileName == NULL)
    {
        Log("Bad file name");
        goto cleanup;
    }

    targetFileName++;

    success = CreateFileInQueue(targetFileName, pTargetFileExtension, pSourceFileName, pSendData->Protocol == FXSENDDATA_PROTOCOL_HTTPZ, FALSE);
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

BOOL CctTransferManager::PushMediaToOutgoing(CHAR *pSourceMediaName, CHAR *pTargetFileName, UINT TargetId, FXSENDDATA *pSendData)
{
    BOOL success = FALSE;
    CHAR *targetFileName = AllocMaxPath("");
    CHAR *sourceFileName = GetSession()->AllocMediaFileName(pSourceMediaName);
    CHAR *sourceExtension = NULL;
    if (!FxDoesFileExist(sourceFileName))
    {
        //Testing: FxCopyFile("c:\\Users\\Builder\\Pictures\\Elephant.png", sourceFileName);
        Log("Media file does not exist");
        goto cleanup;
    }

    sourceExtension = GetFileExtension(pSourceMediaName);
    if (sourceExtension == NULL)
    {
        Log("Bad source file name");
        goto cleanup;
    }

    if (targetFileName == NULL)
    {
        Log("Out of memory");
        goto cleanup;
    }

    sprintf(targetFileName, "%s-%lu", strrchr(pTargetFileName, PATH_SLASH_CHAR) + 1, TargetId);

    success = CreateFileInQueue(targetFileName, sourceExtension, sourceFileName, FALSE, TRUE);
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

CHAR *CctTransferManager::BuildFileName(CHAR *pPrefix)
{
    CfxHost *host = GetHost(this);
    CctSession *session = GetSession();
    GUID outgoingId = {0};
    CHAR *outgoingFileId = NULL;
    CHAR *outgoingFileName = NULL;
    CHAR *result = NULL;

    if (!host->CreateGuid(&outgoingId))
    {
        Log("Failed to create outgoing guid.");
        goto cleanup;
    }

    outgoingFileId = AllocGuidName(&outgoingId, NULL);
    outgoingFileName = AllocMaxPath("");

    CHAR *dbFileName;
    dbFileName = session->GetResourceHeader()->ServerFileName;

    if (dbFileName)
    {
        CHAR *p;
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

        for (UINT i = 0; i < 16; i++) 
        {
            CHAR c = *dbFileName;
            if (c == '\0') break;
            if (c == '.') break;

            if (((c < 'A') || (c > 'Z')) &&
                ((c < 'a') || (c > 'z')) &&
                ((c < '0') || (c > '9')))
            {
                c = '_';
            }

            CHAR sz[2];
            sz[0] = c;
            sz[1] = '\0';
            
            strncat(outgoingFileName, sz, 1);
            dbFileName++;
        }
        
        strcat(outgoingFileName, "_");
    }

    result = AllocMaxPath(NULL);
    if (pPrefix)
    {
        strcat(result, pPrefix);
    }
    strcat(result, outgoingFileName);
    strcat(result, outgoingFileId);

  cleanup:

    MEM_FREE(outgoingFileId);
    MEM_FREE(outgoingFileName);

    return result;
}

BOOL CctTransferManager::CreateCTX(CHAR *pFileName, FXFILELIST *pOutMediaFileList, BOOL *pOutHasData, BOOL *pOutHasMore, FXEXPORTFILEINFO* pExportFileInfo)
{
    BOOL success = FALSE;

    *pOutHasMore = FALSE;

    CfxStream stream;
    CfxFileStream fileStream;
    UINT index = 0;
    DBID id = 0;
    CfxHost *host = GetHost(this);
    CctSession *session = GetSession();
    CctSighting *sighting = new CctSighting(this);
    CfxDatabase *sightingDatabase = session->GetSightingDatabase();

    FXEXPORTFILEINFO exportFileInfo = {};
    DOUBLE startTime = 0;
    DOUBLE stopTime = 0;

    CHAR *tempPath = NULL;
    CHAR *workPath = NULL;
    CHAR *elementsPath = NULL;
    CHAR *elementsName = NULL;
	CHAR *serverFileName = NULL;

    workPath = AllocMaxPath(NULL);
    if (workPath == NULL)
    {
        Log("Failed to allocate working space");
        goto cleanup;
    }

    // Get the temporary directory and clear it
    tempPath = session->AllocTempMaxPath(TRUE);
    if (tempPath == NULL)
    {
        Log("Failed to create temp directory");
        goto cleanup;
    }

    //
    // Sightings
    //
    
    strcpy(workPath, tempPath);
    strcat(workPath, "Sightings.dat");
    if (!fileStream.OpenForWrite(workPath))
    {
        Log("Failed to create Sightings.dat");
        goto cleanup;
    }

    index = 1;
    id = INVALID_DBID;
    while (sightingDatabase->Enum(index, &id)) 
    {
        index++;

        // Skip sightings that don't load or fail to build
        if (!sighting->Load(sightingDatabase, id))
        {
            Log("Failed to load sighting %d", index - 1);
            continue;
        }

        if (!HasSentHistoryId(sighting->GetUniqueId()))
        {
            if (fileStream.GetSize() > CTX_QUOTA_SIZE)
            {
                *pOutHasMore = TRUE;
                Log("CTX file reached quota");
                break;
            }

            if (!sighting->BuildExport(session, fileStream, pOutMediaFileList))
            {
                Log("Failed to export sighting to CTX");
            }
            else
            {
                AddSentHistoryId(sighting->GetUniqueId());
                *pOutHasData = TRUE;
                exportFileInfo.sightingCount++;

                DOUBLE dt = EncodeDateTime(sighting->GetDateTime());

                if (startTime == 0 || startTime > dt)
                {
                    startTime = dt;
                }

                if (dt > stopTime)
                {
                    stopTime = dt;
                }
            }
        }
    }

    fileStream.Close();

    delete sighting;
    sighting = NULL;

    if (exportFileInfo.sightingCount == 0)
    {
        FxDeleteFile(workPath);
    }

    //
    // Waypoints
    //

    strcpy(workPath, tempPath);
    strcat(workPath, "Waypoints.dat");
    if (!fileStream.OpenForWrite(workPath))
    {
        Log("Failed to create Waypoints.dat");
        goto cleanup;
    }

    CHAR waypointsName[MAX_PATH];
    strcpy(waypointsName, session->GetName());
    strcat(waypointsName, ".WAY");
    if (host->EnumRecordInit(waypointsName))
    {
        WAYPOINT waypoint;

        // Add new waypoints to the file
        while (host->EnumRecordNext(&waypoint, sizeof(WAYPOINT)))
        {
            if (!HasSentHistoryId(&waypoint.Id))
            {
                if (fileStream.GetSize() > CTX_QUOTA_SIZE)
                {
                    *pOutHasMore = TRUE;
                    Log("CTX file reached quota - waypoints");
                    break;
                }

                if (!fileStream.Write(&waypoint, sizeof(WAYPOINT)))
                {
                    Log("Failed to write waypoint record: %08lx", fileStream.GetLastError());
                }
                else
                {
                    AddSentHistoryId(&waypoint.Id);
                    *pOutHasData = TRUE;
                    exportFileInfo.waypointCount++;

                    if (startTime == 0 || waypoint.DateCurrent < startTime)
                    {
                        startTime = waypoint.DateCurrent;
                    }

                    if (waypoint.DateCurrent > stopTime)
                    {
                        stopTime = waypoint.DateCurrent;
                    }
                }
            }
        }

        host->EnumRecordDone();
    }

    fileStream.Close();

    if (exportFileInfo.waypointCount == 0)
    {
        FxDeleteFile(workPath);
    }

    //
    // No sightings and no waypoints, so we're done.
    //
    if ((exportFileInfo.waypointCount == 0) && (exportFileInfo.sightingCount == 0))
    {
        success = TRUE;
        goto cleanup;
    }

    DecodeDateTime(startTime, &exportFileInfo.startTime);
    DecodeDateTime(stopTime, &exportFileInfo.stopTime);

    //
    // Elements.txt
    //
    
    strcpy(workPath, tempPath);
    strcat(workPath, "Elements.txt");
    
    if (!session->WriteElementsFile(workPath))
    {
        Log("Failed to create elements file");

        // This should not be a fatal failure if someone deleted the Elements.txt file.
    }

    //
    // Info.xml
    //

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
    
    if (!fileStream.OpenForWrite(workPath))
    {
        Log("Failed to create Info.xml");
        goto cleanup;
    }

    if (!fileStream.Write(stream.GetMemory(), stream.GetSize()))
    {
        Log("Failed to write Info.xml file: %08lx", fileStream.GetLastError());
        goto cleanup;
    }

    fileStream.Close();

    //
    // Create the CAB file
    //

    Log("CreateCAB %s, %s", tempPath, workPath);

    if (!CreateCAB(tempPath, pFileName))
    {
        Log("Failed to create CAB file.");
        goto cleanup;
    }

    //
    // CTX has been created successfully
    //

    success = TRUE;

cleanup:

    fileStream.Close();

    if (!*pOutHasData)
    {
        FxDeleteFile(pFileName);
    }

    if (pExportFileInfo)
    {
        *pExportFileInfo = exportFileInfo;
    }

    delete sighting;
    sighting = NULL;

    session->FreeSightingDatabase(sightingDatabase);
    sightingDatabase = NULL;

    if (tempPath)
    {
        ClearDirectory(tempPath);
    }

    MEM_FREE(tempPath);
    MEM_FREE(workPath);
    MEM_FREE(elementsPath);
    MEM_FREE(elementsName);
        
    return success;
}

BOOL CctTransferManager::CreateJSON(CHAR *pFileName, UINT Protocol, FXFILELIST *pOutMediaFileList, BOOL *pOutHasData, INT* startSightingIndex, INT* startWaypointIndex, BOOL *pOverflow)
{
    CfxFileStream fileStream;
    CctJsonBuilder jsonBuilder(GetSession(), &fileStream, Protocol);
    BOOL success = FALSE;
    UINT sightingCount = 0, waypointCount = 0;
    UINT index = 0;
    DBID id = 0;
    CfxHost *host = GetHost(this);
    CctSession *session = GetSession();
    CctSighting *sighting = new CctSighting(this);
    CfxDatabase *sightingDatabase = session->GetSightingDatabase();
    CHAR *workPath = AllocMaxPath("");
    BOOL oneSightingOnly = FALSE;

    *pOutHasData = FALSE;
    *pOverflow = FALSE;

    //
    // Sightings.
    //

    strcpy(workPath, pFileName);
    strcat(workPath, "-0");
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
    
    // Sighting database has changed
    index = *startSightingIndex;
    id = INVALID_DBID;
    while (sightingDatabase->Enum(index, &id)) 
    {
        // Limit size of sighting files.
        if (fileStream.GetSize() > 65536)
        {
            *pOverflow = TRUE;
            break;
        }

        // Overflow if we only want one sighting per file and we already have it.
        if (oneSightingOnly)
        {
            *pOverflow = TRUE;
            break;
        }

        index++;

        // Skip sightings that don't load or fail to build
        if (!sighting->Load(sightingDatabase, id))
        {
            Log("Failed to load sighting %d", index - 1);
            continue;
        }

        if (!HasSentHistoryId(sighting->GetUniqueId()))
        {
            // To make transfer simple in the ESRI case, if there are attachments, just have one sighting.
            if ((Protocol == FXSENDDATA_PROTOCOL_ESRI) && sighting->HasMedia())
            {
                oneSightingOnly = TRUE;
                if (sightingCount > 0)
                {
                    *pOverflow = TRUE;
                    break;
                }
            }

            if (!jsonBuilder.WriteSighting(sighting, NULL, sightingCount == 0, pOutMediaFileList))
            {
                Log("Failed to export sighting to JSON");
            }
            else
            {
                AddSentHistoryId(sighting->GetUniqueId());
                sightingCount++;
                *pOutHasData = TRUE;
            }
        }
    }
    *startSightingIndex = index;

    delete sighting;
    sighting = NULL;

    if (!jsonBuilder.WriteFooter())
    {
        Log("Failed to write stream %s:%d", workPath, fileStream.GetLastError());
        goto cleanup;
    }

    fileStream.Close();

    if (sightingCount == 0)
    {
        FxDeleteFile(workPath);
    }

    //
    // Waypoints.
    //

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

    CHAR waypointsName[MAX_PATH];
    strcpy(waypointsName, session->GetName());
    strcat(waypointsName, ".WAY");
    index = *startWaypointIndex;
    if (host->EnumRecordInit(waypointsName))
    {
        if (host->EnumRecordSetIndex(index, sizeof(WAYPOINT)))
        {
            WAYPOINT waypoint;

            // Add new waypoints to the file
            while (host->EnumRecordNext(&waypoint, sizeof(WAYPOINT)))
            {
                index++;
                if (!HasSentHistoryId(&waypoint.Id))
                {
                    // Limit size of waypoint files.
                    if (fileStream.GetSize() > 65536)
                    {
                        *pOverflow = TRUE;
                        break;
                    }

                    if (!jsonBuilder.WriteWaypoint(&waypoint, (waypointCount == 0)))
                    {
                        Log("Failed to write waypoint to JSON");
                    }
                    else
                    {
                        AddSentHistoryId(&waypoint.Id);
                        waypointCount++;
                        *pOutHasData = TRUE;
                    }
                }
            }
        }

        host->EnumRecordDone();
    }
    *startWaypointIndex = index;

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

    //
    // No sightings and no waypoints, so we're done.
    //

    success = TRUE;

cleanup:

    fileStream.Close();

    delete sighting;
    sighting = NULL;

    session->FreeSightingDatabase(sightingDatabase);
    sightingDatabase = NULL;

    return success;
}

BOOL CctTransferManager::FlushData(CfxHost *host, const CHAR *pAppName)
{
    // Create the host
    auto application = new CctApplication(host);

    // Strip extension
    CHAR Name2[MAX_PATH];
    strcpy(Name2, pAppName);
    CHAR *t = strrchr(Name2, '.');
    if (t != NULL)
    {
        *t = '\0';
    }

    application->Run(Name2);
    auto result = application->GetSession()->GetTransferManager()->CreateTransfer(TRUE);

    delete application;

    return result;
}

BOOL CctTransferManager::CreateTransfer(BOOL ForceClear)
{
    BOOL success = FALSE;
    CctSession *session = GetSession();
    CHAR *workPath = NULL;
    CHAR *outgoingFileNameBase = NULL;
    FXFILELIST mediaFileList;
    BOOL hasData = FALSE;
    BOOL hasMore = FALSE;
    INT moreCounter = 0;
    BOOL overflow = FALSE;
    FXSENDDATA sendData = session->GetResourceHeader()->SendData;
    const CHAR *sentHistoryExtension = NULL;

    // Override the username and password from the session if they exist.
    if (session->GetUserName() && session->GetPassword())
    {
        strcpy(sendData.UserName, session->GetUserName());
        strcpy(sendData.Password, session->GetPassword());
    }

    // Must have credentials to send with ESRI.
    if ((sendData.Protocol == FXSENDDATA_PROTOCOL_ESRI) &&
        ((sendData.UserName[0] == '\0') || (sendData.Password[0]) == '\0'))
    {
        GetSession()->ShowMessage("ESRI credentials not specified");
        return FALSE;
    }

    workPath = AllocMaxPath(NULL);
    if (workPath == NULL)
    {
        Log("Failed to allocate working space");
        goto cleanup;
    }

    outgoingFileNameBase = BuildFileName();
    if (outgoingFileNameBase == NULL)
    {
        Log("Failed to allocate working space");
        goto cleanup;
    }

    sentHistoryExtension = sendData.Protocol == FXSENDDATA_PROTOCOL_BACKUP ? ".BAK" : ".SNT";
    if (!LoadSentHistory(sentHistoryExtension))
    {
        Log("Failed to init sent history");
        goto cleanup;
    }

    switch (sendData.Protocol)
    {
    case FXSENDDATA_PROTOCOL_UNC:
    case FXSENDDATA_PROTOCOL_FTP:
    case FXSENDDATA_PROTOCOL_AZURE:
        strcpy(workPath, session->GetOutgoingPath());
        AppendSlash(workPath);
        strcat(workPath, outgoingFileNameBase);

        moreCounter = 0;
        success = hasMore = TRUE;

        while (success && hasMore)
        {
            auto filePath = (QString(workPath) + "_" + QString::number(moreCounter++)).toLatin1();
            auto filePath_str = filePath.data();
            success = CreateCTX(filePath_str, &mediaFileList, &hasData, &hasMore);

            if (success && hasData)
            {
                success = PushToOutgoing(filePath_str, CLIENT_EXPORT_EXT, &sendData);
                if (!success)
                {
                    Log("Failed to create data file in queue.");
                    goto cleanup;
                }
            }
        }
        break;

    case FXSENDDATA_PROTOCOL_UNKNOWN:
        {
            strcpy(workPath, session->GetOutgoingPath());
            AppendSlash(workPath);
            strcat(workPath, outgoingFileNameBase);

            moreCounter = 0;
            success = hasMore = TRUE;

            while (success && hasMore)
            {
                auto filePath = (QString(workPath) + "_" + QString::number(moreCounter++) + QString(CLIENT_EXPORT_EXT)).toLatin1();
                auto filePath_str = filePath.data();

                FXEXPORTFILEINFO exportFileInfo = {};
                success = CreateCTX(filePath_str, &mediaFileList, &hasData, &hasMore, &exportFileInfo);
                if (FxDoesFileExist(filePath_str))
                {
                    GetHost(this)->RegisterExportFile(filePath_str, &exportFileInfo);
                    ForceClear = TRUE;
                }
            }
        }

        break;

    case FXSENDDATA_PROTOCOL_BACKUP:
        strcpy(workPath, session->GetBackupPath() ? session->GetBackupPath() : session->GetOutgoingPath());
        AppendSlash(workPath);
        strcat(workPath, outgoingFileNameBase);

        moreCounter = 0;
        success = hasMore = TRUE;

        while (success && hasMore)
        {
            auto filePath = (QString(workPath) + "_" + QString::number(moreCounter++) + QString(CLIENT_EXPORT_EXT)).toLatin1();
            auto filePath_str = filePath.data();

            success = CreateCTX(filePath_str, &mediaFileList, &hasData, &hasMore);
        }
        break;

    case FXSENDDATA_PROTOCOL_HTTP:
    case FXSENDDATA_PROTOCOL_HTTPS:
    case FXSENDDATA_PROTOCOL_HTTPZ:
    case FXSENDDATA_PROTOCOL_ESRI:
        INT startSightingIndex = 1;
        INT startWaypointIndex = 0;
        do
        {
            if (outgoingFileNameBase == NULL)
            {
                outgoingFileNameBase = BuildFileName();
                if (outgoingFileNameBase == NULL)
                {
                    Log("Failed to allocate working space");
                    goto cleanup;
                }
            }

            strcpy(workPath, session->GetOutgoingPath());
            AppendSlash(workPath);
            strcat(workPath, outgoingFileNameBase);
            success = CreateJSON(workPath, sendData.Protocol, &mediaFileList, &hasData, &startSightingIndex, &startWaypointIndex, &overflow);

            if (success && hasData)
            {
                CHAR tempPath[MAX_PATH];

                // Send sighting data
                strcpy(tempPath, workPath);
                strcat(tempPath, "-0");
                if (FxDoesFileExist(tempPath))
                {
                    success = PushToOutgoing(tempPath, CLIENT_EXPORT_JSON_EXT, &sendData);
                    if (!success)
                    {
                        Log("Failed to push sightings");
                        goto cleanup;
                    }
                }
                
                if (sendData.Protocol == FXSENDDATA_PROTOCOL_ESRI)  // ESRI attachments are special.
                {
                    for (UINT i = 0; i < mediaFileList.GetCount(); i++)
                    {
                        if (!PushMediaToOutgoing(mediaFileList.GetPtr(i)->Name, tempPath, i, &sendData))
                        {
                            Log("Failed to push media: continuing...");
                        }
                    }
                }

                // Send waypoint data
                strcpy(tempPath, workPath);
                strcat(tempPath, "-1");
                if (FxDoesFileExist(tempPath))
                {
                    success = PushToOutgoing(tempPath, CLIENT_EXPORT_JSON_EXT, &sendData);
                    if (!success)
                    {
                        Log("Failed to push sightings");
                        goto cleanup;
                    }
                }

                SaveSentHistory(sentHistoryExtension);
            }

            MEM_FREE(outgoingFileNameBase);
            outgoingFileNameBase = NULL;

        } while (overflow);
        break;
    }

    //
    // Cleanup data if required.
    //

    if (success && (session->GetResourceHeader()->ClearDataOnSend || ForceClear))
    {
        // Delete sighting database.
        session->GetSightingDatabase()->DeleteAll();
        if (session->HasFaulted()) goto cleanup;
        session->InitSightingDatabase();
        if (session->HasFaulted()) goto cleanup;

        // Delete waypoint database.
        CHAR waypointsName[MAX_PATH];
        strcpy(waypointsName, session->GetName());
        strcat(waypointsName, ".WAY");
        GetHost(session)->DeleteRecords(waypointsName);

        // Cleanup the media.
        for (UINT i = 0; i < mediaFileList.GetCount(); i++)
        {
            session->DeleteMedia(mediaFileList.Get(i).Name, TRUE);
        }

        // Reset the sighting/waypoint sent indexes.
        // BUGBUG: How bad is it if we don't do this?
        DeleteSentHistory(".SNT");
        DeleteSentHistory(".BAK");
    }

    _dataUniqueness = session->GetDataUniqueness();
    SaveSentHistory(sentHistoryExtension);
    session->SaveState();

cleanup:

    MEM_FREE(outgoingFileNameBase);
    MEM_FREE(workPath);

    ResetSentHistory();

    FileListClear(&mediaFileList);

    session->Fault(success, "Failed to package sightings");

    return success;
}

BOOL CctTransferManager::CreateAlert(CctSighting *pSighting, FXALERT *pAlert)
{
    BOOL result = FALSE;
    CctSession *session = GetSession();

    CHAR *tempPath = NULL;
    CHAR *outgoingFileNameBase = NULL;
    UINT protocol = pAlert->SendData.Protocol;
    CfxFileStream fileStream;
    CctJsonBuilder jsonBuilder(session, &fileStream, protocol);
    CHAR *prefix = NULL;
    if (pSighting)
    {
        prefix = "!";
    }

    //
    // Build the path.
    //

    tempPath = session->AllocTempMaxPath(TRUE);
    if (tempPath == NULL)
    {
        Log("Failed to create temp path.");
        goto cleanup;
    }

    outgoingFileNameBase = BuildFileName(prefix);
    if (outgoingFileNameBase == NULL)
    {
        Log("Failed to create outgoing path.");
        goto cleanup;
    }

    //
    // Create the alert JSON.
    //

    CHAR workPath[MAX_PATH];
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

    if (!jsonBuilder.WriteSighting(pSighting, pAlert, TRUE, NULL))
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

    result = TRUE;

  cleanup:

    MEM_FREE(outgoingFileNameBase);

    return result;
}

BOOL CctTransferManager::CreatePing(FXALERT *pAlert)
{
    CfxEngine *engine = GetEngine(this);
    FXGPS_POSITION *lastFix = engine->GetGPSLastFix();

    // Return TRUE here means that the ping will activated.
    // We skip the ping if there is no GPS.
    if (!TestGPS(lastFix)) return TRUE;

    return CreateAlert(NULL, pAlert);
}

VOID CctTransferManager::Update()
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

VOID CctTransferManager::Disconnect()
{
    GetEngine(this)->KillAlarm(this);
    ClearAlerts();
}

VOID CctTransferManager::OnAlarm()
{
    if (!GetSession()->GetConnected()) return;

    BOOL buildSuccess = FALSE;

    if (HasTransferData())
    {
        buildSuccess = CreateTransfer();
    }

    if (HasOutgoingData()) 
    {
        GetEngine(this)->StartTransfer(this, GetSession()->GetOutgoingPath());
    }

    Update();
}

//*************************************************************************************************
// CctUpdateManager

CctUpdateManager::CctUpdateManager(CfxPersistent *pOwner): CfxPersistent(pOwner)
{
    _lastCheckTime = _lastUpdateTime = 0;    
    _updateReadyPath = NULL;
    _updateTag = _updateUrl = NULL;
}

CctUpdateManager::~CctUpdateManager()
{
    MEM_FREE(_updateReadyPath);
    _updateReadyPath = NULL;

    MEM_FREE(_updateTag);
    _updateTag = NULL;

    MEM_FREE(_updateUrl);
    _updateUrl = NULL;
}

BOOL CctUpdateManager::IsUpdateSupported()
{
    if (!GetHost(this)->IsUpdateSupported())
    {
        return FALSE;
    }

    CTRESOURCEHEADER *h = GetSession()->GetResourceHeader();
    if (h->WebUpdateUrl[0] == '\0')
    {
        return FALSE;
    }

    return TRUE;
}

VOID CctUpdateManager::CheckForUpdate()
{
    if (!IsUpdateSupported())
    {
        return;
    }

    FXDATETIME now;
    GetHost(this)->GetDateTime(&now);
    DOUBLE nowD = EncodeDateTime(&now);

    if ((nowD - _lastCheckTime) * 24 * 60 * 60 < 10)    // Don't check faster than every 10 seconds.
    {
        return;
    }

    _lastCheckTime = nowD;

    CHAR *path = AllocMaxPath(GetSession()->GetApplicationPath());
    strcat(path, "Update");
    FxCreateDirectory(path);
    ClearDirectory(path);
    AppendSlash(path);

    CTRESOURCEHEADER *h = GetSession()->GetResourceHeader();
    CHAR *webUpdateUrl = GetSession()->GetUpdateUrl();
    GetHost(this)->CheckForUpdate(h->SequenceId, h->StampId, webUpdateUrl, path);
    MEM_FREE(webUpdateUrl);

    MEM_FREE(path);
    path = NULL;
}

VOID CctUpdateManager::DefineState(CfxFiler &F)
{
    F.DefineValue("LastCheckTime", dtDouble, &_lastCheckTime);
    F.DefineValue("LastUpdateTime", dtDouble, &_lastUpdateTime);
}

VOID CctUpdateManager::OnAlarm()
{
    if (_updateReadyPath)
    {
        // If the user is working, defer the update.
        if (FxGetTickCount() - GetEngine(this)->GetLastUserEventTime() < FxGetTicksPerSecond() * 120)
        {
            // Come back here in 60 seconds and try again.
            GetEngine(this)->SetAlarm(this, 60, "Update deferred");
            return;
        }

        // Update now.
        UpdateInPlace(_updateReadyPath, _updateTag, _updateUrl);

        MEM_FREE(_updateReadyPath);
        _updateReadyPath = NULL;
        
        MEM_FREE(_updateTag);
        _updateTag = NULL;

        MEM_FREE(_updateUrl);
        _updateUrl = NULL;
    }
    else
    {
        CheckForUpdate();
    }

    CTRESOURCEHEADER *h = GetSession()->GetResourceHeader();
    GetEngine(this)->SetAlarm(this, h->WebUpdateFrequency * 60, "UpdateCheck");
}

VOID CctUpdateManager::OnSessionCommit()
{
    if (_updateReadyPath)
    {
        UpdateInPlace(_updateReadyPath, _updateTag, _updateUrl);

        MEM_FREE(_updateReadyPath);
        _updateReadyPath = NULL;

        MEM_FREE(_updateTag);
        _updateTag = NULL;

        MEM_FREE(_updateUrl);
        _updateUrl = NULL;
    }
}

VOID CctUpdateManager::Connect()
{
    if (!IsUpdateSupported())
    {
        return;
    }

    CTRESOURCEHEADER *h = GetSession()->GetResourceHeader();
    if (h->WebUpdateCheckOnLaunch)
    {
        CheckForUpdate();
    }

    if (h->WebUpdateFrequency)
    {
        FXDATETIME now;
        GetHost(this)->GetDateTime(&now);
        DOUBLE nowD = EncodeDateTime(&now);

        DOUBLE minSinceLastCheck = (nowD - _lastCheckTime) * 24 * 60;

        if (minSinceLastCheck >= (DOUBLE)h->WebUpdateFrequency)
        {
            CheckForUpdate();
            minSinceLastCheck = 0;
        }

        GetEngine(this)->SetAlarm(this, (h->WebUpdateFrequency - (UINT)minSinceLastCheck) * 60, "UpdateCheck");
    }
}

VOID CctUpdateManager::Disconnect()
{
    GetEngine(this)->KillAlarm(this);
}

BOOL CctUpdateManager::HasData(CfxHost* host, const CHAR *pAppName)
{
    auto dbName = QString(pAppName) + ".DAT";
    auto db = host->CreateDatabase((char *)dbName.toStdString().c_str());
    auto result = db->GetCount() > 1;
    delete db;

    return result;
}

VOID CctUpdateManager::Install(const CHAR *pSrcPath, const CHAR *pDstPath, const CHAR *pAppName, const CHAR *pTag, const CHAR *pUrl)
{
    CHAR *workPath = AllocMaxPath(pDstPath);
    AppendSlash(workPath);

    // Build a list of the files to copy.
    FXFILELIST fileList;
    FxBuildFileList(pSrcPath, "*", &fileList);
    CHAR tempPath[MAX_PATH]; 

    for (UINT i = 0; i < fileList.GetCount(); i++)
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
            FileWrite(fileHandle, (VOID *)pTag, strlen(pTag) + 1);
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
            FileWrite(fileHandle, (VOID *)pUrl, strlen(pUrl) + 1);
            FileClose(fileHandle);
        }
    }

    // Delete state file.
    strcpy(tempPath, workPath);
    strcat(tempPath, pAppName);
    strcat(tempPath, ".STA");
    FxDeleteFile(tempPath);

    MEM_FREE(workPath);
    workPath = NULL;
}

VOID CctUpdateManager::UpdateInPlace(const CHAR *pSrcPath, const CHAR *pTag, const CHAR *pUrl)
{
    // Don't update faster than every 60 seconds.
    FXDATETIME now;
    GetHost(this)->GetDateTime(&now);
    DOUBLE nowD = EncodeDateTime(&now);

    if ((nowD - _lastUpdateTime) * 24 * 60 * 60 < 60)    
    {
        Log("Ignoring update request since time since last update is < 60 seconds");
        return;
    }

    CctSession *session = GetSession();

    // Send all data.
    session->GetTransferManager()->CreateTransfer(TRUE);

    // If we faulted sending data, then do not continue.
    if (session->HasFaulted())
    {
        return;
    }

    // Copy out the app name, so we can use it after Disconnect.
    CHAR appName[MAX_PATH];
    strcpy(appName, session->GetName());

    // Disconnect the session
    session->Disconnect();

    // Update
    CHAR *dstPath = GetHost(this)->AllocPathApplication("");
    Install(pSrcPath, dstPath, appName, pTag, pUrl);
    MEM_FREE(dstPath);

    // Reconnect the session
    session->Connect(appName, 0);
    session->Run();
    session->ShowMessage("Application updated. Tap to continue.", 0x008000);

    // Set a timestamp with the last update.
    _lastUpdateTime = nowD;
}

VOID CctUpdateManager::DoUpdateReady(const CHAR *pAppId, const CHAR *pStampId, const CHAR *pTargetPath, const CHAR *pTag, const CHAR *pUrl, UINT ErrorCode)
{
    if (!IsUpdateSupported())
    {
        return;
    }

    switch (ErrorCode)
    {
    case 0:
        {
            // Ensure this update is from this app id.
            CHAR s[39] = {0};
            CTRESOURCEHEADER *h = GetSession()->GetResourceHeader();
            GUID *g = &h->SequenceId;
            sprintf(s,
                    "%08lx%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
                    g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
                    g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);
            if (stricmp(pAppId, s) == 0)
            {
                g = &h->StampId;
                sprintf(s,
                        "%08lx%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
                        g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
                        g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);

                if (stricmp(pStampId, s) != 0)
                {
                    _updateReadyPath = AllocString(pTargetPath);
                    _updateTag = AllocString(pTag);
                    _updateUrl = AllocString(pUrl);

                    UpdateInPlace(pTargetPath, _updateTag, _updateUrl);
                }
            }
            else
            {
                Log("Bad sequence id, probably from another session -> ignoring");
            }
        }
        break;

    case 1: 
        Log("UpdateReady: Error 1: Update in progress");
        break;

    case 2:
        Log("UpdateReady: Error 2: Error retrieving DEF file");
        break;

    case 3:
        Log("UpdateReady: Error 3: Bad DEF file");
        break;

    case 4:
        Log("UpdateReady: Error 4: Bad client version");
        break;

    case 5:
        Log("UpdateReady: Error 5: Already up to date, nothing to do");
        break;

    case 6:
        Log("UpdateReady: Error 6: Failed to download ZIP file");
        break;

    case 7:
        Log("UpdateReady: Error 7: Failed to unpack ZIP file");
        break;

    default:
        Log("UpdateReady: Error %lu: Unknown", ErrorCode);
        break;
    }
}

//*************************************************************************************************
//
// CfxSessionScreen: Suppress the paint routine, since the parent screen would have painted already
//
class CfxSessionScreen: public CfxScreen
{
public:
    CfxSessionScreen(CfxPersistent *pOwner, CfxControl *pParent): CfxScreen(pOwner, pParent) {};
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) {}
};

//*************************************************************************************************

CfxControl *Create_Control_Session(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctSession(pOwner, pParent);
}

CctSession::CctSession(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent), _events(), _actions(0), _stateActions(0), _waypointManager(0), _transferManager(0), _updateManager(0), _sighting(pOwner), _backupSighting(pOwner), _store(), _globalValues(), _retainStates(), _dialogStates(), _customGotos()
{
    InitControl(&GUID_CONTROL_SESSION);

    memset(&_messageText[0], 0, sizeof(_messageText));

    _screen = _screenRight = NULL;
    _messageWindow = NULL;

    _resource = 0;
    _name[0] = '\0';

    _sdPath = NULL;
    _outgoingPath = NULL;
    _mediaPath = NULL;
    _applicationPath = NULL;
    _tempPath = NULL;

    _userName = _password = NULL;

    _stateSlot = 0;
    InitXGuid(&_firstScreenId);

    _dataUniqueness = 0;
    _shareDataEnabled = FALSE;

    _sightingId = INVALID_DBID;
    _resourceStampId = GUID_0;

    _sightingDB = _stateDB = NULL;

    InitXGuid(&_defaultSave1Target);
    InitXGuid(&_defaultSave2Target);

    memset(&_lastGPS, 0, sizeof(_lastGPS));
    _saving = FALSE;
    _transferOnSave = FALSE;

    _pendingGotoTitle = NULL;

    _actions.SetOwner(this);
    _stateActions.SetOwner(this);
    _waypointManager.SetOwner(this);
    _transferManager.SetOwner(this);
    _updateManager.SetOwner(this);

    _faulted = FALSE;
    _connected = FALSE;

    _historyItemDef = _historyItem = NULL;
    _historyItemCount = 0;
    _historyItemIndex = 0;
    _historyItemSize = 0;

    _sightingAccuracyName = NULL;
    InitGpsAccuracy(&_sightingAccuracy);
    _sightingFixCount = SIGHTING_DEFAULT_FIX_COUNT;

    _rangeFinderLastState = _rangeFinderConnected = FALSE;
    
    if (IsLive())
    {
        _screen = new CfxSessionScreen(_owner, this);
        GetWindow()->SetDock(dkFill);
        GetWindow()->SetComposite(TRUE);

        _messageWindow = new CfxControl_Button(_owner, this);
        GetMessageWindow()->SetVisible(FALSE);
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

CfxDatabase *CctSession::GetSightingDatabase() 
{
    if (_sightingDB)
    {
        return _sightingDB;
    }
    else
    {
        CHAR name[MAX_PATH];
        strcpy(name, _name);
        strcat(name, ".DAT");
        return GetHost(this)->CreateDatabase(name); 
    }
}

VOID CctSession::FreeSightingDatabase(CfxDatabase *pDatabase)
{
    if (_sightingDB == NULL)
    {
        delete pDatabase;
    }
}

CfxDatabase *CctSession::GetStateDatabase()
{
    if (_stateDB)
    {
        return _stateDB;
    }
    else 
    {
        CHAR name[MAX_PATH];
        if (_stateSlot == 0)
        {
            sprintf(name, "%s.STA", _name);
        }
        else
        {
            sprintf(name, "%s.%lu.STA", _name, _stateSlot);
        }

        return GetHost(this)->CreateDatabase(name); 
    }
}

VOID CctSession::FreeStateDatabase(CfxDatabase *pDatabase)
{
    if (_stateDB == NULL)
    {
        delete pDatabase;
    }
}

VOID CctSession::SetDatabaseCaching(BOOL Enabled)
{
    delete _stateDB;
    _stateDB = NULL;

    delete _sightingDB;
    _sightingDB = NULL;

    if (Enabled)
    {
        _sightingDB = GetSightingDatabase();
        _stateDB = GetStateDatabase();
    }
}

BOOL CctSession::ValidateSightingHeader(CfxDatabase *SightingDatabase)
{
    BOOL found = FALSE;
    DBID dbId = 0;

    if (SightingDatabase->GetCount() == 0)
    {
        goto Done;
    }

    if (SightingDatabase->Enum(0, &dbId))
    {
        VOID *buffer = NULL;
        if (SightingDatabase->Read(dbId, &buffer) && buffer)
        {
            found = ((FXDATABASEHEADER*)buffer)->Magic == MAGIC_SESSION;
        }
        MEM_FREE(buffer);
    }

Done:
    return found;
}

BOOL CctSession::UseSDStorage()                
{
    return (_sdPath != NULL) && GetResourceHeader()->UseSD && !GetApplication(this)->GetDemoMode();
}

BOOL CctSession::SD_Initialize()
{
    if (!UseSDStorage()) return FALSE;

    BOOL result = FALSE;
    CHAR *serverFileName = NULL;

    CfxStream stream;
    CfxFileStream fileStream;

    // Create Info.XML file.
    CHAR workPath[MAX_PATH];
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

VOID CctSession::SD_Finalize()
{
    if (!UseSDStorage()) return;

    BOOL hasSighting = FALSE;
    BOOL hasWaypoint = FALSE;

    CHAR name[MAX_PATH];
    
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

BOOL CctSession::ArchiveSighting(CctSighting *pSighting)
{
    if (!_shareDataEnabled) return FALSE;

    auto attributes = QVariantMap();

    for (auto i = 0; i < (int)pSighting->GetAttributes()->GetCount(); i++)
    {
        ATTRIBUTE *attribute = pSighting->GetAttributes()->GetPtr(i);

        auto name = GUIDToString(&attribute->ElementGuid);
        auto value = QString();

        if (!IsNullXGuid(&attribute->ValueId))
        {
            FXTEXTRESOURCE* element = (FXTEXTRESOURCE *)_resource->GetObject(this, attribute->ValueId, eaName);
            if (element != NULL)
            {
                value = GUIDToString(&element->Guid);
                _resource->ReleaseObject(element);
            }
        }
        else
        {
            CfxString s;
            Type_VariantToText(attribute->Value, s);
            value = s.Get();
        }

        if (!Type_IsText((FXDATATYPE)(attribute->Value->Type)))
        {
            if (value.isEmpty())
            {
                value = "1";
            }
        }

        if (attribute->IsMedia())
        {
            value = QString("media://") + QString(attribute->GetMediaName());
        }

        attributes.insert(name, value);
    }

    GetHost(this)->ArchiveSighting(pSighting->GetUniqueId(), pSighting->GetDateTime(), pSighting->GetGPS(), attributes);

    return TRUE;
}

BOOL CctSession::SD_WriteSighting(CctSighting *pSighting)
{
    if (!UseSDStorage()) return FALSE;

    BOOL Result = FALSE;

    CHAR *tempFileName = AllocTempMaxPath(FALSE);
    AppendSlash(tempFileName);
    strcat(tempFileName, "Sighting.dat");
    FxDeleteFile(tempFileName);

    CfxFileStream fileStream;
    if (!fileStream.OpenForWrite(tempFileName))
    {
        Log("Error writing to temp file");
        goto cleanup;
    }

    if (pSighting->BuildExport(this, fileStream, NULL))
    {
        fileStream.Close();

        CHAR name[MAX_PATH];
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
    tempFileName = NULL;

    Fault(Result, "Error saving sighting to SD");

    return Result;
}

BOOL CctSession::ArchiveWaypoint(WAYPOINT *pWaypoint)
{
    if (!_shareDataEnabled) return FALSE;

    FXDATETIME dateTime = {};
    DecodeDateTime(pWaypoint->DateCurrent, &dateTime);
    FXGPS_POSITION position = {};
    position.Quality = fq3D;
    position.Latitude = pWaypoint->Latitude;
    position.Longitude = pWaypoint->Longitude;
    position.Altitude = pWaypoint->Altitude;
    position.Accuracy = pWaypoint->Accuracy;

    GetHost(this)->ArchiveWaypoint(&pWaypoint->Id, &dateTime, &position);

    return TRUE;
}

BOOL CctSession::SD_WriteWaypoint(WAYPOINT *pWaypoint)
{
    if (!UseSDStorage()) return FALSE;

    CHAR name[MAX_PATH];
    strcpy(name, _sdPath);
    strcat(name, _name);
    strcat(name, ".WAY");

    BOOL Result;

    Result = GetHost(this)->AppendRecord(name, pWaypoint, sizeof(WAYPOINT));

    Fault(Result, "Error writing track to SD");

    return Result;
}

BOOL CctSession::WriteElementsFile(CHAR *pFileName)
{
#ifdef CLIENT_DLL
    typedef BOOL (APIENTRY *FN_Server_WriteElementsFile)(CHAR *pPath);
    static FN_Server_WriteElementsFile _fn_Server_WriteElementsFile;

    if (_fn_Server_WriteElementsFile == NULL)
    {
        ::HMODULE handle = ::GetModuleHandle(NULL); 
        _fn_Server_WriteElementsFile = (FN_Server_WriteElementsFile) ::GetProcAddress(handle, "Server_WriteElementsFile");
    }
    FX_ASSERT(_fn_Server_WriteElementsFile != NULL);

    if (!_fn_Server_WriteElementsFile(pFileName))
    {
        Log("Failed to write elements file");
        return FALSE;
    }
    else 
    {
        return TRUE;
    }
#else
    BOOL result = FALSE;
    CHAR *srcName = AllocMaxPath(_name);
    strcat(srcName, ".TXT");
    CHAR *srcPath = GetHost(this)->AllocPathApplication(srcName);

    result = FxCopyFile(srcPath, pFileName);

    MEM_FREE(srcPath);
    MEM_FREE(srcName);

    return result;
#endif
}

CHAR *CctSession::AllocMediaFileName(CHAR *pMediaName)
{
    CHAR *result = AllocMaxPath(_mediaPath);
    if (!result)
    {
        return NULL;
    }

    if (pMediaName)
    {
        strcat(result, pMediaName);
    }
    else
    {
        GUID mediaId = {0};
        GetHost(this)->CreateGuid(&mediaId);
        pMediaName = AllocGuidName(&mediaId, NULL);
        strcat(result, pMediaName);
        MEM_FREE(pMediaName);
        pMediaName = NULL;
    }

    return result;
}

BOOL CctSession::DeleteMedia(CHAR *pMediaName, BOOL KeepBackup)
{
    BOOL result = FALSE;
    CHAR *fileName = AllocMediaFileName(pMediaName);

    if (fileName)
    {
        result = FxDeleteFile(fileName);

        GetHost(this)->RequestMediaScan(fileName);

        MEM_FREE(fileName);
    }

    if (UseSDStorage() && !KeepBackup)
    {
        CHAR backupPath[MAX_PATH];
        strcpy(backupPath, _sdPath);
        strcat(backupPath, "Media/");
        strcat(backupPath, pMediaName);
        FxDeleteFile(backupPath);
    }

    return result;
}

BOOL CctSession::BackupMedia(CHAR *pMediaName)
{
    if (!UseSDStorage()) return FALSE;

    CHAR dstPath[MAX_PATH];
    strcpy(dstPath, _sdPath);
    strcat(dstPath, "Media");
    if (!FxCreateDirectory(dstPath))
    {
        return FALSE;
    }

    AppendSlash(dstPath);
    strcat(dstPath, pMediaName);

    CHAR *srcPath = AllocMediaFileName(pMediaName);
    BOOL Result = FxCopyFile(srcPath, dstPath);
    MEM_FREE(srcPath);

    return Result;
}

CHAR *CctSession::GetMediaName(CHAR *pFileName)
{
    if (pFileName == NULL)
    {
        return NULL;
    }

    UINT prefixLen = strlen(_mediaPath);
    UINT len = strlen(pFileName);

    if (len > prefixLen)
    {
        return pFileName + prefixLen;
    }
    else
    {
        return NULL;
    }
}

CHAR *CctSession::AllocTempMaxPath(BOOL Clear)
{
    CHAR *path = AllocMaxPath(_tempPath);
    if (!path)
    {
        return NULL;
    }

    if (Clear)
    {
        if (!ClearDirectory(path))
        {
            MEM_FREE(path);
            return NULL;
        }
    }

    AppendSlash(path);
    return path;
}

BOOL CctSession::InitPaths()
{
    MEM_FREE(_applicationPath);
    _applicationPath = NULL;
    MEM_FREE(_mediaPath);
    _mediaPath = NULL;
    MEM_FREE(_outgoingPath);
    _outgoingPath = NULL;
    MEM_FREE(_tempPath);
    _tempPath = NULL;
    MEM_FREE(_sdPath);
    _sdPath = NULL;

    BOOL result = FALSE;
    CHAR *path = NULL;
    
    //
    // Application path.
    //

    path = GetHost(this)->AllocPathApplication(NULL);
    if (path == NULL)
    {
        goto cleanup;
    }

#ifdef CLIENT_DLL
    // On client, append a unique path to differentiate from other sessions.
    {
        GUID sessionId = {0};
        CHAR *pathWithSession = AllocMaxPath(path);

        GetHost(this)->CreateGuid(&sessionId);
        CHAR *sessionName = AllocGuidName(&sessionId, NULL);
        if (sessionName == NULL)
        {
            MEM_FREE(pathWithSession);
            goto cleanup;
        }

        strcat(pathWithSession, sessionName);
        MEM_FREE(sessionName);

        AppendSlash(pathWithSession);
        MEM_FREE(path);
        path = pathWithSession;
    }
#endif

    _applicationPath = AllocString(path);
    MEM_FREE(path);
    path = NULL;

    //
    // SD Path.
    // This is only active if UseSD or backup.
    // Android writes to this path for backup, but will not use it for anything else.
    // We are moving to a model where backup happens at regular intervals via a timer.
    //

    if (!GetApplication(this)->GetDemoMode() &&
        (GetResourceHeader()->UseSD || (GetResourceHeader()->SendData.Protocol == FXSENDDATA_PROTOCOL_BACKUP)))
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
    // Media path.
    //

    path = AllocMaxPath(_applicationPath);
    if (!path)
    {
        Log("Bad media path");
        goto cleanup;
    }
    strcat(path, "Media");
    AppendSlash(path);
    _mediaPath = AllocString(path);
    MEM_FREE(path);
    path = NULL;

    //
    // Outgoing path.
    //

#ifdef CLIENT_DLL
    // Not in demo mode, write to real outgoing folder
    if (!GetApplication(this)->GetDemoMode())
    {
        typedef BOOL (APIENTRY *FN_Server_GetMaxPath)(CHAR **ppPath);
        FN_Server_GetMaxPath _fn_Server_GetMaxPath;

        ::HMODULE handle = ::GetModuleHandle(NULL); 
        _fn_Server_GetMaxPath = (FN_Server_GetMaxPath) ::GetProcAddress(handle, "Server_GetOutgoingMaxPath");

        FX_ASSERT(_fn_Server_GetMaxPath != NULL);

        _fn_Server_GetMaxPath(&path);
        _outgoingPath = AllocString(path);
    }
#endif

    if (_outgoingPath == NULL)
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
    path = NULL;

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
    path = NULL;

    //
    // Create the directories.
    //

    if (!FxCreateDirectory(_applicationPath))
    {
        Log("Cannot create ApplicationPath %s", _applicationPath);
        goto cleanup;
    }

    if (!FxCreateDirectory(_mediaPath))
    {
        Log("Cannot create MediaPath %s", _mediaPath);
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

    result = TRUE;

cleanup:

    MEM_FREE(path);

    return result;
}

VOID CctSession::FreePaths()
{
#ifdef CLIENT_DLL
    // CLIENT_DLL: Cleanup outgoing and media in demo mode
    if (GetApplication(this)->GetDemoMode())
    {
        if (_outgoingPath)
        {
            ClearDirectory(_outgoingPath);
            FxDeleteDirectory(_outgoingPath);
        }

        if (_mediaPath)
        {
            ClearDirectory(_mediaPath);
            FxDeleteDirectory(_mediaPath);
        }
    }

    // CLIENT_DLL: Always okay to cleanup temp path
    if (_tempPath)
    {
        ClearDirectory(_tempPath);
        FxDeleteDirectory(_tempPath);
    }

    // CLIENT_DLL: Always okay to cleanup application path
    if (_applicationPath)
    {
        ClearDirectory(_applicationPath);
        FxDeleteDirectory(_applicationPath);
    }
#endif

    MEM_FREE(_tempPath);
    _tempPath = NULL;

    MEM_FREE(_outgoingPath);
    _outgoingPath = NULL;

    MEM_FREE(_mediaPath);
    _mediaPath = NULL;

    MEM_FREE(_applicationPath);
    _applicationPath = NULL;

    MEM_FREE(_sdPath);
    _sdPath = NULL;
}

VOID CctSession::InitSightingDatabase(BOOL *pCreated)
{
    BOOL created = FALSE;

    CfxDatabase *database = GetSightingDatabase();
    Fault(database->Verify(), "Could not create Sighting Database");
    if (!_faulted && !ValidateSightingHeader(database))
    {
        DBID newId = INVALID_DBID;
        created = TRUE;

        ResetDataUniqueness();

        // Setup header
        FXDATABASEHEADER header;
        header.Magic = MAGIC_SESSION;
        GetHost(this)->GetDeviceId(&header.DeviceId);
        memcpy(&header.FileName, &GetResourceHeader()->ServerFileName, sizeof(header.FileName));

        // Load the existing first sighting (if it exists)
        VOID *buffer = NULL;
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
            Fault(FALSE, "Error writing sighting header: pass 1");
        }

        // If there was already an entry: write it back
        if (buffer)
        {
            dbId = INVALID_DBID;
            if (!database->Write(&dbId, buffer, MEM_SIZE(buffer)))
            {
                Fault(FALSE, "Error writing sighting header: pass 2");
            }
            MEM_FREE(buffer);
        }
    }

    FreeSightingDatabase(database);
    database = NULL;

    if (pCreated)
    {
        *pCreated = created;
    }
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

BOOL CctSession::MatchProfileExact(CTRESOURCEHEADER *pHeader, UINT Width, UINT Height, UINT DPI, FXPROFILE *pMatch)
{
    BOOL result = FALSE;

    for (UINT i = 0; (i < pHeader->ProfileCount) && !result; i++)
    {
        FXPROFILE *p = _resource->GetProfile(this, i);

        DOUBLE dh, ddpi;
        dh = (DOUBLE)FxAbs((INT)p->Height - (INT)Height) / (DOUBLE)Height;
        ddpi = (DPI != 0) ? (DOUBLE)FxAbs((INT)p->DPI - (INT)DPI) / (DOUBLE)DPI : 0.0;

        if ((p->Width == Width) && (dh < 0.1) && (ddpi < 0.1))
        {
            *pMatch = *p;
            result = TRUE;
        }

        _resource->ReleaseProfile(p);
    }

    return result;
}

BOOL CctSession::MatchProfileFuzzy(CTRESOURCEHEADER *pHeader, UINT Width, UINT Height, UINT DPI, FXPROFILE *pMatch)
{
    UINT delta = 10000;
    UINT multiplier, t;

    for (UINT i = 0; i < pHeader->ProfileCount; i++)
    {
        FXPROFILE *p = _resource->GetProfile(this, i);
        DOUBLE ddpi = (DPI != 0) ? (DOUBLE)FxAbs((INT)p->DPI - (INT)DPI) / (DOUBLE)DPI : 0.0;

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
        t = FxAbs((INT)Height - ((INT)p->Height * (INT)multiplier));
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

    return ((DOUBLE)delta / (DOUBLE)Height) < 0.15;
}

BOOL CctSession::Connect(CHAR *pName, UINT StateSlot, XGUID FirstScreenId)
{
    Disconnect();

    CfxDatabase *database = NULL;
    CfxEngine *engine = GetEngine(this);
    CTRESOURCEHEADER *header = NULL;
    CfxHost *host = GetHost(this);

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
    Fault(header != NULL, "Cannot read Application header");

    Fault(header->Version >= 510, "Version Error - Requires at least 3.510");
    if (_faulted) goto Done;

    if (host->GetUseResourceScale())
    {
        FXPROFILE *profile = host->GetProfile();
        FXPROFILE profileMatch = {0};
        UINT w = profile->Width;
        UINT h = profile->Height;
        UINT dpi = profile->DPI;//((profile->DPI + 49) / 100) * 100;

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
    #ifndef CLIENT_DLL
    if (GetResourceHeader()->TestTime)
    {
        FXDATETIME currentTime;
        host->GetDateTime(&currentTime);
        DOUBLE time1 = EncodeDateTime(&currentTime); 
        DOUBLE time2 = EncodeDateTime(&_resource->GetHeader(this)->StampTime);
        DOUBLE tdiff = EncodeTime(0, 10, 0, 0); // 10 minutes

/*        Fault((time1 > time2) || (fabs(time1 - time2) < tdiff), "Time is wrong");

        if (_faulted) goto Done;*/
    }
    #endif

    // Initialize the sighting database
    BOOL created;
    InitSightingDatabase(&created);
    if (_faulted) goto Done;

    // Tell the application about the kiosk mode
    GetApplication(this)->SetKioskMode(header->KioskMode);

    // Test write the state database
    database = GetStateDatabase();

    // Reset the state database if the sighting database was just created
    if (created && header->ResetStateOnSync)
    {
        database->DeleteAll();
    }
    Fault(database->Verify(), "Could not create State Database");
    FreeStateDatabase(database);
    database = NULL;

    if (_faulted) goto Done;

    // Start the waypoint Manager
    _waypointManager.Connect();

    // Init the default state from the header
    _sightingAccuracy = header->SightingAccuracy;
    _sightingFixCount = max(1, header->SightingFixCount);
    _waypointManager.SetTimeout(header->WaypointTimer);
    _waypointManager.SetAccuracy(&header->WaypointAccuracy);
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
    #ifndef CLIENT_DLL
        _firstScreenId = _resource->GetHeader(this)->FirstObject;
    #endif
    _sighting.Init(_firstScreenId);

    // Connect the range finder
    SetRangeFinderConnected(_rangeFinderConnected);

    if (_faulted) goto Done;

    Fault(InitPaths(), "Init paths");

    SD_Initialize();

    // Configure the transfer manager
    _transferManager.SetAutoSendTimeout(header->AutoSendTimeout);
    _transferManager.Update();

    // Connect if nothing bad happened
    _connected = !_faulted;

    if (_connected)
    {
        _updateManager.Connect();
    }

Done:
    if (!_connected)
    {
        // Clean up
        Disconnect();
    }

    return _connected;
}

VOID CctSession::Disconnect()
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

    // Disconnect the waypoint manager
    _waypointManager.Disconnect();

    // Disconnect the transfer manager
    SetCredentials(NULL, NULL);
    _transferManager.Disconnect();

    // Update Manager
    _updateManager.Disconnect();

    // Disconnect the range finder
    SetRangeFinderConnected(FALSE);

    // Clear the store
    ClearStore();

    // Finalize the SD backup.
    SD_Finalize();

    _connected = FALSE;
    _stateSlot = 0;
    InitXGuid(&_firstScreenId);

    delete _resource;
    _resource = 0;

    delete _sightingDB;
    _sightingDB = NULL;

    delete _stateDB;
    _stateDB = NULL;

    MEM_FREE(_pendingGotoTitle);
    _pendingGotoTitle = NULL;

    MEM_FREE(_sightingAccuracyName);
    _sightingAccuracyName = NULL;

    FreePaths();

    _name[0] = '\0';
}

VOID CctSession::Reconnect(DBID SightingId, BOOL DoRepaint)
{
    FX_ASSERT(_connected);

    if (_dialogStates.GetCount() <= 1)
    {
        ClearDialogs();

        if ((SightingId != INVALID_DBID) && (SightingId != _sightingId))
        {
            _sightingId = SightingId;
            CfxDatabase *database = GetSightingDatabase();
            _sighting.Load(database, SightingId);
            FreeSightingDatabase(database);
            database = NULL;
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
        Fault((dialog != NULL), "Attempt to load invalid Dialog");
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
        dialogState->StateData = NULL;

        return;
    }
}

VOID CctSession::Run()
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

        // Ask for a password if supported and required
        if (!GetApplication(this)->GetDemoMode() &&
            GetHost(this)->IsEsriSupported() &&
            (GetResourceHeader()->SendData.Protocol == FXSENDDATA_PROTOCOL_ESRI) &&
            ((_userName == NULL) || (_password == NULL) || (_userName[0] == '\0') || (_password[0] == '\0')))
        {
            Password(this);
        }
    }

    Repaint();
}

VOID CctSession::Fault(BOOL Condition, CHAR *pError) 
{ 
    if (Condition) return;

    // We're in a bad state now
    _faulted = TRUE;

    // Disable the screen
    GetWindow()->SetEnabled(FALSE);

    // Force an align in case this happened on startup and we don't yet have a valid size
    GetEngine(this)->AlignControls(GetApplication(this)->GetScreen());

    // Setup fault window
    UINT h = (GetHost(this)->GetProfile()->ScaleY * 40) / 100;
    GetMessageWindow()->SetBounds(_left - 1, _top + (_height - h) / 2, _width + 2, h);
    GetMessageWindow()->SetVisible(TRUE);

    sprintf(_messageText, "Press to %s: %s.", _connected ? "Reset" : "Exit", pError);

    // Repaint to show the fault message
    Repaint();
}

VOID CctSession::ShowMessage(const CHAR *pMessage, COLOR BackColor)
{ 
    GetHost(this)->ShowToast(pMessage);
}

VOID CctSession::Confirm(CfxControl *pControl, CHAR *pTitle, CHAR *pMessage)
{
    CONFIRM_PARAMS params;
    params.ControlId = pControl->GetId();
    params.Title = pTitle;
    params.Message = pMessage;
    
    ShowDialog(&GUID_DIALOG_CONFIRM, NULL, &params, sizeof(CONFIRM_PARAMS)); 
}

VOID CctSession::SetCredentials(CHAR *pUserName, CHAR *pPassword)
{
    MEM_FREE(_userName);
    _userName = NULL;
    if (pUserName)
    {
        _userName = AllocString(pUserName);
    }

    MEM_FREE(_password);
    _password = NULL;

    if (pPassword)
    {
        _password = AllocString(pPassword);
    }
}

BOOL CctSession::StartLogin(CHAR *pUserName, CHAR *pPassword)
{
    SetCredentials(NULL, NULL);
    SaveState();
    return GetHost(this)->TestEsriCredentials(pUserName, pPassword);
}

VOID CctSession::Password(CfxControl *pControl)
{
    PASSWORD_PARAMS params;
	memset(&params, 0, sizeof(params));
    params.ControlId = pControl->GetId();

    if (_userName)
    {
        strcpy(params.Email, _userName);
    }

    if (_password)
    {
        strcpy(params.Password, _password);
    }
    
    ShowDialog(&GUID_DIALOG_PASSWORD, NULL, &params, sizeof(PASSWORD_PARAMS)); 
}

VOID CctSession::OnMessageClick(CfxControl *pSender, VOID *pParams)
{
    // If we never connected then we cannot reset, because things aren't going to get better
    if (!_connected)
    {
        GetEngine(this)->Shutdown();
        return;
    }

    // Enable the screen
    GetWindow()->SetEnabled(TRUE);

    // Hide the fault window
    GetMessageWindow()->SetVisible(FALSE);

    // Put us back in a good state
    if (_faulted)
    {
        _faulted = FALSE;

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

VOID CctSession::OnMessagePaint(CfxControl *pSender, VOID *pParams)
{
    FXPAINTDATA *paintData = (FXPAINTDATA *)pParams; 
    CfxCanvas *canvas = paintData->Canvas;

    canvas->State.BrushColor = _faulted ? 0x000080 : _messageBackColor;
    canvas->FillRect(paintData->Rect);

    canvas->SystemDrawText(_messageText, paintData->Rect, 0xFFFFFF, canvas->State.BrushColor);
}

VOID CctSession::OnShowMenu(CfxPersistent *pSender, VOID *pParams)
{
    if (!IsEditing())
    {
        ShowDialog(&GUID_DIALOG_SIGHTINGEDITOR, this, NULL, 0);
    }
}

VOID CctSession::SetRangeFinderConnected(BOOL Value)
{
    if (_rangeFinderLastState != Value) 
    {
        _rangeFinderLastState = Value;
        if (Value)
        {
            GetEngine(this)->LockRangeFinder();
        }
        else
        {
            GetEngine(this)->UnlockRangeFinder();
        }
    }

    _rangeFinderConnected = Value;
}

VOID CctSession::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
}

VOID CctSession::DefineState(CfxFiler &F)
{
    // Define header magic number
    UINT magic = MAGIC_SESSION;
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
        CHAR ServerFileName[256];
    	F.DefineValue("ServerFileName", dtText, &ServerFileName);
    }

    // Active sighting Id
    F.DefineValue("SightingId", dtInt32, &_sightingId);

    // Stamp GUID so we stay in sync with the resource file
    _resourceStampId = header->StampId;
    F.DefineValue("StampId", dtGuid,  &_resourceStampId);

    // Bail out if the resource stamp is out of sync
    if (F.IsReader() && !CompareGuid(&_resource->GetHeader(this)->StampId, &_resourceStampId)) return;
    
    //
    // We can change any of this at will, because the above check will always be in sync with the
    // resource file - which is versioned.
    //
    _stateActions.DefineProperties(F);
    _stateActions.DefineState(F);
    
    // Track the waypoint manager state
    _waypointManager.DefineState(F);
    F.DefineValue("SightingAccuracyDOP", dtDouble, &_sightingAccuracy.DilutionOfPrecision);
    F.DefineValue("SightingAccuracyMaxSpeed", dtDouble, &_sightingAccuracy.MaximumSpeed);
    F.DefineValue("SightingFixCount", dtInt32, &_sightingFixCount);

    // Track the range finder state
    F.DefineValue("RangeFinderConnected", dtBoolean, &_rangeFinderConnected);

    // Track default save targets
    F.DefineXOBJECT("DefaultSave1Target", &_defaultSave1Target);
    F.DefineXOBJECT("DefaultSave2Target", &_defaultSave2Target);

    _sighting.DefineProperties(F);

    // Validate the sightingId
    if (_sightingId != INVALID_DBID)
    {
        CfxDatabase *database = GetSightingDatabase();
        if (database->Verify() && database->TestId(_sightingId))
        {
            _backupSighting.DefineProperties(F);
        }
        else
        {
            _sightingId = INVALID_DBID;
        }
        FreeSightingDatabase(database);
        database = NULL;
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
            storeItem.Data = NULL;
            F.DefineValue("Data", dtPBinary, &storeItem.Data);
            _store.Add(storeItem);
        }
    }
    else
    {
        UINT i;
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

    // Uniqueness number for sightings and waypoints
    F.DefineValue("DataUniqueness", dtInt32, &_dataUniqueness);

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
    DOUBLE gpsTimeOffset = GetHost(this)->GetGPSTimeOffset();
    F.DefineValue("GpsTimeOffset", dtDouble, &gpsTimeOffset);
    if (F.IsReader())
    {
        GetHost(this)->SetGPSTimeOffset(gpsTimeOffset);
    }

    // Share data support
    // Hack because we did not change the resource version number.
    auto reachedEnd = FALSE;
    if (F.IsReader())
    {
        if (static_cast<CfxReader&>(F).IsEnd())
        {
            _shareDataEnabled = FALSE;
            return;
        }
    }

    F.DefineValue("ShareDataEnabled", dtBoolean, &_shareDataEnabled);
}

VOID CctSession::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    CfxControl::DefinePropertiesUI(F);
#endif
}

VOID CctSession::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    _sighting.DefineResources(F);
    _actions.DefineResources(F);
#endif
}

VOID CctSession::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    if (_screenRight)
    {
        INT w = (Width - 4) / 2;
        GetWindow()->SetBounds(0, 0, w, Height);
        _screenRight->SetBounds(w + 4, 0, w, _height);
    }
    else
    {
        GetWindow()->SetBounds(0, 0, Width, Height);
    }
}

VOID CctSession::LoadState()
{
    VOID *buffer = NULL;
    DBID databaseId = INVALID_DBID;

    CfxDatabase *database = GetStateDatabase();
    Fault(database->Verify(), "Could not create Database");
    if (_faulted) goto Done;
    if (database->Enum(0, &databaseId) == FALSE) goto Done;

    if (database->Read(databaseId, &buffer) && buffer && (MEM_SIZE(buffer) > 0))
    {
        CfxStream stream(buffer, MEM_SIZE(buffer));
        CfxReader reader(&stream);
        DefineState(reader);

        // Check to see if something went out of sync
        if (!CompareGuid(&_resource->GetHeader(this)->StampId, &_resourceStampId))
        {
            // We have a different resource file, so we have to reset
            CfxDatabase *sightingDatabase = GetSightingDatabase();
            
            // First record is a header which we don't care about
            if (sightingDatabase->GetCount() > 1)
            {
                Fault(FALSE, "Sightings not downloaded");
            }
            else
            {
                sightingDatabase->DeleteAll();
            }
            FreeSightingDatabase(sightingDatabase);
            sightingDatabase = NULL;

            if (!_faulted)
            {
                InitSightingDatabase();

                _stateActions.Clear();
                _sighting.Clear();
                _sighting.Init(_firstScreenId);
            }
        }
    }
    MEM_FREE(buffer);

Done:

    FreeStateDatabase(database);
    database = NULL;
}

VOID CctSession::ForceTitleBar(CfxScreen *pScreen)
{
    CfxEngine *engine = GetEngine(this);
    engine->AlignControls(pScreen);

    CfxControl_Panel *topPanel = NULL;

    CfxControl *topControl = engine->HitTest(pScreen, 0, 0);
    if (topControl && 
        (topControl->GetDock() == dkTop) &&
        (topControl->GetHeight() < (60 * GetHost(this)->GetProfile()->ScaleY) / 100) &&
        CompareGuid(topControl->GetTypeId(), &GUID_CONTROL_PANEL))
    {
        topPanel = (CfxControl_Panel *)topControl;
    }

    CfxControl_TitleBar *titleBar = new CfxControl_TitleBar(GetOwner(), pScreen);
    titleBar->SetDock(dkTop);
    titleBar->SetBounds(0, 0, 100, titleBar->GetHeight());
    titleBar->SetShowBattery(FALSE);
    titleBar->SetShowTime(FALSE);
    titleBar->SetShowMenu(TRUE);
    titleBar->AssignFont(pScreen->GetFont());
    titleBar->SetShowBattery(TRUE);
    titleBar->SetShowTime(TRUE);
    titleBar->SetShowExit(!GetApplication(this)->GetKioskMode());
        
    if (topPanel)
    {
        titleBar->SetCaption(topPanel->GetVisibleCaption());
        titleBar->SetBorderStyle(bsNone);
        titleBar->SetColor(topPanel->GetColor());
        titleBar->SetTextColor(topPanel->GetTextColor());

        topPanel->SetBounds(0, titleBar->GetHeight(), 100, 0);
        topPanel->SetCaption(NULL);
    }
    else
    {
        if (pScreen->GetName())
        {
            titleBar->SetUseScreenName(TRUE);
        }
        else
        {
            titleBar->SetCaption("Client");
        }
    }
}

VOID CctSession::LoadScreen(CfxScreen *pScreen, XGUID ScreenId, VOID *pState)
{
    CfxHost *host = GetApplication(this)->GetHost();

    Fault(host->GetFreeMemory() > 65536 * 2, "Memory Low");

    // Load the screen resource
    FXSCREENDATA *screenData = (FXSCREENDATA *)_resource->GetObject(this, ScreenId);
    Fault((screenData != NULL), "Attempt to load invalid screen");
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

    // Restart the waypoint manager timers
    _waypointManager.Restart();

    // Restart the range finder state
    SetRangeFinderConnected(_rangeFinderConnected);
}

VOID CctSession::LoadCurrentScreen()
{
    LoadScreen(GetWindow(), *_sighting.GetCurrentScreenId(), NULL);
}

VOID CctSession::LoadCurrentScreenAndState()
{
    XGUID screenId = *_sighting.GetCurrentScreenId();
    VOID *screenData = _sighting.GetScreenData(screenId, _sighting.GetPathIndex());
    LoadScreen(GetWindow(), screenId, screenData);
}

VOID CctSession::SaveState()
{
    DBID dbId = INVALID_DBID;

    CfxStream stream;
    CfxWriter writer(&stream);
    
    CfxDatabase *database = GetStateDatabase();
    Fault(database->Verify(), "Could not create Database");
    if (_faulted) goto Done;
    Fault(!database->Full(), "Database Full");
    if (_faulted) goto Done;

    // Write out the session state 
    DefineState(writer);

    // Save to the database
    database->DeleteAll();
    database->Write(&dbId, stream.GetMemory(), stream.GetSize());

Done:

    FreeStateDatabase(database);
    database = NULL;
}

VOID CctSession::SaveScreen(CfxScreen *pScreen, XGUID ScreenId, UINT PathIndex)
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

VOID CctSession::SaveCurrentScreen()
{
    if (_sighting.GetPathIndex() < 0) return;

    SaveScreen(GetWindow(), *_sighting.GetCurrentScreenId(), _sighting.GetPathIndex());
}

VOID CctSession::RegisterEvent(SESSIONEVENT Event, CfxControl *pControl, NotifyMethod Method)
{
    FXEVENT event = {Event, pControl, Method};
    _events.Register(&event);
}

VOID CctSession::UnregisterEvent(SESSIONEVENT Event, CfxControl *pControl)
{
    _events.Unregister(Event, pControl);
}

VOID CctSession::RegisterRetainState(CfxControl *pControl, CctRetainState *pRetainState)
{
    RETAINSTATE retainState = { pControl, pRetainState };
    _retainStates.Add(retainState);
}

VOID CctSession::UnregisterRetainState(CfxControl *pControl)
{
    for (UINT i = 0; i < _retainStates.GetCount(); i++)
    {
        if (_retainStates.GetPtr(i)->Control == pControl)
        {
            _retainStates.Delete(i);
            break;
        }
    }
}

VOID CctSession::ProcessElementEffects()
{
    ATTRIBUTE *attribute = NULL;
    CfxResource *resource = GetResource();

    MEM_FREE(_sightingAccuracyName);
    _sightingAccuracyName = NULL;

    for (UINT i = 0; _sighting.EnumAttributes(i, &attribute); i++)
    {
        FXTRAVELMODE *travelMode = NULL;
        FXTEXTRESOURCE *travelModeName = NULL;

        if (!IsNullXGuid(&attribute->ElementId))
        {
            travelMode = (FXTRAVELMODE *)resource->GetObject(this, attribute->ElementId, eaTravelMode);
            if (travelMode != NULL)
            {
                travelModeName = (FXTEXTRESOURCE *)resource->GetObject(this, attribute->ElementId, eaName);
            }
        }

        if (!IsNullXGuid(&attribute->ValueId))
        {
            if (travelMode == NULL)
            {
                travelMode = (FXTRAVELMODE *)resource->GetObject(this, attribute->ValueId, eaTravelMode);
                if (travelMode != NULL)
                {
                    travelModeName = (FXTEXTRESOURCE *)resource->GetObject(this, attribute->ValueId, eaName);
                }
            }
        }

        if (travelMode != NULL)
        {
            FX_ASSERT(travelModeName->Magic == MAGIC_TEXT);
            FX_ASSERT(travelMode->Magic == MAGIC_TRAVELMODE);

            MEM_FREE(_sightingAccuracyName);
            _sightingAccuracyName = NULL;
            _sightingAccuracyName = AllocString(travelModeName->Text);

            SetSightingAccuracy(&travelMode->SightingAccuracy);
            _waypointManager.SetAccuracy(&travelMode->WaypointAccuracy);

            resource->ReleaseObject(travelModeName);
            resource->ReleaseObject(travelMode);
        }
    }
}

VOID CctSession::EnterScreen()
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

VOID CctSession::LeaveScreen()
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

BOOL CctSession::CanDoBack()
{
    if (_faulted || (_sighting.BOP())) return FALSE;

    BOOL canBack = TRUE;
    _events.ExecuteType(seCanBack, this, &canBack);

    return canBack;
}   

VOID CctSession::DoBack()
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

BOOL CctSession::CanDoNext()
{
    if (_faulted) return FALSE;

    BOOL canNext = TRUE;
    _events.ExecuteType(seCanNext, this, &canNext);

    return canNext;
}   

VOID CctSession::DoNext()
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
            Fault(FALSE, "No Next Screen");
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
    BOOL looped = _sighting.FixPathLoop();

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

VOID CctSession::DoSkipToScreen(XGUID TargetScreenId)
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

VOID CctSession::DoJumpToScreen(XGUID BaseScreenId, XGUID JumpScreenId)
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
    _events.ExecuteType(seSetJumpTarget, this, (VOID *)&JumpScreenId);

    // Controls should be set up so that Next will go to the jump target.
    if (CanDoNext())
    {
        // Advance
        DoNext();
    }
    else
    {
        Fault(FALSE, "Jump screen not a Link");
    }
}

VOID CctSession::DoCommit(XGUID TargetScreenId, BOOL DoRepaint)
{
    if (!CanDoNext()) return;

    // Shut down the screen
    LeaveScreen();

    // Save the actions and the screen state
    _saving = TRUE;

    SaveCurrentScreen();

    // Save the last GPS position if exists and we're saving a new sighting
    if ((_sightingId == INVALID_DBID) &&
        (_sighting.GetGPS()->Quality != fqNone))
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
            _pendingGotoTitle = NULL;
        }
    }

    // Ensure the sighting database is valid
    InitSightingDatabase();

    // Save the current sighting
    CfxDatabase *database = GetSightingDatabase();
    Fault(_sighting.Save(database, &_sightingId), "Failed to save");
    FreeSightingDatabase(database);
    database = NULL;

    ArchiveSighting(&_sighting);
    SD_WriteSighting(&_sighting);

    // Update the data uniqueness
    _dataUniqueness++;

    //
    // Save a waypoint at this location if tracks are being recorded.
    //
    if (_waypointManager.GetAlive())
    {
        _waypointManager.StoreRecordFromSighting(&_sighting);
    }

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
    INT targetIndex;

    if (IsNullXGuid(&TargetScreenId))
    {
        targetIndex = 0;
    }
    else
    {
        INT i;
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
        while ((INT)_sighting.GetPathIndex() >= targetIndex)
        {
            _actions.Rollback(this);
            _actions.Clear();
            if ((INT)_sighting.GetPathIndex() == targetIndex) break;
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
    _saving = FALSE;

    if (DoRepaint)
    {
        Repaint();
    }

    _transferManager.Update();
    _updateManager.OnSessionCommit();
}

VOID CctSession::DoCommitInplace()
{
    if (!CanDoNext()) return;

    // Shut down the screen
    LeaveScreen();

    // Ensure the sighting database is valid
    InitSightingDatabase();

    // Save the current sighting
    CfxDatabase *database = GetSightingDatabase();
    Fault(_sighting.Save(database, &_sightingId), "Failed to save");
    FreeSightingDatabase(database);
    database = NULL;

    ArchiveSighting(&_sighting);
    SD_WriteSighting(&_sighting);

    // Update the data uniqueness
    _dataUniqueness++;

    //
    // Save a waypoint at this location if tracks are being recorded and
    // we are using the alarm timer
    //
    if (_waypointManager.GetAlive() && 
        (_waypointManager.GetTimeout() > WAYPOINT_ALARM_THRESHOLD))
    {
        _waypointManager.StoreRecordFromSighting(&_sighting);
    }

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
    _saving = FALSE;

    _transferManager.Update();
}

VOID CctSession::DoStartEdit(DBID SightingId)
{
    FX_ASSERT(_connected);
    FX_ASSERT(SightingId != INVALID_DBID);
    FX_ASSERT(SightingId != _sightingId);

    ClearDialogs();

    _backupSighting.Assign(&_sighting);

    _sightingId = SightingId;

    CfxDatabase *database = GetSightingDatabase();
    _sighting.Load(database, SightingId);
    FreeSightingDatabase(database);
    database = NULL;

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

VOID CctSession::DoAcceptEdit()
{
    if (!CanDoNext()) return;

    // Shut down the screen
    LeaveScreen();

    // Save the actions and the screen state
    SaveCurrentScreen();

    // Ensure the sighting database is valid
    InitSightingDatabase();

    // Save the sighting
    CfxDatabase *database = GetSightingDatabase();
    Fault(_sighting.Save(database, &_sightingId), "Failed to save");
    _transferManager.RemoveSentHistoryId(".BAK", _sighting.GetUniqueId()); // Allow backup of this sighting
    _transferManager.RemoveSentHistoryId(".SNT", _sighting.GetUniqueId()); // Allow resend of this sighting
    _sightingId = INVALID_DBID;
    FreeSightingDatabase(database);
    database = NULL;

    ArchiveSighting(&_sighting);
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

VOID CctSession::DoCancelEdit()
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

VOID CctSession::DoHome(XGUID TargetScreenId)
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

VOID CctSession::ClearDialogs()
{
    UINT i;
    for (i = 0; i < _dialogStates.GetCount(); i++)
    {
        MEM_FREE(_dialogStates.GetPtr(i)->StateData);
    }
    _dialogStates.Clear();
}

VOID CctSession::ShowDialog(const GUID *pDialogId, CfxControl *pSender, VOID *pParam, UINT ParamSize)
{
    VOID *dialogParam = NULL;
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
        FX_ASSERT(dialogState->StateData == NULL);

        CfxStream stream;
        CfxWriter writer(&stream);

        GetWindow()->DefineState(writer);
        dialogState->StateData = stream.CloneMemory();
    }

    // Create a new dialog and add it to the stack.
    {
        DIALOGSTATE dialogState;
        dialogState.Id = *pDialogId;
        dialogState.StateData = NULL;
        _dialogStates.Insert(0, dialogState);
    }

    if (!_faulted)
    {
        FXDIALOG *dialog = (FXDIALOG *)_resource->GetDialog(this, pDialogId);
        Fault((dialog != NULL), "Attempt to load invalid Dialog");
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
                editor->Init(pSender == NULL ? this : pSender, dialogParam, ParamSize);
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

UINT CctSession::InsertScreen(XGUID ScreenId)
{
    return _sighting.InsertScreen(ScreenId);
}

UINT CctSession::AppendScreen(XGUID ScreenId)
{
    if (_sighting.GetPathCount() == 0)
    {
        _firstScreenId = ScreenId;
    }

    return _sighting.AppendScreen(ScreenId);
}

VOID CctSession::DeleteScreen(UINT Index)
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

UINT CctSession::GetSightingCount()
{
    CfxDatabase *database = GetSightingDatabase();
    UINT count = database->GetCount();
    FreeSightingDatabase(database);
    database = NULL;

    return count;
}

UINT CctSession::MergeAttributes(UINT ControlId, ATTRIBUTES *pNewAttributes, ATTRIBUTES *pOldAttributes)
{
    // Pick up the full element guid
    CfxResource *resource = GetResource();
    for (UINT i=0; i<pNewAttributes->GetCount(); i++)
    {
        ATTRIBUTE *attribute = pNewAttributes->GetPtr(i);

        #ifdef CLIENT_DLL
            attribute->ElementGuid = attribute->ElementId;
        #else
            FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(this, attribute->ElementId, eaName);
            if (element)
            {
                attribute->ElementGuid = element->Guid;
                resource->ReleaseObject(element);
            }
        #endif
    }

    // The sighting will now perform the merge
    return _sighting.MergeAttributes(ControlId, pNewAttributes, pOldAttributes);
}

VOID CctSession::ExecuteAction(CfxAction *pAction)
{
    pAction->SetSourceId(_sighting.GetCurrentScreenId());
    _actions.AddCloneAndExecute(pAction, this);
}

VOID CctSession::RestartWaypointTimer()
{
    if (!_connected) return;

    GetEngine(this)->SetLastUserEventTime(FxGetTickCount());
    _waypointManager.Restart();
    Repaint();
}

BOOL CctSession::StoreWaypoint(WAYPOINT *pWaypoint, BOOL CheckTime)
{
    //
    // Make sure we don't write the same waypoint datetime out for the same device
    // This is static so that multiple sessions don't write out the same point at the same time
    //
    static CHAR *__globalValueName = "CT_TOTAL_DISTANCE";

    // Reset the global.
    if (GetGlobalValue(__globalValueName) == NULL)
    {
        SetGlobalValue(NULL, __globalValueName, 0);
    }
    
    if (CheckTime && (pWaypoint->DateCurrent == _lastWaypoint.DateCurrent))
    {
        return FALSE;
    }

    // Ensure the sighting database is valid: this is needed, because the 
    // database header stores information about the target database.
    InitSightingDatabase();

    FX_ASSERT(_name && (strlen(_name) > 0));

    CHAR name[MAX_PATH];
    strcpy(name, _name);
    strcat(name, ".WAY");

    _dataUniqueness++;

    GetHost(this)->AppendRecord(name, pWaypoint, sizeof(WAYPOINT));

    ArchiveWaypoint(pWaypoint);
    SD_WriteWaypoint(pWaypoint);

    // Check for timer track break.
    if (!IsNullGuid(&pWaypoint->Id))
    {
        // If less than 15 seconds have passed
        DOUBLE distance;
        DOUBLE speed = GetEngine(this)->GetGPS()->Position.Speed;   // m/s
        DOUBLE timeDeltaInMs = (pWaypoint->DateCurrent - _lastWaypoint.DateCurrent) * 86400000;
        if (timeDeltaInMs < 5000)
        {
            distance =  (speed * timeDeltaInMs) / 1000; // meters
        }
        else
        {
            distance = _lastWaypoint.DateCurrent == 0 ? 0 : CalcDistanceKm(_lastWaypoint.Latitude, _lastWaypoint.Longitude, pWaypoint->Latitude, pWaypoint->Longitude) * 1000;
        }

        SetGlobalValue(NULL, __globalValueName, GetGlobalValue(__globalValueName)->Value + distance);
        _lastWaypoint = *pWaypoint;
        SaveState();
    }

    _events.ExecuteType(seNewWaypoint, this, pWaypoint);

    _transferManager.Update();

    return TRUE;
}

VOID CctSession::EnumWaypoints(CfxControl *pControl, EnumWaypointCallback Callback)
{
    CfxHost *host = GetHost(this);

    FX_ASSERT(_name && (strlen(_name) > 0));

    CHAR name[MAX_PATH];
    strcpy(name, _name);
    strcat(name, ".WAY");

    if (host->EnumRecordInit(name))
    {
        WAYPOINT waypoint;
        while (host->EnumRecordNext(&waypoint, sizeof(WAYPOINT)))
        {
            (pControl->*Callback)(&waypoint);
        }
        host->EnumRecordDone();
    }
}

BOOL CctSession::EnumHistoryInit(HISTORY_ITEM **ppHistoryItemDef)
{
    FX_ASSERT(_name && (strlen(_name) > 0));
    FX_ASSERT(_historyItemDef == NULL);
    FX_ASSERT(_historyItem == NULL);

    CfxHost *host = GetHost(this);

    if (ppHistoryItemDef != NULL)
    {
        *ppHistoryItemDef = NULL;
    }
    _historyItemDef = _historyItemDef = NULL;
    _historyItemCount = 0;
    _historyItemIndex = 0;
    _historyItemSize = 0;
    
    CHAR name[MAX_PATH];
    strcpy(name, _name);
    strcat(name, ".HST");

    // Read the header item and get the count and size.
    HISTORY_ITEM historyItemFirst = {0};
    UINT result = 0;

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
        if (_historyItemDef == NULL) return FALSE;
        _historyItem = (HISTORY_ITEM *)MEM_ALLOC(_historyItemSize);
        if (_historyItem == NULL) return FALSE;

        if (host->EnumRecordInit(name))
        {
            if (host->EnumRecordNext(_historyItemDef, _historyItemSize))
            {
                _historyItemIndex = 1;
                if (ppHistoryItemDef != NULL)
                {
                    *ppHistoryItemDef = _historyItemDef;
                }
                return TRUE;
            }

            host->EnumRecordDone();
        }
    }

    return FALSE;
}

BOOL CctSession::EnumHistorySetIndex(UINT Index)
{
    if (Index >= _historyItemCount) return FALSE;
    _historyItemIndex = Index + 1;
    return GetHost(this)->EnumRecordSetIndex(_historyItemIndex, _historyItemSize);
}

BOOL CctSession::EnumHistoryNext(HISTORY_ITEM **ppHistoryItem)
{
    *ppHistoryItem = _historyItem;

    if ((_historyItemCount == 0) || (_historyItemSize == 0)) return FALSE;
    if (_historyItem == NULL) return FALSE;
    if (_historyItemIndex > _historyItemCount) return FALSE;

    BOOL result = GetHost(this)->EnumRecordNext(_historyItem, _historyItemSize);
    
    if (result) 
    {
        _historyItemIndex++;
    }

    return result;
}

VOID CctSession::EnumHistoryDone()
{
    GetHost(this)->EnumRecordDone();

    MEM_FREE(_historyItem);
    _historyItem = NULL;

    MEM_FREE(_historyItemDef);
    _historyItemDef = NULL;
    
    _historyItemCount = 0;
    _historyItemIndex = 0;
    _historyItemSize = 0;
}

VOID CctSession::EnumHistoryWaypoints(CfxControl *pControl, EnumHistoryWaypointCallback Callback)
{
    CfxHost *host = GetHost(this);
    HISTORY_ITEM *historyItemDef;
    HISTORY_WAYPOINT historyWaypoint;
    UINT historyItemSize;

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

HISTORY_ROW *CctSession::GetHistoryItemRow(HISTORY_ITEM *pHistoryItem, UINT Index)
{
    FX_ASSERT(Index < pHistoryItem->ColumnCount);
    HISTORY_ROW *row = (HISTORY_ROW *)(pHistoryItem + 1);
    return row + Index;
}

//*************************************************************************************************
// Store functions

VOID CctSession::ClearStore()
{
    for (UINT i=0; i<_store.GetCount(); i++)
    {
        MEM_FREE(_store.GetPtr(i)->Data);
    }
    _store.Clear();
}

STOREITEM *CctSession::GetStoreItem(GUID PrimaryId, UINT ControlId)
{
    for (UINT i=0; i<_store.GetCount(); i++)
    {
        STOREITEM *item = _store.GetPtr(i);
        if (CompareGuid(&PrimaryId, &item->PrimaryId) && (ControlId == item->ControlId))
        {
            return item;
        }
    }
    
    return NULL;
}

STOREITEM *CctSession::SetStoreItem(GUID PrimaryId, UINT ControlId, VOID *pData)
{
    for (UINT i=0; i<_store.GetCount(); i++)
    {
        STOREITEM *item = _store.GetPtr(i);
        if (CompareGuid(&PrimaryId, &item->PrimaryId) && (ControlId == item->ControlId))
        {
            MEM_FREE(item->Data);
            item->Data = NULL;
            _store.Delete(i);
            break;
        }
    }
    
    if (pData != NULL)
    {
        STOREITEM item;
        item.PrimaryId = PrimaryId;
        item.ControlId = ControlId;
        item.Data = pData;
        return _store.GetPtr(_store.Add(item));
    }
    else
    {
        return NULL;
    }
}

VOID CctSession::SaveControlState(CfxControl *pControl, BOOL Flush)
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

VOID CctSession::LoadControlState(CfxControl *pControl)
{
    STOREITEM *item = GetStoreItem(ConstructGuid(*GetCurrentScreenId()), pControl->GetId());
    if (item)
    {
        CfxStream stream(item->Data);
        CfxReader reader(&stream);
        pControl->DefineState(reader);
    }
}

VOID CctSession::LoadControlStates()
{
    if (IsEditing()) return;

    for (UINT i = 0; i < _retainStates.GetCount(); i++)
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

VOID CctSession::SaveControlStates()
{
    if (IsEditing()) return;

    for (UINT i = 0; i < _retainStates.GetCount(); i++)
    {
        RETAINSTATE *retainState = _retainStates.GetPtr(i);
            
        if (retainState->RetainState->MustRetain())
        {
            SaveControlState(retainState->Control, FALSE);
        }
    }
}

//*************************************************************************************************
// Global value functions

VOID CctSession::ClearGlobalValues()
{
    _globalValues.Clear();
}

GLOBALVALUE *CctSession::EnumGlobalValues(UINT Index)
{
    if (Index >= _globalValues.GetCount())
    {
        return NULL;
    }
    else
    {
        return _globalValues.GetPtr(Index);
    }
}

GLOBALVALUE *CctSession::GetGlobalValue(const CHAR *pName)
{
    UINT i = 0;
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

    return NULL;
}

VOID CctSession::SetGlobalValue(CfxPersistent *pSender, const CHAR *pName, DOUBLE Value)
{
    if ((pName == NULL) || (*pName == '\0')) return;

    DOUBLE oldValue = 0;

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
        eventParams.pSender = (pSender == NULL) ? this : pSender;
        eventParams.Name = pName;
        eventParams.OldValue = oldValue;
        eventParams.NewValue = Value;

        CfxEngine *engine = GetEngine(this);
        engine->BlockPaint();
        _events.ExecuteType(seGlobalChanged, this, &eventParams);
        engine->UnblockPaint();
    }
}

CHAR *CctSession::GetUpdateTag()
{
    CHAR *result = NULL;
    CHAR *workPath = AllocMaxPath(GetApplicationPath());
    AppendSlash(workPath);
    strcat(workPath, _name);
    strcat(workPath, ".TAG");

    FXFILEMAP fileMap = {0};
    if (FxMapFile(workPath, &fileMap))
    {
        result = AllocString((CHAR *)fileMap.BasePointer);
        FxUnmapFile(&fileMap);
    }

    MEM_FREE(workPath);
    workPath = NULL;
       
    return result;
}

CHAR *CctSession::GetUpdateUrl()
{
    CHAR *result = NULL;
    CHAR *workPath = AllocMaxPath(GetApplicationPath());
    AppendSlash(workPath);
    strcat(workPath, _name);
    strcat(workPath, ".DEF");

    FXFILEMAP fileMap = {0};
    if (FxMapFile(workPath, &fileMap))
    {
        result = AllocString((CHAR *)fileMap.BasePointer);
        FxUnmapFile(&fileMap);
    }
    else
    {
        result = AllocString(GetResourceHeader()->WebUpdateUrl);
    }

    MEM_FREE(workPath);
    workPath = NULL;
       
    return result;
}

//*************************************************************************************************
// Utility functions

BOOL RegisterSessionEvent(SESSIONEVENT Event, CfxControl *pControl, NotifyMethod Method)
{
    CctSession *session = GetSession(pControl);
    if (session)
    {
        session->RegisterEvent(Event, pControl, Method);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL UnregisterSessionEvent(SESSIONEVENT Event, CfxControl *pControl)
{
    CctSession *session = GetSession(pControl);
    if (session)
    {
        session->UnregisterEvent(Event, pControl);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL RegisterRetainState(CfxControl *pControl, CctRetainState *pRetainState)
{
    CctSession *session = GetSession(pControl);
    if (session)
    {
        session->RegisterRetainState(pControl, pRetainState);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL UnregisterRetainState(CfxControl *pControl)
{
    CctSession *session = GetSession(pControl);
    if (session)
    {
        session->UnregisterRetainState(pControl);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
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

BOOL IsSessionEditing(CfxControl *pControl)
{
    CctSession *session = GetSession(pControl);
    return session && session->IsEditing();
}

#ifdef CLIENT_DLL
VOID HackFixResultElementLockProperties(CHAR **ppLockProperties)
{
    //StringReplace(ppLockProperties, "Result ", "");
    if ((*ppLockProperties != NULL) && 
        (strstr(*ppLockProperties, "Result Element") == NULL) &&
        (strstr(*ppLockProperties, "Element") != NULL))
    {
        StringReplace(ppLockProperties, "Element", "Result Element");
    }
}
#endif
