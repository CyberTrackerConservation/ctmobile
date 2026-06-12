#pragma once
#include "pch.h"
#include "fxList.h"

const int MAGIC_HISTORY = 'HIST';

// Must be packed for compatibility with the Server
#pragma pack(push, 1) 

struct HISTORY_ITEM
{
    int Magic = 0;
    int ColumnCount = 0;
    int RecordCount = 0;
    GUID Id;
    double DateTime = 0.0;
    double X = 0.0, Y = 0.0;
};

struct HISTORY_WAYPOINT
{
    int Magic = 0;
    int Flags = 0;
    double X = 0.0, Y = 0.0;
};
#pragma pack(pop)

typedef char HISTORY_ROW[32];

class CctHistory_Painter: public CfxListPainter
{
protected:
    HISTORY_ITEM *_historyItemDef = nullptr, *_historyItem = nullptr;
    int _itemCount = 0, _activeIndex = 0;
public:
    CctHistory_Painter(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctHistory_Painter();

    void Connect(int Index);

    int GetItemCount() override;
    void PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index) override;
    void BeforePaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
    void AfterPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

