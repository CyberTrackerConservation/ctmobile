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
#include "fxTitleBar.h"
#include "ctElementKeypad.h"

struct NUMBEREDITOR_PARAMS
{
    UINT ControlId;
    UINT Index;
    BYTE Digits;
    BYTE Decimals;
    BYTE Fraction;
    DOUBLE Value;
    INT MinValue, MaxValue;
    BOOL FormulaMode;
    CHAR Title[64];
};

//*************************************************************************************************

const GUID GUID_DIALOG_NUMBEREDITOR = {0x62b46c4c, 0xf815, 0x45b1, {0xb0, 0x9a, 0x6a, 0x1f, 0x5b, 0x7a, 0xb0, 0x53}};

//
// CctDialog_NumberEditor
//
class CctDialog_NumberEditor: public CfxDialog
{
protected:
    NUMBEREDITOR_PARAMS _source;

    CfxControl_TitleBar *_titleBar;
    CctControl_ElementKeypad *_keypad;
public:
    CctDialog_NumberEditor(CfxPersistent *pOwner, CfxControl *pParent);

    VOID Init(CfxControl *pSender, VOID *pParam, UINT ParamSize);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnCloseDialog(BOOL *pHandled);
};

extern CfxDialog *Create_Dialog_NumberEditor(CfxPersistent *pOwner, CfxControl *pParent);
