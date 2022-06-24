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
#include "fxMemo.h"
#include "ctElement.h"


//
// CctControl_ElementMemo
//
const GUID GUID_CONTROL_ELEMENTMEMO = {0xc921c8a, 0xafb7, 0x47fe, {0x90, 0xbd, 0x97, 0xb6, 0x84, 0x59, 0x47, 0x8}};

class CctControl_ElementMemo: public CfxControl_Memo
{
protected:
    XGUIDLIST _elements;
    ElementAttribute _attribute;

    VOID UpdateText();
    XGUIDLIST *GetElements();
public:
    CctControl_ElementMemo(CfxPersistent *pOwner, CfxControl *pParent);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};

extern CfxControl *Create_Control_ElementMemo(CfxPersistent *pOwner, CfxControl *pParent);

//
// CctControl_ElementPanel
//
const GUID GUID_CONTROL_ELEMENTPANEL = {0x465569d0, 0x3a00, 0x4ad5, {0xb4, 0x8d, 0x6b, 0xb7, 0x88, 0xff, 0x3f, 0xa1}};

class CctControl_ElementPanel: public CfxControl_Panel
{
protected:
    XGUIDLIST _elements;
    ElementAttribute _attribute;

    VOID UpdateText();
    XGUIDLIST *GetElements();
public:
    CctControl_ElementPanel(CfxPersistent *pOwner, CfxControl *pParent);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};

extern CfxControl *Create_Control_ElementPanel(CfxPersistent *pOwner, CfxControl *pParent);

