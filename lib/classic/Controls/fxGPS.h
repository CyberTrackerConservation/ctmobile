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

const GUID GUID_CONTROL_GPS = {0xdbda4351, 0xf047, 0x4aa9, {0x8b, 0x87, 0x68, 0x4c, 0xaf, 0x7e, 0xef, 0xa9}};

enum GPSControlStyle {gcsState=0, gcsData, gcsSky, gcsBars, gcsTriangle, gcsGotoData, gcsGotoPointer, gcsCompass, gcsCompass2, gcsHeading};

//
// CfxControl_GPS
//
class CfxControl_GPS: public CfxControl
{
protected:
    CHAR *_caption;
    COLOR _textColor;
    GPSControlStyle _style;
    BOOL _autoConnect, _connected;
    FXGPS_ACCURACY _accuracy;
    BOOL _extraData;
    BOOL _autoHeight;
    FXEVENT _onClick;

    VOID UpdateHeight();

    VOID PaintStyleState(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    VOID PaintStyleData(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    VOID PaintStyleSky(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    VOID PaintStyleBars(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    VOID PaintStyleCompass(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    VOID PaintStyleCompass2(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    VOID PaintStyleHeading(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    VOID PaintStyleTriangle(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    VOID PaintStyleGotoData(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    VOID PaintStyleGotoPointer(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);

public:
    CfxControl_GPS(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_GPS();

    GPSControlStyle GetStyle()           { return _style;                         }
    VOID SetStyle(GPSControlStyle Value) { _style = Value;                        }
    FXGPS_ACCURACY *GetAccuracy()        { return &_accuracy;                     }
    VOID SetAccuracy(const FXGPS_ACCURACY *pValue) { _accuracy = *pValue;         }
    VOID SetExtraData(BOOL Value)        { _extraData = Value;                    }
    BOOL GetExtraData()                  { return _extraData;                     }

    BOOL GetAutoConnect()                { return _autoConnect;                   }
    VOID SetAutoConnect(BOOL Value)      { _autoConnect = Value;                  }

    BOOL GetAutoHeight()                 { return _autoHeight;                    }
    VOID SetAutoHeight(BOOL Value)       { _autoHeight = Value;                   }

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);
    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);

    VOID SetOnClick(CfxControl *pControl, NotifyMethod OnClick);

    VOID OnPenClick(INT X, INT Y);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

extern CfxControl *Create_Control_GPS(CfxPersistent *pOwner, CfxControl *pParent);

extern VOID PropagateGPSAccuracy(CfxControl *pControl, const FXGPS_ACCURACY *pAccuracy);
