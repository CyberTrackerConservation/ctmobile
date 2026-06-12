#include "fxApplication.h"
#include "fxUtils.h"
#include "ctElementImage.h"
#include "ctSighting.h"
#include "ctSession.h"
#include "ctElementContainer.h"

CfxControl *Create_Control_ElementImage(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementImage(pOwner, pParent);
}

CctControl_ElementImage::CctControl_ElementImage(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_ImageBase(pOwner, pParent), _elements()
{
    InitControl(&GUID_CONTROL_ELEMENTIMAGE, true);
    InitLockProperties("Attribute;Center;Stretch;Proportional;Image;Elements");
    _attribute = ElementAttribute::Image1;
}

XGUIDLIST *CctControl_ElementImage::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != nullptr ? elements : &_elements;
}

void CctControl_ElementImage::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementImage::DefineProperties(CfxFiler &F)
{
    CfxControl_ImageBase::DefineProperties(F);

    F.DefineValue("Attribute",    dtInt8,    &_attribute, "12");
    F.DefineXOBJECTLIST("Elements", &_elements);

    // BUGBUG: remove once stabilized
    _attribute = (ElementAttribute)max((int)ElementAttribute::Icon32, (int)_attribute);
    _attribute = (ElementAttribute)min((int)ElementAttribute::Image8, (int)_attribute);
}

void CctControl_ElementImage::DefinePropertiesUI(CfxFilerUI &F) 
{
}

FXBITMAPRESOURCE *CctControl_ElementImage::GetBitmap()
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

    return (FXBITMAPRESOURCE *)GetResource()->GetObject(this, *elementId, (BYTE)_attribute);
}

void CctControl_ElementImage::ReleaseBitmap(FXBITMAPRESOURCE *pBitmap)
{
    GetResource()->ReleaseObject(pBitmap);
}

//*************************************************************************************************
// CfxControl_ElementZoomImage

CfxControl *Create_Control_ElementZoomImage(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_ElementZoomImage(pOwner, pParent);
}

CfxControl_ElementZoomImage::CfxControl_ElementZoomImage(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_ZoomImageBase(pOwner, pParent), _elements()
{
    InitControl(&GUID_CONTROL_ELEMENTZOOMIMAGE);
    InitLockProperties("Attribute;Elements");
    _attribute = ElementAttribute::Image1;
}

XGUIDLIST *CfxControl_ElementZoomImage::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != nullptr ? elements : &_elements;
}

void CfxControl_ElementZoomImage::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_ElementZoomImage::DefineProperties(CfxFiler &F)
{
    CfxControl_ZoomImageBase::DefineProperties(F);

    F.DefineValue("Attribute",  dtByte, &_attribute, "12");
    F.DefineXOBJECTLIST("Elements", &_elements);
}

void CfxControl_ElementZoomImage::DefinePropertiesUI(CfxFilerUI &F) 
{
}

FXBITMAPRESOURCE *CfxControl_ElementZoomImage::GetBitmap()
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

    return (FXBITMAPRESOURCE *)GetResource()->GetObject(this, *elementId, (BYTE)_attribute);
}

void CfxControl_ElementZoomImage::ReleaseBitmap(FXBITMAPRESOURCE *pBitmap)
{
    GetResource()->ReleaseObject(pBitmap);
}

bool CfxControl_ElementZoomImage::GetImageData(int *pWidth, int *pHeight)
{
    *pWidth = *pHeight = 0;

    FXBITMAPRESOURCE *bitmap = GetBitmap();
    if (bitmap)
    {
        *pWidth = bitmap->Width;
        *pHeight = bitmap->Height;
        ReleaseBitmap(bitmap);
    }

    return (*pWidth && *pHeight);
}

void CfxControl_ElementZoomImage::DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect)
{
    FXBITMAPRESOURCE *bitmap = GetBitmap();
    if (bitmap)
    {
        pDstCanvas->StretchDraw(bitmap, pSrcRect, pDstRect, false, false);
        ReleaseBitmap(bitmap);
    }
}
