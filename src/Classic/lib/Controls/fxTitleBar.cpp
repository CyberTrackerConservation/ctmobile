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
    InitBorder(ControlBorderStyle::None, 0);

    _showExit = _showTime = _showBattery = true;
    _showMenu = false;
    _showOkay = false;

    _penDownMenu = _penDownOkay = _penDownExit = _penOverMenu = _penOverOkay = _penOverExit = false;
    memset(&_menuBounds, 0, sizeof(FXRECT));
    memset(&_okayBounds, 0, sizeof(FXRECT));
    memset(&_exitBounds, 0, sizeof(FXRECT));

    _batteryWidth = 30;
    _batteryPixel = 1;
    _dock = ControlDock::Top;
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

void CfxControl_TitleBar::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _batteryWidth = (int)max(20, ((int)_batteryWidth * ScaleX) / 100);
    _batteryPixel = (int)max(1, ScaleY / 100);
    _minHeight = max(ScaleHitSize, _minHeight);
}

void CfxControl_TitleBar::SetBounds(int Left, int Top, int Width, int Height)
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

void CfxControl_TitleBar::DrawMenu(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect)
{
    bool invert = _penDownMenu && _penOverMenu;
    int spacer = _borderLineWidth * 2;

    FXRECT rect = *pRect;
    InflateRect(&rect, -spacer, -spacer);

    int ox = (rect.Right - rect.Left) + 1;
    int oy = (rect.Bottom - rect.Top) + 1;

    int w = ox / 2 - 2;
    int h = oy / 2 - 2;
    h = min(h, w);
    int x = pRect->Left + oy / 2 + spacer; 
    int y = pRect->Top + oy / 2 + spacer;

    _menuBounds.Left = _menuBounds.Right = x;
    _menuBounds.Top = _menuBounds.Bottom = y;
    InflateRect(&_menuBounds, h + spacer, h + spacer);

    pCanvas->State.BrushColor = pCanvas->InvertColor(_color, invert);
    pCanvas->FillRect(&_menuBounds);
    pCanvas->State.BrushColor = pCanvas->InvertColor(_textColor, invert);
    pCanvas->FillEllipse(x, y, h, h);

    pRect->Left = _menuBounds.Right + spacer;
}

void CfxControl_TitleBar::DrawOkay(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect)
{
    bool invert = _penDownOkay && _penOverOkay;
    int spacer = _borderLineWidth * 2;

    FXRECT rect = *pRect;
    InflateRect(&rect, -spacer, -spacer);

    int h = rect.Bottom - rect.Top + 1; 
    rect.Left = rect.Right - h + 1;
    _okayBounds = rect;
    pRect->Right = rect.Left - spacer;

    pCanvas->State.BrushColor = pCanvas->InvertColor(_textColor, invert);

    if ((pFont->Format != 1) && (h > 17))
    {
        int h2 = (h - 1) / 2;
        int xc = rect.Left + h2;
        int yc = rect.Top + h2;
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

void CfxControl_TitleBar::DrawExit(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect)
{
    bool invert = _penDownExit && _penOverExit;
    int spacer = _borderLineWidth * 2;

    FXRECT rect = *pRect;
    InflateRect(&rect, -spacer, -spacer);

    int h = rect.Bottom - rect.Top + 1;
    rect.Left = rect.Right - h + 1;
    _exitBounds = rect;
    pRect->Right = rect.Left - spacer;

    pCanvas->State.BrushColor = pCanvas->InvertColor(_textColor, invert);
    pCanvas->State.LineColor  = pCanvas->InvertColor(_color, invert);

    pCanvas->FillRect(&rect);
    InflateRect(&rect, -spacer, -spacer);

    int thickness;
    if ((pFont->Format == 0) && (h > 17))
    {
        thickness = _borderLineWidth;
    }
    else
    {
        thickness = 1;
    }

    pCanvas->DrawCross(&rect, _textColor, _color, max(1, thickness), invert, true);
}

void CfxControl_TitleBar::DrawTime(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect)
{
    FXDATETIME dateTime;
    GetHost(this)->GetDateTime(&dateTime);

    pRect->Right -= 4;
    pRect->Right -= pCanvas->DrawTime(&dateTime, pRect, pFont, TEXT_ALIGN_VCENTER | TEXT_ALIGN_RIGHT, _color, _textColor, false, false);
}

void CfxControl_TitleBar::DrawBattery(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect)
{
    CfxHost *host = GetHost(this);
    int level = host->GetBatteryLevel();
    bool charging = host->GetBatteryState() == FXBATTERYSTATE::Charge;

    if (charging)
    {
        static int chargingLevel = 0;
        level = (chargingLevel * 100) / 5;
        chargingLevel = chargingLevel < 5 ? chargingLevel + 1 : 0;
        SetPaintTimer(500);
    }
    
    int spacer = _borderLineWidth * 2;

    FXRECT rect = *pRect;
    InflateRect(&rect, -spacer, -spacer);

    int h = rect.Bottom - rect.Top + 1;
    rect.Right -= 2 * spacer;
    rect.Left = rect.Right - h * 3;

    pCanvas->State.LineWidth = _borderLineWidth;

    pRect->Right -= 2 * spacer;
    pRect->Right -= pCanvas->DrawBattery(level, &rect, TEXT_ALIGN_RIGHT, _color, _textColor, false, _transparent);
}

void CfxControl_TitleBar::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (_onPaint.Object)
    {
        FXPAINTDATA paintData = {pCanvas, pRect, false};
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
    CfxHost *host = IsLive() ? GetHost(this) : nullptr;

    if (_showMenu)
    {
        DrawMenu(pCanvas, font, &rect);
    }

    if (_showOkay)
    {
        DrawOkay(pCanvas, font, &rect);
    }

    if (_showExit)
    {
        DrawExit(pCanvas, font, &rect);
    }

    if (_showTime)
    {
        DrawTime(pCanvas, font, &rect);
    }

    if (_showBattery)
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

void CfxControl_TitleBar::OnPenDown(int X, int Y)
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

void CfxControl_TitleBar::OnPenUp(int X, int Y)
{ 
    _penDownMenu = _penDownExit = _penDownOkay = false;

    Repaint();
}

void CfxControl_TitleBar::OnPenMove(int X, int Y)
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

void CfxControl_TitleBar::OnPenClick(int X, int Y)
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

void CfxControl_TitleBar::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_TitleBar::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("ShowBattery", dtBoolean, &_showBattery, F_TRUE);
    F.DefineValue("ShowExit",    dtBoolean, &_showExit, F_TRUE);
    F.DefineValue("ShowMenu",    dtBoolean, &_showMenu, F_FALSE);
    F.DefineValue("ShowOkay",    dtBoolean, &_showOkay, F_FALSE);
    F.DefineValue("ShowTime",    dtBoolean, &_showTime, F_TRUE);

    F.DefineXFONT("BigFont",     &_bigFont, F_FONT_DEFAULT_LB);
}

void CfxControl_TitleBar::DefinePropertiesUI(CfxFilerUI &F) 
{
}
