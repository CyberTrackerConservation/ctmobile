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

const GUID GUID_CONTROL_MARQUEE = {0xfc003d20, 0x75d1, 0x4b84, {0x88, 0x6c, 0xc4, 0xc1, 0x32, 0x7c, 0x70, 0xee}};

//
// CfxControl_Marquee
//
class CfxControl_Marquee: public CfxControl
{
protected:
    CHAR *_caption;
    COLOR _textColor;
    INT _timeDatum;
    INT _rate;
public:
    CfxControl_Marquee(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Marquee();

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

extern CfxControl *Create_Control_Marquee(CfxPersistent *pOwner, CfxControl *pParent);

