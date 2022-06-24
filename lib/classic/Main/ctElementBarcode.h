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
#include "fxButton.h"

const GUID GUID_CONTROL_ELEMENTBARCODE = { 0x7234220a, 0x65bf, 0x4fb6, { 0x96, 0x37, 0xbc, 0xc7, 0x6, 0x84, 0xa8, 0xf4 } };

//
// CctControl_ElementBarcode
//
class CctControl_ElementBarcode: public CfxControl_Button
{
protected:
    GUID _sightingUniqueId;
    XGUID _elementId;
    BOOL _supported;
    BOOL _required;
    BOOL _waitForTaken, _pending;
    CHAR *_filterBefore;
    CHAR *_filterAfter;

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ElementBarcode(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementBarcode();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPenClick(INT X, INT Y);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    
    VOID OnBarcodeScan(CHAR *pBarcode, BOOL Success);

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
};

extern CfxControl *Create_Control_ElementBarcode(CfxPersistent *pOwner, CfxControl *pParent);
