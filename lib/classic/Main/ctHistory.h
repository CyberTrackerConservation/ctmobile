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
#include "fxList.h"
#include "ctElement.h"
#include "ctSession.h"
#include "ctNavigate.h"

const GUID GUID_CONTROL_HISTORY = {0x5e1cb980, 0xd452, 0x4037, {0xb2, 0x9a, 0x48, 0xc0, 0x6a, 0xe5, 0xb2, 0x15}};

struct HISTORYITEM
{
    UINT ActualIndex;
    BYTE IconAttribute;
};

//
// CctControl_History
//
class CctControl_History: public CfxControl
{
protected:
    CfxControl_Button *_backButton, *_nextButton;
    INT _index;
    BOOL _transparentItems;
    UINT _minHeight;
    
    TList<XGUID> _cacheSnapshot;
    TList<HISTORYITEM> _cache;

    VOID CheckCache();
    VOID RebuildCache();

    VOID OnBackClick(CfxControl *pSender, VOID *pParams);
    VOID OnNextClick(CfxControl *pSender, VOID *pParams);
    VOID OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    UINT GetVisibleItemCount();
    VOID FixButtons();
public:
    CctControl_History(CfxPersistent *pOwner, CfxControl *pParent);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    UINT GetItemCount();
    BOOL PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, ATTRIBUTE *pAttribute);
};

extern CfxControl *Create_Control_History(CfxPersistent *pOwner, CfxControl *pParent);
