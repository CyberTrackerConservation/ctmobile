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
    int i;

    InitControl(&GUID_CONTROL_ELEMENTCALC);
    InitLockProperties("Elements A;Elements B;Elements C;Font;Formula;Result Element");

    InitXGuid(&_resultElementId);

    _linkOnlyOnPathEmpty = false;
    for (i=0; i<ARRAYSIZE(_targetScreenIds); i++)
    {
        InitXGuid(&_targetScreenIds[i]);
    }

    for (i=0; i<ARRAYSIZE(_elements); i++)
    {
        _elements[i] = new XGUIDLIST();
    }

    _formula = nullptr;
    _digits = 8;
    _decimals = 0;
    _fraction = 1;
    _outputAsTime = _outputAsDate = _outputAsInteger = false;

    _resultGlobalValue = nullptr;

    _hidden = false;

    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementCalc::OnSessionNext);
    RegisterSessionEvent(seGlobalChanged, this, (NotifyMethod)&CctControl_ElementCalc::OnGlobalChanged);
}

CctControl_ElementCalc::~CctControl_ElementCalc()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seGlobalChanged, this);

    for (int i=0; i<ARRAYSIZE(_elements); i++)
    {
        delete _elements[i];
        _elements[i] = nullptr;
    }

    MEM_FREE(_formula);
    _formula = nullptr;

    MEM_FREE(_resultGlobalValue);
	_resultGlobalValue = nullptr;
}

void CctControl_ElementCalc::OnGlobalChanged(CfxControl *pSender, void *pParams)
{
    Evaluate();
    Repaint();
}

void CctControl_ElementCalc::OnSessionNext(CfxControl *pSender, void *pParams)
{
    double value = Evaluate();

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
            int ivalue = (int)value;
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
    if (_resultGlobalValue != nullptr)
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

    for (int i=0; i<ARRAYSIZE(_targetScreenIds); i++)
    {
        if (((int)value == i) && !IsNullXGuid(&_targetScreenIds[i]))
        {
            CctAction_InsertScreen action(this);
            action.Setup(_targetScreenIds[i]);
            ((CctSession *)pSender)->ExecuteAction(&action);
        }
    }
}

double CctControl_ElementCalc::CalcValue(XGUIDLIST *pElements)
{
    if (!IsLive())
    {
        return 0;
    }

    ATTRIBUTES *attributes = GetSession(this)->GetCurrentAttributes();
    double value = 0;

    for (int i=0; i<attributes->GetCount(); i++)
    {
        ATTRIBUTE *attribute = attributes->GetPtr(i);
        if (pElements->IndexOf(attribute->ElementId) != -1)
        {
            value += Type_VariantToDouble(attribute->Value);
        }
    }

    return value;
}

double CctControl_ElementCalc::Evaluate()
{
    if (_fraction > 1) 
    {
        _decimals = 3;
    }
    
    CfxExpressionParser parser(_formula);

    for (int i=0; i<ARRAYSIZE(_elements); i++)
    {
        char sym[2] = "A";
        sym[0] += (char)i;
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
        int i = 0;
        GLOBALVALUE *globalValue = session->EnumGlobalValues(0);
        while (globalValue)
        {
            parser.SetSymbol(globalValue->Name, globalValue->Value);
            i++;
            globalValue = session->EnumGlobalValues(i);
        }
    }
   
    // Run the calculation
    double dvalue = FxAbs(parser.Evaluate());

    if (parser.GetParseError())
    {
        dvalue = 0;
        if (IsLive())
        {
            SetCaption(_formula == nullptr ? "No formula" : "Bad formula");
        }
        else
        {
            SetCaption(_formula == nullptr ? "Set \"Formula\" property" : "Bad formula");
        }
    }
    else
    {
        if (_outputAsDate)
        {
            QString valueText;
            Type_DataToText(dtDate, &dvalue, valueText);
            SetCaption(valueText.toUtf8().constData());
        }
        else if (_outputAsInteger)
        {
            QString valueText;
            int ivalue = (int)dvalue;
            Type_DataToText(dtInt32, &ivalue, valueText);
            SetCaption(valueText.toUtf8().constData());
        }
        else if (_outputAsTime)
        {
            QString valueText;
            Type_DataToText(dtTime, &dvalue, valueText);
            SetCaption(valueText.toUtf8().constData());
        }
        else
        {
            char valueText[32];
            int ivalue, pvalue;
            bool inegative;
            DecodeDouble(dvalue, _decimals, &ivalue, &pvalue, &inegative);
            NumberToText(ivalue, _digits, _decimals, _fraction, inegative, valueText, false);
            SetCaption(valueText);
        }
    }

    return dvalue;
}

void CctControl_ElementCalc::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementCalc::DefineProperties(CfxFiler &F)
{
    int i;

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
        char varName[32];
        sprintf(varName, "Elements%c", 'A' + (BYTE)i);
        F.DefineXOBJECTLIST(varName, _elements[i]);
    }

    for (i=0; i<ARRAYSIZE(_targetScreenIds); i++)
    {
        char varName[32];
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

void CctControl_ElementCalc::DefinePropertiesUI(CfxFilerUI &F) 
{
}
