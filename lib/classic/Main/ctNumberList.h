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
#include "ctElement.h"
#include "ctSession.h"

const GUID GUID_CONTROL_NUMBERLIST = {0x70267b40, 0x4e8f, 0x4224, {0x9a, 0xb4, 0xe4, 0x13, 0xf7, 0xe6, 0x96, 0x48}};

//
// CctControl_NumberList
//
class CctControl_NumberList: public CfxControl_List
{
protected:
    BOOL _autoNext;
    XGUID _elementId;
    INT _startValue, _endValue, _interval;
    INT _selectedIndex;
    UINT _autoSelectIndex;
    CctRetainState _retainState;
    CHAR *_resultGlobalValue;

    UINT GetColumnCount() { return _columnCount; }

    VOID SetSelectedIndex(INT Index);
    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionEnterNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_NumberList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_NumberList();

    VOID ResetState();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);

    VOID OnPenDown(INT X, INT Y);
    VOID OnPenClick(INT X, INT Y);

    UINT GetItemCount();
    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index);
};

extern CfxControl *Create_Control_NumberList(CfxPersistent *pOwner, CfxControl *pParent);

