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

#include "fxDialog.h"
#include "fxUtils.h"

// Global dialog registry
CfxDialogRegistry DialogRegistry;

//*************************************************************************************************
// CfxDialogRegistry

CfxDialogRegistry::CfxDialogRegistry(): _dialogs()
{
}

CfxControl *CfxDialogRegistry::CreateRegisteredDialog(const GUID *pTypeId, CfxPersistent *pOwner, CfxControl *pParent)
{
    for (UINT i=0; i<_dialogs.GetCount(); i++)
    {
        GUID value = _dialogs.Get(i).TypeId;
        if (CompareGuid(pTypeId, &value))
        {
            CfxControl *control = _dialogs.Get(i).FnFactory(pOwner, pParent);
            //control->SetId(2);
            return control;
        }
    }

    return NULL;
}

VOID CfxDialogRegistry::RegisterDialog(const GUID *pTypeId, Fn_Dialog_Factory FnFactory)
{
    RegisteredDialog dialog;
	dialog.TypeId = *pTypeId;
	dialog.FnFactory = FnFactory;
    _dialogs.Add(dialog);

    ControlRegistry.RegisterControl(pTypeId, "Dialog", (Fn_Control_Factory)FnFactory, FALSE, 0);
}

BOOL CfxDialogRegistry::EnumRegisteredDialogs(UINT Index, GUID *pTypeId)
{
    if (Index >= _dialogs.GetCount()) 
    {
        return FALSE;
    }

    *pTypeId = _dialogs.Get(Index).TypeId;
    return TRUE;
}

