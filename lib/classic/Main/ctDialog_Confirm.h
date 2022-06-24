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
#include "fxButton.h"

struct CONFIRM_PARAMS
{
    UINT ControlId;
    CHAR *Title;
    CHAR *Message;
};

//*************************************************************************************************

const GUID GUID_DIALOG_CONFIRM = {0xb84c4568, 0x1e72, 0x49f9, {0x81, 0xad, 0x1b, 0xcb, 0x72, 0x90, 0x24, 0x3d}};

//
// CctDialog_Confirm
//
class CctDialog_Confirm: public CfxDialog
{
protected:
    CONFIRM_PARAMS _source;

    BOOL _result;

    CfxControl_TitleBar *_titleBar;
    CfxControl_Memo *_memo;
    CfxControl_Button *_buttonYes;
    CfxControl_Button *_buttonNo;

    VOID OnYesPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnYesClick(CfxControl *pSender, VOID *pParams);
    VOID OnNoPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnNoClick(CfxControl *pSender, VOID *pParams);
public:
    CctDialog_Confirm(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctDialog_Confirm();

    VOID Init(CfxControl *pSender, VOID *pParam, UINT ParamSize);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnCloseDialog(BOOL *pHandled);
};

extern CfxDialog *Create_Dialog_Confirm(CfxPersistent *pOwner, CfxControl *pParent);
