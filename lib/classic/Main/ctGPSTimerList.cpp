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
#include "ctGPSTimerList.h"
#include "ctActions.h"

CfxControl *Create_Control_GPSTimerList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_GPSTimerList(pOwner, pParent);
}

//*************************************************************************************************
// CctControl_GPSTimerList

CctControl_GPSTimerList::CctControl_GPSTimerList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_GPSTIMERLIST);
    InitLockProperties("Columns;ItemHeight;Show caption");

    for (UINT i = 0; i < ARRAYSIZE(_items); i++)
    {
        _items[i].Caption = AllocString(GpsTimerItems[i].Caption);
        _items[i].Timeout = GpsTimerItems[i].Timeout;
        _items[i].Enabled = TRUE;
    }
    RebuildItemMap();

    _rightToLeft = FALSE;
    _autoNext = FALSE;
    _showCaption = TRUE;
    _selectedIndex = -1;
    InitFont(_valueFont, GPS_WAYPOINT_FONT);
   
    // Register for CanNext and Next events
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_GPSTimerList::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_GPSTimerList::OnSessionNext);
}

CctControl_GPSTimerList::~CctControl_GPSTimerList()
{
    ClearItems();

    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);
}

VOID CctControl_GPSTimerList::ClearItems()
{
    _itemMapCount = 0;
    memset(&_itemMap, 0xFF, sizeof(_itemMap));

    for (UINT i = 0; i < ARRAYSIZE(_items); i++)
    {
        MEM_FREE(_items[i].Caption);
        _items[i].Caption = NULL;
    }

    memset(&_items, 0, sizeof(_items));
}

VOID CctControl_GPSTimerList::RebuildItemMap()
{
    _itemMapCount = 0;
    memset(&_itemMap, 0xFF, sizeof(_itemMap));

    for (UINT i = 0; i < ARRAYSIZE(_items); i++)
    {
        if (_items[i].Enabled)
        {
            _itemMap[_itemMapCount] = i;
            _itemMapCount++;
        }
    }
}

VOID CctControl_GPSTimerList::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (_selectedIndex == -1)
    {
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_GPSTimerList::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    if (_selectedIndex == -1) return;

    CctAction_ConfigureGPS action(this);

    CctSession *session = (CctSession *)pSender;
    action.Setup(FALSE, session->GetSightingAccuracy(), session->GetSightingFixCount(),
                 session->GetWaypointManager()->GetAccuracy(), _items[_itemMap[_selectedIndex]].Timeout,
                 GetEngine(this)->GetUseRangeFinderForAltitude());

    session->ExecuteAction(&action);
}

VOID CctControl_GPSTimerList::OnPenDown(INT X, INT Y)
{
    INT index;
    INT offsetX, offsetY;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        _selectedIndex = (index != -1) ? index : _selectedIndex;

        // Repaint to show changed state
        Repaint();
    }
}

VOID CctControl_GPSTimerList::OnPenClick(INT X, INT Y)
{
    if (_autoNext && (_selectedIndex != -1))
    {
        GetEngine(this)->KeyPress(KEY_RIGHT);
    }
}

VOID CctControl_GPSTimerList::DefineResources(CfxFilerResource &F)
{
    #ifdef CLIENT_DLL
        CfxControl_List::DefineResources(F);
        F.DefineFont(_valueFont);
    #endif
}

VOID CctControl_GPSTimerList::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    F.DefineValue("AutoNext", dtBoolean, &_autoNext, F_FALSE);
    F.DefineValue("RightToLeft", dtBoolean, &_rightToLeft, F_FALSE);
    F.DefineValue("ShowCaption", dtBoolean, &_showCaption, F_TRUE);
    F.DefineXFONT("ValueFont", &_valueFont, GPS_WAYPOINT_FONT);

    for (UINT i = 0; i < ARRAYSIZE(_items); i++)
    {
        CHAR index[5], name[32];
        itoa(i + 1, index, 10);

        strcpy(name, "Caption_");
        strcat(name, index);
        F.DefineValue(name, dtPText, &_items[i].Caption);

        strcpy(name, "Timeout_");
        strcat(name, index);
        F.DefineValue(name, dtInt32, &_items[i].Timeout);

        strcpy(name, "Enabled_");
        strcat(name, index);
        F.DefineValue(name, dtBoolean, &_items[i].Enabled);
    }

    if (F.IsReader())
    {
        RebuildItemMap();

        _selectedIndex = -1;
        UpdateScrollBar();
    }
}

VOID CctControl_GPSTimerList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
}

VOID CctControl_GPSTimerList::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Auto next",    dtBoolean,   &_autoNext);
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Columns",      dtInt32,     &_columnCount, "MIN(1);MAX(8)");
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    for (UINT i = 0; i < ARRAYSIZE(_items); i++)
    {
        CHAR index[5], name[32];
        itoa(i + 1, index, 10);
        strcpy(name, "Item_Caption_");
        strcat(name, index);
        F.DefineValue(name, dtPText, &_items[i].Caption);

        strcpy(name, "Item_Enabled_");
        strcat(name, index);
        F.DefineValue(name, dtBoolean, &_items[i].Enabled);

        strcpy(name, "Item_Timeout_");
        strcat(name, index);
        F.DefineValue(name, dtInt32, &_items[i].Timeout, "MIN(0);MAX(600)");
    }
    F.DefineValue("Item height",  dtInt32,     &_itemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Minimum item height", dtInt32, &_minItemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Minimum item width",  dtInt32, &_minItemWidth,   "MIN(32);MAX(640)");
    F.DefineValue("Right to left", dtBoolean,  &_rightToLeft);
    F.DefineValue("Scroll width", dtInt32,     &_scrollBarWidth);
    F.DefineValue("Show caption", dtBoolean,   &_showCaption);
    F.DefineValue("Show lines",   dtBoolean,   &_showLines);
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Value Font",   dtFont,      &_valueFont);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}

UINT CctControl_GPSTimerList::GetItemCount()
{
    return _itemMapCount;
}

VOID CctControl_GPSTimerList::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index)
{
    CfxResource *resource = GetResource();
    INT itemHeight = pRect->Bottom - pRect->Top + 1;

    // Paint if selected
    BOOL selected = ((INT)Index == _selectedIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pRect);
    }

    // Draw the caption
    if (_showCaption)
    {
        FXRECT rect = *pRect;
        UINT flags = TEXT_ALIGN_VCENTER;

        if (_rightToLeft)
        {
            rect.Right -= 2 * _oneSpace;
            flags |= TEXT_ALIGN_RIGHT;
        }
        else
        {
            rect.Left += 2 * _oneSpace;
        }
        
        pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);
        pCanvas->DrawTextRect(pFont, &rect, flags, _items[_itemMap[Index]].Caption);
    }

    {
        FXRECT rect = *pRect;
        INT itemHeight = rect.Bottom - rect.Top + 1;

        if (_showCaption)
        {
            if (_rightToLeft)
            {
                rect.Right = rect.Left + itemHeight;
            }
            else
            {
                rect.Left = rect.Right - itemHeight;
            }
        }
        else
        {
            rect.Left = rect.Left + (rect.Right - rect.Left - itemHeight) / 2;
            rect.Right = rect.Left + itemHeight;
        }

        InflateRect(&rect, -_oneSpace, -_oneSpace);

        FXFONTRESOURCE *font = resource->GetFont(this, _valueFont);
        if (font)
        {
            pCanvas->State.LineWidth = GetBorderLineWidth();
            DrawWaypointIcon(pCanvas, &rect, font, _items[_itemMap[Index]].Timeout, _color, _textColor, FALSE, _items[_itemMap[Index]].Timeout == 0, selected);
            resource->ReleaseFont(font);
        }
    }
}
