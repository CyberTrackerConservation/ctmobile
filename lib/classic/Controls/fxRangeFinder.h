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
#include "fxRangeFinderParse.h"

const GUID GUID_CONTROL_RANGEFINDER = {0xda6b83f4, 0x61f1, 0x4b9c, {0xb5, 0xaf, 0xdf, 0x89, 0x92, 0x71, 0x2c, 0x87}};

enum RangeFinderControlStyle {rcsState=0, rcsRange, rcsAll};

//
// CfxControl_RangeFinder
//
class CfxControl_RangeFinder: public CfxControl
{
protected:
    COLOR _textColor;
    RangeFinderControlStyle _style;
    FXRANGE _range;

    BOOL _legacyMode;
    BYTE _portId, _portIdOverride, _portIdConnected;
    FX_COMPORT _comPort;
    BOOL _connected, _tryConnect;
    BOOL _keepConnected, _hasData, _showPortState;
    CfxRangeFinderParser *_parser;

    VOID Reset(BOOL KeepConnected = FALSE);
    VOID ResetRange();

    VOID PaintStyleState(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXRANGE *pRange);
    VOID PaintStyleRange(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXRANGE *pRange);
    VOID PaintStyleAll(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXRANGE *pRange);
public:
    CfxControl_RangeFinder(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_RangeFinder();

    virtual BOOL IsRangeLocked()         { return FALSE;           }

    RangeFinderControlStyle GetStyle()   { return _style;          }
    VOID SetStyle(RangeFinderControlStyle Value) { _style = Value; }
    
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID OnPortData(BYTE PortId, BYTE *pData, UINT Length);
    VOID OnTimer();
    VOID OnPenClick(INT X, INT Y);
    virtual VOID OnRange(FXRANGE *pRange);
};

extern CfxControl *Create_Control_RangeFinder(CfxPersistent *pOwner, CfxControl *pParent);
