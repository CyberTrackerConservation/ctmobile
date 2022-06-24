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
#include "ctDialog_ComPortSelect.h"

//*************************************************************************************************
// CctDialog_PortSelect

CfxDialog *Create_Dialog_PortSelect(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctDialog_PortSelect(pOwner, pParent);
}

CctDialog_PortSelect::CctDialog_PortSelect(CfxPersistent *pOwner, CfxControl *pParent): CfxDialog(pOwner, pParent)
{
    InitControl(&GUID_DIALOG_PORTSELECT);

    _dock = dkFill;

    _source.ControlId = 0;
    _result = 0;

    _titleBar = new CfxControl_TitleBar(pOwner, this);
    _titleBar->SetComposite(TRUE);
    _titleBar->SetCaption("Select port");
    _titleBar->SetShowBattery(FALSE);
    _titleBar->SetShowExit(FALSE);
    _titleBar->SetShowTime(FALSE);
    _titleBar->SetShowOkay(TRUE);
    _titleBar->SetDock(dkTop);
    _titleBar->SetBounds(0, 0, 20, 24);

    _spacer = new CfxControl_Panel(pOwner, this);
    _spacer->SetComposite(TRUE);
    _spacer->SetBorderStyle(bsNone);
    _spacer->SetDock(dkTop);
    _spacer->SetBounds(0, 1, 24, 4);

    _comPortList = new CctControl_ComPortList(pOwner, this);
    _comPortList->SetComposite(TRUE);
    _comPortList->SetBorderStyle(bsSingle);
    _comPortList->SetDock(dkFill);
    _comPortList->SetFont(F_FONT_DEFAULT_LB);
    _comPortList->SetTransparent(TRUE);
}

CctDialog_PortSelect::~CctDialog_PortSelect()
{
}

VOID CctDialog_PortSelect::Init(CfxControl *pSender, VOID *pParam, UINT ParamSize)
{ 
    _source = *(PORTSELECT_PARAMS *)pParam;
    FX_ASSERT(_source.ControlId > 0);

    _titleBar->SetCaption("Select port");

    _comPortList->SetPortId(_source.PortId);
    _result = _source.PortId;
}

VOID CctDialog_PortSelect::OnCloseDialog(BOOL *pHandled)
{
    *pHandled = TRUE;

    UINT sourceControlId = _source.ControlId;
	GUID typeId = _typeId;

    VOID *result = (VOID *)(intptr_t)_comPortList->GetPortId();

    CctSession *session = GetSession(this);
    session->Reconnect(INVALID_DBID, FALSE);
    CfxControl *sourceControl = session->GetWindow()->FindControl(sourceControlId);
    if (sourceControl)
    {
        sourceControl->OnDialogResult(&typeId, result);
    }

    session->Repaint();
}

VOID CctDialog_PortSelect::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (F.DefineBegin("Title"))
    {
        _titleBar->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("ComPortList"))
    {
        _comPortList->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);
    }
}

VOID CctDialog_PortSelect::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    _titleBar->DefineResources(F);
    _comPortList->DefineResources(F);
#endif
}
