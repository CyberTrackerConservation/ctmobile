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

// {986B4CE5-8266-40ba-8436-65CFC380C7DE}
const GUID GUID_CONTROL_ELEMENTLOOKUPLIST = { 0x986b4ce5, 0x8266, 0x40ba, { 0x84, 0x36, 0x65, 0xcf, 0xc3, 0x80, 0xc7, 0xde } };

class CctControl_ElementLookupList: public CfxControl_List
{
protected:
    BOOL _autoNext;
    XGUID _elementId;
    INT _selectedIndex;
    BOOL _centerText, _rightToLeft;

    CHAR *_resultGlobalValue;
    
    TList<CHAR*> _list; // The list of items to draw
    CHAR *_pListBuffer; // Backing memory for each list item

    BOOL _required;
    CHAR *_listItems;   // Unparsed items
    CHAR *_listName;    // List name
    
    CHAR *GetItem(UINT Index);
    VOID ParseLookupList();
    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);

    VOID SetSelectedIndex(INT Index);

public:
    CctControl_ElementLookupList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementLookupList();

    CHAR *GetItems()  { return _listItems;    }
    VOID SetItems(CHAR *pValue) { MEM_FREE(_listItems); _listItems = AllocString(pValue); }
    CHAR *GetName()   { return _listName; }

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    UINT GetItemCount();
    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index);

    VOID OnPenDown(INT X, INT Y);
    VOID OnPenClick(INT X, INT Y);
};

extern CfxControl *Create_Control_ElementLookupList(CfxPersistent *pOwner, CfxControl *pParent);

