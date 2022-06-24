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

#include "ctElementMemo.h"
#include "fxApplication.h"
#include "fxUtils.h"
#include "ctSession.h"
#include "ctElementContainer.h"

//-------------------------------------------------------------------------------------------------

FXTEXTRESOURCE *LockElementText(CfxControl *pControl, XGUIDLIST *pElements, ElementAttribute Attribute)
{
    FXTEXTRESOURCE *text = NULL;
    CfxResource *resource = pControl->GetResource();
    XGUID *elementId = NULL;

    if (!resource)
    {
        goto Done;
    }

    // Match an element
    if (pElements->GetCount() == 0)
    {
        goto Done;
    }

    if (pControl->IsLive())
    {
        elementId = GetSession(pControl)->GetCurrentSighting()->MatchAttribute(pElements);
    }

    if (!elementId)
    {
        elementId = pElements->GetPtr(0);
    }

    text = (FXTEXTRESOURCE *)resource->GetObject(pControl, *elementId, Attribute);
    if (text)
    {
        FX_ASSERT(text->Magic == MAGIC_TEXT);
    }

Done:

    return text;
}

VOID UnlockElementText(CfxControl *pControl, FXTEXTRESOURCE *pTextResource)
{
    CfxResource *resource = pControl->GetResource();
    if (resource)
    {
        resource->ReleaseObject(pTextResource);
    }
}

VOID GetSightingElementValue(CfxControl *pControl, XGUIDLIST *pElements, CfxString &Value)
{
    XGUID *elementId = NULL;
    ATTRIBUTE *attribute = NULL;
    CctSighting *sighting = NULL;

    Value.Set("");

    if (!pControl->IsLive()) return;

    sighting = GetSession(pControl)->GetCurrentSighting();
    if (!sighting) return;

    elementId = sighting->MatchAttribute(pElements);
    if (!elementId) return;

    attribute = sighting->FindAttribute(elementId);
    if (!attribute) return;

    // If the match is to a null type attribute, then use out the name, because otherwise we only 
    // output "true" which is not meaningful.
    if (attribute->Value->Type == dtNull)
    {
        FXTEXTRESOURCE *text = LockElementText(pControl, pElements, eaName);
        if (text)
        {
            Value.Set(&text->Text[0]);
            UnlockElementText(pControl, text);
        }
    }
    else if (!IsNullXGuid(&attribute->ValueId))
    {
        FXTEXTRESOURCE *text = (FXTEXTRESOURCE *)pControl->GetResource()->GetObject(pControl, attribute->ValueId, eaName);
        if (text)
        {
            Value.Set(&text->Text[0]);
            UnlockElementText(pControl, text);
        }
    }
    else
    {
        Type_VariantToText(attribute->Value, Value);
    }

    if (strncmp(Value.Get(), MEDIA_PREFIX, strlen(MEDIA_PREFIX)) == 0)
    {
        Value.Set("[Media]");
    }
}

//-------------------------------------------------------------------------------------------------

CfxControl *Create_Control_ElementMemo(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementMemo(pOwner, pParent);
}

CctControl_ElementMemo::CctControl_ElementMemo(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Memo(pOwner, pParent), _elements()
{
    InitControl(&GUID_CONTROL_ELEMENTMEMO);
    InitLockProperties("Attribute;Elements;Font");
    _attribute = eaName;
}

XGUIDLIST *CctControl_ElementMemo::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != NULL ? elements : &_elements;
}

VOID CctControl_ElementMemo::UpdateText()
{
    FXTEXTRESOURCE *text = LockElementText(this, GetElements(), _attribute);
    if (text)
    {
        SetCaption(&text->Text[0]);
        UnlockElementText(this, text);
    }
}

VOID CctControl_ElementMemo::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);

    FX_ASSERT((_attribute == eaName) || ((_attribute >= eaText1) && (_attribute <= eaText8)));

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
            F.DefineObject(elements->Get(i), _attribute);
            i++;
        }
    }
#endif
}

VOID CctControl_ElementMemo::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Alignment",    dtInt32,     &_alignment, F_ZERO);
    F.DefineValue("AutoHeight",   dtBoolean,   &_autoHeight, F_FALSE);
    F.DefineValue("MinHeight",    dtInt32,     &_minHeight, "16");
    F.DefineValue("RightToLeft",  dtBoolean,   &_rightToLeft, F_FALSE);
    F.DefineValue("TextColor",    dtColor,     &_textColor, F_COLOR_BLACK);
    F.DefineValue("ScrollBarWidth", dtInt32,   &_scrollBarWidth, "9");

    F.DefineValue("Attribute",    dtInt8,      &_attribute, F_ZERO);
    F.DefineXOBJECTLIST("Elements", &_elements);

    if (F.IsReader())
    {
        UpdateText();
        UpdateScrollBar();
    }
}

VOID CctControl_ElementMemo::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    //
    // We only want to allow selection of eaText1..eaText8
    //
    CHAR items[256];
    sprintf(items, "LIST(Left=%d Center=%d Right=%d)", TEXT_ALIGN_LEFT, TEXT_ALIGN_HCENTER, TEXT_ALIGN_RIGHT);
    F.DefineValue("Alignment",    dtInt32,     &_alignment,   items);

    sprintf(items, "LIST(Name=%d Text1=%d Text2=%d Text3=%d Text4=%d Text5=%d Text6=%d Text7=%d Text8=%d)", eaName, eaText1, eaText2, eaText3, eaText4, eaText5, eaText6, eaText7, eaText8);
    F.DefineValue("Attribute",    dtByte,      &_attribute,   items);

    F.DefineValue("Auto height",  dtBoolean,   &_autoHeight);
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Dock",         dtByte,      &_dock,       "LIST(None Top Bottom Left Right Fill)");
    
    if (GetContainerElements(this) == NULL)
    {
        F.DefineValue("Elements", dtPGuidList, &_elements,   "EDITOR(ScreenElementsEditor);REQUIRED");
    }

    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Minimum height", dtInt32,   &_minHeight, "MIN(0)");
    F.DefineValue("Right to left", dtBoolean,  &_rightToLeft);
    F.DefineValue("Scroll width", dtInt32,     &_scrollBarWidth,    "MIN(7);MAX(40)");
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}

//=================================================================================================

CfxControl *Create_Control_ElementPanel(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementPanel(pOwner, pParent);
}

CctControl_ElementPanel::CctControl_ElementPanel(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent), _elements()
{
    InitControl(&GUID_CONTROL_ELEMENTPANEL);
    InitLockProperties("Attribute;Elements;Font");
    _attribute = eaName;
}

XGUIDLIST *CctControl_ElementPanel::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != NULL ? elements : &_elements;
}

VOID CctControl_ElementPanel::UpdateText()
{
    if (_attribute == (ElementAttribute)0xFF)
    {
        CfxString valueText;
        GetSightingElementValue(this, GetElements(), valueText);

        // Strip from the tab onwards
        CHAR *tabPos = strchr(valueText.Get(), '\t');
        if (tabPos)
        {
            *tabPos = '\0';
        }

        SetCaption(valueText.Get());
        return;
    }

    FXTEXTRESOURCE *text = LockElementText(this, GetElements(), _attribute);
    if (text)
    {
        SetCaption(&text->Text[0]);
        UnlockElementText(this, text);
    }
}

VOID CctControl_ElementPanel::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);

    FX_ASSERT((_attribute == (ElementAttribute)0xFF) || (_attribute == eaName) || ((_attribute >= eaText1) && (_attribute <= eaText8)));

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
            F.DefineObject(elements->Get(i), _attribute == (ElementAttribute)0xFF ? eaName : _attribute);
            i++;
        }
    }
#endif
}

VOID CctControl_ElementPanel::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Alignment",    dtInt32,     &_alignment, F_TWO);
    F.DefineValue("MinHeight",    dtInt32,     &_minHeight, F_ZERO);
    F.DefineValue("TextColor",    dtColor,     &_textColor, F_COLOR_BLACK);

    F.DefineValue("Attribute",    dtInt8,      &_attribute, F_ZERO);
    F.DefineXOBJECTLIST("Elements", &_elements);

    if (F.IsReader())
    {
        UpdateText();
    }
}

VOID CctControl_ElementPanel::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    //
    // We only want to allow selection of eaText1..eaText8
    //
    CHAR items[256];
    sprintf(items, "LIST(Left=%d Center=%d Right=%d)", TEXT_ALIGN_LEFT, TEXT_ALIGN_HCENTER, TEXT_ALIGN_RIGHT);
    F.DefineValue("Alignment",    dtInt32,     &_alignment,   items);

    sprintf(items, "LIST(Name=%d Text1=%d Text2=%d Text3=%d Text4=%d Text5=%d Text6=%d Text7=%d Text8=%d Value=255)", eaName, eaText1, eaText2, eaText3, eaText4, eaText5, eaText6, eaText7, eaText8);
    F.DefineValue("Attribute",    dtByte,      &_attribute,   items);

    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Dock",         dtByte,      &_dock,       "LIST(None Top Bottom Left Right Fill)");
    
    if (GetContainerElements(this) == NULL)
    {
        F.DefineValue("Elements", dtPGuidList, &_elements,   "EDITOR(ScreenElementsEditor);REQUIRED");
    }

    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Minimum height", dtInt32,   &_minHeight, "MIN(0)");
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}
