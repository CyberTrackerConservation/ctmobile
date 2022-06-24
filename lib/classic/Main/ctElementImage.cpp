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
    InitControl(&GUID_CONTROL_ELEMENTIMAGE, TRUE);
    InitLockProperties("Attribute;Center;Stretch;Proportional;Image;Elements");
    _attribute = eaImage1;
}

XGUIDLIST *CctControl_ElementImage::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != NULL ? elements : &_elements;
}

VOID CctControl_ElementImage::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_ImageBase::DefineResources(F);

    FX_ASSERT((_attribute >= eaIcon32) && (_attribute <= eaImage8));

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

VOID CctControl_ElementImage::DefineProperties(CfxFiler &F)
{
    CfxControl_ImageBase::DefineProperties(F);

    F.DefineValue("Attribute",    dtInt8,    &_attribute, "12");
    F.DefineXOBJECTLIST("Elements", &_elements);

    // BUGBUG: remove once stabilized
    _attribute = max(eaIcon32, _attribute);
    _attribute = min(eaImage8, _attribute);
}

VOID CctControl_ElementImage::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    //
    // We only want to allow selection of ehIcon32, ehIcon50, ehIcon100 and ehImage
    // enum ElementAttribute {eaName=0, eaIcon32, eaIcon50, eaIcon100, eaImage, eaSound};
    //
    CHAR items[256];
    sprintf(items, "LIST(Icon32=%d Icon50=%d Icon100=%d Image1=%d Image2=%d Image3=%d Image4=%d Image5=%d Image6=%d Image7=%d Image8=%d)", eaIcon32, eaIcon50, eaIcon100, eaImage1, eaImage2, eaImage3, eaImage4, eaImage5, eaImage6, eaImage7, eaImage8);
    F.DefineValue("Attribute",    dtByte,     &_attribute,   items);

    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Center",       dtBoolean,  &_center);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");

    if (GetContainerElements(this) == NULL)
    {
        F.DefineValue("Elements",     dtPGuidList, &_elements,   "EDITOR(ScreenElementsEditor);REQUIRED");
    }

    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Stretch",      dtBoolean,  &_stretch);
    F.DefineValue("Proportional", dtBoolean,  &_proportional);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent Image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

FXBITMAPRESOURCE *CctControl_ElementImage::GetBitmap()
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

    return (FXBITMAPRESOURCE *)GetResource()->GetObject(this, *elementId, _attribute);
}

VOID CctControl_ElementImage::ReleaseBitmap(FXBITMAPRESOURCE *pBitmap)
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
    _attribute = eaImage1;
}

XGUIDLIST *CfxControl_ElementZoomImage::GetElements()
{
    XGUIDLIST *elements = GetContainerElements(this);
    return elements != NULL ? elements : &_elements;
}

VOID CfxControl_ElementZoomImage::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_ZoomImageBase::DefineResources(F);

    FX_ASSERT((_attribute >= eaImage1) && (_attribute <= eaImage8));

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

VOID CfxControl_ElementZoomImage::DefineProperties(CfxFiler &F)
{
    CfxControl_ZoomImageBase::DefineProperties(F);

    F.DefineValue("Attribute",  dtByte, &_attribute, "12");
    F.DefineXOBJECTLIST("Elements", &_elements);
}

VOID CfxControl_ElementZoomImage::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    // We only want to allow selection of ehImageX
    CHAR items[256];
    sprintf(items, "LIST(Image1=%d Image2=%d Image3=%d Image4=%d Image5=%d Image6=%d Image7=%d Image8=%d)", eaImage1, eaImage2, eaImage3, eaImage4, eaImage5, eaImage6, eaImage7, eaImage8);
    F.DefineValue("Attribute",    dtByte,     &_attribute,   items);

    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button width", dtInt32,    &_buttonWidth, "MIN(16);MAX(100)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");

    if (GetContainerElements(this) == NULL)
    {
        F.DefineValue("Elements",     dtPGuidList, &_elements,   "EDITOR(ScreenElementsEditor);REQUIRED");
    }

    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Initial button state", dtByte, &_initialButtonState, "LIST(Zoom Pan)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Smooth pan",   dtBoolean,  &_smoothPan);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

FXBITMAPRESOURCE *CfxControl_ElementZoomImage::GetBitmap()
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

    return (FXBITMAPRESOURCE *)GetResource()->GetObject(this, *elementId, _attribute);
}

VOID CfxControl_ElementZoomImage::ReleaseBitmap(FXBITMAPRESOURCE *pBitmap)
{
    GetResource()->ReleaseObject(pBitmap);
}

BOOL CfxControl_ElementZoomImage::GetImageData(UINT *pWidth, UINT *pHeight)
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

VOID CfxControl_ElementZoomImage::DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect)
{
    FXBITMAPRESOURCE *bitmap = GetBitmap();
    if (bitmap)
    {
        pDstCanvas->StretchDraw(bitmap, pSrcRect, pDstRect, FALSE, FALSE);
        ReleaseBitmap(bitmap);
    }
}
