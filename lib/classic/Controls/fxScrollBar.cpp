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
#include "fxScrollBar.h"
#include "fxUtils.h"

CfxControl_ScrollBar::CfxControl_ScrollBar(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    _penDown = _penOver = FALSE;

    _onChange.Object = NULL;
    _pageSize = 10;
    _minValue = 0;
    _maxValue = 100;
    _value = 0;
}

VOID CfxControl_ScrollBar::ResetState()
{
    CfxControl::ResetState();
    _value = 0;
}

VOID CfxControl_ScrollBar::DefineState(CfxFiler &F)
{
    F.DefineValue("ScrollValue", dtInt32, &_value);
}

VOID CfxControl_ScrollBar::ExecuteChange()
{    
    if (_onChange.Object)
    {
        ExecuteEvent(&_onChange, this);
    }
}

VOID CfxControl_ScrollBar::SetOnChange(CfxControl *pControl, NotifyMethod OnChange)
{
    _onChange.Object = pControl;
    _onChange.Method = OnChange;
}

VOID CfxControl_ScrollBar::Setup(INT MinValue, INT MaxValue, INT PageSize, INT StepSize)
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

VOID CfxControl_ScrollBar::SetValue(INT Value)     
{ 
    if (_stepSize == 0) return;

    INT orgValue = _value;
    _value = Value; 
    _value = ((_value + _stepSize - 1) / _stepSize) * _stepSize;
    _value = min(_maxValue - _pageSize, _value); 
    _value = max(_minValue, _value);

    if (_value != orgValue)
    {
        ExecuteChange();
    }
}

VOID CfxControl_ScrollBar::StepUp()
{
    SetValue(_value - _stepSize);
}

VOID CfxControl_ScrollBar::StepDown()
{
    SetValue(_value + _stepSize);
}

VOID CfxControl_ScrollBar::PageUp()
{
    SetValue(_value - _pageSize);
}

VOID CfxControl_ScrollBar::PageDown()
{
    SetValue(_value + _pageSize);
}

VOID CfxControl_ScrollBar::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    FXRECT rect;

    pCanvas->State.BrushColor = _color;
    pCanvas->State.LineColor = _borderColor;

    GetThumbBounds(&rect);
    OffsetRect(&rect, pRect->Left, pRect->Top);
    pCanvas->State.BrushColor = _borderColor;
    pCanvas->Rectangle(&rect);

    pCanvas->State.BrushColor = _color;

    UINT l = rect.Left + 2 * (UINT)_oneSpace;
    UINT w = rect.Right - l + 1 - 2 * (UINT)_oneSpace;

    UINT t = (rect.Bottom - rect.Top) / 2 + rect.Top - _oneSpace;
    pCanvas->FillRect(l, t, w, (UINT)_oneSpace * 2);

    t -= 4 * (UINT)_oneSpace;
    pCanvas->FillRect(l, t, w, (UINT)_oneSpace * 2);

    t += 8 * (UINT)_oneSpace;
    pCanvas->FillRect(l, t, w, (UINT)_oneSpace * 2);
}

VOID CfxControl_ScrollBar::OnPenDown(INT X, INT Y)
{
    FXRECT rect;
    GetThumbBounds(&rect);

    FXPOINT point = {X, Y};
    if (!PtInRect(&rect, &point)) return;

    _penDown = _penOver = TRUE;

    _downX = X;
    _downY = Y;
    _downValue = _value;

    Repaint();
}

VOID CfxControl_ScrollBar::OnPenUp(INT X, INT Y)
{
    if (!_penDown) return;
    _penDown = FALSE;

    Repaint();
}

VOID CfxControl_ScrollBar::OnPenMove(INT X, INT Y)
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

VOID CfxControl_ScrollBar::OnPenClick(INT X, INT Y)
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

VOID CfxControl_ScrollBarV::GetEmptyBounds(FXRECT *pRect)
{
    pRect->Left   = 0;
    pRect->Top    = 0;
    pRect->Right  = (INT)_width - 1;
    pRect->Bottom = (INT)_height - 1;
}

VOID CfxControl_ScrollBarV::GetThumbBounds(FXRECT *pRect)
{
    if (_maxValue == _minValue)
    {
        GetEmptyBounds(pRect);
        return;
    }
    
    INT range = _maxValue - _minValue;
    INT thumbHeight = (_pageSize * _height) / range;
    INT validValue = min(_value, _maxValue - _pageSize);
    
    pRect->Left   = 0; 
    pRect->Top    = (INT)((_height * (validValue - _minValue)) / range);
    pRect->Right  = (INT)_width - 1; 
    pRect->Bottom = pRect->Top + (INT)thumbHeight;

    INT diff = pRect->Bottom - pRect->Top + 1;
    if (diff < 20 * _oneSpace)
    {
        INT yc = pRect->Top + diff / 2;
        pRect->Top = yc - 8 * _oneSpace;
        pRect->Bottom = yc + 8 * _oneSpace;

        if (pRect->Top < _oneSpace)
        {
            pRect->Top = 1 * _oneSpace;
            pRect->Bottom = 1 * _oneSpace + 16 * _oneSpace;
        }
        else if (pRect->Bottom > _height - 2 * _oneSpace)
        {
            pRect->Bottom = (UINT)_height - 2 * _oneSpace;
            pRect->Top = (UINT)_height - 2 * _oneSpace - 16 * _oneSpace;
        }
    }
}

VOID CfxControl_ScrollBarV::MoveThumb1(INT DownX, INT DownY, INT DownValue, INT X, INT Y)
{
    INT roundFactor = _height / 2;
    
    INT range = _maxValue - _minValue; 
    INT delta = ((Y - DownY) * range + roundFactor) / _height;
    INT value = DownValue + delta;

    value = max(_minValue, value);
    value = min(_minValue + range - _pageSize, value);

    SetValue(value);
}

VOID CfxControl_ScrollBarV::MoveThumb2(INT X, INT Y)
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
    InitBorder(bsSingle, 0);

    _buttonHeight = 32;

    _backButton = new CfxControl_Button(pOwner, this);
    _backButton->SetComposite(TRUE);
    _backButton->SetBorderStyle(bsNone);
    _backButton->SetBorderWidth(1);
    _backButton->SetOnClick(this, (NotifyMethod)&CfxControl_ScrollBarV2::OnBackClick);
    _backButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ScrollBarV2::OnBackPaint);

    _nextButton = new CfxControl_Button(pOwner, this);
    _nextButton->SetComposite(TRUE);
    _nextButton->SetBorderStyle(bsNone);
    _nextButton->SetBorderWidth(1);
    _nextButton->SetOnClick(this, (NotifyMethod)&CfxControl_ScrollBarV2::OnNextClick);
    _nextButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ScrollBarV2::OnNextPaint);

    _scrollBar = new CfxControl_ScrollBarV(pOwner, this);
    _scrollBar->SetComposite(TRUE);
    _scrollBar->SetBorderStyle(bsSingle);
    _scrollBar->SetBorderWidth(0);
}

VOID CfxControl_ScrollBarV2::FixColors()
{
    _scrollBar->SetColor(_color);
    _scrollBar->SetBorderColor(_borderColor);
}

VOID CfxControl_ScrollBarV2::OnBackClick(CfxControl *pSender, VOID *pParams)
{
    _scrollBar->StepUp();
    Repaint();
}

VOID CfxControl_ScrollBarV2::OnNextClick(CfxControl *pSender, VOID *pParams)
{
    _scrollBar->StepDown();
    Repaint();
}

VOID CfxControl_ScrollBarV2::OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawUpArrow(pParams->Rect, _color, _borderColor, _backButton->GetDown()); 
}

VOID CfxControl_ScrollBarV2::OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawDownArrow(pParams->Rect, _color, _borderColor, _nextButton->GetDown()); 
}

VOID CfxControl_ScrollBarV2::ResetState()
{
    CfxControl::ResetState();
    _scrollBar->ResetState();
}

VOID CfxControl_ScrollBarV2::DefineState(CfxFiler &F)
{
    _scrollBar->DefineState(F);
}

VOID CfxControl_ScrollBarV2::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    
    UINT extraScale = IsLive() ? GetEngine(this)->GetScaleScroller() : 100;

    _width = (_width * extraScale) / 100;
    _width = ComputeScrollBarWidth(_width, 0, 100, ScaleHitSize);
    _buttonHeight = (32 * extraScale) / 100;
    _buttonHeight = (_buttonHeight * ScaleY) / 100;
    _buttonHeight = max(16, _buttonHeight);
    _buttonHeight = max(ScaleHitSize, _buttonHeight);
}

VOID CfxControl_ScrollBarV2::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    UINT w = GetClientWidth();
    UINT h = GetClientHeight();

    _backButton->SetBounds(_borderWidth, _borderWidth, w, _buttonHeight);
    _nextButton->SetBounds(_borderWidth, _borderWidth + h - _buttonHeight, w, _buttonHeight);
    _scrollBar->SetBounds(_borderWidth, _borderWidth + _buttonHeight - _borderLineWidth, w, h - _buttonHeight * 2 + _borderLineWidth * 2);
    _scrollBar->SetBorderLineWidth((UINT)_borderLineWidth);
}

VOID CfxControl_ScrollBarV2::OnKeyDown(BYTE KeyCode, BOOL *pHandled)
{
    if (!GetIsVisible() || (_width == 0) || (_height == 0)) return;

	switch (KeyCode)
    {
    case KEY_UP:   PageUp(); *pHandled = TRUE; Repaint(); break;
    case KEY_DOWN: PageDown(); *pHandled = TRUE; Repaint(); break;
    }
}

UINT ComputeScrollBarWidth(UINT Width, UINT BorderLineWidth, UINT ScaleX, UINT ScaleHitSize) 
{ 
    return max((ScaleHitSize * 2) / 3, 
               max(7 + BorderLineWidth * 2, (Width * ScaleX) / 100));
}
