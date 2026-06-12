#include "fxPlatform.h"
#include "ECW.h"

#include "fxApplication.h"
#include "fxUtils.h"
#include "fxGPS.h"
#include "fxMath.h"
#include "fxTranslate.h"
#include "ctMovingMap.h"
#include "ctSession.h"
#include "ctActions.h"
#include "ctElementList.h"

//*************************************************************************************************

#pragma pack(push, 1) 
struct CachedWaypoint
{
    double TimeStamp = 0.0;
    int X = 0, Y = 0;
};

struct CachedPoint
{
    int Type = 0;
    int X = 0, Y = 0;
};
#pragma pack(pop) 

bool g_bInit = false;
bool g_bValid = false;

bool InitLibrary()
{
    if (g_bInit) return true;

    g_bInit = true;
    NCSecwInit();

    return true;
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

    _alternator = false;

    _smoothPan = false;

    _initialButtonState = 2; // Follow

    _projection = nullptr;

    _connectedGPS = false;
    _autoConnectGPS = true;
    _lastFixGood = _lastFixValid = false;

    _showInspect = false;
    _selectedIndex = 0;
    _onSelectionChange.Object = 0;

    _positionFlag = true;
    memset(&_flagPosition, 0, sizeof(FXGPS_POSITION));

    _onFlagFix.Object = 0;

    _lat1 = _lat2 = _lon1 = _lon2 = 0.0;
    memset(&_extent, 0, sizeof(FXEXTENT));
    _extentWidth = _extentHeight = 0;
    
    _fileName = 0;
    _movingMapAutoSelect = true;
    _movingMapIndex = -1;
    _errorMessage = nullptr;
    _cacheFile = nullptr;
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

    _oneSelect = false;
    _showSightings = _showWaypoints = _showHistory = _showGoto = _showElements = true;
    _showHistoryWaypoints = true;
    _useInspectorForGPS = false;

    _retainState = false;

    // Create buttons
    _locateButton = new CfxControl_Button(pOwner, this);
    _locateButton->SetComposite(true);
    _locateButton->SetOnClick(this, (NotifyMethod)&CctControl_MovingMap::OnLocateClick);
    _locateButton->SetOnPaint(this, (NotifyMethod)&CctControl_MovingMap::OnLocatePaint);

    _followButton = new CfxControl_Button(pOwner, this);
    _followButton->SetComposite(true);
    _followButton->SetGroupId(1);
    _followButton->SetOnPaint(this, (NotifyMethod)&CctControl_MovingMap::OnFollowPaint);

    _flagButton = new CfxControl_Button(pOwner, this);
    _flagButton->SetComposite(true);
    _flagButton->SetGroupId(1);
    _flagButton->SetOnPaint(this, (NotifyMethod)&CctControl_MovingMap::OnFlagPaint);
    _flagButton->SetVisible(true);

    _inspectButton = new CfxControl_Button(pOwner, this);
    _inspectButton->SetComposite(true);
    _inspectButton->SetGroupId(1);
    _inspectButton->SetOnPaint(this, (NotifyMethod)&CctControl_MovingMap::OnInspectPaint);

    _mapsButton = new CfxControl_Button(pOwner, this);
    _mapsButton->SetComposite(true);
    _mapsButton->SetOnPaint(this, (NotifyMethod)&CctControl_MovingMap::OnMapsPaint);
    _mapsButton->SetOnClick(this, (NotifyMethod)&CctControl_MovingMap::OnMapsClick);

    // Map list
    _mapList = new CctControl_MapList(pOwner, this);
    _mapList->SetComposite(true);
    _mapList->SetVisible(false);
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
}

CctControl_MovingMap::~CctControl_MovingMap()
{
    KillTimer();

    if (_connectedGPS)
    {
        GetEngine(this)->UnlockGPS();
    }

    UnregisterSessionEvent(seEnterNext, this);
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);
    
    delete _projection;
    _projection = nullptr;

    SetFileName(nullptr);
}

void CctControl_MovingMap::SetFileName(char *pFileName)
{
    CloseFileCache();

    MEM_FREE(_fileName);
    _fileName = nullptr;
    MEM_FREE(_errorMessage);
    _errorMessage = nullptr;

    if (pFileName)
    {
        _fileName = (char *) MEM_ALLOC(strlen(pFileName) + 1);
        strcpy(_fileName, pFileName);
    }
}

int CctControl_MovingMap::FindBestMovingMapIndex(FXGPS_POSITION *pGPS)
{		
    double lastDiff = 361;
    int index = -1;

    CfxResource *resource = GetResource();
    for (int i = 0; i < resource->GetHeader(this)->MovingMapsCount; i++)
    {
        FXMOVINGMAP *m = resource->GetMovingMap(this, i);

        // Skip bad maps
        if (m == nullptr)
        {
            continue;
        }

        // Skip from out of group
        if (m->GroupId != _groupId)
        {
            continue;
        }

        // It needs to not be inside
        if (pGPS->Quality == FXGPS_QUALITY::None)
        {
            index = i;
            break;
        }

        if (pGPS->Longitude < m->Xa || pGPS->Longitude > m->Xb || pGPS->Latitude < m->Yb || pGPS->Latitude > m->Ya)
        {
            continue;
        }
            
        // Find highest resolution
        double currentDiff = m->Xb - m->Xa;
        if (currentDiff < lastDiff)
        {
            index = i;
        }
    }
    
    // Can't find one.
    return index;
}

void CctControl_MovingMap::SetCenterSafe(int x, int y)
{
    bool inBounds = (x >= 0) && (x < (int)_imageWidth) &&
                    (y >= 0) && (y < (int)_imageHeight);

    if (inBounds)
    {
        SetCenter(x, y);
    }
    else
    {
        SetCenter(_imageWidth / 2, _imageHeight / 2);
    }
}

void CctControl_MovingMap::SetMovingMap(int Index, FXGPS_POSITION *pGPS)
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

    if (_movingMapIndex != (int)Index)
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

            _refreshCacheCanvas = true;
            _imageWidth = _imageHeight = 0;
            GetImageData(&_imageWidth, &_imageHeight);
            
            resource->ReleaseMovingMap(m);
        }

        int x, y;
        if ((pGPS->Quality != FXGPS_QUALITY::None) && Project(pGPS->Latitude, pGPS->Longitude, &x, &y))
        {
            SetCenterSafe(x, y);
        }

        Repaint();
    }
}

void CctControl_MovingMap::SetOnSelectionChange(CfxControl *pControl, NotifyMethod OnClick)
{
    _onSelectionChange.Object = pControl;
    _onSelectionChange.Method = OnClick;
}

void CctControl_MovingMap::SetOnFlagFix(CfxControl *pControl, NotifyMethod OnClick)
{
    _onFlagFix.Object = pControl;
    _onFlagFix.Method = OnClick;
}

void CctControl_MovingMap::SetCoords(double Lon1, double Lat1, double Lon2, double Lat2)
{
    _lon1 = Lon1;
    _lon2 = Lon2;
    _lat1 = Lat1;
    _lat2 = Lat2;

    RecalcExtent();
}

void CctControl_MovingMap::SetPositionFlag(bool Value) 
{ 
    _positionFlag = Value; 
    _flagButton->SetVisible(Value); 
}

void CctControl_MovingMap::PlaceButtons()
{
    if ((_width < (int)_buttonWidth) || (_height < (int)_buttonWidth)) return;

    // Position the toolbar buttons
    int tx = _borderWidth;
    int ty = _borderWidth;
    int bw = _buttonWidth;
    int buttonCount = 10 - (_inspectButton->GetVisible() ? 0 : 1) - (_flagButton->GetVisible() ? 0 : 1) - (_mapsButton->GetVisible() ? 0 : 1);
    int bwm1 = _buttonWidth - _borderLineWidth;

    int spacerCount = 1 + (_mapsButton->GetVisible() ? 1 : 0);
    int spacerWidth = spacerCount * 4 * _borderLineWidth;
    if (bwm1 * buttonCount + spacerWidth > GetClientWidth())
    {
        bwm1 = (GetClientWidth() - spacerWidth) / buttonCount;
        bw = bwm1 + _borderLineWidth;
    }

    spacerWidth = GetClientWidth() - bwm1 * buttonCount - _borderLineWidth;

    int spacer1, spacer2;
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

void CctControl_MovingMap::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_ZoomImageBase::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _mapList->Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _markerSize = max(5, (_markerSize * ScaleX) / 100);
    _trackThickness = max(1, (_trackThickness * ScaleBorder) / 100);
    _lineThickness = max(1, (_lineThickness * ScaleBorder) / 100);
}

void *CctControl_MovingMap::OpenFileCache()
{
    if ((_cacheFile != nullptr) || (_fileName == nullptr))
    {
        return _cacheFile;
    }

    void *pNCSFileView = nullptr;
    char *fileName = GetHost(this)->AllocPathApplication(_fileName);
    int eError = NCScbmOpenFileView(fileName, &pNCSFileView);
    MEM_FREE(fileName);

    if (eError != 0) 
    {
        return _cacheFile;
    }

    char projectionCode[64] = {0};
    int projectionValue = 0;
    NCSFileViewFileInfo *pNCSFileInfo = nullptr;
    NCScbmGetViewFileInfo(pNCSFileView, &pNCSFileInfo);

    _cacheFile = pNCSFileView;
    _cacheFileWidth = pNCSFileInfo->nSizeX;
    _cacheFileHeight = pNCSFileInfo->nSizeY;
    _cacheFileBands = pNCSFileInfo->nBands;
    _extentWidth = _extentHeight = 0;

    delete _projection;
    _projection = nullptr;

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

void CctControl_MovingMap::CloseFileCache()
{
    if (_cacheFile)
    {
        NCScbmCloseFileView(_cacheFile);
        _cacheFile = nullptr;
        _cacheFileWidth = _cacheFileHeight = _cacheFileBands = 0;
    }
    _cacheProjectedWaypoints.Clear();
    _cacheProjectedPoints.Clear();
    _cacheProjectedHistoryWaypoints.Clear();
}

void CctControl_MovingMap::SetError(const char *pError)
{
    MEM_FREE(_errorMessage);
    _errorMessage = nullptr;

    if (pError)
    {
        _errorMessage = (char *) MEM_ALLOC(strlen(pError) + 1);
        strcpy(_errorMessage, pError);
    }
}

void CctControl_MovingMap::LoadGlobalState()
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

void CctControl_MovingMap::SaveGlobalState()
{
    if (!IsLive()) return;

    CfxStream stream;
    CfxWriter writer(&stream);
    DefineState(writer);
    GetSession(this)->SetStoreItem(_typeId, -1, stream.CloneMemory());
}

void CctControl_MovingMap::DefineResources(CfxFilerResource &F)
{
}

void CctControl_MovingMap::DefineProperties(CfxFiler &F)
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
            _fileName = nullptr;
        }

        if (IsLive() && _autoConnectGPS && !_connectedGPS)
        {
            _connectedGPS = true;
            GetEngine(this)->LockGPS();
            InitLastGPS();
        }

        _flagButton->SetVisible(_positionFlag);
        _inspectButton->SetVisible(_showInspect);
        _mapsButton->SetVisible(_fileName == nullptr);
        
        _followButton->SetDown(false);
        _flagButton->SetDown(false);
        _inspectButton->SetDown(false);

        switch (_initialButtonState)
        {
        case 2: _followButton->SetDown(true); break;
        case 3: if (_flagButton->GetVisible())
                {
                    _flagButton->SetDown(true);
                }
                else
                {
                    _followButton->SetDown(true);
                }
                break;
        case 4: if (_inspectButton->GetVisible())
                {
                    _inspectButton->SetDown(true);
                }
                else
                {
                    _followButton->SetDown(true);
                }
        }

        _movingMapAutoSelect = (_fileName == nullptr);
        _mapList->AssignFont(&_font);
        _mapList->SetGroupId(_groupId);
    }
}

void CctControl_MovingMap::DefineState(CfxFiler &F)
{
    CfxControl_ZoomImageBase::DefineState(F);

    F.DefineValue("Selection", dtPBinary, _selection.GetMemoryPtr());
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);

    if (F.IsReader())
    {
        bool downButton = false;
        F.DefineValue("FollowDown", dtBoolean, &downButton);
        _followButton->SetDown(downButton);
        F.DefineValue("FlagDown", dtBoolean, &downButton);
        _flagButton->SetDown(downButton);

        RebuildPoints();
    }
    else
    {
        bool downButton = _followButton->GetDown();
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

void CctControl_MovingMap::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_MovingMap::ResetZoom()
{
    CfxControl_ZoomImageBase::ResetZoom();

    if (_valid)
    {
        SetError(nullptr);
    }
}

bool CctControl_MovingMap::GetImageData(int *pWidth, int *pHeight)
{
    *pWidth = *pHeight = 0;
    if (!_valid) return false;

    if (_fileName && (strlen(_fileName) > 0))
    {
        if (OpenFileCache())
        {
            *pWidth = _cacheFileWidth;
            *pHeight = _cacheFileHeight;

            SetError(nullptr);
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

void CctControl_MovingMap::DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect)
{
    if (!_valid) return;

    CfxCanvas *pTargetCanvas = pDstCanvas;
    CfxCanvas *pTempCanvas = nullptr;

    int eError;
    
    uint32_t nBands;
    uint32_t Bands[3] = {0, 1, 2};
    uint32_t *buffer = nullptr;

    int w = pDstRect->Right - pDstRect->Left + 1;
    int h = pDstRect->Bottom - pDstRect->Top + 1;
    FX_ASSERT((w > 0) && (h > 0)); 
    
    uint32_t viewW = (uint32_t)(pSrcRect->Right - pSrcRect->Left + 1);
    uint32_t viewH = (uint32_t)(pSrcRect->Bottom - pSrcRect->Top + 1);

    int offsetX = pDstRect->Left;
    int offsetY = pDstRect->Top;

    int line;

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
    buffer = (uint32_t *) MEM_ALLOC(w * 4);
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
        buffer = nullptr;
    }

    if (pTempCanvas)
    {
        delete pTempCanvas;
        pTempCanvas = nullptr;
    }

    Render();
}

void CctControl_MovingMap::OnInspectPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    bool invert = _inspectButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(int)_borderLineWidth-1, -(int)_borderLineWidth-1);
    
    pParams->Canvas->State.LineWidth = (rect.Bottom - rect.Top + 1) < 20 ? 1 : _borderLineWidth;
    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, !invert);

    int w, h;
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

void CctControl_MovingMap::OnLocateClick(CfxControl *pSender, void *pParams)
{
    if (!_lastFixValid) return;

    int x, y;
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

void CctControl_MovingMap::OnMapsClick(CfxControl * pSender, void *pParams)
{
    _mapList->SetVisible(true);
    Repaint();
}

void CctControl_MovingMap::OnMapSelect(CfxControl * pSender, void *pParams)
{
    _mapList->SetVisible(false);

    int index = _mapList->GetSelectedIndex();
    _movingMapAutoSelect = (index == -1) ? true : false;
    SetMovingMap(index, GetEngine(this)->GetGPSLastFix());

    Repaint();
}

void CctControl_MovingMap::OnMapsPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    bool invert = _mapsButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(int)_borderLineWidth-1, -(int)_borderLineWidth-1);

    int w = (rect.Right - rect.Left + 1);
    int h = (rect.Bottom - rect.Top + 1);
    
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);
    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_borderColor, invert);
    pParams->Canvas->State.LineWidth = (rect.Bottom - rect.Top + 1) < 20 ? 1 : _borderLineWidth;
    pParams->Canvas->FrameRect(rect.Left, rect.Top, w, h);
    pParams->Canvas->FillRect(rect.Left, rect.Top + h / 3, w, h - 2 * (h / 3));
}

void CctControl_MovingMap::OnLocatePaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    bool invert = _locateButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(int)_borderLineWidth-1, -(int)_borderLineWidth-1);
    
    pParams->Canvas->State.LineWidth = (rect.Bottom - rect.Top + 1) < 20 ? 1 : _borderLineWidth;
    pParams->Canvas->DrawTriangle(&rect, _textColor, _textColor, _lastFixGood, invert, true);
}

void CctControl_MovingMap::OnFollowPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    bool invert = _followButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(int)_borderLineWidth-1, -(int)_borderLineWidth-1);

    int x, y;
    int w = (rect.Right - rect.Left + 1);
    int h = (rect.Bottom - rect.Top + 1);
    
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
    pParams->Canvas->DrawTriangle(&rect, _color, _textColor, true, invert, true);
}

void CctControl_MovingMap::OnFlagPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    bool invert = _flagButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(int)_borderLineWidth-1, -(int)_borderLineWidth-1);
    
    pParams->Canvas->State.LineWidth = (rect.Bottom - rect.Top + 1) < 20 ? 1 : _borderLineWidth;

    pParams->Canvas->DrawFlag(&rect, _color, _textColor, false, invert, true, true);
}

void CctControl_MovingMap::RecalcExtent()
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

bool CctControl_MovingMap::NewProjection()
{
    if ((_extentWidth == 0) || (_extentHeight == 0) ||
        (_viewPortRect.Right - _viewPortRect.Left + 1 == 0) || (_viewPortRect.Bottom - _viewPortRect.Top + 1 == 0) ||
        (_cacheCanvas->GetWidth() == 0) || (_cacheCanvas->GetHeight() == 0))
    {
        // Something went wrong
        return false;
    }

    _project_scaleToImageX = (double)_imageWidth / _extentWidth;
    _project_scaleToImageY = (double)_imageHeight / _extentHeight;

    return true;
}

bool CctControl_MovingMap::Project(double Latitude, double Longitude, int *pX, int *pY)
{
    if (!NewProjection())
    {
        return false;
    }

    if (_projection != nullptr) 
    {
        _projection->Project(Longitude, Latitude, &Longitude, &Latitude);
    }

    *pX = (int)((Longitude - _extent.XMin) * _project_scaleToImageX);
    *pY = (int)((_extent.YMax - Latitude) * _project_scaleToImageY);

    return true;
}

bool CctControl_MovingMap::Unproject(int X, int Y, double *pLatitude, double *pLongitude)
{
    if (!NewProjection())
    {
        return false;
    }

    *pLongitude = _extent.XMin + (double)X / _project_scaleToImageX;
    *pLatitude = _extent.YMax - (double)Y / _project_scaleToImageY;

    if (_projection != nullptr)
    {
        _projection->Unproject(*pLongitude, *pLatitude, pLongitude, pLatitude);
    }

    return true;
}

void CctControl_MovingMap::RebuildPoints()
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
            int i = 0;
            while (session->EnumHistoryNext(&historyItem))
            {
                if ((historyItem->X == 0) && (historyItem->Y == 0))
                {
                    i++;
                    continue;
                }

                CachedPoint point;
                point.Type = (int)MapItemType::History;
                Project(historyItem->Y, historyItem->X, &point.X, &point.Y);
                _cacheProjectedPoints.Write(&point, sizeof(point));

                MAP_ITEM item;
                item.Type = MapItemType::History;
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
        int i;
        CfxResource *resource = GetResource();
        for (i = 0; i < session->GetResourceHeader()->GotosCount; i++)
        {
            FXGOTO *staticGoto = resource->GetGoto(this, i);
            if (staticGoto == nullptr)
            {
                continue;
            }

            CachedPoint point;
            point.Type = (int)MapItemType::Goto;
            Project(staticGoto->Y, staticGoto->X, &point.X, &point.Y);
            _cacheProjectedPoints.Write(&point, sizeof(point));

            MAP_ITEM item;
            item.Type = MapItemType::Goto;
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
        auto database = session->CreateSightingDatabasePtr();
        auto sighting = std::unique_ptr<CctSighting>(new CctSighting(this));
        int i = 0;
        QString uid;
        while (database->Enum(i, &uid))
        {
            qFatalIf(!database->Load(sighting.get(), uid), "Must load");

            FXGPS_POSITION *gpsPosition = sighting->GetGPS();
            if (gpsPosition->Quality != FXGPS_QUALITY::None)
            {
                CachedPoint point;
                point.Type = (int)MapItemType::Sighting;
                Project(gpsPosition->Latitude, gpsPosition->Longitude, &point.X, &point.Y);
                _cacheProjectedPoints.Write(&point, sizeof(point));

                MAP_ITEM item;
                item.Type = MapItemType::Sighting;
                item.Index = i;
                item.X = gpsPosition->Longitude;
                item.Y = gpsPosition->Latitude;
                _items.Write(&item, sizeof(item));
            }

            i++;
        }
    }

    // Elements
    if (_showElements)
    {
        CctControl_ElementList *elementList = (CctControl_ElementList *)GetScreen(this)->FindControlByType(&GUID_CONTROL_ELEMENTLIST);
        if (elementList)
        {
            CfxResource *resource = GetResource();
            int i;
            for (i = 0; i < elementList->GetItemCount(); i++)
            {
                FXGOTO *gotoData = (FXGOTO *)resource->GetObject(this, elementList->GetItem(i), (BYTE)ElementAttribute::Goto);
                if (gotoData != nullptr)
                {
                    CachedPoint point;
                    point.Type = (int)MapItemType::Element;
                    Project(gotoData->Y, gotoData->X, &point.X, &point.Y);
                    _cacheProjectedPoints.Write(&point, sizeof(point));

                    MAP_ITEM item;
                    item.Type = MapItemType::Element;
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

void CctControl_MovingMap::OnEnumWaypoint(WAYPOINT *pWaypoint)
{
    CachedWaypoint item;
    item.TimeStamp = pWaypoint->DateCurrent;
    Project(pWaypoint->Latitude, pWaypoint->Longitude, &item.X, &item.Y);

    // Write to the stream
    _cacheProjectedWaypoints.Write(&item, sizeof(item));
}

void CctControl_MovingMap::OnEnumHistoryWaypoint(HISTORY_WAYPOINT *pHistoryWaypoint)
{
    CachedWaypoint item;
    item.TimeStamp = pHistoryWaypoint->Flags;
    Project(pHistoryWaypoint->Y, pHistoryWaypoint->X, &item.X, &item.Y);

    // Write to the stream
    _cacheProjectedHistoryWaypoints.Write(&item, sizeof(item));
}

void CctControl_MovingMap::Render()
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

        double lastTimeStamp = 0.0;
        double separateDelta = EncodeTime(0, 10, 0, 0); // 10 minutes

        int itemCount = _cacheProjectedWaypoints.GetPosition() / sizeof(CachedWaypoint);
        CachedWaypoint *item = (CachedWaypoint *)_cacheProjectedWaypoints.GetMemory();
        while (itemCount)
        {
            int x = item->X;
            int y = item->Y;
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

        int itemCount = _cacheProjectedHistoryWaypoints.GetPosition() / sizeof(CachedWaypoint);
        CachedWaypoint *item = (CachedWaypoint *)_cacheProjectedHistoryWaypoints.GetMemory();
        while (itemCount)
        {
            int x = item->X;
            int y = item->Y;
            int flags = (int)item->TimeStamp;
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
        int i;
        _cacheCanvas->State.LineColor = 0xFF;
        _cacheCanvas->State.BrushColor = 0xFF;
        _cacheCanvas->State.LineWidth = _lineThickness;

        for (i = 0; i < _selection.GetCount(); i++)
        {
            CachedPoint *item = (CachedPoint *)_cacheProjectedPoints.GetMemory();
            item += _selection.Get(i);

            int x = item->X;
            int y = item->Y;
            int pointSize;

            ImageToScreen(&x, &y);

            switch ((MapItemType)item->Type)
            {
            case MapItemType::Sighting:
                pointSize = _sightingPointSize;
                break;
            case MapItemType::History:
                pointSize = _historyPointSize;
                break;
            case MapItemType::Goto:
                pointSize = _gotoPointSize;
                break;
            case MapItemType::Element:
                pointSize = _elementsPointSize;
                break;
            default:
                pointSize = 3;
            }            

            int l = (pointSize + 2) * 2 * _lineThickness;
            int l2 = (pointSize + 2) * _lineThickness;

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
        int itemCount = _cacheProjectedPoints.GetPosition() / sizeof(CachedPoint);
        CachedPoint *item = (CachedPoint *)_cacheProjectedPoints.GetMemory();
        while (itemCount)
        {
            COLOR color;
            int pointSize;
            int x = item->X;
            int y = item->Y;
            ImageToScreen(&x, &y);

            switch ((MapItemType)item->Type)
            {
            case MapItemType::Sighting:
                color = _sightingColor;
                pointSize = _sightingPointSize;
                break;

            case MapItemType::History:
                color = _historyColor;
                pointSize = _historyPointSize;
                break;

            case MapItemType::Goto:
                color = _gotoColor;
                pointSize = _gotoPointSize;
                break;

            case MapItemType::Element:
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

void CctControl_MovingMap::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if (_positionFlag && (_flagPosition.Quality == FXGPS_QUALITY::None))
    {
        ((CctSession *)pSender)->ShowMessage(fxTranslate("Flag not placed"));
        *((bool *)pParams) = false;
    }
}

void CctControl_MovingMap::OnSessionNext(CfxControl *pSender, void *pParams)
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
        position.Quality = FXGPS_QUALITY::Fix2D;

        CctAction_SnapGPS action(this);
        action.Setup(&position);
        ((CctSession *)pSender)->ExecuteAction(&action);

        return;
    }

    // Set GPS position
    if (_positionFlag && (_flagPosition.Quality != FXGPS_QUALITY::None))
    {
        CctAction_SnapGPS action(this);
        action.Setup(&_flagPosition);
        ((CctSession *)pSender)->ExecuteAction(&action);

        return;
    }
}

void CctControl_MovingMap::OnSessionEnterNext(CfxControl *pSender, void *pParams)
{
    // Load state
    if (_retainState)
    {
        ((CctSession *)pSender)->LoadControlState(this);
    }
}

void CctControl_MovingMap::OnSessionNewWaypoint(CfxControl *pSender, void *pParams)
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

void CctControl_MovingMap::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
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
        int x, y;
        FXGPS_POSITION *lastFix = engine->GetGPSLastFix();
        int lastHeading = engine->GetGPSLastHeading();
        bool fixGood = lastFix && TestGPS(lastFix) && Project(lastFix->Latitude, lastFix->Longitude, &x, &y);
        pCanvas->State.LineWidth = _markerSize < 10 ? 1 : _borderLineWidth;

        if (fixGood)
        {
            ImageToScreen(&x, &y);
            x += _paintRect.Left;
            y += _paintRect.Top;

            //if (_alternator)//((FxGetTickCount() / FxGetTicksPerSecond()) & 1) == 0)
            {
                FXRECT rect = {x - _markerSize, y - _markerSize, x + _markerSize - 1, y + _markerSize - 1};
                pCanvas->DrawTarget(&rect, _lastFixGood ? _gpsColorGood : _gpsColorBad, _lastFixGood ? _gpsColorGood : _gpsColorBad, false, lastHeading, true);
            }
        }

        // Draw the goto point
        int gotoX, gotoY;
        FXGOTO *currGoto = engine->GetCurrentGoto();
        bool gotoGood = currGoto && currGoto->Title[0] && Project(currGoto->Y, currGoto->X, &gotoX, &gotoY);

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
        int nx = _penX2;
        int ny = _penY2;
        ScreenToImage(&nx, &ny);

        _flagPosition.Quality = FXGPS_QUALITY::None;
 
        if (Unproject(nx, ny, &_flagPosition.Latitude, &_flagPosition.Longitude))
        {
            _flagPosition.Quality = FXGPS_QUALITY::User2D;
        }
    }
        
    if (_positionFlag && (_flagPosition.Quality > FXGPS_QUALITY::None))
    {
        int x, y;
        if (Project(_flagPosition.Latitude, _flagPosition.Longitude, &x, &y))
        {
            ImageToScreen(&x, &y);

            FXRECT rect = {x, y - (_markerSize*2-1), x + (_markerSize*2-1), y};
            OffsetRect(&rect, _paintRect.Left, _paintRect.Top);
            pCanvas->State.LineWidth = _markerSize < 10 ? 1 : _borderLineWidth;
            pCanvas->DrawFlag(&rect, _flagColor, _borderColor, true, false, true, false);
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

void CctControl_MovingMap::InitLastGPS()
{
    CfxEngine *engine = GetEngine(this);

    // Set the simulator position to the center point of the map.
    /*if (engine->GetGPSLastFix()->Quality == FXGPS_QUALITY::Sim3D) 
    {
        double lat, lon;
        int x = _imageWidth / 2;
        int y = _imageHeight / 2;
        if (Unproject(x, y, &lat, &lon))
        {
            GetHost(this)->SetupGPSSimulator(lat, lon, (int)-1);
        }
    }*/

    _lastFixGood = TestGPS(engine->GetGPS());
    _lastFixValid = engine->GetGPSLastFix()->Quality != FXGPS_QUALITY::None;
}

void CctControl_MovingMap::OnTimer()
{
    KillTimer();
    InitLastGPS();

    _alternator = !_alternator;

    if (_followButton->GetDown() && _lastFixGood)
    {
        int x, y;
        FXGPS_POSITION *lastFix = GetEngine(this)->GetGPSLastFix();

        if (_movingMapAutoSelect)
        {
            SetMovingMap(-1, lastFix);
        }

        if (Project(lastFix->Latitude, lastFix->Longitude, &x, &y))
        {
            int b = 4 * _oneSpace;
            ImageToScreen(&x, &y);

            if ((x <= b) || (y <= b) || 
                (x >= (int)GetWidth() - b) || (y >= (int)GetHeight() - b))
            {
                OnLocateClick(this, nullptr);
            }
        }
    }

    if (GetIsVisible())
    {
        Repaint();
    }
}

void CctControl_MovingMap::OnPenDown(int X, int Y)
{
    CfxControl_ZoomImageBase::OnPenDown(X, Y);

    Y -= _finalButtonWidth;

    if (_penDown && _flagButton->GetDown())
    {
        Repaint();
    }
}

void CctControl_MovingMap::OnPenUp(int X, int Y)
{
    if (_penDown && _flagButton->GetDown())
    {
        int nx = X;
        int ny = Y - _finalButtonWidth;
        ScreenToImage(&nx, &ny);
 
        if (Unproject(nx, ny, &_flagPosition.Latitude, &_flagPosition.Longitude))
        {
            _flagPosition.Quality = FXGPS_QUALITY::User2D;
        }
        else
        {
            _flagPosition.Quality = FXGPS_QUALITY::None;
        }
    }

    CfxControl_ZoomImageBase::OnPenUp(X, Y);
}

void CctControl_MovingMap::SetSelectedIndex(int Value)     
{ 
    if (_selectedIndex != Value) 
    { 
        _refreshCacheCanvas = true; 
        _selectedIndex = Value; 
    }
}

void CctControl_MovingMap::BuildSelection(int X1, int Y1, int X2, int Y2)
{
    _selectedIndex = 0;
    _selection.Clear();

    X1 -= 2 * _lineThickness;
    X2 += 2 * _lineThickness;
    Y1 -= 2 * _lineThickness;
    Y2 += 2 * _lineThickness;

    int index = 0;
    int itemCount = _cacheProjectedPoints.GetPosition() / sizeof(CachedPoint);
    CachedPoint *item = (CachedPoint *)_cacheProjectedPoints.GetMemory();
    while (itemCount)
    {
        int x = item->X;
        int y = item->Y;
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

    _refreshCacheCanvas = true;
    Repaint();
}

void CctControl_MovingMap::OnPenClick(int X, int Y)
{
    if ((_flagPosition.Quality != FXGPS_QUALITY::None) && _flagButton->GetDown() && _onFlagFix.Object)
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
    _sightingPainter = nullptr;

    delete _historyPainter;
    _historyPainter = nullptr;

    delete _gotoPainter;
    _gotoPainter = nullptr;

    delete _elementPainter;
    _elementPainter = nullptr;
}

void CctControl_MapItemList::SetMapItem(MAP_ITEM *pValue)
{
    if (pValue == nullptr) 
    {
        SetPainter(nullptr);
        UpdateScrollBar();
        return;
    }

    CctSession *session = GetSession(this);

    _mapItem = *pValue;
    switch ((MapItemType)_mapItem.Type)
    {
    case MapItemType::Sighting:
        {
            _sightingPainter->ConnectByIndex(_mapItem.Index);
            SetPainter(_sightingPainter);
        }
        break;

    case MapItemType::History:
        {
            _historyPainter->Connect(_mapItem.Index);
            SetPainter(_historyPainter);
        }
        break;

    case MapItemType::Goto:
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

    case MapItemType::Element:
        {
            CctControl_ElementList *elementList = (CctControl_ElementList *)GetScreen(this)->FindControlByType(&GUID_CONTROL_ELEMENTLIST);
            if (elementList)
            {
                CfxResource *resource = GetResource();
                FXGOTO *pGoto = (FXGOTO *)resource->GetObject(this, elementList->GetItem(_mapItem.Index), (BYTE)ElementAttribute::Goto);
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

int CctControl_MapItemList::GetItemCount()
{
    int result = CfxControl_List::GetItemCount();
    if (_oneLine && (result > 1))
    {
        result = 1;
    }

    return result;
}

void CctControl_MapItemList::OnPenClick(int X, int Y)
{
    int index;
    int offsetX, offsetY;

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
    _onChange.Object = nullptr;
    _onEditClick.Object = nullptr;

    _movingMap = nullptr;

    InitXGuid(&_elementId);
    _required = false;
    _oneLine = false;
    _showType = _showButtons = true;

    _buttonHeight = 20;
    _typeHeight = 20;
    _itemHeight = 20;

    _mapItemList = new CctControl_MapItemList(pOwner, this);
    _mapItemList->SetComposite(true);
    _mapItemList->SetTransparent(true);
    _mapItemList->SetDock(ControlDock::Fill);
    _mapItemList->SetBorderStyle(ControlBorderStyle::None);
    _mapItemList->SetShowLines(false);
    _mapItemList->SetItemHeight(_itemHeight);

    _typePanel = new CfxControl_Panel(pOwner, this);
    _typePanel->SetComposite(true);
    _typePanel->SetDock(ControlDock::Top);
    _typePanel->SetTextColor(0xFFFFFF);
    _typePanel->SetColor(0);
    _typePanel->SetHeight(_typeHeight);
    _typePanel->SetBounds(0, 0, _typeHeight, _typeHeight);
    _typePanel->SetBorderStyle(ControlBorderStyle::None);
    _typePanel->SetFont(F_FONT_DEFAULT_MB);

    _buttonPanel = new CfxControl_Panel(pOwner, this);
    _buttonPanel->SetComposite(true);
    _buttonPanel->SetTransparent(true);
    _buttonPanel->SetBorderWidth(0);
    _buttonPanel->SetBorderStyle(ControlBorderStyle::Single);
    _buttonPanel->SetBorderLineWidth(1);
    _buttonPanel->SetDock(ControlDock::Top);
    _buttonPanel->SetHeight(_buttonHeight);
    _buttonPanel->SetBounds(0, 0, _buttonHeight, _buttonHeight);
    _buttonPanel->SetOnResize(this, (NotifyMethod)&CctControl_MapInspector::OnButtonPanelResize);
    for (int i=0; i<5; i++)
    {
        _buttons[i] = new CfxControl_Button(pOwner, _buttonPanel);
        _buttons[i]->SetComposite(true);
        _buttons[i]->SetTransparent(true);
    }
    _buttons[4]->SetTransparent(false);
    _buttons[4]->SetCaption("Goto");

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_MapInspector::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_MapInspector::OnSessionNext);
}

CctControl_MapInspector::~CctControl_MapInspector()
{
    _movingMap = nullptr;

    UnregisterSessionEvent(seCanNext, this);
    UnregisterSessionEvent(seNext, this);
}

void CctControl_MapInspector::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if ((_movingMap != nullptr) && _required && (_itemCount == 0))
    {
        GetSession(this)->ShowMessage(fxTranslate("Selection is required"));
        *((bool *)pParams) = false;
    }
}

void CctControl_MapInspector::OnSessionNext(CfxControl *pSender, void *pParams)
{
    CctSession *session = GetSession(this);

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId) && (_itemCount > 0))
    {
        char value[64] = {0};
        MAP_ITEM *mapItem = _movingMap->GetSelectedItem(_activeIndex);
        switch ((MapItemType)mapItem->Type)
        {
        case MapItemType::Sighting:
            break;

        case MapItemType::History:
            {
                HISTORY_ITEM *historyItemDef, *historyItem;
                historyItemDef = historyItem = nullptr;

                if (session->EnumHistoryInit(&historyItemDef))
                {
                    session->EnumHistorySetIndex(mapItem->Index);
                    session->EnumHistoryNext(&historyItem);
                    HISTORY_ROW *historyRow = session->GetHistoryItemRow(historyItem, 0);
                    strncpy(value, (char *)historyRow, sizeof(value));
                    session->EnumHistoryDone();
                }
            }
            break;

        case MapItemType::Goto:
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

        case MapItemType::Element:
            {
                CctControl_ElementList *elementList = (CctControl_ElementList *)GetScreen(this)->FindControlByType(&GUID_CONTROL_ELEMENTLIST);
                if (elementList)
                {
                    CfxResource *resource = GetResource();
                    FXGOTO *gotoData = (FXGOTO *)resource->GetObject(this, elementList->GetItem(mapItem->Index), (BYTE)ElementAttribute::Goto);
                    if (gotoData != nullptr)
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

void CctControl_MapInspector::UpdateMapItemList()
{
    memset(&_activeGoto, 0, sizeof(_activeGoto));

    if (_movingMap == nullptr)
    {
        _itemCount = 0;
        _mapItemList->SetMapItem(nullptr);
        _typePanel->SetCaption("No map");
        return;
    }

    //_buttons[4]->SetEnabled(false);
    _buttons[4]->SetTransparent(false);
    _movingMap->SetSelectedIndex(_activeIndex);
    _itemCount = _movingMap->GetSelectedItemCount();

    if (_itemCount == 0)
    {
        _typePanel->SetCaption("No selection");
        //_buttons[4]->SetCaption("-");
        _mapItemList->SetMapItem(nullptr);
        _typePanel->SetColor(0);
    }
    else if (_activeIndex < _movingMap->GetSelectedItemCount())
    {
        _mapItemList->SetOneLine(_oneLine);

        MAP_ITEM *mapItem = _movingMap->GetSelectedItem(_activeIndex);
        _mapItemList->SetMapItem(mapItem);
        switch ((MapItemType)mapItem->Type)
        {
        case MapItemType::Sighting:
            _typePanel->SetCaption("Sighting");
            _typePanel->SetColor(_movingMap->GetTrackColor());
            break;

        case MapItemType::History:
            _typePanel->SetCaption("History");
            _typePanel->SetColor(_movingMap->GetHistoryColor());
            break;

        case MapItemType::Goto:
            _typePanel->SetCaption("Goto");
            _typePanel->SetColor(_movingMap->GetGotoColor());
            break;

        case MapItemType::Element:
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

void CctControl_MapInspector::OnButtonClick(CfxControl *pSender, void *pParams)
{
    int buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;

    int oldActiveIndex = _activeIndex;

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

void CctControl_MapInspector::OnButtonPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    int buttonIndex = 0;
    while (pSender != _buttons[buttonIndex]) buttonIndex++;
    bool down = _buttons[buttonIndex]->GetDown();

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

void CctControl_MapInspector::OnButtonPanelResize(CfxControl *pSender, void *pParams)
{
    int buttonW = _buttonPanel->GetClientWidth() / 5;
    int buttonH = _buttonPanel->GetClientHeight();
    int tx = 0;

    for (int i=0; i<4; i++)
    {
        _buttons[i]->SetBounds(tx, 0, buttonW + _oneSpace, buttonH);
        tx += buttonW;
    }

    // Make sure the last button fits properly
    _buttons[4]->SetBounds(tx, 0, GetClientWidth() - tx, buttonH);
}

void CctControl_MapInspector::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _buttonHeight = max(ScaleHitSize, _buttonHeight);
    _buttonPanel->SetMinHeight(_buttonHeight);
}

void CctControl_MapInspector::DefineState(CfxFiler &F)
{
    F.DefineValue("ActiveIndex", dtInt32, &_activeIndex);
    F.DefineValue("ItemCount", dtInt32, &_itemCount);
}

void CctControl_MapInspector::DefineResources(CfxFilerResource &F)
{
}

void CctControl_MapInspector::DefineProperties(CfxFiler &F)
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
        for (int i=0; i<4; i++)
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

void CctControl_MapInspector::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_MapInspector::SetOnChange(CfxControl *pControl, NotifyMethod OnClick)
{
    _onChange.Object = pControl;
    _onChange.Method = OnClick;
}

void CctControl_MapInspector::SetOnEditClick(CfxControl *pControl, NotifyMethod OnClick)
{
    _onEditClick.Object = pControl;
    _onEditClick.Method = OnClick;
}

void CctControl_MapInspector::OnSelectionChange(CfxControl *pSender, void *pParams)
{
    _itemCount = _activeIndex = 0;
    UpdateMapItemList();
}

void CctControl_MapInspector::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    CfxControl_Panel::OnPaint(pCanvas, pRect);

    if (_movingMap == nullptr)
    {
        _movingMap = (CctControl_MovingMap *)GetScreen(this)->FindControlByType(&GUID_CONTROL_MOVINGMAP);
        if (_movingMap != nullptr)
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
    _selectionChange = false;
    _groupId = 0;

    _onSelectionChange.Object = nullptr;
    SetCaption("Maps");
}

void CctControl_MapList::SetOnSelectionChange(CfxPersistent *pObject, NotifyMethod OnSelectionChange)
{
    _onSelectionChange.Object = pObject;
    _onSelectionChange.Method = OnSelectionChange;
}

void CctControl_MapList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
}

int CctControl_MapList::GetSelectedIndex()
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

int CctControl_MapList::GetItemCount()
{
    int Count = 0;

    if (IsLive())
    {
        Count = _itemMap.GetCount();
        if (Count == 0)
        {
            CfxResource *resource = GetResource();
            for (int i = 0; i < resource->GetHeader(this)->MovingMapsCount; i++)
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

void CctControl_MapList::PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index)
{
    int itemHeight = pItemRect->Bottom - pItemRect->Top + 1;
    char caption[256] = {0};

    // Paint if selected
    bool selected = ((int)Index == _selectedIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);
    }

    // Draw the caption
    int flags = TEXT_ALIGN_VCENTER;
    FXRECT rect = *pItemRect;
    rect.Right -= 2 * _oneSpace;
    rect.Left += 2 * _oneSpace;

    pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);

    CfxResource *resource = GetResource();
    FXMOVINGMAP *movingMap = nullptr;

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

void CctControl_MapList::OnPenClick(int X, int Y)
{
    int index;
    int offsetX, offsetY;

    _selectionChange = false;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        _selectedIndex = index;
        _selectionChange = true;
        Repaint();
    }

    if (_onSelectionChange.Object)
    {
        ExecuteEvent(&_onSelectionChange, this);
    }
}
