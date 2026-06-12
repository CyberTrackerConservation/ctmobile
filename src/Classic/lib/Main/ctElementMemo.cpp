#include "ctElementMemo.h"
#include "fxApplication.h"
#include "fxUtils.h"
#include "ctSession.h"
#include "ctElementContainer.h"

//-------------------------------------------------------------------------------------------------

FXTEXTRESOURCE *LockElementText(CfxControl *pControl, XGUIDLIST *pElements, ElementAttribute Attribute)
{
    FXTEXTRESOURCE *text = nullptr;
    CfxResource *resource = pControl->GetResource();
    XGUID *elementId = nullptr;

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

    text = (FXTEXTRESOURCE *)resource->GetObject(pControl, *elementId, (BYTE)Attribute);
    if (text)
    {
        FX_ASSERT(text->Magic == MAGIC_TEXT);
    }

Done:

    return text;
}

void UnlockElementText(CfxControl *pControl, FXTEXTRESOURCE *pTextResource)
{
    CfxResource *resource = pControl->GetResource();
    if (resource)
    {
        resource->ReleaseObject(pTextResource);
    }
}

void GetSightingElementValue(CfxControl *pControl, XGUIDLIST *pElements, QString &Value)
{
    XGUID *elementId = nullptr;
    ATTRIBUTE *attribute = nullptr;
    CctSighting *sighting = nullptr;

    Value.clear();

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
        FXTEXTRESOURCE *text = LockElementText(pControl, pElements, ElementAttribute::Name);
        if (text)
        {
            Value = QString::fromUtf8(&text->Text[0]);
            UnlockElementText(pControl, text);
        }
    }
    else if (!IsNullXGuid(&attribute->ValueId))
    {
        FXTEXTRESOURCE *text = (FXTEXTRESOURCE *)pControl->GetResource()->GetObject(pControl, attribute->ValueId, (BYTE)ElementAttribute::Name);
        if (text)
        {
            Value = QString::fromUtf8(&text->Text[0]);
            UnlockElementText(pControl, text);
        }
    }
    else
    {
        Type_VariantToText(attribute->Value, Value);
    }

    if (Value.startsWith(QLatin1String(MEDIA_PREFIX)))
    {
        Value = QStringLiteral("[Media]");
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
    _attribute = ElementAttribute::Name;
}

XGUIDLIST *CctControl_ElementMemo::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != nullptr ? elements : &_elements;
}

void CctControl_ElementMemo::UpdateText()
{
    FXTEXTRESOURCE *text = LockElementText(this, GetElements(), _attribute);
    if (text)
    {
        SetCaption(&text->Text[0]);
        UnlockElementText(this, text);
    }
}

void CctControl_ElementMemo::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementMemo::DefineProperties(CfxFiler &F)
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

void CctControl_ElementMemo::DefinePropertiesUI(CfxFilerUI &F) 
{
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
    _attribute = ElementAttribute::Name;
}

XGUIDLIST *CctControl_ElementPanel::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != nullptr ? elements : &_elements;
}

void CctControl_ElementPanel::UpdateText()
{
    if (_attribute == (ElementAttribute)0xFF)
    {
        QString valueText;
        GetSightingElementValue(this, GetElements(), valueText);

        // Strip from the tab onwards
        int tabPos = valueText.indexOf('\t');
        if (tabPos >= 0)
        {
            valueText.truncate(tabPos);
        }

        SetCaption(valueText.toUtf8().constData());
        return;
    }

    FXTEXTRESOURCE *text = LockElementText(this, GetElements(), _attribute);
    if (text)
    {
        SetCaption(&text->Text[0]);
        UnlockElementText(this, text);
    }
}

void CctControl_ElementPanel::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementPanel::DefineProperties(CfxFiler &F)
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

void CctControl_ElementPanel::DefinePropertiesUI(CfxFilerUI &F) 
{
}
