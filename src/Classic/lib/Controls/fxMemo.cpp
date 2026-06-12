#include "fxMemo.h"
#include "fxApplication.h"
#include "fxUtils.h"

CfxControl *Create_Control_Memo(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Memo(pOwner, pParent);
}

CfxControl_Memo::CfxControl_Memo(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_MEMO);
    InitLockProperties("Caption;Font");
    InitBorder(ControlBorderStyle::Single, 0);

    _caption = 0;
    _minHeight = 16;
    _alignment = TEXT_ALIGN_LEFT;
    _rightToLeft = false;

    _onClick.Object = nullptr;

    _wordWrap = true;
    _autoHeight = false;
    
    _scrollBar = new CfxControl_ScrollBarV2(pOwner, this);
    _scrollBar->SetComposite(true);

    _scrollBarWidth = 14;

    InitFont(F_FONT_DEFAULT_M);
    _textColor = _borderColor;
}

CfxControl_Memo::~CfxControl_Memo()
{
    MEM_FREE(_caption);
}

void CfxControl_Memo::SetOnClick(CfxPersistent *pObject, NotifyMethod OnClick)
{
    _onClick.Object = pObject;
    _onClick.Method = OnClick;
}

void CfxControl_Memo::SetCaption(const char *Caption)
{
    MEM_FREE(_caption);
    _caption = (char *) MEM_ALLOC(strlen(Caption) + 1);
    strcpy(_caption, Caption);
    UpdateScrollBar();
}

int CfxControl_Memo::GetScrollBarWidth()
{
    return _scrollBar->GetVisible() ? (int)_scrollBarWidth : 0;
}

void CfxControl_Memo::UpdateScrollBar()
{
    int sw = _scrollBarWidth;

    _scrollBar->SetColor(_color);
    _scrollBar->SetBorderColor(_borderColor);

    CfxResource *resource = GetResource();
    if (resource)
    {
        CfxCanvas *canvas = GetCanvas(this);

        int w = GetClientWidth() - 4;
        int h = GetClientHeight() - 4;

        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        FXRECT rect = {0, 0, (int)w-1, (int)h-1};
        canvas->CalcTextRect(font, _caption, &rect);

        if (_autoHeight && ((_dock == ControlDock::Top) || (_dock == ControlDock::Bottom) || (_dock == ControlDock::None)))
        {
            ResizeHeight(max(16, rect.Bottom + (int)_borderWidth * 2 + _borderLineWidth * 2 + 4));
            h = GetClientHeight() - 4;
        }

        _scrollBar->SetVisible((int)rect.Bottom > h);
        if (_scrollBar->GetVisible())
        {
            if (IsLive())
            {
                sw = (_scrollBarWidth * GetEngine(this)->GetScaleScroller()) / 100;
            }

            rect.Right = w - (int)sw - 1;
            rect.Bottom = h - 1;
            GetCanvas(this)->CalcTextRect(font, _caption, &rect);

            int fh = canvas->FontHeight(font);
            _scrollBar->Setup(0, (rect.Bottom + 1) / fh, h / fh, 1);
        }
        else
        {
            _scrollBar->Setup(0, 0, 0, 0);
        }
        
        resource->ReleaseFont(font);
    }
    _scrollBar->SetBounds(_width - _borderWidth - sw, _borderWidth, _scrollBar->GetVisible() ? sw : 0, GetClientHeight());
}

void CfxControl_Memo::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);
    UpdateScrollBar();
}

void CfxControl_Memo::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _height = max((int)_minHeight, _height);

    _scrollBar->Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _scrollBar->SetCanScale(false);
    _scrollBarWidth = ComputeScrollBarWidth(_scrollBarWidth, _borderLineWidth, ScaleX, ScaleHitSize);
}

void CfxControl_Memo::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_Memo::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Alignment",    dtInt32,    &_alignment, F_ZERO);
    F.DefineValue("AutoHeight",   dtBoolean,  &_autoHeight, F_FALSE);
    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
    F.DefineValue("Caption",      dtPText,    &_caption, F_NULL);
    F.DefineValue("MinHeight",    dtInt32,    &_minHeight, "16");
    F.DefineValue("RightToLeft",  dtBoolean,  &_rightToLeft, F_FALSE);
    F.DefineValue("ScrollBarWidth", dtInt32,  &_scrollBarWidth, "9");

    if (F.IsReader())
    {
        UpdateScrollBar();
    }

    /*_borderColor = RGB(241, 227, 199);
    _textColor = RGB(74, 37, 0);*/

}

void CfxControl_Memo::DefinePropertiesUI(CfxFilerUI &F) 
{
}


void CfxControl_Memo::OnPenClick(int X, int Y)
{
    if (_onClick.Object)
    {
        ExecuteEvent(&_onClick, this);
    }
}

void CfxControl_Memo::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    CfxResource *resource = GetResource(); 

    if (_caption && (strlen(_caption) > 0))
    {
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            FXRECT rect = *pRect;
            rect.Right -= GetScrollBarWidth();
            rect.Top -= (int)_scrollBar->GetValue() * pCanvas->FontHeight(font);
            InflateRect(&rect, -2, -2);
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, &rect, _alignment | TEXT_ALIGN_WORDWRAP | (_rightToLeft ? TEXT_ALIGN_RIGHT : 0), _caption);
            resource->ReleaseFont(font);
        }
    }
}
