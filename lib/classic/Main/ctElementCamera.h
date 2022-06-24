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

const GUID GUID_CONTROL_ELEMENTCAMERA = {0x2f601f3, 0x28bd, 0x4d1f, {0xa9, 0xdd, 0xca, 0xa7, 0x1a, 0xbc, 0x25, 0xfc}};

//
// CctControl_ElementCamera
//
class CctControl_ElementCamera: public CfxControl_Panel
{
protected:
    GUID _sightingUniqueId;
    XGUID _elementId;
    BOOL _supported;
    BOOL _required;
    BOOL _waitForTaken, _pending;
    FXIMAGE_QUALITY _imageQuality;
    FXBITMAPRESOURCE *_cacheBitmap;

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ElementCamera(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementCamera();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPenClick(INT X, INT Y);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID OnPictureTaken(CHAR *pFileName, BOOL Success);

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
};

extern CfxControl *Create_Control_ElementCamera(CfxPersistent *pOwner, CfxControl *pParent);
