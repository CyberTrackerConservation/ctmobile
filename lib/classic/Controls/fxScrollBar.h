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
#include "fxPanel.h"
#include "fxButton.h"

const GUID GUID_CONTROL_SCROLLBARV  = {0xadd48047, 0xd466, 0x48e7, {0x9b, 0x5f, 0xc5, 0x25, 0xc3, 0x88, 0x5a, 0x70}};
const GUID GUID_CONTROL_SCROLLBARV2 = {0x299cf1a3, 0x6071, 0x4d24, {0x8c, 0x6c, 0x40, 0x1c, 0x4d, 0xa4, 0x7c, 0x9d}};

//
// CfxControl_ScrollBar
//
class CfxControl_ScrollBar: public CfxControl
{
protected:
	BOOL _penDown, _penOver;
    INT _pageSize, _stepSize, _minValue, _maxValue, _value;
    INT _downX, _downY, _downValue;
    FXEVENT _onChange;
    VOID ExecuteChange();
    virtual VOID GetEmptyBounds(FXRECT *pRect)=0;
    virtual VOID GetThumbBounds(FXRECT *pRect)=0;
    virtual VOID MoveThumb1(INT DownX, INT DownY, INT DownValue, INT X, INT Y)=0;
    virtual VOID MoveThumb2(INT X, INT Y)=0;
public:
    CfxControl_ScrollBar(CfxPersistent *pOwner, CfxControl *pParent);

    VOID ResetState();
    VOID DefineState(CfxFiler &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    VOID SetOnChange(CfxControl *pControl, NotifyMethod OnChange);

	VOID OnPenDown(INT X, INT Y);
    VOID OnPenUp(INT X, INT Y);
    VOID OnPenMove(INT X, INT Y);
    VOID OnPenClick(INT X, INT Y);

    INT GetMinValue()            { return _minValue;  }
    INT GetMaxValue()            { return _maxValue;  }
    INT GetValue()               { return _value;     }
    VOID SetValue(INT Value);

    VOID StepUp();
    VOID StepDown();

    VOID PageUp();
    VOID PageDown();

    VOID Setup(INT MinValue, INT MaxValue, INT PageSize, INT StepSize);
};

//
// CfxControl_ScrollBarV
//
class CfxControl_ScrollBarV: public CfxControl_ScrollBar
{
protected:
    VOID GetThumbBounds(FXRECT *pRect);
    VOID GetEmptyBounds(FXRECT *pRect);
    VOID MoveThumb1(INT DownX, INT DownY, INT DownValue, INT X, INT Y);
    VOID MoveThumb2(INT X, INT Y);
public:
    CfxControl_ScrollBarV(CfxPersistent *pOwner, CfxControl *pParent);
};

extern CfxControl *Create_Control_ScrollBarV(CfxPersistent *pOwner, CfxControl *pParent);

//
// CfxControl_ScrollBarV2
//
class CfxControl_ScrollBarV2: public CfxControl
{
protected:
    UINT _buttonHeight;
    CfxControl_Button *_backButton, *_nextButton;
    CfxControl_ScrollBarV *_scrollBar;
    VOID OnBackClick(CfxControl *pSender, VOID *pParams);
    VOID OnNextClick(CfxControl *pSender, VOID *pParams);
    VOID OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID FixColors();
public:
    CfxControl_ScrollBarV2(CfxPersistent *pOwner, CfxControl *pParent);
    
    VOID SetOnChange(CfxControl *pControl, NotifyMethod OnChange)   { _scrollBar->SetOnChange(pControl, OnChange); }

    INT GetMinValue()            { return _scrollBar->GetMinValue();  }
    INT GetMaxValue()            { return _scrollBar->GetMaxValue();  }
    INT GetValue()               { return _scrollBar->GetValue();     }
    VOID SetValue(INT Value)     { _scrollBar->SetValue(Value);       }
    VOID PageUp()                  { _scrollBar->PageUp();              }
    VOID PageDown()                { _scrollBar->PageDown();            }
    VOID Setup(INT MinValue, INT MaxValue, INT PageSize, INT StepSize) { _scrollBar->Setup(MinValue, MaxValue, PageSize, StepSize); }

    VOID SetColor(COLOR Color)       { CfxControl::SetColor(Color); FixColors(); }
    VOID SetBorderColor(COLOR Color) { CfxControl::SetBorderColor(Color); FixColors(); }

    VOID ResetState();
    VOID DefineState(CfxFiler &F);
    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);

    VOID OnKeyDown(BYTE KeyCode, BOOL *pHandled);
};

extern CfxControl *Create_Control_ScrollBarV2(CfxPersistent *pOwner, CfxControl *pParent);

UINT ComputeScrollBarWidth(UINT Width, UINT BorderLineWidth, UINT ScaleX, UINT ScaleHitSize);
