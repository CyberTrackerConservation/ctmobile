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
#include "ctSession.h"
#include "ctNavigate.h"
#include "fxUtils.h"
#include "ctDialog_SightingEditor.h"
#include "ctDialog_GPSViewer.h"
#include "ctDialog_GPSReader.h"
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

    _center = _proportional = TRUE;
    _stretch = _transparentImage = FALSE;
    _bitmap = 0;
}

CctControl_NavButton::~CctControl_NavButton()
{
    freeX(_bitmap);
}

VOID CctControl_NavButton::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    // This is so the navigate control can not worry about border scaling on the buttons
    if (!CompareGuid(_parent->GetTypeId(), &GUID_CONTROL_NAVIGATE))
    {
        CfxControl_Button::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    }
}

VOID CctControl_NavButton::DefineProperties(CfxFiler &F) 
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

VOID CctControl_NavButton::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Button::DefineResources(F);
    F.DefineBitmap(_bitmap);
    InternalDefineResources(F);
#endif
}

VOID CctControl_NavButton::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("Center",       dtBoolean,  &_center);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Image",        dtPGraphic, &_bitmap,      "BITDEPTH(16);HINTWIDTH(1024);HINTHEIGHT(1024)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Proportional", dtBoolean,  &_proportional);
    F.DefineValue("Stretch",      dtBoolean,  &_stretch);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_NavButton::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    BOOL invert = GetDown();

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

VOID CctControl_NavButton_Back::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent)
{
    pCanvas->DrawBackArrow(pRect, _color, _borderColor, Invert, Transparent); 
}

VOID CctControl_NavButton_Back::OnPenClick(INT X, INT Y)
{
    GetSession(this)->DoBack();
}

VOID CctControl_NavButton_Back::OnKeyUp(BYTE KeyCode, BOOL *pHandled)
{
    if (KeyCode == KEY_LEFT)
    {
        if (GetVisible())
        {
            GetSession(this)->DoBack();
        }
        *pHandled = TRUE;
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

VOID CctControl_NavButton_Next::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    if (!IsNullXGuid(&_nextScreenId))
    {
        CctAction_InsertScreen action(this);
        action.Setup(_nextScreenId);
        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

VOID CctControl_NavButton_Next::InternalDefineProperties(CfxFiler &F)
{
    F.DefineXOBJECT("NextScreenId", &_nextScreenId);
}

VOID CctControl_NavButton_Next::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent)
{
    pCanvas->DrawNextArrow(pRect, _color, _borderColor, Invert, Transparent); 
}

VOID CctControl_NavButton_Next::InternalDefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineObject(_nextScreenId);
#endif
}

VOID CctControl_NavButton_Next::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    CctControl_NavButton::DefinePropertiesUI(F);
    F.DefineStaticLink("Next screen",  &_nextScreenId);
#endif
}

VOID CctControl_NavButton_Next::OnPenClick(INT X, INT Y)
{
    GetSession(this)->DoNext();
}

VOID CctControl_NavButton_Next::OnKeyUp(BYTE KeyCode, BOOL *pHandled)
{
    if (KeyCode == KEY_RIGHT)
    {
        if (GetVisible())
        {
            GetSession(this)->DoNext();
        }
        *pHandled = TRUE;
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

VOID CctControl_NavButton_Home::InternalDefineProperties(CfxFiler &F)
{
    F.DefineXOBJECT("HomeScreenId", &_homeScreenId);
}

VOID CctControl_NavButton_Home::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent)
{
    pCanvas->DrawHome(pRect, _color, _borderColor, Invert, Transparent); 
}

VOID CctControl_NavButton_Home::InternalDefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineObject(_homeScreenId);
#endif
}

VOID CctControl_NavButton_Home::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    CctControl_NavButton::DefinePropertiesUI(F);
    F.DefineStaticLink("Home screen", &_homeScreenId);
#endif
}

VOID CctControl_NavButton_Home::OnPenClick(INT X, INT Y)
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
    _filterExecuted = FALSE;
    _filterCaption = NULL;
    InitFont(F_FONT_DEFAULT_M);
}

CctControl_NavButton_Skip::~CctControl_NavButton_Skip()
{
    ClearFilterText();
}

VOID CctControl_NavButton_Skip::ClearFilterText()
{
    MEM_FREE(_filterCaption);
    _filterCaption = NULL;
    _filterExecuted = FALSE;
}

VOID CctControl_NavButton_Skip::Setup(XGUID SkipScreenId)
{
    _skipScreenId = SkipScreenId;
    ClearFilterText();
}

VOID CctControl_NavButton_Skip::ExecuteFilter()
{
    ClearFilterText();

    CfxResource *resource = GetResource(); 
    FXSCREENDATA *screenData = (FXSCREENDATA *)resource->GetObject(this, _skipScreenId);
    if (screenData)
    {
        FX_ASSERT(screenData->Magic == MAGIC_SCREEN);

        CfxStream stream(&screenData->Data[0]);
        CfxReader reader(&stream);

        CfxScreen screen(_owner, this);
        screen.DefineProperties(reader);
        
        CctControl_ElementList *elementList = (CctControl_ElementList *)screen.FindControlByType(&GUID_CONTROL_ELEMENTLIST);
        if (elementList && elementList->IsFilterEnabled())
        {
            _filterCaption = (CHAR *)MEM_ALLOC(16);
            itoa(elementList->GetItemCount(), _filterCaption, 10);
        }
        resource->ReleaseObject(screenData);
    }

    _filterExecuted = TRUE;
}

VOID CctControl_NavButton_Skip::InternalDefineProperties(CfxFiler &F)
{
    F.DefineXOBJECT("SkipScreenId", &_skipScreenId);
}

VOID CctControl_NavButton_Skip::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent)
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
            INT h, offsetY;
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

VOID CctControl_NavButton_Skip::InternalDefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineObject(_skipScreenId);
#endif
}

VOID CctControl_NavButton_Skip::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("Center",       dtBoolean,  &_center);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Image",        dtPGraphic, &_bitmap,      "BITDEPTH(16);HINTWIDTH(1024);HINTHEIGHT(1024)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Proportional", dtBoolean,  &_proportional);
    F.DefineValue("Stretch",      dtBoolean,  &_stretch);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);

    F.DefineStaticLink("Skip screen",  &_skipScreenId);
#endif
}

VOID CctControl_NavButton_Skip::OnPenClick(INT X, INT Y)
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
    _takeGPS = FALSE;
    _saveNumber = 0;
    _gpsSkipTimeout = 0;
}

VOID CctControl_NavButton_Save::InternalDefineProperties(CfxFiler &F)
{
    F.DefineValue("TakeGPS",        dtBoolean, &_takeGPS);
    F.DefineValue("GpsSkipTimeout", dtInt32,   &_gpsSkipTimeout);
    F.DefineValue("SaveNumber",     dtInt32,   &_saveNumber);

    F.DefineXOBJECT("SaveScreenId", &_saveScreenId);
}

VOID CctControl_NavButton_Save::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, Invert);
        pCanvas->FillRect(pRect);
    }
    pCanvas->State.BrushColor = pCanvas->InvertColor(_borderColor, Invert);
    pCanvas->State.LineColor = pCanvas->InvertColor(_borderColor, Invert);

    UINT w = GetClientWidth() - 2;
    UINT h = GetClientHeight() - 2;

    UINT t = min(w, h) - 1;
    INT x = pRect->Left + (w - t) / 2 + 1;
    INT y = pRect->Top + (h - t) / 2 + 1;

    // Draw bottom tray
    UINT s = max(1, t / 10);
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
    UINT boxW    = t / 2;
    UINT boxH    = t / 3;
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
    UINT triSide = t / 2;
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

    /*UINT w = GetClientWidth() / 2 - 2;
    UINT h = GetClientHeight() / 2 - 2;
    h = min(h, w);
    w = max(4, h / 2);
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{h, 0}, {h, h*2}, {0, h}};

    INT x = pRect->Left + (GetClientWidth() - h - w) / 2 - 1 * _borderLineWidth;
    INT y = pRect->Top + GetClientHeight() / 2 - h;
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

VOID CctControl_NavButton_Save::InternalDefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineObject(_saveScreenId);
#endif
}

VOID CctControl_NavButton_Save::Setup(BOOL TakeGPS, INT GpsSkipTimeout, XGUID SaveScreenId)
{
    _takeGPS = TakeGPS;
    _saveScreenId = SaveScreenId;
    _gpsSkipTimeout = GpsSkipTimeout;
}

VOID CctControl_NavButton_Save::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("Center",       dtBoolean,  &_center);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Image",        dtPGraphic, &_bitmap,      "BITDEPTH(16);HINTWIDTH(1024);HINTHEIGHT(1024)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Proportional", dtBoolean,  &_proportional);
    F.DefineValue("Save type",    dtInt32,    &_saveNumber,  "LIST(Save1 Save2)");
    F.DefineValue("Stretch",      dtBoolean,  &_stretch);
    F.DefineValue("Take GPS",     dtBoolean,  &_takeGPS);
    F.DefineValue("Take GPS skip timeout", dtInt32, &_gpsSkipTimeout, "MIN(0)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);
    F.DefineStaticLink("Save screen", &_saveScreenId);
#endif
}

VOID CctControl_NavButton_Save::OnPenClick(INT X, INT Y)
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

    if (_takeGPS && (session->GetCurrentSighting()->GetGPS()->Quality == fqNone))
    {
        GPS_READER_CONTEXT context;
        context.SaveTarget = saveScreenId;
        context.SkipTimeout = (_gpsSkipTimeout != 0) ? _gpsSkipTimeout :
                                                       session->GetResourceHeader()->GpsSkipTimeout;

        session->ShowDialog(&GUID_DIALOG_GPSREADER, this, &context, sizeof(context));
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

VOID CctControl_NavButton_Options::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, Invert);
        pCanvas->FillRect(pRect);
    }
    pCanvas->State.BrushColor = pCanvas->InvertColor(_borderColor, Invert);

    UINT w = GetClientWidth() / 2 - 2;
    UINT h = GetClientHeight() / 2 - 2;
    h = min(h, w);
    INT x = pRect->Left + GetClientWidth() / 2;
    INT y = pRect->Top + GetClientHeight() / 2;

    pCanvas->FillEllipse(x, y, h, h);
}

VOID CctControl_NavButton_Options::OnPenClick(INT X, INT Y)
{
    GetSession(this)->ShowDialog(&GUID_DIALOG_SIGHTINGEDITOR, this, NULL, 0);
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

VOID CctControl_NavButton_GPS::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent)
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
        pCanvas->State.LineWidth = (UINT)_borderLineWidth;
        if (IsLive())
        {
            GetSession(this)->GetWaypointManager()->DrawStatusIcon(pCanvas, pRect, font, _color, _borderColor, Invert);
        }
        else
        {
            DrawWaypointIcon(pCanvas, pRect, font, 0, _color, _borderColor, FALSE, FALSE, Invert);
        }            
        resource->ReleaseFont(font);
    }
}

VOID CctControl_NavButton_GPS::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("Center",       dtBoolean,  &_center);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Image",        dtPGraphic, &_bitmap,      "BITDEPTH(16);HINTWIDTH(1024);HINTHEIGHT(1024)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Proportional", dtBoolean,  &_proportional);
    F.DefineValue("Stretch",      dtBoolean,  &_stretch);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_NavButton_GPS::OnPenClick(INT X, INT Y)
{
    GetSession(this)->ShowDialog(&GUID_DIALOG_GPSVIEWER, this, NULL, 0);
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

VOID CctControl_NavButton_AcceptEdit::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent)
{
    FXRECT rect = *pRect;
    InflateRect(&rect, -(INT)_borderLineWidth * 2, -(INT)_borderLineWidth * 2);
    pCanvas->DrawCheck(&rect, _color, _borderColor, Invert, Transparent);
}

VOID CctControl_NavButton_AcceptEdit::OnPenClick(INT X, INT Y)
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

VOID CctControl_NavButton_CancelEdit::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent)
{
    FXRECT rect = *pRect;
    InflateRect(&rect, -(INT)_borderLineWidth * 2, -(INT)_borderLineWidth * 2);
    pCanvas->DrawCross(&rect, _color, _borderColor, 2, Invert, Transparent);
}

VOID CctControl_NavButton_CancelEdit::OnPenClick(INT X, INT Y)
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

VOID CctControl_NavButton_Jump::InternalDefineProperties(CfxFiler &F)
{
    F.DefineXOBJECT("BaseScreenId", &_baseScreenId);
    F.DefineXOBJECT("JumpScreenId", &_jumpScreenId);
}

VOID CctControl_NavButton_Jump::InternalPaint(CfxCanvas *pCanvas, FXRECT *pRect, BOOL Invert, BOOL Transparent)
{
    if (!Transparent)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color, Invert);
        pCanvas->FillRect(pRect);
    }

    FXRECT rect = *pRect;
    InflateRect(&rect, -(INT)_borderLineWidth * 2, -(INT)_borderLineWidth * 2);

    pCanvas->State.BrushColor = pCanvas->InvertColor(_borderColor, Invert);

    INT w = rect.Right - rect.Left + 1;
    INT h = rect.Bottom - rect.Top + 1;
    INT nh = min(h, w);

    INT x = rect.Left + (w - nh) / 2;
    INT y = rect.Top + (h - nh) / 2;
    h = nh-1;

    FXPOINTLIST polygon;
    FXPOINT triangle1[] = {{h/2+1, 0}, {h, 0}, {h, h/2}};
    FILLPOLYGON(pCanvas, triangle1, x, y);

    INT thickness = _borderLineWidth * 2;
    FXPOINT triangle2[] = {{h-thickness, 0}, {0, h-thickness}, {thickness, h}, {h, thickness}};
    FILLPOLYGON(pCanvas, triangle2, x, y);
}

VOID CctControl_NavButton_Jump::InternalDefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineObject(_baseScreenId);
    F.DefineObject(_jumpScreenId);
#endif
}

VOID CctControl_NavButton_Jump::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("Center",       dtBoolean,  &_center);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Image",        dtPGraphic, &_bitmap,      "BITDEPTH(16);HINTWIDTH(1024);HINTHEIGHT(1024)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Proportional", dtBoolean,  &_proportional);
    F.DefineValue("Stretch",      dtBoolean,  &_stretch);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);

    F.DefineStaticLink("Base screen", &_baseScreenId);
    F.DefineStaticLink("Jump screen", &_jumpScreenId);
#endif
}

VOID CctControl_NavButton_Jump::OnPenClick(INT X, INT Y)
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

    _dock = dkBottom;

    _buttonWidth = 28;
    _height = 30;
    _buttonBorderStyle = bsNone;
    _buttonBorderLineWidth = 1;
    _buttonBorderWidth = 1;
    _takeGPS = TRUE;
    _gpsSkipTimeout = 0;
    InitFont(_timerFont, GPS_WAYPOINT_FONT);

    _showHome = FALSE;
    _showOptions = TRUE;
    _showGPS = TRUE;
    _showSave1 = _showSave2 = FALSE;
    _showBack = _showNext = TRUE;
    _showSkip = FALSE;

    InitFont(_skipFont, F_FONT_DEFAULT_M);

    // Create edit buttons
    _acceptEditButton = new CctControl_NavButton_AcceptEdit(pOwner, this);
    _acceptEditButton->SetComposite(TRUE);
    _cancelEditButton = new CctControl_NavButton_CancelEdit(pOwner, this);
    _cancelEditButton->SetComposite(TRUE);

    // Create dialog buttons
    _optionsButton = new CctControl_NavButton_Options(pOwner, this);
    _optionsButton->SetComposite(TRUE);
    _gpsButton = new CctControl_NavButton_GPS(pOwner, this);
    _gpsButton->SetComposite(TRUE);

    // Create navigation buttons
    _homeButton = new CctControl_NavButton_Home(pOwner, this);
    _homeButton->SetComposite(TRUE);
    _skipButton = new CctControl_NavButton_Skip(pOwner, this);
    _skipButton->SetComposite(TRUE);

    _save1Button = new CctControl_NavButton_Save(pOwner, this);
    _save1Button->SetComposite(TRUE);
    _save1Button->SetSaveNumber(0);
    _save2Button = new CctControl_NavButton_Save(pOwner, this);
    _save2Button->SetComposite(TRUE);
    _save2Button->SetSaveNumber(1);

    _backButton = new CctControl_NavButton_Back(pOwner, this);
    _backButton->SetComposite(TRUE);

    _nextButton = new CctControl_NavButton_Next(pOwner, this);
    _nextButton->SetComposite(TRUE);

    // Update
    UpdateButtons();
}

CctControl_Navigate::~CctControl_Navigate()
{
    // Create edit buttons
    _acceptEditButton = NULL;
    _cancelEditButton = NULL;
    _optionsButton = NULL;
    _gpsButton = NULL;
    _homeButton = NULL;

    _skipButton = NULL;
    _save1Button = NULL;
    _save2Button = NULL;

    _backButton = NULL;
    _nextButton = NULL;
}

VOID CctControl_Navigate::UpdateButtons()
{
    BOOL editing = IsSessionEditing(this);

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

VOID CctControl_Navigate::DefineProperties(CfxFiler &F)
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

        #ifdef CLIENT_DLL
            StringReplace(&_lockProperties, "Show Major Target", "Show save 1");
            StringReplace(&_lockProperties, "Major Target", "Save 1 target");
            StringReplace(&_lockProperties, "Show Minor Target", "Show save 2");
            StringReplace(&_lockProperties, "Minor Target", "Save 2 target");

            StringReplace(&_lockProperties, "Show Save 1", "Show save 1");
            StringReplace(&_lockProperties, "Save 1 Target", "Save 1 target");
            StringReplace(&_lockProperties, "Show Save 2", "Show save 2");
            StringReplace(&_lockProperties, "Save 2 Target", "Save 2 target");
            StringReplace(&_lockProperties, "Save 2 Target", "Save 2 target");

            if (_lockProperties && strstr(_lockProperties, "Skip screen") == NULL)
            {
                StringReplace(&_lockProperties, "Save 2 target", "Save 2 target;Skip screen");
            }
        #endif
    }
}

VOID CctControl_Navigate::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color",  dtColor,   &_borderColor, "HINT(\"The color of the border\")");
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style",  dtInt8,    &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width",  dtInt32,   &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button border", dtInt32,   &_buttonBorderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Button border line width", dtInt32, &_buttonBorderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Button border width", dtInt32, &_buttonBorderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button width",  dtInt32,   &_buttonWidth, "MIN(20) MAX(80)");
    F.DefineValue("Color",         dtColor,   &_color);
    F.DefineValue("Dock",          dtByte,    &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Left",          dtInt32,   &_left);
    F.DefineValue("Height",        dtInt32,   &_height);
    F.DefineValue("Show back",     dtBoolean, &_showBack);
    F.DefineValue("Show GPS",      dtBoolean, &_showGPS);
    F.DefineValue("Show home",     dtBoolean, &_showHome);
    F.DefineValue("Show next",     dtBoolean, &_showNext);
    F.DefineValue("Show options",  dtBoolean, &_showOptions);
    F.DefineValue("Show save 1",   dtBoolean, &_showSave1);
    F.DefineValue("Show save 2",   dtBoolean, &_showSave2);
    F.DefineValue("Show skip",     dtBoolean, &_showSkip);
    F.DefineValue("Skip font",     dtFont,    &_skipFont);
    F.DefineValue("Take GPS reading", dtBoolean, &_takeGPS);
    F.DefineValue("Take GPS skip timeout", dtInt32, &_gpsSkipTimeout, "MIN(0)");
    F.DefineValue("Timer font",    dtFont,    &_timerFont);
    F.DefineValue("Top",           dtInt32,   &_top);
    F.DefineValue("Transparent",   dtBoolean, &_transparent);
    F.DefineValue("Width",         dtInt32,   &_width);

    F.DefineStaticLink("Save 1 target", &_save1ScreenId);
    F.DefineStaticLink("Save 2 target", &_save2ScreenId);
    F.DefineStaticLink("Next screen",  &_nextScreenId);
    F.DefineStaticLink("Skip screen",  &_skipScreenId);
    F.DefineStaticLink("Home screen",  &_homeScreenId);
#endif
} 

VOID CctControl_Navigate::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineObject(_save1ScreenId);
    F.DefineObject(_save2ScreenId);
    F.DefineObject(_nextScreenId);
    F.DefineObject(_skipScreenId);
    F.DefineObject(_homeScreenId);
    F.DefineFont(_timerFont);
    F.DefineFont(_skipFont);
#endif
}

VOID CctControl_Navigate::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    UINT th = GetClientHeight();
    UINT tx = _borderWidth;

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

VOID CctControl_Navigate::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _buttonWidth = (_buttonWidth * ScaleX) / 100;
    _buttonWidth = max(16, _buttonWidth);
    _buttonWidth = max(ScaleHitSize, _buttonWidth);

    UINT t = _buttonBorderWidth;
    _buttonBorderWidth = (_buttonBorderWidth * ScaleBorder) / 100;
    if ((_buttonBorderWidth == 0) && (t > 0))
    {
        _buttonBorderWidth = 1;
    }

    _buttonBorderLineWidth = max(1, (_buttonBorderLineWidth * ScaleBorder) / 100);

    _height = max((INT)ScaleHitSize, _height);

    UpdateButtons();
}
