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

#include "fxTypes.h"
#include "fxUtils.h"
#include "fxApplication.h"
#include "fxProjection.h"
#include "ctSighting.h"
#include "ctSession.h"
#include "ctElement.h"

//*************************************************************************************************
// Attribute helper functions

// Find the first attribute with the specified ScreenId and ControlId
INT FindFirstAttribute(ATTRIBUTES *pAttributes, XGUID ScreenId, UINT ControlId)
{
    ATTRIBUTE *attribute = pAttributes->First();
    UINT count = pAttributes->GetCount();
    for (UINT i=0; i<count; i++, attribute++)
    {
        if ((attribute->ScreenId == ScreenId) && 
            (attribute->ControlId == ControlId))
        {
            return i;
        }
    }

    // Not found
    return -1;
}

// Clear the attributes and their variants
VOID ClearAttributes(ATTRIBUTES *pAttributes)
{
    ATTRIBUTE *attribute = pAttributes->First();
    UINT count = pAttributes->GetCount();
    for (UINT i=0; i<count; i++, attribute++)
    {
        Type_FreeVariant(&attribute->Value);
    }
    pAttributes->Clear();
}

VOID CopyAttributes(ATTRIBUTES *pAttributes, XGUID ScreenId, UINT ControlId, ATTRIBUTES *pOutAttributes)
{
    ClearAttributes(pOutAttributes);

    ATTRIBUTE *attribute = pAttributes->First();
    UINT count = pAttributes->GetCount();
    for (UINT i=0; i<count; i++, attribute++)
    {
        if (CompareXGuid(&attribute->ScreenId, &ScreenId) &&
            (attribute->ControlId == ControlId))
        {
            ATTRIBUTE outAttribute = *attribute;
            outAttribute.Value = Type_DuplicateVariant(attribute->Value);
            pOutAttributes->Add(outAttribute);
        }
    }
}

VOID MergeAttributeRange(ATTRIBUTES *pDstAttributes, ATTRIBUTES *pSrcAttributes, UINT Index1, UINT Index2)
{
    FX_ASSERT(Index1 < pSrcAttributes->GetCount());
    FX_ASSERT(Index2 < pSrcAttributes->GetCount());

    for (UINT i=Index1; i<=Index2; i++)
    {
        ATTRIBUTE *srcAttribute = pSrcAttributes->GetPtr(i);
        BOOL found = FALSE;

        for (UINT j=0; j<pDstAttributes->GetCount(); j++)
        {
            ATTRIBUTE *dstAttribute = pDstAttributes->GetPtr(j);

            // Check for merge.
            if (CompareXGuid(&srcAttribute->ScreenId, &dstAttribute->ScreenId) &&
                (srcAttribute->ControlId == dstAttribute->ControlId) &&
                (CompareXGuid(&srcAttribute->ElementId, &dstAttribute->ElementId)))
            {
                found = TRUE;
                Type_FreeVariant(&dstAttribute->Value);
                *dstAttribute = *srcAttribute;
                dstAttribute->Value = Type_DuplicateVariant(srcAttribute->Value);
                break;
            }
        }

        if (!found)
        {
            ATTRIBUTE newAttribute = *srcAttribute;
            newAttribute.Value = Type_DuplicateVariant(srcAttribute->Value);
            pDstAttributes->Add(newAttribute);
        }
    }
}

// Remove attributes with the specified ScreenId and ControlId
VOID RemoveAttributes(ATTRIBUTES *pAttributes, XGUID ScreenId, UINT ControlId)
{
    ATTRIBUTES newAttributes;

    ATTRIBUTE *attribute = pAttributes->First();
    UINT count = pAttributes->GetCount();
    for (UINT i=0; i<count; i++, attribute++)
    {
        if (CompareXGuid(&attribute->ScreenId, &ScreenId) &&
            ((ControlId == 0) || (attribute->ControlId == ControlId)))
        {
            Type_FreeVariant(&attribute->Value);
        }
        else
        {
            newAttributes.Add(*attribute);
        }
    }

    pAttributes->Assign(&newAttributes);
}

VOID RemoveAttributesFromIndex(ATTRIBUTES *pAttributes, UINT IndexToStart)
{
    if (IndexToStart >= pAttributes->GetCount()) return;

    ATTRIBUTES newAttributes;
    ATTRIBUTE *attribute = pAttributes->First();
    for (UINT i=0; i<IndexToStart; i++, attribute++)
    {
        newAttributes.Add(*attribute);
    }

    pAttributes->Assign(&newAttributes);
}

VOID DefineAttributes(CfxFiler &F, ATTRIBUTES *pAttributes)
{
    if (F.IsReader())
    {
        ClearAttributes(pAttributes);
        while (!F.ListEnd())
        {
            ATTRIBUTE attribute = {0};
            F.DefineXOBJECT("ScreenId",   &attribute.ScreenId);
            F.DefineValue("ControlId",    dtInt32, &attribute.ControlId);
            F.DefineXOBJECT("ElementId",  &attribute.ElementId);
            F.DefineXOBJECT("ValueId",    &attribute.ValueId);
            F.DefineValue("ElementGuid",  dtGuid, &attribute.ElementGuid);
            F.DefineVariant("Value",      &attribute.Value);
            pAttributes->Add(attribute);
        }
    }
    else
    {
        for (UINT i=0; i<pAttributes->GetCount(); i++)
        {
            ATTRIBUTE attribute = pAttributes->Get(i);
            
            F.DefineXOBJECT("ScreenId",   &attribute.ScreenId);
            F.DefineValue("ControlId",    dtInt32, &attribute.ControlId);
            F.DefineXOBJECT("ElementId",  &attribute.ElementId);
            F.DefineXOBJECT("ValueId",    &attribute.ValueId);
            F.DefineValue("ElementGuid",  dtGuid, &attribute.ElementGuid);
            F.DefineVariant("Value",      &attribute.Value);
        }
        F.ListEnd();
    }
}


VOID GetAttributeText(CfxControl *pControl, ATTRIBUTE *pAttribute, CfxString &Text)
{
    // Look for non empty ValueId and decode the guid name
    if (!IsNullXGuid(&pAttribute->ValueId))
    {
        CfxResource *resource = pControl->GetResource();
        FXTEXTRESOURCE *valueName = (FXTEXTRESOURCE *)resource->GetObject(pControl, pAttribute->ValueId, eaName);
        if (valueName)
        {
            Text.Set(valueName->Text);
            resource->ReleaseObject(valueName);
        }
    }
    // Just a regular type
    else
    {
        Type_VariantToText(pAttribute->Value, Text);

        FXDATATYPE dataType = GetTypeFromString(Text.Get());

        switch (dataType) 
        {
        case dtSound:
            Text.Set("[Sound]");
            break;
        case dtGraphic:
            Text.Set("[Picture]");
            break;
        }
    }
}

//************************************************************************************************/

CctSighting::CctSighting(CfxPersistent *pOwner): CfxPersistent(pOwner), _path(), _oldState(), _attributes()
{
    Clear();
}

CctSighting::~CctSighting()
{
    Clear();
}

VOID CctSighting::Clear()
{
    Reset();
    ClearGPS();
    ClearPath();
    ClearOldState();
    ClearAttributes();
}
VOID CctSighting::Reset()
{
    InitGuid(&_uniqueId);
    if (GetHost(this) != NULL)
    {
        GetHost(this)->CreateGuid(&_uniqueId);
    }
    memset(&_dateTime, 0, sizeof(_dateTime));
    memset(&_gps, 0, sizeof(_gps));
}

VOID CctSighting::ClearPath()
{
    UINT i;

    for (i=0; i<_path.GetCount(); i++)
    {
        MEM_FREE(_path.Get(i).Data);
    }
    _path.Clear();
    _pathIndex = 0;
}

VOID CctSighting::ClearAttributes()
{
    ::ClearAttributes(&_attributes);
}

VOID CctSighting::RemoveAttributesFromIndex(UINT Index)
{
    ::RemoveAttributesFromIndex(&_attributes, Index);
}

VOID CctSighting::ClearGPS()
{
    memset(&_gps, 0, sizeof(_gps));
}

VOID CctSighting::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    UINT i;
    for (i=0; i<_path.GetCount(); i++)
    {
        F.DefineObject(_path.Get(i).ScreenId);
    }
    for (i=0; i<_oldState.GetCount(); i++)
    {
        F.DefineObject(_oldState.Get(i).ScreenId);
    }
#endif
}

VOID CctSighting::DefineProperties(CfxFiler &F)
{
    PATHITEM pathItem;
	UINT i;

    // Magic number
    UINT magic = MAGIC_SIGHTING3;
    F.DefineValue("Magic", dtInt32, &magic);
    FX_ASSERT(magic == MAGIC_SIGHTING3);

    F.DefineValue("SightingId",  dtGuid,   &_uniqueId);
    F.DefineDateTime(&_dateTime);

    F.DefineValue("Quality",   dtByte,   &_gps.Quality);
    F.DefineValue("Latitude",  dtDouble, &_gps.Latitude);
    F.DefineValue("Longitude", dtDouble, &_gps.Longitude);
    F.DefineValue("Altitude",  dtDouble, &_gps.Altitude);
    F.DefineValue("Accuracy",  dtDouble, &_gps.Accuracy);

    DefineAttributes(F, &_attributes);

    if (F.IsReader())
    {
        ClearPath();
        F.DefineValue("PathIndex", dtInt32, &_pathIndex);
        while (!F.ListEnd())
        {
            F.DefineXOBJECT("ScreenId", &pathItem.ScreenId);
            pathItem.Data = NULL;
            F.DefineValue("State", dtPBinary, &pathItem.Data);
            _path.Add(pathItem);
        }

        ClearOldState();
        while (!F.ListEnd())
        {
            F.DefineXOBJECT("ScreenId", &pathItem.ScreenId);
            pathItem.Data = NULL;
            F.DefineValue("State", dtPBinary, &pathItem.Data);
            _oldState.Add(pathItem);
        }
    }
    else
    {
        F.DefineValue("PathIndex", dtInt32, &_pathIndex);
        for (i=0; i<_path.GetCount(); i++)
        {
            pathItem = _path.Get(i);
            F.DefineXOBJECT("ScreenId", &pathItem.ScreenId);
            F.DefineValue("State", dtPBinary, &pathItem.Data);
        }
        F.ListEnd();
        for (i=0; i<_oldState.GetCount(); i++)
        {
            pathItem = _oldState.Get(i);
            F.DefineXOBJECT("ScreenId", &pathItem.ScreenId);
            F.DefineValue("State", dtPBinary, &pathItem.Data);
        }
        F.ListEnd();
    }
}

VOID CctSighting::Init(XGUID FirstScreenId)
{
    if ((_path.GetCount() == 0) && !IsNullXGuid(&FirstScreenId))
    {
        AppendScreen(FirstScreenId);
    }
}

BOOL CctSighting::Load(CfxDatabase *pDatabase, DBID Id)
{
    FX_ASSERT(Id != INVALID_DBID);

    Clear();

    VOID *buffer = NULL;
    if (pDatabase->Read(Id, &buffer) && buffer)
    {
        CfxStream stream(buffer, MEM_SIZE(buffer));
        CfxReader reader(&stream);
        DefineProperties(reader);
        MEM_FREE(buffer);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL CctSighting::Save(CfxDatabase *pDatabase, DBID *pId)
{
    CfxHost *host = GetHost(this);

    if (_dateTime.Year == 0)
    {
        host->GetDateTime(&_dateTime);
    }
    FX_ASSERT(_dateTime.Year != 0);

    if (IsNullGuid(&_uniqueId))
    {
        host->CreateGuid(&_uniqueId);
    }

    ClearOldState();

    CfxStream stream;
    CfxWriter writer(&stream);
    DefineProperties(writer);

    BOOL result = pDatabase->Write(pId, stream.GetMemory(), stream.GetSize());

    host->RequestMediaScan(pDatabase->GetFileName());

    return result;
}

//*************************************************************************************************
// Screen APIs

VOID *CctSighting::GetScreenData(XGUID ScreenId, UINT Index)
{
    PATHITEM pathItem;
    memset(&pathItem, 0, sizeof(pathItem));
    while (Index > _path.GetCount())
    {
        _path.Add(pathItem);
    }

    pathItem = _path.Get(Index);

    if (!CompareXGuid(&ScreenId, &pathItem.ScreenId))
    {
        MEM_FREE(pathItem.Data);
        pathItem.Data = NULL;
        _path.Put(Index, pathItem);
    }

    return pathItem.Data;
}

VOID CctSighting::SetScreenData(XGUID ScreenId, UINT Index, VOID *pData)
{
    PATHITEM pathItem;
    memset(&pathItem, 0, sizeof(pathItem));
    while (Index > _path.GetCount())
    {
        _path.Add(pathItem);
    }

    pathItem = _path.Get(Index);
    pathItem.ScreenId = ScreenId;
    MEM_FREE(pathItem.Data);
    pathItem.Data = pData;

    _path.Put(Index, pathItem);
}

VOID CctSighting::ClearOldState()
{
    UINT i;

    for (i=0; i<_oldState.GetCount(); i++)
    {
        MEM_FREE(_oldState.GetPtr(i)->Data);
    }
    _oldState.Clear();
}

BOOL CctSighting::ExtractOldState(XGUID ScreenId, PATHITEM *pPathItem)
{
    BOOL result = FALSE;
    UINT i;

    for (i = 0; i < _oldState.GetCount(); i++)
    {
        PATHITEM *pathItem = _oldState.GetPtr(i);
        if (CompareXGuid(&ScreenId, &pathItem->ScreenId))
        {
            *pPathItem = *pathItem;
            _oldState.Delete(i);
            result = TRUE;
            break;
        }
    }

    return result;
}

UINT CctSighting::InsertScreen(XGUID ScreenId)
{
    PATHITEM pathItem;
    pathItem.ScreenId = ScreenId;
    pathItem.Data = NULL;
    ExtractOldState(ScreenId, &pathItem);
    return _path.Insert(_pathIndex + 1, pathItem);
}

UINT CctSighting::AppendScreen(XGUID ScreenId)
{
    PATHITEM pathItem;
    pathItem.ScreenId = ScreenId;
    pathItem.Data = NULL;
    ExtractOldState(ScreenId, &pathItem);
    return _path.Add(pathItem);
}

VOID CctSighting::DeleteScreen(UINT Index)
{
    PATHITEM pathItem, tempPathItem;
    pathItem = _path.Get(Index);

    // Free duplicate item from old path
    if (ExtractOldState(pathItem.ScreenId, &tempPathItem))
    {
        MEM_FREE(tempPathItem.Data);
    }

    // Add the item to the old list
    _oldState.Add(pathItem);

    // Remove from the current path
    _path.Delete(Index);
}

BOOL CctSighting::FixPathLoop()
{
    if (_pathIndex == 0) return FALSE;

    XGUID currentScreenId = _path.GetPtr(_pathIndex)->ScreenId;

    for (UINT i=0; i<_pathIndex-1; i++)
    {
        if (CompareXGuid(&currentScreenId, &_path.GetPtr(i)->ScreenId))
        {
            // Free subsequent screens
            for (UINT j=i+1; j<_path.GetCount(); j++)
            {
                MEM_FREE(_path.GetPtr(j)->Data);
            }
            
            // Clip the path down
            _path.SetCount(i + 1);
            _pathIndex = i;

            //
            // Clear the attributes in the sighting that are from the current screen
            // Note we can't roll them back in the session because of the AppendScreen action that will 
            // now fail. We could get around this by rolling back all non-merge actions on that screen
            // first, but it doesn't seem much worse to simply ignore them altogether, since they 
            // will restart anyway.
            //
            RemoveAttributes(&_attributes, currentScreenId, 0);
            
            // Done
            return TRUE;
        }
    }

    return FALSE;
}

//*************************************************************************************************
// Attribute APIs

BOOL CctSighting::EnumAttributes(UINT Index, ATTRIBUTE **ppAttribute)
{
    if (Index < _attributes.GetCount())
    {
        *ppAttribute = _attributes.GetPtr(Index);
        return TRUE;
    }
    else
    {
        *ppAttribute = NULL;
        return FALSE;
    }
}

XGUID *CctSighting::MatchAttribute(XGUIDLIST *pElements)
{
    for (UINT i=0; i<pElements->GetCount(); i++)
    {
        XGUID id = pElements->Get(i);

        for (UINT j=0; j<_attributes.GetCount(); j++)
        {
            ATTRIBUTE *attribute = _attributes.GetPtr(j);
            if (CompareXGuid(&id, &attribute->ElementId))
            {
                return &attribute->ElementId;
            }

            if (CompareXGuid(&id, &attribute->ValueId))
            {
                return &attribute->ValueId;
            }
        }
    }

    return NULL;
}

ATTRIBUTE *CctSighting::FindAttribute(XGUID *pElementId)
{
    for (UINT i=0; i<_attributes.GetCount(); i++)
    {
        ATTRIBUTE *attribute = _attributes.GetPtr(i);
        if (CompareXGuid(pElementId, &attribute->ElementId))
        {
            return attribute;
        }
    }

    return NULL;
}

UINT CctSighting::MergeAttributes(UINT ControlId, ATTRIBUTES *pNewAttributes, ATTRIBUTES *pOldAttributes)
{
    XGUID screenId = _path.GetPtr(_pathIndex)->ScreenId;

    // Find the first instance of the control id attributes - this will be returned
    INT firstIndex = FindFirstAttribute(&_attributes, screenId, ControlId);

    // Back up old attributes
    if (pOldAttributes)
    {
        CopyAttributes(&_attributes, screenId, ControlId, pOldAttributes);
    }

    // Remove the old attributes
    RemoveAttributes(&_attributes, screenId, ControlId);

    // Setup the first index if it isn't set yet
    if (firstIndex == -1)
    {
        firstIndex = _attributes.GetCount();
    }

    // Insert the attributes
    for (INT i=pNewAttributes->GetCount()-1; i>=0; i--)
    {
        ATTRIBUTE attribute = pNewAttributes->Get(i);

        // Remove blank strings
        if ((attribute.Value->Type == dtPText) || (attribute.Value->Type == dtText))
        {
            CfxString valueString;
            Type_VariantToText(attribute.Value, valueString);

            if (valueString.Length() == 0)
            {
                continue;
            }
        }

        attribute.ScreenId = screenId;
        attribute.Value = Type_DuplicateVariant(attribute.Value);
        _attributes.Insert(firstIndex, attribute);
    }

    // Return the index 
    return firstIndex;
}

VOID WriteDoubleAttribute(const GUID *pGuid, FXDATATYPE DataType, DOUBLE d, CfxFileStream &Stream)
{
    Stream.Write((VOID *)pGuid, sizeof(GUID));

    FXVARIANT *tempVariant;
    tempVariant = Type_CreateVariant(DataType, &d);
    Type_VariantToStream(tempVariant, Stream);
    Type_FreeVariant(&tempVariant);
}

BOOL WriteMediaAttribute(CctSession *pSession, FXVARIANT *pValue, CfxFileStream &Stream, FXFILELIST *pMediaFileList)
{
    if ((pValue->Type != dtText) || (pValue->Size < strlen(MEDIA_PREFIX)))
    {
        return FALSE;
    }

    FXDATATYPE dataType = GetTypeFromString((CHAR *)&pValue->Data[0]);
    if ((dataType != dtSound) && (dataType != dtGraphic))
    {
        return FALSE;
    }

    BOOL result = FALSE;
    CHAR *mediaName = (CHAR *)(&pValue->Data[0]) + strlen(MEDIA_PREFIX);
    CHAR *fileName = NULL;
    FXFILEMAP fileMap = {0};

    fileName = pSession->AllocMediaFileName(mediaName);
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

    result = TRUE;
    if (pMediaFileList != NULL)
    {
        FXFILE file = {0};
        file.Name = AllocString(mediaName);
        pMediaFileList->Add(file);
    }

    WriteType(Stream, dataType, fileMap.FileSize);

    if (!Stream.Write(fileMap.BasePointer, fileMap.FileSize))
    {
        Log("Failed to copy file to stream: %s", fileName);
        goto cleanup;
    }

    result = TRUE;

cleanup:

    FxUnmapFile(&fileMap);

    MEM_FREE(fileName);

    return result;
}

BOOL CctSighting::BuildExport(CfxControl *pSessionControl, CfxFileStream &Stream, FXFILELIST *pMediaFileList)
{
    CctSession *session = GetSession(pSessionControl);

    // Clear the stream
    //Stream.Clear();

    // Write the header
    SIGHTING_EXPORT_HEADER header = {0};
    header.Magic = 'SIGT';
    GetHost(this)->GetDeviceId(&header.DeviceId);
    header.DateCreated = (UINT)EncodeDateTime(&_dateTime);
    header.DateCurrent = header.DateCreated;
    header.Id = _uniqueId;

    UINT startPosition = Stream.GetPosition();
    Stream.Write(&header, sizeof(header));
    
    // Write date/time attributes
    WriteDoubleAttribute(&GUID_ELEMENT_DATE, dtDate, header.DateCreated, Stream);
    WriteDoubleAttribute(&GUID_ELEMENT_TIME, dtTime, EncodeDateTime(&_dateTime) - header.DateCreated, Stream);
    
    // Write GPS attributes
    if (_gps.Quality > fqNone)
    {
        WriteDoubleAttribute(&GUID_ELEMENT_LATITUDE, dtDouble, _gps.Latitude, Stream);
        WriteDoubleAttribute(&GUID_ELEMENT_LONGITUDE, dtDouble, _gps.Longitude, Stream);

        if (_gps.Quality >= fq3D)
        {
            WriteDoubleAttribute(&GUID_ELEMENT_ALTITUDE, dtDouble, _gps.Altitude, Stream);
        }

        if ((_gps.Accuracy > 0.0) && (_gps.Accuracy < 50.0))
        {
            WriteDoubleAttribute(&GUID_ELEMENT_ACCURACY, dtDouble, _gps.Accuracy, Stream);
        }
    }

    // Write all other attributes
    for (UINT i=0; i<_attributes.GetCount(); i++) 
    {
        ATTRIBUTE *attribute = _attributes.GetPtr(i);

        Stream.Write(&attribute->ElementGuid, sizeof(GUID));
        if (!WriteMediaAttribute(session, attribute->Value, Stream, pMediaFileList))
        {
            Type_VariantToStream(attribute->Value, Stream);
        }
    }

    UINT endPosition = Stream.GetPosition();

    // Fix the header
    header.AttributeSize = endPosition - startPosition - sizeof(header);

    Stream.Seek(startPosition);
    Stream.Write(&header, sizeof(header));
    Stream.Seek(endPosition);

    // Success
    return TRUE;
}

BOOL CctSighting::HasMedia()
{
   for (UINT i=0; i<_attributes.GetCount(); i++) 
   {
       if (_attributes.GetPtr(i)->IsMedia())
       {
           return TRUE;
       }
   }

   return FALSE;
}

//*************************************************************************************************
//
// CctSighting_Painter.
//
CctSighting_Painter::CctSighting_Painter(CfxPersistent *pOwner, CfxControl *pParent): _selectedFileName(), CfxListPainter(pOwner, pParent)
{
    _sighting = new CctSighting(pOwner);

    _selectedIndex = -1;

    _playButton = new CfxControl_Button(pOwner, pParent);
    _playButton->SetComposite(TRUE);
    _playButton->SetVisible(FALSE);
    _playButton->SetBorder(bsNone, 0, 0, 0);
    _playButton->SetOnClick(this, (NotifyMethod)&CctSighting_Painter::OnPlayClick);
    _playButton->SetOnPaint(this, (NotifyMethod)&CctSighting_Painter::OnPlayPaint);

    _stopButton = new CfxControl_Button(pOwner, pParent);
    _stopButton->SetComposite(TRUE);
    _stopButton->SetVisible(FALSE);
    _stopButton->SetBorder(bsNone, 0, 0, 0);
    _stopButton->SetOnClick(this, (NotifyMethod)&CctSighting_Painter::OnStopClick);
    _stopButton->SetOnPaint(this, (NotifyMethod)&CctSighting_Painter::OnStopPaint);
}

CctSighting_Painter::~CctSighting_Painter()
{
    delete _sighting;

    if (_parent && _parent->IsLive())
    {
        GetHost(_parent)->StopSound();
    }
}

VOID CctSighting_Painter::ConnectByIndex(UINT Index)
{
    CctSession *session = GetSession(_parent);

    DBID id = INVALID_DBID;
    CfxDatabase *database = session->GetSightingDatabase();

    database->Enum(max(1, Index), &id);

    // Connect and load the sighting
    if (id == INVALID_DBID)
    {
        _sighting->Assign(session->GetCurrentSighting());
    }
    else
    {
        _sighting->Load(database, id);
    }

    delete database;
}

VOID CctSighting_Painter::ConnectById(DBID Id)
{
    CctSession *session = GetSession(_parent);

    // Connect and load the sighting
    if (Id == INVALID_DBID)
    {
        _sighting->Assign(session->GetCurrentSighting());
    }
    else
    {
        CfxDatabase *database = session->GetSightingDatabase();
        _sighting->Load(database, Id);
        delete database;
    }

    SetSelectedIndex(-1); 
}

VOID CctSighting_Painter::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxListPainter::DefineResources(F);
    for (UINT i=0; i < _sighting->GetAttributes()->GetCount(); i++)
    {
        ATTRIBUTE *attribute = _sighting->GetAttributes()->GetPtr(i);
        F.DefineObject(attribute->ElementId, eaName);
        F.DefineObject(attribute->ElementId, eaIcon32);
        F.DefineObject(attribute->ValueId, eaName);
        F.DefineObject(attribute->ValueId, eaIcon32);
    }
#endif
}

UINT CctSighting_Painter::GetItemCount()
{
    UINT count = 1; // For time

    FXGPS_POSITION *gps = _sighting->GetGPS();
    switch (gps->Quality)
    {
    case fqUser2D:
    case fq2D:     count += 2; break; // Lat, Lon
    
    case fq3D: 
    case fqDiff3D:
    case fqSim3D:
    case fqUser3D: count += 3; break; // Lat, Lon, Alt
    }

    // Accuracy
    if ((gps->Quality != fqNone) && (gps->Accuracy > 0.0))
    {
        count += 1;
    } 

    return _sighting->GetAttributes()->GetCount() + count;
}

VOID CctSighting_Painter::OnPlayClick(CfxControl *pSender, VOID *pParams)
{
    FX_ASSERT(_selectedIndex != -1);

    CHAR *fileName = GetSession(_parent)->AllocMediaFileName(_selectedFileName.Get());
    GetHost(_parent)->PlayRecording(fileName);
    MEM_FREE(fileName);
}

VOID CctSighting_Painter::OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPlay(pParams->Rect, pParams->Canvas->InvertColor(_parent->GetColor()), pParams->Canvas->InvertColor(_parent->GetBorderColor()), _playButton->GetDown()); 
}

VOID CctSighting_Painter::OnStopClick(CfxControl *pSender, VOID *pParams)
{
    GetHost(_parent)->StopSound();
}

VOID CctSighting_Painter::OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawStop(pParams->Rect, pParams->Canvas->InvertColor(_parent->GetColor()), pParams->Canvas->InvertColor(_parent->GetBorderColor()), _stopButton->GetDown()); 
}

VOID CctSighting_Painter::PaintDOUBLE(CfxCanvas *pCanvas, FXRECT *pLRect, FXRECT *pRRect, FXFONTRESOURCE *pFont, CHAR *pCaption, DOUBLE Value, UINT16 Precision)
{
    pCanvas->DrawTextRect(pFont, pLRect, TEXT_ALIGN_VCENTER, pCaption);

    CHAR caption[32];
    PrintF(caption, Value, sizeof(caption)-1, Precision);
    pCanvas->DrawTextRect(pFont, pRRect, TEXT_ALIGN_VCENTER, caption);
}

VOID CctSighting_Painter::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index)
{
    UINT itemHeight = pRect->Bottom - pRect->Top + 1;
    UINT paintIndex = Index;

    // Left side rect
    FXRECT lRect = *pRect;
    lRect.Left += (2 * _parent->GetBorderLineWidth()) + itemHeight;
    lRect.Right = pRect->Left + (pRect->Right - pRect->Left + 1) / 2;

    // Right side rect
    FXRECT rRect = *pRect;
    rRect.Left = lRect.Right + 1;

    //
    // Show Date and Time
    //
    if (Index == 0)
    {
        pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, "Day-Time");

        CHAR caption[32];
        FXDATETIME *time = _sighting->GetDateTime();
        if (time->Year == 0)
        {
            strcpy(caption, TEXT_UNSAVED);
        }
        else
        {
            sprintf(caption, "%02d-%02d:%02d:%02d", time->Day, time->Hour, time->Minute, time->Second);
        }
        
        pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, caption);

        return;
    }
    Index--;

    //
    // Show GPS
    //

    // Show Latitude and Longitude
    FXGPS_POSITION *gps = _sighting->GetGPS();

    // Every fix must have Latitude and Longitude
    if (gps->Quality != fqNone)
    {
        CHAR captionLon[32], captionLat[32];
        CHAR *pTitleLon, *pTitleLat;
        BOOL invert;
        INT index;
        GetProjectedParams(&pTitleLon, &pTitleLat, &invert);
        index = Index ^ (INT)invert;

        GetProjectedStrings(gps->Longitude, gps->Latitude, captionLon, captionLat);

        switch (index)
        {
        case 0: 
            pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, pTitleLat);
            pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, captionLat);
            break;
                
        case 1: 
            pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, pTitleLon);
            pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, captionLon);
            break;
        }
        
        if (Index < 2) return;
        Index -= 2;
    }

    // Show Altitude
    if ((gps->Quality == fq3D) || (gps->Quality == fqDiff3D) || (gps->Quality == fqUser3D) || (gps->Quality == fqSim3D))
    {
        switch (Index)
        {
        case 0: PaintDOUBLE(pCanvas, &lRect, &rRect, pFont, "Altitude", gps->Altitude, 2);   break;
        }
        if (Index < 1) return;
        Index -= 1;
    }

    // Show Accuracy
    if ((gps->Quality != fqNone) && (gps->Accuracy > 0.0))
    {
        switch (Index)
        {
        case 0: PaintDOUBLE(pCanvas, &lRect, &rRect, pFont, "Accuracy", gps->Accuracy, 2);   break;
        }
        
        if (Index < 1) return;
        Index -= 1;
    }

    //
    // Draw Attributes
    //
    CfxResource *resource = _parent->GetResource();
    ATTRIBUTE *attribute = _sighting->GetAttributes()->GetPtr(Index);

    XGUID elementId = attribute->ElementId;

    FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(_parent, elementId, eaName);
    if (element)
    {
        lRect.Left -= itemHeight;

        FX_ASSERT(element && (element->Magic == MAGIC_TEXT));
        FXBITMAPRESOURCE *bitmap = (FXBITMAPRESOURCE *)resource->GetObject(_parent, elementId, eaIcon32);
        if (bitmap)
        {
            FX_ASSERT(bitmap->Magic == MAGIC_BITMAP);
            pCanvas->StretchDraw(bitmap, lRect.Left, lRect.Top + 1, itemHeight - 2, itemHeight - 2);
            resource->ReleaseObject(bitmap);
        }
        
        lRect.Left += itemHeight;
        pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, element->Text);

        // Look for non empty ValueId and decode the guid name
        if (!IsNullXGuid(&attribute->ValueId))
        {
            FXTEXTRESOURCE *value = (FXTEXTRESOURCE *)resource->GetObject(_parent, attribute->ValueId, eaName);
            if (value)
            {
                pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, value->Text);
                resource->ReleaseObject(value);
            }
        }
        // Just a regular type
        else
        { 
            CfxString valueText;
            Type_VariantToText(attribute->Value, valueText);
            FXDATATYPE dataType = GetTypeFromString(valueText.Get());

            if (dataType == dtSound)
            {
                if (_selectedIndex == (INT)paintIndex)
                {
                    // Move and display the buttons
                    FXRECT absoluteRect, r;
                    _parent->GetAbsoluteBounds(&absoluteRect);
                    r = *pRect;
                    INT tx = r.Right - absoluteRect.Left;
                    INT ty = r.Top - absoluteRect.Top;

                    // Stop button
                    r.Right -= itemHeight;
                    tx -= itemHeight;
                    _stopButton->SetVisible(TRUE);
                    _stopButton->SetBounds(tx + 1, ty, itemHeight, itemHeight);

                    // Play button
                    r.Right -= itemHeight;
                    tx -= itemHeight;
                    _playButton->SetVisible(TRUE);
                    _playButton->SetBounds(tx, ty, itemHeight, itemHeight);

                    _selectedFileName.Set(valueText.Get() + strlen(MEDIA_PREFIX)); 
                }
            }

            GetAttributeText(_parent, attribute, valueText);

            pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, valueText.Get());
        }

        resource->ReleaseObject(element);
    }
}

VOID CctSighting_Painter::SetSelectedIndex(INT Value) 
{ 
    _playButton->SetVisible(FALSE);
    _stopButton->SetVisible(FALSE);
    _selectedIndex = Value; 
    _parent->Repaint();
}