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

#include "fxApplication.h"
#include "fxUtils.h"
#include "ctSession.h"
#include "ctElementFilter.h"
#include "ctElementList.h"

CfxControl *Create_Control_ElementFilter(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementFilter(pOwner, pParent);
}

CctControl_ElementFilter::CctControl_ElementFilter(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTFILTER, FALSE);
    InitLockProperties("Filter screen");
    InitBorder(bsSingle, 1);

    _caption = 0;
    _minHeight = 0;
    _minWidth = 30;

    _textColor = _borderColor;

    _filtered = FALSE;
    _penDown = _penOver = FALSE;

    InitXGuid(&_filterScreenId);

    SetCaption("??");
}

CctControl_ElementFilter::~CctControl_ElementFilter()
{
    MEM_FREE(_caption);
}

VOID CctControl_ElementFilter::SetCaption(CHAR *pCaption)
{
    MEM_FREE(_caption);
    _caption = NULL;

    if (pCaption)
    {
        _caption = (CHAR *) MEM_ALLOC(strlen(pCaption) + 1);
        strcpy(_caption, pCaption);
    }
}

VOID CctControl_ElementFilter::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
    F.DefineObject(_filterScreenId);
#endif
}

VOID CctControl_ElementFilter::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("TextColor",    dtColor,    &_textColor);
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("MinHeight",    dtInt32,    &_minHeight);
    F.DefineValue("MinWidth",     dtInt32,    &_minWidth);

    F.DefineXOBJECT("FilterScreenId", &_filterScreenId);

    _filtered = FALSE;
}

VOID CctControl_ElementFilter::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Minimum height", dtInt32,  &_minHeight, "MIN(0)");
    F.DefineValue("Minimum width", dtInt32,   &_minWidth, "MIN(0)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);

    F.DefineStaticLink("Filter screen", &_filterScreenId);
#endif
}

VOID CctControl_ElementFilter::ExecuteFilter()
{
    CfxResource *resource = GetResource(); 
    FXSCREENDATA *screenData = (FXSCREENDATA *)resource->GetObject(this, _filterScreenId);
    if (screenData)
    {
        FX_ASSERT(screenData->Magic == MAGIC_SCREEN);

        CfxStream stream(&screenData->Data[0]);
        CfxReader reader(&stream);

        CfxScreen screen(_owner, this);
        screen.DefineProperties(reader);
        
        CctControl_ElementList *elementList = (CctControl_ElementList *)screen.FindControlByType(&GUID_CONTROL_ELEMENTLIST);
        if (elementList)
        {
            CHAR value[16];
            itoa(elementList->GetItemCount(), value, 10);
            SetCaption(value);
        }
        resource->ReleaseObject(screenData);
    }

    _filtered = TRUE;
}

VOID CctControl_ElementFilter::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
	BOOL invert = _penDown && _penOver;
    
    _color = pCanvas->InvertColor(_color, invert);
    _textColor = pCanvas->InvertColor(_textColor, invert);

    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    if (!_filtered)
    {
        ExecuteFilter();
    }

    FXRECT rect = *pRect;
    rect.Left = rect.Right - _oneSpace * 10;
    pCanvas->DrawNextArrow(&rect, _color, _textColor, FALSE);

    if (_caption && (strlen(_caption) > 0))
    {
        CfxResource *resource = GetResource(); 
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            pCanvas->State.TextColor = _textColor;
            rect.Left = pRect->Left;
            rect.Right -= 4 * _oneSpace;
            pCanvas->DrawTextRect(font, &rect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, _caption);
            resource->ReleaseFont(font);
        }
    }

    _color = pCanvas->InvertColor(_color, invert);
    _textColor = pCanvas->InvertColor(_textColor, invert);
}

VOID CctControl_ElementFilter::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _height = max((INT)_minHeight, _height);
    _width = max((INT)_minWidth, _width);
}

VOID CctControl_ElementFilter::OnPenDown(INT X, INT Y)
{
	_penDown = _penOver = TRUE;
    Repaint();
}

VOID CctControl_ElementFilter::OnPenUp(INT X, INT Y)
{
    if (!_penDown) return;
    _penDown = FALSE;

    Repaint();
}

VOID CctControl_ElementFilter::OnPenMove(INT X, INT Y)
{
    if (!_penDown) return;

    if (_penOver != ((X >= 0) && (X < _width) && (Y >= 0) && (Y < _height)))
    {
		_penOver = !_penOver;
        Repaint();
	}
}

VOID CctControl_ElementFilter::OnPenClick(INT X, INT Y)
{
    if (!IsNullXGuid(&_filterScreenId))
    {
        GetSession(this)->DoSkipToScreen(_filterScreenId);
    }
}
