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
#include "fxControl.h"

const GUID GUID_CONTROL_ELEMENTFILTER = {0x71fa9209, 0x9b77, 0x4a40, {0xb4, 0xc1, 0x84, 0x2e, 0xee, 0x84, 0x1d, 0xce}};

//
// CctControl_ElementFilter
//
class CctControl_ElementFilter: public CfxControl
{
protected:
    CHAR *_caption;
    COLOR _textColor;
    UINT _minHeight, _minWidth;
    XGUID _filterScreenId;
    BOOL _filtered;
    BOOL _penDown, _penOver;
    VOID SetCaption(CHAR *pCaption);
    VOID ExecuteFilter();
public:
    CctControl_ElementFilter(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementFilter();

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

	VOID OnPenDown(INT X, INT Y);
    VOID OnPenUp(INT X, INT Y);
    VOID OnPenMove(INT X, INT Y);

    VOID OnPenClick(INT X, INT Y);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

extern CfxControl *Create_Control_ElementFilter(CfxPersistent *pOwner, CfxControl *pParent);

