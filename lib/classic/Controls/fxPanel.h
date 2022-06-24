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

const GUID GUID_CONTROL_PANEL = {0x1baf7223, 0x9df3, 0x44e2, {0x8b, 0x0e, 0x96, 0x9d, 0x95, 0x14, 0x92, 0xac}};

// Data for the paint event
struct FXPAINTDATA
{
    CfxCanvas *Canvas;
    FXRECT *Rect;
    BOOL DefaultPaint;
};

//
// CfxControl_Panel
//
class CfxControl_Panel: public CfxControl
{
protected:
    CHAR *_caption;
    COLOR _textColor;
    BOOL _autoHeight;
    UINT _minHeight;
    UINT _alignment;
    BOOL _useScreenName;
    FXEVENT _onClick, _onPaint, _onResize;
    VOID UpdateHeight();
public:
    CfxControl_Panel(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Panel();

    CHAR *GetCaption()               { return _caption;     }
    CHAR *GetVisibleCaption()        { return _useScreenName ? GetParentScreen(this)->GetName() : _caption; }
    VOID SetCaption(const CHAR *pCaption); 
    COLOR GetTextColor()             { return _textColor;   }
    VOID SetTextColor(COLOR Color);
    UINT GetMinHeight()              { return _minHeight;   }
    VOID SetMinHeight(UINT Value)    { _minHeight = Value;  }

    BOOL GetAutoHeight()             { return _autoHeight;  }
    VOID SetAutoHeight(BOOL Value)   { _autoHeight = Value; }

    UINT GetAlignment()              { return _alignment;   }
    VOID SetAlignment(UINT Value)    { _alignment = Value;  }

    BOOL GetUseScreenName()          { return _useScreenName;  }
    VOID SetUseScreenName(BOOL Value) { _useScreenName = TRUE; }

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    
    VOID SetOnClick(CfxPersistent *pObject, NotifyMethod OnClick);
    VOID SetOnPaint(CfxPersistent *pObject, NotifyMethod OnPaint);
    VOID SetOnResize(CfxPersistent *pObject, NotifyMethod OnResize);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPenClick(INT X, INT Y);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

extern CfxControl *Create_Control_Panel(CfxPersistent *pOwner, CfxControl *pParent);

//*************************************************************************************************
