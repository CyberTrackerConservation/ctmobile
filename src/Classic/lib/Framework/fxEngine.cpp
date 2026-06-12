#include "fxEngine.h"
#include "fxUtils.h"
#include "fxApplication.h"
#include "fxHost.h"
#include "fxMath.h"

// The GPS is a global resource
int g_gpsLockCount = 0;

CfxEngine::CfxEngine(CfxPersistent *pOwner): CfxPersistent(pOwner), _timers(), _alarms(), _ports(), _urlRequests()
{
    _captureControl = nullptr;
    _lastUserEventTime = 0;
    _portsUniqueness = 0;
    _terminating = false;
    _cornerWidth1 = 4;
    _cornerWidth2 = 8;

    _scaleScroller = _scaleTitle = _scaleTab = 100;

    _inPaint = false;
    _blockPaintCount = 0;
    _paintRequested = false;

    _paintTimerElapse = 0;

    memset(&_gps, 0, sizeof(_gps));
    memset(&_gpsLastFix, 0, sizeof(_gpsLastFix));
    _gpsLastHeading = 0;
    memset(&_gpsLastTimeStamp, 0, sizeof(_gpsLastTimeStamp));
    _gpsTimeFirst = true;
    _gpsTimeSync = false;
    _gpsTimeZone = 0;
    _gpsTimeUsedOnce = false;

    _recorder = nullptr;

    _alarmSet = false;
    _alarmMutex = false;

    _currentGoto = (FXGOTO *)MEM_ALLOC(sizeof(FXGOTO));
    memset(_currentGoto, 0, sizeof(FXGOTO));
    _trackTimer = nullptr;

    _keyPressTimer = new QTimer(nullptr);
    _keyPressTimer->setSingleShot(true);
    QObject::connect(_keyPressTimer, &QTimer::timeout, [this]()
    {
        BYTE keyCode = _pendingKeyCode;
        _pendingKeyCode = 0;
        _inputBlocked = false;
        KeyPress(keyCode);
    });
}

CfxEngine::~CfxEngine()
{
    _keyPressTimer->stop();
    delete _keyPressTimer;

    KillTimer(this);
    
    MEM_FREE(_currentGoto);
    _currentGoto = nullptr;

    // Cleanup URL data requests.
    for (int i = 0; i < _urlRequests.GetCount(); i++)
    {
        URL_REQUEST_ITEM *item = _urlRequests.GetPtr(i);
        MEM_FREE(item->Data);
    }
}

void CfxEngine::PreDecorate(CfxControl *pControl, FXRECT *pRect)
{
    ControlBorderStyle borderStyle = pControl->GetBorderStyle();

    if (borderStyle < ControlBorderStyle::Round1) return;

    FXRECT rect;
    pControl->GetAbsoluteBounds(&rect);

    switch (pControl->GetBorderStyle())
    {
    case ControlBorderStyle::Round1:
        _paintCanvas->PushCorners(&rect, _cornerWidth1);
        break;

    case ControlBorderStyle::Round2:
        _paintCanvas->PushCorners(&rect, _cornerWidth2);
        break;
    }
}

void CfxEngine::PostDecorate(CfxControl *pControl, FXRECT *pRect)
{
    if ((pControl->GetBorderWidth() == 0) && (pControl->GetBorderStyle() == ControlBorderStyle::None)) return;

    FXRECT rect;
    int borderWidth = (int)pControl->GetBorderWidth();
    int borderLineWidth = (int)pControl->GetBorderLineWidth();
    int lastLineWidth = _paintCanvas->State.LineWidth;
    _paintCanvas->State.LineWidth = 1;

    // Paint the inside border
    if ((borderWidth > 0) && !pControl->GetTransparent())
    {
        rect = *pRect;
        _paintCanvas->State.LineColor = pControl->GetColor();
        for (int i=0; i<min(borderWidth, 64); i++)
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
    case ControlBorderStyle::None:   
        break;

    case ControlBorderStyle::Single:
        _paintCanvas->FrameRect(&rect);
        break;

    case ControlBorderStyle::Round1:
        _paintCanvas->RoundRect(&rect, _cornerWidth1);
        pControl->GetAbsoluteBounds(&rect);
        _paintCanvas->PopCorners(&rect); 
        break;

    case ControlBorderStyle::Round2:
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
    int _scaleX, _scaleY, _scaleBorder, _scaleHitSize;
protected:
    bool InsertBefore(CfxControl *pControl1, CfxControl *pControl2, ControlDock Dock);
    void DoPosition(CfxControl *pControl, ControlDock Dock);
    void DoAlign(ControlDock Dock);
    bool AlignWork();
    void AlignSubControls(CfxControl *pParent);
    void InternalExecute(CfxControl *pParent);
    void InternalResetScale(CfxControl *pControl);
    void InternalScale(CfxControl *pParent);
public:
    CControlDocker();
    void Execute(CfxControl *pParent);
};

CControlDocker::CControlDocker()
{
}

bool CControlDocker::InsertBefore(CfxControl *pControl1, CfxControl *pControl2, ControlDock Dock)
{
    switch (Dock)
    {
    case ControlDock::Top:    return pControl1->GetTop() < pControl2->GetTop();
    case ControlDock::Bottom: return pControl1->GetTop() >= pControl2->GetTop();
    case ControlDock::Left:   return pControl1->GetLeft() < pControl2->GetLeft();
    case ControlDock::Right:  return pControl1->GetLeft() >= pControl2->GetLeft(); 
    }
    return false;
}

void CControlDocker::DoPosition(CfxControl *pControl, ControlDock Dock)
{
    int newLeft = _rect.Left;
    int newTop  = _rect.Top;
    
    int newWidth = _rect.Right - _rect.Left + 1;
    if ((newWidth < 0) || ((Dock == ControlDock::Left) || (Dock == ControlDock::Right) || (Dock == ControlDock::None)))
    {
        newWidth = pControl->GetWidth();
    }

    int newHeight = _rect.Bottom - _rect.Top + 1;
    if ((newHeight < 0) || ((Dock == ControlDock::Top) || (Dock == ControlDock::Bottom) || (Dock == ControlDock::None)))
    {
        newHeight = pControl->GetHeight();
    }

    switch (Dock)
    {
    case ControlDock::Top:
        _rect.Top += newHeight;
        break;
    case ControlDock::Bottom:
        _rect.Bottom -= newHeight;
        newTop = _rect.Bottom + 1;
        break;
    case ControlDock::Left:
        _rect.Left += newWidth;
        break;
    case ControlDock::Right:
        _rect.Right -= newWidth;
        newLeft = _rect.Right + 1;
        break;
    case ControlDock::None:
        newLeft = pControl->GetLeft();
        newTop = pControl->GetTop();
        break;
    }

    pControl->SetBounds(newLeft, newTop, newWidth, newHeight);

    if ((pControl->GetWidth() != (int)newWidth) || 
        (pControl->GetHeight() != (int)newHeight))
    {
        switch (Dock)
        {
        case ControlDock::Top:
            _rect.Top -= newHeight - pControl->GetHeight();
            break;
        case ControlDock::Bottom:
            _rect.Bottom += newHeight - pControl->GetHeight();
            break;
        case ControlDock::Left:
            _rect.Left -= newWidth - pControl->GetWidth();
            break;
        case ControlDock::Right:
            _rect.Right += newWidth - pControl->GetWidth();
            _rect.Bottom += newHeight - pControl->GetHeight();
            break;
        }
    }
}

void CControlDocker::DoAlign(ControlDock Dock)
{
    _dockList.Clear();
    
    int i;

    for (i=0; i<_parent->GetControlCount(); i++)
    {
        CfxControl *control = _parent->GetControl(i);
        if (control->GetVisible() && (control->GetDock() == Dock))
        {
            int j = 0;
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

bool CControlDocker::AlignWork()
{
    bool result = false;
    int left = 0, top = _parent->GetTop() + _parent->GetHeight();

    for (int i=0; i<_parent->GetControlCount(); i++)
    {
        CfxControl *control = _parent->GetControl(i);
        if (control->GetVisible())
        {
            ControlDock dock = control->GetDock();

            if (dock == ControlDock::Action)
            {
                control->SetBounds(left, top, 0, 0);
                left += control->GetWidth() + 4;
            }
            else 
            {
                result = result || (dock != ControlDock::None);
            }
        }
    }

    return result;
}

void CControlDocker::AlignSubControls(CfxControl *pParent)
{
    _parent = pParent;
    _parent->GetClientBounds(&_rect);

    if (AlignWork())
    {
        OffsetRect(&_rect, -_parent->GetLeft(), -_parent->GetTop());

        DoAlign(ControlDock::Top);
        DoAlign(ControlDock::Bottom);
        DoAlign(ControlDock::Left);
        DoAlign(ControlDock::Right);
        DoAlign(ControlDock::Fill);
        DoAlign(ControlDock::None);
    }
}

void CControlDocker::InternalResetScale(CfxControl *pControl)
{
    pControl->SetCanScale(true);

    for (int i=0; i<pControl->GetControlCount(); i++)
    {
        InternalResetScale(pControl->GetControl(i));
    }
}

void CControlDocker::InternalScale(CfxControl *pControl)
{
    if (!pControl->GetVisible())
    {
        return;
    }

    if (pControl->GetCanScale())
    {
        pControl->Scale(_scaleX, _scaleY, _scaleBorder, _scaleHitSize);
        pControl->SetCanScale(false);
    }

    for (int i=0; i<pControl->GetControlCount(); i++)
    {
        InternalScale(pControl->GetControl(i));
    }
}

void CControlDocker::InternalExecute(CfxControl *pControl)
{
    if (!pControl->GetVisible())
    {
        return;
    }

    AlignSubControls(pControl);

    for (int i=0; i<pControl->GetControlCount(); i++)
    {
        InternalExecute(pControl->GetControl(i));
    }
}

void CControlDocker::Execute(CfxControl *pControl)
{
    CfxHost *host = GetHost(pControl);
    FXPROFILE *profile = host->GetProfile();
    bool live = pControl->IsLive();
    _scaleX = live ? profile->ScaleX : 100;
    _scaleY = live ? profile->ScaleY : 100;
    _scaleBorder = live ? profile->ScaleBorder : 100;
    _scaleHitSize = live ? (profile->ScaleHitSize * profile->DPI) / 100 : 0;

    InternalScale(pControl);
    InternalExecute(pControl);
}

void CfxEngine::AlignControls(CfxControl *pParent)
{
    CControlDocker aligner;
    
    do
    {
        _realignRequested = false;
        aligner.Execute(pParent);
    } while (_realignRequested);
}

//*************************************************************************************************
// Painting

void CfxEngine::PaintControl(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, int OffsetX, int OffsetY)
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
        InflateRect(&innerRect, -(int)pControl->GetBorderWidth(), -(int)pControl->GetBorderWidth());

        if (!IsRectEmpty(&innerRect))
        {
            pControl->OnPaint(_paintCanvas, &innerRect);
        }
    }

    // Paint back to front
    int c = pControl->GetControlCount();
    for (int i=(int)pControl->GetControlCount()-1; i>=0; i--)
    {
        PaintControl(pControl->GetControl(i), &clipRect, absoluteRect.Left, absoluteRect.Top);
    }

    // Post-decorate based on absolute control bounds again, just like PreDecorate
    _paintCanvas->SetClipRect(&decorateRect);
    PostDecorate(pControl, &absoluteRect);
}

void CfxEngine::Paint(CfxCanvas *pCanvas)
{
    if (_terminating || _inPaint) return;
    
    _paintTimerElapse = 0;
    _inPaint = true;
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
    _paintCanvas = nullptr;

    if (_paintTimerElapse != 0)
    {
        SetTimer(this, _paintTimerElapse);
    }
    
    _inPaint = false;
}

void CfxEngine::PaintControlNoClip(CfxControl *pControl, FXRECT *pParentAbsoluteBounds, int OffsetX, int OffsetY)
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
    InflateRect(&paintRect, -(int)pControl->GetBorderWidth(), -(int)pControl->GetBorderWidth());
    pControl->OnPaint(_paintCanvas, &paintRect);

    // Paint back to front
    for (int i=(int)pControl->GetControlCount()-1; i>=0; i--)
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

void CfxEngine::PaintNoClip(CfxCanvas *pCanvas)
{
    FXRECT rect;
    CfxApplication *application = GetApplication(this);
    application->GetBounds(&rect);

    _paintCanvas = pCanvas;
    _paintCanvasState = _paintCanvas->State;
    PaintControlNoClip(application->GetScreen(), &rect, 0, 0);
    _paintCanvas->State = _paintCanvasState;
    _paintCanvas = nullptr;
}

void CfxEngine::BlockPaint()
{ 
    _blockPaintCount++;
}

void CfxEngine::UnblockPaint()
{ 
    _blockPaintCount--;
    if (_terminating) return;

    if (_blockPaintCount == 0)
    {
        if (_paintRequested)
        {
            _paintRequested = false;
            GetHost(this)->Paint();
        }
    }
}

bool CfxEngine::RequestPaint()
{ 
    _paintRequested = true; 
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
        return nullptr;
    }

    for (int i=0; i<pControl->GetControlCount(); i++)
    {
        CfxControl *control = HitControl(pControl->GetControl(i), &rect1, pPoint);
        if (control)
        {
            return control;
        }
    }

    return pControl;
}

CfxControl *CfxEngine::HitTest(CfxControl *pControl, int X, int Y)
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
        return nullptr;
    }

    for (int i=0; i<pControl->GetControlCount(); i++)
    {
        CfxControl *control = HitControlNoClip(pControl->GetControl(i), &rect, pPoint);
        if (control)
        {
            return control;
        }
    }

    if (!PtInRect(&rect, pPoint) || pControl->GetComposite())
    {
        return nullptr;
    }
    else
    {
        return pControl;
    }
}

CfxControl *CfxEngine::HitTestNoClip(CfxControl *pControl, int X, int Y)
{
    FXRECT rect;
    CfxApplication *application = GetApplication(this);
    application->GetBounds(&rect);
    FXPOINT point = {X, Y};

    return HitControlNoClip(pControl, &rect, &point);
}

void CfxEngine::PenDown(int X, int Y)
{
    if (_inputBlocked) return;

    _lastUserEventTime = FxGetTickCount();

    _penDownX = X;
    _penDownY = Y;
    _penMoved = false;

    CfxControl *control = HitTest(GetScreen(this), X, Y);

    if (control && control->GetEnabled())
    {
        FXRECT rect;
        _captureControl = control;
        _captureControl->GetAbsoluteBounds(&rect);
        _captureControl->OnPenDown(X - rect.Left, Y - rect.Top);
    }
}

void CfxEngine::PenMove(int X, int Y)
{
    if (_inputBlocked) return;

    if (!_penMoved && _captureControl && _captureControl->GetSupportsDrag() &&
        ((abs(X - _penDownX) > 10) || (abs(Y - _penDownY) > 10)))
    {
        _penMoved = true;
    }

    CfxControl *control = _captureControl ? _captureControl : HitTest(GetApplication(this)->GetScreen(), X, Y);

    if (control && control->GetEnabled())
    {
        FXRECT rect;
        control->GetAbsoluteBounds(&rect);
        control->OnPenMove(X - rect.Left, Y - rect.Top);

        if (_penMoved && _captureControl)
        {
            _captureControl->OnPenDrag(_penDownX - rect.Left, _penDownY - rect.Top, X - rect.Left, Y - rect.Top);
        }
    }
}

void CfxEngine::PenUp(int X, int Y)
{
    if (_inputBlocked) return;
    if (!_captureControl) return;
    CfxControl *captureControl = _captureControl;
    _captureControl = nullptr;

    FXRECT rect;
    captureControl->GetAbsoluteBounds(&rect);
    captureControl->OnPenUp(X - rect.Left, Y - rect.Top);

    FXPOINT point = {X, Y};
    if (!_penMoved && PtInRect(&rect, &point))
    {
        captureControl->OnPenClick(X - rect.Left, Y - rect.Top);
    }

    // No code here, since the click may have caused app termination.
}

enum KeyEvent {keDown, keUp};

bool RunKeyEvent(CfxControl *pControl, BYTE KeyCode, KeyEvent Event)
{
    bool handled = false;
    
    switch (Event)
    {
    case keDown:  pControl->OnKeyDown(KeyCode, &handled); break;
    case keUp:    pControl->OnKeyUp(KeyCode, &handled); break;
    }

    if (!handled)
    {
        for (int i=0; i<pControl->GetControlCount(); i++)
        {
            handled = RunKeyEvent(pControl->GetControl(i), KeyCode, Event);
            if (handled) break;
        }
    }

    return handled;
}

bool CfxEngine::KeyDown(BYTE KeyCode)
{
    if (_inputBlocked) return false;
    return RunKeyEvent(GetScreen(this), KeyCode, keDown);
}

bool CfxEngine::KeyUp(BYTE KeyCode)
{
    if (_inputBlocked) return false;
    return RunKeyEvent(GetScreen(this), KeyCode, keUp);
}

void CfxEngine::KeyPress(BYTE KeyCode, int DelayMs)
{
    if (DelayMs > 0)
    {
        _pendingKeyCode = KeyCode;
        _inputBlocked = true;
        _keyPressTimer->start(DelayMs);
        return;
    }

    KeyDown(KeyCode);
    KeyUp(KeyCode);
}

bool RunCloseDialog(CfxControl *pControl)
{
    bool handled = false;

    pControl->OnCloseDialog(&handled);
    
    if (!handled)
    {
        for (int i=0; i<pControl->GetControlCount(); i++)
        {
            handled = handled || RunCloseDialog(pControl->GetControl(i));
        }
    }

    return handled;
}

bool CfxEngine::CloseDialog()
{
    return RunCloseDialog(GetScreen(this));
}

void CfxEngine::Startup()
{
    CfxHost *host = GetHost(this);

    _terminating = false;

    _lastUserEventTime = 0;
    host->Startup();
    AlignControls(GetScreen(this));

    FXPROFILE *profile = host->GetProfile();
    _cornerWidth1 = (4 * profile->ScaleX) / 100;
    _cornerWidth2 = (8 * profile->ScaleX) / 100;
}

void CfxEngine::Shutdown()
{

    KillTimer(this);
    
    if (_terminating) return;
    _terminating = true;

    GetHost(this)->Shutdown();
}

void CfxEngine::Repaint()
{
    GetHost(this)->Paint();
}

void CfxEngine::OnTimer()
{
    if (_blockPaintCount == 0)
    {
        Repaint();
    }
}

void CfxEngine::SetPaintTimer(int Elapse)
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

void CfxEngine::DoTimer(int TimerId)
{
    // Normal timer
    FX_ASSERT((int)TimerId >= 0);
    
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

void CfxEngine::SetTimer(CfxPersistent *pObject, int Elapse)
{
    // Check if we already have a timer on this control
    int index = _timers.IndexOf(pObject);
    if (index == -1)
    {
        // Find an empty index
        index = _timers.IndexOf(nullptr);
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

void CfxEngine::KillTimer(CfxPersistent *pObject)
{
    int index = _timers.IndexOf(pObject);
    if (index != -1)
    {
        GetHost(this)->KillTimer(index);
        _timers.Put(index, nullptr);
    }
}

bool CfxEngine::HasTimer(CfxPersistent *pObject)
{
    return _timers.IndexOf(pObject) != -1;
}

void CfxEngine::SetNextAlarm()
{
    ALARM_ITEM *alarmItem, *nextAlarmItem;
    FXDATETIME time;
    double currentTime;
    CfxHost *host;
    double elapseInSeconds;
    int i;
    CfxPersistent *alarmObject;

    _alarmSet = false;

    host = GetHost(this);
    host->KillAlarm();
    host->GetDateTime(&time);
    currentTime = EncodeDateTime(&time);

    // Find the next alarm.
    nextAlarmItem = nullptr;
    for (i = 0; i < _alarms.GetCount(); i++)
    {
        alarmItem = _alarms.GetPtr(i);
        alarmObject = alarmItem->Object;

        if ((alarmObject == nullptr) || !alarmItem->Active) continue;

        if ((nextAlarmItem == nullptr) || (alarmItem->ExpectedTime < nextAlarmItem->ExpectedTime))
        {
            nextAlarmItem = alarmItem;
        } 
    }

    // Set the next alarm if found.
    if (nextAlarmItem != nullptr)
    {
        elapseInSeconds = (nextAlarmItem->ExpectedTime - currentTime) * 24 * 60 * 60;
        _alarmSet = true;
        host->SetAlarm((int)max(10, (int)elapseInSeconds));
    }
}

void CfxEngine::DoAlarm()
{
    if (_alarmMutex)
    {
        return;
    }
    _alarmMutex = true;

    FXDATETIME time;
    double currentTime, deltaTime;
    ALARM_ITEM *alarmItem;
    int i;
    CfxPersistent *alarmObject;

    _alarmSet = false;

    GetHost(this)->GetDateTime(&time);
    currentTime = EncodeDateTime(&time);

    for (i = 0; i < _alarms.GetCount(); i++)
    {
        alarmItem = _alarms.GetPtr(i);
        alarmObject = alarmItem->Object;
        if ((alarmObject == nullptr) || !alarmItem->Active) continue;

        deltaTime = (int)((alarmItem->ExpectedTime - currentTime) * 24 * 60 * 60);

        if (deltaTime <= 3)
        {
            alarmItem->Active = false;
            alarmObject->OnAlarm();
        }
    }

    if (!_alarmSet)
    {
        SetNextAlarm();
    }

    _alarmMutex = false;
}

void CfxEngine::SetAlarm(CfxPersistent *pObject, int ElapseInSeconds, const char *pName)
{
    ALARM_ITEM *alarmItem, *existingAlarmItem;
    ALARM_ITEM newAlarmItem = {0};
    int i;
    FXDATETIME time;
    double currentTime;

    GetHost(this)->GetDateTime(&time);
    currentTime = EncodeDateTime(&time);

    newAlarmItem.ExpectedTime = currentTime + (double)ElapseInSeconds / (24 * 60 * 60);
    newAlarmItem.Object = pObject;
    newAlarmItem.Active = true;
    newAlarmItem.Name = pName;

    existingAlarmItem = nullptr;
    for (i = 0; i < _alarms.GetCount(); i++)
    {
        alarmItem = _alarms.GetPtr(i);
        
        if (alarmItem->Object == pObject)
        {
            existingAlarmItem = alarmItem;
            break;
        }

        if (alarmItem->Object == nullptr)
        {
            existingAlarmItem = alarmItem;
            break;
        }
    }

    if (existingAlarmItem == nullptr)
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

void CfxEngine::KillAlarm(CfxPersistent *pObject)
{
    ALARM_ITEM *alarmItem;
    int i;

    for (i = 0; i < _alarms.GetCount(); i++)
    {
        alarmItem = _alarms.GetPtr(i);
        if (alarmItem->Object == pObject)
        {
            alarmItem->Object = nullptr;
            alarmItem->Active = false;
            break;
        }
    }

    SetNextAlarm();
}

bool CfxEngine::HasAlarm(CfxPersistent *pObject)
{
    ALARM_ITEM *alarmItem;
    int i;
    bool result = false;

    for (i = 0; i < _alarms.GetCount(); i++)
    {
        alarmItem = _alarms.GetPtr(i);
        if ((alarmItem->Object == pObject) && alarmItem->Active)
        {
            result = true;
            break;
        }
    }

    return result;
}

void CfxEngine::DoTrackTimer()
{
    if (_trackTimer)
    {
        _trackTimer->OnTrackTimer();
    }
}

void CfxEngine::SetTrackTimer(CfxPersistent *pObject, int ElapseInSeconds)
{
    _trackTimer = pObject;
    GetHost(this)->SetTrackTimer(ElapseInSeconds);
}

void CfxEngine::KillTrackTimer(CfxPersistent *pObject)
{
    if (_trackTimer && (pObject != _trackTimer))
    {
        Log("TrackTimer: ERROR - wrong track timer object");
    }

    if (_trackTimer)
    {
        GetHost(this)->SetTrackTimer(0);
        _trackTimer = nullptr;
    }
}

void CfxEngine::DoUrlRequestCompleted(int Id, const char *pFileName, int ErrorCode)
{
    int i;
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

void CfxEngine::SetUrlRequest(CfxPersistent *pObject, char *pUrl, char *pUserName, char *pPassword, CfxStream &Data)
{
    CancelUrlRequest(pObject);
    
    URL_REQUEST_ITEM newItem;
    memset(&newItem, 0, sizeof(newItem));
    newItem.Object = pObject;
    newItem.Data = (BYTE *)Data.CloneMemory();
    newItem.DataSize = Data.GetSize();
    char *fileName = GetHost(this)->AllocPathApplication("UrlRequest");

    GetHost(this)->RequestUrl(pUrl, pUserName, pPassword, Data, fileName, &newItem.Id);
    if (newItem.Id > 0)
    {
        _urlRequests.Add(newItem);
    }

    MEM_FREE(fileName);
}

void CfxEngine::CancelUrlRequest(CfxPersistent *pObject)
{
    int i;

    for (i = 0; i < _urlRequests.GetCount(); i++)
    {
        URL_REQUEST_ITEM *item = _urlRequests.GetPtr(i);
        if (item->Object == pObject)
        {
            MEM_FREE(item->Data);
            item->Data = nullptr;
            _urlRequests.Delete(i);
            break;
        }
    }
}

void CfxEngine::DoPortData(FX_COMPORT_DATA *pComPortData)
{
    PORT_ITEM *portItem;
    int i;
    int portsUniqueness = _portsUniqueness;

    for (i = 0; i < _ports.GetCount(); i++)
    {
        portItem = _ports.GetPtr(i);
        if ((portItem->PortId == pComPortData->PortId) &&
            (portItem->Object != nullptr))
        {
            portItem->Object->OnPortData(pComPortData->PortId, pComPortData->Data, pComPortData->Length);
        }

        if (_portsUniqueness != portsUniqueness) break;
    }
}

bool CfxEngine::StartRecording(CfxPersistent *pObject, char *pMediaPath, int Duration, int Frequency)
{
    StopRecording();

    _recorder = pObject;

    return GetHost(this)->StartRecording(pMediaPath, Duration, Frequency);
}

bool CfxEngine::StopRecording()
{
    bool result = false;

    if (_recorder != nullptr)
    {
        result = GetHost(this)->StopRecording();
    }

    return result;
}

void CfxEngine::CancelRecording()
{
    if (_recorder != nullptr)
    {
        _recorder = nullptr;
        GetHost(this)->CancelRecording();
    }
}

bool CfxEngine::IsRecording()
{
    return (_recorder != nullptr) && GetHost(this)->IsRecording();
}

void CfxEngine::DoRecordingComplete(char *pFileName, int Duration)
{
    if ((_recorder != nullptr) && (pFileName != nullptr))
    {
        _recorder->OnRecordingComplete(pFileName, Duration);
    }

    _recorder = nullptr;
}

void CfxEngine::DoPictureTaken(CfxControl *pControl, char *pFileName, bool Success)
{
    pControl->OnPictureTaken(pFileName, Success);

    for (int i=0; i < pControl->GetControlCount(); i++)
    {
        DoPictureTaken(pControl->GetControl(i), pFileName, Success);
    }
}

void CfxEngine::DoBarcodeScan(CfxControl *pControl, char *pBarcode, bool Success)
{
    pControl->OnBarcodeScan(pBarcode, Success);

    for (int i=0; i < pControl->GetControlCount(); i++)
    {
        DoBarcodeScan(pControl->GetControl(i), pBarcode, Success);
    }
}

void CfxEngine::DoPhoneNumberTaken(CfxControl *pControl, char *pPhoneNumber, bool Success)
{
    pControl->OnPhoneNumberTaken(pPhoneNumber, Success);

    for (int i=0; i < pControl->GetControlCount(); i++)
    {
        DoPhoneNumberTaken(pControl->GetControl(i), pPhoneNumber, Success);
    }
}

bool CfxEngine::IsPortConnected(BYTE PortId)
{
    return GetHost(this)->IsPortConnected(PortId);
}

void CfxEngine::CyclePort(BYTE PortId, FX_COMPORT *pComPort)
{
    CfxHost *host = GetHost(this);

    if (!host->IsPortConnected(PortId))
    {
        host->DisconnectPort(PortId);
        host->ConnectPort(PortId, pComPort);
    }
}

bool CfxEngine::ConnectPort(CfxPersistent *pObject, BYTE PortId, FX_COMPORT *pComPort)
{
    PORT_ITEM *portItem;
    PORT_ITEM newPortItem = {0};
    int i;
    bool alreadyConnected = false;

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
            return true;
        }

        if (memcmp(pComPort, &portItem->ComPort, sizeof(FX_COMPORT)) != 0)
        {
            // Another connection with different parameters!
            return false;
        }

        alreadyConnected = true;
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
        return true;
    }
    else
    {
        return false;
    }
}

void CfxEngine::DisconnectPort(CfxPersistent *pObject, BYTE PortId, bool KeepConnected)
{
    int i = 0;
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

    if (KeepConnected == false)
    {
        GetHost(this)->DisconnectPort(PortId);
    }
}

void CfxEngine::WritePortData(BYTE PortId, BYTE *pData, int Length)
{
    GetHost(this)->WritePortData(PortId, pData, Length);
}


void CfxEngine::SetGPSTimeSync(bool Enabled, int Zone)
{
    _gpsTimeSync = Enabled;
    _gpsTimeZone = Zone;
}

FXGPS *CfxEngine::GetGPS()
{
    CfxHost *host = GetHost(this);

    host->GetGPS(&_gps);

    /*
    {
        SYSTEM_POWER_STATUS_EX2 sps;
        GetSystemPowerStatusEx2(&sps, sizeof(sps), true);
        _gps.Position.Altitude = sps.BatteryLifePercent;
    }
    */

    if (TestGPS(&_gps, GPS_DEFAULT_ACCURACY))
    {
        _gpsLastFix = _gps.Position;
        _gpsLastHeading = _gps.Heading;
        GetHost(this)->GetDateTime(&_gpsLastTimeStamp);
    }

    _gps.TimeInSync = false;
    if (_gpsTimeSync && (_gps.DateTime.Year != 0))
    {
        double now, gpsNow;
        FXDATETIME dateTime;
        int timeDiff;
        
        host->GetDateTime(&dateTime);
        now = EncodeDateTime(&dateTime);
    
        gpsNow = EncodeDateTime(&_gps.DateTime);
        gpsNow += ((double)_gpsTimeZone / 100) / 24;

        timeDiff = FxAbs((int)((gpsNow - now) * 24 * 60));
        if ((timeDiff > 2) || _gpsTimeFirst)
        {
            // Ensure that the GPS reading is not old
            int tickDiff = (FxGetTickCount() - _gps.TimeStamp) / FxGetTicksPerSecond();
            if (tickDiff < 3)
            {
                _gpsTimeFirst = false;
                DecodeDateTime(gpsNow, &dateTime);
                host->SetTimeZone((_gpsTimeZone / 100) * 60);
                host->SetDateTime(&dateTime);

                host->SetDateTimeFromGPS(&dateTime);

                _gps.TimeInSync = true;
                _gpsTimeUsedOnce = true;
            }
            else
            {
                // We need to set the time, but the data is old
                _gps.TimeInSync = false;
            }
        }
        else 
        {
            _gps.TimeInSync = true;
            _gpsTimeUsedOnce = true;
        }
    }

    return &_gps;
}

void CfxEngine::LockGPS()
{
    g_gpsLockCount++;
    GetHost(this)->SetGPS(true);
}

void CfxEngine::UnlockGPS()
{
    FX_ASSERT(g_gpsLockCount > 0);

    g_gpsLockCount--;

    if (g_gpsLockCount == 0)
    {
        GetHost(this)->SetGPS(false);
    }
}

void CfxEngine::DoStartGPS()
{
    if (g_gpsLockCount > 0)
    {
        GetHost(this)->SetGPS(true);
    }
}

void CfxEngine::DoWakeup()
{
    if (g_gpsLockCount > 0)
    {
        GetHost(this)->ResetGPSTimeouts();
    }

}

void CfxEngine::DoSleep()
{
}
