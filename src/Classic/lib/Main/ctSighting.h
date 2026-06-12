#pragma once
#include "pch.h"
#include "fxClasses.h"
#include "fxControl.h"
#include "fxList.h"
#include "fxDatabase.h"

//const int MAGIC_SIGHTING  = 'SIGT'; // version 1
//const int MAGIC_SIGHTING2 = 'SIG2'; // version 2
const int MAGIC_SIGHTING3 = 'SIG3'; // version 3

struct ATTRIBUTE
{
    XGUID ScreenId = 0;
    int ControlId = 0;
    XGUID ElementId = 0;
    XGUID ValueId = 0;

    GUID ElementGuid;
    FXVARIANT *Value = nullptr;

    bool IsMedia()
    {
        if ((Value->Type != dtText) || (Value->Size < strlen(MEDIA_PREFIX)))
        {
            return false;
        }

        FXDATATYPE dataType = GetTypeFromString((char *)&Value->Data[0]);
        if ((dataType != dtSound) && (dataType != dtGraphic))
        {
            return false;
        }

        return true;
    }

    char *GetMediaName()
    {
        if (!IsMedia())
        {
            return nullptr;
        }

        return (char *)(&Value->Data[0]) + strlen(MEDIA_PREFIX);
    }
};

struct PATHITEM
{
    XGUID ScreenId = 0;
    void *Data = nullptr;
};

typedef TList<ATTRIBUTE> ATTRIBUTES;



#pragma pack(push, 1) 
struct SIGHTING_EXPORT_HEADER {
    int Magic = 0;                   // 4
    GUID Id;                         // 16
    GUID DeviceId;                   // 16
    int DateCreated = 0;             // 4
    int DateCurrent = 0;             // 4
    bool Deleted = false;            // 1
    bool CheckedOut = false;         // 1
    int AttributeSize = 0;           // 4
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

    int _pathIndex = 0;
    TList<PATHITEM> _path;
    TList<PATHITEM> _oldState;
    
    FXGPS_POSITION _gps;
    ATTRIBUTES _attributes;

    bool ExtractOldState(XGUID ScreenId, PATHITEM *pPathItem);

    void ClearPath();
    void ClearAttributes();
public:
    CctSighting(CfxPersistent *pOwner);
    ~CctSighting();

    void DefineProperties(CfxFiler &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void Clear();
    void ClearOldState();
    void ClearGPS();

    void Init(XGUID FirstScreenId);
    void Reset();
    bool Load(CfxDatabase *pDatabase, DBID Id);
    bool LoadBlob(const QByteArray &In);
    bool Save(CfxDatabase *pDatabase, DBID *pId);
    void SaveBlob(QByteArray &Out);

    FXGPS_POSITION *GetGPS()    { return &_gps;            }

    void *GetScreenData(XGUID ScreenId, int Index);
    void SetScreenData(XGUID ScreenId, int Index, void *pData);
    
    int GetPathIndex()       { return _pathIndex;       }
    int GetPathCount()       { return _path.GetCount(); }
    XGUID *GetPathItem(int Index) { return &_path.GetPtr(Index)->ScreenId; }

    FXDATETIME *GetDateTime()   { return &_dateTime;       }
    void IncPathIndex()         { _pathIndex++;            }
    void DecPathIndex()         { _pathIndex--;            }
    bool BOP()                  { return _pathIndex <= 0;  }
    bool EOP()                  { return _pathIndex == (_path.GetCount() - 1); }
    GUID *GetUniqueId()         { return &_uniqueId;       }
    XGUID *GetCurrentScreenId() { return &_path.GetPtr(_pathIndex)->ScreenId;  }
    XGUID *GetFirstScreenId()   { return &_path.GetPtr(0)->ScreenId;           }
    int InsertScreen(XGUID ScreenId);
    int AppendScreen(XGUID ScreenId);
    void DeleteScreen(int Index);
    bool FixPathLoop();

    bool BuildExport(CfxControl *pSessionControl, CfxFileStream &Stream, FXFILELIST *pMediaFileList);

    bool EnumAttributes(int Index, ATTRIBUTE **ppAttribute);
    int MergeAttributes(int ControlId, ATTRIBUTES *pNewAttributes, ATTRIBUTES *pOldAttributes);
    void RemoveAttributesFromIndex(int Index);

    XGUID *MatchAttribute(XGUIDLIST *pElements);
    ATTRIBUTE *FindAttribute(XGUID *pElementId);
    
    ATTRIBUTES *GetAttributes() { return &_attributes; }

    bool HasMedia();
    QStringList GetMediaFileList();
};

//
// CctSighting_Painter.
//

const char TEXT_UNSAVED[] = "Unsaved";

class CctSighting_Painter: public CfxListPainter
{
protected:
    CctSighting *_sighting = nullptr;
    int _selectedIndex = 0;
    QString _selectedFileName;
    CfxControl_Button *_playButton = nullptr, *_stopButton = nullptr;
    void OnPlayClick(CfxControl *pSender, void *pParams);
    void OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnStopClick(CfxControl *pSender, void *pParams);
    void OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void PaintDOUBLE(CfxCanvas *pCanvas, FXRECT *pLRect, FXRECT *pRRect, FXFONTRESOURCE *pFont, const char *pCaption, double Value, UINT16 Precision);
public:
    CctSighting_Painter(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctSighting_Painter();

    void ConnectByIndex(int Index);
    void ConnectById(const QString &sightingUid);

    int GetSelectedIndex()           { return _selectedIndex;  }
    void SetSelectedIndex(int Value);

    void DefineResources(CfxFilerResource &F) override;

    int GetItemCount() override;
    void PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index) override;
};

extern int FindFirstAttribute(ATTRIBUTES *pAttributes, XGUID ScreenId, int ControlId);
extern void ClearAttributes(ATTRIBUTES *pAttributes);
extern void CopyAttributes(ATTRIBUTES *pAttributes, XGUID ScreenId, int ControlId, ATTRIBUTES *pOutAttributes);
extern void MergeAttributeRange(ATTRIBUTES *pDstAttributes, ATTRIBUTES *pSrcAttributes, int Index1, int Index2);
extern void RemoveAttributes(ATTRIBUTES *pAttributes, XGUID ScreenId, int ControlId);
extern void RemoveAttributesFromIndex(ATTRIBUTES *pAttributes, int IndexToStart);
extern void DefineAttributes(CfxFiler &F, ATTRIBUTES *pAttributes);
extern void GetAttributeText(CfxControl *pControl, ATTRIBUTE *pAttribute, QString &Value);
