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
#include "ctElement.h"
#include "ctDialog_NumberEditor.h"
#include "ctElementList.h"

//*************************************************************************************************
// CctDialog_NumberEditor

CfxDialog *Create_Dialog_NumberEditor(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctDialog_NumberEditor(pOwner, pParent);
}

CctDialog_NumberEditor::CctDialog_NumberEditor(CfxPersistent *pOwner, CfxControl *pParent): CfxDialog(pOwner, pParent)
{
    InitControl(&GUID_DIALOG_NUMBEREDITOR);

    _dock = dkFill;

    memset(&_source, 0, sizeof(_source));

    _titleBar = new CfxControl_TitleBar(pOwner, this);
    _titleBar->SetComposite(TRUE);
    _titleBar->SetCaption("Number Editor");
    _titleBar->SetShowBattery(FALSE);
    _titleBar->SetShowExit(FALSE);
    _titleBar->SetShowTime(FALSE);
    _titleBar->SetShowOkay(TRUE);
    _titleBar->SetDock(dkTop);
    _titleBar->SetBounds(0, 0, 20, 24);

    _keypad = new CctControl_ElementKeypad(pOwner, this);
    _keypad->SetComposite(TRUE);
    _keypad->SetBorderStyle(bsNone);
    _keypad->SetBorderWidth(0);
    _keypad->SetDock(dkFill);
    _keypad->SetupDisplay(60, 0xFFFFFF);
    _keypad->SetupButtons(0, 0, 1, bsSingle, 0xFFFFFF, 0);
    _keypad->SetButtonFont(F_FONT_DEFAULT_XXLB);
    _keypad->SetFont(F_FONT_DEFAULT_XXLB);
}

VOID CctDialog_NumberEditor::Init(CfxControl *pSender, VOID *pParam, UINT ParamSize)
{ 
    _source = *(NUMBEREDITOR_PARAMS *)pParam;
    FX_ASSERT(_source.ControlId > 0);

    _titleBar->SetCaption(_source.Title);

    if (_source.FormulaMode)
    {
        _keypad->SetupDisplay(50, 0xFFFFFF);
        _keypad->SetupButtons(0, 0, 1, bsSingle, 0xFFFFFF, 0);
    }
    else
    {
        _keypad->SetupDisplay(60, 0xFFFFFF);
        _keypad->SetupButtons(0, 0, 1, bsSingle, 0xFFFFFF, 0);
    }

    _keypad->Setup(_source.Digits, _source.Decimals, _source.Fraction, _source.MinValue, _source.MaxValue, _source.FormulaMode, FALSE);
    _keypad->SetValue(_source.Value);
}

VOID CctDialog_NumberEditor::OnCloseDialog(BOOL *pHandled)
{
    CctSession *session = GetSession(this);

    *pHandled = TRUE;

    if (_keypad->FinalizeValue() && _keypad->TestValueValid())
    {
        UINT sourceControlId = _source.ControlId;
        UINT sourceIndex = _source.Index;
        INT value, valuePoint;
        BOOL valueNegative;

        DecodeDouble(_keypad->GetValue(), _source.Decimals, &value, &valuePoint, &valueNegative);

        if (valueNegative)
        {
            value = -value;
        }

        session->Reconnect(INVALID_DBID, FALSE);

        CctControl_ElementList *sourceControl = (CctControl_ElementList *)(session->GetWindow()->FindControl(sourceControlId));
        if (sourceControl)
        {
            sourceControl->SetItemValue(sourceIndex, value);
        }
    }

    session->Repaint();
}

VOID CctDialog_NumberEditor::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (F.DefineBegin("Title"))
    {
        _titleBar->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Keypad"))
    {
        _keypad->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);
    }
}

VOID CctDialog_NumberEditor::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    _titleBar->DefineResources(F);
    _keypad->DefineResources(F);
#endif
}

