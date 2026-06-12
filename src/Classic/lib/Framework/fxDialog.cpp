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
    for (int i=0; i<_dialogs.GetCount(); i++)
    {
        GUID value = _dialogs.Get(i).TypeId;
        if (CompareGuid(pTypeId, &value))
        {
            CfxControl *control = _dialogs.Get(i).FnFactory(pOwner, pParent);
            //control->SetId(2);
            return control;
        }
    }

    return nullptr;
}

void CfxDialogRegistry::RegisterDialog(const GUID *pTypeId, Fn_Dialog_Factory FnFactory)
{
    RegisteredDialog dialog;
	dialog.TypeId = *pTypeId;
	dialog.FnFactory = FnFactory;
    _dialogs.Add(dialog);

    ControlRegistry.RegisterControl(pTypeId, "Dialog", (Fn_Control_Factory)FnFactory, false, 0);
}

bool CfxDialogRegistry::EnumRegisteredDialogs(int Index, GUID *pTypeId)
{
    if (Index >= _dialogs.GetCount()) 
    {
        return false;
    }

    *pTypeId = _dialogs.Get(Index).TypeId;
    return true;
}

