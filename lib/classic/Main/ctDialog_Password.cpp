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

#include "fxApplication.h"
#include "fxUtils.h"
#include "fxMemo.h"
#include "ctSession.h"
#include "ctActions.h"
#include "ctDialog_Password.h"

#define COLOR_GRAY  0x808080
#define COLOR_WHITE 0xFFFFFF
#define COLOR_BLACK 0x000000

//*************************************************************************************************
// CctDialog_Password

CfxDialog *Create_Dialog_Password(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctDialog_Password(pOwner, pParent);
}

CctDialog_Password::CctDialog_Password(CfxPersistent *pOwner, CfxControl *pParent): CfxDialog(pOwner, pParent)
{
    InitControl(&GUID_DIALOG_PASSWORD);

    _dock = dkFill;
    InitBorder(bsNone, 0);

    _source.ControlId = 0;
    strcpy(_source.Email, "");
    strcpy(_source.Password, "");

    _state = pdsNone;

    _titleBar = new CfxControl_TitleBar(pOwner, this);
    _titleBar->SetComposite(TRUE);
    _titleBar->SetCaption("Login");
    _titleBar->SetShowBattery(FALSE);
    _titleBar->SetShowExit(TRUE);
    _titleBar->SetShowTime(FALSE);
    _titleBar->SetShowOkay(FALSE);
    _titleBar->SetDock(dkTop);
    _titleBar->SetBounds(0, 0, 20, 24);

    _memo = new CfxControl_Memo(pOwner, this);
    _memo->SetComposite(TRUE);
    _memo->SetBorderStyle(bsNone);
    _memo->SetAlignment(TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER | TEXT_ALIGN_WORDWRAP);
    _memo->SetFont(F_FONT_DEFAULT_LB);
    _memo->SetTransparent(TRUE);
    _memo->SetVisible(FALSE);

    // Create buttons
    _buttonEmail = new CfxControl_Button(pOwner, this);
    _buttonEmail->SetComposite(TRUE);
    _buttonEmail->SetAlignment(TEXT_ALIGN_LEFT);
    _buttonEmail->SetFont(F_FONT_DEFAULT_MB);
    _buttonEmail->SetBorder(bsSingle, _oneSpace * 4, _oneSpace, _borderColor);
    _buttonEmail->SetOnClick(this, (NotifyMethod)&CctDialog_Password::OnEmailClick);

    _buttonPassword = new CfxControl_Button(pOwner, this);
    _buttonPassword->SetComposite(TRUE);
    _buttonPassword->SetAlignment(TEXT_ALIGN_LEFT);
    _buttonPassword->SetFont(F_FONT_DEFAULT_MB);
    _buttonPassword->SetBorder(bsSingle, _oneSpace * 4, _oneSpace, _borderColor);
    _buttonPassword->SetOnClick(this, (NotifyMethod)&CctDialog_Password::OnPasswordClick);

    _buttonLogin = new CfxControl_Button(pOwner, this);
    _buttonLogin->SetCaption("Login");
    _buttonLogin->SetComposite(TRUE);
    _buttonLogin->SetBorder(bsSingle, _oneSpace, _oneSpace, _borderColor);
    _buttonLogin->SetOnClick(this, (NotifyMethod)&CctDialog_Password::OnLoginClick);

    _textEditor = NULL;

    if (IsLive())
    {
        GetEngine(this)->AddTransferListener(this);
    }
}

CctDialog_Password::~CctDialog_Password()
{
    delete _textEditor;
    _textEditor = NULL;

    if (IsLive())
    {
        GetEngine(this)->RemoveTransferListener(this);
    }
}

VOID CctDialog_Password::SetState(PASSWORD_DIALOG_STATE NewState)
{
    // Snap the last state.
    switch (_state)
    {
    case pdsEmail:
        strncpy(_source.Email, _textEditor->GetCaption(), ARRAYSIZE(_source.Email));
        break;

    case pdsPassword:
        strncpy(_source.Password, _textEditor->GetCaption(), ARRAYSIZE(_source.Password));
        break;
    }

    // Set color and text defaults.
    delete _textEditor;
    _textEditor = NULL;

    if (_source.Email[0])
    {
        _buttonEmail->SetCaption(_source.Email);
        _buttonEmail->SetTextColor(COLOR_BLACK);
    }
    else
    {
        _buttonEmail->SetCaption("User name");
        _buttonEmail->SetTextColor(COLOR_GRAY);
    }

    if (_source.Password[0])
    {
        CHAR passwordHash[128];
		memset(passwordHash, 0, sizeof(passwordHash));
        for (UINT i = 0; i < strlen(_source.Password); i++)
        {
            passwordHash[i] = '*';
        }
        _buttonPassword->SetCaption(passwordHash);
        _buttonPassword->SetTextColor(COLOR_BLACK);
    }
    else
    {
        _buttonPassword->SetCaption("Password");
        _buttonPassword->SetTextColor(COLOR_GRAY);
    }

    // Move text editor.
    switch (NewState)
    {
    case pdsNone:
        break;

    case pdsEmail:
        _buttonEmail->SetTextColor(COLOR_WHITE);
        Repaint();
        _textEditor = new CfxControl_NativeTextEdit(_owner, _buttonEmail, TRUE);
        _textEditor->SetBorderStyle(bsNone);
        _textEditor->SetDock(dkFill);
        _textEditor->AssignFont(_buttonEmail->GetDefaultFont());
        _textEditor->SetCaption(_source.Email);
        break;

    case pdsPassword:                   
        _buttonPassword->SetTextColor(COLOR_WHITE);
        Repaint();
        _textEditor = new CfxControl_NativeTextEdit(_owner, _buttonPassword, TRUE, TRUE);
        _textEditor->SetBorderStyle(bsNone);
        _textEditor->SetDock(dkFill);
        _textEditor->AssignFont(_buttonPassword->GetDefaultFont());
        _textEditor->SetCaption(_source.Password);
        break;
    }

    _state = NewState;
    GetEngine(this)->AlignControls(GetScreen(this));
    Repaint();

    if (_textEditor)
    {
        _textEditor->SetFocus();
        GetHost(this)->ShowSIP();
    }
}

VOID CctDialog_Password::OnEmailClick(CfxControl *pSender, VOID *pParams)
{
    SetState(pdsEmail);
}

VOID CctDialog_Password::OnPasswordClick(CfxControl *pSender, VOID *pParams)
{
    SetState(pdsPassword);
}

VOID CctDialog_Password::OnLoginClick(CfxControl *pSender, VOID *pParams)
{
    SetState(pdsNone);

    CctSession *session = GetSession(this);
    if (!session->StartLogin(_source.Email, _source.Password))
    {
        OnTransfer(SS_AUTH_FAILURE);
    }
    else
    {
        OnTransfer(SS_AUTH_SUCCESS);
    }
}

VOID CctDialog_Password::OnPenDown(INT X, INT Y)
{
    SetState(pdsNone);
}

VOID CctDialog_Password::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    UINT spacer = (8 * ScaleY) / 100;
    FXRECT rect;
    GetSession(this)->GetClientBounds(&rect);
    INT w = rect.Right - rect.Left + 1;
    INT h = rect.Bottom - rect.Top + 1;
    INT ty = rect.Top + _titleBar->GetHeight();

    _memo->SetBounds(0, ty, w, h / 8);
    ty += _memo->GetHeight();
    _memo->SetVisible(FALSE);

    _buttonEmail->SetBounds(spacer, ty, w - spacer * 2, 4 * spacer);
    ty += _buttonEmail->GetHeight() + spacer;

    _buttonPassword->SetBounds(spacer, ty, w - spacer * 2, 4 * spacer);
    ty += _buttonPassword->GetHeight() + 2 * spacer;

    _buttonLogin->SetBounds(spacer, ty, (w / 2) - spacer, 4 * spacer);
    ty += _buttonLogin->GetHeight();
}

VOID CctDialog_Password::Init(CfxControl *pSender, VOID *pParam, UINT ParamSize)
{ 
    _source = *(PASSWORD_PARAMS *)pParam;
    SetState(pdsNone);
}

VOID CctDialog_Password::OnCloseDialog(BOOL *pHandled)
{
    *pHandled = TRUE;
    GetEngine(this)->Shutdown();
}

VOID CctDialog_Password::OnTransfer(FXSENDSTATEMODE Mode)
{
    switch (Mode)
    {
    case SS_AUTH_START:
        SetEnabled(FALSE);
        Repaint();
        break;

    case SS_AUTH_SUCCESS:
        {
            GetHost(this)->HideSIP();

            CctSession *session = GetSession(this);
            UINT sourceControlId = _source.ControlId;
            PASSWORD_PARAMS result = _source;
	        GUID typeId = _typeId;

            session->SetCredentials(_source.Email, _source.Password);
            session->Reconnect(INVALID_DBID, FALSE);
            CfxControl *sourceControl = session->GetWindow()->FindControl(sourceControlId);
            if (sourceControl)
            {
                sourceControl->OnDialogResult(&typeId, &result);
            }
            session->Repaint();
        }

        break;

    case SS_AUTH_FAILURE:
        GetSession(this)->ShowMessage("Incorrect user name or password");
        SetEnabled(TRUE);
        Repaint();
        break;
    }
}

VOID CctDialog_Password::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (F.DefineBegin("Title"))
    {
        _titleBar->DefineProperties(F);
        F.DefineEnd();
        _titleBar->SetShowExit(TRUE);
    }

    if (F.DefineBegin("Memo"))
    {
        _memo->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Buttons"))
    {
        _buttonEmail->DefineProperties(F);
        _buttonPassword->DefineProperties(F);
        _buttonLogin->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);
    }
}

VOID CctDialog_Password::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    _titleBar->DefineResources(F);
    _memo->DefineResources(F);
    _buttonEmail->DefineResources(F);
    _buttonPassword->DefineResources(F);
    _buttonLogin->DefineResources(F);
#endif
}
