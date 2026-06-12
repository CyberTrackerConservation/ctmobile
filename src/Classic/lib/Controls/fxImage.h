#pragma once
#include "pch.h"
#include "fxPanel.h"
#include "fxButton.h"

//
// CfxControl_ImageBase
//
class CfxControl_ImageBase: public CfxControl
{
protected:
    bool _center = false, _stretch = false, _proportional = false;
    bool _transparentImage = false;
    virtual FXBITMAPRESOURCE *GetBitmap()=0;
    virtual void ReleaseBitmap(FXBITMAPRESOURCE *pBitmap)=0;
public:
    CfxControl_ImageBase(CfxPersistent *pOwner, CfxControl *pParent);
    void DefineProperties(CfxFiler &F) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

//
// CfxControl_Image
//
const GUID GUID_CONTROL_IMAGE = {0xa25e984a, 0x294b, 0x4adf, {0xb7, 0xf7, 0x90, 0x0c, 0x4d, 0x99, 0x1d, 0xe5}};

class CfxControl_Image: public CfxControl_ImageBase
{
protected:
    XBITMAP _bitmap = 0;
    FXBITMAPRESOURCE *GetBitmap() override;
    void ReleaseBitmap(FXBITMAPRESOURCE *pBitmap) override;
public:
    CfxControl_Image(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Image();

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_Image(CfxPersistent *pOwner, CfxControl *pParent);

//
// CfxControl_ZoomImageBase
//
class CfxControl_ZoomImageBase: public CfxControl
{
protected:
    bool _lock100 = false;
    bool _smoothPan = false;
    BYTE _initialButtonState = 0;

    int _buttonWidth = 0, _finalButtonWidth = 0;
    CfxControl_Button *_fitButton = nullptr, *_zoomInButton = nullptr, *_zoomOutButton = nullptr, *_zoomButton = nullptr, *_panButton = nullptr;

    COLOR _textColor = 0;

    int _centerX = 0, _centerY = 0;
    double _zoom = 0.0;
    int _imageWidth = 0, _imageHeight = 0;

    bool _penDown = false;
    int _penX1 = 0, _penY1 = 0, _penX2 = 0, _penY2 = 0;
    int _panCenterX = 0, _panCenterY = 0;

    bool _allInView = false;
    double _minZoom = 0.0, _maxZoom = 0.0, _scaleMaxZoom = 0.0;
    bool _refreshCacheCanvas = false;
    CfxCanvas *_cacheCanvas = nullptr;

    FXRECT _paintRect, _viewPortRect, _imageRect, _virtualRect, _renderSrcRect, _renderDstRect;

    bool Initialize();

    virtual void ResetZoom();
    virtual void PlaceButtons();
    virtual bool GetImageData(int *pWidth, int *pHeight)=0;
    virtual void DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect)=0;

    void ScreenToImage(int *pX, int *pY);
    void ImageToScreen(int *pX, int *pY);

    void OnFitClick(CfxControl *pSender, void *pParams);
    void OnZoomInClick(CfxControl *pSender, void *pParams);
    void OnZoomOutClick(CfxControl *pSender, void *pParams);

    void OnFitPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnZoomInPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnZoomOutPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnZoomPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnPanPaint(CfxControl *pSender, FXPAINTDATA *pParams);
public:
    CfxControl_ZoomImageBase(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_ZoomImageBase();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;

    void SetZoom(double Value);
    void SetCenter(int x, int y);
    void SetLock100(bool Value);

    void SetBounds(int Left, int Top, int Width, int Height) override;
    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

	void OnPenDown(int X, int Y) override;
    void OnPenUp(int X, int Y) override;
    void OnPenMove(int X, int Y) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

//
// CfxControl_ZoomImage
//

const GUID GUID_CONTROL_ZOOMIMAGE = {0x864f16f8, 0x1260, 0x474a, { 0xbf, 0x4e, 0xd3, 0x5e, 0x51, 0x3d, 0x6e, 0x9c}};

class CfxControl_ZoomImage: public CfxControl_ZoomImageBase
{
protected:
    XBITMAP _bitmap = 0;
    bool GetImageData(int *pWidth, int *pHeight) override;
    void DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect) override;
public:
    CfxControl_ZoomImage(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_ZoomImage();

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_ZoomImage(CfxPersistent *pOwner, CfxControl *pParent);

