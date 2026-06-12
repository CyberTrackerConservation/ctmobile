#pragma once
#include "pch.h"
#include "fxControl.h"
#include "fxApplication.h"

const GUID GUID_CONTROL_SYSTEM = {0xbc657fa, 0xc6c4, 0x4348, {0x99, 0x5, 0x20, 0x20, 0x23, 0x41, 0x66, 0x2c}};

enum class SystemControlStyle : int8_t {Battery=0, Time};

//
// CfxControl_System
//
class CfxControl_System: public CfxControl
{
protected:
    char *_caption = nullptr;
    COLOR _textColor = 0;
    int _minHeight = 0;
    int _alignment = 0;
    SystemControlStyle _style = SystemControlStyle::Battery;

public:
    CfxControl_System(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_System();

    char *GetCaption()               { return _caption;    }
    void SetCaption(char *pCaption); 
    COLOR GetTextColor()             { return _textColor;  }
    void SetTextColor(COLOR Color)   { _textColor = Color; }
    int GetMinHeight()            { return _minHeight;  }
    void SetMinHeight(int Value)  { _minHeight = Value; }

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
    
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

extern CfxControl *Create_Control_System(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
