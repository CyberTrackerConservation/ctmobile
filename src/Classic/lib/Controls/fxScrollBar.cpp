#include "fxApplication.h"
#include "fxScrollBar.h"
#include "fxUtils.h"

CfxControl_ScrollBar::CfxControl_ScrollBar(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    _penDown = _penOver = false;

    _onChange.Object = nullptr;
    _pageSize = 10;
    _minValue = 0;
    _maxValue = 100;
    _value = 0;
}

void CfxControl_ScrollBar::ResetState()
{
    CfxControl::ResetState();
    _value = 0;
}

void CfxControl_ScrollBar::DefineState(CfxFiler &F)
{
    F.DefineValue("ScrollValue", dtInt32, &_value);
}

void CfxControl_ScrollBar::ExecuteChange()
{    
    if (_onChange.Object)
    {
        ExecuteEvent(&_onChange, this);
    }
}

void CfxControl_ScrollBar::SetOnChange(CfxControl *pControl, NotifyMethod OnChange)
{
    _onChange.Object = pControl;
    _onChange.Method = OnChange;
}

void CfxControl_ScrollBar::Setup(int MinValue, int MaxValue, int PageSize, int StepSize)
{
    _minValue = MinValue;
    _maxValue = MaxValue;
    _pageSize = PageSize;
    _stepSize = StepSize;

    _maxValue = max(_minValue, _maxValue); 
    _minValue = min(_maxValue, _minValue); 

    // Don't reset the value if it's good
    if ((_value < _minValue) || (_value > _maxValue - _pageSize + 1))
    {
        _value = _minValue; 
    }
}

void CfxControl_ScrollBar::SetValue(int Value)     
{ 
    if (_stepSize == 0) return;

    int orgValue = _value;
    _value = Value;
    _value = min(_maxValue - _pageSize, _value);
    _value = max(_minValue, _value);

    if (_value != orgValue)
    {
        ExecuteChange();
    }
}

void CfxControl_ScrollBar::StepUp()
{
    SetValue(_value - _stepSize);
}

void CfxControl_ScrollBar::StepDown()
{
    SetValue(_value + _stepSize);
}

void CfxControl_ScrollBar::PageUp()
{
    SetValue(_value - _pageSize);
}

void CfxControl_ScrollBar::PageDown()
{
    SetValue(_value + _pageSize);
}

void CfxControl_ScrollBar::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    FXRECT rect;

    pCanvas->State.BrushColor = _color;
    pCanvas->State.LineColor = _borderColor;

    GetThumbBounds(&rect);
    OffsetRect(&rect, pRect->Left, pRect->Top);
    pCanvas->State.BrushColor = _borderColor;
    pCanvas->Rectangle(&rect);

    pCanvas->State.BrushColor = _color;

    int l = rect.Left + 2 * (int)_oneSpace;
    int w = rect.Right - l + 1 - 2 * (int)_oneSpace;

    int t = (rect.Bottom - rect.Top) / 2 + rect.Top - _oneSpace;
    pCanvas->FillRect(l, t, w, (int)_oneSpace * 2);

    t -= 4 * (int)_oneSpace;
    pCanvas->FillRect(l, t, w, (int)_oneSpace * 2);

    t += 8 * (int)_oneSpace;
    pCanvas->FillRect(l, t, w, (int)_oneSpace * 2);
}

void CfxControl_ScrollBar::OnPenDown(int X, int Y)
{
    FXRECT rect;
    GetThumbBounds(&rect);

    FXPOINT point = {X, Y};
    if (!PtInRect(&rect, &point)) return;

    _penDown = _penOver = true;

    _downX = X;
    _downY = Y;
    _downValue = _value;

    Repaint();
}

void CfxControl_ScrollBar::OnPenUp(int X, int Y)
{
    if (!_penDown) return;
    _penDown = false;

    Repaint();
}

void CfxControl_ScrollBar::OnPenMove(int X, int Y)
{
    if (!_penDown) return;

    if (_penOver != ((X >= 0) && (X < _width) && (Y >= 0) && (Y < _height)))
    {
		_penOver = !_penOver;
        Repaint();
        return;
	}

    if (_maxValue == _minValue) return;

    MoveThumb1(_downX, _downY, _downValue, X, Y);

    Repaint();
}

void CfxControl_ScrollBar::OnPenClick(int X, int Y)
{
    FXRECT rect;
    GetEmptyBounds(&rect);

    FXPOINT point = {X, Y};
    if (!PtInRect(&rect, &point)) return;

    GetThumbBounds(&rect);
    if (PtInRect(&rect, &point)) return;

    MoveThumb2(X, Y);

    Repaint();
}

//*************************************************************************************************
// ScrollBarV

CfxControl *Create_Control_ScrollBarV(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_ScrollBarV(pOwner, pParent);
}

CfxControl_ScrollBarV::CfxControl_ScrollBarV(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_ScrollBar(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SCROLLBARV);
}

void CfxControl_ScrollBarV::GetEmptyBounds(FXRECT *pRect)
{
    pRect->Left   = 0;
    pRect->Top    = 0;
    pRect->Right  = (int)_width - 1;
    pRect->Bottom = (int)_height - 1;
}

void CfxControl_ScrollBarV::GetThumbBounds(FXRECT *pRect)
{
    if (_maxValue == _minValue)
    {
        GetEmptyBounds(pRect);
        return;
    }
    
    int range = _maxValue - _minValue;
    int thumbHeight = (_pageSize * _height) / range;
    int validValue = min(_value, _maxValue - _pageSize);
    
    pRect->Left   = 0; 
    pRect->Top    = (int)((_height * (validValue - _minValue)) / range);
    pRect->Right  = (int)_width - 1; 
    pRect->Bottom = pRect->Top + (int)thumbHeight;

    int diff = pRect->Bottom - pRect->Top + 1;
    if (diff < 20 * _oneSpace)
    {
        int yc = pRect->Top + diff / 2;
        pRect->Top = yc - 8 * _oneSpace;
        pRect->Bottom = yc + 8 * _oneSpace;

        if (pRect->Top < _oneSpace)
        {
            pRect->Top = 1 * _oneSpace;
            pRect->Bottom = 1 * _oneSpace + 16 * _oneSpace;
        }
        else if (pRect->Bottom > _height - 2 * _oneSpace)
        {
            pRect->Bottom = (int)_height - 2 * _oneSpace;
            pRect->Top = (int)_height - 2 * _oneSpace - 16 * _oneSpace;
        }
    }
}

void CfxControl_ScrollBarV::MoveThumb1(int DownX, int DownY, int DownValue, int X, int Y)
{
    int roundFactor = _height / 2;
    
    int range = _maxValue - _minValue; 
    int delta = ((Y - DownY) * range + roundFactor) / _height;
    int value = DownValue + delta;

    value = max(_minValue, value);
    value = min(_minValue + range - _pageSize, value);

    SetValue(value);
}

void CfxControl_ScrollBarV::MoveThumb2(int X, int Y)
{
    FXRECT rect;
    GetThumbBounds(&rect);

    if (Y <= rect.Top)
    {
        SetValue(_value - _stepSize);
    }
    else if (Y >= rect.Bottom) 
    {
        SetValue(_value + _stepSize);
    }
}

//*************************************************************************************************
// CfxControl_ScrollBarV2

CfxControl *Create_Control_ScrollBarV2(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_ScrollBarV2(pOwner, pParent);
}

CfxControl_ScrollBarV2::CfxControl_ScrollBarV2(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SCROLLBARV2);
    InitBorder(ControlBorderStyle::Single, 0);

    _buttonHeight = 32;

    _backButton = new CfxControl_Button(pOwner, this);
    _backButton->SetComposite(true);
    _backButton->SetBorderStyle(ControlBorderStyle::None);
    _backButton->SetBorderWidth(1);
    _backButton->SetOnClick(this, (NotifyMethod)&CfxControl_ScrollBarV2::OnBackClick);
    _backButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ScrollBarV2::OnBackPaint);

    _nextButton = new CfxControl_Button(pOwner, this);
    _nextButton->SetComposite(true);
    _nextButton->SetBorderStyle(ControlBorderStyle::None);
    _nextButton->SetBorderWidth(1);
    _nextButton->SetOnClick(this, (NotifyMethod)&CfxControl_ScrollBarV2::OnNextClick);
    _nextButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ScrollBarV2::OnNextPaint);

    _scrollBar = new CfxControl_ScrollBarV(pOwner, this);
    _scrollBar->SetComposite(true);
    _scrollBar->SetBorderStyle(ControlBorderStyle::Single);
    _scrollBar->SetBorderWidth(0);
}

void CfxControl_ScrollBarV2::FixColors()
{
    _scrollBar->SetColor(_color);
    _scrollBar->SetBorderColor(_borderColor);
}

void CfxControl_ScrollBarV2::OnBackClick(CfxControl *pSender, void *pParams)
{
    _scrollBar->StepUp();
    Repaint();
}

void CfxControl_ScrollBarV2::OnNextClick(CfxControl *pSender, void *pParams)
{
    _scrollBar->StepDown();
    Repaint();
}

void CfxControl_ScrollBarV2::OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawUpArrow(pParams->Rect, _color, _borderColor, _backButton->GetDown()); 
}

void CfxControl_ScrollBarV2::OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawDownArrow(pParams->Rect, _color, _borderColor, _nextButton->GetDown()); 
}

void CfxControl_ScrollBarV2::ResetState()
{
    CfxControl::ResetState();
    _scrollBar->ResetState();
}

void CfxControl_ScrollBarV2::DefineState(CfxFiler &F)
{
    _scrollBar->DefineState(F);
}

void CfxControl_ScrollBarV2::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    
    int extraScale = IsLive() ? GetEngine(this)->GetScaleScroller() : 100;

    _width = (_width * extraScale) / 100;
    _width = ComputeScrollBarWidth(_width, 0, 100, ScaleHitSize);
    _buttonHeight = (32 * extraScale) / 100;
    _buttonHeight = (_buttonHeight * ScaleY) / 100;
    _buttonHeight = max(16, _buttonHeight);
    _buttonHeight = max(ScaleHitSize, _buttonHeight);
}

void CfxControl_ScrollBarV2::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    int w = GetClientWidth();
    int h = GetClientHeight();

    _backButton->SetBounds(_borderWidth, _borderWidth, w, _buttonHeight);
    _nextButton->SetBounds(_borderWidth, _borderWidth + h - _buttonHeight, w, _buttonHeight);
    _scrollBar->SetBounds(_borderWidth, _borderWidth + _buttonHeight - _borderLineWidth, w, h - _buttonHeight * 2 + _borderLineWidth * 2);
    _scrollBar->SetBorderLineWidth((int)_borderLineWidth);
}

void CfxControl_ScrollBarV2::OnKeyDown(BYTE KeyCode, bool *pHandled)
{
    if (!GetIsVisible() || (_width == 0) || (_height == 0)) return;

	switch (KeyCode)
    {
    case KEY_UP:   PageUp(); *pHandled = true; Repaint(); break;
    case KEY_DOWN: PageDown(); *pHandled = true; Repaint(); break;
    }
}

int ComputeScrollBarWidth(int Width, int BorderLineWidth, int ScaleX, int ScaleHitSize) 
{ 
    return max((ScaleHitSize * 2) / 3, 
               max(7 + BorderLineWidth * 2, (Width * ScaleX) / 100));
}
