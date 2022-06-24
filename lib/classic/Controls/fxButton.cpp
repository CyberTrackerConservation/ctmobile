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

    _penDown = _penOver = _down = FALSE;
    _groupId = 0;
}

VOID CfxControl_Button::SetDown(const BOOL Value)
{
    //
    // Do nothing if:
    // 1. GroupId == 0 means it's a regular button, not a pushbutton
    // 2. GroupId != 0 means we need at least one button up, so Value cannot be FALSE because
    //    the down states are managed internally
    //
    if ((_groupId == 0) || !Value) return;

    INT i = 0;
    CfxControl *control = NULL;
    do 
    {
        control = _parent->EnumControls(i);
        if (control)
        {
            if (CompareGuid(control->GetTypeId(), &GUID_CONTROL_BUTTON) &&
                (((CfxControl_Button *)control)->_groupId == _groupId))
            {
                ((CfxControl_Button *)control)->_down = FALSE;
            }
            i++;
        }
    } while (control);

    _down = TRUE;
}

VOID CfxControl_Button::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
	BOOL invert = GetDown();

    COLOR oldColor = _color;
    COLOR oldTextColor = _textColor;

    _color = pCanvas->InvertColor(_color, invert);
    _textColor = pCanvas->InvertColor(_textColor, invert);
    CfxControl_Panel::OnPaint(pCanvas, pRect);
    _textColor = oldTextColor;
    _color = oldColor;
}

VOID CfxControl_Button::OnPenDown(INT X, INT Y)
{
	_penDown = _penOver = TRUE;
    Repaint();
}

VOID CfxControl_Button::OnPenUp(INT X, INT Y)
{
    if (!_penDown) return;

    _penDown = FALSE;
    SetDown(!_down);

    Repaint();
}

VOID CfxControl_Button::OnPenMove(INT X, INT Y)
{
    if (!_penDown) return;

    if (_penOver != ((X >= 0) && (X < _width) && (Y >= 0) && (Y < _height)))
    {
		_penOver = !_penOver;
        Repaint();
	}
}

VOID CfxControl_Button::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _width = max((INT)ScaleHitSize, _width);
    _height = max((INT)ScaleHitSize, _height);
}
