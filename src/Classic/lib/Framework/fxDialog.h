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
    Fn_Dialog_Factory FnFactory = nullptr;
};

class CfxDialogRegistry 
{
protected:
    TList<RegisteredDialog> _dialogs;
public:
    CfxDialogRegistry();
    CfxControl *CreateRegisteredDialog(const GUID *pTypeId, CfxPersistent *pOwner, CfxControl *pParent);
    void RegisterDialog(const GUID *pTypeId, Fn_Dialog_Factory FnFactory);
    bool EnumRegisteredDialogs(int Index, GUID *pTypeId);
};

// Global registry
extern CfxDialogRegistry DialogRegistry;

