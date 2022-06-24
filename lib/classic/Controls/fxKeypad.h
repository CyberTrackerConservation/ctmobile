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

#pragma once
#include "pch.h"
#include "fxPanel.h"
#include "fxButton.h"

const GUID GUID_CONTROL_KEYPAD = {0x3f3eccef, 0x637b, 0x4418, {0x91, 0x0d, 0x9a, 0x17, 0xe2, 0xe6, 0x98, 0xe4}};

//
// CfxControl_Keypad
//
class CfxControl_Keypad: public CfxControl_Panel
{
protected:
    BOOL _scaleForHitSize;
    COLOR _formulaColor;
    UINT _formulaHeight;
    XFONT _formulaFont;
    BOOL _formulaMode;
    COLOR _displayColor;
    UINT _displayHeight;
    UINT _buttonSpacingX, _buttonSpacingY;
    ControlBorderStyle _buttonBorderStyle;
    UINT _buttonBorderLineWidth;
    XFONT _buttonFont;
    COLOR _buttonColor, _buttonTextColor;
    BOOL _showPlusMinus;
    BOOL _passwordMode;

    CfxControl_Button *_buttons[4][5];
    VOID OnButtonClick(CfxControl *pSender, VOID *pParams);
    UINT GetDisplayHeight();
public:
    CfxControl_Keypad(CfxPersistent *pOwner, CfxControl *pParent);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    VOID SetupFormula(UINT FormulaHeight, COLOR FormulaColor);
    VOID SetupDisplay(UINT DisplayHeight, COLOR DisplayColor);
    VOID SetupButtons(UINT ButtonSpacingX, 
                      UINT ButtonSpacingY, 
                      UINT ButtonBorderLineWidth, 
                      ControlBorderStyle ButtonBorderStyle, 
                      COLOR ButtonColor,
                      COLOR ButtonTextColor);
    VOID SetButtonFont(const CHAR *pValue)  { InitFont(_buttonFont, pValue);  }
    VOID SetFormulaFont(const CHAR *pValue) { InitFont(_formulaFont, pValue); }

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    virtual VOID OnKeypress(CHAR *pValue) {}
    virtual VOID PaintValue(CfxCanvas *pCanvas, FXRECT *pRect) {}
    virtual VOID PaintFormula(CfxCanvas *pCanvas, FXRECT *pRect) {}
};

extern CfxControl *Create_Control_Keypad(CfxPersistent *pOwner, CfxControl *pParent);

