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

#include "fxEngine.h"
#include "fxUtils.h"
#include "fxApplication.h"
#include "fxHost.h"
#include "fxMath.h"

// The GPS is a global resource
INT g_gpsLockCount = 0;

// The RangeFinder is a global resource
INT g_rangeFinderLockCount = 0;

CfxEngine::CfxEngine(CfxPersistent *pOwner): CfxPersistent(pOwner), _timers(), _alarms(), _ports(), _transfers(), _urlRequests()
{
    _captureControl = NULL;
    _lastUserEventTime = 0;
    _portsUniqueness = 0;
    _terminating = FALSE;
    _cornerWidth1 = 4;
    _cornerWidth2 = 8;

    _scaleScroller = _scaleTitle = _scaleTab = 100;

    _inPaint = FALSE;
    _blockPaintCount = 0;
    _paintRequested = FALSE;

    _paintTimerElapse = 0;

    memset(&_gps, 0, sizeof(_gps));
    memset(&_gpsLastFix, 0, sizeof(_gpsLastFix));
    _gpsLastHeading = 0;
    memset(&_gpsLastTimeStamp, 0, sizeof(_gpsLastTimeStamp));
    memset(&_range, 0, sizeof(_range));

    _gpsTimeFirst = TRUE;
    _gpsTimeSync = FALSE;
    _gpsTimeZone = 0;
    _gpsTimeUsedOnce = FALSE;

    _recorder = NULL;

    _useRangeFinderForAltitude = FALSE;
    _alarmSet = FALSE;

    _currentGoto = (FXGOTO *)MEM_ALLOC(sizeof(FXGOTO));
    memset(_currentGoto, 0, sizeof(FXGOTO));
    _trackTimer = NULL;
}

CfxEngine::~CfxEngine()
{
    KillTimer(this);
    
    MEM_FREE(_currentGoto);
    _currentGoto = NULL;

    // Cleanup URL data requests.
    for (UINT i = 0; i < _urlRequests.GetCount(); i++)
    {
        URL_REQUEST_ITEM *item = _urlRequests.GetPtr(i);
        MEM_FREE(item->Data);
    }
}

VOID CfxEngine::PreDecorate(CfxControl *pControl, FXRECT *pRect)
{
    ControlBorderStyle borderStyle = pControl->GetBorderStyle();

    if (borderStyle < bsRound1) return;

    FXRECT rect;
    pControl->GetAbsoluteBounds(&rect);

    switch (pControl->GetBorderStyle())
    {
    case bsRound1:
        _paintCanvas->PushCorners(&rect, _cornerWidth1);
        break;

    case bsRound2:
        _paintCanvas->PushCorners(&rect, _cornerWidth2);
        break;
    }
}

VOID CfxEngine::PostDecorate(CfxControl *pControl, FXRECT *pRect)
{
    if ((pControl->GetBorderWidth() == 0) && (pControl->GetBorderStyle() == bsNone)) return;

    FXRECT rect;
    UINT borderWidth = (INT)pControl->GetBorderWidth();
    UINT borderLineWidth = (INT)pControl->GetBorderLineWidth();
    UINT lastLineWidth = _paintCanvas->State.LineWidth;
    _paintCanvas->State.LineWidth = 1;

    // Paint the inside border
    if ((borderWidth > 0) && !pControl->GetTransparent())
    {
        rect = *pRect;
        _paintCanvas->State.LineColor = pControl->GetColor();
        for (UINT i=0; i<min(borderWidth, 64); i++)
        {
            _paintCanvas->FrameRect(&rect);
            InflateRect(&rect, -1, -1);
        }
    }

    // Paint the outside border
    _paintCanvas->State.LineColor = pControl->GetBorderColor();
    _paintCanvas->State.LineWidth = borderLineWidth;

    rect = *pRect;
    switch (pControl->GetBorderStyle())
    {
    case bsNone:   
        break;

    case bsSingle:
        _paintCanvas->FrameRect(&rect);
        break;

    case bsRound1:
        _paintCanvas->RoundRect(&rect, _cornerWidth1);
        pControl->GetAbsoluteBounds(&rect);
        _paintCanvas->PopCorners(&rect); 
        break;

    case bsRound2:
        _paintCanvas->RoundRect(&rect, _cornerWidth2);
        pControl->GetAbsoluteBounds(&rect);
        _paintCanvas->PopCorners(&rect); 
        break;
    }

    _paintCanvas->State.LineWidth = lastLineWidth;
}

//*************************************************************************************************
// Dock Controls

class CControlDocker
{
protected:
    CfxControl *_parent;
    TList<CfxControl *> _dockList;
    FXRECT _rect;
    UINT _scaleX, _scaleY, _scaleBorder, _scaleHitSize;
protected:
    BOOL InsertBefore(CfxControl *pControl1, CfxControl *pControl2, ControlDock Dock);
    VOID DoPosition(CfxControl *pControl, ControlDock Dock);
    VOID DoAlign(ControlDock Dock);
    BOOL AlignWork();
    VOID AlignSubControls(CfxControl *pParent);
    VOID InternalExecute(CfxControl *pParent);
    VOID InternalResetScale(CfxControl *pControl);
    VOID InternalScale(CfxControl *pParent);
public:
    CControlDocker();
    VOID Execute(CfxControl *pParent);
};

CControlDocker::CControlDocker()
{
}

BOOL CControlDocker::InsertBefore(CfxControl *pControl1, CfxControl *pControl2, ControlDock Dock)
{
    switch (Dock)
    {
    case dkTop:    return pControl1->GetTop() < pControl2->GetTop();
    case dkBottom: return pControl1->GetTop() >= pControl2->GetTop();
    case dkLeft:   return pControl1->GetLeft() < pControl2->GetLeft();
    case dkRight:  return pControl1->GetLeft() >= pControl2->GetLeft(); 
    }
    return FALSE;
}

VOID CControlDocker::DoPosition(CfxControl *pControl, ControlDock Dock)
{
    INT newLeft = _rect.Left;
    INT newTop  = _rect.Top;
    
    INT newWidth = _rect.Right - _rect.Left + 1;
    if ((newWidth < 0) || ((Dock == dkLeft) || (Dock == dkRight) || (Dock == dkNone)))
    {
        newWidth = pControl->GetWidth();
    }

    INT newHeight = _rect.Bottom - _rect.Top + 1;
    if ((newHeight < 0) || ((Dock == dkTop) || (Dock == dkBottom) || (Dock == dkNone)))
    {
        newHeight = pControl->GetHeight();
    }

    switch (Dock)
    {
    case dkTop:
        _rect.Top += newHeight;
        break;
    case dkBottom:
        _rect.Bottom -= newHeight;
        newTop = _rect.Bottom + 1;
        break;
    case dkLeft:
        _rect.Left += newWidth;
        break;
    case dkRight:
        _rect.Right -= newWidth;
        newLeft = _rect.Right + 1;
        break;
    case dkNone:
        newLeft = pControl->GetLeft();
        newTop = pControl->GetTop();
        break;
    }

    pControl->SetBounds(newLeft, newTop, newWidth, newHeight);

    if ((pControl->GetWidth() != (UINT)newWidth) || 
        (pControl->GetHeight() != (UINT)newHeight))
    {
        switch (Dock)
        {
        case dkTop:
            _rect.Top -= newHeight - pControl->GetHeight();
            break;
        case dkBottom:
            _rect.Bottom += newHeight - pControl->GetHeight();
            break;
        case dkLeft:
            _rect.Left -= newWidth - pControl->GetWidth();
            break;
        case dkRight:
            _rect.Right += newWidth - pControl->GetWidth();
            _rect.Bottom += newHeight - pControl->GetHeight();
            break;
        }
    }
}

VOID CControlDocker::DoAlign(ControlDock Dock)
{
    _dockList.Clear();
    
    UINT i;

    for (i=0; i<_parent->GetControlCount(); i++)
    {
        CfxControl *control = _parent->GetControl(i);
        if (control->GetVisible() && (control->GetDock() == Dock))
        {
            UINT j = 0;
            while ((j < _dockList.GetCount()) && (!InsertBefore(control, _dockList.Get(j), Dock)))
            {
                j++;
            }
            _dockList.Insert(j, control);
        }
    }

    for (i=0; i<_dockList.GetCount(); i++)
    {
        DoPosition(_dockList.Get(i), Dock);
    }
}

BOOL CControlDocker::AlignWork()
{
    BOOL result = FALSE;
    UINT left = 0, top = _parent->GetTop() + _parent->GetHeight();

    for (UINT i=0; i<_parent->GetControlCount(); i++)
    {
        CfxControl *control = _parent->GetControl(i);
        if (control->GetVisible())
        {
            ControlDock dock = control->GetDock();

            if (dock == dkAction)
            {
                control->SetBounds(left, top, 0, 0);
                left += control->GetWidth() + 4;
            }
            else 
            {
                result = result || (dock != dkNone);
            }
        }
    }

    return result;
}

VOID CControlDocker::AlignSubControls(CfxControl *pParent)
{
    _parent = pParent;
    _parent->GetClientBounds(&_rect);

    if (AlignWork())
    {
        OffsetRect(&_rect, -_parent->GetLeft(), -_parent->GetTop());

        DoAlign(dkTop);
        DoAlign(dkBottom);
        DoAlign(dkLeft);
        DoAlign(dkRight);
        DoAlign(dkFill);
        DoAlign(dkNone);
    }
}

VOID CControlDocker::InternalResetScale(CfxControl *pControl)
{
    pControl->SetCanScale(TRUE);

    for (UINT i=0; i<pControl->GetControlCount(); i++)
    {
        InternalResetScale(pControl->GetControl(i));
    }
}

VOID CControlDocker::InternalScale(CfxControl *pControl)
{
    if (!pControl->GetVisible())
    {
        return;
    }

    if (pControl->GetCanScale())
    {
        pControl->Scale(_scaleX, _scaleY, _scaleBorder, _scaleHitSize);
        pControl->SetCanScale(FALSE);
    }

    for (UINT i=0; i<pControl->GetControlCount(); i++)
    {
        InternalScale(pControl->GetControl(i));
    }
}

VOID CControlDocker::InternalExecute(CfxControl *pControl)
{
    if (!pControl->GetVisible())
    {
        return;
    }

    AlignSubControls(pControl);

    for (UINT i=0; i<pControl->GetControlCount(); i++)
    {
        InternalExecute(pControl->GetControl(i));
    }
}

VOID CControlDocker::Execute(CfxControl *pControl)
{
    CfxHost *host = GetHost(pControl);
    FXPROFILE *profile = host->GetProfile();
    BOOL live = pControl->IsLive();
    _scaleX = live ? profile->ScaleX : 100;
    _scaleY = live ? profile->ScaleY : 100;
    _scaleBorder = live ? profile->ScaleBorder : 100;
    _scaleHitSize = live ? (profile->ScaleHitSize * profile->DPI) / 100 : 0;

    InternalScale(pControl);
    InternalExecute(pControl);
}

VOID CfxEngine::AlignControls(CfxControl *pParent)
{
    CControlDocker aligner;
    
    do
    {
        _realignRequested = FALSE;
        aligner.Execute(pParent);
    } while (_realignRequested);
}

//*************************************************************************************************
// Painting

VOID CfxEngine::PaintControl(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, INT OffsetX, INT OffsetY)
{
    if (!pControl->GetVisible()) return;

    // absoluteRect = Absolute bounds of control
    FXRECT absoluteRect;
    pControl->GetBounds(&absoluteRect);
    OffsetRect(&absoluteRect, OffsetX, OffsetY);

    //
    // Pre-decorate based on absolute control bounds
    //   decorateRect = absoluteRect in the visible area
    //
    FXRECT decorateRect;
    IntersectRect(&decorateRect, pParentAbsoluteBounds, &absoluteRect);
    _paintCanvas->SetClipRect(&decorateRect);

    // We are clipped correctly, so hand off the absoluteRect
    PreDecorate(pControl, &absoluteRect);

    // paintRect = absoluteRect inflated by borderwidth
    FXRECT clipRect;
    if (IntersectRect(&clipRect, pParentAbsoluteBounds, &absoluteRect))
    {
        _paintCanvas->State = _paintCanvasState;
        _paintCanvas->SetClipRect(&clipRect);
        FXRECT innerRect = absoluteRect;
        InflateRect(&innerRect, -(INT)pControl->GetBorderWidth(), -(INT)pControl->GetBorderWidth());

        if (!IsRectEmpty(&innerRect))
        {
            pControl->OnPaint(_paintCanvas, &innerRect);
        }
    }

    // Paint back to front
    INT c = pControl->GetControlCount();
    for (INT i=(INT)pControl->GetControlCount()-1; i>=0; i--)
    {
        PaintControl(pControl->GetControl(i), &clipRect, absoluteRect.Left, absoluteRect.Top);
    }

    // Post-decorate based on absolute control bounds again, just like PreDecorate
    _paintCanvas->SetClipRect(&decorateRect);
    PostDecorate(pControl, &absoluteRect);
}

VOID CfxEngine::Paint(CfxCanvas *pCanvas)
{
    if (_terminating || _inPaint) return;
    
    _paintTimerElapse = 0;
    _inPaint = TRUE;
    KillTimer(this);

    FXRECT rect;
    CfxApplication *application = GetApplication(this);
    application->GetBounds(&rect);

    _paintCanvas = pCanvas;
    _paintCanvasState = _paintCanvas->State;
    _paintCanvas->BeginPaint();
    PaintControl(application->GetScreen(), &rect, 0, 0);
    _paintCanvas->EndPaint();
    _paintCanvas->State = _paintCanvasState;
    _paintCanvas = NULL;

    if (_paintTimerElapse != 0)
    {
        SetTimer(this, _paintTimerElapse);
    }
    
    _inPaint = FALSE;
}

VOID CfxEngine::PaintControlNoClip(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, INT OffsetX, INT OffsetY)
{
    if (!pControl->GetVisible() && pControl->GetComposite()) return;

    // absoluteRect = Absolute bounds of control
    FXRECT absoluteRect;
    pControl->GetBounds(&absoluteRect);
    OffsetRect(&absoluteRect, OffsetX, OffsetY);

    //
    // Pre-decorate based on absolute control bounds
    //   decorateRect = absoluteRect in the visible area
    //
    FXRECT decorateRect = absoluteRect;
    _paintCanvas->SetClipRect(&decorateRect);
    PreDecorate(pControl, &absoluteRect);

    // paintRect = absoluteRect inflated by borderwidth
    FXRECT paintRect = absoluteRect;
    _paintCanvas->State = _paintCanvasState;
    _paintCanvas->SetClipRect(&paintRect);
    InflateRect(&paintRect, -(INT)pControl->GetBorderWidth(), -(INT)pControl->GetBorderWidth());
    pControl->OnPaint(_paintCanvas, &paintRect);

    // Paint back to front
    for (INT i=(INT)pControl->GetControlCount()-1; i>=0; i--)
    {
        PaintControlNoClip(pControl->GetControl(i), &absoluteRect, absoluteRect.Left, absoluteRect.Top);
    }

    //
    // Post-decorate based on absolute control bounds again, just like
    // PreDecorate
    //
    _paintCanvas->SetClipRect(&decorateRect);
    PostDecorate(pControl, &absoluteRect);
}

VOID CfxEngine::PaintNoClip(CfxCanvas *pCanvas)
{
    FXRECT rect;
    CfxApplication *application = GetApplication(this);
    application->GetBounds(&rect);

    _paintCanvas = pCanvas;
    _paintCanvasState = _paintCanvas->State;
    PaintControlNoClip(application->GetScreen(), &rect, 0, 0);
    _paintCanvas->State = _paintCanvasState;
    _paintCanvas = NULL;
}

VOID CfxEngine::BlockPaint()
{ 
    _blockPaintCount++;
}

VOID CfxEngine::UnblockPaint()
{ 
    _blockPaintCount--;
    if (_terminating) return;

    if (_blockPaintCount == 0)
    {
        if (_paintRequested)
        {
            _paintRequested = FALSE;
            GetHost(this)->Paint();
        }
    }
}

BOOL CfxEngine::RequestPaint()
{ 
    _paintRequested = TRUE; 
    return _blockPaintCount == 0;
}

//*************************************************************************************************
// Hit Testing and PenDown / PenMove / PenUp

CfxControl *CfxEngine::HitControl(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, FXPOINT *pPoint)
{
    FXRECT rect1, rect2;

    pControl->GetBounds(&rect1);
    OffsetRect(&rect1, pParentAbsoluteBounds->Left, pParentAbsoluteBounds->Top);
    IntersectRect(&rect2, pParentAbsoluteBounds, &rect1);

    if (!pControl->GetIsVisible() || !PtInRect(&rect2, pPoint)) 
    {
        return NULL;
    }

    for (UINT i=0; i<pControl->GetControlCount(); i++)
    {
        CfxControl *control = HitControl(pControl->GetControl(i), &rect1, pPoint);
        if (control)
        {
            return control;
        }
    }

    return pControl;
}

CfxControl *CfxEngine::HitTest(CfxControl *pControl, INT X, INT Y)
{
    FXRECT rect;
    CfxApplication *application = GetApplication(this);
    application->GetBounds(&rect);
    FXPOINT point = {X, Y};

    return HitControl(pControl, &rect, &point);
}

CfxControl *CfxEngine::HitControlNoClip(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, FXPOINT *pPoint)
{
    FXRECT rect;

    pControl->GetBounds(&rect);
    OffsetRect(&rect, pParentAbsoluteBounds->Left, pParentAbsoluteBounds->Top);

    if (!pControl->GetIsVisible()) 
    {
        return NULL;
    }

    for (UINT i=0; i<pControl->GetControlCount(); i++)
    {
        CfxControl *control = HitControlNoClip(pControl->GetControl(i), &rect, pPoint);
        if (control)
        {
            return control;
        }
    }

    if (!PtInRect(&rect, pPoint) || pControl->GetComposite())
    {
        return NULL;
    }
    else
    {
        return pControl;
    }
}

CfxControl *CfxEngine::HitTestNoClip(CfxControl *pControl, INT X, INT Y)
{
    FXRECT rect;
    CfxApplication *application = GetApplication(this);
    application->GetBounds(&rect);
    FXPOINT point = {X, Y};

    return HitControlNoClip(pControl, &rect, &point);
}

VOID CfxEngine::PenDown(INT X, INT Y)
{
    _lastUserEventTime = FxGetTickCount();

    CfxControl *control = HitTest(GetScreen(this), X, Y);

    if (control && control->GetEnabled())
    {
        FXRECT rect;
        _captureControl = control;
        _captureControl->GetAbsoluteBounds(&rect);
        _captureControl->OnPenDown(X - rect.Left, Y - rect.Top);
    }
}

VOID CfxEngine::PenMove(INT X, INT Y)
{
    CfxControl *control = _captureControl ? _captureControl : HitTest(GetApplication(this)->GetScreen(), X, Y);
    
    if (control && control->GetEnabled())
    {
        FXRECT rect;
        control->GetAbsoluteBounds(&rect);
        control->OnPenMove(X - rect.Left, Y - rect.Top);
    }
}

VOID CfxEngine::PenUp(INT X, INT Y)
{
    if (!_captureControl) return;

    FXRECT rect;
    _captureControl->GetAbsoluteBounds(&rect);
    _captureControl->OnPenUp(X - rect.Left, Y - rect.Top);

    FXPOINT point = {X, Y};
    if (PtInRect(&rect, &point))
    {
        _captureControl->OnPenClick(X - rect.Left, Y - rect.Top);
    }
    _captureControl = NULL;
}

enum KeyEvent {keDown, keUp};

BOOL RunKeyEvent(CfxControl *pControl, BYTE KeyCode, KeyEvent Event)
{
    BOOL handled = FALSE;
    
    switch (Event)
    {
    case keDown:  pControl->OnKeyDown(KeyCode, &handled); break;
    case keUp:    pControl->OnKeyUp(KeyCode, &handled); break;
    }

    if (!handled)
    {
        for (UINT i=0; i<pControl->GetControlCount(); i++)
        {
            handled = RunKeyEvent(pControl->GetControl(i), KeyCode, Event);
            if (handled) break;
        }
    }

    return handled;
}

BOOL CfxEngine::KeyDown(BYTE KeyCode)
{
    return RunKeyEvent(GetScreen(this), KeyCode, keDown);
}

BOOL CfxEngine::KeyUp(BYTE KeyCode)
{
    return RunKeyEvent(GetScreen(this), KeyCode, keUp);
}

VOID CfxEngine::KeyPress(BYTE KeyCode)
{
    KeyDown(KeyCode);
    KeyUp(KeyCode);
}

BOOL RunCloseDialog(CfxControl *pControl)
{
    BOOL handled = FALSE;

    pControl->OnCloseDialog(&handled);
    
    if (!handled)
    {
        for (UINT i=0; i<pControl->GetControlCount(); i++)
        {
            handled = handled || RunCloseDialog(pControl->GetControl(i));
        }
    }

    return handled;
}

BOOL CfxEngine::CloseDialog()
{
    return RunCloseDialog(GetScreen(this));
}

VOID CfxEngine::Startup()
{
    CfxHost *host = GetHost(this);

    _terminating = FALSE;

    _lastUserEventTime = 0;
    host->Startup();
    AlignControls(GetScreen(this));

    FXPROFILE *profile = host->GetProfile();
    _cornerWidth1 = (4 * profile->ScaleX) / 100;
    _cornerWidth2 = (8 * profile->ScaleX) / 100;
}

VOID CfxEngine::Shutdown()
{
    #ifdef CLIENT_DLL
        return;
    #endif

    KillTimer(this);
    
    if (_terminating) return;
   
    GetHost(this)->Shutdown();
    _terminating = TRUE;
}

VOID CfxEngine::Repaint()
{
    GetHost(this)->Paint();
}

VOID CfxEngine::OnTimer()
{
    if (_blockPaintCount == 0)
    {
        Repaint();
    }
}

VOID CfxEngine::SetPaintTimer(UINT Elapse)
{
    if (Elapse == 0) return;

    if (_paintTimerElapse == 0)
    {
        _paintTimerElapse = Elapse;
    }
    else
    {
        _paintTimerElapse = min(_paintTimerElapse, Elapse);
    }
}

VOID CfxEngine::DoTimer(UINT TimerId)
{
    // Normal timer
    FX_ASSERT((INT)TimerId >= 0);
    
    if (TimerId >= _timers.GetCount()) 
    {
        Log("Bad timer id: %d %d", TimerId, _timers.GetCount());
        return;
    }

    FX_ASSERT(TimerId < _timers.GetCount());

    CfxPersistent *timerObject = _timers.Get(TimerId);
    
    if (timerObject)
    {
        timerObject->OnTimer();
    }
}

VOID CfxEngine::SetTimer(CfxPersistent *pObject, UINT Elapse)
{
    // Check if we already have a timer on this control
    INT index = _timers.IndexOf(pObject);
    if (index == -1)
    {
        // Find an empty index
        index = _timers.IndexOf(NULL);
    }

    if (index != -1)
    {
        // Re-use existing entry
        _timers.Put(index, pObject);
    }
    else
    {
        // Add a new timer
        index = _timers.Add(pObject);
    }
    
    // Note: SetTimer overwrites existing timers
    GetHost(this)->SetTimer(Elapse, index);
}

VOID CfxEngine::KillTimer(CfxPersistent *pObject)
{
    INT index = _timers.IndexOf(pObject);
    if (index != -1)
    {
        GetHost(this)->KillTimer(index);
        _timers.Put(index, NULL);
    }
}

BOOL CfxEngine::HasTimer(CfxPersistent *pObject)
{
    return _timers.IndexOf(pObject) != -1;
}

VOID CfxEngine::SetNextAlarm()
{
    ALARM_ITEM *alarmItem, *nextAlarmItem;
    FXDATETIME time;
    DOUBLE currentTime;
    CfxHost *host;
    DOUBLE elapseInSeconds;
    UINT i;
    CfxPersistent *alarmObject;

    _alarmSet = FALSE;

    host = GetHost(this);
    host->KillAlarm();
    host->GetDateTime(&time);
    currentTime = EncodeDateTime(&time);

    // Find the next alarm.
    nextAlarmItem = NULL;
    for (i = 0; i < _alarms.GetCount(); i++)
    {
        alarmItem = _alarms.GetPtr(i);
        alarmObject = alarmItem->Object;

        if ((alarmObject == NULL) || !alarmItem->Active) continue;

        if ((nextAlarmItem == NULL) || (alarmItem->ExpectedTime < nextAlarmItem->ExpectedTime))
        {
            nextAlarmItem = alarmItem;
        } 
    }

    // Set the next alarm if found.
    if (nextAlarmItem != NULL)
    {
        elapseInSeconds = (nextAlarmItem->ExpectedTime - currentTime) * 24 * 60 * 60;
        _alarmSet = TRUE;
        host->SetAlarm((UINT)max(10, (INT)elapseInSeconds));
    }
}

VOID CfxEngine::DoAlarm()
{
    FXDATETIME time;
    DOUBLE currentTime, deltaTime;
    ALARM_ITEM *alarmItem;
    UINT i;
    CfxPersistent *alarmObject;

    _alarmSet = FALSE;

    GetHost(this)->GetDateTime(&time);
    currentTime = EncodeDateTime(&time);

    for (i = 0; i < _alarms.GetCount(); i++)
    {
        alarmItem = _alarms.GetPtr(i);
        alarmObject = alarmItem->Object;
        if ((alarmObject == NULL) || !alarmItem->Active) continue;

        deltaTime = (INT)((alarmItem->ExpectedTime - currentTime) * 24 * 60 * 60);

        if (deltaTime <= 3)
        {
            alarmItem->Active = FALSE;
            alarmObject->OnAlarm();
        }
    }

    if (!_alarmSet)
    {
        SetNextAlarm();
    }
}

VOID CfxEngine::SetAlarm(CfxPersistent *pObject, UINT ElapseInSeconds, const CHAR *pName)
{
    ALARM_ITEM *alarmItem, *existingAlarmItem;
    ALARM_ITEM newAlarmItem = {0};
    UINT i;
    FXDATETIME time;
    DOUBLE currentTime;

    GetHost(this)->GetDateTime(&time);
    currentTime = EncodeDateTime(&time);

    newAlarmItem.ExpectedTime = currentTime + (DOUBLE)ElapseInSeconds / (24 * 60 * 60);
    newAlarmItem.Object = pObject;
    newAlarmItem.Active = TRUE;
    newAlarmItem.Name = pName;

    existingAlarmItem = NULL;
    for (i = 0; i < _alarms.GetCount(); i++)
    {
        alarmItem = _alarms.GetPtr(i);
        
        if (alarmItem->Object == pObject)
        {
            existingAlarmItem = alarmItem;
            break;
        }

        if (alarmItem->Object == NULL)
        {
            existingAlarmItem = alarmItem;
            break;
        }
    }

    if (existingAlarmItem == NULL)
    {
        _alarms.Add(newAlarmItem);
    }
    else
    {
        memcpy(existingAlarmItem, &newAlarmItem, sizeof(ALARM_ITEM));
    }

    // Fire any expired alarms and set the next alarm.
    DoAlarm();
}

VOID CfxEngine::KillAlarm(CfxPersistent *pObject)
{
    ALARM_ITEM *alarmItem;
    UINT i;

    for (i = 0; i < _alarms.GetCount(); i++)
    {
        alarmItem = _alarms.GetPtr(i);
        if (alarmItem->Object == pObject)
        {
            alarmItem->Object = NULL;
            alarmItem->Active = FALSE;
            break;
        }
    }

    SetNextAlarm();
}

BOOL CfxEngine::HasAlarm(CfxPersistent *pObject)
{
    ALARM_ITEM *alarmItem;
    UINT i;
    BOOL result = FALSE;

    for (i = 0; i < _alarms.GetCount(); i++)
    {
        alarmItem = _alarms.GetPtr(i);
        if ((alarmItem->Object == pObject) && alarmItem->Active)
        {
            result = TRUE;
            break;
        }
    }

    return result;
}

VOID CfxEngine::DoTrackTimer()
{
    if (_trackTimer)
    {
        _trackTimer->OnTrackTimer();
    }
}

VOID CfxEngine::SetTrackTimer(CfxPersistent *pObject, UINT ElapseInSeconds)
{
    _trackTimer = pObject;
    GetHost(this)->SetTrackTimer(ElapseInSeconds);
}

VOID CfxEngine::KillTrackTimer(CfxPersistent *pObject)
{
    if (_trackTimer && (pObject != _trackTimer))
    {
        Log("TrackTimer: ERROR - wrong track timer object");
    }

    if (_trackTimer)
    {
        GetHost(this)->SetTrackTimer(0);
        _trackTimer = NULL;
    }
}

VOID CfxEngine::DoUrlRequestCompleted(UINT Id, const CHAR *pFileName, UINT ErrorCode)
{
    UINT i;
    for (i = 0; i < _urlRequests.GetCount(); i++)
    {
        if (_urlRequests.GetPtr(i)->Id == Id)
        {
            URL_REQUEST_ITEM *item = _urlRequests.GetPtr(i);
            item->Object->OnUrlReceived(pFileName, ErrorCode);
            MEM_FREE(item->Data);
            FxDeleteFile(pFileName);
            _urlRequests.Delete(i);
            break;
        }
    }
}

VOID CfxEngine::SetUrlRequest(CfxPersistent *pObject, CHAR *pUrl, CHAR *pUserName, CHAR *pPassword, CfxStream &Data)
{
    CancelUrlRequest(pObject);
    
    URL_REQUEST_ITEM newItem;
    memset(&newItem, 0, sizeof(newItem));
    newItem.Object = pObject;
    newItem.Data = (BYTE *)Data.CloneMemory();
    newItem.DataSize = Data.GetSize();
    CHAR *fileName = GetHost(this)->AllocPathApplication("UrlRequest");

    GetHost(this)->RequestUrl(pUrl, pUserName, pPassword, Data, fileName, &newItem.Id);
    if (newItem.Id > 0)
    {
        _urlRequests.Add(newItem);
    }

    MEM_FREE(fileName);
}

VOID CfxEngine::CancelUrlRequest(CfxPersistent *pObject)
{
    UINT i;

    for (i = 0; i < _urlRequests.GetCount(); i++)
    {
        URL_REQUEST_ITEM *item = _urlRequests.GetPtr(i);
        if (item->Object == pObject)
        {
            MEM_FREE(item->Data);
            item->Data = NULL;
            _urlRequests.Delete(i);
            break;
        }
    }
}

VOID CfxEngine::DoPortData(FX_COMPORT_DATA *pComPortData)
{
    PORT_ITEM *portItem;
    UINT i;
    UINT portsUniqueness = _portsUniqueness;

    for (i = 0; i < _ports.GetCount(); i++)
    {
        portItem = _ports.GetPtr(i);
        if ((portItem->PortId == pComPortData->PortId) &&
            (portItem->Object != NULL))
        {
            portItem->Object->OnPortData(pComPortData->PortId, pComPortData->Data, pComPortData->Length);
        }

        if (_portsUniqueness != portsUniqueness) break;
    }
}

BOOL CfxEngine::StartRecording(CfxPersistent *pObject, CHAR *pMediaPath, UINT Duration, UINT Frequency)
{
    StopRecording();

    _recorder = pObject;

    return GetHost(this)->StartRecording(pMediaPath, Duration, Frequency);
}

BOOL CfxEngine::StopRecording()
{
    BOOL result = FALSE;

    if (_recorder != NULL)
    {
        result = GetHost(this)->StopRecording();
    }

    return result;
}

VOID CfxEngine::CancelRecording()
{
    if (_recorder != NULL)
    {
        _recorder = NULL;
        GetHost(this)->CancelRecording();
    }
}

BOOL CfxEngine::IsRecording()
{
    return (_recorder != NULL) && GetHost(this)->IsRecording();
}

VOID CfxEngine::DoRecordingComplete(CHAR *pFileName, UINT Duration)
{
    if ((_recorder != NULL) && (pFileName != NULL))
    {
        _recorder->OnRecordingComplete(pFileName, Duration);
    }

    _recorder = NULL;
}

VOID CfxEngine::DoPictureTaken(CfxControl *pControl, CHAR *pFileName, BOOL Success)
{
    pControl->OnPictureTaken(pFileName, Success);

    for (UINT i=0; i < pControl->GetControlCount(); i++)
    {
        DoPictureTaken(pControl->GetControl(i), pFileName, Success);
    }
}

VOID CfxEngine::DoBarcodeScan(CfxControl *pControl, CHAR *pBarcode, BOOL Success)
{
    pControl->OnBarcodeScan(pBarcode, Success);

    for (UINT i=0; i < pControl->GetControlCount(); i++)
    {
        DoBarcodeScan(pControl->GetControl(i), pBarcode, Success);
    }
}

VOID CfxEngine::DoPhoneNumberTaken(CfxControl *pControl, CHAR *pPhoneNumber, BOOL Success)
{
    pControl->OnPhoneNumberTaken(pPhoneNumber, Success);

    for (UINT i=0; i < pControl->GetControlCount(); i++)
    {
        DoPhoneNumberTaken(pControl->GetControl(i), pPhoneNumber, Success);
    }
}

BOOL CfxEngine::IsPortConnected(BYTE PortId)
{
    return GetHost(this)->IsPortConnected(PortId);
}

VOID CfxEngine::CyclePort(BYTE PortId, FX_COMPORT *pComPort)
{
    CfxHost *host = GetHost(this);

    if (!host->IsPortConnected(PortId))
    {
        host->DisconnectPort(PortId);
        host->ConnectPort(PortId, pComPort);
    }
}

BOOL CfxEngine::ConnectPort(CfxPersistent *pObject, BYTE PortId, FX_COMPORT *pComPort)
{
    PORT_ITEM *portItem;
    PORT_ITEM newPortItem = {0};
    UINT i;
    BOOL alreadyConnected = FALSE;

    for (i = 0; i < _ports.GetCount(); i++)
    {
        portItem = _ports.GetPtr(i);
        
        if (portItem->PortId != PortId)
        {
            // Different port
            continue;
        }

        if (portItem->Object == pObject)
        {
            // Already hooked
            CyclePort(PortId, pComPort);
            return TRUE;
        }

        if (memcmp(pComPort, &portItem->ComPort, sizeof(FX_COMPORT)) != 0)
        {
            // Another connection with different parameters!
            return FALSE;
        }

        alreadyConnected = TRUE;
        CyclePort(PortId, pComPort);
    }

    // No other readers 
    newPortItem.PortId = PortId;
    newPortItem.Object = pObject;
    newPortItem.ComPort = *pComPort;

    if (alreadyConnected || (GetHost(this)->ConnectPort(PortId, pComPort)))
    {
        _portsUniqueness++;
        _ports.Add(newPortItem);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID CfxEngine::DisconnectPort(CfxPersistent *pObject, BYTE PortId, BOOL KeepConnected)
{
    UINT i = 0;
    while (i < _ports.GetCount())
    {
        PORT_ITEM *portItem = _ports.GetPtr(i);
        if ((portItem->Object == pObject) && (portItem->PortId == PortId))
        {
            _portsUniqueness++;
            _ports.Delete(i);
            i = 0;
        }
        else
        {
            i++;
        }
    }

    for (i = 0; i < _ports.GetCount(); i++)
    {
        if (_ports.GetPtr(i)->PortId == PortId)
        {
            // Still some connections to this port.
            return;
        }
    }

    if (KeepConnected == FALSE)
    {
        GetHost(this)->DisconnectPort(PortId);
    }
}

VOID CfxEngine::WritePortData(BYTE PortId, BYTE *pData, UINT Length)
{
    GetHost(this)->WritePortData(PortId, pData, Length);
}

VOID CfxEngine::DoTransferEvent(FXSENDSTATEMODE Mode)
{
    UINT i;
    for (i = 0; i < _transfers.GetCount(); i++)
    {
        _transfers.Get(i)->OnTransfer(Mode);
    }
}

VOID CfxEngine::GetTransferState(FXSENDSTATE *pState)
{
    GetHost(this)->GetTransferState(pState);
}

VOID CfxEngine::StartTransfer(CfxPersistent *pObject, CHAR *pOutgoingPath)
{
    if (_transfers.IndexOf(pObject) == -1)
    {
        _transfers.Add(pObject);
    }

    GetHost(this)->StartTransfer(pOutgoingPath);
}

VOID CfxEngine::AddTransferListener(CfxPersistent *pObject)
{
    if (_transfers.IndexOf(pObject) == -1)
    {
        _transfers.Add(pObject);
    }
}

VOID CfxEngine::RemoveTransferListener(CfxPersistent *pObject)
{
    _transfers.Remove(pObject);
}

VOID CfxEngine::CancelTransfers()
{
    GetHost(this)->CancelTransfer();
}

VOID CfxEngine::SetGPSTimeSync(BOOL Enabled, INT Zone)
{
    _gpsTimeSync = Enabled;
    _gpsTimeZone = Zone;
}

FXGPS *CfxEngine::GetGPS()
{
    CfxHost *host = GetHost(this);

    host->GetGPS(&_gps);

    if (_useRangeFinderForAltitude)
    {
        FXRANGE *range = &_range;

        if (g_rangeFinderLockCount > 0)
        {
            range = GetRange();
        }

        _gps.Position.Altitude = range->HasRange() ? range->Range.Range : 0;
    }

    /*
    {
        SYSTEM_POWER_STATUS_EX2 sps;
        GetSystemPowerStatusEx2(&sps, sizeof(sps), TRUE);
        _gps.Position.Altitude = sps.BatteryLifePercent;
    }
    */

    if (TestGPS(&_gps, GPS_DEFAULT_ACCURACY))
    {
        _gpsLastFix = _gps.Position;
        _gpsLastHeading = _gps.Heading;
        GetHost(this)->GetDateTime(&_gpsLastTimeStamp);
    }

    _gps.TimeInSync = FALSE;
    if (_gpsTimeSync && (_gps.DateTime.Year != 0))
    {
        DOUBLE now, gpsNow;
        FXDATETIME dateTime;
        INT timeDiff;
        
        host->GetDateTime(&dateTime);
        now = EncodeDateTime(&dateTime);
    
        gpsNow = EncodeDateTime(&_gps.DateTime);
        gpsNow += ((DOUBLE)_gpsTimeZone / 100) / 24;

        timeDiff = FxAbs((INT)((gpsNow - now) * 24 * 60));
        if ((timeDiff > 2) || _gpsTimeFirst)
        {
            // Ensure that the GPS reading is not old
            UINT tickDiff = (FxGetTickCount() - _gps.TimeStamp) / FxGetTicksPerSecond();
            if (tickDiff < 3)
            {
                _gpsTimeFirst = FALSE;
                DecodeDateTime(gpsNow, &dateTime);
                host->SetTimeZone((_gpsTimeZone / 100) * 60);
                host->SetDateTime(&dateTime);

                host->SetDateTimeFromGPS(&dateTime);

                _gps.TimeInSync = TRUE;
                _gpsTimeUsedOnce = TRUE;
            }
            else
            {
                // We need to set the time, but the data is old
                _gps.TimeInSync = FALSE;
            }
        }
        else 
        {
            _gps.TimeInSync = TRUE;
            _gpsTimeUsedOnce = TRUE;
        }
    }

    return &_gps;
}

VOID CfxEngine::LockGPS()
{
    g_gpsLockCount++;
    GetHost(this)->SetGPS(TRUE);
}

VOID CfxEngine::UnlockGPS()
{
    FX_ASSERT(g_gpsLockCount > 0);

    g_gpsLockCount--;

    if (g_gpsLockCount == 0)
    {
        GetHost(this)->SetGPS(FALSE);
    }
}

VOID CfxEngine::DoStartGPS()
{
    if (g_gpsLockCount > 0)
    {
        GetHost(this)->SetGPS(TRUE);
    }
}

FXRANGE *CfxEngine::GetRange()
{
    GetHost(this)->GetRange(&_range);
    return &_range;
}

VOID CfxEngine::LockRangeFinder()
{
    g_rangeFinderLockCount++;
    GetHost(this)->SetRangeFinder(TRUE);
}

VOID CfxEngine::UnlockRangeFinder()
{
    FX_ASSERT(g_rangeFinderLockCount > 0);

    g_rangeFinderLockCount--;

    if (g_rangeFinderLockCount == 0)
    {
        GetHost(this)->SetRangeFinder(FALSE);
    }
}

VOID CfxEngine::DoStartRangeFinder()
{
    if (g_rangeFinderLockCount > 0)
    {
        GetHost(this)->SetRangeFinder(TRUE);
    }
}

VOID CfxEngine::ClearLastRange()
{
    memset(&_range.Range, 0, sizeof(_range.Range));
}

VOID CfxEngine::DoWakeup()
{
    if (g_gpsLockCount > 0)
    {
        GetHost(this)->ResetGPSTimeouts();
    }

    if (g_rangeFinderLockCount > 0)
    {
        GetHost(this)->ResetRangeFinderTimeouts();
    }
}

VOID CfxEngine::DoSleep()
{
}
