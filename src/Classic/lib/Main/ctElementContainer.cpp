#include "fxApplication.h"
#include "fxUtils.h"
#include "ctElementContainer.h"

//*************************************************************************************************

XGUIDLIST *GetContainerElements(CfxControl *pControl)
{
    CfxControl *control = pControl;

    while (control && !CompareGuid(control->GetTypeId(), &GUID_CONTROL_ELEMENTCONTAINER))
    {
        control = control->GetParent();
    }

    return control != nullptr ? ((CctControl_ElementContainer *)control)->GetElements() : nullptr;
}

//*************************************************************************************************
// CctControl_ElementContainer

CfxControl *Create_Control_ElementContainer(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementContainer(pOwner, pParent);
}

CctControl_ElementContainer::CctControl_ElementContainer(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent), _elements()
{
    InitControl(&GUID_CONTROL_ELEMENTCONTAINER, true);
    InitLockProperties("Elements");
    InitBorder(ControlBorderStyle::Single, 1);
}

void CctControl_ElementContainer::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementContainer::DefineProperties(CfxFiler &F)
{
    //
    // We want the Elements to be available to the control in its own DefineProperties method
    // therefore we populate it before the control DefineProperties (which handles the children).
    //
    F.DefineXOBJECTLIST("Elements", &_elements);

    CfxControl::DefineProperties(F);
}

void CctControl_ElementContainer::DefinePropertiesUI(CfxFilerUI &F) 
{
}

