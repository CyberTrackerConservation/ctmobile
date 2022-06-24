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

#include "fxApplication.h"
#include "fxControl.h"
#include "fxUtils.h"
#include "ctSession.h"
#include "ctActions.h"
#include "ctElement.h"
#include "ctDialog_GPSReader.h"

//*************************************************************************************************
// CctDialog_GPSReader

CfxDialog *Create_Dialog_GPSReader(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctDialog_GPSReader(pOwner, pParent);
}

CctDialog_GPSReader::CctDialog_GPSReader(CfxPersistent *pOwner, CfxControl *pParent): CfxDialog(pOwner, pParent)
{
    InitControl(&GUID_DIALOG_GPSREADER);

    _stage = rsNormalGps;

    _dock = dkFill;

    _fixCount = 0;
    _triangleTimeOut = 0;

    _useMovingMap = _useManualGps = FALSE;

    _originalId = 0;
    InitXGuid(&_commitTargetId);

    memset(&_gps, 0, sizeof(_gps));

    _titleBar = new CfxControl_TitleBar(pOwner, this);
    _titleBar->SetComposite(TRUE);
    _titleBar->SetCaption("GPS");
    _titleBar->SetShowBattery(FALSE);
    _titleBar->SetShowExit(FALSE);
    _titleBar->SetShowOkay(TRUE);
    _titleBar->SetShowBattery(TRUE);
    _titleBar->SetDock(dkTop);
    _titleBar->SetBounds(0, 0, 24, 24);

    _spacer = new CfxControl_Panel(pOwner, this);
    _spacer->SetComposite(TRUE);
    _spacer->SetBorderStyle(bsNone);
    _spacer->SetDock(dkTop);
    _spacer->SetBounds(0, 1, 24, 4);

    _noteBook = new CfxControl_Notebook(pOwner, this);
    _noteBook->SetComposite(TRUE);
    _noteBook->SetDock(dkFill);
    _noteBook->SetCaption("Fix;Sky;Compass");
    _noteBook->ProcessPages();
    _noteBook->SetActivePageIndex(5);
    _noteBook->SetOnPageChange(this, (NotifyMethod)&CctDialog_GPSReader::OnNoteBookPageChange);

    CfxControl_GPS *gps;

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(0));
    gps->SetBorderStyle(bsNone);
    gps->SetDock(dkTop);
    gps->SetStyle(gcsState);    
    gps->SetBounds(0, 0, 100, 32);
    gps->SetAutoHeight(TRUE);
    gps->SetFont(F_FONT_DEFAULT_LB);

    _travelMode = new CfxControl_Panel(pOwner, this);
    _travelMode->SetBorderStyle(bsNone);
    _travelMode->SetDock(dkTop);
    _travelMode->SetBounds(0, 0, 100, 32);
    _travelMode->SetAutoHeight(TRUE);
    _travelMode->SetFont(F_FONT_DEFAULT_MB);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(0));
    gps->SetBorderStyle(bsNone);
    gps->SetDock(dkFill);
    gps->SetStyle(gcsData);
    gps->SetExtraData(TRUE);
    gps->SetFont(F_FONT_DEFAULT_LB);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(1));
    gps->SetBorderStyle(bsNone);
    gps->SetDock(dkTop);
    gps->SetStyle(gcsState);    
    gps->SetBounds(0, 0, 100, 32);
    gps->SetFont(F_FONT_DEFAULT_LB);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(1));
    gps->SetBorderStyle(bsNone);
    gps->SetBorderWidth(4);
    gps->SetDock(dkFill);
    gps->SetStyle(gcsSky);
    gps->SetFont(F_FONT_FIXED_S);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(1));
    gps->SetBorderStyle(bsNone);
    gps->SetBorderWidth(4);
    gps->SetDock(dkBottom);
    gps->SetBounds(0, 0, 100, 64);
    gps->SetStyle(gcsBars);    
    gps->SetFont(F_FONT_FIXED_S);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(2));
    gps->SetBorderStyle(bsNone);
    gps->SetDock(dkTop);
    gps->SetStyle(gcsState);
    gps->SetBounds(0, 0, 100, 32);
    gps->SetFont(F_FONT_DEFAULT_LB);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(2));
    gps->SetBorderStyle(bsNone);
    gps->SetBorderWidth(4);
    gps->SetDock(dkFill);
    gps->SetStyle(gcsCompass2);
    gps->SetFont(F_FONT_DEFAULT_M);

    gps = new CfxControl_GPS(pOwner, _noteBook->GetPage(2));
    gps->SetBorderStyle(bsNone);
    gps->SetDock(dkBottom);
    gps->SetBounds(0, 0, 100, 20);
    gps->SetStyle(gcsHeading);
    gps->SetBorderWidth(2);
    gps->SetAutoHeight(TRUE);
    gps->SetFont(F_FONT_DEFAULT_M);

    _movingMap = new CctControl_MovingMap(pOwner, this);
    _movingMap->SetComposite(TRUE);
    _movingMap->SetDock(dkFill);
    _movingMap->SetVisible(FALSE);
    _movingMap->SetPositionFlag(TRUE);
    _movingMap->SetBounds(0, 10, 24, 24);

    _manualGps = new CctControl_ElementList(pOwner, this);
    _manualGps->SetId(100);
    _manualGps->SetComposite(TRUE);
    _manualGps->SetDock(dkFill);
    _manualGps->SetVisible(FALSE);
    _manualGps->SetListMode(elmGps);
    _manualGps->SetBounds(0, 10, 24, 24);
    _manualGps->SetFont(F_FONT_DEFAULT_LB);

    _manualButton = new CfxControl_Button(pOwner, _manualGps);
    _manualButton->SetComposite(TRUE);
    _manualButton->SetBorderStyle(bsSingle);
    _manualButton->SetOnClick(this, (NotifyMethod)&CctDialog_GPSReader::OnManualClick);
    _manualButton->SetCaption("Confirm");
    _manualButton->SetFont(F_FONT_DEFAULT_LB);
    _manualButton->SetBounds(0, 0, 24, 32);
    _manualButton->SetVisible(FALSE);
    _manualButton->SetBorderStyle(bsRound1);

    _skipButton = new CfxControl_Button(pOwner, this);
    _skipButton->SetComposite(TRUE);
    _skipButton->SetBorderStyle(bsSingle);
    _skipButton->SetDock(dkBottom);
    _skipButton->SetOnClick(this, (NotifyMethod)&CctDialog_GPSReader::OnSkipClick);
    _skipButton->SetCaption("Skip GPS");
    _skipButton->SetBounds(0, 0, 24, 32);
    _skipButton->SetFont(F_FONT_DEFAULT_LB);

    if (IsLive())
    {
        GetEngine(this)->LockGPS();
    }

    SetTimer(1000);
}

CctDialog_GPSReader::~CctDialog_GPSReader()
{
    KillTimer();

    if (IsLive())
    {
        GetEngine(this)->UnlockGPS();
        GetHost(this)->ShowSkyplot(0, 0, 0, 0, false);
    }
}

VOID CctDialog_GPSReader::OnNoteBookPageChange(CfxControl * pSender, VOID *pParams)
{
    if (!_noteBook)
    {
        return;
    }

    auto skyPage = _noteBook->GetPage(1);
    skyPage->ClearControls();

    FXRECT r;
    skyPage->GetAbsoluteBounds(&r);

    GetHost(this)->ShowSkyplot(r.Left, r.Top, r.Right - r.Left + 1, r.Bottom - r.Top + 1, _noteBook->GetActivePageIndex() == 2);
}

VOID CctDialog_GPSReader::ClearDialogControls()
{
    _movingMap = NULL;
    _manualGps = NULL;
    _manualButton = NULL;
    _skipButton = NULL;
    _travelMode = NULL;
    _noteBook = NULL;

    GetHost(this)->ShowSkyplot(0, 0, 0, 0, false);

    ClearControls();
}

VOID CctDialog_GPSReader::Init(CfxControl *pSender, VOID *pParam, UINT ParamSize)
{ 
    CctSession *session = GetSession(this);
    CTRESOURCEHEADER *header = session->GetResourceHeader();

    memset(&_gps, 0, sizeof(_gps));
    _fixCount = _triangleTimeOut = 0;

    GPS_READER_CONTEXT context;
    FX_ASSERT(ParamSize == sizeof(context));
    memcpy(&context, pParam, ParamSize);

    _originalId = GetSession(this)->GetCurrentSightingId();
    _commitTargetId = context.SaveTarget; 
    _skipTimeout = context.SkipTimeout;

    _useMovingMap = GetResource() && 
                    (GetCanvas(this)->GetBitDepth() > 1) &&
                    (header->MovingMapsCount > 0) &&
                    header->ManualMapOnSkip &&
                    (_movingMap != NULL);

    _useManualGps = GetResource() && 
                    header->ManualOnSkip && 
                    (_manualGps != NULL) &&
                    (_manualButton != NULL);

    FX_ASSERT(_fixCount == 0);
    FX_ASSERT(_triangleTimeOut == 0);
    FXGPS *gps = GetEngine(this)->GetGPS();
    ConstructFix(gps);
    if (FixSufficient(gps))
    {
        _triangleTimeOut = FxGetTickCount();
        ClearDialogControls();
    }
    else
    {
        SetStage(rsNormalGps);
    }
}

VOID CctDialog_GPSReader::PostInit(CfxControl *pSender)
{
    OnNoteBookPageChange(this, NULL);
}

VOID CctDialog_GPSReader::OnCloseDialog(BOOL *pHandled)
{
    *pHandled = TRUE;

    ClearDialogControls();

    GetSession(this)->Reconnect(_originalId);
}

VOID CctDialog_GPSReader::CommitWithPosition(FXGPS_POSITION *pGPS, BOOL DoRepaint)
{
    KillTimer();

    // Note: reconnect will kill this control, so we have to store everything on the stack
    CctSession *session = GetSession(this);
    FXGPS_POSITION position = *pGPS;
    FXGPS_POSITION *sightingPosition = session->GetCurrentSighting()->GetGPS();

    XGUID commitTargetId = _commitTargetId;
    session->Reconnect(_originalId, FALSE);

    memcpy(sightingPosition, &position, sizeof(FXGPS_POSITION));
    session->DoCommit(commitTargetId, DoRepaint);
}


VOID CctDialog_GPSReader::OnSkipClick(CfxControl *pSender, VOID *pParams)
{
    // Ensure the timer is done
    KillTimer();

    BOOL complete = FALSE;

    switch (_stage)
    {
    case rsNormalGps:
        if (TestGPS(&_gps))
        {
            CommitWithPosition(&_gps.Position, TRUE);
            return;
        }
        else if (_useManualGps)
        {
            SetStage(rsManualGps);
        }
        else if (_useMovingMap)
        {
            SetStage(rsMovingMap);
        }
        else 
        {
            complete = TRUE;
        }
        break;

    case rsManualGps:
        if (_useMovingMap)
        {
            SetStage(rsMovingMap);
        }
        else
        {
            complete = TRUE;
        }
        break;

    case rsMovingMap:
        complete = TRUE;
        break;
    }

    if (complete)
    {
        ClearDialogControls();

        CctSession *session = GetSession(this);
        XGUID commitTargetId = _commitTargetId;
        session->Reconnect(_originalId, FALSE);
        session->DoCommit(commitTargetId);
    }
}

VOID CctDialog_GPSReader::OnManualClick(CfxControl *pSender, VOID *pParams)
{
    FXGPS_POSITION position;
    _manualGps->GetPosition(&position);
    CommitWithPosition(&position, TRUE);
}

VOID CctDialog_GPSReader::OnMovingMapFix(CfxControl *pSender, VOID *pParams)
{
    CommitWithPosition(_movingMap->GetFlagPosition(), TRUE);
}

VOID CctDialog_GPSReader::SetStage(GPS_READER_STAGE Value)
{
    CctSession *session = GetSession(this);
    _stage = Value;
    _titleBar->SetCaption("GPS");

    switch (_stage)
    {
    case rsNormalGps:
        _skipButton->SetVisible(_skipTimeout <= 0);
        _manualButton->SetVisible(FALSE);
        _noteBook->SetVisible(TRUE);

        _travelMode->SetParent(_noteBook->GetPage(0));
        if (session->GetSightingAccuracyName())
        {
            _travelMode->SetVisible(TRUE);
            _travelMode->SetCaption(session->GetSightingAccuracyName());
        }
        else
        {
            _travelMode->SetVisible(FALSE);
        }

        PropagateGPSAccuracy(_noteBook->GetPage(0), session->GetSightingAccuracy());

        if (_manualGps) _manualGps->SetVisible(FALSE);
        if (_movingMap) _movingMap->SetVisible(FALSE);

        break;

    case rsManualGps:
        _skipButton->SetVisible(GetSession(this)->GetResourceHeader()->ManualAllowSkip);
        _noteBook->SetVisible(FALSE);
        _manualButton->SetVisible(TRUE);
        if (_manualGps) _manualGps->SetVisible(TRUE);
        if (_movingMap) _movingMap->SetVisible(FALSE);
        break;

    case rsMovingMap:
        _skipButton->SetVisible(TRUE);
        _manualButton->SetVisible(FALSE);
        _noteBook->SetVisible(FALSE);
        if (_manualGps) _manualGps->SetVisible(FALSE);
        if (_movingMap) _movingMap->SetVisible(TRUE);
        
        _movingMap->SetOnFlagFix(this, (NotifyMethod)&CctDialog_GPSReader::OnMovingMapFix);
        _movingMap->LoadGlobalState();

        break;
    }

    if (IsLive())
    {
        GetEngine(this)->AlignControls(this);

        if (_manualButton && _manualButton->GetVisible())
        {
            _manualButton->SetBounds(16 * _oneSpace, 
                                     (_manualGps->GetItemCount() + 1) * _manualGps->GetItemHeight(),
                                     _manualGps->GetWidth() - 32 * _oneSpace,
                                     _manualGps->GetItemHeight() * 2);
        }

        Repaint();
    }
}

VOID CctDialog_GPSReader::DefineState(CfxFiler &F)
{
    F.DefineValue("Stage", dtByte, &_stage);
    F.DefineValue("SkipTimeOut", dtInt32, &_skipTimeout);
    F.DefineValue("UseManualGps", dtBoolean, &_useManualGps);
    F.DefineValue("UseMovingMap", dtBoolean, &_useMovingMap);
    F.DefineValue("OriginalId", dtInt32, &_originalId);
    F.DefineXOBJECT("CommitTargetId", &_commitTargetId);

    _manualGps->DefineState(F);

    if (F.IsReader())
    {
        SetStage(_stage);
    }
}

VOID CctDialog_GPSReader::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);
    if (F.DefineBegin("Title"))
    {
        _titleBar->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Spacer"))
    {
        _spacer->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Notebook"))
    {
        _noteBook->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("MovingMap"))
    {
        _movingMap->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("SkipButton"))
    {
        _skipButton->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("ManualGps"))
    {
        _manualGps->DefineProperties(F);
        _manualGps->SetId(100);
        F.DefineEnd();
    }

    if (F.DefineBegin("ManualButton"))
    {
        _manualButton->DefineProperties(F);
        F.DefineEnd();
    }
}

VOID CctDialog_GPSReader::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    _titleBar->DefineResources(F);
    _noteBook->DefineResources(F);
    _movingMap->DefineResources(F);
    _skipButton->DefineResources(F);
    _manualGps->DefineResources(F);
    _manualButton->DefineResources(F);
    _travelMode->DefineResources(F);
#endif
}

VOID CctDialog_GPSReader::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!IsLive()) return;

    if (GetControlCount() == 0)
    {
        FXRECT triRect = *pRect;
        if (triRect.Right - triRect.Left > triRect.Bottom - triRect.Top)
        {
            triRect.Right = triRect.Left + (triRect.Bottom - triRect.Top + 1);
        }
        else
        {
            triRect.Bottom = triRect.Top + (triRect.Right - triRect.Left + 1);
        }
        UINT triRadius = (triRect.Right - triRect.Left + 1)/4-1;
        OffsetRect(&triRect, GetClientWidth()/2 - triRadius, GetClientHeight()/2 - triRadius);

        FXPOINTLIST polygon;
        FXPOINT triangle[] = {{triRadius, 0}, {triRadius*2, triRadius*2}, {0, triRadius*2}};

        pCanvas->State.BrushColor = _borderColor;
        FILLPOLYGON(pCanvas, triangle, triRect.Left, triRect.Top);
    }
}

BOOL CctDialog_GPSReader::FixSufficient(FXGPS *pGPS)
{
    CctSession *session = GetSession(this);

    if (session->GetResourceHeader()->GpsTimeSync && !pGPS->TimeInSync)
    {
        return FALSE;
    }

    if (_fixCount < GetSession(this)->GetSightingFixCount())
    {
        return FALSE;
    }

    return TRUE;
}

VOID CctDialog_GPSReader::ConstructFix(FXGPS *pGPS)
{
    if ((pGPS->Position.Accuracy < _gps.Position.Accuracy) ||
        (pGPS->Position.Quality > _gps.Position.Quality))
    {
        _gps = *pGPS;
    }

    if (TestGPS(pGPS, GetSession(this)->GetSightingAccuracy()))
    {
        _fixCount++;
    }
}

VOID CctDialog_GPSReader::OnTimer()
{
    CfxEngine *engine = GetEngine(this);

    if (GetControlCount() == 0)
    {
        if (FxGetTickCount() - _triangleTimeOut > 2000)
        {
            CommitWithPosition(&_gps.Position, FALSE);

            engine->Repaint();
            // Note: this control is invalid - do not add any more code here
        }

        return;
    }

    CctSession *session = GetSession(this);
    BOOL fixSufficient = FALSE;

    fixSufficient = FixSufficient(&_gps);

    if (!fixSufficient)
    {
        FXGPS *gps = engine->GetGPS();
        UINT requiredCount;
    
        ConstructFix(gps);

        fixSufficient = FixSufficient(gps);
        requiredCount = session->GetSightingFixCount();
        if (!fixSufficient && (requiredCount > 1))
        {
            CHAR caption[32];
            sprintf(caption, "GPS - %lu%% complete", min(100, (_fixCount * 100) / requiredCount)); 
            _titleBar->SetCaption(caption);
        }
    }

    if (_skipButton && !_skipButton->GetVisible())
    {
        _skipTimeout--;
        if (_skipTimeout <= 0)
        {
            _skipButton->SetVisible(TRUE);
            engine->AlignControls(this);
        }
    }

    // ClearControls and set triangle timeout if fix conditions are met
    if (fixSufficient && (_triangleTimeOut == 0))
    {
        ClearDialogControls();
        _triangleTimeOut = FxGetTickCount();
    }

    Repaint();
}
