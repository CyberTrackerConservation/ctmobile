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
#include "ctElementContainer.h"

//*************************************************************************************************

XGUIDLIST *GetContainerElements(CfxControl *pControl)
{
    CfxControl *control = pControl;

    while (control && !CompareGuid(control->GetTypeId(), &GUID_CONTROL_ELEMENTCONTAINER))
    {
        control = control->GetParent();
    }

    return control != NULL ? ((CctControl_ElementContainer *)control)->GetElements() : NULL;
}

//*************************************************************************************************
// CctControl_ElementContainer

CfxControl *Create_Control_ElementContainer(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementContainer(pOwner, pParent);
}

CctControl_ElementContainer::CctControl_ElementContainer(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent), _elements()
{
    InitControl(&GUID_CONTROL_ELEMENTCONTAINER, TRUE);
    InitLockProperties("Elements");
    InitBorder(bsSingle, 1);
}

VOID CctControl_ElementContainer::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);

    UINT i = 0;
    while (i < _elements.GetCount()) 
    {
        if (IsNullXGuid(_elements.GetPtr(i)))
        {
            _elements.Delete(i);
            i = 0;
        }
        else
        {
            F.DefineObject(_elements.Get(i), eaName);
            i++;
        }
    }
#endif
}

VOID CctControl_ElementContainer::DefineProperties(CfxFiler &F)
{
    //
    // We want the Elements to be available to the control in its own DefineProperties method
    // therefore we populate it before the control DefineProperties (which handles the children).
    //
    F.DefineXOBJECTLIST("Elements", &_elements);

    CfxControl::DefineProperties(F);
}

VOID CctControl_ElementContainer::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Elements",     dtPGuidList, &_elements,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

