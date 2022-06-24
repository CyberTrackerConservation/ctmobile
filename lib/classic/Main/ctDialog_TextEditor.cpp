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
#include "ctDialog_TextEditor.h"

//*************************************************************************************************
// CctDialog_TextEditor

CfxDialog *Create_Dialog_TextEditor(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctDialog_TextEditor(pOwner, pParent);
}

CctDialog_TextEditor::CctDialog_TextEditor(CfxPersistent *pOwner, CfxControl *pParent): CfxDialog(pOwner, pParent)
{
    InitControl(&GUID_DIALOG_TEXTEDITOR);

    _dock = dkFill;

    _source.ControlId = 0;
    _source.Text = 0;

    _titleBar = new CfxControl_TitleBar(pOwner, this);
    _titleBar->SetComposite(TRUE);
    _titleBar->SetCaption("Text Editor");
    _titleBar->SetShowBattery(FALSE);
    _titleBar->SetShowExit(FALSE);
    _titleBar->SetShowTime(FALSE);
    _titleBar->SetShowOkay(TRUE);
    _titleBar->SetDock(dkTop);
    _titleBar->SetBounds(0, 0, 20, 24);

    _editor = new CfxControl_NativeTextEdit(pOwner, this);
    _editor->SetComposite(TRUE);
    _editor->SetBorderStyle(bsNone);
    _editor->SetBorderWidth(0);
    _editor->SetDock(dkFill);

    if (IsLive())
    {
        GetHost(this)->ShowSIP();
    }
}

CctDialog_TextEditor::~CctDialog_TextEditor()
{
    if (IsLive())
    {
        GetHost(this)->HideSIP();
    }
}

VOID CctDialog_TextEditor::Init(CfxControl *pSender, VOID *pParam, UINT ParamSize)
{ 
    _source = *(TEXTEDITOR_PARAMS *)pParam;
    FX_ASSERT(_source.ControlId > 0);

    _editor->AssignFont(&_source.Font);
    _editor->SetCaption(_source.Text);

    MEM_FREE(_source.Text);
    _source.Text = 0;
}

VOID CctDialog_TextEditor::OnCloseDialog(BOOL *pHandled)
{
    *pHandled = TRUE;

    CHAR *clonedText = AllocString(_editor->GetCaption());
    TrimString(clonedText);

    UINT sourceControlId = _source.ControlId;
    CctSession *session = GetSession(this);
    session->Reconnect(INVALID_DBID, FALSE);

    CfxControl_Memo *sourceControl = (CfxControl_Memo *)(session->GetWindow()->FindControl(sourceControlId));
    if (sourceControl)
    {
        sourceControl->SetCaption(clonedText);
    }
    MEM_FREE(clonedText);

    session->Repaint();
}

VOID CctDialog_TextEditor::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (F.DefineBegin("Title"))
    {
        _titleBar->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Editor"))
    {
        _editor->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);
    }
}

VOID CctDialog_TextEditor::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    _titleBar->DefineResources(F);
    _editor->DefineResources(F);
#endif
}

VOID CctDialog_TextEditor::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    _editor->SetFocus();
}

