#include "fxNotebook.h"
#include "fxApplication.h"
#include "fxUtils.h"

CfxControl *Create_Control_Notebook(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Notebook(pOwner, pParent);
}

//*************************************************************************************************
// CfxControl_Notebook

CfxControl_Notebook::CfxControl_Notebook(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent), _pages(), _pageWidths(), _pageRects()
{
    InitControl(&GUID_CONTROL_NOTEBOOK, true);
    InitLockProperties("Default Page Index");
    InitBorder(ControlBorderStyle::Single, 0);
    
    _minTitleHeight = 0;

    _firstIndex = 0;
    _activeIndex = 1;
    _lastActiveIndex = -1;

    _backButton = new CfxControl_Button(pOwner, this);
    _backButton->SetComposite(true);
    _backButton->SetVisible(false);
    _backButton->SetOnClick(this, (NotifyMethod)&CfxControl_Notebook::OnBackClick);
    _backButton->SetOnPaint(this, (NotifyMethod)&CfxControl_Notebook::OnBackPaint);

    _nextButton = new CfxControl_Button(pOwner, this);
    _nextButton->SetComposite(true);
    _nextButton->SetVisible(false);
    _nextButton->SetOnClick(this, (NotifyMethod)&CfxControl_Notebook::OnNextClick);
    _nextButton->SetOnPaint(this, (NotifyMethod)&CfxControl_Notebook::OnNextPaint);

    _onPageChange.Object = nullptr;

    SetCaption("Page 1");
    ProcessPages();

    InitFont(F_FONT_DEFAULT_M);
    InitFont(_bigFont, F_FONT_DEFAULT_LB);
}

CfxControl_Notebook::~CfxControl_Notebook()
{
    _pages.Clear();
    _pageWidths.Clear();
    _pageRects.Clear();
    _backButton = _nextButton = nullptr;
}

void CfxControl_Notebook::SetActivePageIndex(int Index)
{
    if ((Index > 0) && (Index <= GetPageCount()))
    {
        _activeIndex = Index;
    }
}

void CfxControl_Notebook::SetOnPageChange(CfxControl *pControl, NotifyMethod OnPageChange)
{
    _onPageChange.Object = pControl;
    _onPageChange.Method = OnPageChange;
}

char *CfxControl_Notebook::ParseTitle(int Index)
{
    int currentIndex = 0;

    char *cursor = _caption;
    char *start = _caption;
    while (1)
    {
        char ch = *cursor;
        if ((ch == ';') || (ch == '\0'))
        {
            if (currentIndex == Index)
            {
                int len = cursor - start;
                char *title = (char *) MEM_ALLOC(len + 1);
                strncpy(title, start, len);
                title[len] = 0;
                return title;
            }
            else
            {
                start = cursor;
                start ++;
                currentIndex++;
            }
        }

        if (ch == '\0')
        {
            return nullptr;
        }
        else
        {
            cursor++;
        }
    }
}

XFONT *CfxControl_Notebook::GetDefaultFont()
{
    if (IsLive() && ((GetEngine(this)->GetScaleTab() > 100) || (_minTitleHeight > 16)))
    {
        return &_bigFont;
    }
    else
    {
        return &_font;
    }
}

void CfxControl_Notebook::UpdatePages()
{
    for (int i=0; i<GetPageCount(); i++)
    {
        _pages.Get(i)->SetVisible(i == _activeIndex - 1);
    }

    int titleHeight = GetTitleHeight();

    GetActivePage()->SetBounds(_borderWidth, _borderWidth + titleHeight + _borderLineWidth, GetClientWidth(), GetClientHeight() - titleHeight - _borderLineWidth);
    GetActivePage()->SetTransparent(_transparent);

    _backButton->SetBorder(ControlBorderStyle::Single, 0, _borderLineWidth, _borderColor);
    _backButton->SetBounds(GetClientWidth() - _borderWidth - titleHeight * 2, _borderWidth, titleHeight, titleHeight);

    _nextButton->SetBorder(ControlBorderStyle::Single, 0, _borderLineWidth, _borderColor);
    _nextButton->SetBounds(_backButton->GetLeft() + titleHeight, _borderWidth, titleHeight, titleHeight);

    UpdateButtons();

    CfxEngine *engine = GetEngine(this);
    if (engine)
    {
        engine->AlignControls(GetActivePage());
    }

    // Run the change event now that everything is set up
    if (_lastActiveIndex != _activeIndex)
    {
        if (_onPageChange.Object)
        {
            ExecuteEvent(&_onPageChange, this);
        }
        _lastActiveIndex = _activeIndex;
    }
}

void CfxControl_Notebook::ProcessPages()
{
    int currentIndex = 0;
    char *cursor = _caption;
    while (1)
    {
        char ch = *cursor;
        if ((ch == ';') || (ch == '\0'))
        {
            currentIndex++;

            if (GetPageCount() < currentIndex)
            {
                CfxControl_Panel *panel = new CfxControl_Panel(GetOwner(), this);
                panel->SetBorderWidth(0);
                panel->SetBorderStyle(ControlBorderStyle::None);
                panel->SetComposite(true);
                _pages.Add(panel);
            }
        }

        if (ch == '\0') 
        {
            break;
        }
        else
        {
            cursor++;
        }
    }

    while (currentIndex < GetPageCount())
    {
        delete _pages.Get(currentIndex);
        _pages.Delete(currentIndex);
    }
}

void CfxControl_Notebook::UpdateButtons()
{
    bool visible = GetTitleWidth(0) > GetClientWidth();
    _backButton->SetVisible(_visible && (_firstIndex > 0));
    _nextButton->SetVisible(visible && (_firstIndex < GetPageCount() - 1));
}

void CfxControl_Notebook::OnBackClick(CfxControl *pSender, void *pParams)
{
    if (_firstIndex > 0)
    {
        _firstIndex--;
    }
    
    if (_activeIndex > 0)
    {
        SetActivePageIndex(_activeIndex - 1);
        _firstIndex = _activeIndex - 1;
        UpdatePages();
    }

    UpdateButtons();

    Repaint();
}

void CfxControl_Notebook::OnNextClick(CfxControl *pSender, void *pParams)
{
    _firstIndex = max(1, _activeIndex) - 1;
    SetActivePageIndex(_activeIndex + 1);
    UpdatePages();
   
    UpdateButtons();

    Repaint();
}

void CfxControl_Notebook::OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawBackArrow(pParams->Rect, 
                                   _borderColor, 
                                   _color,
                                   _backButton->GetDown());
}

void CfxControl_Notebook::OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawNextArrow(pParams->Rect, 
                                   _borderColor,
                                   _color,
                                   _nextButton->GetDown());
}

void CfxControl_Notebook::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _minTitleHeight = ScaleHitSize;
    UpdatePages();
}

void CfxControl_Notebook::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl_Panel::SetBounds(Left, Top, Width, Height);

    UpdatePages();
}

void CfxControl_Notebook::DefineControlsState(CfxFiler &F)
{
    for (int i=0; i<GetPageCount(); i++)
    {
        GetPage(i)->DefineState(F);
    }
}

void CfxControl_Notebook::DefineControls(CfxFiler &F)
{
    if (F.DefineBegin("Pages"))
    {
        if (F.IsWriter())
        {
            ProcessPages();

            for (int i=0; i<GetPageCount(); i++)
            {
                F.DefineBegin("Page");
                _pages.Get(i)->DefineProperties(F);
                F.DefineEnd();
            }
            F.ListEnd();
        }
        else
        {
            FX_ASSERT(F.IsReader());

            int oldPageCount = _pages.GetCount();
            int pageIndex = 0;

            while (!F.ListEnd())
            {
                if (F.DefineBegin("Page"))
                {
                    CfxControl_Panel *panel = nullptr;
                    
                    if (pageIndex < oldPageCount)
                    {
                        panel = _pages.Get(pageIndex);
                    }
                    else
                    {
                        panel = new CfxControl_Panel(GetOwner(), this);
                        panel->SetComposite(true);
                        _pages.Add(panel);
                    }

                    panel->DefineProperties(F);
                    F.DefineEnd();

                    pageIndex++;
                }
                else break;
            }

            int count = _pages.GetCount();
            while (_pages.GetCount() > pageIndex)
            {
                delete _pages.Get(pageIndex);
                _pages.Delete(pageIndex);
            }
        }

        F.DefineEnd();
    }
}

void CfxControl_Notebook::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("ActivePage", dtInt32, &_activeIndex, F_ONE);
    F.DefineXFONT("BigFont", &_bigFont, F_FONT_DEFAULT_LB);


    if (F.IsReader())
    {
        if (_activeIndex <= 0)
        {
            _activeIndex = 1;
        }
        else if (_activeIndex > GetPageCount())
        {
            _activeIndex = GetPageCount();
        }

        if (!_caption || (strlen(_caption) == 0))
        {
            SetCaption("Page 1");
        }

        _lastActiveIndex = -1;

        ProcessPages();
        UpdatePages();
    }
}

void CfxControl_Notebook::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CfxControl_Notebook::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_Notebook::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);
    F.DefineValue("ActivePage", dtInt32, &_activeIndex);
}

int CfxControl_Notebook::GetTitleWidth(int FirstIndex)
{
    CfxResource *resource = GetResource(); 
    if (!resource) return 0;
    FXFONTRESOURCE *font = resource->GetFont(this, *GetDefaultFont());
    if (!font) return 0;

    int pageCount = GetPageCount();

    _pageWidths.SetCount(pageCount);

    int totalWidth = 0;
    for (int i=0; i<pageCount; i++)
    {
        char *title = ParseTitle(i);
        int pageWidth;
        if (!title) continue;

        pageWidth = max(_minTitleHeight, GetCanvas(this)->TextWidth(font, title));
        _pageWidths.Put(i, pageWidth);

        if (i >= FirstIndex)
        {
            totalWidth += pageWidth + 8 * _oneSpace;
        }

        MEM_FREE(title);
    }

    resource->ReleaseFont(font);

    return totalWidth;
}

int CfxControl_Notebook::GetTitleHeight()
{
    return max(_minTitleHeight, max(GetDefaultFontHeight(), 12) + (int)_borderLineWidth * 2);
}

void CfxControl_Notebook::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    CfxResource *resource = GetResource(); 
    FXFONTRESOURCE *font = resource->GetFont(this, *GetDefaultFont());
    if (!font) return;

    // Hack to prevent crash if we get called before the pages have been set up.
    // This happened when someone set a global result value incorrectly.
    GetTitleWidth(0);

    FXRECT rect = *pRect;

    rect.Bottom = rect.Top + max(_minTitleHeight, pCanvas->FontHeight(font) + (int)_borderLineWidth * 2);

    pCanvas->State.BrushColor = _borderColor;
    pCanvas->FillRect(&rect);

    pCanvas->State.LineColor = _color;

    pCanvas->State.BrushColor = _color;
    rect.Top = rect.Bottom;
    pCanvas->FillRect(&rect);
    rect.Top = pRect->Top + 1;
    pCanvas->State.BrushColor = _borderColor;
    rect.Bottom--;

    int pageCount = GetPageCount();

    _pageRects.Clear();
    _pageRects.SetCount(pageCount);

    for (int i=(int)_firstIndex; i<pageCount; i++)
    {
        char *title = ParseTitle(i);
        int pageWidth;
        if (!title) continue;

        pageWidth = _pageWidths.Get(i);

        COLOR brushColor = pCanvas->InvertColor(_color, i != (_activeIndex - 1));
        COLOR textColor = pCanvas->InvertColor(_textColor, i != (_activeIndex - 1));
        pCanvas->State.BrushColor = brushColor;
        pCanvas->State.TextColor = textColor;

        rect.Right = rect.Left + pageWidth + 8 * _oneSpace;
        pCanvas->FillRect(&rect);
        if (i > _firstIndex)
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->FillRect(rect.Left, 
							  rect.Top, 
							  max((int)_oneSpace, (int)_borderLineWidth), 
							  (int)(rect.Bottom - rect.Top + 1));
        }

        _pageRects.Put(i, rect);

        pCanvas->DrawTextRect(font, &rect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, title);
        rect.Left += pageWidth + 4 * _oneSpace;
        MEM_FREE(title);

        rect.Left = rect.Right;
    }

    resource->ReleaseFont(font);
}

void CfxControl_Notebook::OnPenClick(int X, int Y)
{
    if (Y > (int)(GetTitleHeight() + _borderWidth)) return;

    FXRECT r;
    GetAbsoluteBounds(&r);

    for (int i=(int)_firstIndex; i<_pageRects.GetCount(); i++)
    {
        FXRECT *rect = _pageRects.GetPtr(i);
        FXPOINT pt;
        pt.X = X + r.Left;
        pt.Y = rect->Top;
        if (PtInRect(rect, &pt))
        {
            SetActivePageIndex(i + 1);
            UpdatePages();
            break;
        }
    }

    /*CfxResource *resource = GetResource(); 
    if (!resource) return;
    FXFONTRESOURCE *font = resource->GetFont(this, *GetDefaultFont());
    if (!font) return;

    int w = _borderWidth;
    for (int i=(int)_firstIndex; i<GetPageCount(); i++)
    {
        char *title = ParseTitle(i);
        if (!title) continue;

        int wt = w;
        w = w + GetCanvas(this)->TextWidth(font, title) + 8 * _oneSpace;

        if ((X >= (int)wt) && (X < (int)w)) 
        {
            SetActivePageIndex(i + 1);
            UpdatePages();
        }
        
        MEM_FREE(title);
    }

    resource->ReleaseFont(font);*/

    Repaint();
}
