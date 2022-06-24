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
#include "fxControl.h"

// 
// CfxDialogRegistry
//
typedef CfxDialog* (*Fn_Dialog_Factory)(CfxPersistent *pOwner, CfxControl *pParent);

struct RegisteredDialog
{
    GUID TypeId;
    Fn_Dialog_Factory FnFactory;
};

class CfxDialogRegistry 
{
protected:
    TList<RegisteredDialog> _dialogs;
public:
    CfxDialogRegistry();
    CfxControl *CreateRegisteredDialog(const GUID *pTypeId, CfxPersistent *pOwner, CfxControl *pParent);
    VOID RegisterDialog(const GUID *pTypeId, Fn_Dialog_Factory FnFactory);
    BOOL EnumRegisteredDialogs(UINT Index, GUID *pTypeId);
};

// Global registry
extern CfxDialogRegistry DialogRegistry;

