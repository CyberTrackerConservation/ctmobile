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
#include "fxPanel.h"
#include "fxButton.h"
#include "fxNativeTextEdit.h"

struct PASSWORD_PARAMS
{
    UINT ControlId;
    CHAR Email[128];
    CHAR Password[128];
};

//*************************************************************************************************

const GUID GUID_DIALOG_PASSWORD = {0x9f5c7d4d, 0x90ca, 0x4a21, {0x97, 0x66, 0x32, 0x4c, 0x3c, 0x7f, 0xfb, 0x1b}};

enum PASSWORD_DIALOG_STATE
{
    pdsNone = 0,
    pdsEmail,
    pdsPassword
};

//
// CctDialog_Password
//
class CctDialog_Password: public CfxDialog
{
protected:
    PASSWORD_PARAMS _source;

    PASSWORD_DIALOG_STATE _state;
    
    CfxControl_TitleBar *_titleBar;
    CfxControl_Memo *_memo;
    CfxControl_Button *_buttonEmail, *_buttonPassword;
    CfxControl_Button *_buttonLogin;
    CfxControl_NativeTextEdit *_textEditor;

    VOID SetState(PASSWORD_DIALOG_STATE NewState);

    VOID OnEmailClick(CfxControl *pSender, VOID *pParams);
    VOID OnPasswordClick(CfxControl *pSender, VOID *pParams);
    VOID OnLoginClick(CfxControl *pSender, VOID *pParams);
public:
    CctDialog_Password(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctDialog_Password();

	VOID OnPenDown(INT X, INT Y);

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    VOID Init(CfxControl *pSender, VOID *pParam, UINT ParamSize);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnCloseDialog(BOOL *pHandled);

    VOID OnTransfer(FXSENDSTATEMODE Mode);
};

extern CfxDialog *Create_Dialog_Password(CfxPersistent *pOwner, CfxControl *pParent);
