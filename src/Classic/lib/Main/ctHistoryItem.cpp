#include "ctHistoryItem.h"
#include "ctSession.h"

CctHistory_Painter::CctHistory_Painter(CfxPersistent *pOwner, CfxControl *pParent): CfxListPainter(pOwner, pParent)
{
    _historyItemDef = _historyItem = nullptr;
    _itemCount = _activeIndex = 0;
}

CctHistory_Painter::~CctHistory_Painter()
{
    _historyItemDef = _historyItem = nullptr;
}

void CctHistory_Painter::Connect(int Index)
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

    _historyItem = nullptr;
}

int CctHistory_Painter::GetItemCount()
{
    return _itemCount;
}

void CctHistory_Painter::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index)
{
    CctSession *session = GetSession(_parent);

    int itemHeight = pRect->Bottom - pRect->Top + 1;

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
    pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, (char *)historyRowDef);
    HISTORY_ROW *historyRow = session->GetHistoryItemRow(_historyItem, Index);
    pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, (char *)historyRow);
}

void CctHistory_Painter::BeforePaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    CctSession *session = GetSession(_parent);
    
    if (session->EnumHistoryInit(&_historyItemDef))
    {
        session->EnumHistorySetIndex(_activeIndex);
        session->EnumHistoryNext(&_historyItem);
    }
}

void CctHistory_Painter::AfterPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (_historyItemDef)
    {
        GetSession(_parent)->EnumHistoryDone();
    }

    _historyItemDef = _historyItem = nullptr;
}
