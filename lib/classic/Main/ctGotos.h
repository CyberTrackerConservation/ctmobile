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
#include "fxList.h"
#include "ctSession.h"

//*************************************************************************************************
//
// CctControl_Gotos
//

const GUID GUID_CONTROL_GOTOLIST = {0x7b7db615, 0xbb0d, 0x49e7, {0xa0, 0xf2, 0x47, 0xce, 0xf8, 0x9, 0x74, 0x19}};

class CctControl_GotoList: public CfxControl_List
{
protected:
    INT _selectedIndex;
    BOOL _centerText, _rightToLeft;
    FXGPS_POSITION _cachePosition;
    FXGOTO _cacheItem;
    
    FXGOTO *GetItem(UINT Index, BOOL *pBoundary);
public:
    CctControl_GotoList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_GotoList();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    UINT GetItemCount();
    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    VOID OnPenDown(INT X, INT Y);
};

extern CfxControl *Create_Control_GotoList(CfxPersistent *pOwner, CfxControl *pParent);

class CctGoto_Painter: public CfxListPainter
{
protected:
    FXGOTO _goto;
public:
    CctGoto_Painter(CfxPersistent *pOwner, CfxControl *pParent): CfxListPainter(pOwner, pParent), _goto() { }
    ~CctGoto_Painter() { }

    VOID Connect(FXGOTO *pGoto) { _goto = *pGoto; }

    UINT GetItemCount();
    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index);
};

