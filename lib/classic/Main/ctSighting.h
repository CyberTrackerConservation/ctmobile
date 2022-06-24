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
#include "fxClasses.h"
#include "fxControl.h"
#include "fxList.h"
#include "fxDatabase.h"

//const UINT MAGIC_SIGHTING  = 'SIGT'; // version 1
//const UINT MAGIC_SIGHTING2 = 'SIG2'; // version 2
const UINT MAGIC_SIGHTING3 = 'SIG3'; // version 3

struct ATTRIBUTE
{
    XGUID ScreenId;
    UINT ControlId;
    XGUID ElementId;
    XGUID ValueId;

    GUID ElementGuid;
    FXVARIANT *Value;

    BOOL IsMedia()
    {
        if ((Value->Type != dtText) || (Value->Size < strlen(MEDIA_PREFIX)))
        {
            return FALSE;
        }

        FXDATATYPE dataType = GetTypeFromString((CHAR *)&Value->Data[0]);
        if ((dataType != dtSound) && (dataType != dtGraphic))
        {
            return FALSE;
        }

        return TRUE;
    }

    CHAR *GetMediaName()
    {
        if (!IsMedia())
        {
            return NULL;
        }

        return (CHAR *)(&Value->Data[0]) + strlen(MEDIA_PREFIX);
    }
};

struct PATHITEM
{
    XGUID ScreenId;
    VOID *Data;
};

typedef TList<ATTRIBUTE> ATTRIBUTES;

typedef TList<CfxString *> STRINGLIST;

#pragma pack(push, 1) 
struct SIGHTING_EXPORT_HEADER {
    UINT Magic;                   // 4
    GUID Id;                      // 16
    GUID DeviceId;                // 16
    UINT DateCreated;             // 4
    UINT DateCurrent;             // 4
    BOOL Deleted;                 // 1
    BOOL CheckedOut;              // 1
    UINT AttributeSize;           // 4
};
#pragma pack(pop)

//
// CctSighting
//
class CctSighting: public CfxPersistent
{
protected:
    FXDATETIME _dateTime;
    GUID _uniqueId;

    UINT _pathIndex;
    TList<PATHITEM> _path;
    TList<PATHITEM> _oldState;
    
    FXGPS_POSITION _gps;
    ATTRIBUTES _attributes;

    BOOL ExtractOldState(XGUID ScreenId, PATHITEM *pPathItem);

    VOID ClearPath();
    VOID ClearAttributes();
public:
    CctSighting(CfxPersistent *pOwner);
    ~CctSighting();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID Clear();
    VOID ClearOldState();
    VOID ClearGPS();

    VOID Init(XGUID FirstScreenId);
    VOID Reset();
    BOOL Load(CfxDatabase *pDatabase, DBID Id);
    BOOL Save(CfxDatabase *pDatabase, DBID *pId);

    FXGPS_POSITION *GetGPS()    { return &_gps;            }

    VOID *GetScreenData(XGUID ScreenId, UINT Index);
    VOID SetScreenData(XGUID ScreenId, UINT Index, VOID *pData);
    
    UINT GetPathIndex()       { return _pathIndex;       }
    UINT GetPathCount()       { return _path.GetCount(); }
    XGUID *GetPathItem(UINT Index) { return &_path.GetPtr(Index)->ScreenId; }

    FXDATETIME *GetDateTime()   { return &_dateTime;       }
    VOID IncPathIndex()         { _pathIndex++;            }
    VOID DecPathIndex()         { _pathIndex--;            }
    BOOL BOP()                  { return _pathIndex <= 0;  }
    BOOL EOP()                  { return _pathIndex == (_path.GetCount() - 1); }
    GUID *GetUniqueId()         { return &_uniqueId;       }
    XGUID *GetCurrentScreenId() { return &_path.GetPtr(_pathIndex)->ScreenId;  }
    XGUID *GetFirstScreenId()   { return &_path.GetPtr(0)->ScreenId;           }
    UINT InsertScreen(XGUID ScreenId);
    UINT AppendScreen(XGUID ScreenId);
    VOID DeleteScreen(UINT Index);
    BOOL FixPathLoop();

    BOOL BuildExport(CfxControl *pSessionControl, CfxFileStream &Stream, FXFILELIST *pMediaFileList);

    BOOL EnumAttributes(UINT Index, ATTRIBUTE **ppAttribute);
    UINT MergeAttributes(UINT ControlId, ATTRIBUTES *pNewAttributes, ATTRIBUTES *pOldAttributes);
    VOID RemoveAttributesFromIndex(UINT Index);

    XGUID *MatchAttribute(XGUIDLIST *pElements);
    ATTRIBUTE *FindAttribute(XGUID *pElementId);
    
    ATTRIBUTES *GetAttributes() { return &_attributes; }

    BOOL HasMedia();
};

//
// CctSighting_Painter.
//

const CHAR TEXT_UNSAVED[] = "Unsaved";

class CctSighting_Painter: public CfxListPainter
{
protected:
    CctSighting *_sighting;
    INT _selectedIndex;
    CfxString _selectedFileName;
    CfxControl_Button *_playButton, *_stopButton;
    VOID OnPlayClick(CfxControl *pSender, VOID *pParams);
    VOID OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnStopClick(CfxControl *pSender, VOID *pParams);
    VOID OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID PaintDOUBLE(CfxCanvas *pCanvas, FXRECT *pLRect, FXRECT *pRRect, FXFONTRESOURCE *pFont, CHAR *pCaption, DOUBLE Value, UINT16 Precision);
public:
    CctSighting_Painter(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctSighting_Painter();

    VOID ConnectByIndex(UINT Index);
    VOID ConnectById(DBID Id);

    INT GetSelectedIndex()           { return _selectedIndex;  }
    VOID SetSelectedIndex(INT Value);

    VOID DefineResources(CfxFilerResource &F);

    UINT GetItemCount();
    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index);
};

extern INT FindFirstAttribute(ATTRIBUTES *pAttributes, XGUID ScreenId, UINT ControlId);
extern VOID ClearAttributes(ATTRIBUTES *pAttributes);
extern VOID CopyAttributes(ATTRIBUTES *pAttributes, XGUID ScreenId, UINT ControlId, ATTRIBUTES *pOutAttributes);
extern VOID MergeAttributeRange(ATTRIBUTES *pDstAttributes, ATTRIBUTES *pSrcAttributes, UINT Index1, UINT Index2);
extern VOID RemoveAttributes(ATTRIBUTES *pAttributes, XGUID ScreenId, UINT ControlId);
extern VOID RemoveAttributesFromIndex(ATTRIBUTES *pAttributes, UINT IndexToStart);
extern VOID DefineAttributes(CfxFiler &F, ATTRIBUTES *pAttributes);
extern VOID GetAttributeText(CfxControl *pControl, ATTRIBUTE *pAttribute, CfxString &Value);
