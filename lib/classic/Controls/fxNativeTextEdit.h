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

const GUID GUID_CONTROL_NATIVETEXTEDIT = {0x36a9dc11, 0xb912, 0x4b34, {0xac, 0x5b, 0x46, 0x44, 0xa7, 0xf0, 0x32, 0xa4}};

//
// CfxControl_NativeTextEdit
//
class CfxControl_NativeTextEdit: public CfxControl
{
protected:
    CHAR *_caption;
    COLOR _textColor;
    HANDLE _handle;
public:
    CfxControl_NativeTextEdit(CfxPersistent *pOwner, CfxControl *pParent, BOOL SingleLine = FALSE, BOOL Password = FALSE);
    ~CfxControl_NativeTextEdit();

    CHAR *GetCaption();
    VOID SetCaption(CHAR *pValue); 
    COLOR GetTextColor()            { return _textColor;  }
    VOID SetTextColor(COLOR Value)  { _textColor = Value; }

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    VOID SetFocus();
};

extern CfxControl *Create_Control_NativeTextEdit(CfxPersistent *pOwner, CfxControl *pParent);
