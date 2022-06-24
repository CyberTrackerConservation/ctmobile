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

#include "fxPlatform.h"
#include "ECW.h"

#include "fxApplication.h"
#include "fxUtils.h"
#include "fxGPS.h"
#include "fxMath.h"
#include "ctMovingMap.h"
#include "ctSession.h"
#include "ctActions.h"
#include "ctElementList.h"
#include "ctDialog_SightingEditor.h"

//*************************************************************************************************

#pragma pack(push, 1) 
struct CachedWaypoint
{
    DOUBLE TimeStamp;
    INT X, Y;
};

struct CachedPoint
{
    INT Type;
    INT X, Y;
};
#pragma pack(pop) 

BOOL g_bInit = FALSE;
BOOL g_bValid = FALSE;

BOOL InitLibrary()
{
    if (g_bInit) return TRUE;

    g_bInit = TRUE;
    NCSecwInit();

    return TRUE;
}

//*************************************************************************************************
// CctControl_MovingMap

CfxControl *Create_Control_MovingMap(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_MovingMap(pOwner, pParent);
}

CctControl_MovingMap::CctControl_MovingMap(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_ZoomImageBase(pOwner, pParent), _cacheProjectedWaypoints(), _cacheProjectedPoints(), _cacheProjectedHistoryWaypoints(), _items(), _selection()
{
    InitControl(&GUID_CONTROL_MOVINGMAP);
    InitLockProperties("Filename;Longitude (Left);Latitude (Top);Longitude (Right);Latitude (Bottom)");

    _alternator = FALSE;

    _smoothPan = FALSE;

    _initialButtonState = 2; // Follow

    _projection = NULL;

    _connectedGPS = FALSE;
    _autoConnectGPS = TRUE;
    _lastFixGood = _lastFixValid = FALSE;

    _showInspect = FALSE;
    _selectedIndex = 0;
    _onSelectionChange.Object = 0;

    _positionFlag = TRUE;
    memset(&_flagPosition, 0, sizeof(FXGPS_POSITION));

    _onFlagFix.Object = 0;

    _lat1 = _lat2 = _lon1 = _lon2 = 0.0;
    memset(&_extent, 0, sizeof(FXEXTENT));
    _extentWidth = _extentHeight = 0;
    
    _fileName = 0;
    _movingMapAutoSelect = TRUE;
    _movingMapIndex = -1;
    _errorMessage = NULL;
    _cacheFile = NULL;
    _cacheFileWidth = _cacheFileHeight = _cacheFileBands = 0;
    _cacheProjectedUniqueness = 0;
    _projectedUniqueness = 1;

    _markerSize = 15;
    _lineThickness = 1;
    _trackThickness = 1;
    _gpsColorGood = 0xFF00; // Green
    _gpsColorBad = 0xFF;    // Red
    _flagColor = 0xFF00FF;  // Purple
    _trackColor = 0xFFFF;   // Yellow
    _sightingColor = 0xFFFF; // Yellow;
    _gotoColor = 0xFF0000;  // Blue
    _historyColor = 0xFFFF00; // Cyan
    _historyTrackColor = 0xFFFF00; // Cyan
    _elementsColor = 0xFFFFFF; // White

    _groupId = 0;

    _sightingPointSize = _gotoPointSize = _historyPointSize = _elementsPointSize = 3;

    _oneSelect = FALSE;
    _showSightings = _showWaypoints = _showHistory = _showGoto = _showElements = TRUE;
    _showHistoryWaypoints = TRUE;
    _useInspectorForGPS = FALSE;

    _retainState = FALSE;

    // Create buttons
    _locateButton = new CfxControl_Button(pOwner, this);
    _locateButton->SetComposite(TRUE);
    _locateButton->SetOnClick(this, (NotifyMethod)&CctControl_MovingMap::OnLocateClick);
    _locateButton->SetOnPaint(this, (NotifyMethod)&CctControl_MovingMap::OnLocatePaint);

    _followButton = new CfxControl_Button(pOwner, this);
    _followButton->SetComposite(TRUE);
    _followButton->SetGroupId(1);
    _followButton->SetOnPaint(this, (NotifyMethod)&CctControl_MovingMap::OnFollowPaint);

    _flagButton = new CfxControl_Button(pOwner, this);
    _flagButton->SetComposite(TRUE);
    _flagButton->SetGroupId(1);
    _flagButton->SetOnPaint(this, (NotifyMethod)&CctControl_MovingMap::OnFlagPaint);
    _flagButton->SetVisible(TRUE);

    _inspectButton = new CfxControl_Button(pOwner, this);
    _inspectButton->SetComposite(TRUE);
    _inspectButton->SetGroupId(1);
    _inspectButton->SetOnPaint(this, (NotifyMethod)&CctControl_MovingMap::OnInspectPaint);

    _mapsButton = new CfxControl_Button(pOwner, this);
    _mapsButton->SetComposite(TRUE);
    _mapsButton->SetOnPaint(this, (NotifyMethod)&CctControl_MovingMap::OnMapsPaint);
    _mapsButton->SetOnClick(this, (NotifyMethod)&CctControl_MovingMap::OnMapsClick);

    // Map list
    _mapList = new CctControl_MapList(pOwner, this);
    _mapList->SetComposite(TRUE);
    _mapList->SetVisible(FALSE);
    _mapList->SetOnSelectionChange(this, (NotifyMethod)&CctControl_MovingMap::OnMapSelect);

    // Init Library
    _valid = InitLibrary();
    if (!_valid)
    {
        SetError("Not supported!");
    }

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_MovingMap::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_MovingMap::OnSessionNext);
    RegisterSessionEvent(seEnterNext, this, (NotifyMethod)&CctControl_MovingMap::OnSessionEnterNext);
    RegisterSessionEvent(seNewWaypoint, this, (NotifyMethod)&CctControl_MovingMap::OnSessionNewWaypoint);
}

CctControl_MovingMap::~CctControl_MovingMap()
{
    KillTimer();

    if (_connectedGPS)
    {
        GetEngine(this)->UnlockGPS();
    }

    UnregisterSessionEvent(seNewWaypoint, this);
    UnregisterSessionEvent(seEnterNext, this);
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);
    
    delete _projection;
    _projection = NULL;

    SetFileName(NULL);
}

VOID CctControl_MovingMap::SetFileName(CHAR *pFileName)
{
    CloseFileCache();

    MEM_FREE(_fileName);
    _fileName = NULL;
    MEM_FREE(_errorMessage);
    _errorMessage = NULL;

    if (pFileName)
    {
        _fileName = (CHAR *) MEM_ALLOC(strlen(pFileName) + 1);
        strcpy(_fileName, pFileName);
    }
}

INT CctControl_MovingMap::FindBestMovingMapIndex(FXGPS_POSITION *pGPS)
{		
    DOUBLE lastDiff = 361;
    INT index = -1;

    CfxResource *resource = GetResource();
    for (UINT i = 0; i < resource->GetHeader(this)->MovingMapsCount; i++)
    {
        FXMOVINGMAP *m = resource->GetMovingMap(this, i);

        // Skip bad maps
        if (m == NULL)
        {
            continue;
        }

        // Skip from out of group
        if (m->GroupId != _groupId)
        {
            continue;
        }

        // It needs to not be inside
        if (pGPS->Quality == fqNone)
        {
            index = i;
            break;
        }

        if (pGPS->Longitude < m->Xa || pGPS->Longitude > m->Xb || pGPS->Latitude < m->Yb || pGPS->Latitude > m->Ya)
        {
            continue;
        }
            
        // Find highest resolution
        DOUBLE currentDiff = m->Xb - m->Xa;
        if (currentDiff < lastDiff)
        {
            index = i;
        }
    }
    
    // Can't find one.
    return index;
}

VOID CctControl_MovingMap::SetCenterSafe(INT x, INT y)
{
    BOOL inBounds = (x >= 0) && (x < (INT)_imageWidth) &&
                    (y >= 0) && (y < (INT)_imageHeight);

    if (inBounds)
    {
        SetCenter(x, y);
    }
    else
    {
        SetCenter(_imageWidth / 2, _imageHeight / 2);
    }
}

VOID CctControl_MovingMap::SetMovingMap(UINT Index, FXGPS_POSITION *pGPS)
{
    CctSession* session = GetSession(this);
    CTRESOURCEHEADER *header = session->GetResourceHeader();

    if (Index == -1)
    {
        Index = this->FindBestMovingMapIndex(pGPS);
    }

    if ((Index < 0) || (Index >= session->GetResourceHeader()->MovingMapsCount))
    {
        _movingMapIndex = -1;
        return;
    }

    if (_movingMapIndex != (INT)Index)
    {
        _movingMapIndex = Index;

        CfxResource *resource = GetResource();
        FXMOVINGMAP *m = resource->GetMovingMap(this, Index);

        if (m)
        {
        
            SetFileName(m->FileName);
            SetColors(m->TrackColor, m->SightingColor, m->HistoryTrackColor, m->HistoryColor, m->GotoColor);
            SetLock100(m->Lock100);
            
            OpenFileCache();

            _refreshCacheCanvas = TRUE;
            _imageWidth = _imageHeight = 0;
            GetImageData(&_imageWidth, &_imageHeight);
            
            resource->ReleaseMovingMap(m);
        }

        INT x, y;
        if ((pGPS->Quality != fqNone) && Project(pGPS->Latitude, pGPS->Longitude, &x, &y))
        {
            SetCenterSafe(x, y);
        }

        Repaint();
    }
}

VOID CctControl_MovingMap::SetOnSelectionChange(CfxControl *pControl, NotifyMethod OnClick)
{
    _onSelectionChange.Object = pControl;
    _onSelectionChange.Method = OnClick;
}

VOID CctControl_MovingMap::SetOnFlagFix(CfxControl *pControl, NotifyMethod OnClick)
{
    _onFlagFix.Object = pControl;
    _onFlagFix.Method = OnClick;
}

VOID CctControl_MovingMap::SetCoords(DOUBLE Lon1, DOUBLE Lat1, DOUBLE Lon2, DOUBLE Lat2)
{
    _lon1 = Lon1;
    _lon2 = Lon2;
    _lat1 = Lat1;
    _lat2 = Lat2;

    RecalcExtent();
}

VOID CctControl_MovingMap::SetPositionFlag(BOOL Value) 
{ 
    _positionFlag = Value; 
    _flagButton->SetVisible(Value); 
}

VOID CctControl_MovingMap::PlaceButtons()
{
    if ((_width < (INT)_buttonWidth) || (_height < (INT)_buttonWidth)) return;

    // Position the toolbar buttons
    UINT tx = _borderWidth;
    UINT ty = _borderWidth;
    UINT bw = _buttonWidth;
    UINT buttonCount = 10 - (_inspectButton->GetVisible() ? 0 : 1) - (_flagButton->GetVisible() ? 0 : 1) - (_mapsButton->GetVisible() ? 0 : 1);
    INT bwm1 = _buttonWidth - _borderLineWidth;

    UINT spacerCount = 1 + (_mapsButton->GetVisible() ? 1 : 0);
    UINT spacerWidth = spacerCount * 4 * _borderLineWidth;
    if (bwm1 * buttonCount + spacerWidth > GetClientWidth())
    {
        bwm1 = (GetClientWidth() - spacerWidth) / buttonCount;
        bw = bwm1 + _borderLineWidth;
    }

    spacerWidth = GetClientWidth() - bwm1 * buttonCount - _borderLineWidth;

    UINT spacer1, spacer2;
    if (spacerCount == 1)
    {
        spacer1 = spacerWidth;
        spacer2 = spacer1;
    }
    else
    {
        spacer1 = spacerWidth / 2;
        spacer2 = spacerWidth - spacer1;
    }

    spacer1 = min(12 * _borderLineWidth, spacer1);
    spacer2 = min(12 * _borderLineWidth, spacer2);

    _locateButton->SetBounds(tx, ty, bw, bw);
    _locateButton->SetBorderColor(_borderColor);
    tx += bwm1;
    _fitButton->SetBounds(tx, ty, bw, bw);
    _fitButton->SetBorderColor(_borderColor);
    tx += bwm1;
    _zoomInButton->SetBounds(tx, ty, bw, bw);
    _zoomInButton->SetBorderColor(_borderColor);
    tx += bwm1;
    _zoomOutButton->SetBounds(tx, ty, bw, bw);
    _zoomOutButton->SetBorderColor(_borderColor);
    tx += bwm1 + spacer1;
    _zoomButton->SetBounds(tx, ty, bw, bw);
    _zoomButton->SetBorderColor(_borderColor);
    tx += bwm1;
    _panButton->SetBounds(tx, ty, bw, bw);
    _panButton->SetBorderColor(_borderColor);
    tx += bwm1;
    _followButton->SetBounds(tx, ty, bw, bw);
    _followButton->SetBorderColor(_borderColor);
    tx += bwm1;

    if (_flagButton->GetVisible())
    {
        _flagButton->SetBounds(tx, ty, bw, bw);
        _flagButton->SetBorderColor(_borderColor);
        tx += bwm1;
    }

    if (_inspectButton->GetVisible())
    {
        _inspectButton->SetBounds(tx, ty, bw, bw);
        _inspectButton->SetBorderColor(_borderColor);
        tx += bwm1;
    }

    if (_mapsButton->GetVisible())
    {
        tx += spacer2;
        _mapsButton->SetBounds(tx, ty, bw, bw);
        _mapsButton->SetBorderColor(_borderColor);
    }
    
    _mapList->SetBounds(0, 0, _width, _height);

    _finalButtonWidth = bw;
}

VOID CctControl_MovingMap::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_ZoomImageBase::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _mapList->Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _markerSize = max(5, (_markerSize * ScaleX) / 100);
    _trackThickness = max(1, (_trackThickness * ScaleBorder) / 100);
    _lineThickness = max(1, (_lineThickness * ScaleBorder) / 100);
}

VOID *CctControl_MovingMap::OpenFileCache()
{
    if ((_cacheFile != NULL) || (_fileName == NULL))
    {
        return _cacheFile;
    }

    void *pNCSFileView = NULL;
    CHAR *fileName = GetHost(this)->AllocPathApplication(_fileName);
    int eError = NCScbmOpenFileView(fileName, &pNCSFileView);
    MEM_FREE(fileName);

    if (eError != 0) 
    {
        return _cacheFile;
    }

    CHAR projectionCode[64] = {0};
    UINT projectionValue = 0;
    NCSFileViewFileInfo *pNCSFileInfo = NULL;
    NCScbmGetViewFileInfo(pNCSFileView, &pNCSFileInfo);

    _cacheFile = pNCSFileView;
    _cacheFileWidth = pNCSFileInfo->nSizeX;
    _cacheFileHeight = pNCSFileInfo->nSizeY;
    _cacheFileBands = pNCSFileInfo->nBands;
    _extentWidth = _extentHeight = 0;

    delete _projection;
    _projection = NULL;

    if (sscanf(pNCSFileInfo->szProjection, "%s %lu", &projectionCode[0], &projectionValue) == 2)
    {
        if (strcmp(projectionCode, "UTM") == 0)
        {
            _projection = new CfxProjectionUTM(projectionValue, pNCSFileInfo->szDatum);
        }
        else if ((strcmp(projectionCode, "VEA") == 0) || (strcmp(projectionCode, "VEF") == 0))
        {
            _projection = new CfxProjectionVirtualEarth(projectionValue);
        }
    }
    else if ((strncmp(pNCSFileInfo->szProjection, "NUTM", 4) == 0) &&
             (sscanf(pNCSFileInfo->szProjection + 4, "%lu", &projectionValue) == 1))
    {
        _projection = new CfxProjectionUTM(projectionValue, pNCSFileInfo->szDatum);
    }

    _lon1 = pNCSFileInfo->fOriginX - 0.5 * pNCSFileInfo->fCellIncrementX;
    _lat2 = pNCSFileInfo->fOriginY - 0.5 * pNCSFileInfo->fCellIncrementY;
    _lon2 = pNCSFileInfo->fOriginX + pNCSFileInfo->nSizeX * pNCSFileInfo->fCellIncrementX;
    _lat1 = pNCSFileInfo->fOriginY + pNCSFileInfo->nSizeY * pNCSFileInfo->fCellIncrementY;

    RecalcExtent();

    return _cacheFile;
}

VOID CctControl_MovingMap::CloseFileCache()
{
    if (_cacheFile)
    {
        NCScbmCloseFileView(_cacheFile);
        _cacheFile = NULL;
        _cacheFileWidth = _cacheFileHeight = _cacheFileBands = 0;
    }
    _cacheProjectedWaypoints.Clear();
    _cacheProjectedPoints.Clear();
    _cacheProjectedHistoryWaypoints.Clear();
}

VOID CctControl_MovingMap::SetError(CHAR *pError)
{
    MEM_FREE(_errorMessage);
    _errorMessage = NULL;

    if (pError)
    {
        _errorMessage = (CHAR *) MEM_ALLOC(strlen(pError) + 1);
        strcpy(_errorMessage, pError);
    }
}

VOID CctControl_MovingMap::LoadGlobalState()
{
    if (!IsLive()) return;

    STOREITEM *item = GetSession(this)->GetStoreItem(_typeId, -1);
    if (item)
    {
        CfxStream stream(item->Data);
        CfxReader reader(&stream);
        DefineState(reader);
    }
}

VOID CctControl_MovingMap::SaveGlobalState()
{
    if (!IsLive()) return;

    CfxStream stream;
    CfxWriter writer(&stream);
    DefineState(writer);
    GetSession(this)->SetStoreItem(_typeId, -1, stream.CloneMemory());
}

VOID CctControl_MovingMap::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_ZoomImageBase::DefineResources(F);

    F.DefineFile(_fileName);
    F.DefineFont(_font);

    CctControl_ElementList *elementList = NULL;
    for (CfxControl *control = this;
         (control != NULL) && (elementList == NULL);
         control = control->GetParent()) {
        elementList = (CctControl_ElementList *)control->FindControlByType(&GUID_CONTROL_ELEMENTLIST);
    }

    if (elementList)
    {
        UINT i;
        for (i = 0; i < elementList->GetItemCount(); i++)
        {
            XGUID elementId = elementList->GetItem(i);
            if (!IsNullXGuid(&elementId))
            {
                F.DefineObject(elementId, eaGoto);
            }
        }
    }
#endif
}

VOID CctControl_MovingMap::DefineProperties(CfxFiler &F)
{
    CfxControl_ZoomImageBase::DefineProperties(F);

    F.DefineValue("AutoConnectGPS", dtBoolean,  &_autoConnectGPS, F_TRUE);
    F.DefineXFILE("FileName",    &_fileName);
    F.DefineValue("MarkerSize",   dtInt32,   &_markerSize, "15");
    F.DefineValue("TrackThickness", dtInt32, &_trackThickness, F_ONE);
    F.DefineValue("GPSColorGood", dtColor,   &_gpsColorGood);
    F.DefineValue("GPSColorBad",  dtColor,   &_gpsColorBad);
    F.DefineValue("TrackColor",   dtColor,   &_trackColor); 
    F.DefineValue("FlagColor",    dtColor,   &_flagColor);
    F.DefineValue("GotoColor",    dtColor,   &_gotoColor);
    F.DefineValue("HistoryColor", dtColor,   &_historyColor);
    F.DefineValue("HistoryTrackColor", dtColor, &_historyTrackColor);
    F.DefineValue("ElementsColor", dtColor,  &_elementsColor);
    F.DefineValue("PositionFlag", dtBoolean, &_positionFlag, F_TRUE);
    F.DefineValue("Inspector",    dtBoolean, &_showInspect, F_FALSE);
    F.DefineValue("SightingPointSize", dtInt32, &_sightingPointSize, "3");
    F.DefineValue("GotoPointSize", dtInt32, &_gotoPointSize, "3");
    F.DefineValue("HistoryPointSize", dtInt32, &_historyPointSize, "3");
    F.DefineValue("ElementsPointSize", dtInt32, &_elementsPointSize, "3");

    F.DefineValue("UseInspectorForGPS", dtBoolean, &_useInspectorForGPS, F_FALSE);
    F.DefineValue("OneSelect",     dtBoolean, &_oneSelect, F_FALSE);
    F.DefineValue("ShowGoto",      dtBoolean, &_showGoto, F_TRUE);
    F.DefineValue("ShowHistory",   dtBoolean, &_showHistory, F_TRUE);
    F.DefineValue("ShowHistoryTracks", dtBoolean, &_showHistoryWaypoints, F_TRUE);
    F.DefineValue("ShowSightings", dtBoolean, &_showSightings, F_TRUE);
    F.DefineValue("ShowWaypoints", dtBoolean, &_showWaypoints, F_TRUE);
    F.DefineValue("ShowElements", dtBoolean,  &_showElements, F_TRUE);
    
    F.DefineValue("GroupId", dtByte, &_groupId, 0);

    F.DefineValue("RetainState", dtBoolean, &_retainState, F_FALSE);

    if (F.IsReader())
    {
        RecalcExtent();

        CloseFileCache();

        if (_fileName && (_fileName[0] == L'\0'))
        {
            MEM_FREE(_fileName);
            _fileName = NULL;
        }

        if (IsLive() && _autoConnectGPS && !_connectedGPS)
        {
            _connectedGPS = TRUE;
            GetEngine(this)->LockGPS();
            InitLastGPS();
        }

        _flagButton->SetVisible(_positionFlag);
        _inspectButton->SetVisible(_showInspect);
        _mapsButton->SetVisible(_fileName == NULL);
        
        _followButton->SetDown(FALSE);
        _flagButton->SetDown(FALSE);
        _inspectButton->SetDown(FALSE);

        switch (_initialButtonState)
        {
        case 2: _followButton->SetDown(TRUE); break;
        case 3: if (_flagButton->GetVisible())
                {
                    _flagButton->SetDown(TRUE);
                }
                else
                {
                    _followButton->SetDown(TRUE);
                }
                break;
        case 4: if (_inspectButton->GetVisible())
                {
                    _inspectButton->SetDown(TRUE);
                }
                else
                {
                    _followButton->SetDown(TRUE);
                }
        }

        _movingMapAutoSelect = (_fileName == NULL);
        _mapList->AssignFont(&_font);
        _mapList->SetGroupId(_groupId);
    }
}

VOID CctControl_MovingMap::DefineState(CfxFiler &F)
{
    CfxControl_ZoomImageBase::DefineState(F);

    F.DefineValue("Selection", dtPBinary, _selection.GetMemoryPtr());
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);

    if (F.IsReader())
    {
        BOOL downButton = FALSE;
        F.DefineValue("FollowDown", dtBoolean, &downButton);
        _followButton->SetDown(downButton);
        F.DefineValue("FlagDown", dtBoolean, &downButton);
        _flagButton->SetDown(downButton);

        RebuildPoints();
    }
    else
    {
        BOOL downButton = _followButton->GetDown();
        F.DefineValue("FollowDown", dtBoolean, &downButton);
        downButton = _flagButton->GetDown();
        F.DefineValue("FlagDown", dtBoolean, &downButton);
    }

    // Track flag position
    if (F.DefineBegin("Flag"))
    {
        F.DefineGPS(&_flagPosition);
    }
}

VOID CctControl_MovingMap::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Auto connect GPS", dtBoolean, &_autoConnectGPS);
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button width", dtInt32,    &_buttonWidth, "MIN(16);MAX(100)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");

    F.DefineValue("Elements color", dtColor,   &_elementsColor);
    F.DefineValue("Elements point size", dtInt32, &_elementsPointSize, "MIN(3);MAX(64)");
    F.DefineValue("Elements visible", dtBoolean, &_showElements);

    F.DefineValue("Filename",     dtPTextAnsi, &_fileName,    "FILENAME(ECW \"ECW Compressed Images (*.ecw)|*.ECW\")");
    F.DefineValue("Font",         dtFont,     &_font);

    F.DefineValue("Goto color",   dtColor, &_gotoColor);
    F.DefineValue("Goto point size", dtInt32, &_gotoPointSize, "MIN(3);MAX(64)");
    F.DefineValue("Goto visible", dtBoolean, &_showGoto);

    F.DefineValue("Group filter", dtByte, &_groupId, "LIST(None=0 1 2 3 4 5 6 7 8 9 10)");

    F.DefineValue("Height",       dtInt32,    &_height);

    F.DefineValue("History color", dtColor,   &_historyColor);
    F.DefineValue("History point size", dtInt32, &_historyPointSize, "MIN(3);MAX(64)");
    F.DefineValue("History tracks color", dtColor, &_historyTrackColor);
    F.DefineValue("History tracks visible", dtBoolean, &_showHistoryWaypoints);
    F.DefineValue("History visible", dtBoolean, &_showHistory);

    F.DefineValue("Initial button state", dtByte, &_initialButtonState, "LIST(Zoom Pan Follow Flag Inspect)");
    F.DefineValue("Inspector",    dtBoolean,  &_showInspect);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Lock 100",     dtBoolean,  &_lock100);

    F.DefineValue("Marker flag color", dtColor, &_flagColor);
    F.DefineValue("Marker GPS color good",  dtColor, &_gpsColorGood);
    F.DefineValue("Marker GPS color bad",  dtColor, &_gpsColorBad);
    F.DefineValue("Marker size",   dtInt32,    &_markerSize, "MIN(3);MAX(64)");

    F.DefineValue("One select",    dtBoolean,  &_oneSelect);
    F.DefineValue("Position flag", dtBoolean,  &_positionFlag);
    F.DefineValue("Retain state",  dtBoolean,  &_retainState);

    F.DefineValue("Sighting color",      dtColor,   &_sightingColor);
    F.DefineValue("Sighting point size", dtInt32,   &_sightingPointSize, "MIN(3);MAX(64)");
    F.DefineValue("Sightings visible",   dtBoolean, &_showSightings);

    F.DefineValue("Smooth pan",    dtBoolean,  &_smoothPan);

    F.DefineValue("Timer track color",     dtColor,   &_trackColor); 
    F.DefineValue("Timer track thickness", dtInt32,  &_trackThickness, "MIN(1); MAX(4)");
    F.DefineValue("Timer tracks visible",  dtBoolean, &_showWaypoints);

    F.DefineValue("Text color",    dtColor,    &_textColor);
    F.DefineValue("Top",           dtInt32,    &_top);
    F.DefineValue("Transparent",   dtBoolean,  &_transparent);
    F.DefineValue("Use Inspect for GPS", dtBoolean, &_useInspectorForGPS);
    F.DefineValue("Width",         dtInt32,    &_width);
#endif
}

VOID CctControl_MovingMap::ResetZoom()
{
    CfxControl_ZoomImageBase::ResetZoom();

    if (_valid)
    {
        SetError(NULL);
    }
}

BOOL CctControl_MovingMap::GetImageData(UINT *pWidth, UINT *pHeight)
{
    *pWidth = *pHeight = 0;
    if (!_valid) return FALSE;

    if (_fileName && (strlen(_fileName) > 0))
    {
        if (OpenFileCache())
        {
            *pWidth = _cacheFileWidth;
            *pHeight = _cacheFileHeight;

            SetError(NULL);
        }
        else
        {
            SetError("Image not found");
        }
    }
    else
    {
        SetError("No file specified");
    }

    return (*pWidth && *pHeight);
}

VOID CctControl_MovingMap::DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect)
{
    if (!_valid) return;

    CfxCanvas *pTargetCanvas = pDstCanvas;
    CfxCanvas *pTempCanvas = NULL;

    int eError;
    
    UINT nBands;
    UINT Bands[3] = {0, 1, 2};
    UINT *buffer = NULL;

    INT w = pDstRect->Right - pDstRect->Left + 1;
    INT h = pDstRect->Bottom - pDstRect->Top + 1;
    FX_ASSERT((w > 0) && (h > 0)); 
    
    INT viewW = pSrcRect->Right - pSrcRect->Left + 1;
    INT viewH = pSrcRect->Bottom - pSrcRect->Top + 1;

    INT offsetX = pDstRect->Left;
    INT offsetY = pDstRect->Top;

    INT line;

    // Open file
    void *pNCSFileView = OpenFileCache();
    if (!pNCSFileView) goto Fail;

    // Allocate bands
    nBands = _cacheFileBands < 3 ? 1 : 3;

    // Lock at 1:1
    if ((viewW < w) || (viewH < h))
    {
        pTempCanvas = GetHost(this)->CreateCanvas(viewW, viewH);
        if (!pTempCanvas) goto Fail;
        pTargetCanvas = pTempCanvas;
        w = viewW;
        h = viewH;
        offsetX = offsetY = 0;
    }

    // Set view 
    eError = NCScbmSetFileView(
                    pNCSFileView, 
                    nBands, 
                    &Bands[0],
                    pSrcRect->Left, pSrcRect->Top, pSrcRect->Right, pSrcRect->Bottom,
                    w, h);

    if (eError != 0)
    {
        SetError("Cannot set view");
        goto Fail;
    }

    // Allocate line buffer
    buffer = (UINT *) MEM_ALLOC(w * 4);
    if (!buffer) goto Fail;
    
    //  Read each line of the compressed file
    for (line = 0; line < h; line++) 
    {
        NCSEcwReadStatus eReadStatus = NCScbmReadViewLineRGBA(pNCSFileView, buffer);
        if (eReadStatus != NCSECW_READ_OK) break;
        pTargetCanvas->ImportLine(offsetX, offsetY + line, (COLOR *)buffer, w);
    }

    if (pTargetCanvas == pTempCanvas)
    {
        pDstCanvas->StretchCopyRect(pTempCanvas,
                                    0, 0, viewW-1, viewH-1, 
                                    pDstRect->Left, pDstRect->Top, pDstRect->Right, pDstRect->Bottom);
    }

Fail:

    if (buffer)
    {
        MEM_FREE(buffer);
        buffer = NULL;
    }

    if (pTempCanvas)
    {
        delete pTempCanvas;
        pTempCanvas = NULL;
    }

    Render();
}

VOID CctControl_MovingMap::OnInspectPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    BOOL invert = _inspectButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(INT)_borderLineWidth-1, -(INT)_borderLineWidth-1);
    
    pParams->Canvas->State.LineWidth = (rect.Bottom - rect.Top + 1) < 20 ? 1 : _borderLineWidth;
    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, !invert);

    INT w, h;
    w = rect.Right - rect.Left + 1;
    h = rect.Bottom - rect.Top + 1;
    pParams->Canvas->FillRect(rect.Left + w / 2 -_lineThickness * 2,
                              rect.Top + h / 2,
                              _lineThickness * 5,
                              h / 2);

    pParams->Canvas->FillEllipse(rect.Left + w / 2,
                                 rect.Top + h / 4,
                                 _lineThickness * 2,
                                 _lineThickness * 2);

}

VOID CctControl_MovingMap::OnLocateClick(CfxControl *pSender, VOID *pParams)
{
    if (!_lastFixValid) return;

    INT x, y;
    FXGPS_POSITION *lastFix = GetEngine(this)->GetGPSLastFix();

    if (_movingMapAutoSelect)
    {
        SetMovingMap(-1, lastFix);
    }

    if (Project(lastFix->Latitude, lastFix->Longitude, &x, &y))
    {
        SetCenterSafe(x, y);
        Repaint();
    }
}

VOID CctControl_MovingMap::OnMapsClick(CfxControl * pSender, VOID *pParams)
{
    _mapList->SetVisible(TRUE);
    Repaint();
}

VOID CctControl_MovingMap::OnMapSelect(CfxControl * pSender, VOID *pParams)
{
    _mapList->SetVisible(FALSE);

    INT index = _mapList->GetSelectedIndex();
    _movingMapAutoSelect = (index == -1) ? TRUE : FALSE;
    SetMovingMap(index, GetEngine(this)->GetGPSLastFix());

    Repaint();
}

VOID CctControl_MovingMap::OnMapsPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    BOOL invert = _mapsButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(INT)_borderLineWidth-1, -(INT)_borderLineWidth-1);

    INT w = (rect.Right - rect.Left + 1);
    INT h = (rect.Bottom - rect.Top + 1);
    
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);
    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_borderColor, invert);
    pParams->Canvas->State.LineWidth = (rect.Bottom - rect.Top + 1) < 20 ? 1 : _borderLineWidth;
    pParams->Canvas->FrameRect(rect.Left, rect.Top, w, h);
    pParams->Canvas->FillRect(rect.Left, rect.Top + h / 3, w, h - 2 * (h / 3));
}

VOID CctControl_MovingMap::OnLocatePaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    BOOL invert = _locateButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(INT)_borderLineWidth-1, -(INT)_borderLineWidth-1);
    
    pParams->Canvas->State.LineWidth = (rect.Bottom - rect.Top + 1) < 20 ? 1 : _borderLineWidth;
    pParams->Canvas->DrawTriangle(&rect, _textColor, _textColor, _lastFixGood, invert, TRUE);
}

VOID CctControl_MovingMap::OnFollowPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    BOOL invert = _followButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(INT)_borderLineWidth-1, -(INT)_borderLineWidth-1);

    INT x, y;
    INT w = (rect.Right - rect.Left + 1);
    INT h = (rect.Bottom - rect.Top + 1);
    
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);
    pParams->Canvas->State.LineWidth = (rect.Bottom - rect.Top + 1) < 20 ? 1 : _borderLineWidth;
    pParams->Canvas->MoveTo(rect.Left, rect.Top);
    x = rect.Right - h/2;
    y = rect.Bottom - h/2;
    pParams->Canvas->LineTo(x, y);
    pParams->Canvas->LineTo(x, rect.Top + h / 3);
    pParams->Canvas->MoveTo(x, y);
    pParams->Canvas->LineTo(rect.Left + h/3, y);

    rect.Left += w / 4;
    rect.Top += h / 4;
    pParams->Canvas->DrawTriangle(&rect, _color, _textColor, TRUE, invert, TRUE);
}

VOID CctControl_MovingMap::OnFlagPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    BOOL invert = _flagButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(INT)_borderLineWidth-1, -(INT)_borderLineWidth-1);
    
    pParams->Canvas->State.LineWidth = (rect.Bottom - rect.Top + 1) < 20 ? 1 : _borderLineWidth;

    pParams->Canvas->DrawFlag(&rect, _color, _textColor, FALSE, invert, TRUE, TRUE);
}

VOID CctControl_MovingMap::RecalcExtent()
{
    _extent.XMin = _lon1;
    _extent.YMin = _lat1;
    _extent.XMax = _lon2;
    _extent.YMax = _lat2;

    if (_extent.XMin > _extent.XMax)
    {
        Swap(&_extent.XMin, &_extent.XMax);
    }

    if (_extent.YMin > _extent.YMax)
    {
        Swap(&_extent.YMin, &_extent.YMax);
    }

    _extentWidth = _extent.XMax - _extent.XMin;
    _extentHeight = _extent.YMax - _extent.YMin;
}  

BOOL CctControl_MovingMap::NewProjection()
{
    if ((_extentWidth == 0) || (_extentHeight == 0) ||
        (_viewPortRect.Right - _viewPortRect.Left + 1 == 0) || (_viewPortRect.Bottom - _viewPortRect.Top + 1 == 0) ||
        (_cacheCanvas->GetWidth() == 0) || (_cacheCanvas->GetHeight() == 0))
    {
        // Something went wrong
        return FALSE;
    }

    _project_scaleToImageX = (DOUBLE)_imageWidth / _extentWidth;
    _project_scaleToImageY = (DOUBLE)_imageHeight / _extentHeight;

    return TRUE;
}

BOOL CctControl_MovingMap::Project(DOUBLE Latitude, DOUBLE Longitude, INT *pX, INT *pY)
{
    if (!NewProjection())
    {
        return FALSE;
    }

    if (_projection != NULL) 
    {
        _projection->Project(Longitude, Latitude, &Longitude, &Latitude);
    }

    *pX = (INT)((Longitude - _extent.XMin) * _project_scaleToImageX);
    *pY = (INT)((_extent.YMax - Latitude) * _project_scaleToImageY);

    return TRUE;
}

BOOL CctControl_MovingMap::Unproject(INT X, INT Y, DOUBLE *pLatitude, DOUBLE *pLongitude)
{
    if (!NewProjection())
    {
        return FALSE;
    }

    *pLongitude = _extent.XMin + (DOUBLE)X / _project_scaleToImageX;
    *pLatitude = _extent.YMax - (DOUBLE)Y / _project_scaleToImageY;

    if (_projection != NULL)
    {
        _projection->Unproject(*pLongitude, *pLatitude, pLongitude, pLatitude);
    }

    return TRUE;
}

VOID CctControl_MovingMap::RebuildPoints()
{
    _items.Clear();
    _cacheProjectedPoints.Clear();

    CctSession *session = GetSession(this);

    // History
    if (_showHistory)
    {
        HISTORY_ITEM *historyItemDef;
        if (session->EnumHistoryInit(&historyItemDef))
        {
            HISTORY_ITEM *historyItem;
            UINT i = 0;
            while (session->EnumHistoryNext(&historyItem))
            {
                if ((historyItem->X == 0) && (historyItem->Y == 0))
                {
                    i++;
                    continue;
                }

                CachedPoint point;
                point.Type = (INT)mitHistory;
                Project(historyItem->Y, historyItem->X, &point.X, &point.Y);
                _cacheProjectedPoints.Write(&point, sizeof(point));

                MAP_ITEM item;
                item.Type = mitHistory;
                item.Index = i;
                item.X = historyItem->X;
                item.Y = historyItem->Y;
                _items.Write(&item, sizeof(item));

                i++;
            }
            session->EnumHistoryDone();
        }
    }

    // Goto 
    if (_showGoto)
    {
        UINT i;
        CfxResource *resource = GetResource();
        for (i = 0; i < session->GetResourceHeader()->GotosCount; i++)
        {
            FXGOTO *staticGoto = resource->GetGoto(this, i);
            if (staticGoto == NULL)
            {
                continue;
            }

            CachedPoint point;
            point.Type = (INT)mitGoto;
            Project(staticGoto->Y, staticGoto->X, &point.X, &point.Y);
            _cacheProjectedPoints.Write(&point, sizeof(point));

            MAP_ITEM item;
            item.Type = mitGoto;
            item.Index = i;
            item.X = staticGoto->X;
            item.Y = staticGoto->Y;
            _items.Write(&item, sizeof(item));

            resource->ReleaseGoto(staticGoto);
        }
    }

    // Sightings
    if (_showSightings)
    {
        CfxDatabase *database = session->GetSightingDatabase();
        CctSighting *sighting = new CctSighting(this);
        UINT i = 1;
        DBID id = INVALID_DBID;
        while ((i < database->GetCount()) && database->Enum(i, &id))
        {
            sighting->Load(database, id);
            FXGPS_POSITION *gpsPosition = sighting->GetGPS();
            if (gpsPosition->Quality != fqNone)
            {
                CachedPoint point;
                point.Type = (INT)mitSighting;
                Project(gpsPosition->Latitude, gpsPosition->Longitude, &point.X, &point.Y);
                _cacheProjectedPoints.Write(&point, sizeof(point));

                MAP_ITEM item;
                item.Type = mitSighting;
                item.Index = i;
                item.X = gpsPosition->Longitude;
                item.Y = gpsPosition->Latitude;
                _items.Write(&item, sizeof(item));
            }

            i++;
            id = INVALID_DBID;
        }
        delete database;
        delete sighting;
    }

    // Elements
    if (_showElements)
    {
        CctControl_ElementList *elementList = (CctControl_ElementList *)GetScreen(this)->FindControlByType(&GUID_CONTROL_ELEMENTLIST);
        if (elementList)
        {
            CfxResource *resource = GetResource();
            UINT i;
            for (i = 0; i < elementList->GetItemCount(); i++)
            {
                FXGOTO *gotoData = (FXGOTO *)resource->GetObject(this, elementList->GetItem(i), eaGoto);
                if (gotoData != NULL)
                {
                    CachedPoint point;
                    point.Type = (INT)mitElement;
                    Project(gotoData->Y, gotoData->X, &point.X, &point.Y);
                    _cacheProjectedPoints.Write(&point, sizeof(point));

                    MAP_ITEM item;
                    item.Type = mitElement;
                    item.Index = i;
                    item.X = gotoData->X;
                    item.Y = gotoData->Y;
                    _items.Write(&item, sizeof(item));

                    resource->ReleaseObject(gotoData);
                }
            }
        }
    }
}

VOID CctControl_MovingMap::OnEnumWaypoint(WAYPOINT *pWaypoint)
{
    CachedWaypoint item;
    item.TimeStamp = pWaypoint->DateCurrent;
    Project(pWaypoint->Latitude, pWaypoint->Longitude, &item.X, &item.Y);

    // Write to the stream
    _cacheProjectedWaypoints.Write(&item, sizeof(item));
}

VOID CctControl_MovingMap::OnEnumHistoryWaypoint(HISTORY_WAYPOINT *pHistoryWaypoint)
{
    CachedWaypoint item;
    item.TimeStamp = pHistoryWaypoint->Flags;
    Project(pHistoryWaypoint->Y, pHistoryWaypoint->X, &item.X, &item.Y);

    // Write to the stream
    _cacheProjectedHistoryWaypoints.Write(&item, sizeof(item));
}

VOID CctControl_MovingMap::Render()
{
    if (!IsLive() || !NewProjection()) return;

    // Clear the cache if the imageRect is a different size, i.e. we're not just panning.
    if (_cacheProjectedUniqueness != _projectedUniqueness)
    {
        _cacheProjectedWaypoints.Clear();
        _cacheProjectedPoints.Clear();
        _cacheProjectedHistoryWaypoints.Clear();
        _cacheProjectedUniqueness = _projectedUniqueness;
    }

    // If our cache is empty, rebuild it                
    if (_showWaypoints && (_cacheProjectedWaypoints.GetSize() == 0))
    {
        GetSession(this)->EnumWaypoints(this, (EnumWaypointCallback)&CctControl_MovingMap::OnEnumWaypoint);
    }

    // If our cache is empty, rebuild it                
    if (_showHistoryWaypoints && (_cacheProjectedHistoryWaypoints.GetSize() == 0))
    {
        GetSession(this)->EnumHistoryWaypoints(this, (EnumHistoryWaypointCallback)&CctControl_MovingMap::OnEnumHistoryWaypoint);
    }

    if (_cacheProjectedPoints.GetSize() == 0)
    {
        RebuildPoints();
    }

    // Render timer track.
    if (_showWaypoints)
    {
        _cacheCanvas->State.LineColor = _trackColor;
        _cacheCanvas->State.LineWidth = max(1, _trackThickness);

        DOUBLE lastTimeStamp = 0.0;
        DOUBLE separateDelta = EncodeTime(0, 10, 0, 0); // 10 minutes

        UINT itemCount = _cacheProjectedWaypoints.GetPosition() / sizeof(CachedWaypoint);
        CachedWaypoint *item = (CachedWaypoint *)_cacheProjectedWaypoints.GetMemory();
        while (itemCount)
        {
            INT x = item->X;
            INT y = item->Y;
            ImageToScreen(&x, &y);

            if ((lastTimeStamp == 0) || (item->TimeStamp - lastTimeStamp > separateDelta))
            {
                _cacheCanvas->MoveTo(x, y);
            }
            else if (item->TimeStamp != 0)
            {
                _cacheCanvas->LineTo(x, y);
            }

            lastTimeStamp = item->TimeStamp;
            item++;
            itemCount--;
        }
    }

    // Render timer track.
    if (_showHistoryWaypoints)
    {
        _cacheCanvas->State.LineColor = _historyTrackColor;
        _cacheCanvas->State.LineWidth = max(1, _trackThickness);

        UINT itemCount = _cacheProjectedHistoryWaypoints.GetPosition() / sizeof(CachedWaypoint);
        CachedWaypoint *item = (CachedWaypoint *)_cacheProjectedHistoryWaypoints.GetMemory();
        while (itemCount)
        {
            INT x = item->X;
            INT y = item->Y;
            UINT flags = (UINT)item->TimeStamp;
            ImageToScreen(&x, &y);

            if (flags == 1)
            {
                _cacheCanvas->MoveTo(x, y);
            }
            else
            {
                _cacheCanvas->LineTo(x, y);
            }

            item++;
            itemCount--;
        }
    }

    // Render selection
    {
        UINT i;
        _cacheCanvas->State.LineColor = 0xFF;
        _cacheCanvas->State.BrushColor = 0xFF;
        _cacheCanvas->State.LineWidth = _lineThickness;

        for (i = 0; i < _selection.GetCount(); i++)
        {
            CachedPoint *item = (CachedPoint *)_cacheProjectedPoints.GetMemory();
            item += _selection.Get(i);

            INT x = item->X;
            INT y = item->Y;
            INT pointSize;

            ImageToScreen(&x, &y);

            switch ((MapItemType)item->Type)
            {
            case mitSighting:
                pointSize = _sightingPointSize;
                break;
            case mitHistory:
                pointSize = _historyPointSize;
                break;
            case mitGoto:
                pointSize = _gotoPointSize;
                break;
            case mitElement:
                pointSize = _elementsPointSize;
                break;
            default:
                pointSize = 3;
            }            

            INT l = (pointSize + 2) * 2 * _lineThickness;
            INT l2 = (pointSize + 2) * _lineThickness;

            if (i == _selectedIndex)
            {
                _cacheCanvas->FillRect(x - l2, y - l2, l + 1, l + 1);
            }
            else
            {
                _cacheCanvas->FrameRect(x - l2, y - l2, l + 1, l + 1);
            }
        }
    }

    // Render points
    {
        UINT itemCount = _cacheProjectedPoints.GetPosition() / sizeof(CachedPoint);
        CachedPoint *item = (CachedPoint *)_cacheProjectedPoints.GetMemory();
        while (itemCount)
        {
            COLOR color;
            INT pointSize;
            INT x = item->X;
            INT y = item->Y;
            ImageToScreen(&x, &y);

            switch ((MapItemType)item->Type)
            {
            case mitSighting:
                color = _sightingColor;
                pointSize = _sightingPointSize;
                break;

            case mitHistory:
                color = _historyColor;
                pointSize = _historyPointSize;
                break;

            case mitGoto:
                color = _gotoColor;
                pointSize = _gotoPointSize;
                break;

            case mitElement:
                color = _elementsColor;
                pointSize = _elementsPointSize;
                break;

            default:
                color = 0xFFFFFF;
                pointSize = 3;
            }            

            _cacheCanvas->State.BrushColor = color;
            _cacheCanvas->FillEllipse(x, y, pointSize * _lineThickness, pointSize * _lineThickness);
            item++;
            itemCount--;
        }
    }
}

VOID CctControl_MovingMap::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (_positionFlag && (_flagPosition.Quality == fqNone))
    {
        ((CctSession *)pSender)->ShowMessage("Flag not placed");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_MovingMap::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    // Save state
    if (_retainState)
    {
        ((CctSession *)pSender)->SaveControlState(this);
    }

    if (_useInspectorForGPS && (GetSelectedItemCount() > 0))
    {
        MAP_ITEM *selectedItem = GetSelectedItem(0);
        FXGPS_POSITION position;

        position.Accuracy = 1.0;
        position.Latitude = selectedItem->Y;
        position.Longitude = selectedItem->X;
        position.Altitude = 0;
        position.Quality = fq2D;

        CctAction_SnapGPS action(this);
        action.Setup(&position);
        ((CctSession *)pSender)->ExecuteAction(&action);

        return;
    }

    // Set GPS position
    if (_positionFlag && (_flagPosition.Quality != fqNone))
    {
        CctAction_SnapGPS action(this);
        action.Setup(&_flagPosition);
        ((CctSession *)pSender)->ExecuteAction(&action);

        return;
    }
}

VOID CctControl_MovingMap::OnSessionEnterNext(CfxControl *pSender, VOID *pParams)
{
    // Load state
    if (_retainState)
    {
        ((CctSession *)pSender)->LoadControlState(this);
    }
}

VOID CctControl_MovingMap::OnSessionNewWaypoint(CfxControl *pSender, VOID *pParams)
{
    if (!NewProjection()) return;

    // Append the waypoint to the stream
    OnEnumWaypoint((WAYPOINT *)pParams);

    // Re-render the waypoints
    if (!_penDown)
    {
        Render();
    }
}

VOID CctControl_MovingMap::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    KillTimer();
    SetTimer(500);

    if (IsLive() && _movingMapAutoSelect)
    {
        SetMovingMap(-1, GetEngine(this)->GetGPSLastFix());
    }

    if (_valid && !_errorMessage)
    {
        if (pCanvas->GetBitDepth() < 16)
        {
            SetError("Color required");
        }
        else
        {
            CfxControl_ZoomImageBase::OnPaint(pCanvas, pRect);
        }
    }

    if (_errorMessage)
    {
        if (!_transparent)
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->FillRect(pRect);
        }

        CfxResource *resource = GetResource(); 

        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, _errorMessage);
            resource->ReleaseFont(font);
        }
    }
    else if (IsLive() && _lastFixValid)
    {
        CfxEngine *engine = GetEngine(this);

        // Draw the last fix
        INT x, y;
        FXGPS_POSITION *lastFix = engine->GetGPSLastFix();
        INT lastHeading = engine->GetGPSLastHeading();
        BOOL fixGood = lastFix && TestGPS(lastFix) && Project(lastFix->Latitude, lastFix->Longitude, &x, &y);
        pCanvas->State.LineWidth = _markerSize < 10 ? 1 : _borderLineWidth;

        if (fixGood)
        {
            ImageToScreen(&x, &y);
            x += _paintRect.Left;
            y += _paintRect.Top;

            //if (_alternator)//((FxGetTickCount() / FxGetTicksPerSecond()) & 1) == 0)
            {
                FXRECT rect = {x - _markerSize, y - _markerSize, x + _markerSize - 1, y + _markerSize - 1};
                pCanvas->DrawTarget(&rect, _lastFixGood ? _gpsColorGood : _gpsColorBad, _lastFixGood ? _gpsColorGood : _gpsColorBad, FALSE, lastHeading, TRUE);
            }
        }

        // Draw the goto point
        INT gotoX, gotoY;
        FXGOTO *currGoto = engine->GetCurrentGoto();
        BOOL gotoGood = currGoto && currGoto->Title[0] && Project(currGoto->Y, currGoto->X, &gotoX, &gotoY);

        if (gotoGood)
        {
            ImageToScreen(&gotoX, &gotoY);
            gotoX += _paintRect.Left;
            gotoY += _paintRect.Top;
            pCanvas->State.LineColor = _gotoColor;
            pCanvas->Ellipse(gotoX, gotoY, _markerSize / 2, _markerSize / 2);
        }

        if (fixGood && gotoGood)
        {
            pCanvas->State.LineColor = _gotoColor;
            pCanvas->Line(x, y, gotoX, gotoY);
        }
    }

    // Draw the flag marker
    if (_penDown && _flagButton->GetDown())
    {
        INT nx = _penX2;
        INT ny = _penY2;
        ScreenToImage(&nx, &ny);

        _flagPosition.Quality = fqNone;
 
        if (Unproject(nx, ny, &_flagPosition.Latitude, &_flagPosition.Longitude))
        {
            _flagPosition.Quality = fqUser2D;
        }
    }
        
    if (_positionFlag && (_flagPosition.Quality > fqNone))
    {
        INT x, y;
        if (Project(_flagPosition.Latitude, _flagPosition.Longitude, &x, &y))
        {
            ImageToScreen(&x, &y);

            FXRECT rect = {x, y - (_markerSize*2-1), x + (_markerSize*2-1), y};
            OffsetRect(&rect, _paintRect.Left, _paintRect.Top);
            pCanvas->State.LineWidth = _markerSize < 10 ? 1 : _borderLineWidth;
            pCanvas->DrawFlag(&rect, _flagColor, _borderColor, TRUE, FALSE, TRUE, FALSE);
        }
    }

    // Draw the zoom rectangle
    if (_penDown && _inspectButton->GetDown())
    {
        FXRECT focusRect = {min(_penX1, _penX2), min(_penY1, _penY2), max(_penX1, _penX2), max(_penY1, _penY2)};
        OffsetRect(&focusRect, _paintRect.Left, _paintRect.Top);

        pCanvas->State.LineColor = 0xFF;
        pCanvas->State.LineWidth = _borderLineWidth;
        pCanvas->FrameRect(&focusRect);
    } 
}

VOID CctControl_MovingMap::InitLastGPS()
{
    CfxEngine *engine = GetEngine(this);

    // Set the simulator position to the center point of the map.
    /*if (engine->GetGPSLastFix()->Quality == fqSim3D) 
    {
        DOUBLE lat, lon;
        INT x = _imageWidth / 2;
        INT y = _imageHeight / 2;
        if (Unproject(x, y, &lat, &lon))
        {
            GetHost(this)->SetupGPSSimulator(lat, lon, (UINT)-1);
        }
    }*/

    _lastFixGood = TestGPS(engine->GetGPS());
    _lastFixValid = engine->GetGPSLastFix()->Quality != fqNone;
}

VOID CctControl_MovingMap::OnTimer()
{
    KillTimer();
    InitLastGPS();

    _alternator = !_alternator;

    if (_followButton->GetDown() && _lastFixGood)
    {
        INT x, y;
        FXGPS_POSITION *lastFix = GetEngine(this)->GetGPSLastFix();

        if (_movingMapAutoSelect)
        {
            SetMovingMap(-1, lastFix);
        }

        if (Project(lastFix->Latitude, lastFix->Longitude, &x, &y))
        {
            INT b = 4 * _oneSpace;
            ImageToScreen(&x, &y);

            if ((x <= b) || (y <= b) || 
                (x >= (INT)GetWidth() - b) || (y >= (INT)GetHeight() - b))
            {
                OnLocateClick(this, NULL);
            }
        }
    }

    if (GetIsVisible())
    {
        Repaint();
    }
}

VOID CctControl_MovingMap::OnPenDown(INT X, INT Y)
{
    CfxControl_ZoomImageBase::OnPenDown(X, Y);

    Y -= _finalButtonWidth;

    if (_penDown && _flagButton->GetDown())
    {
        Repaint();
    }
}

VOID CctControl_MovingMap::OnPenUp(INT X, INT Y)
{
    if (_penDown && _flagButton->GetDown())
    {
        INT nx = X;
        INT ny = Y - _finalButtonWidth;
        ScreenToImage(&nx, &ny);
 
        if (Unproject(nx, ny, &_flagPosition.Latitude, &_flagPosition.Longitude))
        {
            _flagPosition.Quality = fqUser2D;
        }
        else
        {
            _flagPosition.Quality = fqNone;
        }
    }

    CfxControl_ZoomImageBase::OnPenUp(X, Y);
}

VOID CctControl_MovingMap::SetSelectedIndex(UINT Value)     
{ 
    if (_selectedIndex != Value) 
    { 
        _refreshCacheCanvas = TRUE; 
        _selectedIndex = Value; 
    }
}

VOID CctControl_MovingMap::BuildSelection(INT X1, INT Y1, INT X2, INT Y2)
{
    _selectedIndex = 0;
    _selection.Clear();

    X1 -= 2 * _lineThickness;
    X2 += 2 * _lineThickness;
    Y1 -= 2 * _lineThickness;
    Y2 += 2 * _lineThickness;

    UINT index = 0;
    UINT itemCount = _cacheProjectedPoints.GetPosition() / sizeof(CachedPoint);
    CachedPoint *item = (CachedPoint *)_cacheProjectedPoints.GetMemory();
    while (itemCount)
    {
        INT x = item->X;
        INT y = item->Y;
        ImageToScreen(&x, &y);

        if ((x >= X1) && (x <= X2) &&
            (y >= Y1) && (y <= Y2))
        {
            _selection.Add(index);
            if (_oneSelect)
            {
                break;
            }
        }
        item++;
        itemCount--;
        index++;
    }

    _refreshCacheCanvas = TRUE;
    Repaint();
}

VOID CctControl_MovingMap::OnPenClick(INT X, INT Y)
{
    if ((_flagPosition.Quality != fqNone) && _flagButton->GetDown() && _onFlagFix.Object)
    {
        ExecuteEvent(&_onFlagFix, this);
        return;
    }

    if (_inspectButton->GetDown())
    {
        BuildSelection(_penX1, _penY1, _penX2, _penY2);
        
        if (_onSelectionChange.Object)
        {
            ExecuteEvent(&_onSelectionChange, this);
        }
        return;
    }
}

//*************************************************************************************************
// CctControl_MapItemList

CfxControl *Create_Control_MapItemList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_MapItemList(pOwner, pParent);
}

CctControl_MapItemList::CctControl_MapItemList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_MAPITEMLIST);
    InitFont(F_FONT_DEFAULT_S);

    _itemHeight = 20;

    _sightingPainter = new CctSighting_Painter(pOwner, this);
    _historyPainter = new CctHistory_Painter(pOwner, this);
    _gotoPainter = new CctGoto_Painter(pOwner, this);
    _elementPainter = new CctGoto_Painter(pOwner, this);
}

CctControl_MapItemList::~CctControl_MapItemList()
{
    delete _sightingPainter;
    _sightingPainter = NULL;

    delete _historyPainter;
    _historyPainter = NULL;

    delete _gotoPainter;
    _gotoPainter = NULL;

    delete _elementPainter;
    _elementPainter = NULL;
}

VOID CctControl_MapItemList::SetMapItem(MAP_ITEM *pValue)
{
    if (pValue == NULL) 
    {
        SetPainter(NULL);
        UpdateScrollBar();
        return;
    }

    CctSession *session = GetSession(this);

    _mapItem = *pValue;
    switch ((MapItemType)_mapItem.Type)
    {
    case mitSighting:
        {
            _sightingPainter->ConnectByIndex(_mapItem.Index);
            SetPainter(_sightingPainter);
        }
        break;

    case mitHistory:
        {
            _historyPainter->Connect(_mapItem.Index);
            SetPainter(_historyPainter);
        }
        break;

    case mitGoto:
        {
            CfxResource *resource = GetResource();
            FXGOTO *pGoto = resource->GetGoto(this, _mapItem.Index);
            if (pGoto)
            {
                _gotoPainter->Connect(pGoto);
                resource->ReleaseGoto(pGoto);
                SetPainter(_gotoPainter);
            }
        }
        break;

    case mitElement:
        {
            CctControl_ElementList *elementList = (CctControl_ElementList *)GetScreen(this)->FindControlByType(&GUID_CONTROL_ELEMENTLIST);
            if (elementList)
            {
                CfxResource *resource = GetResource();
                FXGOTO *pGoto = (FXGOTO *)resource->GetObject(this, elementList->GetItem(_mapItem.Index), eaGoto);
                if (pGoto)
                {
                    _elementPainter->Connect(pGoto);
                    resource->ReleaseObject(pGoto);
                    SetPainter(_elementPainter);
                }
            }
        }
        break;
    }

    UpdateScrollBar();
}

UINT CctControl_MapItemList::GetItemCount()
{
    UINT result = CfxControl_List::GetItemCount();
    if (_oneLine && (result > 1))
    {
        result = 1;
    }

    return result;
}

VOID CctControl_MapItemList::OnPenDown(INT X, INT Y)
{
    INT index;
    INT offsetX, offsetY;

    if (!HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        return;
    }

    if (_painter == _sightingPainter) 
    {
        ((CctSighting_Painter *)_painter)->SetSelectedIndex(index);
    }
}

//*************************************************************************************************
// CctControl_MapInspector

CfxControl *Create_Control_MapInspector(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_MapInspector(pOwner, pParent);
}

CctControl_MapInspector::CctControl_MapInspector(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_MAPINSPECTOR);

    _activeIndex = _itemCount = 0; 
    memset(&_activeGoto, 0, sizeof(_activeGoto));
    _onChange.Object = NULL;
    _onEditClick.Object = NULL;

    _movingMap = NULL;

    InitXGuid(&_elementId);
    _required = FALSE;
    _oneLine = FALSE;
    _showType = _showButtons = TRUE;

    _buttonHeight = 20;
    _typeHeight = 20;
    _itemHeight = 20;

    _mapItemList = new CctControl_MapItemList(pOwner, this);
    _mapItemList->SetComposite(TRUE);
    _mapItemList->SetTransparent(TRUE);
    _mapItemList->SetDock(dkFill);
    _mapItemList->SetBorderStyle(bsNone);
    _mapItemList->SetShowLines(FALSE);
    _mapItemList->SetItemHeight(_itemHeight);

    _typePanel = new CfxControl_Panel(pOwner, this);
    _typePanel->SetComposite(TRUE);
    _typePanel->SetDock(dkTop);
    _typePanel->SetTextColor(0xFFFFFF);
    _typePanel->SetColor(0);
    _typePanel->SetHeight(_typeHeight);
    _typePanel->SetBounds(0, 0, _typeHeight, _typeHeight);
    _typePanel->SetBorderStyle(bsNone);
    _typePanel->SetFont(F_FONT_DEFAULT_MB);

    _buttonPanel = new CfxControl_Panel(pOwner, this);
    _buttonPanel->SetComposite(TRUE);
    _buttonPanel->SetTransparent(TRUE);
    _buttonPanel->SetBorderWidth(0);
    _buttonPanel->SetBorderStyle(bsSingle);
    _buttonPanel->SetBorderLineWidth(1);
    _buttonPanel->SetDock(dkTop);
    _buttonPanel->SetHeight(_buttonHeight);
    _buttonPanel->SetBounds(0, 0, _buttonHeight, _buttonHeight);
    _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_MapInspector::OnButtonPanelResize);
    for (UINT i=0; i<5; i++)
    {
        _buttons[i] = new CfxControl_Button(pOwner, _buttonPanel);
        _buttons[i]->SetComposite(TRUE);
        _buttons[i]->SetTransparent(TRUE);
    }
    _buttons[4]->SetTransparent(FALSE);
    _buttons[4]->SetCaption("Goto");

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_MapInspector::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_MapInspector::OnSessionNext);
}

CctControl_MapInspector::~CctControl_MapInspector()
{
    _movingMap = NULL;

    UnregisterSessionEvent(seCanNext, this);
    UnregisterSessionEvent(seNext, this);
}

VOID CctControl_MapInspector::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if ((_movingMap != NULL) && _required && (_itemCount == 0))
    {
        GetSession(this)->ShowMessage("Selection is required");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_MapInspector::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    CctSession *session = GetSession(this);

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId) && (_itemCount > 0))
    {
        CHAR value[64] = {0};
        MAP_ITEM *mapItem = _movingMap->GetSelectedItem(_activeIndex);
        switch ((MapItemType)mapItem->Type)
        {
        case mitSighting:
            break;

        case mitHistory:
            {
                HISTORY_ITEM *historyItemDef, *historyItem;
                historyItemDef = historyItem = NULL;

                if (session->EnumHistoryInit(&historyItemDef))
                {
                    session->EnumHistorySetIndex(mapItem->Index);
                    session->EnumHistoryNext(&historyItem);
                    HISTORY_ROW *historyRow = session->GetHistoryItemRow(historyItem, 0);
                    strncpy(value, (CHAR *)historyRow, sizeof(value));
                    session->EnumHistoryDone();
                }
            }
            break;

        case mitGoto:
            {
                CfxResource *resource = GetResource();
                FXGOTO *pGoto = resource->GetGoto(this, mapItem->Index);
                if (pGoto)
                {
                    strncpy(value, pGoto->Title, sizeof(value));
                    resource->ReleaseGoto(pGoto);
                }
            }
            break;

        case mitElement:
            {
                CctControl_ElementList *elementList = (CctControl_ElementList *)GetScreen(this)->FindControlByType(&GUID_CONTROL_ELEMENTLIST);
                if (elementList)
                {
                    CfxResource *resource = GetResource();
                    FXGOTO *gotoData = (FXGOTO *)resource->GetObject(this, elementList->GetItem(mapItem->Index), eaGoto);
                    if (gotoData != NULL)
                    {
                        strncpy(value, gotoData->Title, sizeof(value));
                        resource->ReleaseObject(gotoData);
                    }
                }
            }
            break;

        default:
            ;
        }
        
        if (strlen(value) > 0)
        {
            CctAction_MergeAttribute action(this);
            action.SetupAdd(_id, _elementId, dtText, &value);
            ((CctSession *)pSender)->ExecuteAction(&action);
        }
    }
}

VOID CctControl_MapInspector::UpdateMapItemList()
{
    memset(&_activeGoto, 0, sizeof(_activeGoto));

    if (_movingMap == NULL)
    {
        _itemCount = 0;
        _mapItemList->SetMapItem(NULL);
        _typePanel->SetCaption("No map");
        return;
    }

    //_buttons[4]->SetEnabled(FALSE);
    _buttons[4]->SetTransparent(FALSE);
    _movingMap->SetSelectedIndex(_activeIndex);
    _itemCount = _movingMap->GetSelectedItemCount();

    if (_itemCount == 0)
    {
        _typePanel->SetCaption("No selection");
        //_buttons[4]->SetCaption("-");
        _mapItemList->SetMapItem(NULL);
        _typePanel->SetColor(0);
    }
    else if (_activeIndex < _movingMap->GetSelectedItemCount())
    {
        _mapItemList->SetOneLine(_oneLine);

        MAP_ITEM *mapItem = _movingMap->GetSelectedItem(_activeIndex);
        _mapItemList->SetMapItem(mapItem);
        switch ((MapItemType)mapItem->Type)
        {
        case mitSighting:
            _typePanel->SetCaption("Sighting");
            _typePanel->SetColor(_movingMap->GetTrackColor());
            break;

        case mitHistory:
            _typePanel->SetCaption("History");
            _typePanel->SetColor(_movingMap->GetHistoryColor());
            break;

        case mitGoto:
            _typePanel->SetCaption("Goto");
            _typePanel->SetColor(_movingMap->GetGotoColor());
            break;

        case mitElement:
            _typePanel->SetCaption("Element");
            _typePanel->SetColor(_movingMap->GetElementsColor());
            break;

        default:
            _typePanel->SetCaption("Unknown");
        }

        strncpy(_activeGoto.Title, _typePanel->GetCaption(), ARRAYSIZE(_activeGoto.Title)-1);
        _activeGoto.X = mapItem->X;
        _activeGoto.Y = mapItem->Y;
    }

    _typePanel->SetTextColor(_typePanel->GetColor() ^ 0xFFFFFF);
}

VOID CctControl_MapInspector::OnButtonClick(CfxControl *pSender, VOID *pParams)
{
    UINT buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;

    UINT oldActiveIndex = _activeIndex;

    switch (buttonIndex)
    {
    case 0: 
        _activeIndex = 0;
        break;

    case 1:
        _activeIndex = _activeIndex > 0 ? _activeIndex - 1 : _activeIndex;
        break;

    case 2: 
        _activeIndex = _activeIndex < _itemCount - 1 ? _activeIndex + 1 : _activeIndex;
        break;

    case 3: 
        _activeIndex = _itemCount - 1;
        break;

    case 4:
        GetEngine(this)->SetCurrentGoto(&_activeGoto);
        break;
    }

    if (_itemCount == 0)
    {
        _activeIndex = oldActiveIndex;
    }

    // Fire change event
    if (oldActiveIndex != _activeIndex)
    {
        if (_onChange.Object)
        {
            ExecuteEvent(&_onChange, this);
        }

        UpdateMapItemList();
    }

    // Repaint
    Repaint();
}

VOID CctControl_MapInspector::OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    UINT buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;
    BOOL down = _buttons[buttonIndex]->GetDown();

    pParams->Canvas->State.LineWidth = GetBorderLineWidth();

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -_oneSpace, -_oneSpace);

    switch (buttonIndex)
    {
    case 0: 
        pParams->Canvas->DrawFirstArrow(&rect, _color, _borderColor, down); 
        break;

    case 1:
        pParams->Canvas->DrawBackArrow(&rect, _color, _borderColor, down); 
        break;

    case 2: 
        pParams->Canvas->DrawNextArrow(&rect, _color, _borderColor, down); 
        break;

    case 3: 
        pParams->Canvas->DrawLastArrow(&rect, _color, _borderColor, down); 
        break;
    }
}

VOID CctControl_MapInspector::OnButtonPanelResize(CfxControl *pSender, VOID *pParams)
{
    UINT buttonW = _buttonPanel->GetClientWidth() / 5;
    UINT buttonH = _buttonPanel->GetClientHeight();
    UINT tx = 0;

    for (UINT i=0; i<4; i++)
    {
        _buttons[i]->SetBounds(tx, 0, buttonW + _oneSpace, buttonH);
        tx += buttonW;
    }

    // Make sure the last button fits properly
    _buttons[4]->SetBounds(tx, 0, GetClientWidth() - tx, buttonH);
}

VOID CctControl_MapInspector::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _buttonHeight = max(ScaleHitSize, _buttonHeight);
    _buttonPanel->SetMinHeight(_buttonHeight);
}

VOID CctControl_MapInspector::DefineState(CfxFiler &F)
{
    F.DefineValue("ActiveIndex", dtInt32, &_activeIndex);
    F.DefineValue("ItemCount", dtInt32, &_itemCount);
}

VOID CctControl_MapInspector::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Panel::DefineResources(F);
    _buttons[4]->DefineResources(F);
    _mapItemList->DefineResources(F);
    _typePanel->DefineResources(F);
    F.DefineObject(_elementId, eaName);
#endif
}

VOID CctControl_MapInspector::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("OneLine",     dtBoolean, &_oneLine, F_FALSE);
    F.DefineValue("Required",    dtBoolean, &_required, F_FALSE);
    F.DefineValue("ShowButtons", dtBoolean, &_showButtons, F_TRUE);
    F.DefineValue("ShowType",    dtBoolean, &_showType, F_TRUE);
    F.DefineValue("ButtonHeight", dtInt32, &_buttonHeight, "20");
    F.DefineValue("TitleHeight", dtInt32, &_typeHeight, "20");
    F.DefineValue("ItemHeight",  dtInt32, &_itemHeight, "20");

    F.DefineXOBJECT("Element",   &_elementId);

    if (F.IsReader())
    {
        _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_MapInspector::OnButtonPanelResize);
        for (UINT i=0; i<4; i++)
        {
            _buttons[i]->SetOnClick(this, (NotifyMethod)&CctControl_MapInspector::OnButtonClick);
            _buttons[i]->SetOnPaint(this, (NotifyMethod)&CctControl_MapInspector::OnButtonPaint);
        }
        //_buttons[4]->SetOnClick((CfxControl *)(_onEditClick.Object), _onEditClick.Method);
    }

    // For edit button text
    if (F.DefineBegin("EditButton"))
    {
        _buttons[4]->DefineProperties(F);
        _buttons[4]->SetCaption("Goto");
        _buttons[4]->SetTextColor(_borderColor);
        _buttons[4]->SetOnClick(this, (NotifyMethod)&CctControl_MapInspector::OnButtonClick);
        F.DefineEnd();
    }

    // For type title.
    if (F.DefineBegin("TypePanel"))
    {
        _typePanel->DefineProperties(F);
        F.DefineEnd();
    }

    // For attributes list text
    if (F.DefineBegin("MapItemList"))
    {
        _mapItemList->DefineProperties(F);
        _mapItemList->SetTextColor(_textColor);
        //_mapItemList->SetFont(_font);
        F.DefineEnd();
    }

    if (F.IsReader())
    {
        _typePanel->SetVisible(_showType);
        _buttonPanel->SetVisible(_showButtons);

        _buttonPanel->SetHeight(_buttonHeight);
        _typePanel->SetHeight(_typeHeight);
        _typePanel->AssignFont(&_font);
        _mapItemList->SetItemHeight(_itemHeight);
        _mapItemList->AssignFont(&_font);
    }
}

VOID CctControl_MapInspector::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    CHAR items[256];
    sprintf(items, "LIST(Left=%d Center=%d Right=%d)", TEXT_ALIGN_LEFT, TEXT_ALIGN_HCENTER, TEXT_ALIGN_RIGHT);
    F.DefineValue("Alignment",    dtInt32,    &_alignment,   items);

    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button row height", dtInt32, &_buttonHeight, "MIN(16);MAX(64)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,       "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Item height",  dtInt32,    &_itemHeight, "MIN(16);MAX(64)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Minimum height", dtInt32,  &_minHeight, "MIN(0)");
    F.DefineValue("One line",     dtBoolean,  &_oneLine);
    F.DefineValue("Result Element", dtGuid,   &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Required",     dtBoolean,  &_required);
    F.DefineValue("Show buttons", dtBoolean,  &_showButtons);
    F.DefineValue("Show type",    dtBoolean,  &_showType);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Type row height", dtInt32, &_typeHeight, "MIN(16);MAX(64)");
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_MapInspector::SetOnChange(CfxControl *pControl, NotifyMethod OnClick)
{
    _onChange.Object = pControl;
    _onChange.Method = OnClick;
}

VOID CctControl_MapInspector::SetOnEditClick(CfxControl *pControl, NotifyMethod OnClick)
{
    _onEditClick.Object = pControl;
    _onEditClick.Method = OnClick;
}

VOID CctControl_MapInspector::OnSelectionChange(CfxControl *pSender, VOID *pParams)
{
    _itemCount = _activeIndex = 0;
    UpdateMapItemList();
}

VOID CctControl_MapInspector::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    CfxControl_Panel::OnPaint(pCanvas, pRect);

    if (_movingMap == NULL)
    {
        _movingMap = (CctControl_MovingMap *)GetScreen(this)->FindControlByType(&GUID_CONTROL_MOVINGMAP);
        if (_movingMap != NULL)
        {
            _movingMap->SetOnSelectionChange(this, (NotifyMethod)&CctControl_MapInspector::OnSelectionChange);
        }

        UpdateMapItemList();
    }
}

//*************************************************************************************************
// CctControl_MapList

CfxControl *Create_Control_MapList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_MapList(pOwner, pParent);
}

CctControl_MapList::CctControl_MapList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent), _itemMap()
{
    InitControl(&GUID_CONTROL_MAPLIST);
    InitLockProperties("");

    _selectedIndex = 0;
    _selectionChange = FALSE;
    _groupId = 0;

    _onSelectionChange.Object = NULL;
    SetCaption("Maps");
}

VOID CctControl_MapList::SetOnSelectionChange(CfxPersistent *pObject, NotifyMethod OnSelectionChange)
{
    _onSelectionChange.Object = pObject;
    _onSelectionChange.Method = OnSelectionChange;
}

VOID CctControl_MapList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
}

INT CctControl_MapList::GetSelectedIndex()
{ 
    if (_selectedIndex <= 0)
    {
        return -1;
    }
    else
    {
        return _itemMap.Get(_selectedIndex - 1);
    }
}

UINT CctControl_MapList::GetItemCount()
{
    UINT Count = 0;

    if (IsLive())
    {
        Count = _itemMap.GetCount();
        if (Count == 0)
        {
            CfxResource *resource = GetResource();
            for (UINT i = 0; i < resource->GetHeader(this)->MovingMapsCount; i++)
            {
                FXMOVINGMAP *m = resource->GetMovingMap(this, i);
                if (m)
                {
                    if (m->GroupId == _groupId)
                    {
                        Count++;
                        _itemMap.Add(i);
                    }

                    resource->ReleaseMovingMap(m);
                }
            }
        }

        if (Count > 0)
        {
            Count++;
        }
    }

    return Count;
}

VOID CctControl_MapList::PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index)
{
    INT itemHeight = pItemRect->Bottom - pItemRect->Top + 1;
    CHAR caption[256] = {0};

    // Paint if selected
    BOOL selected = ((INT)Index == _selectedIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);
    }

    // Draw the caption
    UINT flags = TEXT_ALIGN_VCENTER;
    FXRECT rect = *pItemRect;
    rect.Right -= 2 * _oneSpace;
    rect.Left += 2 * _oneSpace;

    pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);

    CfxResource *resource = GetResource();
    FXMOVINGMAP *movingMap = NULL;

    if (Index == 0)
    {
        strcpy(caption, "Auto");
        flags |= TEXT_ALIGN_HCENTER;
    }
    else
    {
        movingMap = resource->GetMovingMap(this, _itemMap.Get(Index - 1));
        strcpy(caption, movingMap->Name);
    }

    pCanvas->DrawTextRect(pFont, &rect, flags, caption);

    if (movingMap)
    {
        resource->ReleaseMovingMap(movingMap);
    }

    if (Index == 1)
    {
        FXRECT rect = *pItemRect;
        rect.Bottom = rect.Top + _oneSpace;
        pCanvas->State.BrushColor = _borderColor;
        pCanvas->FillRect(&rect);
    }
}

VOID CctControl_MapList::OnPenDown(INT X, INT Y)
{
    INT index;
    INT offsetX, offsetY;

    _selectionChange = FALSE;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        _selectedIndex = index;
        _selectionChange = TRUE;
        Repaint();
    }
}

VOID CctControl_MapList::OnPenClick(INT X, INT Y)
{
    if (_onSelectionChange.Object)
    {
        ExecuteEvent(&_onSelectionChange, this);
    }
}
