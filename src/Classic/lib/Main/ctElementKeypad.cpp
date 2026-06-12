#include "fxUtils.h"
#include "fxApplication.h"
#include "fxTranslate.h"
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
    _requireNonZero = _requireSetValue = false;
    _fraction = 1;

    _valueSet = false;
    _formulaResult = 0;
    _formulaCaption = nullptr;
    _formulaExpression = nullptr;

    _value = 0;
    _valuePoint = -1;
    _negative = false;

    _textMode = false;
    _valueText = AllocString("");

    _password = 0;
    _passwordAutoNext = ElementKeypadAutoNext::Nothing;

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

bool CctControl_ElementKeypad::TestValueValid()
{
    CctSession *session = GetSession(this);
    double value = GetValue();
    bool result = false;
    char rangeText[32];

    if (_password != 0)
    {
        result = (_value == _password);
    }
    else if (_textMode)
    {
        if (_requireSetValue && (strlen(_valueText) == 0))
        {
            session->ShowMessage(fxTranslate("Value not set"));
        }
        else
        {
            result = true;
        }
    }
    else if (_requireNonZero && (value == 0))
    {
        session->ShowMessage(fxTranslate("Number not set"));
    }
    else if (_requireSetValue && !HasValue())
    {
        session->ShowMessage(fxTranslate("Number not set"));
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
        result = true;
    }

    return result;
}

bool CctControl_ElementKeypad::FinalizeValue()
{
    if (_formulaMode)
    {
        int c = _formulaItems.GetCount();
        if ((c > 0) && (_formulaItems.GetPtr(c - 1)->OperatorNext != '='))
        {
            OnKeypress("=");
            return false;
        }
    }

    return true;
}

void CctControl_ElementKeypad::OnSessionEnterNext(CfxControl *pSender, void *pParams)
{
    if (_passwordAutoNext == ElementKeypadAutoNext::Shutdown)
    {
        GetEngine(this)->Shutdown();
    }
}

void CctControl_ElementKeypad::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if (!FinalizeValue() || !TestValueValid())
    {
        *((bool *)pParams) = false;
        Repaint();
    }
    else if ((_password != 0) && (_password == _value) && (_passwordAutoNext == ElementKeypadAutoNext::Shutdown))
    {
        *((bool *)pParams) = false;
        GetEngine(this)->Shutdown();
    }
}

void CctControl_ElementKeypad::OnSessionNext(CfxControl *pSender, void *pParams)
{
    if ((_password != 0) && (_password == _value) && (_passwordAutoNext == ElementKeypadAutoNext::Shutdown))
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
            double value = GetValue();
            if ((_formulaMode == false) && (_valuePoint == -1))
            {
                int v = (int)value;
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
        ((CctSession *)pSender)->ShowMessage(fxTranslate("No Result: Value not saved"));
    }
}

void CctControl_ElementKeypad::Setup(BYTE Digits, BYTE Decimals, BYTE Fraction, int MinValue, int MaxValue, bool FormulaMode, bool TextMode) 
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
        _formulaMode = false;
    }

    _value = 0;
    _valuePoint = -1;
    _negative = false;

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
        _buttons[3][3]->SetCaption(nullptr);
    }

    if (_formulaMode && !_textMode)
    {
        _buttons[3][3]->SetCaption("=");
    }
}

void CctControl_ElementKeypad::ClearValue()
{
    _valueSet = false;
    _value = 0;
    _valuePoint = -1;
    _negative = false;
    _formulaItems.Clear();
    strcpy(_valueText, "");

    MEM_FREE(_formulaCaption);
    _formulaCaption = nullptr;
    MEM_FREE(_formulaExpression);
    _formulaExpression = nullptr;
}

bool CctControl_ElementKeypad::HasValue()
{
    return _valueSet || (_formulaMode && (_formulaItems.GetCount() > 0)) || (_textMode && strlen(_valueText));
}

double CctControl_ElementKeypad::GetValue()
{
    if ((_formulaMode == false) || (_formulaItems.GetCount() == 0))
    {
        return EncodeDouble(_value, _valuePoint, _negative, -999999999, 999999999);
    }
    else 
    {
        return _formulaResult;
    }
}

void CctControl_ElementKeypad::SetValue(double Value)
{
    DecodeDouble(Value, _decimals, &_value, &_valuePoint, &_negative, true);
    _valueSet = true;
    SetupCaption();
}

void CctControl_ElementKeypad::DefineProperties(CfxFiler &F)
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
            _formulaMode = false;
            _passwordMode = true;
        }

        Setup(_digits, _decimals, _fraction, _minValue, _maxValue, _formulaMode, _textMode);

        SetupCaption();
    }

    HackFixResultElementLockProperties(&_lockProperties);
}

void CctControl_ElementKeypad::ResetState()
{
    CfxControl_Keypad::ResetState();
    ClearValue();
    SetupCaption();
}

void CctControl_ElementKeypad::DefineState(CfxFiler &F)
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

void CctControl_ElementKeypad::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_ElementKeypad::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementKeypad::SetupCaption()
{
    char valueText[32];
    int i;

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

bool CctControl_ElementKeypad::HandleOperator(char Value)
{
    if ((Value != '+') && (Value != '-') && (Value != '/') && (Value != '*') && (Value != '='))
    {
        return false;
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

        int len = strlen(newItem.NumberText);
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

    _valueSet = false;
    _value = 0;
    _valuePoint = -1;
    _negative = false;

    Repaint();

    return true;
}

void CctControl_ElementKeypad::OnKeypressDecimal(const char *pValue)
{
    int tempValue = _value;
    int digitCount = 0;

    if (pValue == nullptr) return;
    char Value = pValue[0];

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

    _valueSet = true;

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
    _value += ((int)Value - (int)'0');
    if (_valuePoint != -1)
    {
        _valuePoint++;
    }
Done:
    if (_value == 0)
    {
        _negative = false;
    }
}

void CctControl_ElementKeypad::OnKeypressFraction(const char *pValue)
{
    int tempValue = _value;
    int digitCount = 0;
    double v = 0;

    if (pValue == nullptr) return;
    char Value = pValue[0];

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

    _valueSet = true;

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
    v = (double)((int)Value - (int)'0');

    if (_valuePoint == 0)
    {
        if (v < _fraction)
        {
            _valuePoint = 3;
            _value = _value * 1000;
            _value += (int)((v * 1000) / _fraction);
        }
        goto Done;
    }
    else if (_valuePoint > 0)
    {
        goto Done;
    }

    _value *= 10;
    _value += ((int)Value - (int)'0');
    if (_valuePoint != -1)
    {
        _valuePoint++;
    }
Done:;
}

void CctControl_ElementKeypad::OnKeypressText(const char *pValue)
{
    if (pValue == nullptr) return;
    char newValue[17];
    char Value = pValue[0];

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

    _valueSet = true;

    if (strlen(_valueText) < 16)
    {
        strcpy(newValue, _valueText);
        strcat(newValue, pValue);
        MEM_FREE(_valueText);
        _valueText = AllocString(newValue);
    }
Done:;
}

void CctControl_ElementKeypad::OnKeypress(const char *pValue)
{
    int oldValue = _value;
    int oldValuePoint = _valuePoint;
    bool oldNegative = _negative;

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
        case ElementKeypadAutoNext::Nothing:
            break;

        case ElementKeypadAutoNext::NextScreen:
            GetEngine(this)->KeyPress(KEY_RIGHT);
            break;

        case ElementKeypadAutoNext::Shutdown:
            GetEngine(this)->Shutdown();
            break;
        }
    }
}

void CctControl_ElementKeypad::PaintValue(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!HasValue()) return;

    FXFONTRESOURCE *font = GetResource()->GetFont(this, _font);
    if (font)
    {
        int h = pRect->Bottom - pRect->Top + 1;
        int x;
        int y = (h - pCanvas->FontHeight(font)) / 2;
        char *caption = _caption;

        if (_textMode)
        {
            caption = _valueText;
        }
        else if ((_valueSet == false) && (_formulaCaption != nullptr))
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

void CctControl_ElementKeypad::RebuildFormula()
{
    MEM_FREE(_formulaCaption);
    _formulaCaption = nullptr;
    MEM_FREE(_formulaExpression);
    _formulaExpression = nullptr;
    _formulaResult = 0;

    if (_formulaItems.GetCount() == 0) return;

    int maxLen = _formulaItems.GetCount() * 20;
    
    char *formulaParse = (char *)MEM_ALLOC(maxLen);
    _formulaExpression = (char *)MEM_ALLOC(maxLen);
    _formulaCaption = (char *)MEM_ALLOC(256);

    int i;
    char operatorNext[2];
    char numberText[32];

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

    int len = strlen(_formulaExpression);
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

        if (strchr(_formulaCaption, '.') == nullptr)
        {
            strcat(_formulaCaption, ".");
        }
    }

    MEM_FREE(formulaParse);
}

void CctControl_ElementKeypad::PaintFormula(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (_formulaExpression == nullptr) return;

    FXFONTRESOURCE *font = GetResource()->GetFont(this, _formulaFont);
    if (font)
    {
        int h = pRect->Bottom - pRect->Top + 1;
        int x;
        int y = (h - (int)pCanvas->FontHeight(font)) / 2;

        x = pRect->Right - pCanvas->TextWidth(font, _formulaExpression) - 4;
        pCanvas->State.TextColor = _textColor;
        pCanvas->DrawText(font, x, pRect->Top + y, _formulaExpression);

        GetResource()->ReleaseFont(font);
    }
}