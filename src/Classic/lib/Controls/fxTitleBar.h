#pragma once
#include "pch.h"
#include "fxPanel.h"

const GUID GUID_CONTROL_TITLEBAR = {0xb318f5ab, 0x6d56, 0x46bd, {0x8c, 0xc4, 0x10, 0x63, 0xd2, 0x60, 0x9f, 0x85}};

//
// CfxControl_TitleBar
//
class CfxControl_TitleBar: public CfxControl_Panel
{
protected:
    XFONT _bigFont = 0;

	FXRECT _menuBounds, _okayBounds, _exitBounds;
    int _batteryWidth = 0, _batteryPixel = 0;
    bool _penDownMenu = false, _penDownOkay = false, _penDownExit = false, _penOverMenu = false, _penOverOkay = false, _penOverExit = false;
    bool _showMenu = false, _showOkay = false, _showExit = false, _showTime = false, _showBattery = false;
    void DrawBattery(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect);
    void DrawMenu(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect);
    void DrawOkay(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect);
    void DrawExit(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect);
    void DrawTime(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect);

    FXFONTRESOURCE *GetFont(CfxResource *pResource);
public:
    CfxControl_TitleBar(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_TitleBar();

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
    void SetBounds(int Left, int Top, int Width, int Height) override;

    bool GetShowMenu()              { return _showMenu;     }
    void SetShowMenu(bool Value)    { _showMenu = Value;    }
    bool GetShowExit()              { return _showExit;     }
    void SetShowExit(bool Value)    
    { 
        _showExit = Value;
    }
    bool GetShowOkay()              { return _showOkay;     }
    void SetShowOkay(bool Value)    { _showOkay = Value;    }
    bool GetShowTime()              { return _showTime;     }
    void SetShowTime(bool Value)    { _showTime = Value;    }
    bool GetShowBattery()           { return _showBattery;  }
    void SetShowBattery(bool Value) { _showBattery = Value; }

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

	void OnPenDown(int X, int Y) override;
    void OnPenUp(int X, int Y) override;
    void OnPenMove(int X, int Y) override;
    void OnPenClick(int X, int Y) override;

    void DefineResources(CfxFilerResource &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
};

extern CfxControl *Create_Control_TitleBar(CfxPersistent *pOwner, CfxControl *pParent);

