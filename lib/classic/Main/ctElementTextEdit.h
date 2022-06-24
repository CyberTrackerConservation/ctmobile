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
#include "fxMemo.h"
#include "ctElement.h"
#include "ctSession.h"

const GUID GUID_CONTROL_ELEMENTEDIT = {0x49728018, 0xd9f6, 0x49b8, {0x86, 0xe4, 0x3e, 0xa4, 0x9a, 0x29, 0xf4, 0xbc}};

//
// CctControl_ElementTextEdit
//
class CctControl_ElementTextEdit: public CfxControl_Memo
{
protected:
    XGUID _elementId;
    BOOL _required;
    BOOL _oneLine;
    UINT _maxLength;

    CctRetainState _retainState;

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ElementTextEdit(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementTextEdit();

    VOID SetCaption(const CHAR *Caption);

    VOID ResetState();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPenClick(INT X, INT Y);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
};

extern CfxControl *Create_Control_ElementTextEdit(CfxPersistent *pOwner, CfxControl *pParent);
