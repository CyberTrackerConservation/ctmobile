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
#include "fxPanel.h"
#include "fxImage.h"
#include "fxList.h"
#include "ctWaypoint.h"
#include "ctGotos.h"
#include "ctElement.h"
#include "ctSession.h"
#include "fxProjection.h"
#include "fxMath.h"

enum MapItemType { mitSighting = 1, mitHistory, mitGoto, mitElement };

struct MAP_ITEM
{
    UINT Index;
    UINT Type;
    DOUBLE X, Y;
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
    UINT _selectedIndex;
    TList<UINT> _selection;

    CHAR *_fileName;
    BOOL _valid;
    CHAR *_errorMessage;

    BOOL _movingMapAutoSelect;
    INT _movingMapIndex;

    CfxProjection *_projection;

    BOOL _alternator;

    DOUBLE _lat1, _lon1, _lat2, _lon2;
    FXEXTENT _extent;
    DOUBLE _extentWidth, _extentHeight;

    BOOL _autoConnectGPS, _connectedGPS;
    INT _markerSize, _lineThickness, _trackThickness;
    COLOR _sightingColor, _gpsColorGood, _gpsColorBad, _trackColor, _flagColor, _gotoColor, _historyColor, _historyTrackColor, _elementsColor;
    INT _sightingPointSize, _gotoPointSize, _historyPointSize, _elementsPointSize;
    
    BOOL _lastFixValid, _lastFixGood;

    BOOL _showInspect;
    FXEVENT _onSelectionChange;

    FXEVENT _onFlagFix;
    BOOL _positionFlag;
    FXGPS_POSITION _flagPosition;
    BOOL _retainState;

    BOOL _oneSelect;
    BOOL _showSightings, _showWaypoints, _showHistory, _showGoto, _showElements, _showHistoryWaypoints;
    BOOL _useInspectorForGPS;

    DOUBLE _project_scaleToImageX, _project_scaleToImageY;

    VOID *_cacheFile;
    UINT _cacheFileWidth, _cacheFileHeight, _cacheFileBands;
    DOUBLE _cacheProjectedUniqueness, _projectedUniqueness;
    CfxStream _cacheProjectedWaypoints;
    CfxStream _cacheProjectedPoints;
    CfxStream _cacheProjectedHistoryWaypoints;

    CfxControl_Button *_inspectButton, *_locateButton, *_followButton, *_flagButton, *_mapsButton;

    CctControl_MapList *_mapList;
    BYTE _groupId;

    VOID ResetZoom();
    VOID PlaceButtons();
    BOOL GetImageData(UINT *pWidth, UINT *pHeight);
    VOID DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect);

    VOID SetError(CHAR *pError); 

    VOID RecalcExtent();
    BOOL NewProjection();
    BOOL Project(DOUBLE Latitude, DOUBLE Longitude, INT *pX, INT *pY);
    BOOL Unproject(INT X, INT Y, DOUBLE *pLatitude, DOUBLE *pLongitude);
    VOID RebuildPoints();

    VOID OnEnumWaypoint(WAYPOINT *pWaypoint);
    VOID OnEnumHistoryWaypoint(HISTORY_WAYPOINT *pHistoryWaypoint);

    VOID Render();
    VOID InitLastGPS();
    VOID BuildSelection(INT X1, INT Y1, INT X2, INT Y2);

    VOID *OpenFileCache();
    VOID CloseFileCache();

    VOID OnLocateClick(CfxControl *pSender, VOID *pParams);
    VOID OnMapsClick(CfxControl * pSender, VOID *pParams);
    VOID OnMapSelect(CfxControl * pSender, VOID *pParams);
    VOID OnMapsPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnLocatePaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnFollowPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnFlagPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnInspectPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionEnterNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNewWaypoint(CfxControl *pSender, VOID *pParams);
public:
    CctControl_MovingMap(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_MovingMap();

    MAP_ITEM *GetItem(UINT Index) { return (MAP_ITEM *)_items.GetMemory() + Index; }
    UINT GetItemCount()           { return _items.GetSize() / sizeof(MAP_ITEM);    }
    
    VOID SetCenterSafe(INT x, INT y);
    VOID SetSelectedIndex(UINT Value);
    MAP_ITEM *GetSelectedItem(UINT Index) { return GetItem(_selection.Get(Index)); }
    UINT GetSelectedItemCount()           { return _selection.GetCount();          }

    FXGPS_POSITION *GetFlagPosition() { return &_flagPosition; }

    INT	GetMovingMapIndex()  { return _movingMapIndex; }
    COLOR GetTrackColor()    { return _trackColor;    }
    COLOR GetSightingColor() { return _sightingColor; }
    COLOR GetHistoryColor()  { return _historyColor;  }
    COLOR GetHistoryTrackColor() { return _historyTrackColor; }
    COLOR GetGotoColor()     { return _gotoColor;     }
    COLOR GetElementsColor() { return _elementsColor; }

    VOID SetColors(COLOR TrackValue, COLOR SightingValue, COLOR HistoryTrackValue, COLOR HistoryValue, COLOR GotoValue) 
    { 
        _trackColor = TrackValue;
        _sightingColor = SightingValue;
        _historyColor = HistoryValue;
        _historyTrackColor = HistoryTrackValue;
        _gotoColor = GotoValue;
    }

    VOID SetFileName(CHAR *pFileName);
    INT FindBestMovingMapIndex(FXGPS_POSITION *pGPS);
    VOID SetMovingMap(UINT Index, FXGPS_POSITION *pGPS);
    VOID SetCoords(DOUBLE Lon1, DOUBLE Lat1, DOUBLE Lon2, DOUBLE Lat2);
    VOID SetPositionFlag(BOOL Value);
    VOID SetShowInspect(BOOL Value) { _showInspect = Value; }
    VOID SetOnSelectionChange(CfxControl *pControl, NotifyMethod OnClick);
    VOID SetOnFlagFix(CfxControl *pControl, NotifyMethod OnClick);

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    VOID LoadGlobalState();
    VOID SaveGlobalState();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    VOID OnTimer();
    VOID OnPenDown(INT X, INT Y);
    VOID OnPenUp(INT X, INT Y);
    VOID OnPenClick(INT X, INT Y);
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
    BOOL _oneLine;
    CctSighting_Painter *_sightingPainter;
    CctHistory_Painter *_historyPainter;
    CctGoto_Painter *_gotoPainter;
    CctGoto_Painter *_elementPainter;
public:
    CctControl_MapItemList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_MapItemList();
    
    MAP_ITEM *GetMapItem()      { return &_mapItem; }
    VOID SetMapItem(MAP_ITEM *pValue);
    VOID SetOneLine(BOOL Value) { _oneLine = Value; }

    UINT GetItemCount();
    VOID OnPenDown(INT X, INT Y);
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
    UINT _activeIndex; 
    FXGOTO _activeGoto;
    UINT _itemCount;

    CctControl_MovingMap *_movingMap;
    CctControl_MapItemList *_mapItemList;
    FXEVENT _onChange, _onEditClick;

    CfxControl_Panel *_buttonPanel;
    CfxControl_Button *_buttons[5];

    CfxControl_Panel *_typePanel;

    XGUID _elementId;
    BOOL _required;
    BOOL _oneLine, _showType, _showButtons;

    UINT _buttonHeight, _typeHeight, _itemHeight;

    VOID OnButtonClick(CfxControl *pSender, VOID *pParams);
    VOID OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnButtonPanelResize(CfxControl *pSender, VOID *pParams);
    VOID OnSelectionChange(CfxControl *pSender, VOID *pParams);

    VOID UpdateMapItemList();
    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_MapInspector(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_MapInspector();

    VOID DefineState(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    VOID SetOnChange(CfxControl *pControl, NotifyMethod OnClick);
    VOID SetOnEditClick(CfxControl *pControl, NotifyMethod OnClick);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
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
    INT _selectedIndex;
    TList<UINT> _itemMap;
    BYTE _groupId;
    BOOL _selectionChange;
    FXEVENT _onSelectionChange;
public:
    CctControl_MapList(CfxPersistent *pOwner, CfxControl *pParent);

    VOID DefineState(CfxFiler &F);

    VOID SetGroupId(BYTE Value) { _groupId = Value; _itemMap.Clear(); }

    INT GetSelectedIndex();
    UINT GetItemCount();

    VOID SetOnSelectionChange(CfxPersistent *pObject, NotifyMethod OnSelectionChange);

    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index);
    VOID OnPenDown(INT X, INT Y);
    VOID OnPenClick(INT X, INT Y);
};

extern CfxControl *Create_Control_MapList(CfxPersistent *pOwner, CfxControl *pParent);
