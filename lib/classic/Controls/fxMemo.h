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
#include "fxScrollBar.h"

const GUID GUID_CONTROL_MEMO = {0xf4d19e36, 0xbc93, 0x4d89, {0xb8, 0x2b, 0x1a, 0x89, 0x0, 0x71, 0x0, 0x77}};

//
// CfxControl_Memo
//
class CfxControl_Memo: public CfxControl
{
protected:
    CHAR *_caption;
    COLOR _textColor;
    BOOL _wordWrap;
    BOOL _autoHeight;
    UINT _minHeight;
    UINT _alignment;
    BOOL _rightToLeft;
    FXEVENT _onClick;

    UINT _scrollBarWidth;
    CfxControl_ScrollBarV2 *_scrollBar;
    UINT GetScrollBarWidth();
    VOID UpdateScrollBar();
public:
    CfxControl_Memo(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Memo();

    virtual CHAR *GetCaption()       { return _caption;     }
    virtual VOID SetCaption(const CHAR *Caption); 
    COLOR GetTextColor()             { return _textColor;   }
    VOID SetTextColor(COLOR Color)   { _textColor = Color;  }
    BOOL GetWordWrap()               { return _wordWrap;    }
    VOID SetWordWrap(BOOL Value)     { _wordWrap = Value;   }
    BOOL GetAutoHeight()             { return _autoHeight;  }
    VOID SetAutoHeight(BOOL Value)   { _autoHeight = Value; }
    UINT GetAlignment()            { return _alignment;   }
    VOID SetAlignment(UINT Value)  { _alignment = Value;  }

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);

    VOID SetOnClick(CfxPersistent *pObject, NotifyMethod OnClick);

    VOID OnPenClick(INT X, INT Y);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

extern CfxControl *Create_Control_Memo(CfxPersistent *pOwner, CfxControl *pParent);

