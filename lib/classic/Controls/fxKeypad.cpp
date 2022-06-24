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

#include "fxKeypad.h"
#include "fxApplication.h"
#include "fxUtils.h"
#include "fxMath.h"

CfxControl *Create_Control_Keypad(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Keypad(pOwner, pParent);
}

CfxControl_Keypad::CfxControl_Keypad(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_KEYPAD);
    InitBorder(_borderStyle, 0);

    InitFont(F_FONT_DEFAULT_MB);

    InitFont(_formulaFont, F_FONT_FIXED_L);

    _scaleForHitSize = TRUE;

    _formulaColor = _color;
    _formulaHeight = 16;
    _formulaMode = FALSE;
    _passwordMode = FALSE;

    InitFont(_buttonFont, F_FONT_DEFAULT_LB);
    _buttonSpacingX = _buttonSpacingY = 4;
    _buttonBorderLineWidth = 1;
    _buttonBorderStyle = bsSingle;
    _buttonColor = _color;
    _buttonTextColor = _textColor;

    _displayColor = _color;
    _displayHeight = 30;

    _showPlusMinus = FALSE;

    for (UINT j=0; j<5; j++)
    for (UINT i=0; i<4; i++)
    {
        _buttons[i][j] = new CfxControl_Button(pOwner, this);
        _buttons[i][j]->SetComposite(TRUE);
        _buttons[i][j]->SetOnClick(this, (NotifyMethod)&CfxControl_Keypad::OnButtonClick);
    }

    _buttons[0][0]->SetCaption("7");
    _buttons[1][0]->SetCaption("8");
    _buttons[2][0]->SetCaption("9");
    _buttons[3][0]->SetCaption("C");

    _buttons[0][1]->SetCaption("4");
    _buttons[1][1]->SetCaption("5");
    _buttons[2][1]->SetCaption("6");
    _buttons[3][1]->SetCaption("<");

    _buttons[0][2]->SetCaption("1");
    _buttons[1][2]->SetCaption("2");
    _buttons[2][2]->SetCaption("3");
    _buttons[3][2]->SetCaption(".");

    _buttons[0][3]->SetCaption("0");
    _buttons[1][3]->SetCaption(NULL);
    _buttons[2][3]->SetCaption(NULL);
    _buttons[3][3]->SetCaption("+/-");

    _buttons[0][4]->SetCaption("+");
    _buttons[1][4]->SetCaption("-");
    _buttons[2][4]->SetCaption("/");
    _buttons[3][4]->SetCaption("*");
}

VOID CfxControl_Keypad::SetupFormula(UINT FormulaHeight, COLOR FormulaColor)
{
    _formulaHeight = FormulaHeight;
    _formulaColor = FormulaColor;
}

VOID CfxControl_Keypad::SetupDisplay(UINT DisplayHeight, COLOR DisplayColor)
{
    _displayHeight = DisplayHeight;
    _displayColor = DisplayColor;
}

VOID CfxControl_Keypad::SetupButtons(
    UINT ButtonSpacingX, 
    UINT ButtonSpacingY, 
    UINT ButtonBorderLineWidth, 
    ControlBorderStyle ButtonBorderStyle, 
    COLOR ButtonColor,
    COLOR ButtonTextColor)
{
    _buttonSpacingX = ButtonSpacingX;
    _buttonSpacingY = ButtonSpacingY;
    _buttonBorderLineWidth = ButtonBorderLineWidth;
    _buttonBorderStyle = ButtonBorderStyle;
    _buttonColor = ButtonColor;
    _buttonTextColor = ButtonTextColor;
}

VOID CfxControl_Keypad::OnButtonClick(CfxControl *pSender, VOID *pParams)
{
    CHAR *caption = ((CfxControl_Button *)pSender)->GetCaption();
    OnKeypress(caption);
}

VOID CfxControl_Keypad::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    UINT h;
    INT fh;

    CfxControl_Panel::SetBounds(Left, Top, Width, Height);

    if (_passwordMode)
    {
        h = 3;
        fh = 0;
        _buttons[3][2]->SetCaption("0");
    }
    else if (_formulaMode)
    {
        h = 5;
        fh = _formulaHeight;
    }
    else
    {
        h = 4;
        fh = 0;
    }

    // Move the buttons and set their captions
    INT buttonAreaW = (INT)GetClientWidth();
    INT buttonAreaH = (INT)GetClientHeight() - (INT)_displayHeight - fh;

    INT actualButtonW = (buttonAreaW - 5 * _buttonSpacingX) / 4;
    INT actualButtonH = (buttonAreaH - (h + 1) * _buttonSpacingY) / h;

    if (buttonAreaW < actualButtonW * 4)
    {
        actualButtonW = buttonAreaW / 4;
    }

    if (buttonAreaH < actualButtonH * (INT)h)
    {
        actualButtonH = buttonAreaH / h;
    }

    INT buttonSpacingX = (buttonAreaW - (4 * actualButtonW)) / 5;
    INT buttonSpacingY = (buttonAreaH - (h * actualButtonH)) / (h + 1);
    INT buttonCloserX = (buttonSpacingX == 0) ? _buttonBorderLineWidth : 0;
    INT buttonCloserY = (buttonSpacingY == 0) ? _buttonBorderLineWidth : 0;
    
    INT xstart;
    INT ystart = _displayHeight + _borderWidth + buttonSpacingY + fh;

    for (UINT j=0; j<5; j++)
    {
        xstart = (INT)_borderWidth + buttonSpacingX;
        for (UINT i=0; i<4; i++)
        {
            CfxControl_Button *button = _buttons[i][j];
            UINT bw = actualButtonW;
            UINT bh = actualButtonH;

            if (j >= h)
            {
                button->SetVisible(FALSE);
                continue;
            }

            if ((i == 3) && (buttonSpacingX == 0))
            {
                bw = buttonAreaW - xstart - buttonCloserX;
            }

            if ((j == h - 1) && (buttonSpacingY == 0))
            {
                bh = (INT)GetClientHeight() - ystart - buttonCloserY;
            }

            if ((i == 0) && (j == 3))
            {
                button->SetBounds(xstart, ystart, bw * 3 + buttonSpacingX * 2 + buttonCloserX, bh + buttonCloserY);
            }
            else
            {
                button->SetBounds(xstart, ystart, bw + buttonCloserX, bh + buttonCloserY);
            }

            button->SetVisible(button->GetCaption() != NULL);
            button->AssignFont(&_buttonFont);
            button->SetColor(_buttonColor);
            button->SetTextColor(_buttonTextColor);
            button->SetBorder(_buttonBorderStyle, 0, _buttonBorderLineWidth, _borderColor);
            xstart = xstart + actualButtonW + buttonSpacingX;
        }

        ystart = ystart + actualButtonH + buttonSpacingY;
    }
}

VOID CfxControl_Keypad::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    if (!_scaleForHitSize)
    {
        ScaleHitSize = 0;
    }

    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _buttonSpacingX = (_buttonSpacingX * ScaleX) / 100;
    _buttonSpacingY = (_buttonSpacingY * ScaleY) / 100;
    _displayHeight = (_displayHeight * ScaleY) / 100;
    _formulaHeight = (_formulaHeight * ScaleY) / 100;
    _buttonBorderLineWidth = (_buttonBorderLineWidth * ScaleBorder ) / 100;

    CfxResource *resource = GetResource();
    CfxCanvas *canvas = GetCanvas(this);

    FXFONTRESOURCE *font;

    font = resource->GetFont(this, _formulaFont);
    if (font)
    {
        _formulaHeight = max(_formulaHeight, canvas->FontHeight(font) + _borderLineWidth);
        resource->ReleaseFont(font);
    }

    font = resource->GetFont(this, _font);
    if (font)
    {
    	_displayHeight = max(_displayHeight, canvas->FontHeight(font) + _borderLineWidth);
        resource->ReleaseFont(font);
    }
}

VOID CfxControl_Keypad::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);
    
    F.DefineValue("ButtonSpacingX", dtInt32, &_buttonSpacingX, "4");
    F.DefineValue("ButtonSpacingY", dtInt32, &_buttonSpacingY, "4");
    F.DefineValue("ButtonBorderStyle", dtInt8,  &_buttonBorderStyle, F_ONE);
    F.DefineValue("ButtonBorderLineWidth", dtInt32, &_buttonBorderLineWidth, F_ONE);

    F.DefineXFONT("ButtonFont", &_buttonFont, F_FONT_DEFAULT_LB);
    F.DefineValue("ButtonColor",       dtColor, &_buttonColor, F_COLOR_WHITE);
    F.DefineValue("ButtonTextColor",   dtColor, &_buttonTextColor, F_COLOR_BLACK);

    F.DefineValue("DisplayColor",      dtColor, &_displayColor, F_COLOR_WHITE);
    F.DefineValue("DisplayHeight",     dtInt32, &_displayHeight, "30");

    F.DefineXFONT("FormulaFont", &_formulaFont, F_FONT_FIXED_L);
    F.DefineValue("FormulaColor",      dtColor, &_formulaColor, F_COLOR_WHITE);
    F.DefineValue("FormulaHeight",     dtInt32, &_formulaHeight, "16");
    F.DefineValue("FormulaMode",       dtBoolean, &_formulaMode, F_FALSE);

    F.DefineValue("ScaleForHitSize",   dtBoolean, &_scaleForHitSize, F_TRUE);

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);
    }
}

VOID CfxControl_Keypad::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color",  dtColor,   &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style",  dtInt8,    &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width",  dtInt32,   &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button border", dtInt8,    &_buttonBorderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Button border line width", dtInt32, &_buttonBorderLineWidth, "MIN(1);MAX(16)");
    F.DefineValue("Button color",  dtColor,   &_buttonColor);
    F.DefineValue("Button font",   dtFont,    &_buttonFont);
    F.DefineValue("Button spacing X", dtInt32, &_buttonSpacingX,  "MIN(0);MAX(80)");
    F.DefineValue("Button spacing Y", dtInt32, &_buttonSpacingY,  "MIN(0);MAX(80)");
    F.DefineValue("Button text color", dtColor, &_buttonTextColor);
    F.DefineValue("Color",         dtColor,   &_color);
    F.DefineValue("Display color", dtColor,   &_displayColor);
    F.DefineValue("Display height", dtInt32,  &_displayHeight);
    F.DefineValue("Dock",          dtByte,    &_dock,         "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",          dtFont,    &_font);
    F.DefineValue("Formula font",  dtFont,    &_formulaFont);
    F.DefineValue("Formula color", dtColor,   &_formulaColor);
    F.DefineValue("Formula height",dtInt32,   &_formulaHeight);
    F.DefineValue("Formula mode",  dtBoolean, &_formulaMode);
    F.DefineValue("Height",        dtInt32,   &_height);
    F.DefineValue("Left",          dtInt32,   &_left);
    F.DefineValue("Scale for touch", dtBoolean, &_scaleForHitSize);  
    F.DefineValue("Show +/-",      dtBoolean, &_showPlusMinus);
    F.DefineValue("Text color",    dtColor,   &_textColor);
    F.DefineValue("Transparent",   dtBoolean, &_transparent);
    F.DefineValue("Top",           dtInt32,   &_top);
    F.DefineValue("Width",         dtInt32,   &_width);
#endif
}

VOID CfxControl_Keypad::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Panel::DefineResources(F);

    F.DefineFont(_buttonFont);
    for (UINT j=0; j<5; j++)
    for (UINT i=0; i<4; i++)
    {
        _buttons[i][j]->DefineResources(F);
    }

    F.DefineFont(_formulaFont);
#endif
}

VOID CfxControl_Keypad::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    FXRECT rect = *pRect;
    InflateRect(&rect, -(INT)_borderLineWidth, -(INT)_borderLineWidth);
    rect.Bottom = rect.Top;

    if (_formulaMode && (_formulaHeight > 0))
    {
        rect.Bottom = rect.Top + (UINT)_formulaHeight - _borderLineWidth;
        pCanvas->State.BrushColor = _formulaColor;
        pCanvas->FillRect(&rect);
        PaintFormula(pCanvas, &rect);
    }

    rect.Top = rect.Bottom;
    rect.Bottom = rect.Top + (UINT)_displayHeight - _borderLineWidth;
    pCanvas->State.BrushColor = _displayColor;
    pCanvas->FillRect(&rect);
    PaintValue(pCanvas, &rect);

    rect = *pRect;
    rect.Bottom = rect.Top + (UINT)_displayHeight;
    if (_formulaMode) rect.Bottom += (UINT)_formulaHeight;

    pCanvas->State.LineColor = _borderColor;
    pCanvas->State.LineWidth = (UINT)_borderLineWidth;
    pCanvas->FrameRect(&rect);
}
