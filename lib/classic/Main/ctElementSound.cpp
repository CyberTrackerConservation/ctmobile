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
#include "ctElementSound.h"
#include "ctSighting.h"
#include "ctSession.h"
#include "ctElementContainer.h"

CfxControl *Create_Control_ElementSound(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementSound(pOwner, pParent);
}

CctControl_ElementSound::CctControl_ElementSound(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_SoundBase(pOwner, pParent), _elements()
{
    InitControl(&GUID_CONTROL_ELEMENTSOUND);
    InitLockProperties("Elements");
}

XGUIDLIST *CctControl_ElementSound::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != NULL ? elements : &_elements;
}

VOID CctControl_ElementSound::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_SoundBase::DefineResources(F);

    XGUIDLIST *elements = GetElements();

    UINT i = 0;
    while (i < elements->GetCount()) 
    {
        if (IsNullXGuid(elements->GetPtr(i)))
        {
            elements->Delete(i);
            i = 0;
        }
        else
        {
            F.DefineObject(elements->Get(i), eaSound);
            i++;
        }
    }
#endif
}

VOID CctControl_ElementSound::DefineProperties(CfxFiler &F)
{
    CfxControl_SoundBase::DefineProperties(F);
    F.DefineXOBJECTLIST("Elements", &_elements);
}

VOID CctControl_ElementSound::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color",  dtColor,   &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style",  dtInt8,    &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width",  dtInt32,   &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button border", dtInt32,   &_buttonBorderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Button border line width", dtInt32, &_buttonBorderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Button border width", dtInt32, &_buttonBorderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button width",  dtInt32,   &_buttonWidth, "MIN(20) MAX(80)");
    F.DefineValue("Color",         dtColor,   &_color);
    F.DefineValue("Dock",          dtByte,    &_dock,        "LIST(None Top Bottom Left Right Fill)");
    if (GetContainerElements(this) == NULL)
    {
        F.DefineValue("Elements",     dtPGuidList, &_elements,   "EDITOR(ScreenElementsEditor);REQUIRED");
    }
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Show play",    dtBoolean,  &_showPlay);
    F.DefineValue("Show stop",    dtBoolean,  &_showStop);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

FXSOUNDRESOURCE *CctControl_ElementSound::GetSound()
{
    XGUIDLIST *elements = GetElements();
    if (elements->GetCount() == 0) return NULL;

    // Match an element
    XGUID *elementId = NULL;
    if (IsLive())
    {
        elementId = GetSession(this)->GetCurrentSighting()->MatchAttribute(elements);
    }
    if (!elementId)
    {
        elementId = elements->GetPtr(0);
    }

    return (FXSOUNDRESOURCE *)GetResource()->GetObject(this, *elementId, eaSound);
}

VOID CctControl_ElementSound::ReleaseSound(FXSOUNDRESOURCE *pSound)
{
    GetResource()->ReleaseObject(pSound);
}

