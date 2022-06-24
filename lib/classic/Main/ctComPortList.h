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

//*************************************************************************************************
// CctControl_ComPortList

const GUID GUID_CONTROL_COMPORTLIST = {0x945aed3d, 0x3df6, 0x4aec, {0xa3, 0xba, 0xe9, 0x6d, 0x26, 0x74, 0xaf, 0x0}};

struct COMPORTITEM
{
    CHAR *Caption;
    BYTE PortId;
    BOOL Enabled;
};

const COMPORTITEM ComPortItems[] = 
{
    { "Detect",  0, TRUE },
    { "COM 1",   1, TRUE },
    { "COM 2",   2, TRUE },
    { "COM 3",   3, TRUE },
    { "COM 4",   4, TRUE },
    { "COM 5",   5, TRUE },
    { "COM 6",   6, TRUE },
    { "COM 7",   7, TRUE },
    { "COM 8",   8, TRUE },
    { "COM 9",   9, TRUE },
#ifdef CLIENT_DLL
    { "COM 10", 10, TRUE },
    { "COM 11", 11, TRUE },
    { "COM 12", 12, TRUE },
    { "COM 13", 13, TRUE },
    { "COM 14", 14, TRUE },
    { "COM 15", 15, TRUE },
    { "COM 16", 16, TRUE },
    { "COM 17", 17, TRUE },
    { "COM 18", 18, TRUE },
    { "COM 19", 19, TRUE },
    { "COM 20", 20, TRUE },
#endif
};

enum PortDeviceType {pdtNone=0, pdtGps, pdtRangeFinder};

//
// CctControl_ComPortList
//
class CctControl_ComPortList: public CfxControl_List
{
protected:
    BOOL _autoNext;
    INT _selectedIndex;
    BYTE _defaultPort;
    BOOL _rightToLeft;
    COMPORTITEM _items[ARRAYSIZE(ComPortItems)];
    INT _itemMap[ARRAYSIZE(ComPortItems)];
    UINT _itemMapCount;
    BYTE _paramsId;
    PortDeviceType _portDeviceType;
    CHAR *_resultGlobalValue;

    VOID RebuildItemMap();
    VOID ClearItems();
    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ComPortList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ComPortList();

    VOID OnPenDown(INT X, INT Y);
    VOID OnPenClick(INT X, INT Y);

    BYTE GetPortId();
    VOID SetPortId(BYTE Value);

    BYTE GetParamsId()           { return _paramsId;  }
    VOID SetParamsId(BYTE Value) { _paramsId = Value; }

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    UINT GetItemCount();
    VOID PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index);
};

extern CfxControl *Create_Control_ComPortList(CfxPersistent *pOwner, CfxControl *pParent);

