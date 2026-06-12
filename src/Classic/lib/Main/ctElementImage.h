#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "ctElement.h"
#include "fxImage.h"

//*************************************************************************************************
//
// CctControl_ElementImage
//

const GUID GUID_CONTROL_ELEMENTIMAGE = {0x633ef9eb, 0x5224, 0x4730, {0x97, 0x0a, 0x77, 0x2a, 0x89, 0x49, 0xb4, 0x9c}};

class CctControl_ElementImage: public CfxControl_ImageBase
{
protected:
    XGUIDLIST _elements;
    ElementAttribute _attribute = ElementAttribute::Name;

    XGUIDLIST *GetElements();
    FXBITMAPRESOURCE *GetBitmap() override;
    void ReleaseBitmap(FXBITMAPRESOURCE *pBitmap) override;
public:
    CctControl_ElementImage(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_ElementImage(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
//
// CfxControl_ElementZoomImage
//

const GUID GUID_CONTROL_ELEMENTZOOMIMAGE = {0xc4231538, 0x48b8, 0x4e49, {0x9f, 0x76, 0x68, 0xee, 0xb3, 0x9d, 0x3a, 0x15}};

class CfxControl_ElementZoomImage: public CfxControl_ZoomImageBase
{
protected:
    XGUIDLIST _elements;
    ElementAttribute _attribute = ElementAttribute::Name;

    XGUIDLIST *GetElements();
    FXBITMAPRESOURCE *GetBitmap();
    void ReleaseBitmap(FXBITMAPRESOURCE *pBitmap);

    bool GetImageData(int *pWidth, int *pHeight) override;
    void DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect) override;
public:
    CfxControl_ElementZoomImage(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_ElementZoomImage(CfxPersistent *pOwner, CfxControl *pParent);

