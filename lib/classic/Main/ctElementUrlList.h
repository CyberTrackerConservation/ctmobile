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

// {023FC6A7-B186-4a67-AB30-322C4DC1408A}
const GUID GUID_CONTROL_ELEMENTURLLIST = { 0x23fc6a7, 0xb186, 0x4a67, { 0xab, 0x30, 0x32, 0x2c, 0x4d, 0xc1, 0x40, 0x8a } };

class CctControl_ElementUrlList: public CfxControl_List
{
protected:
    BOOL _autoNext;
    XGUID _elementId;
    INT _selectedIndex;
    BOOL _centerText, _rightToLeft;
    CHAR *_separator;

    CHAR *_url;
    CHAR *_userName;
    CHAR *_password;
    
    TList<CHAR*> _list; // The list of items to draw
    CHAR *_pListBuffer; // Backing memory for each list item

    BOOL _required;
    CHAR *_listItems;   // Unparsed items

    BOOL _waitingForResponse;
    BOOL _waitingInvert;
    
    CHAR *GetItem(UINT Index);
    VOID ParseList();

    VOID OnSessionEnter(CfxControl *pSender, VOID *pParams);
    VOID OnSessionEnterNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
    VOID OnUrlReceived(const CHAR *pFileName, UINT ErrorCode);
    VOID OnTimer();

    VOID SetSelectedIndex(INT Index);

public:
    CctControl_ElementUrlList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementUrlList();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    UINT GetItemCount();
    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    VOID OnPenDown(INT X, INT Y);
    VOID OnPenClick(INT X, INT Y);
};

extern CfxControl *Create_Control_ElementUrlList(CfxPersistent *pOwner, CfxControl *pParent);

