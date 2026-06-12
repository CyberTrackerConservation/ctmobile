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

    _scaleForHitSize = true;

    _formulaColor = _color;
    _formulaHeight = 16;
    _formulaMode = false;
    _passwordMode = false;

    InitFont(_buttonFont, F_FONT_DEFAULT_LB);
    _buttonSpacingX = _buttonSpacingY = 4;
    _buttonBorderLineWidth = 1;
    _buttonBorderStyle = ControlBorderStyle::Single;
    _buttonColor = _color;
    _buttonTextColor = _textColor;

    _displayColor = _color;
    _displayHeight = 30;

    _showPlusMinus = false;

    for (int j=0; j<5; j++)
    for (int i=0; i<4; i++)
    {
        _buttons[i][j] = new CfxControl_Button(pOwner, this);
        _buttons[i][j]->SetComposite(true);
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
    _buttons[1][3]->SetCaption(nullptr);
    _buttons[2][3]->SetCaption(nullptr);
    _buttons[3][3]->SetCaption("+/-");

    _buttons[0][4]->SetCaption("+");
    _buttons[1][4]->SetCaption("-");
    _buttons[2][4]->SetCaption("/");
    _buttons[3][4]->SetCaption("*");
}

void CfxControl_Keypad::SetupFormula(int FormulaHeight, COLOR FormulaColor)
{
    _formulaHeight = FormulaHeight;
    _formulaColor = FormulaColor;
}

void CfxControl_Keypad::SetupDisplay(int DisplayHeight, COLOR DisplayColor)
{
    _displayHeight = DisplayHeight;
    _displayColor = DisplayColor;
}

void CfxControl_Keypad::SetupButtons(
    int ButtonSpacingX, 
    int ButtonSpacingY, 
    int ButtonBorderLineWidth, 
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

void CfxControl_Keypad::OnButtonClick(CfxControl *pSender, void *pParams)
{
    char *caption = ((CfxControl_Button *)pSender)->GetCaption();
    OnKeypress(caption);
}

void CfxControl_Keypad::SetBounds(int Left, int Top, int Width, int Height)
{
    int h;
    int fh;

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
    int buttonAreaW = (int)GetClientWidth();
    int buttonAreaH = (int)GetClientHeight() - (int)_displayHeight - fh;

    int actualButtonW = (buttonAreaW - 5 * _buttonSpacingX) / 4;
    int actualButtonH = (buttonAreaH - (h + 1) * _buttonSpacingY) / h;

    if (buttonAreaW < actualButtonW * 4)
    {
        actualButtonW = buttonAreaW / 4;
    }

    if (buttonAreaH < actualButtonH * (int)h)
    {
        actualButtonH = buttonAreaH / h;
    }

    int buttonSpacingX = (buttonAreaW - (4 * actualButtonW)) / 5;
    int buttonSpacingY = (buttonAreaH - (h * actualButtonH)) / (h + 1);
    int buttonCloserX = (buttonSpacingX == 0) ? _buttonBorderLineWidth : 0;
    int buttonCloserY = (buttonSpacingY == 0) ? _buttonBorderLineWidth : 0;
    
    int xstart;
    int ystart = _displayHeight + _borderWidth + buttonSpacingY + fh;

    for (int j=0; j<5; j++)
    {
        xstart = (int)_borderWidth + buttonSpacingX;
        for (int i=0; i<4; i++)
        {
            CfxControl_Button *button = _buttons[i][j];
            int bw = actualButtonW;
            int bh = actualButtonH;

            if (j >= h)
            {
                button->SetVisible(false);
                continue;
            }

            if ((i == 3) && (buttonSpacingX == 0))
            {
                bw = buttonAreaW - xstart - buttonCloserX;
            }

            if ((j == h - 1) && (buttonSpacingY == 0))
            {
                bh = (int)GetClientHeight() - ystart - buttonCloserY;
            }

            if ((i == 0) && (j == 3))
            {
                button->SetBounds(xstart, ystart, bw * 3 + buttonSpacingX * 2 + buttonCloserX, bh + buttonCloserY);
            }
            else
            {
                button->SetBounds(xstart, ystart, bw + buttonCloserX, bh + buttonCloserY);
            }

            button->SetVisible(button->GetCaption() != nullptr);
            button->AssignFont(&_buttonFont);
            button->SetColor(_buttonColor);
            button->SetTextColor(_buttonTextColor);
            button->SetBorder(_buttonBorderStyle, 0, _buttonBorderLineWidth, _borderColor);
            xstart = xstart + actualButtonW + buttonSpacingX;
        }

        ystart = ystart + actualButtonH + buttonSpacingY;
    }
}

void CfxControl_Keypad::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
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

void CfxControl_Keypad::DefineProperties(CfxFiler &F)
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

void CfxControl_Keypad::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CfxControl_Keypad::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_Keypad::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    FXRECT rect = *pRect;
    InflateRect(&rect, -(int)_borderLineWidth, -(int)_borderLineWidth);
    rect.Bottom = rect.Top;

    if (_formulaMode && (_formulaHeight > 0))
    {
        rect.Bottom = rect.Top + (int)_formulaHeight - _borderLineWidth;
        pCanvas->State.BrushColor = _formulaColor;
        pCanvas->FillRect(&rect);
        PaintFormula(pCanvas, &rect);
    }

    rect.Top = rect.Bottom;
    rect.Bottom = rect.Top + (int)_displayHeight - _borderLineWidth;
    pCanvas->State.BrushColor = _displayColor;
    pCanvas->FillRect(&rect);
    PaintValue(pCanvas, &rect);

    rect = *pRect;
    rect.Bottom = rect.Top + (int)_displayHeight;
    if (_formulaMode) rect.Bottom += (int)_formulaHeight;

    pCanvas->State.LineColor = _borderColor;
    pCanvas->State.LineWidth = (int)_borderLineWidth;
    pCanvas->FrameRect(&rect);
}
