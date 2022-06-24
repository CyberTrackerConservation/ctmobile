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

const GUID GUID_CONTROL_ELEMENTPHONENUMBER = {0x46d640cc, 0xff89, 0x4374, {0x9e, 0x21, 0xed, 0x83, 0xdc, 0xf7, 0xc7, 0x93}};

//
// CctControl_ElementCamera
//
class CctControl_ElementPhoneNumber: public CfxControl_Panel
{
protected:
    XGUID _elementId;
    BOOL _required;
    BOOL _waitForTaken, _pending;
    CHAR *_defaultCaption;

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ElementPhoneNumber(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementPhoneNumber();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPenClick(INT X, INT Y);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID OnPhoneNumberTaken(CHAR *pPhoneNumber, BOOL Success);
};

extern CfxControl *Create_Control_ElementPhoneNumber(CfxPersistent *pOwner, CfxControl *pParent);
