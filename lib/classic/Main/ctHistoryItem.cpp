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

#include "ctHistoryItem.h"
#include "ctSession.h"

CctHistory_Painter::CctHistory_Painter(CfxPersistent *pOwner, CfxControl *pParent): CfxListPainter(pOwner, pParent)
{
    _historyItemDef = _historyItem = NULL;
    _itemCount = _activeIndex = 0;
}

CctHistory_Painter::~CctHistory_Painter()
{
    _historyItemDef = _historyItem = NULL;
}

VOID CctHistory_Painter::Connect(UINT Index)
{
    _itemCount = _activeIndex = 0;

    if (!_parent->IsLive()) return;

    CctSession *session = GetSession(_parent);
    
    if (session->EnumHistoryInit(&_historyItem))
    {
        _itemCount = _historyItem->ColumnCount;
        _activeIndex = Index;
        session->EnumHistoryDone();
    }

    _historyItem = NULL;
}

UINT CctHistory_Painter::GetItemCount()
{
    return _itemCount;
}

VOID CctHistory_Painter::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index)
{
    CctSession *session = GetSession(_parent);

    UINT itemHeight = pRect->Bottom - pRect->Top + 1;

    // Left side rect
    FXRECT lRect = *pRect;
    lRect.Left += (2 * _parent->GetBorderLineWidth()) + itemHeight;
    lRect.Right = pRect->Left + (pRect->Right - pRect->Left + 1) / 2;

    // Right side rect
    FXRECT rRect = *pRect;
    rRect.Left = lRect.Right + 1;

    // Set text color
    //pCanvas->State.TextColor = _parent->GetTextColor();

    //
    // Draw History
    //

    HISTORY_ROW *historyRowDef = session->GetHistoryItemRow(_historyItemDef, Index);
    pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, (CHAR *)historyRowDef);
    HISTORY_ROW *historyRow = session->GetHistoryItemRow(_historyItem, Index);
    pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, (CHAR *)historyRow);
}

VOID CctHistory_Painter::BeforePaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    CctSession *session = GetSession(_parent);
    
    if (session->EnumHistoryInit(&_historyItemDef))
    {
        session->EnumHistorySetIndex(_activeIndex);
        session->EnumHistoryNext(&_historyItem);
    }
}

VOID CctHistory_Painter::AfterPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (_historyItemDef)
    {
        GetSession(_parent)->EnumHistoryDone();
    }

    _historyItemDef = _historyItem = NULL;
}
