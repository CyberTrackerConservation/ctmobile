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

#include "fxNativeTextEdit.h"
#include "fxApplication.h"
#include "fxUtils.h"

CfxControl *Create_Control_NativeTextEdit(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_NativeTextEdit(pOwner, pParent);
}

CfxControl_NativeTextEdit::CfxControl_NativeTextEdit(CfxPersistent *pOwner, CfxControl *pParent, BOOL SingleLine, BOOL Password): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NATIVETEXTEDIT, TRUE);
    InitBorder(bsSingle, 1);

    _caption = 0;
    _textColor = _borderColor;
    _handle = 0;

    if (IsLive())
    {
        _handle = GetHost(this)->CreateEditControl(SingleLine, Password);
    }
}

CfxControl_NativeTextEdit::~CfxControl_NativeTextEdit()
{
    if (_handle)
    {
        GetHost(this)->DestroyEditControl(_handle);
    }
    MEM_FREE(_caption);
}

VOID CfxControl_NativeTextEdit::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);
    
    if (IsLive())
    {
        FXRECT rect;
        GetAbsoluteBounds(&rect);
        InflateRect(&rect, -(INT)_borderWidth, -(INT)_borderWidth);
        GetHost(this)->MoveEditControl(_handle, rect.Left, rect.Top, rect.Right - rect.Left + 1, rect.Bottom - rect.Top + 1);
    }
}

CHAR *CfxControl_NativeTextEdit::GetCaption()
{ 
    if (IsLive())
    {
        MEM_FREE(_caption);
        _caption = GetHost(this)->GetEditControlText(_handle);
    }

    return _caption;    
}

VOID CfxControl_NativeTextEdit::SetCaption(CHAR *pValue)
{
    MEM_FREE(_caption);
    _caption = NULL;

    if (pValue)
    {
        _caption = (CHAR *) MEM_ALLOC(strlen(pValue) + 1);
        strcpy(_caption, pValue);
    }

    if (IsLive())
    {
        CfxHost *host = GetHost(this);
        CfxResource *resource = GetResource();
        if (resource)
        {
            FXFONTRESOURCE *font = resource->GetFont(this, _font);
            host->SetEditControlFont(_handle, font);
            resource->ReleaseFont(font);
        }

        host->SetEditControlText(_handle, _caption);
    }
}

VOID CfxControl_NativeTextEdit::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
#endif
}

VOID CfxControl_NativeTextEdit::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("TextColor",    dtColor,    &_textColor);
    F.DefineValue("Caption",      dtPText,    &_caption);
    
    if (F.IsReader())
    {
        if (_dock == dkNone)
        {
            SetBounds(_left, _top, _width, _height);
        }

        if (IsLive())
        {
            GetHost(this)->SetEditControlText(_handle, _caption);
        }
    }
}

VOID CfxControl_NativeTextEdit::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,       "LIST(None Top Bottom Left Right Fill)");
    //F.DefineValue("Font Size",    dtInt32,    &_fontSize,   "MIN(8);MAX(20)");
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CfxControl_NativeTextEdit::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!IsLive())
    {
        pCanvas->SystemDrawText("Text Box", pRect, _textColor, _color);
    }
}

VOID CfxControl_NativeTextEdit::SetFocus()
{
    if (IsLive())
    {
        GetHost(this)->FocusEditControl(_handle);
    }
}

