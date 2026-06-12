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
int FindFirstAttribute(ATTRIBUTES *pAttributes, XGUID ScreenId, int ControlId)
{
    ATTRIBUTE *attribute = pAttributes->First();
    int count = pAttributes->GetCount();
    for (int i=0; i<count; i++, attribute++)
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
void ClearAttributes(ATTRIBUTES *pAttributes)
{
    ATTRIBUTE *attribute = pAttributes->First();
    int count = pAttributes->GetCount();
    for (int i=0; i<count; i++, attribute++)
    {
        Type_FreeVariant(&attribute->Value);
    }
    pAttributes->Clear();
}

void CopyAttributes(ATTRIBUTES *pAttributes, XGUID ScreenId, int ControlId, ATTRIBUTES *pOutAttributes)
{
    ClearAttributes(pOutAttributes);

    ATTRIBUTE *attribute = pAttributes->First();
    int count = pAttributes->GetCount();
    for (int i=0; i<count; i++, attribute++)
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

void MergeAttributeRange(ATTRIBUTES *pDstAttributes, ATTRIBUTES *pSrcAttributes, int Index1, int Index2)
{
    FX_ASSERT(Index1 < pSrcAttributes->GetCount());
    FX_ASSERT(Index2 < pSrcAttributes->GetCount());

    for (int i=Index1; i<=Index2; i++)
    {
        ATTRIBUTE *srcAttribute = pSrcAttributes->GetPtr(i);
        bool found = false;

        for (int j=0; j<pDstAttributes->GetCount(); j++)
        {
            ATTRIBUTE *dstAttribute = pDstAttributes->GetPtr(j);

            // Check for merge.
            if (CompareXGuid(&srcAttribute->ScreenId, &dstAttribute->ScreenId) &&
                (srcAttribute->ControlId == dstAttribute->ControlId) &&
                (CompareXGuid(&srcAttribute->ElementId, &dstAttribute->ElementId)))
            {
                found = true;
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
void RemoveAttributes(ATTRIBUTES *pAttributes, XGUID ScreenId, int ControlId)
{
    ATTRIBUTES newAttributes;

    ATTRIBUTE *attribute = pAttributes->First();
    int count = pAttributes->GetCount();
    for (int i=0; i<count; i++, attribute++)
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

void RemoveAttributesFromIndex(ATTRIBUTES *pAttributes, int IndexToStart)
{
    if (IndexToStart >= pAttributes->GetCount()) return;

    ATTRIBUTES newAttributes;
    ATTRIBUTE *attribute = pAttributes->First();
    for (int i=0; i<IndexToStart; i++, attribute++)
    {
        newAttributes.Add(*attribute);
    }

    pAttributes->Assign(&newAttributes);
}

void DefineAttributes(CfxFiler &F, ATTRIBUTES *pAttributes)
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
        for (int i=0; i<pAttributes->GetCount(); i++)
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


void GetAttributeText(CfxControl *pControl, ATTRIBUTE *pAttribute, QString &Text)
{
    // Look for non empty ValueId and decode the guid name
    if (!IsNullXGuid(&pAttribute->ValueId))
    {
        CfxResource *resource = pControl->GetResource();
        FXTEXTRESOURCE *valueName = (FXTEXTRESOURCE *)resource->GetObject(pControl, pAttribute->ValueId, (BYTE)ElementAttribute::Name);
        if (valueName)
        {
            Text = QString::fromUtf8(&valueName->Text[0]);
            resource->ReleaseObject(valueName);
        }
    }
    // Just a regular type
    else
    {
        Type_VariantToText(pAttribute->Value, Text);

        FXDATATYPE dataType = GetTypeFromString(Text.toUtf8().constData());

        switch (dataType)
        {
        case dtSound:
            Text = QStringLiteral("[Sound]");
            break;
        case dtGraphic:
            Text = QStringLiteral("[Picture]");
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

void CctSighting::Clear()
{
    Reset();
    ClearGPS();
    ClearPath();
    ClearOldState();
    ClearAttributes();
}
void CctSighting::Reset()
{
    InitGuid(&_uniqueId);
    if (GetHost(this) != nullptr)
    {
        GetHost(this)->CreateGuid(&_uniqueId);
    }
    memset(&_dateTime, 0, sizeof(_dateTime));
    memset(&_gps, 0, sizeof(_gps));
}

void CctSighting::ClearPath()
{
    int i;

    for (i=0; i<_path.GetCount(); i++)
    {
        MEM_FREE(_path.Get(i).Data);
    }
    _path.Clear();
    _pathIndex = 0;
}

void CctSighting::ClearAttributes()
{
    ::ClearAttributes(&_attributes);
}

void CctSighting::RemoveAttributesFromIndex(int Index)
{
    ::RemoveAttributesFromIndex(&_attributes, Index);
}

void CctSighting::ClearGPS()
{
    memset(&_gps, 0, sizeof(_gps));
}

void CctSighting::DefineResources(CfxFilerResource &F)
{
}

void CctSighting::DefineProperties(CfxFiler &F)
{
    PATHITEM pathItem;
	int i;

    // Magic number
    int magic = MAGIC_SIGHTING3;
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
            pathItem.Data = nullptr;
            F.DefineValue("State", dtPBinary, &pathItem.Data);
            _path.Add(pathItem);
        }

        ClearOldState();
        while (!F.ListEnd())
        {
            F.DefineXOBJECT("ScreenId", &pathItem.ScreenId);
            pathItem.Data = nullptr;
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

void CctSighting::Init(XGUID FirstScreenId)
{
    if ((_path.GetCount() == 0) && !IsNullXGuid(&FirstScreenId))
    {
        AppendScreen(FirstScreenId);
    }
}

bool CctSighting::Load(CfxDatabase *pDatabase, DBID Id)
{
    FX_ASSERT(Id != INVALID_DBID);

    Clear();

    void *buffer = nullptr;
    if (pDatabase->Read(Id, &buffer) && buffer)
    {
        CfxStream stream(buffer, MEM_SIZE(buffer));
        CfxReader reader(&stream);
        DefineProperties(reader);
        MEM_FREE(buffer);
        return true;
    }
    else
    {
        return false;
    }
}

bool CctSighting::LoadBlob(const QByteArray &In)
{
    Clear();

    if (In.isEmpty())
    {
        return false;
    }

    CfxStream stream((void *)In.constData(), In.size());
    CfxReader reader(&stream);
    DefineProperties(reader);

    return true;
}

void CctSighting::SaveBlob(QByteArray &Out)
{
    CfxHost *host = GetHost(this);

    if (_dateTime.Year == 0)
    {
        host->GetDateTime(&_dateTime);
    }

    if (IsNullGuid(&_uniqueId))
    {
        host->CreateGuid(&_uniqueId);
    }

    ClearOldState();

    CfxStream stream;
    CfxWriter writer(&stream);
    DefineProperties(writer);

    Out = QByteArray((const char *)stream.GetMemory(), stream.GetSize());
}

bool CctSighting::Save(CfxDatabase *pDatabase, DBID *pId)
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

    bool result = pDatabase->Write(pId, stream.GetMemory(), stream.GetSize());

    host->RequestMediaScan(pDatabase->GetFileName());

    return result;
}

//*************************************************************************************************
// Screen APIs

void *CctSighting::GetScreenData(XGUID ScreenId, int Index)
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
        pathItem.Data = nullptr;
        _path.Put(Index, pathItem);
    }

    return pathItem.Data;
}

void CctSighting::SetScreenData(XGUID ScreenId, int Index, void *pData)
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

void CctSighting::ClearOldState()
{
    int i;

    for (i=0; i<_oldState.GetCount(); i++)
    {
        MEM_FREE(_oldState.GetPtr(i)->Data);
    }
    _oldState.Clear();
}

bool CctSighting::ExtractOldState(XGUID ScreenId, PATHITEM *pPathItem)
{
    bool result = false;
    int i;

    for (i = 0; i < _oldState.GetCount(); i++)
    {
        PATHITEM *pathItem = _oldState.GetPtr(i);
        if (CompareXGuid(&ScreenId, &pathItem->ScreenId))
        {
            *pPathItem = *pathItem;
            _oldState.Delete(i);
            result = true;
            break;
        }
    }

    return result;
}

int CctSighting::InsertScreen(XGUID ScreenId)
{
    PATHITEM pathItem;
    pathItem.ScreenId = ScreenId;
    pathItem.Data = nullptr;
    ExtractOldState(ScreenId, &pathItem);
    return _path.Insert(_pathIndex + 1, pathItem);
}

int CctSighting::AppendScreen(XGUID ScreenId)
{
    PATHITEM pathItem;
    pathItem.ScreenId = ScreenId;
    pathItem.Data = nullptr;
    ExtractOldState(ScreenId, &pathItem);
    return _path.Add(pathItem);
}

void CctSighting::DeleteScreen(int Index)
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

bool CctSighting::FixPathLoop()
{
    if (_pathIndex == 0) return false;

    XGUID currentScreenId = _path.GetPtr(_pathIndex)->ScreenId;

    for (int i=0; i<_pathIndex-1; i++)
    {
        if (CompareXGuid(&currentScreenId, &_path.GetPtr(i)->ScreenId))
        {
            // Free subsequent screens
            for (int j=i+1; j<_path.GetCount(); j++)
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
            return true;
        }
    }

    return false;
}

//*************************************************************************************************
// Attribute APIs

bool CctSighting::EnumAttributes(int Index, ATTRIBUTE **ppAttribute)
{
    if (Index < _attributes.GetCount())
    {
        *ppAttribute = _attributes.GetPtr(Index);
        return true;
    }
    else
    {
        *ppAttribute = nullptr;
        return false;
    }
}

XGUID *CctSighting::MatchAttribute(XGUIDLIST *pElements)
{
    for (int i=0; i<pElements->GetCount(); i++)
    {
        XGUID id = pElements->Get(i);

        for (int j=0; j<_attributes.GetCount(); j++)
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

    return nullptr;
}

ATTRIBUTE *CctSighting::FindAttribute(XGUID *pElementId)
{
    for (int i=0; i<_attributes.GetCount(); i++)
    {
        ATTRIBUTE *attribute = _attributes.GetPtr(i);
        if (CompareXGuid(pElementId, &attribute->ElementId))
        {
            return attribute;
        }
    }

    return nullptr;
}

int CctSighting::MergeAttributes(int ControlId, ATTRIBUTES *pNewAttributes, ATTRIBUTES *pOldAttributes)
{
    XGUID screenId = _path.GetPtr(_pathIndex)->ScreenId;

    // Find the first instance of the control id attributes - this will be returned
    int firstIndex = FindFirstAttribute(&_attributes, screenId, ControlId);

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
    for (int i=pNewAttributes->GetCount()-1; i>=0; i--)
    {
        ATTRIBUTE attribute = pNewAttributes->Get(i);

        // Remove blank strings
        if ((attribute.Value->Type == dtPText) || (attribute.Value->Type == dtText))
        {
            QString valueString;
            Type_VariantToText(attribute.Value, valueString);

            if (valueString.isEmpty())
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

void WriteDoubleAttribute(const GUID *pGuid, FXDATATYPE DataType, double d, CfxFileStream &Stream)
{
    Stream.Write((void *)pGuid, sizeof(GUID));

    FXVARIANT *tempVariant;
    tempVariant = Type_CreateVariant(DataType, &d);
    Type_VariantToStream(tempVariant, Stream);
    Type_FreeVariant(&tempVariant);
}

bool WriteMediaAttribute(CctSession *pSession, FXVARIANT *pValue, CfxFileStream &Stream, FXFILELIST *pMediaFileList)
{
    if ((pValue->Type != dtText) || (pValue->Size < strlen(MEDIA_PREFIX)))
    {
        return false;
    }

    FXDATATYPE dataType = GetTypeFromString((char *)&pValue->Data[0]);
    if ((dataType != dtSound) && (dataType != dtGraphic))
    {
        return false;
    }

    bool result = false;
    char *mediaName = (char *)(&pValue->Data[0]) + strlen(MEDIA_PREFIX);
    char *fileName = nullptr;
    FXFILEMAP fileMap = {0};

    fileName = pSession->AllocMediaFileName(mediaName);
    if (fileName == nullptr)
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

    result = true;
    if (pMediaFileList != nullptr)
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

    result = true;

cleanup:

    FxUnmapFile(&fileMap);

    MEM_FREE(fileName);

    return result;
}

bool CctSighting::BuildExport(CfxControl *pSessionControl, CfxFileStream &Stream, FXFILELIST *pMediaFileList)
{
    CctSession *session = GetSession(pSessionControl);

    // Clear the stream
    //Stream.Clear();

    // Write the header
    SIGHTING_EXPORT_HEADER header = {0};
    header.Magic = 'SIGT';
    GetHost(this)->GetDeviceId(&header.DeviceId);
    header.DateCreated = (int)EncodeDateTime(&_dateTime);
    header.DateCurrent = header.DateCreated;
    header.Id = _uniqueId;

    int startPosition = Stream.GetPosition();
    Stream.Write(&header, sizeof(header));
    
    // Write date/time attributes
    WriteDoubleAttribute(&GUID_ELEMENT_DATE, dtDate, header.DateCreated, Stream);
    WriteDoubleAttribute(&GUID_ELEMENT_TIME, dtTime, EncodeDateTime(&_dateTime) - header.DateCreated, Stream);
    
    // Write GPS attributes
    if (_gps.Quality > FXGPS_QUALITY::None)
    {
        WriteDoubleAttribute(&GUID_ELEMENT_LATITUDE, dtDouble, _gps.Latitude, Stream);
        WriteDoubleAttribute(&GUID_ELEMENT_LONGITUDE, dtDouble, _gps.Longitude, Stream);

        if (_gps.Quality >= FXGPS_QUALITY::Fix3D)
        {
            WriteDoubleAttribute(&GUID_ELEMENT_ALTITUDE, dtDouble, _gps.Altitude, Stream);
        }

        if ((_gps.Accuracy > 0.0) && (_gps.Accuracy < 50.0))
        {
            WriteDoubleAttribute(&GUID_ELEMENT_ACCURACY, dtDouble, _gps.Accuracy, Stream);
        }

        if (_gps.Speed > 0.0)
        {
            WriteDoubleAttribute(&GUID_ELEMENT_SPEED, dtDouble, _gps.Speed, Stream);
        }
    }

    // Write all other attributes
    for (int i=0; i<_attributes.GetCount(); i++) 
    {
        ATTRIBUTE *attribute = _attributes.GetPtr(i);

        Stream.Write(&attribute->ElementGuid, sizeof(GUID));
        if (!WriteMediaAttribute(session, attribute->Value, Stream, pMediaFileList))
        {
            Type_VariantToStream(attribute->Value, Stream);
        }
    }

    int endPosition = Stream.GetPosition();

    // Fix the header
    header.AttributeSize = endPosition - startPosition - sizeof(header);

    Stream.Seek(startPosition);
    Stream.Write(&header, sizeof(header));
    Stream.Seek(endPosition);

    // Success
    return true;
}

bool CctSighting::HasMedia()
{
   for (int i=0; i<_attributes.GetCount(); i++)
   {
       if (_attributes.GetPtr(i)->IsMedia())
       {
           return true;
       }
   }

   return false;
}

QStringList CctSighting::GetMediaFileList()
{
    QStringList result;

    // Committed attribute media.
    for (int i = 0; i < _attributes.GetCount(); i++)
    {
        char *mediaName = _attributes.GetPtr(i)->GetMediaName();
        if (mediaName)
        {
            result.append(QString::fromUtf8(mediaName));
        }
    }

    // Uncommitted media in screen state blobs.
    for (int i = 0; i < _path.GetCount(); i++)
    {
        auto *data = _path.GetPtr(i)->Data;
        if (data)
        {
            result += CfxReader::FindFileNames(data, MEM_SIZE(data));
        }
    }

    for (int i = 0; i < _oldState.GetCount(); i++)
    {
        auto *data = _oldState.GetPtr(i)->Data;
        if (data)
        {
            result += CfxReader::FindFileNames(data, MEM_SIZE(data));
        }
    }

    return result;
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
    _playButton->SetComposite(true);
    _playButton->SetVisible(false);
    _playButton->SetBorder(ControlBorderStyle::None, 0, 0, 0);
    _playButton->SetOnClick(this, (NotifyMethod)&CctSighting_Painter::OnPlayClick);
    _playButton->SetOnPaint(this, (NotifyMethod)&CctSighting_Painter::OnPlayPaint);

    _stopButton = new CfxControl_Button(pOwner, pParent);
    _stopButton->SetComposite(true);
    _stopButton->SetVisible(false);
    _stopButton->SetBorder(ControlBorderStyle::None, 0, 0, 0);
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

void CctSighting_Painter::ConnectByIndex(int Index)
{
    CctSession *session = GetSession(_parent);

    QString uid;
    auto database = session->CreateSightingDatabasePtr();

    database->Enum(Index, &uid);

    // Connect and load the sighting
    if (uid.isEmpty())
    {
        _sighting->Assign(session->GetCurrentSighting());
    }
    else
    {
        database->Load(_sighting, uid);
    }
}

void CctSighting_Painter::ConnectById(const QString &sightingUid)
{
    CctSession *session = GetSession(_parent);

    // Connect and load the sighting
    if (sightingUid.isEmpty())
    {
        _sighting->Assign(session->GetCurrentSighting());
    }
    else
    {
        auto database = session->CreateSightingDatabasePtr();
        database->Load(_sighting, sightingUid);
    }

    SetSelectedIndex(-1);
}

void CctSighting_Painter::DefineResources(CfxFilerResource &F)
{
}

int CctSighting_Painter::GetItemCount()
{
    int count = 1; // For time

    FXGPS_POSITION *gps = _sighting->GetGPS();
    switch (gps->Quality)
    {
    case FXGPS_QUALITY::User2D:
    case FXGPS_QUALITY::Fix2D:
        count += 2; break; // Lat, Lon
    
    case FXGPS_QUALITY::Fix3D: 
    case FXGPS_QUALITY::Diff3D:
    case FXGPS_QUALITY::Sim3D:
    case FXGPS_QUALITY::User3D:
        count += 3; break; // Lat, Lon, Alt

    case FXGPS_QUALITY::None:
        ;
    }

    // Accuracy
    if ((gps->Quality != FXGPS_QUALITY::None) && (gps->Accuracy > 0.0))
    {
        count += 1;
    } 

    return _sighting->GetAttributes()->GetCount() + count;
}

void CctSighting_Painter::OnPlayClick(CfxControl *pSender, void *pParams)
{
    FX_ASSERT(_selectedIndex != -1);

    char *fileName = GetSession(_parent)->AllocMediaFileName(_selectedFileName.toUtf8().constData());
    GetHost(_parent)->PlayRecording(fileName);
    MEM_FREE(fileName);
}

void CctSighting_Painter::OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPlay(pParams->Rect, pParams->Canvas->InvertColor(_parent->GetColor()), pParams->Canvas->InvertColor(_parent->GetBorderColor()), _playButton->GetDown()); 
}

void CctSighting_Painter::OnStopClick(CfxControl *pSender, void *pParams)
{
    GetHost(_parent)->StopSound();
}

void CctSighting_Painter::OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawStop(pParams->Rect, pParams->Canvas->InvertColor(_parent->GetColor()), pParams->Canvas->InvertColor(_parent->GetBorderColor()), _stopButton->GetDown()); 
}

void CctSighting_Painter::PaintDOUBLE(CfxCanvas *pCanvas, FXRECT *pLRect, FXRECT *pRRect, FXFONTRESOURCE *pFont, const char *pCaption, double Value, UINT16 Precision)
{
    pCanvas->DrawTextRect(pFont, pLRect, TEXT_ALIGN_VCENTER, pCaption);

    char caption[32];
    PrintF(caption, Value, sizeof(caption)-1, Precision);
    pCanvas->DrawTextRect(pFont, pRRect, TEXT_ALIGN_VCENTER, caption);
}

void CctSighting_Painter::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index)
{
    int itemHeight = pRect->Bottom - pRect->Top + 1;
    int paintIndex = Index;

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

        char caption[32];
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
    if (gps->Quality != FXGPS_QUALITY::None)
    {
        char captionLon[32], captionLat[32];
        const char *pTitleLon, *pTitleLat;
        bool invert;
        int index;
        GetProjectedParams(&pTitleLon, &pTitleLat, &invert);
        index = Index ^ (int)invert;

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
    if ((gps->Quality == FXGPS_QUALITY::Fix3D) || (gps->Quality == FXGPS_QUALITY::Diff3D) || (gps->Quality == FXGPS_QUALITY::User3D) || (gps->Quality == FXGPS_QUALITY::Sim3D))
    {
        switch (Index)
        {
        case 0: PaintDOUBLE(pCanvas, &lRect, &rRect, pFont, "Altitude", gps->Altitude, 2);   break;
        }
        if (Index < 1) return;
        Index -= 1;
    }

    // Show Accuracy
    if ((gps->Quality != FXGPS_QUALITY::None) && (gps->Accuracy > 0.0))
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

    FXTEXTRESOURCE *element = (FXTEXTRESOURCE *)resource->GetObject(_parent, elementId, (BYTE)ElementAttribute::Name);
    if (element)
    {
        lRect.Left -= itemHeight;

        FX_ASSERT(element && (element->Magic == MAGIC_TEXT));
        FXBITMAPRESOURCE *bitmap = (FXBITMAPRESOURCE *)resource->GetObject(_parent, elementId, (BYTE)ElementAttribute::Icon32);
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
            FXTEXTRESOURCE *value = (FXTEXTRESOURCE *)resource->GetObject(_parent, attribute->ValueId, (BYTE)ElementAttribute::Name);
            if (value)
            {
                pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, value->Text);
                resource->ReleaseObject(value);
            }
        }
        // Just a regular type
        else
        { 
            QString valueText;
            Type_VariantToText(attribute->Value, valueText);
            FXDATATYPE dataType = GetTypeFromString(valueText.toUtf8().constData());

            if (dataType == dtSound)
            {
                if (_selectedIndex == (int)paintIndex)
                {
                    // Move and display the buttons
                    FXRECT absoluteRect, r;
                    _parent->GetAbsoluteBounds(&absoluteRect);
                    r = *pRect;
                    int tx = r.Right - absoluteRect.Left;
                    int ty = r.Top - absoluteRect.Top;

                    // Stop button
                    r.Right -= itemHeight;
                    tx -= itemHeight;
                    _stopButton->SetVisible(true);
                    _stopButton->SetBounds(tx + 1, ty, itemHeight, itemHeight);

                    // Play button
                    r.Right -= itemHeight;
                    tx -= itemHeight;
                    _playButton->SetVisible(true);
                    _playButton->SetBounds(tx, ty, itemHeight, itemHeight);

                    _selectedFileName = QString::fromUtf8(valueText.toUtf8().constData() + strlen(MEDIA_PREFIX));
                }
            }

            GetAttributeText(_parent, attribute, valueText);

            pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, valueText);
        }

        resource->ReleaseObject(element);
    }
}

void CctSighting_Painter::SetSelectedIndex(int Value) 
{ 
    _playButton->SetVisible(false);
    _stopButton->SetVisible(false);
    _selectedIndex = Value; 
    _parent->Repaint();
}
