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
    char *_caption = nullptr;
    COLOR _textColor = 0;
    GPSControlStyle _style = {};
    bool _autoConnect = false, _connected = false;
    FXGPS_ACCURACY _accuracy;
    bool _extraData = false;
    bool _autoHeight = false;
    FXEVENT _onClick;

    void UpdateHeight();

    void PaintStyleState(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    void PaintStyleData(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    void PaintStyleSky(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    void PaintStyleBars(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    void PaintStyleCompass(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    void PaintStyleCompass2(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    void PaintStyleHeading(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    void PaintStyleTriangle(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    void PaintStyleGotoData(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);
    void PaintStyleGotoPointer(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData);

public:
    CfxControl_GPS(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_GPS();

    GPSControlStyle GetStyle()           { return _style;                         }
    void SetStyle(GPSControlStyle Value) { _style = Value;                        }
    FXGPS_ACCURACY *GetAccuracy()        { return &_accuracy;                     }
    void SetAccuracy(const FXGPS_ACCURACY *pValue) { _accuracy = *pValue;         }
    void SetExtraData(bool Value)        { _extraData = Value;                    }
    bool GetExtraData()                  { return _extraData;                     }

    bool GetAutoConnect()                { return _autoConnect;                   }
    void SetAutoConnect(bool Value)      { _autoConnect = Value;                  }

    bool GetAutoHeight()                 { return _autoHeight;                    }
    void SetAutoHeight(bool Value)       { _autoHeight = Value;                   }

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
    void SetBounds(int Left, int Top, int Width, int Height) override;

    void SetOnClick(CfxControl *pControl, NotifyMethod OnClick);

    void OnPenClick(int X, int Y) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

extern CfxControl *Create_Control_GPS(CfxPersistent *pOwner, CfxControl *pParent);

extern void PropagateGPSAccuracy(CfxControl *pControl, const FXGPS_ACCURACY *pAccuracy);
