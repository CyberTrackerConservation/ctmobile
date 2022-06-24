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

const GUID GUID_CONTROL_TITLEBAR = {0xb318f5ab, 0x6d56, 0x46bd, {0x8c, 0xc4, 0x10, 0x63, 0xd2, 0x60, 0x9f, 0x85}};

//
// CfxControl_TitleBar
//
class CfxControl_TitleBar: public CfxControl_Panel
{
protected:
    XFONT _bigFont;

	FXRECT _menuBounds, _okayBounds, _exitBounds;
    INT _batteryWidth, _batteryPixel;
    BOOL _penDownMenu, _penDownOkay, _penDownExit, _penOverMenu, _penOverOkay, _penOverExit;
    BOOL _showMenu, _showOkay, _showExit, _showTime, _showBattery;
    VOID DrawBattery(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect);
    VOID DrawMenu(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect);
    VOID DrawOkay(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect);
    VOID DrawExit(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect);
    VOID DrawTime(CfxCanvas *pCanvas, FXFONTRESOURCE *pFont, FXRECT *pRect);

    FXFONTRESOURCE *GetFont(CfxResource *pResource);
public:
    CfxControl_TitleBar(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_TitleBar();

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);

    BOOL GetShowMenu()              { return _showMenu;     }
    VOID SetShowMenu(BOOL Value)    { _showMenu = Value;    }
    BOOL GetShowExit()              { return _showExit;     }
    VOID SetShowExit(BOOL Value)    
    { 
        _showExit = Value;
        #ifdef CLIENT_DLL
        _showExit = FALSE;    
        #endif
    }
    BOOL GetShowOkay()              { return _showOkay;     }
    VOID SetShowOkay(BOOL Value)    { _showOkay = Value;    }
    BOOL GetShowTime()              { return _showTime;     }
    VOID SetShowTime(BOOL Value)    { _showTime = Value;    }
    BOOL GetShowBattery()           { return _showBattery;  }
    VOID SetShowBattery(BOOL Value) { _showBattery = Value; }

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

	VOID OnPenDown(INT X, INT Y);
    VOID OnPenUp(INT X, INT Y);
    VOID OnPenMove(INT X, INT Y);
    VOID OnPenClick(INT X, INT Y);

    VOID DefineResources(CfxFilerResource &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
};

extern CfxControl *Create_Control_TitleBar(CfxPersistent *pOwner, CfxControl *pParent);

