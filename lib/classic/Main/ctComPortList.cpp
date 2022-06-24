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
#include "ctComPortList.h"
#include "ctActions.h"

#if defined(_WIN32) && !defined(QTBUILD)
#include "Win32_GPS.h"
#include "Win32_RangeFinder.h"
#endif

//*************************************************************************************************

CfxControl *Create_Control_ComPortList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ComPortList(pOwner, pParent);
}

//*************************************************************************************************
// CctControl_ComPortList

CctControl_ComPortList::CctControl_ComPortList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_COMPORTLIST);
    InitLockProperties("Font;ItemHeight;Port device type");

    for (UINT i = 0; i < ARRAYSIZE(_items); i++)
    {
        _items[i].Caption = AllocString(ComPortItems[i].Caption);
        _items[i].PortId = ComPortItems[i].PortId;
        _items[i].Enabled = TRUE;
    }
    RebuildItemMap();

    _rightToLeft = FALSE;
    _autoNext = FALSE;
    _selectedIndex = -1;
    _defaultPort = 255;
    _itemHeight = 20;
    _portDeviceType = pdtNone;
    _paramsId = 1;

    _resultGlobalValue = NULL;

    // Register for CanNext and Next events
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ComPortList::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ComPortList::OnSessionNext);
}

CctControl_ComPortList::~CctControl_ComPortList()
{
    MEM_FREE(_resultGlobalValue);
    _resultGlobalValue = NULL;

    ClearItems();

    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);
}

VOID CctControl_ComPortList::ClearItems()
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

VOID CctControl_ComPortList::RebuildItemMap()
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

VOID CctControl_ComPortList::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (_selectedIndex == -1)
    {
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_ComPortList::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    if (_selectedIndex == -1) return;

    // Set the global value
    if (_resultGlobalValue != NULL)
    {
        GetSession(this)->SetGlobalValue(this, _resultGlobalValue, GetPortId());
    }

#if defined(_WIN32) && !defined(QTBUILD)
    switch (_portDeviceType) 
    {
    case pdtNone:
        break;

    case pdtGps:
        g_GPS->Detect(GetPortId(), GetParamsId());
        break;

    case pdtRangeFinder:
        g_RangeFinder->Detect(GetPortId(), GetParamsId());
        break;

    default:
        FX_ASSERT(FALSE);
    }
#endif
}

VOID CctControl_ComPortList::OnPenDown(INT X, INT Y)
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

VOID CctControl_ComPortList::OnPenClick(INT X, INT Y)
{
    if (_autoNext && (_selectedIndex != -1))
    {
        GetEngine(this)->KeyPress(KEY_RIGHT);
    }
}

VOID CctControl_ComPortList::DefineResources(CfxFilerResource &F)
{
    #ifdef CLIENT_DLL
        CfxControl_List::DefineResources(F);
    #endif
}

VOID CctControl_ComPortList::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    F.DefineValue("AutoNext", dtBoolean, &_autoNext, F_FALSE);
    F.DefineValue("RightToLeft", dtBoolean, &_rightToLeft, F_FALSE);
    F.DefineValue("PortDeviceType", dtByte, &_portDeviceType, F_ZERO);
    F.DefineValue("ParamsId", dtByte, &_paramsId, F_ONE);
    F.DefineValue("DefaultPort", dtByte, &_defaultPort, "FF");
    F.DefineValue("ResultGlobalValue", dtPText, &_resultGlobalValue, "");

    if (F.IsReader())
    {
        _paramsId = max(1, _paramsId);

        RebuildItemMap();

        _selectedIndex = -1;
        if ((_defaultPort >= 0) && (_defaultPort <= ARRAYSIZE(ComPortItems)))
        {
            _selectedIndex = _defaultPort;
        }

        UpdateScrollBar();
    }
}

VOID CctControl_ComPortList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
}

VOID CctControl_ComPortList::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    CHAR items[256];

    F.DefineValue("Auto next",    dtBoolean,   &_autoNext);
    sprintf(items, "LIST(2400=%d 4800=%d 9600=%d 19200=%d 38400=%d 57600=%d 115200=%d)", 7, 1, 2, 3, 4, 5, 6);
    F.DefineValue("Baud rate", dtByte, &_paramsId, items);

    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Columns",      dtInt32,     &_columnCount, "MIN(1);MAX(8)");
    F.DefineValue("Default port", dtByte,      &_defaultPort, "LIST(None=255 Detect=0 COM1=1 COM2=2 COM3=3 COM4=4 COM5=5 COM6=6 COM7=7 COM8=8 COM9=9 COM10=10 COM11=11 COM12=12 COM13=13 COM14=14 COM15=15 COM16=16 COM17=17 COM18=18 COM19=19 COM20=20)");
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Item height",  dtInt32,     &_itemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Minimum item height", dtInt32, &_minItemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Minimum item width",  dtInt32, &_minItemWidth,   "MIN(32);MAX(640)");
    F.DefineValue("Port device type", dtByte,  (BYTE *)&_portDeviceType, "LIST(None GPS RangeFinder)");
    F.DefineValue("Right to left", dtBoolean,  &_rightToLeft);
    F.DefineValue("Result global value", dtPText, &_resultGlobalValue);
    F.DefineValue("Scroll width", dtInt32,     &_scrollBarWidth);
    F.DefineValue("Show lines",   dtBoolean,   &_showLines);
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}

UINT CctControl_ComPortList::GetItemCount()
{
    return _itemMapCount;
}

VOID CctControl_ComPortList::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index)
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
//    if (_showCaption)
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
}

BYTE CctControl_ComPortList::GetPortId()
{
    if (_selectedIndex != -1) 
    {
        return _items[_itemMap[_selectedIndex]].PortId;
    }
    else 
    {
        return 0;
    }
}

VOID CctControl_ComPortList::SetPortId(BYTE Value)
{
    if (Value == 0) 
    {
        _selectedIndex = -1;
    }
    else
    {
        _selectedIndex = Value;
    }
}
