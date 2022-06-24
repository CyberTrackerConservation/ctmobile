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
#include "fxApplication.h"

const GUID GUID_CONTROL_SYSTEM = {0xbc657fa, 0xc6c4, 0x4348, {0x99, 0x5, 0x20, 0x20, 0x23, 0x41, 0x66, 0x2c}};

enum SystemControlStyle {scsBattery=0, scsTime};

//
// CfxControl_System
//
class CfxControl_System: public CfxControl
{
protected:
    CHAR *_caption;
    COLOR _textColor;
    UINT _minHeight;
    UINT _alignment;
    SystemControlStyle _style;

public:
    CfxControl_System(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_System();

    CHAR *GetCaption()               { return _caption;    }
    VOID SetCaption(CHAR *pCaption); 
    COLOR GetTextColor()             { return _textColor;  }
    VOID SetTextColor(COLOR Color)   { _textColor = Color; }
    UINT GetMinHeight()            { return _minHeight;  }
    VOID SetMinHeight(UINT Value)  { _minHeight = Value; }

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
    
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

extern CfxControl *Create_Control_System(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
