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
#include "fxMemo.h"

const GUID GUID_CONTROL_OWNERINFO = {0x4023eaa, 0x6f34, 0x48b1, {0xa8, 0x60, 0x43, 0xd2, 0xec, 0xc, 0x89, 0x3c}};

enum OwnerInfoStyle {oikName, oikOrganization, oikAddress1, oikAddress2, oikAddress3, oikPhone, oikEmail, oikWebSite};

//
// CctControl_OwnerInfo
//
class CctControl_OwnerInfo: public CfxControl_Memo
{
protected:
    OwnerInfoStyle _style;
public:
    CctControl_OwnerInfo(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_OwnerInfo();

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};

extern CfxControl *Create_Control_OwnerInfo(CfxPersistent *pOwner, CfxControl *pParent);

