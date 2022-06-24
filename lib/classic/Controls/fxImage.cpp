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
#include "fxImage.h"
#include "fxUtils.h"
#include "fxMath.h"

//*************************************************************************************************
// CfxControl_ImageBase

CfxControl_ImageBase::CfxControl_ImageBase(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitBorder(bsSingle, 1);

    _center = TRUE;
    _stretch = FALSE;
    _proportional = TRUE;

    _transparentImage = FALSE;
}

VOID CfxControl_ImageBase::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Transparent",  dtBoolean,  &_transparent, F_FALSE);

    F.DefineValue("Center",       dtBoolean, &_center, F_TRUE);
    F.DefineValue("Stretch",      dtBoolean, &_stretch, F_FALSE);
    F.DefineValue("Proportional", dtBoolean, &_proportional, F_TRUE);
    F.DefineValue("TransparentImage", dtBoolean, &_transparentImage, F_FALSE);
}

VOID CfxControl_ImageBase::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    FXBITMAPRESOURCE *bitmap = GetBitmap();
    if (bitmap)
    {
        FXRECT rect;
        GetBitmapRect(bitmap, pRect->Right - pRect->Left + 1, pRect->Bottom - pRect->Top + 1, _center, _stretch, _proportional, &rect);
        pCanvas->StretchDraw(bitmap, pRect->Left + rect.Left, pRect->Top + rect.Top, rect.Right - rect.Left + 1 , rect.Bottom - rect.Top + 1, FALSE, _transparentImage);
        ReleaseBitmap(bitmap);
    }
}

//*************************************************************************************************
// CfxControl_Image

CfxControl *Create_Control_Image(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Image(pOwner, pParent);
}

CfxControl_Image::CfxControl_Image(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_ImageBase(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_IMAGE, TRUE);
    InitLockProperties("Center;Stretch;Proportional;Image");
    _bitmap = 0;
}

CfxControl_Image::~CfxControl_Image()
{
    freeX(_bitmap);
}

VOID CfxControl_Image::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    F.DefineBitmap(_bitmap);
#endif
}

VOID CfxControl_Image::DefineProperties(CfxFiler &F)
{
    CfxControl_ImageBase::DefineProperties(F);
    F.DefineXBITMAP("Bitmap", &_bitmap);
}

VOID CfxControl_Image::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Center",       dtBoolean,  &_center);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Image",        dtPGraphic, &_bitmap,      "BITDEPTH(16);HINTWIDTH(2048);HINTHEIGHT(2048)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Proportional", dtBoolean,  &_proportional);
    F.DefineValue("Stretch",      dtBoolean,  &_stretch);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Transparent image", dtBoolean, &_transparentImage);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

FXBITMAPRESOURCE *CfxControl_Image::GetBitmap()
{
    return GetResource()->GetBitmap(this, _bitmap);
}

VOID CfxControl_Image::ReleaseBitmap(FXBITMAPRESOURCE *pBitmap)
{
    GetResource()->ReleaseBitmap(pBitmap);
}

//*************************************************************************************************
// CfxControl_ZoomImageBase

CfxControl_ZoomImageBase::CfxControl_ZoomImageBase(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    _lock100 = FALSE;

    _smoothPan = FALSE;

    _buttonWidth = _finalButtonWidth = 32;
    _initialButtonState = 1;    // Default to pan

    _centerX = _centerY = 0;
    _zoom = 0.0;

    _allInView = FALSE;
    _minZoom = _maxZoom = 0.0;
    _scaleMaxZoom = 1.0;

    _penDown = FALSE;
    _penX1 = _penX2 = _penY1 = _penY2 = 0;
    _imageWidth = _imageHeight = 0;

    _panCenterX = _panCenterY = 0;

    _textColor = _borderColor;

    // Create buttons
    _fitButton = new CfxControl_Button(pOwner, this);
    _fitButton->SetComposite(TRUE);
    _fitButton->SetOnClick(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnFitClick);
    _fitButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnFitPaint);

    _zoomInButton = new CfxControl_Button(pOwner, this);
    _zoomInButton->SetComposite(TRUE);
    _zoomInButton->SetOnClick(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnZoomInClick);
    _zoomInButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnZoomInPaint);

    _zoomOutButton = new CfxControl_Button(pOwner, this);
    _zoomOutButton->SetComposite(TRUE);
    _zoomOutButton->SetOnClick(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnZoomOutClick);
    _zoomOutButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnZoomOutPaint);

    _zoomButton = new CfxControl_Button(pOwner, this);
    _zoomButton->SetComposite(TRUE);
    _zoomButton->SetGroupId(1);
    _zoomButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnZoomPaint);
    _zoomButton->SetDown(TRUE);

    _panButton = new CfxControl_Button(pOwner, this);
    _panButton->SetComposite(TRUE);
    _panButton->SetGroupId(1);
    _panButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnPanPaint);

    _refreshCacheCanvas = TRUE;
    _cacheCanvas = GetHost(this) ? GetHost(this)->CreateCanvas(0, 0) : NULL;
}

CfxControl_ZoomImageBase::~CfxControl_ZoomImageBase()
{
    delete _cacheCanvas;
    _cacheCanvas = NULL;
}

BOOL CfxControl_ZoomImageBase::Initialize()
{
    // Do nothing if image not already configured
    if ((_imageWidth == 0) || (_imageHeight == 0)) return FALSE;

    // Compute the bounds of the control
    FXRECT bounds;
    GetAbsoluteBounds(&bounds);
    InflateRect(&bounds, _borderWidth, _borderWidth);

    // Compute the paintable part
    _paintRect = bounds;
    _paintRect.Top += _finalButtonWidth;

    // Compute the viewport size
    UINT viewPortW = _paintRect.Right - _paintRect.Left + 1;
    UINT viewPortH = _paintRect.Bottom - _paintRect.Top + 1;

    // Compute the minimum zoom level: how much zoomed out
    DOUBLE nz1, nz2;
    nz1 = (DOUBLE)viewPortW / (DOUBLE)_imageWidth;
    nz2 = (DOUBLE)viewPortH / (DOUBLE)_imageHeight; 
    _minZoom = min(nz1, nz2);

    // Compute the maxium zoom level: how much zoomed in
    nz1 = (DOUBLE)viewPortW / (DOUBLE)min(16, _imageWidth);
    nz2 = (DOUBLE)viewPortH / (DOUBLE)min(16, _imageHeight); 
    _maxZoom = max(nz1, nz2);
    if (_lock100 && (_maxZoom > _scaleMaxZoom))
    {
        _maxZoom = _scaleMaxZoom;
    }

    // Initialize the zoom if not already set
    if (_zoom == 0.0)
    {
        ResetZoom();
    }

    // Compute scaled ImageRect, VirtualRect and ViewPortRect
    UINT imageW = (UINT)((DOUBLE)_imageWidth * _zoom + 0.5);
    UINT imageH = (UINT)((DOUBLE)_imageHeight * _zoom + 0.5);
    UINT virtualW = max(viewPortW, imageW * 2);
    UINT virtualH = max(viewPortH, imageH * 2);

    // Compute the virtual image rect
    _virtualRect.Left = _virtualRect.Top = 0;
    _virtualRect.Right = virtualW - 1;
    _virtualRect.Bottom = virtualH - 1;

    // Compute the scaled image rect
    _imageRect.Left = _imageRect.Top = 0;
    _imageRect.Right = imageW - 1;
    _imageRect.Bottom = imageH - 1;
    OffsetRect(&_imageRect, (virtualW - imageW) / 2, (virtualH - imageH) / 2);

    // Compute the viewport
    _viewPortRect.Left = _viewPortRect.Top = 0;
    _viewPortRect.Right = viewPortW - 1;
    _viewPortRect.Bottom = viewPortH - 1;
    OffsetRect(&_viewPortRect, 
               (INT)((_centerX * _zoom) + _imageRect.Left - viewPortW / 2), 
               (INT)((_centerY * _zoom) + _imageRect.Top - viewPortH / 2));

    // Test if the entire image is in view
    FXRECT tempRect;
    IntersectRect(&tempRect, &_viewPortRect, &_imageRect);
    _allInView = CompareRect(&tempRect, &_imageRect);

    // Compute the render rectangles

    // Get the rectangle that is in the image
    IntersectRect(&_renderDstRect, &_viewPortRect, &_imageRect);
    _renderSrcRect = _renderDstRect;

    // Move this back so it can be used for the Draw operation
    OffsetRect(&_renderDstRect, -_viewPortRect.Left, -_viewPortRect.Top);

    // Map the rectangle back to the underlying image
    OffsetRect(&_renderSrcRect, -_imageRect.Left, -_imageRect.Top);
    _renderSrcRect.Left = (INT)(_renderSrcRect.Left / _zoom);
    _renderSrcRect.Top = (INT)(_renderSrcRect.Top / _zoom);
    _renderSrcRect.Right = (INT)(_renderSrcRect.Right / _zoom);
    _renderSrcRect.Bottom = (INT)(_renderSrcRect.Bottom / _zoom);

    return TRUE;
}

VOID CfxControl_ZoomImageBase::ResetZoom()
{
    SetZoom(_minZoom);
    SetCenter(_imageWidth / 2, _imageHeight / 2);
}

VOID CfxControl_ZoomImageBase::SetZoom(DOUBLE Value)
{
    Value = max(_minZoom, Value);
    Value = min(_maxZoom, Value);

    _refreshCacheCanvas = _refreshCacheCanvas || (Value != _zoom);
    _zoom = Value;
}

VOID CfxControl_ZoomImageBase::SetCenter(INT x, INT y)
{
    if ((_imageWidth == 0) || (_imageHeight == 0)) return;

    if (x < 0)
    {
        x = 0;
    }

    if (y < 0)
    {
        y = 0;
    }

    if (x >= (INT)_imageWidth)
    {
        x = (INT)_imageWidth - 1;
    }

    if (y >= (INT)_imageHeight)
    {
        y = (INT)_imageHeight - 1;
    }

    _refreshCacheCanvas = _refreshCacheCanvas || ((_centerX != x) || (_centerY != y));
    _centerX = x;
    _centerY = y;
}

VOID CfxControl_ZoomImageBase::SetLock100(BOOL Value)
{
    _lock100 = Value;
}

VOID CfxControl_ZoomImageBase::ScreenToImage(INT *pX, INT *pY)
{
    *pX = FxRound((DOUBLE)(*pX + _viewPortRect.Left - _imageRect.Left) / _zoom);
    *pY = FxRound((DOUBLE)(*pY + _viewPortRect.Top - _imageRect.Top) / _zoom);
}

VOID CfxControl_ZoomImageBase::ImageToScreen(INT *pX, INT *pY)
{
    *pX = FxRound(((DOUBLE)*pX * _zoom) - _viewPortRect.Left + _imageRect.Left);
    *pY = FxRound(((DOUBLE)*pY * _zoom) - _viewPortRect.Top + _imageRect.Top);
}

VOID CfxControl_ZoomImageBase::PlaceButtons()
{
    INT buttonWidth = _buttonWidth - _borderLineWidth;

    // Position the toolbar buttons
    UINT tx = _borderWidth;
    UINT ty = _borderWidth;
    _fitButton->SetBounds(tx, ty, _buttonWidth, _buttonWidth);
    tx += buttonWidth;
    _zoomInButton->SetBounds(tx, ty, _buttonWidth, _buttonWidth);
    tx += buttonWidth;
    _zoomOutButton->SetBounds(tx, ty, _buttonWidth, _buttonWidth);
    tx += _buttonWidth + 4 *  _borderLineWidth;
    _zoomButton->SetBounds(tx, ty, _buttonWidth, _buttonWidth);
    tx += buttonWidth;
    _panButton->SetBounds(tx, ty, _buttonWidth, _buttonWidth);

    _finalButtonWidth = _buttonWidth;
}

VOID CfxControl_ZoomImageBase::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    PlaceButtons();

    // Clear the cache rect and reset the image
    _refreshCacheCanvas = TRUE;

    if (!IsLive())
    {
        _zoom = 0.0;
    }
}

VOID CfxControl_ZoomImageBase::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _buttonWidth = (_buttonWidth * ScaleX) / 100;
    _buttonWidth = max(16, _buttonWidth);
    _buttonWidth = max(ScaleHitSize, _buttonWidth);

    _scaleMaxZoom = (DOUBLE)max(ScaleX, ScaleY) / 100;

    PlaceButtons();
}

VOID CfxControl_ZoomImageBase::OnFitClick(CfxControl *pSender, VOID *pParams)
{
    ResetZoom();
    Repaint();
}

VOID CfxControl_ZoomImageBase::OnZoomInClick(CfxControl *pSender, VOID *pParams)
{
    SetZoom(_zoom * 1.2);
    Repaint();
}

VOID CfxControl_ZoomImageBase::OnZoomOutClick(CfxControl *pSender, VOID *pParams)
{
    SetZoom(_zoom / 1.2);
    Repaint();
}

VOID CfxControl_ZoomImageBase::OnFitPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    BOOL invert = _fitButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(INT)_borderLineWidth-1, -(INT)_borderLineWidth-1);

    INT rw = rect.Right - rect.Left + 1;
    INT rh = rect.Bottom - rect.Top + 1;

    INT w = rw / 2;
    INT h = rh / 2;
    h = min(h, w);
    INT lw = (h < 10) ? 1 : _borderLineWidth;

    INT x = rect.Left + w - lw/2;
    INT y = rect.Top + h - lw/2;

    pParams->Canvas->State.LineWidth = lw;
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);
    pParams->Canvas->Ellipse(x, y, h, h);

    // Draw an X
    pParams->Canvas->MoveTo(x-h, y-h);
    pParams->Canvas->LineTo(x+h, y+h);
    pParams->Canvas->MoveTo(x-h, y+h);
    pParams->Canvas->LineTo(x+h, y-h);
}

VOID CfxControl_ZoomImageBase::OnZoomInPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    BOOL invert = _zoomInButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(INT)_borderLineWidth-1, -(INT)_borderLineWidth-1);

    INT rw = rect.Right - rect.Left + 1;
    INT rh = rect.Bottom - rect.Top + 1;

    INT w = rw / 2;
    INT h = rh / 2;
    h = min(h, w);
    INT lw = (h < 10) ? 1 : _borderLineWidth;

    INT x = rect.Left + w - lw/2;
    INT y = rect.Top + h - lw/2;

    pParams->Canvas->State.LineWidth = lw;
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);
    pParams->Canvas->Ellipse(x, y, h, h);

    h /= 2;
    pParams->Canvas->MoveTo(x, y-h);
    pParams->Canvas->LineTo(x, y+h);
    pParams->Canvas->MoveTo(x-h, y);
    pParams->Canvas->LineTo(x+h, y);
}

VOID CfxControl_ZoomImageBase::OnZoomOutPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    BOOL invert = _zoomOutButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(INT)_borderLineWidth-1, -(INT)_borderLineWidth-1);

    INT rw = rect.Right - rect.Left + 1;
    INT rh = rect.Bottom - rect.Top + 1;

    INT w = rw / 2;
    INT h = rh / 2;
    h = min(h, w);
    INT lw = (h < 10) ? 1 : _borderLineWidth;

    INT x = rect.Left + w - lw/2;
    INT y = rect.Top + h - lw/2;

    pParams->Canvas->State.LineWidth = lw;
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);
    pParams->Canvas->Ellipse(x, y, h, h);

    h /= 2;
    pParams->Canvas->MoveTo(x-h, y);
    pParams->Canvas->LineTo(x+h, y);
}

VOID CfxControl_ZoomImageBase::OnZoomPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    BOOL invert = _zoomButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(INT)_borderLineWidth-1, -(INT)_borderLineWidth-1);

    INT rw = rect.Right - rect.Left + 1;
    INT rh = rect.Bottom - rect.Top + 1;

    INT w = rw / 2;
    INT h = rh / 2;
    h = min(h, w);
    INT lw = (h < 10) ? 1 : _borderLineWidth;

    pParams->Canvas->State.LineWidth = lw;
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);

    h /= 2;
    rect.Right -= h;
    rect.Bottom -= h;
    pParams->Canvas->FrameRect(&rect);

    INT x = rect.Right + 2 * lw;
    INT y = rect.Bottom + 2 * lw;
    h -= 2 * lw;

    pParams->Canvas->MoveTo(x, y-h);
    pParams->Canvas->LineTo(x, y+h);
    pParams->Canvas->MoveTo(x-h, y);
    pParams->Canvas->LineTo(x+h, y);
}

VOID CfxControl_ZoomImageBase::OnPanPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    BOOL invert = _panButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(INT)_borderLineWidth-1, -(INT)_borderLineWidth-1);

    INT rw = rect.Right - rect.Left + 1;
    INT rh = rect.Bottom - rect.Top + 1;

    INT w = rw / 2;
    INT h = rh / 2;
    h = min(h, w);
    INT lw = (h < 10) ? 1 : _borderLineWidth;

    INT x = rect.Left + w - lw/2;
    INT y = rect.Top + h - lw/2;

    pParams->Canvas->State.LineWidth = lw;
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);

    h /= 2;
    h += lw * 4;
    h--;
    pParams->Canvas->MoveTo(x, y-h);
    pParams->Canvas->LineTo(x, y+h);
    pParams->Canvas->MoveTo(x-h, y);
    pParams->Canvas->LineTo(x+h, y);

    INT h2 = h/3;
    
    pParams->Canvas->MoveTo(x, y-h);
    pParams->Canvas->LineTo(x-h2, y-h+h2);
    pParams->Canvas->MoveTo(x, y-h);
    pParams->Canvas->LineTo(x+h2, y-h+h2);

    pParams->Canvas->MoveTo(x, y+h);
    pParams->Canvas->LineTo(x-h2, y+h-h2);
    pParams->Canvas->MoveTo(x, y+h);
    pParams->Canvas->LineTo(x+h2, y+h-h2);

    pParams->Canvas->MoveTo(x-h, y);
    pParams->Canvas->LineTo(x-h+h2, y-h2);
    pParams->Canvas->MoveTo(x-h, y);
    pParams->Canvas->LineTo(x-h+h2, y+h2);

    pParams->Canvas->MoveTo(x+h, y);
    pParams->Canvas->LineTo(x+h-h2, y-h2);
    pParams->Canvas->MoveTo(x+h, y);
    pParams->Canvas->LineTo(x+h-h2, y+h2);
}

VOID CfxControl_ZoomImageBase::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("ButtonWidth",   dtInt32,   &_buttonWidth, "32");
    F.DefineValue("SmoothPan",     dtBoolean, &_smoothPan, F_FALSE);
    F.DefineValue("InitialButtonState", dtByte, &_initialButtonState, F_ONE);
    F.DefineValue("Lock100",       dtBoolean, &_lock100, F_FALSE);
    F.DefineValue("TextColor",     dtColor,   &_textColor, F_COLOR_BLACK);

    if (F.IsReader())
    {
        // Refresh the cache and clear the imagerect
        _refreshCacheCanvas = TRUE;
        _imageWidth = _imageHeight = 0;

        // Move the buttons
        SetBounds(_left, _top, _width, _height);

        // Setup inital button state
        if (_initialButtonState == 0)
        {
            _zoomButton->SetDown(TRUE);
        } 
        else if (_initialButtonState == 1)
        {
            _panButton->SetDown(TRUE);
        }
    }
}

VOID CfxControl_ZoomImageBase::DefineState(CfxFiler &F)
{
    if (F.IsReader())
    {
        BOOL downButton = FALSE;
        F.DefineValue("ZoomDown", dtBoolean, &downButton);
        _zoomButton->SetDown(downButton);
        F.DefineValue("PanDown", dtBoolean, &downButton);
        _panButton->SetDown(downButton);
    }
    else
    {
        BOOL downButton = _zoomButton->GetDown();
        F.DefineValue("ZoomDown", dtBoolean, &downButton);
        downButton = _panButton->GetDown();
        F.DefineValue("PanDown", dtBoolean, &downButton);
    }

    F.DefineValue("CenterX", dtInt32, &_centerX);
    F.DefineValue("CenterY", dtInt32, &_centerY);
    F.DefineValue("Zoom", dtDouble, &_zoom);
}

VOID CfxControl_ZoomImageBase::OnPenDown(INT X, INT Y)
{
    Y -= _finalButtonWidth;

    // Nothing to do if we have no bitmap
    if ((_imageWidth == 0) || (_imageHeight == 0)) return;

    // Ignore clicks on anything but the image
    //if (!PtInRect(&_paintRect, X, Y)) return;

    // Setup start state
    _penDown = TRUE;
    _penX1 = X - _borderWidth;
    _penY1 = Y - _borderWidth;

    _penX1 = max(0, min(_penX1, _width));
    _penY1 = max(0, min(_penY1, _height));
    _penX2 = _penX1;
    _penY2 = _penY1;

    _panCenterX = _centerX;
    _panCenterY = _centerY;
}

VOID CfxControl_ZoomImageBase::OnPenMove(INT X, INT Y)
{
    Y -= _finalButtonWidth;

    if (!_penDown) return;

    _penX2 = X - _borderWidth;
    _penY2 = Y - _borderWidth;

    // Calculate the new zoom rect based on the pen-coordinates
    if (_panButton->GetDown())
    {
        INT d;
        INT ncx, ncy;

        d = (INT)((_penX2 - _penX1) / _zoom);
        ncx = _panCenterX - d;

        d = (INT)((_penY2 - _penY1) / _zoom);
        ncy = _panCenterY - d;

        SetCenter(ncx, ncy);
    }

    // Always repaint after changing anything    
    Repaint();
}

VOID CfxControl_ZoomImageBase::OnPenUp(INT X, INT Y)
{
    Y -= _finalButtonWidth;

    if (!_penDown) return;

    _penDown = FALSE;

    // Correct co-ordinates
    _penX2 = max(0, min(_penX2, _width));
    _penY2 = max(0, min(_penY2, _height));

    if (_penX1 > _penX2)
    {
        Swap(&_penX1, &_penX2);
    }

    if (_penY1 > _penY2)
    {
        Swap(&_penY1, &_penY2);
    }

    // Calculate the new zoom rect: tiny zooms disallowed
    if (_zoomButton->GetDown() && (_penX2 - _penX1 > 4) && (_penY2 - _penY1 > 4))
    {
        // Set the center point of the zoom
        INT nx, ny;
        nx = _penX1 + (_penX2 - _penX1) / 2;
        ny = _penY1 + (_penY2 - _penY1) / 2;
        ScreenToImage(&nx, &ny);
        SetCenter(nx, ny);

        // Set the zoom level
        INT nx1, ny1, nx2, ny2;
        DOUBLE nz;

        nx1 = _penX1;
        ny1 = _penY1;
        nx2 = _penX2;
        ny2 = _penY2;
        ScreenToImage(&nx1, &ny1);
        ScreenToImage(&nx2, &ny2);

        INT viewW = _viewPortRect.Right - _viewPortRect.Left + 1;
        INT viewH = _viewPortRect.Bottom - _viewPortRect.Top + 1;

        nz = min((DOUBLE)viewW / (DOUBLE)(nx2 - nx1), (DOUBLE)viewH / (DOUBLE)(ny2 - ny1));
        SetZoom(nz);

    // Force the cache to rescan the image
    //_refreshCacheCanvas = TRUE;
    }

    // Always repaint after changing anything    
    Repaint();       
}

VOID CfxControl_ZoomImageBase::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    // Set up the image
    if ((_imageWidth == 0) || (_imageHeight == 0))
    {
        if (!GetImageData(&_imageWidth, &_imageHeight)) return;
    }

    if (!Initialize()) return;

    // Draw the image
    _cacheCanvas->SetSize(_paintRect.Right - _paintRect.Left + 1, _paintRect.Bottom - _paintRect.Top + 1);

    if (!_smoothPan && _penDown && _panButton->GetDown())
    {
        // We're panning, so just paint the cached image
        pCanvas->Draw(_cacheCanvas, 
                      _paintRect.Left + (INT)((_panCenterX - _centerX) * _zoom), 
                      _paintRect.Top + (INT)((_panCenterY - _centerY) * _zoom));
    }
    else
    {
        // Check if we have to refresh the cache canvas
        if (_refreshCacheCanvas)
        {
            _refreshCacheCanvas = FALSE;

            // Clear the canvas
            FXRECT tempRect = {0};
            tempRect.Right = _cacheCanvas->GetWidth() - 1;
            tempRect.Bottom = _cacheCanvas->GetHeight() - 1;
            _cacheCanvas->State.BrushColor = _color;
            _cacheCanvas->FillRect(&tempRect);

            // Redraw the image
            DrawImage(_cacheCanvas, &_renderDstRect, &_renderSrcRect);
        }

        // Draw the cache canvas onto the main canvas
        pCanvas->Draw(_cacheCanvas, _paintRect.Left, _paintRect.Top);
    }

    // Draw the zoom rectangle
    if (_penDown && _zoomButton->GetDown())
    {
        FXRECT focusRect = {min(_penX1, _penX2), min(_penY1, _penY2), max(_penX1, _penX2), max(_penY1, _penY2)};
        OffsetRect(&focusRect, _paintRect.Left, _paintRect.Top);

        pCanvas->State.LineColor = 0xFF;
        pCanvas->State.LineWidth = _borderLineWidth;
        pCanvas->FrameRect(&focusRect);
    }
}

//*************************************************************************************************
// CfxControl_ZoomImage

CfxControl *Create_Control_ZoomImage(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_ZoomImage(pOwner, pParent);
}

CfxControl_ZoomImage::CfxControl_ZoomImage(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_ZoomImageBase(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ZOOMIMAGE);
    InitLockProperties("Image");
    _bitmap = 0;
    _smoothPan = TRUE;
}

CfxControl_ZoomImage::~CfxControl_ZoomImage()
{
    freeX(_bitmap);
}

VOID CfxControl_ZoomImage::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_ZoomImageBase::DefineResources(F);
    F.DefineBitmap(_bitmap);
#endif
}

VOID CfxControl_ZoomImage::DefineProperties(CfxFiler &F)
{
    CfxControl_ZoomImageBase::DefineProperties(F);
    F.DefineXBITMAP("Bitmap", &_bitmap);
}

VOID CfxControl_ZoomImage::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button width", dtInt32,    &_buttonWidth, "MIN(16);MAX(100)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Image",        dtPGraphic, &_bitmap,      "BITDEPTH(16);HINTWIDTH(4096);HINTHEIGHT(4096)");
    F.DefineValue("Initial button state", dtByte, &_initialButtonState, "LIST(Zoom Pan)");
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Lock 100",     dtBoolean,  &_lock100);
    F.DefineValue("Smooth pan",   dtBoolean,  &_smoothPan);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

BOOL CfxControl_ZoomImage::GetImageData(UINT *pWidth, UINT *pHeight)
{
    *pWidth = *pHeight = 0;

    CfxResource *resource = GetResource();
    FXBITMAPRESOURCE *bitmap = resource->GetBitmap(this, _bitmap);
    if (bitmap)
    {
        *pWidth = bitmap->Width;
        *pHeight = bitmap->Height;
        resource->ReleaseBitmap(bitmap);
    }

    return (*pWidth && *pHeight);
}

VOID CfxControl_ZoomImage::DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect)
{
    CfxResource *resource = GetResource();
    
    FXBITMAPRESOURCE *bitmap = resource->GetBitmap(this, _bitmap);
    if (bitmap)
    {
        pDstCanvas->StretchDraw(bitmap, pSrcRect, pDstRect, FALSE, FALSE);
        resource->ReleaseBitmap(bitmap);
    }
}
