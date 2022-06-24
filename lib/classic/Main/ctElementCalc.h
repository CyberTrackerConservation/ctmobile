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
#include "fxPanel.h"
#include "ctElement.h"

const GUID GUID_CONTROL_ELEMENTCALC = {0xc26aca43, 0x8c7c, 0x497c, {0x95, 0x86, 0x38, 0x2e, 0x5b, 0xd2, 0x71, 0x15}};

//
// CctControl_ElementCalc
//
class CctControl_ElementCalc: public CfxControl_Panel
{
protected:
    XGUIDLIST *_elements[26];
    XGUID _resultElementId;

    CHAR *_formula;
    BYTE _digits, _decimals, _fraction;

    BOOL _outputAsTime, _outputAsDate, _outputAsInteger;

    CHAR *_resultGlobalValue;

    BOOL _evaluateOnPaint;

    BOOL _hidden;
    BOOL _linkOnlyOnPathEmpty;
    XGUID _targetScreenIds[16];

    DOUBLE CalcValue(XGUIDLIST *pElements);
    DOUBLE Evaluate();
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
    VOID OnGlobalChanged(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ElementCalc(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementCalc();

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};

extern CfxControl *Create_Control_ElementCalc(CfxPersistent *pOwner, CfxControl *pParent);

