#pragma once
#include "pch.h"
#include "fxControl.h"
#include "fxPanel.h"
#include "fxScrollBar.h"

const GUID GUID_CONTROL_NOTEBOOK = {0xd882a9c7, 0x3351, 0x4286, {0x90, 0x63, 0x3b, 0x7c, 0xf5, 0x19, 0x23, 0x5b}};

//
// CfxControl_Notebook
//
class CfxControl_Notebook: public CfxControl_Panel
{
protected:
    XFONT _bigFont = 0;
    int _minTitleHeight = 0;

    int _firstIndex = 0, _activeIndex = 0, _lastActiveIndex = 0;
    CfxControl_Button *_backButton = nullptr, *_nextButton = nullptr;
    TList<CfxControl_Panel *> _pages;
    TList<FXRECT> _pageRects;
    TList<int> _pageWidths;

    FXEVENT _onPageChange;

    char *ParseTitle(int Index);
    void UpdatePages();
    int GetTitleWidth(int FirstIndex = 0);
    int GetTitleHeight();

    void DefineControls(CfxFiler &F) override;
    void DefineControlsState(CfxFiler &F) override;

    void OnBackClick(CfxControl *pSender, void *pParams);
    void OnNextClick(CfxControl *pSender, void *pParams);
    void OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    void UpdateButtons();
public:
    CfxControl_Notebook(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Notebook();

    XFONT *GetDefaultFont() override;

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
    
    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
    void SetBounds(int Left, int Top, int Width, int Height) override;
    void SetOnPageChange(CfxControl *pControl, NotifyMethod OnPageChange);

    CfxControl *GetChildParent() override { return GetActivePage(); }

    void SetActivePageIndex(int Index);
    int GetActivePageIndex()            { return (int)_activeIndex;                                      }
    CfxControl *GetActivePage()         { return _activeIndex != -1 ? _pages.Get(_activeIndex-1) : this; }
    CfxControl *GetPage(int Index)     { return Index != -1 ? _pages.Get(Index) : nullptr;                 }
    int GetPageCount()                 { return (int)_pages.GetCount();                                }

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
    void OnPenClick(int X, int Y) override;

    void ProcessPages();
};

extern CfxControl *Create_Control_Notebook(CfxPersistent *pOwner, CfxControl *pParent);

