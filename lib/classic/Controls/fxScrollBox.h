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
#include "fxPanel.h"
#include "fxScrollBar.h"

const GUID GUID_CONTROL_SCROLLBOX = {0x4753aa0b, 0x2072, 0x4b03, {0x88, 0x1d, 0x0c, 0xd7, 0x67, 0x53, 0x22, 0x85}};

//
// CfxControl_ScrollBox
//
class CfxControl_ScrollBox: public CfxControl_Panel
{
protected:
    CfxControl_ScrollBarV2 *_scrollBar;
    UINT _scrollBarWidth;
    CfxControl *_container;
    BOOL _once;

    UINT GetScrollBarWidth();
    VOID UpdateScrollBar();
    VOID OnScrollChange(CfxControl *pSender, VOID *pParams);
public:
    CfxControl_ScrollBox(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_ScrollBox();

    CfxControl *GetChildParent() { return _container; }

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

extern CfxControl *Create_Control_ScrollBox(CfxPersistent *pOwner, CfxControl *pParent);

