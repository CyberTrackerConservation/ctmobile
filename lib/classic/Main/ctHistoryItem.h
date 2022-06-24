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

const UINT MAGIC_HISTORY = 'HIST';

// Must be packed for compatibility with the Server
#pragma pack(push, 1) 

struct HISTORY_ITEM
{
    UINT Magic; 
    UINT ColumnCount;
    UINT RecordCount;
    GUID Id;
    DOUBLE DateTime;
    DOUBLE X, Y;
};

struct HISTORY_WAYPOINT
{
    UINT Magic;
    UINT Flags;
    DOUBLE X, Y;
};
#pragma pack(pop)

typedef CHAR HISTORY_ROW[32];

class CctHistory_Painter: public CfxListPainter
{
protected:
    HISTORY_ITEM *_historyItemDef, *_historyItem;
    UINT _itemCount, _activeIndex;
public:
    CctHistory_Painter(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctHistory_Painter();

    VOID Connect(UINT Index);

    UINT GetItemCount();
    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index);
    VOID BeforePaint(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID AfterPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

