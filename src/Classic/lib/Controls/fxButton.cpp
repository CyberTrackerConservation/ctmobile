#include "fxApplication.h"
#include "fxUtils.h"
#include "fxButton.h"

//*************************************************************************************************
// CfxControl_Button

CfxControl *Create_Control_Button(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Button(pOwner, pParent);
}

CfxControl_Button::CfxControl_Button(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_BUTTON);

    _penDown = _penOver = _down = false;
    _groupId = 0;
}

void CfxControl_Button::SetDown(const bool Value)
{
    //
    // Do nothing if:
    // 1. GroupId == 0 means it's a regular button, not a pushbutton
    // 2. GroupId != 0 means we need at least one button up, so Value cannot be false because
    //    the down states are managed internally
    //
    if ((_groupId == 0) || !Value) return;

    int i = 0;
    CfxControl *control = nullptr;
    do 
    {
        control = _parent->EnumControls(i);
        if (control)
        {
            if (CompareGuid(control->GetTypeId(), &GUID_CONTROL_BUTTON) &&
                (((CfxControl_Button *)control)->_groupId == _groupId))
            {
                ((CfxControl_Button *)control)->_down = false;
            }
            i++;
        }
    } while (control);

    _down = true;
}

void CfxControl_Button::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
	bool invert = GetDown();

    COLOR oldColor = _color;
    COLOR oldTextColor = _textColor;

    _color = pCanvas->InvertColor(_color, invert);
    _textColor = pCanvas->InvertColor(_textColor, invert);
    CfxControl_Panel::OnPaint(pCanvas, pRect);
    _textColor = oldTextColor;
    _color = oldColor;
}

void CfxControl_Button::OnPenDown(int X, int Y)
{
	_penDown = _penOver = true;
    Repaint();
}

void CfxControl_Button::OnPenUp(int X, int Y)
{
    if (!_penDown) return;

    _penDown = false;
    SetDown(!_down);

    Repaint();
}

void CfxControl_Button::OnPenMove(int X, int Y)
{
    if (!_penDown) return;

    if (_penOver != ((X >= 0) && (X < _width) && (Y >= 0) && (Y < _height)))
    {
		_penOver = !_penOver;
        Repaint();
	}
}

void CfxControl_Button::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _width = max((int)ScaleHitSize, _width);
    _height = max((int)ScaleHitSize, _height);
}
