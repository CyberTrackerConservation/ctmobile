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

#include "fxUtils.h"
#include "fxApplication.h"
#include "ctElementKeypad.h"
#include "ctActions.h"
#include "fxParser.h"

CfxControl *Create_Control_ElementKeypad(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementKeypad(pOwner, pParent);
}

CctControl_ElementKeypad::CctControl_ElementKeypad(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Keypad(pOwner, pParent), _retainState(pOwner, pParent), _formulaItems()
{
    InitControl(&GUID_CONTROL_ELEMENTKEYPAD);
    InitLockProperties("Digits;Decimals;Result Element;Minimum value;Maximum value");

    InitXGuid(&_elementId);
    _digits = 8;
    _decimals = 0;
    _value = 0;
    _minValue = -999999999;
    _maxValue = 999999999;
    _requireNonZero = _requireSetValue = FALSE;
    _fraction = 1;

    _valueSet = FALSE;
    _formulaResult = 0;
    _formulaCaption = NULL;
    _formulaExpression = NULL;

    _value = 0;
    _valuePoint = -1;
    _negative = FALSE;

    _textMode = FALSE;
    _valueText = AllocString("");

    _password = 0;
    _passwordAutoNext = ekanNothing;

    SetCaption("0.");

    RegisterRetainState(this, &_retainState);

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementKeypad::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementKeypad::OnSessionNext);
    RegisterSessionEvent(seEnterNext, this, (NotifyMethod)&CctControl_ElementKeypad::OnSessionEnterNext);
}

CctControl_ElementKeypad::~CctControl_ElementKeypad()
{
    UnregisterRetainState(this);

    UnregisterSessionEvent(seEnterNext, this);
    UnregisterSessionEvent(seCanNext, this);
    UnregisterSessionEvent(seNext, this);

    ClearValue();

    MEM_FREE(_valueText);
}

BOOL CctControl_ElementKeypad::TestValueValid()
{
    CctSession *session = GetSession(this);
    DOUBLE value = GetValue();
    BOOL result = FALSE;
    CHAR rangeText[32];

    if (_password != 0)
    {
        result = (_value == _password);
    }
    else if (_textMode)
    {
        if (_requireSetValue && (strlen(_valueText) == 0))
        {
            session->ShowMessage("Value not set");
        }
        else
        {
            result = true;
        }
    }
    else if (_requireNonZero && (value == 0))
    {
        session->ShowMessage("Number not set");
    }
    else if (_requireSetValue && !HasValue())
    {
        session->ShowMessage("Number not set");
    }
    else if (value < _minValue)
    {
        sprintf(rangeText, "Number less than %ld", _minValue);
        session->ShowMessage(rangeText);
    }
    else if (value > _maxValue)
    {
        sprintf(rangeText, "Number more than %ld", _maxValue);
        session->ShowMessage(rangeText);
    }
    else
    {
        result = TRUE;
    }

    return result;
}

BOOL CctControl_ElementKeypad::FinalizeValue()
{
    if (_formulaMode)
    {
        UINT c = _formulaItems.GetCount();
        if ((c > 0) && (_formulaItems.GetPtr(c - 1)->OperatorNext != '='))
        {
            OnKeypress("=");
            return FALSE;
        }
    }

    return TRUE;
}

VOID CctControl_ElementKeypad::OnSessionEnterNext(CfxControl *pSender, VOID *pParams)
{
    if (!GetApplication(this)->GetKioskMode() && (_passwordAutoNext == ekanShutdown))
    {
        GetEngine(this)->Shutdown();
    }
}

VOID CctControl_ElementKeypad::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (!FinalizeValue() || !TestValueValid())
    {
        *((BOOL *)pParams) = FALSE;
        Repaint();
    }
    else if ((_password != 0) && (_password == _value) && (_passwordAutoNext == ekanShutdown))
    {
        *((BOOL *)pParams) = FALSE;
        GetEngine(this)->Shutdown();
    }
}

VOID CctControl_ElementKeypad::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    if ((_password != 0) && (_password == _value) && (_passwordAutoNext == ekanShutdown))
    {
        GetEngine(this)->Shutdown();
        return;
    }

    if (!HasValue())
    {
        return;
    }

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId))
    {
        CctAction_MergeAttribute action(this);

        if (_textMode)
        {
            action.SetupAdd(_id, _elementId, dtPText, &_valueText);
        }
        else
        {
            DOUBLE value = GetValue();
            if ((_formulaMode == FALSE) && (_valuePoint == -1))
            {
                INT v = (INT)value;
                action.SetupAdd(_id, _elementId, dtInt32, &v);
            }
            else
            {
                action.SetupAdd(_id, _elementId, dtDouble, &value);
            }
        }

        ((CctSession *)pSender)->ExecuteAction(&action);
    }
    else if (_password == 0)
    {
        ((CctSession *)pSender)->ShowMessage("No Result: Value not saved");
    }
}

VOID CctControl_ElementKeypad::Setup(BYTE Digits, BYTE Decimals, BYTE Fraction, INT MinValue, INT MaxValue, BOOL FormulaMode, BOOL TextMode) 
{ 
    _digits = Digits; 
    _decimals = Decimals; 
    _fraction = Fraction;
    _fraction = max(1, _fraction);
    _fraction = min(9, _fraction);
    _minValue = min(MinValue, MaxValue);
    _maxValue = max(MinValue, MaxValue);
    _showPlusMinus = (_minValue < 0);
    _formulaMode = FormulaMode;
    _textMode = TextMode;

    if (_textMode)
    {
        _formulaMode = FALSE;
    }

    _value = 0;
    _valuePoint = -1;
    _negative = FALSE;

    if (_textMode)
    {
        _buttons[3][2]->SetCaption("-");
        _buttons[3][3]->SetCaption(".");
    } 
    else if (_fraction > 1)
    {
        _decimals = 3;
        _buttons[3][2]->SetCaption("F");
    }
    else
    {
        _buttons[3][2]->SetCaption(".");
    }

    if (_password != 0)
    {
        _buttons[3][2]->SetCaption("0");
        _buttons[3][3]->SetCaption(NULL);
    }

    if (_formulaMode && !_textMode)
    {
        _buttons[3][3]->SetCaption("=");
    }
}

VOID CctControl_ElementKeypad::ClearValue()
{
    _valueSet = FALSE;
    _value = 0;
    _valuePoint = -1;
    _negative = FALSE;
    _formulaItems.Clear();
    strcpy(_valueText, "");

    MEM_FREE(_formulaCaption);
    _formulaCaption = NULL;
    MEM_FREE(_formulaExpression);
    _formulaExpression = NULL;
}

BOOL CctControl_ElementKeypad::HasValue()
{
    return _valueSet || (_formulaMode && (_formulaItems.GetCount() > 0)) || (_textMode && strlen(_valueText));
}

DOUBLE CctControl_ElementKeypad::GetValue()
{
    if ((_formulaMode == FALSE) || (_formulaItems.GetCount() == 0))
    {
        return EncodeDouble(_value, _valuePoint, _negative, -999999999, 999999999);
    }
    else 
    {
        return _formulaResult;
    }
}

VOID CctControl_ElementKeypad::SetValue(DOUBLE Value)
{
    DecodeDouble(Value, _decimals, &_value, &_valuePoint, &_negative, TRUE);
    _valueSet = TRUE;
    SetupCaption();
}

VOID CctControl_ElementKeypad::DefineProperties(CfxFiler &F)
{
    CfxControl_Keypad::DefineProperties(F);
    _retainState.DefineProperties(F);

    F.DefineValue("Digits",   dtByte,    &_digits, "8");
    F.DefineValue("Decimals", dtByte,    &_decimals, F_ZERO);
    F.DefineValue("Fraction", dtByte,    &_fraction, F_ONE);
    F.DefineValue("MinValue", dtInt32,   &_minValue, "-999999999");
    F.DefineValue("MaxValue", dtInt32,   &_maxValue, "999999999");
    F.DefineValue("NonZero",  dtBoolean, &_requireNonZero, F_FALSE);
    F.DefineValue("RequireSetValue", dtBoolean, &_requireSetValue, F_FALSE);
    F.DefineValue("Password", dtInt32,   &_password, F_ZERO);
    F.DefineValue("PasswordAutoNext", dtInt8, &_passwordAutoNext, F_ZERO);

    F.DefineValue("TextMode",  dtBoolean, &_textMode, F_FALSE);

    F.DefineXOBJECT("Element", &_elementId);

    if (F.IsReader())
    {
        if (_password != 0) 
        {
            _digits = 8;
            _fraction = 0;
            _minValue = 0;
            _maxValue = 999999999;
            _formulaMode = FALSE;
            _passwordMode = TRUE;
        }

        Setup(_digits, _decimals, _fraction, _minValue, _maxValue, _formulaMode, _textMode);

        SetupCaption();
    }

    HackFixResultElementLockProperties(&_lockProperties);
}

VOID CctControl_ElementKeypad::ResetState()
{
    CfxControl_Keypad::ResetState();
    ClearValue();
    SetupCaption();
}

VOID CctControl_ElementKeypad::DefineState(CfxFiler &F)
{
    CfxControl_Keypad::DefineState(F);
    _retainState.DefineState(F);

    if (_password == 0)
    {
        F.DefineValue("Value", dtInt32, &_value);
        F.DefineValue("ValuePoint", dtInt32, &_valuePoint);
        F.DefineValue("Negative", dtBoolean, &_negative);
        F.DefineValue("ValueSet", dtBoolean, &_valueSet);
        F.DefineValue("FormulaItems", dtPBinary, _formulaItems.GetMemoryPtr());
        F.DefineValue("ValueText", dtPText, &_valueText);
    }

    if (F.IsReader())
    {
        SetupCaption();
        if (_formulaMode)
        {
            RebuildFormula();
        }
    }
}

VOID CctControl_ElementKeypad::DefinePropertiesUI(CfxFilerUI &F)
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
    F.DefineValue("Decimals",      dtByte,    &_decimals,    "MIN(0);MAX(8)");
    F.DefineValue("Digits",        dtByte,    &_digits,      "MIN(1);MAX(9)");
    F.DefineValue("Display color", dtColor,   &_displayColor);
    F.DefineValue("Display height", dtInt32,  &_displayHeight);
    F.DefineValue("Dock",          dtByte,    &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",          dtFont,    &_font);
    F.DefineValue("Formula font",  dtFont,    &_formulaFont);
    F.DefineValue("Formula color", dtColor,   &_formulaColor);
    F.DefineValue("Formula height",dtInt32,   &_formulaHeight);
    F.DefineValue("Formula mode",  dtBoolean, &_formulaMode);
    F.DefineValue("Fraction",      dtByte,    &_fraction,    "MIN(1);MAX(9)");
    F.DefineValue("Height",        dtInt32,   &_height);
    F.DefineValue("Left",          dtInt32,   &_left);
    F.DefineValue("Minimum value", dtInt32,   &_minValue,    "MIN(-999999999);MAX(999999999)");
    F.DefineValue("Maximum value", dtInt32,   &_maxValue,    "MIN(-999999999);MAX(999999999)");
    F.DefineValue("Password",      dtInt32,   &_password);
    F.DefineValue("Password auto next", dtInt8, &_passwordAutoNext, "LIST(Normal \"Advance to next screen\" Shutdown)");

    F.DefineValue("Require non-zero", dtBoolean, &_requireNonZero);
    F.DefineValue("Require set value", dtBoolean, &_requireSetValue);
    _retainState.DefinePropertiesUI(F);
    F.DefineValue("Result Element", dtGuid,   &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    //F.DefineValue("Scale for touch", dtBoolean, &_scaleForHitSize);
    F.DefineValue("Text color",    dtColor,   &_textColor);
    F.DefineValue("Text mode",     dtBoolean, &_textMode);
    F.DefineValue("Transparent",   dtBoolean, &_transparent);
    F.DefineValue("Top",           dtInt32,   &_top);
    F.DefineValue("Width",         dtInt32,   &_width);
#endif
}

VOID CctControl_ElementKeypad::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Keypad::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineObject(_elementId, eaIcon32);

    if (_textMode)
    {
        F.DefineField(_elementId, dtText);
    }
    else
    {
        if ((_formulaMode == FALSE) && (_valuePoint == -1))
        {
            F.DefineField(_elementId, dtInt32);
        }
        else
        {
            F.DefineField(_elementId, dtDouble);
        }
    }
#endif
}

VOID CctControl_ElementKeypad::SetupCaption()
{
    CHAR valueText[32];
    UINT i;

    if (_textMode)
    {
        SetCaption(_valueText);
        return;
    }

    NumberToText(_value, _digits, (_valuePoint == -1) ? 0 : (BYTE)_valuePoint, _fraction, _negative, valueText);

    if (_password != 0)
    {
        if (_value == 0)
        {
            valueText[0] = '\0';
        }
        else
        {
            for (i = 0; i < strlen(valueText); i++)
            {
                valueText[i] = (valueText[i] == '.') ? ' ' : '*';
            }
        }
        TrimString(valueText);

        if (valueText[0] == '\0')
        {
            strcpy(valueText, "Enter pin");
        }
    }

    SetCaption(valueText);
}

BOOL CctControl_ElementKeypad::HandleOperator(CHAR Value)
{
    if ((Value != '+') && (Value != '-') && (Value != '/') && (Value != '*') && (Value != '='))
    {
        return FALSE;
    }

    if (_valueSet || (_formulaItems.GetCount() == 0))
    {
        if (_formulaItems.GetCount() > 0)
        {
            FORMULA_ITEM *lastItem = _formulaItems.GetPtr(_formulaItems.GetCount() - 1);
            if (lastItem->OperatorNext == '=')
            {
                lastItem->OperatorNext = '+';
            }
        }

        FORMULA_ITEM newItem;
        strcpy(newItem.NumberText, _caption);

        UINT len = strlen(newItem.NumberText);
        if ((len > 0) && (newItem.NumberText[len - 1] == '.'))
        {
            newItem.NumberText[len - 1] = '\0';
        }

        newItem.OperatorNext = Value;
        newItem.NumberValue = EncodeDouble(_value, _valuePoint, _negative, -999999999, 999999999);
        _formulaItems.Add(newItem);
    }
    else
    {
        FORMULA_ITEM *formulaItem = _formulaItems.GetPtr(_formulaItems.GetCount() - 1);
        formulaItem->OperatorNext = Value;
    }

    RebuildFormula();

    _valueSet = FALSE;
    _value = 0;
    _valuePoint = -1;
    _negative = FALSE;

    Repaint();

    return TRUE;
}

VOID CctControl_ElementKeypad::OnKeypressDecimal(CHAR *pValue)
{
    INT tempValue = _value;
    INT digitCount = 0;

    if (pValue == NULL) return;
    CHAR Value = pValue[0];

    if (Value == 'C')
    {
        ClearValue();
        goto Done;
    }

    if (strcmp(pValue, "+/-") == 0)
    {
        _negative = !_negative;
        goto Done;
    }

    if (Value == '<')
    {
        if (_valuePoint == 0)
        {
            // Backspace on '.' does nothing but remove the point
            _valuePoint = -1;
        }
        else
        {
            // shift everything to the right
            _value /= 10;
            if (_valuePoint != -1)
            {
                _valuePoint--;
            }
        }

        goto Done;
    }

    if (HandleOperator(Value))
    {
        goto Done;
    }

    // Check if we've hit the digit limit
    while (tempValue > 0) 
    {
        digitCount++;
        tempValue/=10;
    }

    if (digitCount == _digits)
    {
        goto Done;
    }

    _valueSet = TRUE;

    // Set point position if not already there
    if (Value == '.')
    {
        if ((_valuePoint == -1) && (_decimals > 0))
        {
            _valuePoint = 0;
        }
        goto Done;
    }

    // Check if we are at the decimal limit
    if (_valuePoint + 1 > _decimals) 
    {
        goto Done;
    }

    FX_ASSERT(Value >= '0');
    FX_ASSERT(Value <= '9');

    _value *= 10;
    _value += ((INT)Value - (INT)'0');
    if (_valuePoint != -1)
    {
        _valuePoint++;
    }
Done:
    if (_value == 0)
    {
        _negative = FALSE;
    }
}

VOID CctControl_ElementKeypad::OnKeypressFraction(CHAR *pValue)
{
    INT tempValue = _value;
    INT digitCount = 0;
    DOUBLE v = 0;

    if (pValue == NULL) return;
    CHAR Value = pValue[0];

    if (Value == 'C')
    {
        ClearValue();
        goto Done;
    }

    if (strcmp(pValue, "+/-") == 0)
    {
        _negative = !_negative;
        goto Done;
    }

    if (Value == '<')
    {
        if (_valuePoint == 0)
        {
            // Backspace on '.' does nothing but remove the point
            _valuePoint = -1;
        }
        else
        {
            // shift everything to the right
            do
            {
                _value /= 10;
                if (_valuePoint != -1)
                {
                    _valuePoint--;
                }
            }
            while (_valuePoint > 0);
        }
        goto Done;
    }

    if (HandleOperator(Value))
    {
        goto Done;
    }

    // Check if we've hit the digit limit
    while (tempValue > 0) 
    {
        digitCount++;
        tempValue/=10;
    }
    if (digitCount == _digits)
    {
        goto Done;
    }

    _valueSet = TRUE;

    // Set point position if not already there
    if (Value == 'F')
    {
        if ((_valuePoint == -1) && (_decimals > 0))
        {
            _valuePoint = 0;
        }
        goto Done;
    }

    // Check if we are at the decimal limit
    if (_valuePoint + 1 > 1) 
    {
        goto Done;
    }

    FX_ASSERT(Value >= '0');
    FX_ASSERT(Value <= '9');

    // At this point we are only handling digits
    v = (DOUBLE)((INT)Value - (INT)'0');

    if (_valuePoint == 0)
    {
        if (v < _fraction)
        {
            _valuePoint = 3;
            _value = _value * 1000;
            _value += (INT)((v * 1000) / _fraction);
        }
        goto Done;
    }
    else if (_valuePoint > 0)
    {
        goto Done;
    }

    _value *= 10;
    _value += ((INT)Value - (INT)'0');
    if (_valuePoint != -1)
    {
        _valuePoint++;
    }
Done:;
}

VOID CctControl_ElementKeypad::OnKeypressText(CHAR *pValue)
{
    if (pValue == NULL) return;
    CHAR newValue[17];
    CHAR Value = pValue[0];

    if (Value == 'C')
    {
        ClearValue();
        goto Done;
    }

    if (Value == '<')
    {
        if (strlen(_valueText) > 0)
        {
            _valueText[strlen(_valueText) - 1] = '\0';
        }

        goto Done;
    }

    _valueSet = TRUE;

    if (strlen(_valueText) < 16)
    {
        strcpy(newValue, _valueText);
        strcat(newValue, pValue);
        MEM_FREE(_valueText);
        _valueText = AllocString(newValue);
    }
Done:;
}

VOID CctControl_ElementKeypad::OnKeypress(CHAR *pValue)
{
    INT oldValue = _value;
    INT oldValuePoint = _valuePoint;
    BOOL oldNegative = _negative;

    if (_textMode)
    {
        OnKeypressText(pValue);
    }
    else if (_fraction > 1)
    {
        OnKeypressFraction(pValue);
    }
    else
    {
        OnKeypressDecimal(pValue);
    }
    
    SetupCaption();

    Repaint();

    if ((_password != 0) && (_password == _value))
    {
        switch (_passwordAutoNext)
        {
        case ekanNothing: 
            break;

        case ekanNextScreen:
            GetEngine(this)->KeyPress(KEY_RIGHT);
            break;

        case ekanShutdown:
            GetEngine(this)->Shutdown();
            break;
        }
    }
}

VOID CctControl_ElementKeypad::PaintValue(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!HasValue()) return;

    FXFONTRESOURCE *font = GetResource()->GetFont(this, _font);
    if (font)
    {
        INT h = pRect->Bottom - pRect->Top + 1;
        INT x;
        INT y = (h - pCanvas->FontHeight(font)) / 2;
        CHAR *caption = _caption;

        if (_textMode)
        {
            caption = _valueText;
        }
        else if ((_valueSet == FALSE) && (_formulaCaption != NULL))
        {
            caption = _formulaCaption;
        }

        if (_password == 0)
        {
            x = pRect->Right - pCanvas->TextWidth(font, caption) - 4;
        }
        else
        {
            x = pRect->Left + (pRect->Right - pRect->Left - pCanvas->TextWidth(font, caption)) / 2;
        }

        pCanvas->State.TextColor = _textColor;
        pCanvas->DrawText(font, x, pRect->Top + y, caption);
        GetResource()->ReleaseFont(font);
    }
}

VOID CctControl_ElementKeypad::RebuildFormula()
{
    MEM_FREE(_formulaCaption);
    _formulaCaption = NULL;
    MEM_FREE(_formulaExpression);
    _formulaExpression = NULL;
    _formulaResult = 0;

    if (_formulaItems.GetCount() == 0) return;

    UINT maxLen = _formulaItems.GetCount() * 20;
    
    CHAR *formulaParse = (CHAR *)MEM_ALLOC(maxLen);
    _formulaExpression = (CHAR *)MEM_ALLOC(maxLen);
    _formulaCaption = (CHAR *)MEM_ALLOC(256);

    UINT i;
    CHAR operatorNext[2];
    CHAR numberText[32];

    strcpy(formulaParse, "");
    strcpy(_formulaExpression, "");
    strcpy(operatorNext, "X");

    for (i = 0; i < _formulaItems.GetCount(); i++)
    {
        FORMULA_ITEM *item = _formulaItems.GetPtr(i);

        sprintf(numberText, "%f%c", item->NumberValue, item->OperatorNext);
        strcat(formulaParse, numberText);

        strcat(_formulaExpression, item->NumberText);
        if (item->OperatorNext == '=') break;

        operatorNext[0] = item->OperatorNext;
        strcat(_formulaExpression, " ");
        strcat(_formulaExpression, operatorNext);
        strcat(_formulaExpression, " ");
    }

    UINT len = strlen(_formulaExpression);
    if ((len > 0) && (_formulaExpression[len - 1] == ' '))
    {
        _formulaExpression[len - 1] = '\0';
    }

    len = strlen(formulaParse);
    if (len > 0)
    {
        formulaParse[len - 1] = '\0';
    }

    {
        CfxExpressionParser parser(formulaParse);
        _formulaResult = parser.Evaluate();
        sprintf(_formulaCaption, "%f", _formulaResult);

        len = strlen(_formulaCaption);
        while (len > 0)
        {
            if (_formulaCaption[len-1] != '0') break;
            _formulaCaption[len-1] = '\0';
            len--;
        }

        if (strchr(_formulaCaption, '.') == NULL)
        {
            strcat(_formulaCaption, ".");
        }
    }

    MEM_FREE(formulaParse);
}

VOID CctControl_ElementKeypad::PaintFormula(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (_formulaExpression == NULL) return;

    FXFONTRESOURCE *font = GetResource()->GetFont(this, _formulaFont);
    if (font)
    {
        INT h = pRect->Bottom - pRect->Top + 1;
        INT x;
        INT y = (h - (INT)pCanvas->FontHeight(font)) / 2;

        x = pRect->Right - pCanvas->TextWidth(font, _formulaExpression) - 4;
        pCanvas->State.TextColor = _textColor;
        pCanvas->DrawText(font, x, pRect->Top + y, _formulaExpression);

        GetResource()->ReleaseFont(font);
    }
}