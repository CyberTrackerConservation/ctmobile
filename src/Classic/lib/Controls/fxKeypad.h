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
    bool _scaleForHitSize = false;
    COLOR _formulaColor = 0;
    int _formulaHeight = 0;
    XFONT _formulaFont = 0;
    bool _formulaMode = false;
    COLOR _displayColor = 0;
    int _displayHeight = 0;
    int _buttonSpacingX = 0, _buttonSpacingY = 0;
    ControlBorderStyle _buttonBorderStyle = {};
    int _buttonBorderLineWidth = 0;
    XFONT _buttonFont = 0;
    COLOR _buttonColor = 0, _buttonTextColor = 0;
    bool _showPlusMinus = false;
    bool _passwordMode = false;

    CfxControl_Button *_buttons[4][5] = {};
    void OnButtonClick(CfxControl *pSender, void *pParams);
    int GetDisplayHeight();
public:
    CfxControl_Keypad(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void SetBounds(int Left, int Top, int Width, int Height) override;
    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    void SetupFormula(int FormulaHeight, COLOR FormulaColor);
    void SetupDisplay(int DisplayHeight, COLOR DisplayColor);
    void SetupButtons(int ButtonSpacingX, 
                      int ButtonSpacingY, 
                      int ButtonBorderLineWidth, 
                      ControlBorderStyle ButtonBorderStyle, 
                      COLOR ButtonColor,
                      COLOR ButtonTextColor);
    void SetButtonFont(const char *pValue)  { InitFont(_buttonFont, pValue);  }
    void SetFormulaFont(const char *pValue) { InitFont(_formulaFont, pValue); }

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    virtual void OnKeypress(const char *pValue) {}
    virtual void PaintValue(CfxCanvas *pCanvas, FXRECT *pRect) {}
    virtual void PaintFormula(CfxCanvas *pCanvas, FXRECT *pRect) {}
};

extern CfxControl *Create_Control_Keypad(CfxPersistent *pOwner, CfxControl *pParent);

