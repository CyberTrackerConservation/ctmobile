#include "fxScrollBox.h"

#include "fxApplication.h"
#include "fxUtils.h"

CfxControl *Create_Control_ScrollBox(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_ScrollBox(pOwner, pParent);
}

//*************************************************************************************************
// CfxControl_ScrollBox

CfxControl_ScrollBox::CfxControl_ScrollBox(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SCROLLBOX, true);
    InitBorder(_borderStyle, 0);

    _scrollBarWidth = 14;

    _container = new CfxControl_Panel(pOwner, this);
    _container->SetComposite(true);
    _container->SetBorderStyle(ControlBorderStyle::None);
    _container->SetBorderWidth(0);

    _scrollBar = new CfxControl_ScrollBarV2(GetOwner(), this);
    _scrollBar->SetComposite(true);
    _scrollBar->SetOnChange(this, (NotifyMethod)&CfxControl_ScrollBox::OnScrollChange);

    _once = false;
}

CfxControl_ScrollBox::~CfxControl_ScrollBox()
{
}

int CfxControl_ScrollBox::GetScrollBarWidth()
{
    return _scrollBar->GetVisible() ? _scrollBarWidth : 0;
}

void CfxControl_ScrollBox::OnScrollChange(CfxControl *pSender, void *pParams)
{
    _container->SetBounds(_borderWidth, _borderWidth - _scrollBar->GetValue(), _scrollBar->GetLeft(), GetClientHeight() + _scrollBar->GetValue());
}

void CfxControl_ScrollBox::UpdateScrollBar()
{
    int maxY = 0;
    int minY = 0;
    int value = _scrollBar->GetValue();

    _scrollBar->SetValue(0);

    for (int i=0; i<_container->GetControlCount(); i++)
    {
        CfxControl *control = _container->GetControl(i);
        minY = min(minY, control->GetTop());
        maxY = max(maxY, (int)(control->GetTop() + control->GetHeight()));
    }

    _scrollBar->SetVisible(maxY > (int)GetClientHeight());

    int sw = _scrollBar->GetVisible() ? _scrollBarWidth : 0;
    if (IsLive())
    {
        sw = (sw * GetEngine(this)->GetScaleScroller()) / 100;
    }

	_scrollBar->SetBounds(_width - _borderWidth - sw, _borderWidth, sw, GetClientHeight());
    _scrollBar->Setup(0, maxY, _container->GetClientHeight(), 1);
    _scrollBar->SetColor(_color);
    _scrollBar->SetBorderColor(_borderColor);

    _scrollBar->SetBounds(_width - _borderWidth - sw, _borderWidth, sw, GetClientHeight());
    _container->SetBounds(_borderWidth, _borderWidth - _scrollBar->GetValue(), _scrollBar->GetLeft(), GetClientHeight() + _scrollBar->GetValue());

    _scrollBar->SetValue(value);
}

void CfxControl_ScrollBox::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _scrollBar->Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _scrollBar->SetCanScale(false);
    _scrollBarWidth = ComputeScrollBarWidth(_scrollBarWidth, _borderLineWidth, ScaleX, ScaleHitSize);
}

void CfxControl_ScrollBox::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl_Panel::SetBounds(Left, Top, Width, Height);
    
    _scrollBar->SetBounds(_width - _borderWidth - GetScrollBarWidth(), _borderWidth, GetScrollBarWidth(), GetClientHeight());
    _container->SetBounds(_borderWidth, _borderWidth - _scrollBar->GetValue(), _scrollBar->GetLeft(), GetClientHeight() + _scrollBar->GetValue());
}

void CfxControl_ScrollBox::DefineProperties(CfxFiler &F)
{
    if (F.DefineBegin("Container"))
    {
        _container->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Scrollbar"))
    {
        _scrollBar->DefineProperties(F);
        F.DefineEnd();
    }

    F.DefineValue("Version",         dtInt32,   &_version, F_ZERO);
    F.DefineValue("LockProperties",  dtPText,   &_lockProperties, _lockPropertiesDefault);
    F.DefineValue("Id",              dtInt32,   &_id);
    F.DefineValue("BorderWidth",     dtInt32,   &_borderWidth, F_ZERO);
    F.DefineValue("BorderLineWidth", dtInt32,   &_borderLineWidth, F_ONE);
    F.DefineValue("BorderStyle",     dtInt8,    &_borderStyle, F_ONE);
    F.DefineValue("BorderColor",     dtColor,   &_borderColor, F_COLOR_BLACK);
    F.DefineValue("Color",           dtColor,   &_color, F_COLOR_WHITE);
    F.DefineValue("Composite",       dtBoolean, &_composite, F_FALSE);
    F.DefineValue("Visible",         dtBoolean, &_visible, F_TRUE);
    F.DefineValue("Transparent",     dtBoolean, &_transparent, F_TRUE);
    F.DefineValue("Align",           dtByte,    &_dock);
    F.DefineValue("Left",            dtInt32,   &_left);
    F.DefineValue("Top",             dtInt32,   &_top);
    F.DefineValue("Width",           dtInt32,   &_width);
    F.DefineValue("Height",          dtInt32,   &_height);

    F.DefineValue("ScrollBarWidth", dtInt32,   &_scrollBarWidth, "9");

	if (F.IsReader())
	{
        _once = false;
	}
}

void CfxControl_ScrollBox::DefineState(CfxFiler &F)
{
    _container->DefineState(F);
    _scrollBar->DefineState(F);
}

void CfxControl_ScrollBox::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CfxControl_ScrollBox::DefineResources(CfxFilerResource &F)
{
    _scrollBar->DefineResources(F);
    _container->DefineResources(F);
}

void CfxControl_ScrollBox::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_once)
    { 
        _once = true;
        UpdateScrollBar();
        GetEngine(this)->AlignControls(this);
    }

    CfxControl_Panel::OnPaint(pCanvas, pRect);
}