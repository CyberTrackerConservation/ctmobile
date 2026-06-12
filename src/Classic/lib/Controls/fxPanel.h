#pragma once
#include "pch.h"
#include "fxControl.h"
#include "fxApplication.h"

const GUID GUID_CONTROL_PANEL = {0x1baf7223, 0x9df3, 0x44e2, {0x8b, 0x0e, 0x96, 0x9d, 0x95, 0x14, 0x92, 0xac}};

// Data for the paint event
struct FXPAINTDATA
{
    CfxCanvas *Canvas = nullptr;
    FXRECT *Rect = nullptr;
    bool DefaultPaint = false;
};

//
// CfxControl_Panel
//
class CfxControl_Panel: public CfxControl
{
protected:
    char *_caption = nullptr;
    COLOR _textColor = 0;
    bool _autoHeight = false;
    int _minHeight = 0;
    int _alignment = 0;
    bool _useScreenName = false;
    FXEVENT _onClick, _onPaint, _onResize;
    void UpdateHeight();
public:
    CfxControl_Panel(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Panel();

    char *GetCaption()               { return _caption;     }
    char *GetVisibleCaption()        { return _useScreenName ? GetParentScreen(this)->GetName() : _caption; }
    void SetCaption(const char *pCaption); 
    COLOR GetTextColor()             { return _textColor;   }
    void SetTextColor(COLOR Color);
    int GetMinHeight()              { return _minHeight;   }
    void SetMinHeight(int Value)    { _minHeight = Value;  }

    bool GetAutoHeight()             { return _autoHeight;  }
    void SetAutoHeight(bool Value)   { _autoHeight = Value; }

    int GetAlignment()              { return _alignment;   }
    void SetAlignment(int Value)    { _alignment = Value;  }

    bool GetUseScreenName()          { return _useScreenName;  }
    void SetUseScreenName(bool Value) { _useScreenName = true; }

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
    void SetBounds(int Left, int Top, int Width, int Height) override;
    
    void SetOnClick(CfxPersistent *pObject, NotifyMethod OnClick);
    void SetOnPaint(CfxPersistent *pObject, NotifyMethod OnPaint);
    void SetOnResize(CfxPersistent *pObject, NotifyMethod OnResize);

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPenClick(int X, int Y) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

extern CfxControl *Create_Control_Panel(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
