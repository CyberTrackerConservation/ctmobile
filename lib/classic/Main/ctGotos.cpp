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
#include "fxGPS.h"
#include "fxMath.h"
#include "ctGotos.h"
#include "ctSession.h"
#include "ctActions.h"

//*************************************************************************************************
// CctControl_GotoList

CfxControl *Create_Control_GotoList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_GotoList(pOwner, pParent);
}

CctControl_GotoList::CctControl_GotoList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_GOTOLIST);
    InitLockProperties("");

    _textColor = _borderColor;

    _selectedIndex = -1;
    _rightToLeft = FALSE;
    memset(&_cachePosition, 0, sizeof(_cachePosition));

    SetCaption("Goto list");
}

CctControl_GotoList::~CctControl_GotoList()
{
}

VOID CctControl_GotoList::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_List::DefineResources(F);
#endif
}

VOID CctControl_GotoList::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    F.DefineValue("RightToLeft", dtBoolean, &_rightToLeft, F_FALSE);

    if (F.IsReader())
    {
        UpdateScrollBar();
    }
}

VOID CctControl_GotoList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
}

VOID CctControl_GotoList::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Columns",      dtInt32,     &_columnCount, "MIN(1);MAX(8)");
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Item height",  dtInt32,     &_itemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Minimum item height", dtInt32, &_minItemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Minimum item width",  dtInt32, &_minItemWidth,   "MIN(32);MAX(640)");
    F.DefineValue("Right to left",dtBoolean,   &_rightToLeft);
    F.DefineValue("Scroll width", dtInt32,     &_scrollBarWidth,    "MIN(7);MAX(40)");
    F.DefineValue("Show lines",   dtBoolean,   &_showLines);
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}

UINT CctControl_GotoList::GetItemCount()
{
    UINT Count = 0;

    if (IsLive())
    {
        CctSession *session = GetSession(this);
        Count = 1;  // None
        Count += session->GetCustomGotoCount(); // Custom
        Count += session->GetResourceHeader()->GotosCount;  // Static
    }

    return Count;
}

FXGOTO *CctControl_GotoList::GetItem(UINT Index, BOOL *pBoundary)
{
    *pBoundary = FALSE;

    if (Index == 0)
    {
        memset(&_cacheItem, 0, sizeof(FXGOTO));
        strcpy(_cacheItem.Title, "---");
        return &_cacheItem;
    }

    CctSession *session = GetSession(this);
    UINT customCount = session->GetCustomGotoCount();

    Index--;
    if (Index < customCount)
    {
        *pBoundary = (Index == 0);
        return session->GetCustomGoto(Index);
    }

    Index -= customCount;
    CfxResource *resource = GetResource();
    FXGOTO *pGoto = resource->GetGoto(this, Index);
    if (pGoto)
    {
        _cacheItem = *pGoto;
        resource->ReleaseGoto(pGoto);
    }

    *pBoundary = (Index == 0);
    return &_cacheItem;
}

VOID CctControl_GotoList::PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index)
{
    INT itemHeight = pItemRect->Bottom - pItemRect->Top + 1;

    // Paint if selected
    BOOL selected = ((INT)Index == _selectedIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);
    }

    // Draw the caption
    UINT flags = TEXT_ALIGN_VCENTER;
    FXRECT rect = *pItemRect;
    rect.Right -= 2 * _oneSpace;
    rect.Left += 2 * _oneSpace;

    if (Index == 0)
    {
        flags |= TEXT_ALIGN_HCENTER;
    }
    else if (_rightToLeft)
    {
        flags |= TEXT_ALIGN_RIGHT;
    }

    pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);

    BOOL boundary = FALSE;
    FXGOTO *pGoto = GetItem(Index, &boundary);
    if (pGoto)
    {
        pCanvas->DrawTextRect(pFont, &rect, flags, pGoto->Title);

        if ((Index > 0) && TestGPS(&_cachePosition))
        {
            UINT tw = pCanvas->TextWidth(pFont, "00000000 km");
            flags = TEXT_ALIGN_VCENTER;
            if (_rightToLeft)
            {
                rect.Right = rect.Left + tw;
                flags |= TEXT_ALIGN_LEFT;
            }
            else
            {
                rect.Left = rect.Right - tw;
                flags |= TEXT_ALIGN_RIGHT;
            }

            DOUBLE d = CalcDistanceKm(pGoto->Y, pGoto->X, _cachePosition.Latitude, _cachePosition.Longitude);
            
            CHAR caption[16], captionF[16];
            if ((INT)d == 0)
            {
                d *= 1000;
                sprintf(caption, "%ld m", (INT)d);
            }
            else 
            {
                PrintF(captionF, d, sizeof(captionF)-1, d < 10 ? 3 : 1); 
                sprintf(caption, "%s km", captionF);
            }

            pCanvas->DrawTextRect(pFont, &rect, flags, caption);
        }
    }
    else
    {
        pCanvas->DrawTextRect(pFont, &rect, flags, "Bad waypoint index");
    }

    if (boundary)
    {
        FXRECT rect = *pItemRect;
        rect.Bottom = rect.Top + _oneSpace;
        pCanvas->State.BrushColor = _borderColor;
        pCanvas->FillRect(&rect);
    }
}

VOID CctControl_GotoList::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (IsLive())
    {
        SetPaintTimer(2000);
        _cachePosition = GetEngine(this)->GetGPS()->Position;
        CfxControl_List::OnPaint(pCanvas, pRect);
    }
    else
    {
        CfxControl_Panel::OnPaint(pCanvas, pRect);
    }
}

VOID CctControl_GotoList::OnPenDown(INT X, INT Y)
{
    INT index;
    INT offsetX, offsetY;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        _selectedIndex = index;

        if (index == 0)
        {
            FXGOTO gotoNone = {MAGIC_GOTO};
            GetEngine(this)->SetCurrentGoto(&gotoNone);
        }
        else
        {
            BOOL boundary = FALSE;
            FXGOTO *pGoto = GetItem(index, &boundary);
            if (pGoto)
            {
                GetEngine(this)->SetCurrentGoto(pGoto);
            }
        }

        Repaint();
    }
}

//*************************************************************************************************
// CctGoto_Painter

UINT CctGoto_Painter::GetItemCount()
{
    return 3;
}

VOID CctGoto_Painter::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index)
{
    CHAR caption[32];

    UINT itemHeight = pRect->Bottom - pRect->Top + 1;

    // Left side rect
    FXRECT lRect = *pRect;
    lRect.Left += (2 * _parent->GetBorderLineWidth()) + itemHeight;
    lRect.Right = pRect->Left + (pRect->Right - pRect->Left + 1) / 2;

    // Right side rect
    FXRECT rRect = *pRect;
    rRect.Left = lRect.Right + 1;

    switch (Index)
    {
    case 0:
        pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, "Title");
        pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, _goto.Title);
        break;
    case 1:
        pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, "Latitude");
        PrintF(caption, _goto.Y, sizeof(caption)-1, 6);
        pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, caption);
        break;
    case 2:
        pCanvas->DrawTextRect(pFont, &lRect, TEXT_ALIGN_VCENTER, "Longitude");
        PrintF(caption, _goto.X, sizeof(caption)-1, 6);
        pCanvas->DrawTextRect(pFont, &rRect, TEXT_ALIGN_VCENTER, caption);
        break;
    }
}
