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
#include "fxTitleBar.h"
#include "fxUtils.h"

CfxControl *Create_Control_TitleBar(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_TitleBar(pOwner, pParent);
}

CfxControl_TitleBar::CfxControl_TitleBar(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_TITLEBAR);
    InitLockProperties("Caption;Show battery;Show exit;Show options;Show time");
    InitBorder(bsNone, 0);

    _showExit = _showTime = _showBattery = TRUE;
    _showMenu = FALSE;
    _showOkay = FALSE;

    _penDownMenu = _penDownOkay = _penDownExit = _penOverMenu = _penOverOkay = _penOverExit = FALSE;
    memset(&_menuBounds, 0, sizeof(FXRECT));
    memset(&_okayBounds, 0, sizeof(FXRECT));
    memset(&_exitBounds, 0, sizeof(FXRECT));

    _batteryWidth = 30;
    _batteryPixel = 1;
    _dock = dkTop;
    _minHeight = 24;
    _height = 24;

    //_textColor = 0xFFFFFF;
    //_color = 0;
    
    InitFont(F_FONT_DEFAULT_SB);
    InitFont(_bigFont, F_FONT_DEFAULT_LB);
}

CfxControl_TitleBar::~CfxControl_TitleBar()
{
}

FXFONTRESOURCE *CfxControl_TitleBar::GetFont(CfxResource *pResource)
{
    if (_bigFont && IsLive() && ((GetEngine(this)->GetScaleTitle() > 100) || (_minHeight > 24)))
    {
        return pResource->GetFont(this, _bigFont);
    }
    else
    {
        return pResource->GetFont(this, _font);
    }
}

VOID CfxControl_TitleBar::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _batteryWidth = (UINT)max(20, ((UINT)_batteryWidth * ScaleX) / 100);
    _batteryPixel = (UINT)max(1, ScaleY / 100);
    _minHeight = max(ScaleHitSize, _minHeight);
}

VOID CfxControl_TitleBar::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    if (IsLive())
    {
        CfxResource *resource = GetResource();
        FXFONTRESOURCE *font = GetFont(resource);
        if (font)
        {
            Height = max(_minHeight, (GetCanvas(this)->FontHeight(font) * 150) / 100);
            resource->ReleaseFont(font);
        }
    }

    Height = (Height & ~1) + 1;
    CfxControl_Panel::SetBounds(Left, Top, Width, (Height & ~1) + 1);
}

VOID CfxControl_TitleBar::DrawMenu(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect)
{
    BOOL invert = _penDownMenu && _penOverMenu;
    INT spacer = _borderLineWidth * 2;

    FXRECT rect = *pRect;
    InflateRect(&rect, -spacer, -spacer);

    UINT ox = (rect.Right - rect.Left) + 1;
    UINT oy = (rect.Bottom - rect.Top) + 1;

    UINT w = ox / 2 - 2;
    UINT h = oy / 2 - 2;
    h = min(h, w);
    INT x = pRect->Left + oy / 2 + spacer; 
    INT y = pRect->Top + oy / 2 + spacer;

    _menuBounds.Left = _menuBounds.Right = x;
    _menuBounds.Top = _menuBounds.Bottom = y;
    InflateRect(&_menuBounds, h + spacer, h + spacer);

    pCanvas->State.BrushColor = pCanvas->InvertColor(_color, invert);
    pCanvas->FillRect(&_menuBounds);
    pCanvas->State.BrushColor = pCanvas->InvertColor(_textColor, invert);
    pCanvas->FillEllipse(x, y, h, h);

    pRect->Left = _menuBounds.Right + spacer;
}

VOID CfxControl_TitleBar::DrawOkay(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect)
{
    BOOL invert = _penDownOkay && _penOverOkay;
    INT spacer = _borderLineWidth * 2;

    FXRECT rect = *pRect;
    InflateRect(&rect, -spacer, -spacer);

    UINT h = rect.Bottom - rect.Top + 1; 
    rect.Left = rect.Right - h + 1;
    _okayBounds = rect;
    pRect->Right = rect.Left - spacer;

    pCanvas->State.BrushColor = pCanvas->InvertColor(_textColor, invert);

    if ((pFont->Format != 1) && (h > 17))
    {
        UINT h2 = (h - 1) / 2;
        INT xc = rect.Left + h2;
        INT yc = rect.Top + h2;
        pCanvas->FillEllipse(xc, yc, h2, h2);
        rect.Right = xc + h2;
        InflateRect(&rect, -spacer, -spacer);
    }
    else
    {
        pCanvas->FillRect(&rect);
    }

    pCanvas->State.TextColor = pCanvas->InvertColor(_color, invert);

    pCanvas->DrawTextRect(pFont, &rect, TEXT_ALIGN_VCENTER | TEXT_ALIGN_HCENTER, "ok");
}

VOID CfxControl_TitleBar::DrawExit(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect)
{
    BOOL invert = _penDownExit && _penOverExit;
    INT spacer = _borderLineWidth * 2;

    FXRECT rect = *pRect;
    InflateRect(&rect, -spacer, -spacer);

    UINT h = rect.Bottom - rect.Top + 1;
    rect.Left = rect.Right - h + 1;
    _exitBounds = rect;
    pRect->Right = rect.Left - spacer;

    pCanvas->State.BrushColor = pCanvas->InvertColor(_textColor, invert);
    pCanvas->State.LineColor  = pCanvas->InvertColor(_color, invert);

    pCanvas->FillRect(&rect);
    InflateRect(&rect, -spacer, -spacer);

    UINT thickness;
    if ((pFont->Format == 0) && (h > 17))
    {
        thickness = _borderLineWidth;
    }
    else
    {
        thickness = 1;
    }

    pCanvas->DrawCross(&rect, _textColor, _color, max(1, thickness), invert, TRUE);
}

VOID CfxControl_TitleBar::DrawTime(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect)
{
    FXDATETIME dateTime;
    GetHost(this)->GetDateTime(&dateTime);

    pRect->Right -= 4;
    pRect->Right -= pCanvas->DrawTime(&dateTime, pRect, pFont, TEXT_ALIGN_VCENTER | TEXT_ALIGN_RIGHT, _color, _textColor, FALSE, FALSE);
}

VOID CfxControl_TitleBar::DrawBattery(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect)
{
    CfxHost *host = GetHost(this);
    UINT level = host->GetBatteryLevel();
    BOOL charging = host->GetBatteryState() == batCharge;

    if (charging)
    {
        static UINT chargingLevel = 0;
        level = (chargingLevel * 100) / 5;
        chargingLevel = chargingLevel < 5 ? chargingLevel + 1 : 0;
        SetPaintTimer(500);
    }
    
    INT spacer = _borderLineWidth * 2;

    FXRECT rect = *pRect;
    InflateRect(&rect, -spacer, -spacer);

    UINT h = rect.Bottom - rect.Top + 1;
    rect.Right -= 2 * spacer;
    rect.Left = rect.Right - h * 3;

    pCanvas->State.LineWidth = _borderLineWidth;

    pRect->Right -= 2 * spacer;
    pRect->Right -= pCanvas->DrawBattery(level, &rect, TEXT_ALIGN_RIGHT, _color, _textColor, FALSE, _transparent);
}

VOID CfxControl_TitleBar::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (_onPaint.Object)
    {
        FXPAINTDATA paintData = {pCanvas, pRect, FALSE};
        ExecuteEvent(&_onPaint, this, &paintData);
        if (!paintData.DefaultPaint) return;
    }

    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    FXRECT rect = *pRect;
    CfxResource *resource = GetResource(); 
    FXFONTRESOURCE *font = GetFont(resource);
    CfxHost *host = IsLive() ? GetHost(this) : NULL;

    BOOL hasTitleClose = host ? host->HasExternalCloseButton() : FALSE; 
    BOOL hasTitleData = host ? host->HasExternalStateData() : FALSE; 
        
    if (_showMenu)
    {
        DrawMenu(pCanvas, font, &rect);
    }

    if (_showOkay && !hasTitleClose)
    {
        DrawOkay(pCanvas, font, &rect);
    }

    if (_showExit && !hasTitleClose)
    {
        DrawExit(pCanvas, font, &rect);
    }

    if (_showTime && !hasTitleData)
    {
        DrawTime(pCanvas, font, &rect);
    }

    if (_showBattery && !hasTitleData)
    {
        DrawBattery(pCanvas, font, &rect);
    }

    if (font && _caption && (strlen(_caption) > 0))
    {
        rect.Left += 4;
        rect.Right -= 4;
        pCanvas->State.TextColor = _textColor;
        pCanvas->DrawTextRect(font, &rect, TEXT_ALIGN_VCENTER, _caption);
    }

    if (font)
    {
        resource->ReleaseFont(font);
    }

    // Turn off our timer if we are not showing the time or battery
    if (_showTime || _showBattery)
    {
        SetPaintTimer(30000);
    }
}

VOID CfxControl_TitleBar::OnPenDown(INT X, INT Y)
{
    FXPOINT pt;
    FXRECT r;
    GetAbsoluteBounds(&r);
    pt.X = X + r.Left;
    pt.Y = Y + r.Top;

    _penOverMenu = _showMenu && PtInRect(&_menuBounds, &pt);
    _penOverExit = _showExit && PtInRect(&_exitBounds, &pt);
    _penOverOkay = _showOkay && PtInRect(&_okayBounds, &pt);
    _penDownMenu = _penOverMenu;
    _penDownExit = _penOverExit;
    _penDownOkay = _penOverOkay;

    Repaint();
}

VOID CfxControl_TitleBar::OnPenUp(INT X, INT Y)
{ 
    _penDownMenu = _penDownExit = _penDownOkay = FALSE;

    Repaint();
}

VOID CfxControl_TitleBar::OnPenMove(INT X, INT Y)
{
    if (!_penDownExit && !_penDownOkay && !_penDownMenu) return;

    FXPOINT pt;
    FXRECT r;
    GetAbsoluteBounds(&r);
    pt.X = X + r.Left;
    pt.Y = Y + r.Top;

    _penOverMenu = _showMenu && PtInRect(&_menuBounds, &pt);;
    _penOverExit = _showExit && PtInRect(&_exitBounds, &pt);;
    _penOverOkay = _showOkay && PtInRect(&_okayBounds, &pt);;

    Repaint();
}

VOID CfxControl_TitleBar::OnPenClick(INT X, INT Y)
{
    if (!_penOverExit && !_penOverOkay && !_penOverMenu) return;

    if (_showMenu && _penOverMenu)
    {
        GetApplication(this)->ShowMenu();
    }
    else if (_showOkay && _penOverOkay)
    {
        GetEngine(this)->CloseDialog();
    }
    else if (_showExit && _penOverExit)
    {
        GetEngine(this)->Shutdown();
    }
}

VOID CfxControl_TitleBar::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Panel::DefineResources(F);
    F.DefineFont(_bigFont);
#endif
}

VOID CfxControl_TitleBar::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("ShowBattery", dtBoolean, &_showBattery, F_TRUE);
    F.DefineValue("ShowExit",    dtBoolean, &_showExit, F_TRUE);
    F.DefineValue("ShowMenu",    dtBoolean, &_showMenu, F_FALSE);
    F.DefineValue("ShowOkay",    dtBoolean, &_showOkay, F_FALSE);
    F.DefineValue("ShowTime",    dtBoolean, &_showTime, F_TRUE);

    F.DefineXFONT("BigFont",     &_bigFont, F_FONT_DEFAULT_LB);
}

VOID CfxControl_TitleBar::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Show battery", dtBoolean,  &_showBattery);
    F.DefineValue("Show exit",    dtBoolean,  &_showExit);
    F.DefineValue("Show options", dtBoolean,  &_showMenu);
    F.DefineValue("Show time",    dtBoolean,  &_showTime);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}
