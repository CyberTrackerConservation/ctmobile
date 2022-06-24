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

const GUID GUID_CONTROL_ELEMENTCONTAINER = {0x9418a58, 0x7e96, 0x4e83, {0xb4, 0x34, 0x4, 0x45, 0x21, 0xc3, 0x48, 0x6a}};

//
// CctControl_ElementContainer
//
class CctControl_ElementContainer: public CfxControl
{
protected:
    XGUIDLIST _elements;
public:
    CctControl_ElementContainer(CfxPersistent *pOwner, CfxControl *pParent);

    XGUIDLIST *GetElements()    { return &_elements; }

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};

extern CfxControl *Create_Control_ElementContainer(CfxPersistent *pOwner, CfxControl *pParent);

// Utility API
extern XGUIDLIST *GetContainerElements(CfxControl *pControl);
