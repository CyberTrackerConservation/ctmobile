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
    ElementAttribute _attribute;

    XGUIDLIST *GetElements();
    FXBITMAPRESOURCE *GetBitmap();
    VOID ReleaseBitmap(FXBITMAPRESOURCE *pBitmap);
public:
    CctControl_ElementImage(CfxPersistent *pOwner, CfxControl *pParent);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
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
    ElementAttribute _attribute;

    XGUIDLIST *GetElements();
    FXBITMAPRESOURCE *GetBitmap();
    VOID ReleaseBitmap(FXBITMAPRESOURCE *pBitmap);

    BOOL GetImageData(UINT *pWidth, UINT *pHeight);
    VOID DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect);
public:
    CfxControl_ElementZoomImage(CfxPersistent *pOwner, CfxControl *pParent);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};

extern CfxControl *Create_Control_ElementZoomImage(CfxPersistent *pOwner, CfxControl *pParent);

