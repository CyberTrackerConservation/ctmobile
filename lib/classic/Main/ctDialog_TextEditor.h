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
#include "fxTitleBar.h"
#include "fxNativeTextEdit.h"

struct TEXTEDITOR_PARAMS
{
    UINT ControlId;
    CHAR *Text;
    XFONT Font;
};

//*************************************************************************************************

const GUID GUID_DIALOG_TEXTEDITOR = {0x860000b3, 0x7f68, 0x4cd2, {0x97, 0xfe, 0xff, 0x7f, 0x49, 0x61, 0xc1, 0x2d}};

//
// CctDialog_TextEditor
//
class CctDialog_TextEditor: public CfxDialog
{
protected:
    TEXTEDITOR_PARAMS _source;

    CfxControl_TitleBar *_titleBar;
    CfxControl_NativeTextEdit *_editor;
public:
    CctDialog_TextEditor(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctDialog_TextEditor();

    VOID Init(CfxControl *pSender, VOID *pParam, UINT ParamSize);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);

    VOID OnCloseDialog(BOOL *pHandled);
};

extern CfxDialog *Create_Dialog_TextEditor(CfxPersistent *pOwner, CfxControl *pParent);
