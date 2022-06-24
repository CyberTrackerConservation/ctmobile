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
#include "fxSound.h"

//*************************************************************************************************
//
// CctControl_ElementSound
//

const GUID GUID_CONTROL_ELEMENTSOUND = {0x2687cee1, 0x589, 0x4ee7, {0xb8, 0x16, 0xe0, 0xe4, 0x69, 0x92, 0xb5, 0xd7}};

class CctControl_ElementSound: public CfxControl_SoundBase
{
protected:
    XGUIDLIST _elements;

    XGUIDLIST *GetElements();
    FXSOUNDRESOURCE *GetSound();
    VOID ReleaseSound(FXSOUNDRESOURCE *pSound);
public:
    CctControl_ElementSound(CfxPersistent *pOwner, CfxControl *pParent);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};

extern CfxControl *Create_Control_ElementSound(CfxPersistent *pOwner, CfxControl *pParent);

