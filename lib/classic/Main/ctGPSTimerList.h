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
#include "fxList.h"

const GUID GUID_CONTROL_GPSTIMERLIST = {0xa65f0e62, 0x76a, 0x43b6, {0x80, 0xf2, 0xc3, 0x7a, 0x57, 0x4f, 0x91, 0xf3}};

struct GPSTIMERITEM
{
    CHAR *Caption;
    UINT Timeout;
    BOOL Enabled;
};

const GPSTIMERITEM GpsTimerItems[] = 
{
    { "Off",        0,    TRUE },
    { "1 second",   1,    TRUE },
    { "5 seconds",  5,    TRUE },
    { "10 seconds", 10,   TRUE },
    { "30 seconds", 30,   TRUE },
    { "1 minute",   60,   TRUE },
    { "2 minutes",  60*2, TRUE },
    { "3 minutes",  60*3, TRUE },
    { "5 minutes",  60*5, TRUE }
};

//
// CctControl_GPSTimerList
//
class CctControl_GPSTimerList: public CfxControl_List
{
protected:
    BOOL _autoNext;
    BOOL _showCaption;
    INT _selectedIndex;
    XFONT _valueFont;
    BOOL _rightToLeft;
    GPSTIMERITEM _items[ARRAYSIZE(GpsTimerItems)];
    INT _itemMap[ARRAYSIZE(GpsTimerItems)];
    UINT _itemMapCount;

    VOID RebuildItemMap();
    VOID ClearItems();
    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_GPSTimerList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_GPSTimerList();

    VOID OnPenDown(INT X, INT Y);
    VOID OnPenClick(INT X, INT Y);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    UINT GetItemCount();
    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index);
};

extern CfxControl *Create_Control_GPSTimerList(CfxPersistent *pOwner, CfxControl *pParent);

