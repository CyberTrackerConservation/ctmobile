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
#include "fxKeypad.h"
#include "ctElement.h"
#include "ctSession.h"

const GUID GUID_CONTROL_ELEMENTKEYPAD = {0x5d9a98ba, 0x0f3a, 0x4b6c, {0x94, 0x39, 0x7d, 0x72, 0xd6, 0xb0, 0x6f, 0x9e}};

enum ElementKeypadAutoNext {ekanNothing = 0, ekanNextScreen, ekanShutdown};

struct FORMULA_ITEM
{
    CHAR NumberText[16];
    DOUBLE NumberValue;
    CHAR OperatorNext;
};

//
// CctControl_ElementKeypad
//
class CctControl_ElementKeypad: public CfxControl_Keypad
{
protected:
    XGUID _elementId;
    BYTE _digits, _decimals, _fraction;
    INT _minValue, _maxValue;
    BOOL _requireNonZero, _requireSetValue;
    INT _password;
    ElementKeypadAutoNext _passwordAutoNext;

    INT _value;
    INT _valuePoint;
    BOOL _negative;

    BOOL _textMode;
    CHAR *_valueText;

    BOOL _valueSet;
    TList<FORMULA_ITEM> _formulaItems;
    DOUBLE _formulaResult;
    CHAR *_formulaCaption;
    CHAR *_formulaExpression;

    CctRetainState _retainState;

    VOID OnSessionEnterNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
    VOID SetupCaption();
    VOID RebuildFormula();

    BOOL HandleOperator(CHAR Value);
    VOID OnKeypressDecimal(CHAR *pValue);
    VOID OnKeypressFraction(CHAR *pValue);
    VOID OnKeypressText(CHAR *pValue);

public:
    CctControl_ElementKeypad(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementKeypad();

    VOID ClearValue();
    VOID Setup(BYTE Digits, BYTE Decimals, BYTE Fraction, INT MinValue, INT MaxValue, BOOL FormulaMode, BOOL TextMode);

    BOOL HasValue();
    DOUBLE GetValue();
    VOID SetValue(DOUBLE Value);

    BOOL TestValueValid();
    BOOL FinalizeValue();

    VOID ResetState();
    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnKeypress(CHAR *pValue);
    VOID PaintValue(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID PaintFormula(CfxCanvas *pCanvas, FXRECT *pRect);
};

extern CfxControl *Create_Control_ElementKeypad(CfxPersistent *pOwner, CfxControl *pParent);

