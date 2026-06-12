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
    return elements != nullptr ? elements : &_elements;
}

void CctControl_ElementSound::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementSound::DefineProperties(CfxFiler &F)
{
    CfxControl_SoundBase::DefineProperties(F);
    F.DefineXOBJECTLIST("Elements", &_elements);
}

void CctControl_ElementSound::DefinePropertiesUI(CfxFilerUI &F) 
{
}

FXSOUNDRESOURCE *CctControl_ElementSound::GetSound()
{
    XGUIDLIST *elements = GetElements();
    if (elements->GetCount() == 0) return nullptr;

    // Match an element
    XGUID *elementId = nullptr;
    if (IsLive())
    {
        elementId = GetSession(this)->GetCurrentSighting()->MatchAttribute(elements);
    }
    if (!elementId)
    {
        elementId = elements->GetPtr(0);
    }

    return (FXSOUNDRESOURCE *)GetResource()->GetObject(this, *elementId, (BYTE)ElementAttribute::Sound);
}

void CctControl_ElementSound::ReleaseSound(FXSOUNDRESOURCE *pSound)
{
    GetResource()->ReleaseObject(pSound);
}

