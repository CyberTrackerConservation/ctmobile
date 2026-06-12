#pragma once

#include "pch.h"
#include "fxKeypad.h"
#include "ctElement.h"
#include "ctSession.h"

const GUID GUID_CONTROL_ELEMENTKEYPAD = {0x5d9a98ba, 0x0f3a, 0x4b6c, {0x94, 0x39, 0x7d, 0x72, 0xd6, 0xb0, 0x6f, 0x9e}};

enum class ElementKeypadAutoNext : int8_t {Nothing = 0, NextScreen, Shutdown};

struct FORMULA_ITEM
{
    char NumberText[16] = {};
    double NumberValue = 0.0;
    char OperatorNext = 0;
};

//
// CctControl_ElementKeypad
//
class CctControl_ElementKeypad: public CfxControl_Keypad
{
protected:
    XGUID _elementId = 0;
    BYTE _digits = 0, _decimals = 0, _fraction = 0;
    int _minValue = 0, _maxValue = 0;
    bool _requireNonZero = false, _requireSetValue = false;
    int _password = 0;
    ElementKeypadAutoNext _passwordAutoNext = ElementKeypadAutoNext::Nothing;

    int _value = 0;
    int _valuePoint = 0;
    bool _negative = false;

    bool _textMode = false;
    char *_valueText = nullptr;

    bool _valueSet = false;
    TList<FORMULA_ITEM> _formulaItems;
    double _formulaResult = 0.0;
    char *_formulaCaption = nullptr;
    char *_formulaExpression = nullptr;

    CctRetainState _retainState;

    void OnSessionEnterNext(CfxControl *pSender, void *pParams);
    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
    void SetupCaption();
    void RebuildFormula();

    bool HandleOperator(char Value);
    void OnKeypressDecimal(const char *pValue);
    void OnKeypressFraction(const char *pValue);
    void OnKeypressText(const char *pValue);

public:
    CctControl_ElementKeypad(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementKeypad();

    void ClearValue();
    void Setup(BYTE Digits, BYTE Decimals, BYTE Fraction, int MinValue, int MaxValue, bool FormulaMode, bool TextMode);

    bool HasValue();
    double GetValue();
    void SetValue(double Value);

    bool TestValueValid();
    bool FinalizeValue();

    void ResetState() override;
    void DefineState(CfxFiler &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnKeypress(const char *pValue) override;
    void PaintValue(CfxCanvas *pCanvas, FXRECT *pRect) override;
    void PaintFormula(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

extern CfxControl *Create_Control_ElementKeypad(CfxPersistent *pOwner, CfxControl *pParent);

