#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "ctElement.h"

const GUID GUID_CONTROL_ELEMENTCALC = {0xc26aca43, 0x8c7c, 0x497c, {0x95, 0x86, 0x38, 0x2e, 0x5b, 0xd2, 0x71, 0x15}};

//
// CctControl_ElementCalc
//
class CctControl_ElementCalc: public CfxControl_Panel
{
protected:
    XGUIDLIST *_elements[26] = {};
    XGUID _resultElementId = 0;

    char *_formula = nullptr;
    BYTE _digits = 0, _decimals = 0, _fraction = 0;

    bool _outputAsTime = false, _outputAsDate = false, _outputAsInteger = false;

    char *_resultGlobalValue = nullptr;

    bool _evaluateOnPaint = false;

    bool _hidden = false;
    bool _linkOnlyOnPathEmpty = false;
    XGUID _targetScreenIds[16] = {};

    double CalcValue(XGUIDLIST *pElements);
    double Evaluate();
    void OnSessionNext(CfxControl *pSender, void *pParams);
    void OnGlobalChanged(CfxControl *pSender, void *pParams);
public:
    CctControl_ElementCalc(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementCalc();

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_ElementCalc(CfxPersistent *pOwner, CfxControl *pParent);

