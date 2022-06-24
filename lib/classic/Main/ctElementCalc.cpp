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

#include "ctElementCalc.h"
#include "fxApplication.h"
#include "fxUtils.h"
#include "ctSession.h"
#include "ctActions.h"

#include "fxMath.h"
#include "fxParser.h"


CfxControl *Create_Control_ElementCalc(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementCalc(pOwner, pParent);
}

CctControl_ElementCalc::CctControl_ElementCalc(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)    
{
    INT i;

    InitControl(&GUID_CONTROL_ELEMENTCALC);
    InitLockProperties("Elements A;Elements B;Elements C;Font;Formula;Result Element");

    InitXGuid(&_resultElementId);

    _linkOnlyOnPathEmpty = FALSE;
    for (i=0; i<ARRAYSIZE(_targetScreenIds); i++)
    {
        InitXGuid(&_targetScreenIds[i]);
    }

    for (i=0; i<ARRAYSIZE(_elements); i++)
    {
        _elements[i] = new XGUIDLIST();
    }

    _formula = NULL;
    _digits = 8;
    _decimals = 0;
    _fraction = 1;
    _outputAsTime = _outputAsDate = _outputAsInteger = FALSE;

    _resultGlobalValue = NULL;

    _hidden = FALSE;

    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementCalc::OnSessionNext);
    RegisterSessionEvent(seGlobalChanged, this, (NotifyMethod)&CctControl_ElementCalc::OnGlobalChanged);
}

CctControl_ElementCalc::~CctControl_ElementCalc()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seGlobalChanged, this);

    for (INT i=0; i<ARRAYSIZE(_elements); i++)
    {
        delete _elements[i];
        _elements[i] = NULL;
    }

    MEM_FREE(_formula);
    _formula = NULL;

    MEM_FREE(_resultGlobalValue);
	_resultGlobalValue = NULL;
}

VOID CctControl_ElementCalc::OnGlobalChanged(CfxControl *pSender, VOID *pParams)
{
    Evaluate();
    Repaint();
}

VOID CctControl_ElementCalc::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    DOUBLE value = Evaluate();

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_resultElementId))
    {
        CctAction_MergeAttribute action(this);

        if (_outputAsDate)
        {
            action.SetupAdd(_id, _resultElementId, dtDate, &value);
        }
        else if (_outputAsInteger)
        {
            INT ivalue = (INT)value;
            action.SetupAdd(_id, _resultElementId, dtInt32, &ivalue);
        }
        else if (_outputAsTime)
        {
            action.SetupAdd(_id, _resultElementId, dtTime, &value);
        }
        else
        {
            action.SetupAdd(_id, _resultElementId, dtDouble, &value);
        }

        ((CctSession *)pSender)->ExecuteAction(&action);
    }

    // Set the global value
    if (_resultGlobalValue != NULL)
    {
        CctAction_SetGlobalValue action(this);
        action.Setup(_resultGlobalValue, value);
        ((CctSession *)pSender)->ExecuteAction(&action);
    }

    // Add the links
    if (_linkOnlyOnPathEmpty && !GetSession(this)->GetCurrentSighting()->EOP())
    {
        return;
    }

    for (INT i=0; i<ARRAYSIZE(_targetScreenIds); i++)
    {
        if (((INT)value == i) && !IsNullXGuid(&_targetScreenIds[i]))
        {
            CctAction_InsertScreen action(this);
            action.Setup(_targetScreenIds[i]);
            ((CctSession *)pSender)->ExecuteAction(&action);
        }
    }
}

DOUBLE CctControl_ElementCalc::CalcValue(XGUIDLIST *pElements)
{
    if (!IsLive())
    {
        return 0;
    }

    ATTRIBUTES *attributes = GetSession(this)->GetCurrentAttributes();
    DOUBLE value = 0;

    for (UINT i=0; i<attributes->GetCount(); i++)
    {
        ATTRIBUTE *attribute = attributes->GetPtr(i);
        if (pElements->IndexOf(attribute->ElementId) != -1)
        {
            value += Type_VariantToDouble(attribute->Value);
        }
    }

    return value;
}

DOUBLE CctControl_ElementCalc::Evaluate()
{
    if (_fraction > 1) 
    {
        _decimals = 3;
    }
    
    CfxExpressionParser parser(_formula);

    for (INT i=0; i<ARRAYSIZE(_elements); i++)
    {
        CHAR sym[2] = "A";
        sym[0] += (CHAR)i;
        parser.SetSymbol(sym, CalcValue(_elements[i]));
    }

    // Add the "Today" symbol
    if (IsLive())
    {
        FXDATETIME dateTime;
        GetHost(this)->GetDateTime(&dateTime);
        parser.SetSymbol("Today", EncodeDateTime(&dateTime));
    }

    // Add global variables
    if (IsLive()) 
    {
        CctSession *session = GetSession(this);
        UINT i = 0;
        GLOBALVALUE *globalValue = session->EnumGlobalValues(0);
        while (globalValue)
        {
            parser.SetSymbol(globalValue->Name, globalValue->Value);
            i++;
            globalValue = session->EnumGlobalValues(i);
        }
    }
   
    // Run the calculation
    DOUBLE dvalue = FxAbs(parser.Evaluate());

    if (parser.GetParseError())
    {
        dvalue = 0;
        if (IsLive())
        {
            SetCaption(_formula == NULL ? "No formula" : "Bad formula");
        }
        else
        {
            SetCaption(_formula == NULL ? "Set \"Formula\" property" : "Bad formula");
        }
    }
    else
    {
        if (_outputAsDate)
        {
            CfxString valueText;
            Type_DataToText(dtDate, &dvalue, valueText);
            SetCaption(valueText.Get());
        }
        else if (_outputAsInteger)
        {
            CfxString valueText;
            INT ivalue = (INT)dvalue;
            Type_DataToText(dtInt32, &ivalue, valueText);
            SetCaption(valueText.Get());
        }
        else if (_outputAsTime)
        {
            CfxString valueText;
            Type_DataToText(dtTime, &dvalue, valueText);
            SetCaption(valueText.Get());
        }
        else
        {
            CHAR valueText[32];
            INT ivalue, pvalue;
            BOOL inegative;
            DecodeDouble(dvalue, _decimals, &ivalue, &pvalue, &inegative);
            NumberToText(ivalue, _digits, _decimals, _fraction, inegative, valueText, FALSE);
            SetCaption(valueText);
        }
    }

    return dvalue;
}

VOID CctControl_ElementCalc::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    UINT i, j;

    F.DefineFont(_font);

    for (j=0; j<ARRAYSIZE(_elements); j++)
    {
        XGUIDLIST *elements = _elements[j];
        i = 0;
        while (i < elements->GetCount()) 
        {
            if (IsNullXGuid(elements->GetPtr(i)))
            {
                elements->Delete(i);
                i = 0;
            }
            else
            {
                F.DefineObject(elements->Get(i));
                i++;
            }
        }
    }

    F.DefineObject(_resultElementId);

    for (i=0; i<ARRAYSIZE(_targetScreenIds); i++)
    {
        F.DefineObject(_targetScreenIds[i]);
    }

    if (_outputAsDate)
    {
        F.DefineField(_resultElementId, dtDate);
    }
    else if (_outputAsInteger)
    {
        F.DefineField(_resultElementId, dtInt32);
    }
    else if (_outputAsTime)
    {
        F.DefineField(_resultElementId, dtTime);
    }
    else
    {
        F.DefineField(_resultElementId, dtDouble);
    }
#endif
}

VOID CctControl_ElementCalc::DefineProperties(CfxFiler &F)
{
    INT i;

    CfxControl::DefineProperties(F);

    F.DefineValue("Alignment",    dtInt32,    &_alignment, F_TWO);
    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
    F.DefineValue("MinHeight",    dtInt32,    &_minHeight, F_ZERO);

    F.DefineValue("Digits",       dtByte,     &_digits, "8");
    F.DefineValue("Decimals",     dtByte,     &_decimals, F_ZERO);
    F.DefineValue("Formula",      dtPText,    &_formula);
    F.DefineValue("Fraction",     dtByte,     &_fraction, F_ONE);
    F.DefineValue("Hidden",       dtBoolean,  &_hidden, F_FALSE);
    F.DefineValue("LinkOnlyOnPathEmpty", dtBoolean, &_linkOnlyOnPathEmpty, F_FALSE);
    F.DefineValue("OutputAsDate", dtBoolean,  &_outputAsDate, F_FALSE);
    F.DefineValue("OutputAsInteger", dtBoolean, &_outputAsInteger, F_FALSE);
    F.DefineValue("OutputAsTime", dtBoolean,  &_outputAsTime, F_FALSE);

    F.DefineXOBJECT("ResultElementId", &_resultElementId);
    F.DefineValue("ResultGlobalValue", dtPText, &_resultGlobalValue);

    for (i=0; i<ARRAYSIZE(_elements); i++)
    {
        CHAR varName[32];
        sprintf(varName, "Elements%c", 'A' + (BYTE)i);
        F.DefineXOBJECTLIST(varName, _elements[i]);
    }

    for (i=0; i<ARRAYSIZE(_targetScreenIds); i++)
    {
        CHAR varName[32];
        sprintf(varName, "TargetScreenId%ld", i);
        F.DefineXOBJECT(varName, &_targetScreenIds[i]);
    }

    if (F.IsReader())
    {
        Evaluate();

        if (IsLive())
        {
            SetVisible(!_hidden);
        }
    }
}

VOID CctControl_ElementCalc::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    INT i;

    CHAR items[256];
    sprintf(items, "LIST(Left=%d Center=%d Right=%d)", TEXT_ALIGN_LEFT, TEXT_ALIGN_HCENTER, TEXT_ALIGN_RIGHT);
    F.DefineValue("Alignment",    dtInt32,    &_alignment,   items);

    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Decimals",     dtByte,     &_decimals,    "MIN(0);MAX(8)");
    F.DefineValue("Digits",       dtByte,     &_digits,      "MIN(1);MAX(9)");
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    
    for (i=0; i<ARRAYSIZE(_elements); i++)
    {
        sprintf(items, "Elements %c", 'A' + (BYTE)i);
        F.DefineValue(items, dtPGuidList, _elements[i], "EDITOR(ScreenElementsEditor)");
    }

    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Formula",      dtPText,    &_formula);
    F.DefineValue("Fraction",     dtByte,     &_fraction,   "MIN(1);MAX(9)");
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Hidden",       dtBoolean,  &_hidden);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Minimum height", dtInt32,  &_minHeight,  "MIN(0)");
    F.DefineValue("Link only when no next screen", dtBoolean, &_linkOnlyOnPathEmpty);
    F.DefineValue("Output as date", dtBoolean,  &_outputAsDate);
    F.DefineValue("Output as integer", dtBoolean, &_outputAsInteger);
    F.DefineValue("Output as time", dtBoolean,  &_outputAsTime);
    F.DefineValue("Result Element", dtGuid,   &_resultElementId, "EDITOR(ScreenElementsEditor)");
    F.DefineValue("Result global value", dtPText, &_resultGlobalValue);

    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);

    for (i=0; i<ARRAYSIZE(_targetScreenIds); i++)
    {
        sprintf(items, "Link %d", i);
        F.DefineStaticLink(items, &_targetScreenIds[i]);
    }
#endif
}
