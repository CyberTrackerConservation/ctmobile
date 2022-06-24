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

//*************************************************************************************************
//
// CfxControl_Button
//
const GUID GUID_CONTROL_BUTTON = {0x1758d8b1, 0xc5c4, 0x4b81, {0xa9, 0x58, 0x42, 0xbf, 0xb7, 0xd0, 0x9a, 0xfe}};

class CfxControl_Button: public CfxControl_Panel
{
protected:
	BOOL _penDown, _penOver, _down;
    BYTE _groupId;
public:
    CfxControl_Button(CfxPersistent *pOwner, CfxControl *pParent);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

	VOID OnPenDown(INT X, INT Y);
    VOID OnPenUp(INT X, INT Y);
    VOID OnPenMove(INT X, INT Y);

    BOOL GetDown()                    { return _down || (_penDown && _penOver); }
    VOID SetDown(const BOOL Value);
    BYTE GetGroupId()                 { return _groupId;  }
    VOID SetGroupId(const BYTE Value) { _groupId = Value; }

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
};

extern CfxControl *Create_Control_Button(CfxPersistent *pOwner, CfxControl *pParent);
