#pragma once
#include "pch.h"
#include "fxControl.h"
#include "fxScrollBar.h"

const GUID GUID_CONTROL_MEMO = {0xf4d19e36, 0xbc93, 0x4d89, {0xb8, 0x2b, 0x1a, 0x89, 0x0, 0x71, 0x0, 0x77}};

//
// CfxControl_Memo
//
class CfxControl_Memo: public CfxControl
{
protected:
    char *_caption = nullptr;
    COLOR _textColor = 0;
    bool _wordWrap = false;
    bool _autoHeight = false;
    int _minHeight = 0;
    int _alignment = 0;
    bool _rightToLeft = false;
    FXEVENT _onClick;

    int _scrollBarWidth = 0;
    CfxControl_ScrollBarV2 *_scrollBar = nullptr;
    int GetScrollBarWidth();
    void UpdateScrollBar();
public:
    CfxControl_Memo(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Memo();

    virtual char *GetCaption()       { return _caption;     }
    virtual void SetCaption(const char *Caption); 
    COLOR GetTextColor()             { return _textColor;   }
    void SetTextColor(COLOR Color)   { _textColor = Color;  }
    bool GetWordWrap()               { return _wordWrap;    }
    void SetWordWrap(bool Value)     { _wordWrap = Value;   }
    bool GetAutoHeight()             { return _autoHeight;  }
    void SetAutoHeight(bool Value)   { _autoHeight = Value; }
    int GetAlignment()            { return _alignment;   }
    void SetAlignment(int Value)  { _alignment = Value;  }

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
    void SetBounds(int Left, int Top, int Width, int Height) override;

    void SetOnClick(CfxPersistent *pObject, NotifyMethod OnClick);

    void OnPenClick(int X, int Y) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

extern CfxControl *Create_Control_Memo(CfxPersistent *pOwner, CfxControl *pParent);

