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
#include "ctDialog_Confirm.h"

//*************************************************************************************************
// CctDialog_Confirm

CfxDialog *Create_Dialog_Confirm(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctDialog_Confirm(pOwner, pParent);
}

CctDialog_Confirm::CctDialog_Confirm(CfxPersistent *pOwner, CfxControl *pParent): CfxDialog(pOwner, pParent)
{
    InitControl(&GUID_DIALOG_CONFIRM);

    _dock = dkFill;
    InitBorder(bsNone, 0);

    _source.ControlId = 0;
    _result = FALSE;

    _titleBar = new CfxControl_TitleBar(pOwner, this);
    _titleBar->SetComposite(TRUE);
    _titleBar->SetCaption("Confirm");
    _titleBar->SetShowBattery(FALSE);
    _titleBar->SetShowExit(FALSE);
    _titleBar->SetShowTime(FALSE);
    _titleBar->SetShowOkay(TRUE);
    _titleBar->SetDock(dkTop);
    _titleBar->SetBounds(0, 0, 20, 24);

    _memo = new CfxControl_Memo(pOwner, this);
    _memo->SetComposite(TRUE);
    _memo->SetBorderStyle(bsNone);
    _memo->SetAlignment(TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER | TEXT_ALIGN_WORDWRAP);
    _memo->SetFont(F_FONT_DEFAULT_LB);
    _memo->SetTransparent(TRUE);

    // Create buttons
    _buttonYes = new CfxControl_Button(pOwner, this);
    _buttonYes->SetComposite(TRUE);
    _buttonYes->SetBorder(bsRound2, 1, 1, _borderColor);
    _buttonYes->SetOnClick(this, (NotifyMethod)&CctDialog_Confirm::OnYesClick);
    _buttonYes->SetOnPaint(this, (NotifyMethod)&CctDialog_Confirm::OnYesPaint);

    _buttonNo = new CfxControl_Button(pOwner, this);
    _buttonNo->SetComposite(TRUE);
    _buttonNo->SetBorder(bsRound2, 1, 1, _borderColor);
    _buttonNo->SetOnClick(this, (NotifyMethod)&CctDialog_Confirm::OnNoClick);
    _buttonNo->SetOnPaint(this, (NotifyMethod)&CctDialog_Confirm::OnNoPaint);
}

CctDialog_Confirm::~CctDialog_Confirm()
{
}

VOID CctDialog_Confirm::OnYesPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, _buttonYes->GetDown());
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -8, -8);
    pParams->Canvas->DrawCheck(&rect, _color, _borderColor, _buttonYes->GetDown());
}

VOID CctDialog_Confirm::OnYesClick(CfxControl *pSender, VOID *pParams)
{
    _result = TRUE;
    BOOL handled;
    OnCloseDialog(&handled);
}

VOID CctDialog_Confirm::OnNoPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, _buttonNo->GetDown());
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -8, -8);
    pParams->Canvas->DrawCross(&rect, _color, _borderColor, 2, _buttonNo->GetDown());
}

VOID CctDialog_Confirm::OnNoClick(CfxControl *pSender, VOID *pParams)
{
    _result = FALSE;
    BOOL handled;
    OnCloseDialog(&handled);
}

VOID CctDialog_Confirm::Init(CfxControl *pSender, VOID *pParam, UINT ParamSize)
{ 
    _result = FALSE;
    _source = *(CONFIRM_PARAMS *)pParam;
    FX_ASSERT(_source.ControlId > 0);

    _titleBar->SetCaption(_source.Title);
    _memo->SetCaption(_source.Message);

    FXRECT rect;
    GetSession(this)->GetClientBounds(&rect);
    INT w = rect.Right - rect.Left + 1;
    INT h = rect.Bottom - rect.Top + 1;
    INT ty = rect.Top + _titleBar->GetHeight() * _oneSpace;

    _memo->SetBounds(0, ty, w, h / 2);
    ty += _memo->GetHeight();

    _buttonYes->SetBounds(w / 9, ty, w / 3, h / 4);
    _buttonNo->SetBounds((w * 5) / 9, ty, w / 3, h / 4);

    GetEngine(this)->AlignControls(GetScreen(this));
}

VOID CctDialog_Confirm::OnCloseDialog(BOOL *pHandled)
{
    *pHandled = TRUE;

    UINT sourceControlId = _source.ControlId;
    VOID *result = (VOID *)(intptr_t)_result;
	GUID typeId = _typeId;

    CctSession *session = GetSession(this);
    session->Reconnect(INVALID_DBID, FALSE);
    CfxControl *sourceControl = session->GetWindow()->FindControl(sourceControlId);
    if (sourceControl)
    {
        sourceControl->OnDialogResult(&typeId, result);
    }

    session->Repaint();
}

VOID CctDialog_Confirm::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (F.DefineBegin("Title"))
    {
        _titleBar->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Memo"))
    {
        _memo->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);
    }
}

VOID CctDialog_Confirm::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    _titleBar->DefineResources(F);
#endif
}
