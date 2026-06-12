#pragma once
#include "pch.h"
#include "fxPanel.h"
#include "fxImage.h"
#include "fxList.h"
#include "ctWaypoint.h"
#include "ctGotos.h"
#include "ctElement.h"
#include "ctSession.h"
#include "fxProjection.h"
#include "fxMath.h"

enum class MapItemType : int8_t { Sighting = 1, History, Goto, Element };

struct MAP_ITEM
{
    int Index = 0;
    MapItemType Type = MapItemType::Sighting;
    double X = 0.0, Y = 0.0;
};

//*************************************************************************************************
//
// CctControl_MovingMap
//

const GUID GUID_CONTROL_MOVINGMAP = {0x79de22, 0x12d, 0x4acb, {0x85, 0xe0, 0x10, 0x0, 0xfc, 0x1, 0x87, 0x97}};

class CctControl_MapList;
class CctControl_MovingMap: public CfxControl_ZoomImageBase
{
protected:
    CfxStream _items;
    int _selectedIndex = 0;
    TList<int> _selection;

    char *_fileName = nullptr;
    bool _valid = false;
    char *_errorMessage = nullptr;

    bool _movingMapAutoSelect = false;
    int _movingMapIndex = 0;

    CfxProjection *_projection = nullptr;

    bool _alternator = false;

    double _lat1 = 0.0, _lon1 = 0.0, _lat2 = 0.0, _lon2 = 0.0;
    FXEXTENT _extent;
    double _extentWidth = 0.0, _extentHeight = 0.0;

    bool _autoConnectGPS = false, _connectedGPS = false;
    int _markerSize = 0, _lineThickness = 0, _trackThickness = 0;
    COLOR _sightingColor = 0, _gpsColorGood = 0, _gpsColorBad = 0, _trackColor = 0, _flagColor = 0, _gotoColor = 0, _historyColor = 0, _historyTrackColor = 0, _elementsColor = 0;
    int _sightingPointSize = 0, _gotoPointSize = 0, _historyPointSize = 0, _elementsPointSize = 0;

    bool _lastFixValid = false, _lastFixGood = false;

    bool _showInspect = false;
    FXEVENT _onSelectionChange;

    FXEVENT _onFlagFix;
    bool _positionFlag = false;
    FXGPS_POSITION _flagPosition;
    bool _retainState = false;

    bool _oneSelect = false;
    bool _showSightings = false, _showWaypoints = false, _showHistory = false, _showGoto = false, _showElements = false, _showHistoryWaypoints = false;
    bool _useInspectorForGPS = false;

    double _project_scaleToImageX = 0.0, _project_scaleToImageY = 0.0;

    void *_cacheFile = nullptr;
    int _cacheFileWidth = 0, _cacheFileHeight = 0, _cacheFileBands = 0;
    double _cacheProjectedUniqueness = 0.0, _projectedUniqueness = 0.0;
    CfxStream _cacheProjectedWaypoints;
    CfxStream _cacheProjectedPoints;
    CfxStream _cacheProjectedHistoryWaypoints;

    CfxControl_Button *_inspectButton = nullptr, *_locateButton = nullptr, *_followButton = nullptr, *_flagButton = nullptr, *_mapsButton = nullptr;

    CctControl_MapList *_mapList = nullptr;
    BYTE _groupId = 0;

    void ResetZoom() override;
    void PlaceButtons() override;
    bool GetImageData(int *pWidth, int *pHeight) override;
    void DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect) override;

    void SetError(const char *pError);

    void RecalcExtent();
    bool NewProjection();
    bool Project(double Latitude, double Longitude, int *pX, int *pY);
    bool Unproject(int X, int Y, double *pLatitude, double *pLongitude);
    void RebuildPoints();

    void OnEnumWaypoint(WAYPOINT *pWaypoint);
    void OnEnumHistoryWaypoint(HISTORY_WAYPOINT *pHistoryWaypoint);

    void Render();
    void InitLastGPS();
    void BuildSelection(int X1, int Y1, int X2, int Y2);

    void *OpenFileCache();
    void CloseFileCache();

    void OnLocateClick(CfxControl *pSender, void *pParams);
    void OnMapsClick(CfxControl * pSender, void *pParams);
    void OnMapSelect(CfxControl * pSender, void *pParams);
    void OnMapsPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnLocatePaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnFollowPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnFlagPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnInspectPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
    void OnSessionEnterNext(CfxControl *pSender, void *pParams);
    void OnSessionNewWaypoint(CfxControl *pSender, void *pParams);
public:
    CctControl_MovingMap(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_MovingMap();

    MAP_ITEM *GetItem(int Index) { return (MAP_ITEM *)_items.GetMemory() + Index; }
    int GetItemCount() { return _items.GetSize() / sizeof(MAP_ITEM);    }
    
    void SetCenterSafe(int x, int y);
    void SetSelectedIndex(int Value);
    MAP_ITEM *GetSelectedItem(int Index) { return GetItem(_selection.Get(Index)); }
    int GetSelectedItemCount()           { return _selection.GetCount();          }

    FXGPS_POSITION *GetFlagPosition() { return &_flagPosition; }

    int	GetMovingMapIndex()  { return _movingMapIndex; }
    COLOR GetTrackColor()    { return _trackColor;    }
    COLOR GetSightingColor() { return _sightingColor; }
    COLOR GetHistoryColor()  { return _historyColor;  }
    COLOR GetHistoryTrackColor() { return _historyTrackColor; }
    COLOR GetGotoColor()     { return _gotoColor;     }
    COLOR GetElementsColor() { return _elementsColor; }

    void SetColors(COLOR TrackValue, COLOR SightingValue, COLOR HistoryTrackValue, COLOR HistoryValue, COLOR GotoValue) 
    { 
        _trackColor = TrackValue;
        _sightingColor = SightingValue;
        _historyColor = HistoryValue;
        _historyTrackColor = HistoryTrackValue;
        _gotoColor = GotoValue;
    }

    void SetFileName(char *pFileName);
    int FindBestMovingMapIndex(FXGPS_POSITION *pGPS);
    void SetMovingMap(int Index, FXGPS_POSITION *pGPS);
    void SetCoords(double Lon1, double Lat1, double Lon2, double Lat2);
    void SetPositionFlag(bool Value);
    void SetShowInspect(bool Value) { _showInspect = Value; }
    void SetOnSelectionChange(CfxControl *pControl, NotifyMethod OnClick);
    void SetOnFlagFix(CfxControl *pControl, NotifyMethod OnClick);

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    void LoadGlobalState();
    void SaveGlobalState();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    void OnTimer() override;
    void OnPenDown(int X, int Y) override;
    void OnPenUp(int X, int Y) override;
    void OnPenClick(int X, int Y) override;
};

extern CfxControl *Create_Control_MovingMap(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************

const GUID GUID_CONTROL_MAPITEMLIST = {0xb172adac, 0x6787, 0x4456, {0xb6, 0x34, 0x61, 0x1d, 0x5b, 0xeb, 0x98, 0xf5}};

//
// CctControl_MapItemList
//
class CctControl_MapItemList: public CfxControl_List
{
protected:
    MAP_ITEM _mapItem;
    bool _oneLine = false;
    CctSighting_Painter *_sightingPainter = nullptr;
    CctHistory_Painter *_historyPainter = nullptr;
    CctGoto_Painter *_gotoPainter = nullptr;
    CctGoto_Painter *_elementPainter = nullptr;
public:
    CctControl_MapItemList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_MapItemList();
    
    MAP_ITEM *GetMapItem()      { return &_mapItem; }
    void SetMapItem(MAP_ITEM *pValue);
    void SetOneLine(bool Value) { _oneLine = Value; }

    int GetItemCount() override;
    void OnPenClick(int X, int Y) override;
};

extern CfxControl *Create_Control_MapItemList(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_MapInspector
//

const GUID GUID_CONTROL_MAPINSPECTOR = {0x2acccd30, 0xed54, 0x4fd0, {0x8d, 0x49, 0x37, 0x10, 0x4d, 0xdd, 0x76, 0x5f}};

class CctControl_MapInspector: public CfxControl_Panel
{
protected:
    int _activeIndex = 0;
    FXGOTO _activeGoto;
    int _itemCount = 0;

    CctControl_MovingMap *_movingMap = nullptr;
    CctControl_MapItemList *_mapItemList = nullptr;
    FXEVENT _onChange, _onEditClick;

    CfxControl_Panel *_buttonPanel = nullptr;
    CfxControl_Button *_buttons[5] = {};

    CfxControl_Panel *_typePanel = nullptr;

    XGUID _elementId = 0;
    bool _required = false;
    bool _oneLine = false, _showType = false, _showButtons = false;

    int _buttonHeight = 0, _typeHeight = 0, _itemHeight = 0;

    void OnButtonClick(CfxControl *pSender, void *pParams);
    void OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnButtonPanelResize(CfxControl *pSender, void *pParams);
    void OnSelectionChange(CfxControl *pSender, void *pParams);

    void UpdateMapItemList();
    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_MapInspector(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_MapInspector();

    void DefineState(CfxFiler &F) override;
    void DefineResources(CfxFilerResource &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    void SetOnChange(CfxControl *pControl, NotifyMethod OnClick);
    void SetOnEditClick(CfxControl *pControl, NotifyMethod OnClick);

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

extern CfxControl *Create_Control_MapInspector(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CctControl_MapList
//

const GUID GUID_CONTROL_MAPLIST = {0xcf257177, 0x59c3, 0x4cab, {0x8a, 0x46, 0xb, 0xe3, 0x59, 0xe1, 0x46, 0xce}};

class CctControl_MapList: public CfxControl_List
{
protected:
    int _selectedIndex = 0;
    TList<int> _itemMap;
    BYTE _groupId = 0;
    bool _selectionChange = false;
    FXEVENT _onSelectionChange;
public:
    CctControl_MapList(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineState(CfxFiler &F) override;

    void SetGroupId(BYTE Value) { _groupId = Value; _itemMap.Clear(); }

    int GetSelectedIndex();
    int GetItemCount() override;

    void SetOnSelectionChange(CfxPersistent *pObject, NotifyMethod OnSelectionChange);

    void PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index) override;
    void OnPenClick(int X, int Y) override;
};

extern CfxControl *Create_Control_MapList(CfxPersistent *pOwner, CfxControl *pParent);
