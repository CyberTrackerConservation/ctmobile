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
#include "fxPanel.h"
#include "fxButton.h"

//
// CfxControl_ImageBase
//
class CfxControl_ImageBase: public CfxControl
{
protected:
    BOOL _center, _stretch, _proportional;
    BOOL _transparentImage;
    virtual FXBITMAPRESOURCE *GetBitmap()=0;
    virtual VOID ReleaseBitmap(FXBITMAPRESOURCE *pBitmap)=0;
public:
    CfxControl_ImageBase(CfxPersistent *pOwner, CfxControl *pParent);
    VOID DefineProperties(CfxFiler &F);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

//
// CfxControl_Image
//
const GUID GUID_CONTROL_IMAGE = {0xa25e984a, 0x294b, 0x4adf, {0xb7, 0xf7, 0x90, 0x0c, 0x4d, 0x99, 0x1d, 0xe5}};

class CfxControl_Image: public CfxControl_ImageBase
{
protected:
    XBITMAP _bitmap;
    FXBITMAPRESOURCE *GetBitmap();
    VOID ReleaseBitmap(FXBITMAPRESOURCE *pBitmap);
public:
    CfxControl_Image(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Image();

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};

extern CfxControl *Create_Control_Image(CfxPersistent *pOwner, CfxControl *pParent);

//
// CfxControl_ZoomImageBase
//
class CfxControl_ZoomImageBase: public CfxControl
{
protected:
    BOOL _lock100;
    BOOL _smoothPan;
    BYTE _initialButtonState;

    UINT _buttonWidth, _finalButtonWidth;
    CfxControl_Button *_fitButton, *_zoomInButton, *_zoomOutButton, *_zoomButton, *_panButton;
    
    COLOR _textColor;    

    INT _centerX, _centerY;
    DOUBLE _zoom;
    UINT _imageWidth, _imageHeight;

    BOOL _penDown;
    INT _penX1, _penY1, _penX2, _penY2;
    INT _panCenterX, _panCenterY;

    BOOL _allInView;
    DOUBLE _minZoom, _maxZoom, _scaleMaxZoom;
    BOOL _refreshCacheCanvas;
    CfxCanvas *_cacheCanvas;

    FXRECT _paintRect, _viewPortRect, _imageRect, _virtualRect, _renderSrcRect, _renderDstRect;

    BOOL Initialize();

    virtual VOID ResetZoom();
    virtual VOID PlaceButtons();
    virtual BOOL GetImageData(UINT *pWidth, UINT *pHeight)=0;
    virtual VOID DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect)=0;

    VOID ScreenToImage(INT *pX, INT *pY);
    VOID ImageToScreen(INT *pX, INT *pY);

    VOID OnFitClick(CfxControl *pSender, VOID *pParams);
    VOID OnZoomInClick(CfxControl *pSender, VOID *pParams);
    VOID OnZoomOutClick(CfxControl *pSender, VOID *pParams);

    VOID OnFitPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnZoomInPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnZoomOutPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnZoomPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnPanPaint(CfxControl *pSender, FXPAINTDATA *pParams);
public:
    CfxControl_ZoomImageBase(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_ZoomImageBase();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);

    VOID SetZoom(DOUBLE Value);
    VOID SetCenter(INT x, INT y);
    VOID SetLock100(BOOL Value);

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

	VOID OnPenDown(INT X, INT Y);
    VOID OnPenUp(INT X, INT Y);
    VOID OnPenMove(INT X, INT Y);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

//
// CfxControl_ZoomImage
//

const GUID GUID_CONTROL_ZOOMIMAGE = {0x864f16f8, 0x1260, 0x474a, { 0xbf, 0x4e, 0xd3, 0x5e, 0x51, 0x3d, 0x6e, 0x9c}};

class CfxControl_ZoomImage: public CfxControl_ZoomImageBase
{
protected:
    XBITMAP _bitmap;
    BOOL GetImageData(UINT *pWidth, UINT *pHeight);
    VOID DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect);
public:
    CfxControl_ZoomImage(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_ZoomImage();

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};

extern CfxControl *Create_Control_ZoomImage(CfxPersistent *pOwner, CfxControl *pParent);

