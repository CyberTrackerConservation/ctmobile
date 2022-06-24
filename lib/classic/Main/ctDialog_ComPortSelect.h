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
#include "fxMemo.h"
#include "ctComPortList.h"

struct PORTSELECT_PARAMS
{
    UINT ControlId;
    BYTE PortId;
};

//*************************************************************************************************

const GUID GUID_DIALOG_PORTSELECT = {0x960b2491, 0xb6f0, 0x40f6, {0x81, 0x35, 0xa1, 0x78, 0xda, 0x5d, 0x93, 0xff}};

//
// CctDialog_PortSelect
//
class CctDialog_PortSelect: public CfxDialog
{
protected:
    PORTSELECT_PARAMS _source;

    BYTE _result;

    CfxControl_TitleBar *_titleBar;
    CfxControl_Panel *_spacer;
    CctControl_ComPortList *_comPortList;
public:
    CctDialog_PortSelect(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctDialog_PortSelect();

    VOID Init(CfxControl *pSender, VOID *pParam, UINT ParamSize);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnCloseDialog(BOOL *pHandled);
};

extern CfxDialog *Create_Dialog_PortSelect(CfxPersistent *pOwner, CfxControl *pParent);
