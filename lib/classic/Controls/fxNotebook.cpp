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
    InitControl(&GUID_CONTROL_NOTEBOOK, TRUE);
    InitLockProperties("Default Page Index");
    InitBorder(bsSingle, 0);
    
    _minTitleHeight = 0;

    _firstIndex = 0;
    _activeIndex = 1;
    _lastActiveIndex = -1;

    _backButton = new CfxControl_Button(pOwner, this);
    _backButton->SetComposite(TRUE);
    _backButton->SetVisible(FALSE);
    _backButton->SetOnClick(this, (NotifyMethod)&CfxControl_Notebook::OnBackClick);
    _backButton->SetOnPaint(this, (NotifyMethod)&CfxControl_Notebook::OnBackPaint);

    _nextButton = new CfxControl_Button(pOwner, this);
    _nextButton->SetComposite(TRUE);
    _nextButton->SetVisible(FALSE);
    _nextButton->SetOnClick(this, (NotifyMethod)&CfxControl_Notebook::OnNextClick);
    _nextButton->SetOnPaint(this, (NotifyMethod)&CfxControl_Notebook::OnNextPaint);

    _onPageChange.Object = NULL;

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
    _backButton = _nextButton = NULL;
}

VOID CfxControl_Notebook::SetActivePageIndex(UINT Index)
{
    if ((Index > 0) && (Index <= GetPageCount()))
    {
        _activeIndex = Index;
    }
}

VOID CfxControl_Notebook::SetOnPageChange(CfxControl *pControl, NotifyMethod OnPageChange)
{
    _onPageChange.Object = pControl;
    _onPageChange.Method = OnPageChange;
}

CHAR *CfxControl_Notebook::ParseTitle(UINT Index)
{
    UINT currentIndex = 0;

    CHAR *cursor = _caption;
    CHAR *start = _caption;
    while (1)
    {
        CHAR ch = *cursor;
        if ((ch == ';') || (ch == '\0'))
        {
            if (currentIndex == Index)
            {
                UINT len = cursor - start;
                CHAR *title = (CHAR *) MEM_ALLOC(len + 1);
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
            return NULL;
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

VOID CfxControl_Notebook::UpdatePages()
{
    for (UINT i=0; i<GetPageCount(); i++)
    {
        _pages.Get(i)->SetVisible(i == _activeIndex - 1);
    }

    UINT titleHeight = GetTitleHeight();

    GetActivePage()->SetBounds(_borderWidth, _borderWidth + titleHeight + _borderLineWidth, GetClientWidth(), GetClientHeight() - titleHeight - _borderLineWidth);
    GetActivePage()->SetTransparent(_transparent);

    _backButton->SetBorder(bsSingle, 0, _borderLineWidth, _borderColor);
    _backButton->SetBounds(GetClientWidth() - _borderWidth - titleHeight * 2, _borderWidth, titleHeight, titleHeight);

    _nextButton->SetBorder(bsSingle, 0, _borderLineWidth, _borderColor);
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

VOID CfxControl_Notebook::ProcessPages()
{
    UINT currentIndex = 0;
    CHAR *cursor = _caption;
    while (1)
    {
        CHAR ch = *cursor;
        if ((ch == ';') || (ch == '\0'))
        {
            currentIndex++;

            if (GetPageCount() < currentIndex)
            {
                CfxControl_Panel *panel = new CfxControl_Panel(_owner, this);
                panel->SetBorderWidth(0);
                panel->SetBorderStyle(bsNone);
                panel->SetComposite(TRUE);
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

VOID CfxControl_Notebook::UpdateButtons()
{
    BOOL visible = GetTitleWidth(0) > GetClientWidth();
    _backButton->SetVisible(_visible && (_firstIndex > 0));
    _nextButton->SetVisible(visible && (_firstIndex < GetPageCount() - 1));
}

VOID CfxControl_Notebook::OnBackClick(CfxControl *pSender, VOID *pParams)
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

VOID CfxControl_Notebook::OnNextClick(CfxControl *pSender, VOID *pParams)
{
    _firstIndex = max(1, _activeIndex) - 1;
    SetActivePageIndex(_activeIndex + 1);
    UpdatePages();
   
    UpdateButtons();

    Repaint();
}

VOID CfxControl_Notebook::OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawBackArrow(pParams->Rect, 
                                   _borderColor, 
                                   _color,
                                   _backButton->GetDown());
}

VOID CfxControl_Notebook::OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawNextArrow(pParams->Rect, 
                                   _borderColor,
                                   _color,
                                   _nextButton->GetDown());
}

VOID CfxControl_Notebook::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _minTitleHeight = ScaleHitSize;
    UpdatePages();
}

VOID CfxControl_Notebook::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl_Panel::SetBounds(Left, Top, Width, Height);

    UpdatePages();
}

VOID CfxControl_Notebook::DefineControlsState(CfxFiler &F)
{
    for (UINT i=0; i<GetPageCount(); i++)
    {
        GetPage(i)->DefineState(F);
    }
}

VOID CfxControl_Notebook::DefineControls(CfxFiler &F)
{
    if (F.DefineBegin("Pages"))
    {
        if (F.IsWriter())
        {
            ProcessPages();

            for (UINT i=0; i<GetPageCount(); i++)
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

            UINT oldPageCount = _pages.GetCount();
            UINT pageIndex = 0;

            while (!F.ListEnd())
            {
                if (F.DefineBegin("Page"))
                {
                    CfxControl_Panel *panel = NULL;
                    
                    if (pageIndex < oldPageCount)
                    {
                        panel = _pages.Get(pageIndex);
                    }
                    else
                    {
                        panel = new CfxControl_Panel(_owner, this);
                        panel->SetComposite(TRUE);
                        _pages.Add(panel);
                    }

                    panel->DefineProperties(F);
                    F.DefineEnd();

                    pageIndex++;
                }
                else break;
            }

            UINT count = _pages.GetCount();
            while (_pages.GetCount() > pageIndex)
            {
                delete _pages.Get(pageIndex);
                _pages.Delete(pageIndex);
            }
        }

        F.DefineEnd();
    }
}

VOID CfxControl_Notebook::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("ActivePage", dtInt32, &_activeIndex, F_ONE);
    F.DefineXFONT("BigFont", &_bigFont, F_FONT_DEFAULT_LB);

    #ifdef CLIENT_DLL
        StringReplace(&_lockProperties, "Pages", "");   // unsafe
    #endif

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

VOID CfxControl_Notebook::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Default Page Index", dtInt32, &_activeIndex, "MIN(1)");
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Pages",        dtPText,     &_caption);
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}

VOID CfxControl_Notebook::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
    F.DefineFont(_bigFont);
#endif
}

VOID CfxControl_Notebook::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);
    F.DefineValue("ActivePage", dtInt32, &_activeIndex);
}

UINT CfxControl_Notebook::GetTitleWidth(UINT FirstIndex)
{
    CfxResource *resource = GetResource(); 
    if (!resource) return 0;
    FXFONTRESOURCE *font = resource->GetFont(this, *GetDefaultFont());
    if (!font) return 0;

    UINT pageCount = GetPageCount();

    _pageWidths.SetCount(pageCount);

    UINT totalWidth = 0;
    for (UINT i=0; i<pageCount; i++)
    {
        CHAR *title = ParseTitle(i);
        UINT pageWidth;
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

UINT CfxControl_Notebook::GetTitleHeight()
{
    return max(_minTitleHeight, max(GetDefaultFontHeight(), 12) + (UINT)_borderLineWidth * 2);
}

VOID CfxControl_Notebook::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    CfxResource *resource = GetResource(); 
    FXFONTRESOURCE *font = resource->GetFont(this, *GetDefaultFont());
    if (!font) return;

    // Hack to prevent crash if we get called before the pages have been set up.
    // This happened when someone set a global result value incorrectly.
    GetTitleWidth(0);

    FXRECT rect = *pRect;

    rect.Bottom = rect.Top + max(_minTitleHeight, pCanvas->FontHeight(font) + (UINT)_borderLineWidth * 2);

    pCanvas->State.BrushColor = _borderColor;
    pCanvas->FillRect(&rect);

    pCanvas->State.LineColor = _color;

    pCanvas->State.BrushColor = _color;
    rect.Top = rect.Bottom;
    pCanvas->FillRect(&rect);
    rect.Top = pRect->Top + 1;
    pCanvas->State.BrushColor = _borderColor;
    rect.Bottom--;

    UINT pageCount = GetPageCount();

    _pageRects.Clear();
    _pageRects.SetCount(pageCount);

    for (UINT i=(UINT)_firstIndex; i<pageCount; i++)
    {
        CHAR *title = ParseTitle(i);
        UINT pageWidth;
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
							  max((UINT)_oneSpace, (UINT)_borderLineWidth), 
							  (UINT)(rect.Bottom - rect.Top + 1));
        }

        _pageRects.Put(i, rect);

        pCanvas->DrawTextRect(font, &rect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, title);
        rect.Left += pageWidth + 4 * _oneSpace;
        MEM_FREE(title);

        rect.Left = rect.Right;
    }

    resource->ReleaseFont(font);
}

VOID CfxControl_Notebook::OnPenClick(INT X, INT Y)
{
    if (Y > (INT)(GetTitleHeight() + _borderWidth)) return;

    FXRECT r;
    GetAbsoluteBounds(&r);

    for (UINT i=(UINT)_firstIndex; i<_pageRects.GetCount(); i++)
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

    UINT w = _borderWidth;
    for (UINT i=(UINT)_firstIndex; i<GetPageCount(); i++)
    {
        CHAR *title = ParseTitle(i);
        if (!title) continue;

        UINT wt = w;
        w = w + GetCanvas(this)->TextWidth(font, title) + 8 * _oneSpace;

        if ((X >= (INT)wt) && (X < (INT)w)) 
        {
            SetActivePageIndex(i + 1);
            UpdatePages();
        }
        
        MEM_FREE(title);
    }

    resource->ReleaseFont(font);*/

    Repaint();
}