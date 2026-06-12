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
	bool _penDown = false, _penOver = false;
    int _pageSize = 0, _stepSize = 0, _minValue = 0, _maxValue = 0, _value = 0;
    int _downX = 0, _downY = 0, _downValue = 0;
    FXEVENT _onChange;
    void ExecuteChange();
    virtual void GetEmptyBounds(FXRECT *pRect)=0;
    virtual void GetThumbBounds(FXRECT *pRect)=0;
    virtual void MoveThumb1(int DownX, int DownY, int DownValue, int X, int Y)=0;
    virtual void MoveThumb2(int X, int Y)=0;
public:
    CfxControl_ScrollBar(CfxPersistent *pOwner, CfxControl *pParent);

    void ResetState() override;
    void DefineState(CfxFiler &F) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    void SetOnChange(CfxControl *pControl, NotifyMethod OnChange);

	void OnPenDown(int X, int Y) override;
    void OnPenUp(int X, int Y) override;
    void OnPenMove(int X, int Y) override;
    void OnPenClick(int X, int Y) override;

    int GetMinValue()            { return _minValue;  }
    int GetMaxValue()            { return _maxValue;  }
    int GetValue()               { return _value;     }
    void SetValue(int Value);

    void StepUp();
    void StepDown();

    void PageUp();
    void PageDown();

    void Setup(int MinValue, int MaxValue, int PageSize, int StepSize);
};

//
// CfxControl_ScrollBarV
//
class CfxControl_ScrollBarV: public CfxControl_ScrollBar
{
protected:
    void GetThumbBounds(FXRECT *pRect);
    void GetEmptyBounds(FXRECT *pRect);
    void MoveThumb1(int DownX, int DownY, int DownValue, int X, int Y);
    void MoveThumb2(int X, int Y);
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
    int _buttonHeight = 0;
    CfxControl_Button *_backButton = nullptr, *_nextButton = nullptr;
    CfxControl_ScrollBarV *_scrollBar = nullptr;
    void OnBackClick(CfxControl *pSender, void *pParams);
    void OnNextClick(CfxControl *pSender, void *pParams);
    void OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void FixColors();
public:
    CfxControl_ScrollBarV2(CfxPersistent *pOwner, CfxControl *pParent);
    
    void SetOnChange(CfxControl *pControl, NotifyMethod OnChange)   { _scrollBar->SetOnChange(pControl, OnChange); }

    int GetMinValue()            { return _scrollBar->GetMinValue();  }
    int GetMaxValue()            { return _scrollBar->GetMaxValue();  }
    int GetValue()               { return _scrollBar->GetValue();     }
    void SetValue(int Value)     { _scrollBar->SetValue(Value);       }
    void PageUp()                  { _scrollBar->PageUp();              }
    void PageDown()                { _scrollBar->PageDown();            }
    void Setup(int MinValue, int MaxValue, int PageSize, int StepSize) { _scrollBar->Setup(MinValue, MaxValue, PageSize, StepSize); }

    void SetColor(COLOR Color)       { CfxControl::SetColor(Color); FixColors(); }
    void SetBorderColor(COLOR Color) { CfxControl::SetBorderColor(Color); FixColors(); }

    void ResetState() override;
    void DefineState(CfxFiler &F) override;
    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
    void SetBounds(int Left, int Top, int Width, int Height) override;

    void OnKeyDown(BYTE KeyCode, bool *pHandled) override;
};

extern CfxControl *Create_Control_ScrollBarV2(CfxPersistent *pOwner, CfxControl *pParent);

int ComputeScrollBarWidth(int Width, int BorderLineWidth, int ScaleX, int ScaleHitSize);
