#include "fxApplication.h"
#include "fxUtils.h"
#include "fxList.h"

CfxControl *Create_Control_List(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_List(pOwner, pParent);
}

//*************************************************************************************************
// CfxControl_List

CfxControl_List::CfxControl_List(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_LIST);
    InitBorder(ControlBorderStyle::Single, 0);

    _supportsDrag = true;
    _painter = nullptr;

    _scrollBar = new CfxControl_ScrollBarV2(pOwner, this);
    _scrollBar->SetComposite(true);

	_autoHeight = false;
    _scaleForHitSize = true;

    _itemHeight = 33;
    _minItemHeight = 16;
    _minItemWidth = 32;

    _columnCount = 1;
    _scrollBarWidth = 14;
    _showLines = true;

    _momentumTimer = new QTimer(nullptr);
    _momentumTimer->setInterval(30);
    QObject::connect(_momentumTimer, &QTimer::timeout, [this]() { MomentumTick(); });
}

CfxControl_List::~CfxControl_List()
{
    _momentumTimer->stop();
    delete _momentumTimer;
    _painter = nullptr;
}

int CfxControl_List::GetScrollBarWidth()
{
    return _scrollBar->GetVisible() ? (_scrollBar->GetWidth() - (int)_borderLineWidth) : 0;
}

int CfxControl_List::GetVisibleItemCount()
{
    int lineWidth = (int)_borderLineWidth * (_showLines ? 1 : 0);
    int t = (GetClientHeight() - lineWidth + (int)_itemHeight - 1) / ((int)_itemHeight + lineWidth);
    return t * GetColumnCount();
}

int CfxControl_List::GetColumnCount()
{
    int w = GetClientWidth() - GetScrollBarWidth();
    return max(1, min(w / (int)_minItemWidth, (int)_columnCount));
}

void CfxControl_List::UpdateScrollBar()
{
    if (_autoHeight && ((_dock == ControlDock::Top) || (_dock == ControlDock::Bottom) || (_dock == ControlDock::None)))
	{
		int visibleRows = GetItemCount() / _columnCount;
		int lineWidth = (int)_borderLineWidth * (_showLines ? 1 : 0);
		int maxHeight = _parent == nullptr ? _height : _parent->GetHeight();
        ResizeHeight(min(maxHeight, (_itemHeight + lineWidth) * visibleRows + lineWidth));
	}

    int lineWidth = (int)_borderLineWidth * (_showLines ? 1 : 0);
    int itemStep = (int)_itemHeight + lineWidth;
    int visibleRows = _height == 0 ? 0 : (GetClientHeight() - lineWidth) / itemStep;
    int visibleItems = visibleRows * GetColumnCount();
    int totalRows = (GetItemCount() + GetColumnCount() - 1) / GetColumnCount();

    int sw = _scrollBarWidth;
    if (IsLive())
    {
        sw = (_scrollBarWidth * GetEngine(this)->GetScaleScroller()) / 100;
    }

	_scrollBar->SetBounds(_width - _borderWidth - sw, _borderWidth, sw, GetClientHeight());
    _scrollBar->Setup(0, totalRows * itemStep, visibleRows * itemStep, itemStep);
    _scrollBar->SetVisible(GetItemCount() > visibleItems);
    _scrollBar->SetColor(_color);
    _scrollBar->SetBorderColor(_borderColor == _color ? _textColor : _borderColor);
    _scrollBar->SetBorderLineWidth((int)_borderLineWidth);
}

bool CfxControl_List::HitTest(int X, int Y, int *pIndex, int *pOffsetX, int *pOffsetY)
{
    int lineWidth = (int)_borderLineWidth * (_showLines ? 1 : 0);
    int itemStep = (int)_itemHeight + lineWidth;
    int columnCount = GetColumnCount();
    int columnWidth = (GetClientWidth() - GetScrollBarWidth()) / columnCount;

    int scrollPixelOffset = itemStep > 0 ? _scrollBar->GetValue() % itemStep : 0;
    int adjustedY = Y + scrollPixelOffset;
    int row = (adjustedY - (int)_borderWidth) / itemStep;
    int col = (X - (int)_borderWidth) / (int)columnWidth;

    int scrollRow = itemStep > 0 ? _scrollBar->GetValue() / itemStep : 0;
    *pIndex = (scrollRow + row) * columnCount + col;

    *pOffsetX = X - (int)_borderWidth - col * (int)columnWidth;
    *pOffsetY = adjustedY - (int)_borderWidth - row * itemStep;

    return ((*pIndex >= 0) && (*pIndex < (int)GetItemCount()));
}

void CfxControl_List::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

	UpdateScrollBar();
}

void CfxControl_List::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    if (!_scaleForHitSize) 
    {
        ScaleHitSize = 0;
    }

    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _scrollBar->Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _scrollBar->SetCanScale(false);
    _scrollBarWidth = ComputeScrollBarWidth(_scrollBarWidth, _borderLineWidth, ScaleX, ScaleHitSize);

    _minItemHeight = max(ScaleHitSize, _minItemHeight); 
    _itemHeight = (_itemHeight * ScaleY) / 100;
    _itemHeight = max(_itemHeight, _minItemHeight);

    if (ScaleX > 100)
    {
        _minItemWidth = (_minItemWidth * ScaleX) / 100;
    }

    if (ScaleY > 100)
    {
        _minItemHeight = (_minItemHeight * ScaleY) / 100;
    }

    _minItemWidth = max(ScaleHitSize, _minItemWidth);

    UpdateScrollBar();
}

void CfxControl_List::ResetState()
{
    CfxControl_Panel::ResetState();
    _scrollBar->ResetState();
}

void CfxControl_List::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_List::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (_painter)
    {
        _painter->DefineProperties(F);
    }

    F.DefineValue("AutoHeight",     dtBoolean, &_autoHeight, F_FALSE);
    F.DefineValue("ScaleForHitSize", dtBoolean,&_scaleForHitSize, F_TRUE);

    F.DefineValue("BorderColor",    dtColor,   &_borderColor, F_COLOR_BLACK);
    F.DefineValue("Color",          dtColor,   &_color, F_COLOR_WHITE);
    F.DefineValue("TextColor",      dtColor,   &_textColor, F_COLOR_BLACK);

    F.DefineValue("ItemHeight",     dtInt32,   &_itemHeight, "33");
    F.DefineValue("MinItemHeight",  dtInt32,   &_minItemHeight, "16");
    F.DefineValue("MinItemWidth",   dtInt32,   &_minItemWidth, "32");
    F.DefineValue("ColumnCount",    dtInt32,   &_columnCount, F_ONE);
    F.DefineValue("ScrollBarWidth", dtInt32,   &_scrollBarWidth, "9");
    F.DefineValue("ShowLines",      dtBoolean, &_showLines, F_TRUE);

    // Force a Min Item Height of 30: remove later
    if (_minItemHeight == 32)
    {
        _minItemHeight = 30;
    }

    // Force an Item Height of 34: remove later
    if (_itemHeight == 36)
    {
        _itemHeight = 34;
    }

    if (F.IsReader())
    {
        _scrollBar->SetValue(0);
    }
}

void CfxControl_List::DefineState(CfxFiler &F)
{
    _scrollBar->DefineState(F);
}

void CfxControl_List::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CfxControl_List::OnPenDown(int X, int Y)
{
    _momentumTimer->stop();
    _velocity = 0;
    _penStartScrollValue = _scrollBar->GetValue();
    _lastDragY = Y;
    _lastDragTime = FxGetTickCount();
}

void CfxControl_List::OnPenUp(int X, int Y)
{
    if (abs(_velocity) > 50)
    {
        _momentumTimer->start();
    }
    else
    {
        // Snap to nearest item boundary
        int lineWidth = (int)_borderLineWidth * (_showLines ? 1 : 0);
        int itemStep = (int)_itemHeight + lineWidth;
        if (itemStep > 0)
        {
            int value = _scrollBar->GetValue();
            _scrollBar->SetValue(((value + itemStep / 2) / itemStep) * itemStep);
        }
    }

    Repaint();
}

void CfxControl_List::OnPenDrag(int StartX, int StartY, int X, int Y)
{
    _scrollBar->SetValue(_penStartScrollValue + (StartY - Y));

    // Track velocity
    int now = FxGetTickCount();
    int dt = now - _lastDragTime;
    if (dt > 0)
    {
        int tps = FxGetTicksPerSecond();
        _velocity = ((_lastDragY - Y) * tps) / dt;
        _lastDragY = Y;
        _lastDragTime = now;
    }

    Repaint();
}

void CfxControl_List::MomentumTick()
{
    int tps = FxGetTicksPerSecond();
    int pixelDelta = (_velocity * 30) / (tps > 0 ? tps : 1000);

    int oldValue = _scrollBar->GetValue();
    _scrollBar->SetValue(oldValue + pixelDelta);

    // Stop at scroll limits
    if (_scrollBar->GetValue() == oldValue)
    {
        _velocity = 0;
    }

    // Apply friction
    _velocity = (_velocity * 95) / 100;

    // Stop when slow enough
    if (abs(_velocity) < 50)
    {
        _momentumTimer->stop();
        _velocity = 0;

        // Snap to nearest item boundary
        int lineWidth = (int)_borderLineWidth * (_showLines ? 1 : 0);
        int itemStep = (int)_itemHeight + lineWidth;
        if (itemStep > 0)
        {
            int value = _scrollBar->GetValue();
            _scrollBar->SetValue(((value + itemStep / 2) / itemStep) * itemStep);
        }
    }

    Repaint();
}

void CfxControl_List::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    pCanvas->State.TextColor = _textColor;

    if (GetItemCount() == 0)
    {
        CfxControl_Panel::OnPaint(pCanvas, pRect);
        return;
    }
    
    if (_painter)
    {
        _painter->BeforePaint(pCanvas, pRect);
    }

    CfxResource *resource = GetResource(); 
    FXFONTRESOURCE *font = resource->GetFont(this, _font);

    // Set clip rect for smooth scrolling (partial items at edges)
    CanvasState savedState = pCanvas->State;
    FXRECT clipRect = *pRect;
    pCanvas->SetClipRect(&clipRect);

    // Calculations to speed up drawing
    int lineWidth   = (int)_borderLineWidth * (_showLines ? 1 : 0);
    int itemStep = (int)_itemHeight + lineWidth;
    int columnCount = GetColumnCount();
    int columnWidth = (GetClientWidth() - GetScrollBarWidth()) / columnCount;
    int visibleRows = GetVisibleItemCount() / columnCount + 1;

    int scrollValue = _scrollBar->GetValue();
    int scrollRow = itemStep > 0 ? scrollValue / itemStep : 0;
    int scrollPixelOffset = itemStep > 0 ? scrollValue % itemStep : 0;
    int index = scrollRow * columnCount;
    int startIndex = index;
    int count = GetItemCount();

    int xstart = pRect->Left;
    int ystart = pRect->Top - scrollPixelOffset;

    for (int j=0; j<visibleRows; j++)
    {
        xstart = pRect->Left;

        for (int i=0; i<(int)columnCount; i++)
        {
            int colWidth = columnWidth;

            if (i == (int)columnCount - 1)
            {
                colWidth = GetClientWidth() - GetScrollBarWidth() - xstart + pRect->Left;
            }

            FXRECT rectLines = {xstart, ystart, xstart + (int)colWidth - 1, ystart + (int)_itemHeight + (int)lineWidth * 2 - 1};
            if (index < count)
            {
                pCanvas->State.BrushColor = _color;
                pCanvas->State.LineColor = _borderColor;
                pCanvas->State.LineWidth = lineWidth;
                _transparent ? pCanvas->FrameRect(&rectLines) : pCanvas->Rectangle(&rectLines);
                
                pCanvas->State.LineWidth = 1;
                InflateRect(&rectLines, -(int)lineWidth, -(int)lineWidth);

                PaintItem(pCanvas, &rectLines, font, index);
            }
            else if (!_transparent)
            {
                rectLines.Top += lineWidth;
                if (i > 0)
                {
                    rectLines.Left += lineWidth;//showLines;
                }
                if (i == (int)columnCount - 1)
                {
                    rectLines.Right = pRect->Right;
                }
                pCanvas->State.BrushColor = _color;
                pCanvas->FillRect(&rectLines);
            }

            xstart += columnWidth - lineWidth;
            index++;
        }
        ystart += (int)_itemHeight + lineWidth;
    }

    // Restore clip rect
    pCanvas->State.ClipRect = savedState.ClipRect;

    if (!_transparent)
    {
        FXRECT rectBottom = *pRect;
        int contentBottom = pRect->Top - scrollPixelOffset + visibleRows * ((int)_itemHeight + lineWidth) + lineWidth;
        rectBottom.Top = max(rectBottom.Top, contentBottom);
        if (rectBottom.Top <= rectBottom.Bottom)
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->FillRect(&rectBottom);
        }
    }

    resource->ReleaseFont(font);

    if (_painter)
    {
        _painter->AfterPaint(pCanvas, pRect);
    }
}

