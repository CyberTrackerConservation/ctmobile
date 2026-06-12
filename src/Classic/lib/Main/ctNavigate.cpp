#include "fxApplication.h"
#include "ctSession.h"
#include "ctNavigate.h"
#include "fxUtils.h"
#include "ctElementList.h"

//*************************************************************************************************
// CctControl_NavButton

CfxControl *Create_Control_NavButton(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton(pOwner, pParent);
}

CctControl_NavButton::CctControl_NavButton(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Button(pOwner, pParent)
{
    InitLockProperties("Caption;Center;Stretch;Proportional;Image");

    _center = _proportional = true;
    _stretch = _transparentImage = false;
    _bitmap = 0;
}

CctControl_NavButton::~CctControl_NavButton()
{
    freeX(_bitmap);
}

void CctControl_NavButton::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    // This is so the navigate control can not worry about border scaling on the buttons
    if (!CompareGuid(_parent->GetTypeId(), &GUID_CONTROL_NAVIGATE))
    {
        CfxControl_Button::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    }
}

void CctControl_NavButton::DefineProperties(CfxFiler &F) 
{ 
    CfxControl_Button::DefineProperties(F);

    F.DefineValue("Transparent",  dtBoolean,  &_transparent, F_FALSE);

    F.DefineValue("Center",       dtBoolean, &_center, F_TRUE);
    F.DefineValue("Stretch",      dtBoolean, &_stretch, F_FALSE);
    F.DefineValue("Proportional", dtBoolean, &_proportional, F_TRUE);
    F.DefineValue("TransparentImage", dtBoolean, &_transparentImage, F_FALSE);
    F.DefineXBITMAP("Bitmap", &_bitmap);

    InternalDefineProperties(F);
}

void CctControl_NavButton::DefineResources(CfxFilerResource &F)
{
}

void CctControl_NavButton::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_NavButton::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    bool invert = GetDown();

    CfxResource *resource = GetResource();
    FXBITMAPRESOURCE *bitmap = resource->GetBitmap(this, _bitmap);
    if (bitmap)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, invert);
        pCanvas->FillRect(pRect);

        FXRECT rect;
        GetBitmapRect(bitmap, pRect->Right - pRect->Left + 1, pRect->Bottom - pRect->Top + 1, _center, _stretch, _proportional, &rect);
        pCanvas->StretchDraw(bitmap, pRect->Left + rect.Left, pRect->Top + rect.Top, rect.Right - rect.Left + 1 , rect.Bottom - rect.Top + 1, invert, _transparentImage);
        resource->ReleaseBitmap(bitmap);
    }
    else if (_caption && (strlen(_caption) > 0))
    {
        CfxControl_Button::OnPaint(pCanvas, pRect);         
    }
    else
    {
        InternalPaint(pCanvas, pRect, invert, _transparent);
    }
}

//*************************************************************************************************
// CctControl_NavButton_Back

CfxControl *Create_Control_NavButton_Back(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton_Back(pOwner, pParent);
}

CctControl_NavButton_Back::CctControl_NavButton_Back(CfxPersistent *pOwner, CfxControl *pParent): CctControl_NavButton(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVBUTTON_BACK);
}

void CctControl_NavButton_Back::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent)
{
    pCanvas->DrawBackArrow(pRect, _color, _borderColor, Invert, Transparent); 
}

void CctControl_NavButton_Back::OnPenClick(int X, int Y)
{
    GetSession(this)->DoBack();
}

void CctControl_NavButton_Back::OnKeyUp(BYTE KeyCode, bool *pHandled)
{
    if (KeyCode == KEY_LEFT)
    {
        if (GetVisible())
        {
            GetSession(this)->DoBack();
        }
        *pHandled = true;
    }
}

//*************************************************************************************************
// CctControl_NavButton_Next

CfxControl *Create_Control_NavButton_Next(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton_Next(pOwner, pParent);
}

CctControl_NavButton_Next::CctControl_NavButton_Next(CfxPersistent *pOwner, CfxControl *pParent): CctControl_NavButton(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVBUTTON_NEXT);
    InitLockProperties("Caption;Center;Stretch;Proportional;Image;Next screen");
    InitXGuid(&_nextScreenId);
    RegisterSessionEvent(seBeforeNext, this, (NotifyMethod)&CctControl_NavButton_Next::OnSessionNext);
}

CctControl_NavButton_Next::~CctControl_NavButton_Next()
{
    UnregisterSessionEvent(seBeforeNext, this);
}

void CctControl_NavButton_Next::OnSessionNext(CfxControl *pSender, void *pParams)
{
    if (!IsNullXGuid(&_nextScreenId))
    {
        CctAction_InsertScreen action(this);
        action.Setup(_nextScreenId);
        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

void CctControl_NavButton_Next::InternalDefineProperties(CfxFiler &F)
{
    F.DefineXOBJECT("NextScreenId", &_nextScreenId);
}

void CctControl_NavButton_Next::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent)
{
    pCanvas->DrawNextArrow(pRect, _color, _borderColor, Invert, Transparent); 
}

void CctControl_NavButton_Next::InternalDefineResources(CfxFilerResource &F)
{
}

void CctControl_NavButton_Next::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_NavButton_Next::OnPenClick(int X, int Y)
{
    GetSession(this)->DoNext();
}

void CctControl_NavButton_Next::OnKeyUp(BYTE KeyCode, bool *pHandled)
{
    if (KeyCode == KEY_RIGHT)
    {
        if (GetVisible())
        {
            GetSession(this)->DoNext();
        }
        *pHandled = true;
    }
}

//*************************************************************************************************
// CctControl_NavButton_Home

CfxControl *Create_Control_NavButton_Home(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton_Home(pOwner, pParent);
}

CctControl_NavButton_Home::CctControl_NavButton_Home(CfxPersistent *pOwner, CfxControl *pParent): CctControl_NavButton(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVBUTTON_HOME);
    InitLockProperties("Caption;Center;Stretch;Proportional;Image;Home screen");
    InitXGuid(&_homeScreenId);
}

void CctControl_NavButton_Home::InternalDefineProperties(CfxFiler &F)
{
    F.DefineXOBJECT("HomeScreenId", &_homeScreenId);
}

void CctControl_NavButton_Home::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent)
{
    pCanvas->DrawHome(pRect, _color, _borderColor, Invert, Transparent); 
}

void CctControl_NavButton_Home::InternalDefineResources(CfxFilerResource &F)
{
}

void CctControl_NavButton_Home::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_NavButton_Home::OnPenClick(int X, int Y)
{
    GetSession(this)->DoHome(_homeScreenId);
}

//*************************************************************************************************
// CctControl_NavButton_Skip

CfxControl *Create_Control_NavButton_Skip(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton_Skip(pOwner, pParent);
}

CctControl_NavButton_Skip::CctControl_NavButton_Skip(CfxPersistent *pOwner, CfxControl *pParent): CctControl_NavButton(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVBUTTON_SKIP);
    InitLockProperties("Caption;Center;Stretch;Proportional;Image;Skip screen");
    InitXGuid(&_skipScreenId);
    _filterExecuted = false;
    _filterCaption = nullptr;
    InitFont(F_FONT_DEFAULT_M);
}

CctControl_NavButton_Skip::~CctControl_NavButton_Skip()
{
    ClearFilterText();
}

void CctControl_NavButton_Skip::ClearFilterText()
{
    MEM_FREE(_filterCaption);
    _filterCaption = nullptr;
    _filterExecuted = false;
}

void CctControl_NavButton_Skip::Setup(XGUID SkipScreenId)
{
    _skipScreenId = SkipScreenId;
    ClearFilterText();
}

void CctControl_NavButton_Skip::ExecuteFilter()
{
    ClearFilterText();

    CfxResource *resource = GetResource(); 
    FXSCREENDATA *screenData = (FXSCREENDATA *)resource->GetObject(this, _skipScreenId);
    if (screenData)
    {
        FX_ASSERT(screenData->Magic == MAGIC_SCREEN);

        CfxStream stream(&screenData->Data[0]);
        CfxReader reader(&stream);

        CfxScreen screen(GetOwner(), this);
        screen.DefineProperties(reader);
        
        CctControl_ElementList *elementList = (CctControl_ElementList *)screen.FindControlByType(&GUID_CONTROL_ELEMENTLIST);
        if (elementList && elementList->IsFilterEnabled())
        {
            _filterCaption = (char *)MEM_ALLOC(16);
            itoa(elementList->GetItemCount(), _filterCaption, 10);
        }
        resource->ReleaseObject(screenData);
    }

    _filterExecuted = true;
}

void CctControl_NavButton_Skip::InternalDefineProperties(CfxFiler &F)
{
    F.DefineXOBJECT("SkipScreenId", &_skipScreenId);
}

void CctControl_NavButton_Skip::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent)
{
    if (!_filterExecuted)
    {
        ExecuteFilter();
    }

    FXRECT rect = *pRect;
    if (!Transparent)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, Invert);
        pCanvas->FillRect(&rect);
    }

    if (_filterCaption && (strlen(_filterCaption) > 0))
    {
        CfxResource *resource = GetResource(); 
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            int h, offsetY;
            h = pCanvas->TextHeight(font, _filterCaption, 0, &offsetY);
            rect.Right = rect.Left + GetClientWidth() / 2;
            rect.Top = rect.Top + (rect.Bottom - rect.Top + 1 - h - offsetY) / 2;

            pCanvas->State.TextColor = pCanvas->InvertColor(_borderColor, Invert);
            pCanvas->DrawTextRect(font, &rect, TEXT_ALIGN_HCENTER, _filterCaption);
            resource->ReleaseFont(font);
        }
        rect = *pRect;
        rect.Left = rect.Right - GetClientWidth() / 2;
    }
    
    pCanvas->DrawSkipArrow(&rect, _color, _borderColor, Invert, Transparent);
}

void CctControl_NavButton_Skip::InternalDefineResources(CfxFilerResource &F)
{
}

void CctControl_NavButton_Skip::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_NavButton_Skip::OnPenClick(int X, int Y)
{
    if (!IsNullXGuid(&_skipScreenId))
    {
        GetSession(this)->DoSkipToScreen(_skipScreenId);
    }
}

//*************************************************************************************************
// CctControl_NavButton_Save

CfxControl *Create_Control_NavButton_Save(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton_Save(pOwner, pParent);
}

CctControl_NavButton_Save::CctControl_NavButton_Save(CfxPersistent *pOwner, CfxControl *pParent): CctControl_NavButton(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVBUTTON_SAVE);
    InitLockProperties("Caption;Center;Stretch;Proportional;Image;Save screen;Ensure result on save;Take GPS;Save type");
    InitXGuid(&_saveScreenId);
    _takeGPS = false;
    _saveNumber = 0;
    _gpsSkipTimeout = 0;
}

void CctControl_NavButton_Save::InternalDefineProperties(CfxFiler &F)
{
    F.DefineValue("TakeGPS",        dtBoolean, &_takeGPS);
    F.DefineValue("GpsSkipTimeout", dtInt32,   &_gpsSkipTimeout);
    F.DefineValue("SaveNumber",     dtInt32,   &_saveNumber);

    F.DefineXOBJECT("SaveScreenId", &_saveScreenId);
}

void CctControl_NavButton_Save::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, Invert);
        pCanvas->FillRect(pRect);
    }
    pCanvas->State.BrushColor = pCanvas->InvertColor(_borderColor, Invert);
    pCanvas->State.LineColor = pCanvas->InvertColor(_borderColor, Invert);

    int w = GetClientWidth() - 2;
    int h = GetClientHeight() - 2;

    int t = min(w, h) - 1;
    int x = pRect->Left + (w - t) / 2 + 1;
    int y = pRect->Top + (h - t) / 2 + 1;

    // Draw bottom tray
    int s = max(1, t / 10);
    FXRECT rect;
    rect.Left    = x;
    rect.Top     = y + t - s + 1;
    rect.Right   = x + t - 1;
    rect.Bottom  = y + t; 
    pCanvas->FillRect(&rect);
    
    rect.Top     = y + t - s * 3 + 1;
    rect.Right   = x + s - 1;
    rect.Bottom  = rect.Bottom - s;
    pCanvas->FillRect(&rect);

    rect.Right   = x + t - 1;
    rect.Left    = x + t - s; 
    pCanvas->FillRect(&rect);

    // Draw top middle box
    int boxW    = t / 2;
    int boxH    = t / 3;
    rect.Left    = x + t / 2 - boxW / 2;
    rect.Right   = rect.Left + boxW;
    rect.Top     = y;
    rect.Bottom  = y + boxH;
    if (_saveNumber == 0) 
    {
        pCanvas->FillRect(&rect); 
    }
    else 
    {
        pCanvas->FrameRect(&rect);
    }

    // Draw triangle
    int triSide = (int)t / 2;
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{0, 0}, {triSide * 2, 0}, {triSide, triSide}};
    if (_saveNumber == 0)
    {
        FILLPOLYGON(pCanvas, triangle, x + t / 2 - triSide, y + boxH);
    }
    else
    {
        POLYGON(pCanvas, triangle, x + t / 2 - triSide, y + boxH);
        rect.Top = rect.Bottom;
        rect.Left++;
        rect.Right--;
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, Invert);
        pCanvas->FillRect(&rect);
    }

    /*int w = GetClientWidth() / 2 - 2;
    int h = GetClientHeight() / 2 - 2;
    h = min(h, w);
    w = max(4, h / 2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{h, 0}, {h, h*2}, {0, h}};

    int x = pRect->Left + (GetClientWidth() - h - w) / 2 - 1 * _borderLineWidth;
    int y = pRect->Top + GetClientHeight() / 2 - h;
    if (_saveNumber == 0)
    {
        FILLPOLYGON(pCanvas, triangle, x, y);
        pCanvas->FillRect(x + h + 3 * _borderLineWidth, y, w, h * 2 + 1);
    }
    else
    {
        pCanvas->State.LineWidth = _borderLineWidth;
        POLYGON(pCanvas, triangle, x, y);
        pCanvas->FrameRect(x + h + 3 * _borderLineWidth, y, w, h * 2 + 1);
    }*/
}

void CctControl_NavButton_Save::InternalDefineResources(CfxFilerResource &F)
{
}

void CctControl_NavButton_Save::Setup(bool TakeGPS, int GpsSkipTimeout, XGUID SaveScreenId)
{
    _takeGPS = TakeGPS;
    _saveScreenId = SaveScreenId;
    _gpsSkipTimeout = GpsSkipTimeout;
}

void CctControl_NavButton_Save::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_NavButton_Save::OnPenClick(int X, int Y)
{
    CctSession *session = GetSession(this);

    if (!session->CanDoNext()) return;

    XGUID saveScreenId = _saveScreenId;
    if (IsNullXGuid(&saveScreenId))
    {
        XGUID g1, g2;
        session->GetDefaultSaveTargets(&g1, &g2);
        saveScreenId = (_saveNumber == 0) ? g1 : g2;
    }

    if (_takeGPS && (session->GetCurrentSighting()->GetGPS()->Quality == FXGPS_QUALITY::None))
    {
        int skipTimeout = (_gpsSkipTimeout != 0) ? _gpsSkipTimeout :
                                                   session->GetResourceHeader()->GpsSkipTimeout;
        bool skipAllowed = session->GetResourceHeader()->ManualAllowSkip;

        GetSession(this)->ShowGPSReaderPage(saveScreenId, skipTimeout, skipAllowed);
    }
    else
    {
        session->DoCommit(saveScreenId);
    }
}

//*************************************************************************************************
// CctControl_NavButton_Options

CfxControl *Create_Control_NavButton_Options(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton_Options(pOwner, pParent);
}

CctControl_NavButton_Options::CctControl_NavButton_Options(CfxPersistent *pOwner, CfxControl *pParent): CctControl_NavButton(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVBUTTON_OPTIONS);
}

void CctControl_NavButton_Options::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, Invert);
        pCanvas->FillRect(pRect);
    }
    pCanvas->State.BrushColor = pCanvas->InvertColor(_borderColor, Invert);

    int w = GetClientWidth() / 2 - 2;
    int h = GetClientHeight() / 2 - 2;
    h = min(h, w);
    int x = pRect->Left + GetClientWidth() / 2;
    int y = pRect->Top + GetClientHeight() / 2;

    pCanvas->FillEllipse(x, y, h, h);
}

void CctControl_NavButton_Options::OnPenClick(int X, int Y)
{
    GetSession(this)->ShowSightingEditorPage();
}

//*************************************************************************************************
// CctControl_NavButton_GPS

CfxControl *Create_Control_NavButton_GPS(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton_GPS(pOwner, pParent);
}

CctControl_NavButton_GPS::CctControl_NavButton_GPS(CfxPersistent *pOwner, CfxControl *pParent): CctControl_NavButton(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVBUTTON_GPS);
    InitFont(GPS_WAYPOINT_FONT);
}

void CctControl_NavButton_GPS::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, Invert);
        pCanvas->FillRect(pRect);
    }

    CfxResource *resource = GetResource();
    FXFONTRESOURCE *font = resource->GetFont(this, _font);
    if (font)
    {
        pCanvas->State.LineWidth = (int)_borderLineWidth;
        auto interval = GetHost(this)->GetTrackTimer();
        auto running = GetHost(this)->GetTrackTimerRunning();
        DrawWaypointIcon(pCanvas, pRect, font, interval, _color, _borderColor, running, !running, Invert);
        resource->ReleaseFont(font);
    }
}

void CctControl_NavButton_GPS::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_NavButton_GPS::OnPenClick(int X, int Y)
{
    GetSession(this)->ShowMapPage();
}

//*************************************************************************************************
// CctControl_NavButton_AcceptEdit

CfxControl *Create_Control_NavButton_AcceptEdit(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton_AcceptEdit(pOwner, pParent);
}

CctControl_NavButton_AcceptEdit::CctControl_NavButton_AcceptEdit(CfxPersistent *pOwner, CfxControl *pParent): CctControl_NavButton(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVBUTTON_ACCEPTEDIT);
}

void CctControl_NavButton_AcceptEdit::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent)
{
    FXRECT rect = *pRect;
    InflateRect(&rect, -(int)_borderLineWidth * 2, -(int)_borderLineWidth * 2);
    pCanvas->DrawCheck(&rect, _color, _borderColor, Invert, Transparent);
}

void CctControl_NavButton_AcceptEdit::OnPenClick(int X, int Y)
{
    if (IsSessionEditing(this))
    {
        GetSession(this)->DoAcceptEdit();
    }
}

//*************************************************************************************************
// CctControl_NavButton_CancelEdit

CfxControl *Create_Control_NavButton_CancelEdit(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton_CancelEdit(pOwner, pParent);
}

CctControl_NavButton_CancelEdit::CctControl_NavButton_CancelEdit(CfxPersistent *pOwner, CfxControl *pParent): CctControl_NavButton(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVBUTTON_CANCELEDIT);
}

void CctControl_NavButton_CancelEdit::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent)
{
    FXRECT rect = *pRect;
    InflateRect(&rect, -(int)_borderLineWidth * 2, -(int)_borderLineWidth * 2);
    pCanvas->DrawCross(&rect, _color, _borderColor, 2, Invert, Transparent);
}

void CctControl_NavButton_CancelEdit::OnPenClick(int X, int Y)
{
    if (IsSessionEditing(this))
    {
        GetSession(this)->DoCancelEdit();
    }
}

//*************************************************************************************************
// CctControl_NavButton_Jump

CfxControl *Create_Control_NavButton_Jump(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NavButton_Jump(pOwner, pParent);
}

CctControl_NavButton_Jump::CctControl_NavButton_Jump(CfxPersistent *pOwner, CfxControl *pParent): CctControl_NavButton(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVBUTTON_JUMP);
    InitLockProperties("Caption;Center;Stretch;Proportional;Image;Next screen;Base screen;Jump screen");
    InitXGuid(&_baseScreenId);
    InitXGuid(&_jumpScreenId);
}

void CctControl_NavButton_Jump::InternalDefineProperties(CfxFiler &F)
{
    F.DefineXOBJECT("BaseScreenId", &_baseScreenId);
    F.DefineXOBJECT("JumpScreenId", &_jumpScreenId);
}

void CctControl_NavButton_Jump::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, bool Invert, bool Transparent)
{
    if (!Transparent)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, Invert);
        pCanvas->FillRect(pRect);
    }

    FXRECT rect = *pRect;
    InflateRect(&rect, -(int)_borderLineWidth * 2, -(int)_borderLineWidth * 2);

    pCanvas->State.BrushColor = pCanvas->InvertColor(_borderColor, Invert);

    int w = rect.Right - rect.Left + 1;
    int h = rect.Bottom - rect.Top + 1;
    int nh = min(h, w);

    int x = rect.Left + (w - nh) / 2;
    int y = rect.Top + (h - nh) / 2;
    h = nh-1;

    FXPOINTLIST polygon;
    FXPOINT triangle1[] = {{h/2+1, 0}, {h, 0}, {h, h/2}};
    FILLPOLYGON(pCanvas, triangle1, x, y);

    int thickness = _borderLineWidth * 2;
    FXPOINT triangle2[] = {{h-thickness, 0}, {0, h-thickness}, {thickness, h}, {h, thickness}};
    FILLPOLYGON(pCanvas, triangle2, x, y);
}

void CctControl_NavButton_Jump::InternalDefineResources(CfxFilerResource &F)
{
}

void CctControl_NavButton_Jump::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_NavButton_Jump::OnPenClick(int X, int Y)
{
    GetSession(this)->DoJumpToScreen(_baseScreenId, _jumpScreenId);
}

//*************************************************************************************************
// CctControl_Navigate

CfxControl *Create_Control_Navigate(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_Navigate(pOwner, pParent);
}

CctControl_Navigate::CctControl_Navigate(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NAVIGATE);
    InitLockProperties("Save 1 target;Save 2 target;Skip screen;Show save 1;Show save 2;Show options;Show GPS;Show back;Show next;Next screen");

    InitXGuid(&_homeScreenId);
    InitXGuid(&_nextScreenId);
    InitXGuid(&_skipScreenId);
    InitXGuid(&_save1ScreenId);
    InitXGuid(&_save2ScreenId);

    _dock = ControlDock::Bottom;

    _buttonWidth = 28;
    _height = 30;
    _buttonBorderStyle = ControlBorderStyle::None;
    _buttonBorderLineWidth = 1;
    _buttonBorderWidth = 1;
    _takeGPS = true;
    _gpsSkipTimeout = 0;
    InitFont(_timerFont, GPS_WAYPOINT_FONT);

    _showHome = false;
    _showOptions = true;
    _showGPS = true;
    _showSave1 = _showSave2 = false;
    _showBack = _showNext = true;
    _showSkip = false;

    InitFont(_skipFont, F_FONT_DEFAULT_M);

    // Create edit buttons
    _acceptEditButton = new CctControl_NavButton_AcceptEdit(pOwner, this);
    _acceptEditButton->SetComposite(true);
    _cancelEditButton = new CctControl_NavButton_CancelEdit(pOwner, this);
    _cancelEditButton->SetComposite(true);

    // Create dialog buttons
    _optionsButton = new CctControl_NavButton_Options(pOwner, this);
    _optionsButton->SetComposite(true);
    _gpsButton = new CctControl_NavButton_GPS(pOwner, this);
    _gpsButton->SetComposite(true);

    // Create navigation buttons
    _homeButton = new CctControl_NavButton_Home(pOwner, this);
    _homeButton->SetComposite(true);
    _skipButton = new CctControl_NavButton_Skip(pOwner, this);
    _skipButton->SetComposite(true);

    _save1Button = new CctControl_NavButton_Save(pOwner, this);
    _save1Button->SetComposite(true);
    _save1Button->SetSaveNumber(0);
    _save2Button = new CctControl_NavButton_Save(pOwner, this);
    _save2Button->SetComposite(true);
    _save2Button->SetSaveNumber(1);

    _backButton = new CctControl_NavButton_Back(pOwner, this);
    _backButton->SetComposite(true);

    _nextButton = new CctControl_NavButton_Next(pOwner, this);
    _nextButton->SetComposite(true);

    // Update
    UpdateButtons();
}

CctControl_Navigate::~CctControl_Navigate()
{
    // Create edit buttons
    _acceptEditButton = nullptr;
    _cancelEditButton = nullptr;
    _optionsButton = nullptr;
    _gpsButton = nullptr;
    _homeButton = nullptr;

    _skipButton = nullptr;
    _save1Button = nullptr;
    _save2Button = nullptr;

    _backButton = nullptr;
    _nextButton = nullptr;
}

void CctControl_Navigate::UpdateButtons()
{
    bool editing = IsSessionEditing(this);

    // Sighting editing buttons
    _acceptEditButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
    _acceptEditButton->SetColor(_color);
    _acceptEditButton->SetVisible(editing && (_showSave1 || _showSave2));

    _cancelEditButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
    _cancelEditButton->SetColor(_color);
    _cancelEditButton->SetVisible(editing);

    // Dialog buttons
    _optionsButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
    _optionsButton->SetColor(_color);
    _optionsButton->SetVisible(_showOptions && !editing);

    _gpsButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
    _gpsButton->SetColor(_color);
    _gpsButton->AssignFont(&_timerFont);
    _gpsButton->SetVisible(_showGPS && !editing);

    // Navigation buttons
    _homeButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
    _homeButton->SetColor(_color);
    _homeButton->SetVisible(_showHome && !editing);
    _homeButton->Setup(_homeScreenId);

    _skipButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
    _skipButton->SetColor(_color);
    _skipButton->AssignFont(&_skipFont);
    _skipButton->SetVisible(!IsNullXGuid(&_skipScreenId) || _showSkip);
    _skipButton->Setup(_skipScreenId);

    _save1Button->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
    _save1Button->SetColor(_color);
    _save1Button->Setup(_takeGPS, _gpsSkipTimeout, _save1ScreenId);
    _save1Button->SetVisible(_showSave1 && !editing);

    _save2Button->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
    _save2Button->SetColor(_color);
    _save2Button->Setup(_takeGPS, _gpsSkipTimeout, _save2ScreenId);
    _save2Button->SetVisible(_showSave2 && !editing);

    _backButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
    _backButton->SetColor(_color);
    _backButton->SetVisible(_showBack && (!IsLive() || !GetSession(this)->GetCurrentSighting()->BOP()));

    _nextButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
    _nextButton->SetColor(_color);
    _nextButton->SetVisible(_showNext);
    _nextButton->Setup(_nextScreenId);
}

void CctControl_Navigate::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    //_borderColor = RGB(241, 227, 199);
    //_borderColor = RGB(255, 255, 255);
    //_color = RGB(74, 37, 0);

    F.DefineValue("ButtonWidth",           dtInt32,   &_buttonWidth, "28");
    F.DefineValue("ButtonBorderStyle",     dtInt8,    &_buttonBorderStyle, F_ZERO);
    F.DefineValue("ButtonBorderWidth",     dtInt32,   &_buttonBorderWidth, F_ONE);
    F.DefineValue("ButtonBorderLineWidth", dtInt32,   &_buttonBorderLineWidth, F_ONE);

    F.DefineValue("ShowEdit",    dtBoolean,  &_showOptions, F_TRUE);
    F.DefineValue("ShowGPS",     dtBoolean,  &_showGPS, F_TRUE);
    F.DefineValue("ShowHome",    dtBoolean,  &_showHome, F_FALSE);
    F.DefineValue("ShowMajor",   dtBoolean,  &_showSave1, F_FALSE);
    F.DefineValue("ShowMinor",   dtBoolean,  &_showSave2, F_FALSE);
    F.DefineValue("ShowBack",    dtBoolean,  &_showBack, F_TRUE);
    F.DefineValue("ShowNext",    dtBoolean,  &_showNext, F_TRUE);
    F.DefineValue("ShowSkip",    dtBoolean,  &_showSkip, F_FALSE);
    F.DefineValue("TakeGPS",     dtBoolean,  &_takeGPS, F_TRUE);
    F.DefineValue("GpsSkipTimeout", dtInt32, &_gpsSkipTimeout, F_ZERO);

    F.DefineXOBJECT("MajorScreenId", &_save1ScreenId);
    F.DefineXOBJECT("MinorScreenId", &_save2ScreenId);
    F.DefineXOBJECT("NextScreenId",  &_nextScreenId);
    F.DefineXFONT("TimerFont", &_timerFont, GPS_WAYPOINT_FONT);
    F.DefineXFONT("SkipFont", &_skipFont, F_FONT_DEFAULT_M);
    F.DefineXOBJECT("SkipScreenId", &_skipScreenId);
    F.DefineXOBJECT("HomeScreenId", &_homeScreenId);

    UpdateButtons();

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);

    }
}

void CctControl_Navigate::DefinePropertiesUI(CfxFilerUI &F)
{
} 

void CctControl_Navigate::DefineResources(CfxFilerResource &F)
{
}

void CctControl_Navigate::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    int th = GetClientHeight();
    int tx = _borderWidth;

    if (_acceptEditButton->GetVisible())
    {
        _acceptEditButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
        tx += _buttonWidth + 4;
    }

    if (_cancelEditButton->GetVisible())
    {
        _cancelEditButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
        tx += _buttonWidth + 4;
    }

    if (_homeButton->GetVisible())
    {
        _homeButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
        tx += _buttonWidth + 4;
    }

    if (_optionsButton->GetVisible())
    {
        _optionsButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
        tx += _buttonWidth + 4;
    }
    
    if (_gpsButton->GetVisible())
    {
        _gpsButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
        tx += _buttonWidth + 4;
    }

    if (_skipButton->GetVisible())
    {
        _skipButton->SetBounds(tx, _borderWidth, _buttonWidth * 2, th);
    }

    // rightmost
    tx = Width - _borderWidth;

    _nextButton->SetBounds(tx - _buttonWidth, _borderWidth, _buttonWidth, th);

    tx = tx - _buttonWidth - 2;
    _backButton->SetBounds(tx - _buttonWidth, _borderWidth, _buttonWidth, th);
    
    tx = tx - _buttonWidth - 4;
    _save2Button->SetBounds(tx - _buttonWidth, _borderWidth, _buttonWidth, th);

    tx = tx - _buttonWidth - 2;
    _save1Button->SetBounds(tx - _buttonWidth, _borderWidth, _buttonWidth, th);
}

void CctControl_Navigate::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _buttonWidth = (_buttonWidth * ScaleX) / 100;
    _buttonWidth = max(16, _buttonWidth);
    _buttonWidth = max(ScaleHitSize, _buttonWidth);

    int t = _buttonBorderWidth;
    _buttonBorderWidth = (_buttonBorderWidth * ScaleBorder) / 100;
    if ((_buttonBorderWidth == 0) && (t > 0))
    {
        _buttonBorderWidth = 1;
    }

    _buttonBorderLineWidth = max(1, (_buttonBorderLineWidth * ScaleBorder) / 100);

    _height = max((int)ScaleHitSize, _height);

    UpdateButtons();
}
