#include "fxPanel.h"
#include "fxUtils.h"

CfxControl *Create_Control_Panel(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Panel(pOwner, pParent);
}

CfxControl_Panel::CfxControl_Panel(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_PANEL, true);
    InitBorder(ControlBorderStyle::Single, 1);
    
    _caption = nullptr;
    _autoHeight = false;
    _minHeight = 0;
    _alignment = TEXT_ALIGN_HCENTER;

    _useScreenName = false;

    _onClick.Object = nullptr;
    _onPaint.Object = nullptr;
    _onResize.Object = nullptr;

    _textColor = 0;
}

CfxControl_Panel::~CfxControl_Panel()
{
    MEM_FREE(_caption);
	_caption = nullptr;
}

void CfxControl_Panel::UpdateHeight()
{
    if (!_autoHeight) return;
    if ((_dock != ControlDock::Top) && (_dock != ControlDock::Bottom) && (_dock != ControlDock::None)) return;
    if (!_useScreenName && ((_caption == nullptr) || (_caption[0] == '\0'))) return;

    CfxResource *resource = GetResource();
    if (!resource) return;
    
    ResizeHeight(max(_minHeight, GetDefaultFontHeight() + (int)_borderWidth * 2 + _borderLineWidth * 2 + 4));
}

void CfxControl_Panel::SetCaption(const char *pCaption)
{
    MEM_FREE(_caption);
    _caption = nullptr;
    _caption = AllocString(pCaption);
    UpdateHeight();
}

void CfxControl_Panel::SetTextColor(COLOR Color)
{
    _textColor = Color;
}

void CfxControl_Panel::SetOnClick(CfxPersistent *pObject, NotifyMethod OnClick)
{
    _onClick.Object = pObject;
    _onClick.Method = OnClick;
}

void CfxControl_Panel::SetOnPaint(CfxPersistent *pObject, NotifyMethod OnPaint)
{
    _onPaint.Object = pObject;
    _onPaint.Method = OnPaint;
}

void CfxControl_Panel::SetOnResize(CfxPersistent *pObject, NotifyMethod OnResize)
{
    _onResize.Object = pObject;
    _onResize.Method = OnResize;
}

void CfxControl_Panel::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_Panel::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    /*if ((_dock == 1) && (_height == 32)) {
        _color = RGB(241, 227, 199);
        _textColor = RGB(74, 37, 0);
    }

    if ((_borderStyle > 0) && (_borderColor == 0)) {
        _borderColor = RGB(241, 227, 199);
    }*/

    F.DefineValue("Alignment",    dtInt32,    &_alignment, F_TWO);
    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
	F.DefineValue("Caption",      dtPText,    &_caption, F_NULL);
    F.DefineValue("MinHeight",    dtInt32,    &_minHeight, F_ZERO);
    F.DefineValue("UseScreenName", dtBoolean, &_useScreenName, F_FALSE);
    F.DefineValue("AutoHeight",    dtBoolean, &_autoHeight, F_FALSE);

    /*#ifdef CLIENT_DLL
    if (F.IsReader())
    {
        if (_stricmp(_font, "MS Sans Serif,12,B") == 0) 
        {
            strcpy(_font, "Arial,12,B");
        }
    }
    #endif*/

    if (F.IsReader())
    {
        UpdateHeight();
    }
}

void CfxControl_Panel::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CfxControl_Panel::OnPenClick(int X, int Y)
{
    if (_onClick.Object)
    {
        ExecuteEvent(&_onClick, this);
    }
}

void CfxControl_Panel::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
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

    CfxResource *resource = GetResource(); 

    char *caption = GetVisibleCaption();
    
    if (caption && (strlen(caption) > 0))
    {
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, pRect, _alignment | TEXT_ALIGN_VCENTER, caption);
            resource->ReleaseFont(font);
        }
    }
}

void CfxControl_Panel::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    if (_height == 1)
    {
        _height = max(1, ScaleBorder / 100);
        _color = (_borderStyle == ControlBorderStyle::None) ? _color : _borderColor;
    }
    else if (_width == 1)
    {
        _width = max(1, ScaleBorder / 100);
        _color = (_borderStyle == ControlBorderStyle::None) ? _color : _borderColor;
    }
    else 
    {
        CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
        _height = max((int)_minHeight, _height);
    }

    UpdateHeight();
}

void CfxControl_Panel::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    if (_onResize.Object)
    {
        ExecuteEvent(&_onResize, this);
    }
}

//*************************************************************************************************
