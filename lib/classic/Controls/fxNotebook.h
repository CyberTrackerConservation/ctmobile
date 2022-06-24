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
    XFONT _bigFont;
    UINT _minTitleHeight;

    UINT _firstIndex, _activeIndex, _lastActiveIndex;
    CfxControl_Button *_backButton, *_nextButton;
    TList<CfxControl_Panel *> _pages;
    TList<FXRECT> _pageRects;
    TList<UINT> _pageWidths;

    FXEVENT _onPageChange;

    CHAR *ParseTitle(UINT Index);
    VOID UpdatePages();
    UINT GetTitleWidth(UINT FirstIndex = 0);
    UINT GetTitleHeight();

    VOID DefineControls(CfxFiler &F);
    VOID DefineControlsState(CfxFiler &F);

    VOID OnBackClick(CfxControl *pSender, VOID *pParams);
    VOID OnNextClick(CfxControl *pSender, VOID *pParams);
    VOID OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    VOID UpdateButtons();
public:
    CfxControl_Notebook(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Notebook();

    XFONT *GetDefaultFont();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
    
    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    VOID SetOnPageChange(CfxControl *pControl, NotifyMethod OnPageChange);

    CfxControl *GetChildParent()        { return GetActivePage(); }

    VOID SetActivePageIndex(UINT Index);
    INT GetActivePageIndex()            { return (INT)_activeIndex;                                      }
    CfxControl *GetActivePage()         { return _activeIndex != -1 ? _pages.Get(_activeIndex-1) : this; }
    CfxControl *GetPage(UINT Index)     { return Index != -1 ? _pages.Get(Index) : NULL;                 }
    UINT GetPageCount()                 { return (UINT)_pages.GetCount();                                }

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID OnPenClick(INT X, INT Y);

    VOID ProcessPages();
};

extern CfxControl *Create_Control_Notebook(CfxPersistent *pOwner, CfxControl *pParent);

