#include "fxApplication.h"
#include "fxImage.h"
#include "fxUtils.h"
#include "fxMath.h"

//*************************************************************************************************
// CfxControl_ImageBase

CfxControl_ImageBase::CfxControl_ImageBase(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitBorder(ControlBorderStyle::Single, 1);

    _center = true;
    _stretch = false;
    _proportional = true;

    _transparentImage = false;
}

void CfxControl_ImageBase::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Transparent",  dtBoolean,  &_transparent, F_FALSE);

    F.DefineValue("Center",       dtBoolean, &_center, F_TRUE);
    F.DefineValue("Stretch",      dtBoolean, &_stretch, F_FALSE);
    F.DefineValue("Proportional", dtBoolean, &_proportional, F_TRUE);
    F.DefineValue("TransparentImage", dtBoolean, &_transparentImage, F_FALSE);
}

void CfxControl_ImageBase::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
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
        pCanvas->StretchDraw(bitmap, pRect->Left + rect.Left, pRect->Top + rect.Top, rect.Right - rect.Left + 1 , rect.Bottom - rect.Top + 1, false, _transparentImage);
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
    InitControl(&GUID_CONTROL_IMAGE, true);
    InitLockProperties("Center;Stretch;Proportional;Image");
    _bitmap = 0;
}

CfxControl_Image::~CfxControl_Image()
{
    freeX(_bitmap);
}

void CfxControl_Image::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_Image::DefineProperties(CfxFiler &F)
{
    CfxControl_ImageBase::DefineProperties(F);
    F.DefineXBITMAP("Bitmap", &_bitmap);
}

void CfxControl_Image::DefinePropertiesUI(CfxFilerUI &F) 
{
}

FXBITMAPRESOURCE *CfxControl_Image::GetBitmap()
{
    return GetResource()->GetBitmap(this, _bitmap);
}

void CfxControl_Image::ReleaseBitmap(FXBITMAPRESOURCE *pBitmap)
{
    GetResource()->ReleaseBitmap(pBitmap);
}

//*************************************************************************************************
// CfxControl_ZoomImageBase

CfxControl_ZoomImageBase::CfxControl_ZoomImageBase(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    _lock100 = false;

    _smoothPan = false;

    _buttonWidth = _finalButtonWidth = 32;
    _initialButtonState = 1;    // Default to pan

    _centerX = _centerY = 0;
    _zoom = 0.0;

    _allInView = false;
    _minZoom = _maxZoom = 0.0;
    _scaleMaxZoom = 1.0;

    _penDown = false;
    _penX1 = _penX2 = _penY1 = _penY2 = 0;
    _imageWidth = _imageHeight = 0;

    _panCenterX = _panCenterY = 0;

    _textColor = _borderColor;

    // Create buttons
    _fitButton = new CfxControl_Button(pOwner, this);
    _fitButton->SetComposite(true);
    _fitButton->SetOnClick(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnFitClick);
    _fitButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnFitPaint);

    _zoomInButton = new CfxControl_Button(pOwner, this);
    _zoomInButton->SetComposite(true);
    _zoomInButton->SetOnClick(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnZoomInClick);
    _zoomInButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnZoomInPaint);

    _zoomOutButton = new CfxControl_Button(pOwner, this);
    _zoomOutButton->SetComposite(true);
    _zoomOutButton->SetOnClick(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnZoomOutClick);
    _zoomOutButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnZoomOutPaint);

    _zoomButton = new CfxControl_Button(pOwner, this);
    _zoomButton->SetComposite(true);
    _zoomButton->SetGroupId(1);
    _zoomButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnZoomPaint);
    _zoomButton->SetDown(true);

    _panButton = new CfxControl_Button(pOwner, this);
    _panButton->SetComposite(true);
    _panButton->SetGroupId(1);
    _panButton->SetOnPaint(this, (NotifyMethod)&CfxControl_ZoomImageBase::OnPanPaint);

    _refreshCacheCanvas = true;
    _cacheCanvas = GetHost(this) ? GetHost(this)->CreateCanvas(0, 0) : nullptr;
}

CfxControl_ZoomImageBase::~CfxControl_ZoomImageBase()
{
    delete _cacheCanvas;
    _cacheCanvas = nullptr;
}

bool CfxControl_ZoomImageBase::Initialize()
{
    // Do nothing if image not already configured
    if ((_imageWidth == 0) || (_imageHeight == 0)) return false;

    // Compute the bounds of the control
    FXRECT bounds;
    GetAbsoluteBounds(&bounds);
    InflateRect(&bounds, _borderWidth, _borderWidth);

    // Compute the paintable part
    _paintRect = bounds;
    _paintRect.Top += _finalButtonWidth;

    // Compute the viewport size
    int viewPortW = _paintRect.Right - _paintRect.Left + 1;
    int viewPortH = _paintRect.Bottom - _paintRect.Top + 1;

    // Compute the minimum zoom level: how much zoomed out
    double nz1, nz2;
    nz1 = (double)viewPortW / (double)_imageWidth;
    nz2 = (double)viewPortH / (double)_imageHeight; 
    _minZoom = min(nz1, nz2);

    // Compute the maxium zoom level: how much zoomed in
    nz1 = (double)viewPortW / (double)min(16, _imageWidth);
    nz2 = (double)viewPortH / (double)min(16, _imageHeight); 
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
    int imageW = (int)((double)_imageWidth * _zoom + 0.5);
    int imageH = (int)((double)_imageHeight * _zoom + 0.5);
    int virtualW = max(viewPortW, imageW * 2);
    int virtualH = max(viewPortH, imageH * 2);

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
               (int)((_centerX * _zoom) + _imageRect.Left - viewPortW / 2), 
               (int)((_centerY * _zoom) + _imageRect.Top - viewPortH / 2));

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
    _renderSrcRect.Left = (int)(_renderSrcRect.Left / _zoom);
    _renderSrcRect.Top = (int)(_renderSrcRect.Top / _zoom);
    _renderSrcRect.Right = (int)(_renderSrcRect.Right / _zoom);
    _renderSrcRect.Bottom = (int)(_renderSrcRect.Bottom / _zoom);

    return true;
}

void CfxControl_ZoomImageBase::ResetZoom()
{
    SetZoom(_minZoom);
    SetCenter(_imageWidth / 2, _imageHeight / 2);
}

void CfxControl_ZoomImageBase::SetZoom(double Value)
{
    Value = max(_minZoom, Value);
    Value = min(_maxZoom, Value);

    _refreshCacheCanvas = _refreshCacheCanvas || (Value != _zoom);
    _zoom = Value;
}

void CfxControl_ZoomImageBase::SetCenter(int x, int y)
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

    if (x >= (int)_imageWidth)
    {
        x = (int)_imageWidth - 1;
    }

    if (y >= (int)_imageHeight)
    {
        y = (int)_imageHeight - 1;
    }

    _refreshCacheCanvas = _refreshCacheCanvas || ((_centerX != x) || (_centerY != y));
    _centerX = x;
    _centerY = y;
}

void CfxControl_ZoomImageBase::SetLock100(bool Value)
{
    _lock100 = Value;
}

void CfxControl_ZoomImageBase::ScreenToImage(int *pX, int *pY)
{
    *pX = FxRound((double)(*pX + _viewPortRect.Left - _imageRect.Left) / _zoom);
    *pY = FxRound((double)(*pY + _viewPortRect.Top - _imageRect.Top) / _zoom);
}

void CfxControl_ZoomImageBase::ImageToScreen(int *pX, int *pY)
{
    *pX = FxRound(((double)*pX * _zoom) - _viewPortRect.Left + _imageRect.Left);
    *pY = FxRound(((double)*pY * _zoom) - _viewPortRect.Top + _imageRect.Top);
}

void CfxControl_ZoomImageBase::PlaceButtons()
{
    int buttonWidth = _buttonWidth - _borderLineWidth;

    // Position the toolbar buttons
    int tx = _borderWidth;
    int ty = _borderWidth;
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

void CfxControl_ZoomImageBase::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    PlaceButtons();

    // Clear the cache rect and reset the image
    _refreshCacheCanvas = true;

    if (!IsLive())
    {
        _zoom = 0.0;
    }
}

void CfxControl_ZoomImageBase::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _buttonWidth = (_buttonWidth * ScaleX) / 100;
    _buttonWidth = max(16, _buttonWidth);
    _buttonWidth = max(ScaleHitSize, _buttonWidth);

    _scaleMaxZoom = (double)max(ScaleX, ScaleY) / 100;

    PlaceButtons();
}

void CfxControl_ZoomImageBase::OnFitClick(CfxControl *pSender, void *pParams)
{
    ResetZoom();
    Repaint();
}

void CfxControl_ZoomImageBase::OnZoomInClick(CfxControl *pSender, void *pParams)
{
    SetZoom(_zoom * 1.2);
    Repaint();
}

void CfxControl_ZoomImageBase::OnZoomOutClick(CfxControl *pSender, void *pParams)
{
    SetZoom(_zoom / 1.2);
    Repaint();
}

void CfxControl_ZoomImageBase::OnFitPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    bool invert = _fitButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(int)_borderLineWidth-1, -(int)_borderLineWidth-1);

    int rw = rect.Right - rect.Left + 1;
    int rh = rect.Bottom - rect.Top + 1;

    int w = rw / 2;
    int h = rh / 2;
    h = min(h, w);
    int lw = (h < 10) ? 1 : _borderLineWidth;

    int x = rect.Left + w - lw/2;
    int y = rect.Top + h - lw/2;

    pParams->Canvas->State.LineWidth = lw;
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);
    pParams->Canvas->Ellipse(x, y, h, h);

    // Draw an X
    pParams->Canvas->MoveTo(x-h, y-h);
    pParams->Canvas->LineTo(x+h, y+h);
    pParams->Canvas->MoveTo(x-h, y+h);
    pParams->Canvas->LineTo(x+h, y-h);
}

void CfxControl_ZoomImageBase::OnZoomInPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    bool invert = _zoomInButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(int)_borderLineWidth-1, -(int)_borderLineWidth-1);

    int rw = rect.Right - rect.Left + 1;
    int rh = rect.Bottom - rect.Top + 1;

    int w = rw / 2;
    int h = rh / 2;
    h = min(h, w);
    int lw = (h < 10) ? 1 : _borderLineWidth;

    int x = rect.Left + w - lw/2;
    int y = rect.Top + h - lw/2;

    pParams->Canvas->State.LineWidth = lw;
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);
    pParams->Canvas->Ellipse(x, y, h, h);

    h /= 2;
    pParams->Canvas->MoveTo(x, y-h);
    pParams->Canvas->LineTo(x, y+h);
    pParams->Canvas->MoveTo(x-h, y);
    pParams->Canvas->LineTo(x+h, y);
}

void CfxControl_ZoomImageBase::OnZoomOutPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    bool invert = _zoomOutButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(int)_borderLineWidth-1, -(int)_borderLineWidth-1);

    int rw = rect.Right - rect.Left + 1;
    int rh = rect.Bottom - rect.Top + 1;

    int w = rw / 2;
    int h = rh / 2;
    h = min(h, w);
    int lw = (h < 10) ? 1 : _borderLineWidth;

    int x = rect.Left + w - lw/2;
    int y = rect.Top + h - lw/2;

    pParams->Canvas->State.LineWidth = lw;
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);
    pParams->Canvas->Ellipse(x, y, h, h);

    h /= 2;
    pParams->Canvas->MoveTo(x-h, y);
    pParams->Canvas->LineTo(x+h, y);
}

void CfxControl_ZoomImageBase::OnZoomPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    bool invert = _zoomButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(int)_borderLineWidth-1, -(int)_borderLineWidth-1);

    int rw = rect.Right - rect.Left + 1;
    int rh = rect.Bottom - rect.Top + 1;

    int w = rw / 2;
    int h = rh / 2;
    h = min(h, w);
    int lw = (h < 10) ? 1 : _borderLineWidth;

    pParams->Canvas->State.LineWidth = lw;
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);

    h /= 2;
    rect.Right -= h;
    rect.Bottom -= h;
    pParams->Canvas->FrameRect(&rect);

    int x = rect.Right + 2 * lw;
    int y = rect.Bottom + 2 * lw;
    h -= 2 * lw;

    pParams->Canvas->MoveTo(x, y-h);
    pParams->Canvas->LineTo(x, y+h);
    pParams->Canvas->MoveTo(x-h, y);
    pParams->Canvas->LineTo(x+h, y);
}

void CfxControl_ZoomImageBase::OnPanPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    bool invert = _panButton->GetDown();

    pParams->Canvas->State.BrushColor = pParams->Canvas->InvertColor(_color, invert);
    pParams->Canvas->FillRect(pParams->Rect);

    FXRECT rect = *pParams->Rect;
    InflateRect(&rect, -(int)_borderLineWidth-1, -(int)_borderLineWidth-1);

    int rw = rect.Right - rect.Left + 1;
    int rh = rect.Bottom - rect.Top + 1;

    int w = rw / 2;
    int h = rh / 2;
    h = min(h, w);
    int lw = (h < 10) ? 1 : _borderLineWidth;

    int x = rect.Left + w - lw/2;
    int y = rect.Top + h - lw/2;

    pParams->Canvas->State.LineWidth = lw;
    pParams->Canvas->State.LineColor = pParams->Canvas->InvertColor(_textColor, invert);

    h /= 2;
    h += lw * 4;
    h--;
    pParams->Canvas->MoveTo(x, y-h);
    pParams->Canvas->LineTo(x, y+h);
    pParams->Canvas->MoveTo(x-h, y);
    pParams->Canvas->LineTo(x+h, y);

    int h2 = h/3;
    
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

void CfxControl_ZoomImageBase::DefineProperties(CfxFiler &F)
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
        _refreshCacheCanvas = true;
        _imageWidth = _imageHeight = 0;

        // Move the buttons
        SetBounds(_left, _top, _width, _height);

        // Setup inital button state
        if (_initialButtonState == 0)
        {
            _zoomButton->SetDown(true);
        } 
        else if (_initialButtonState == 1)
        {
            _panButton->SetDown(true);
        }
    }
}

void CfxControl_ZoomImageBase::DefineState(CfxFiler &F)
{
    if (F.IsReader())
    {
        bool downButton = false;
        F.DefineValue("ZoomDown", dtBoolean, &downButton);
        _zoomButton->SetDown(downButton);
        F.DefineValue("PanDown", dtBoolean, &downButton);
        _panButton->SetDown(downButton);
    }
    else
    {
        bool downButton = _zoomButton->GetDown();
        F.DefineValue("ZoomDown", dtBoolean, &downButton);
        downButton = _panButton->GetDown();
        F.DefineValue("PanDown", dtBoolean, &downButton);
    }

    F.DefineValue("CenterX", dtInt32, &_centerX);
    F.DefineValue("CenterY", dtInt32, &_centerY);
    F.DefineValue("Zoom", dtDouble, &_zoom);
}

void CfxControl_ZoomImageBase::OnPenDown(int X, int Y)
{
    Y -= _finalButtonWidth;

    // Nothing to do if we have no bitmap
    if ((_imageWidth == 0) || (_imageHeight == 0)) return;

    // Ignore clicks on anything but the image
    //if (!PtInRect(&_paintRect, X, Y)) return;

    // Setup start state
    _penDown = true;
    _penX1 = X - _borderWidth;
    _penY1 = Y - _borderWidth;

    _penX1 = max(0, min(_penX1, _width));
    _penY1 = max(0, min(_penY1, _height));
    _penX2 = _penX1;
    _penY2 = _penY1;

    _panCenterX = _centerX;
    _panCenterY = _centerY;
}

void CfxControl_ZoomImageBase::OnPenMove(int X, int Y)
{
    Y -= _finalButtonWidth;

    if (!_penDown) return;

    _penX2 = X - _borderWidth;
    _penY2 = Y - _borderWidth;

    // Calculate the new zoom rect based on the pen-coordinates
    if (_panButton->GetDown())
    {
        int d;
        int ncx, ncy;

        d = (int)((_penX2 - _penX1) / _zoom);
        ncx = _panCenterX - d;

        d = (int)((_penY2 - _penY1) / _zoom);
        ncy = _panCenterY - d;

        SetCenter(ncx, ncy);
    }

    // Always repaint after changing anything    
    Repaint();
}

void CfxControl_ZoomImageBase::OnPenUp(int X, int Y)
{
    Y -= _finalButtonWidth;

    if (!_penDown) return;

    _penDown = false;

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
        int nx, ny;
        nx = _penX1 + (_penX2 - _penX1) / 2;
        ny = _penY1 + (_penY2 - _penY1) / 2;
        ScreenToImage(&nx, &ny);
        SetCenter(nx, ny);

        // Set the zoom level
        int nx1, ny1, nx2, ny2;
        double nz;

        nx1 = _penX1;
        ny1 = _penY1;
        nx2 = _penX2;
        ny2 = _penY2;
        ScreenToImage(&nx1, &ny1);
        ScreenToImage(&nx2, &ny2);

        int viewW = _viewPortRect.Right - _viewPortRect.Left + 1;
        int viewH = _viewPortRect.Bottom - _viewPortRect.Top + 1;

        nz = min((double)viewW / (double)(nx2 - nx1), (double)viewH / (double)(ny2 - ny1));
        SetZoom(nz);

    // Force the cache to rescan the image
    //_refreshCacheCanvas = true;
    }

    // Always repaint after changing anything    
    Repaint();       
}

void CfxControl_ZoomImageBase::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
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
                      _paintRect.Left + (int)((_panCenterX - _centerX) * _zoom), 
                      _paintRect.Top + (int)((_panCenterY - _centerY) * _zoom));
    }
    else
    {
        // Check if we have to refresh the cache canvas
        if (_refreshCacheCanvas)
        {
            _refreshCacheCanvas = false;

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
    _smoothPan = true;
}

CfxControl_ZoomImage::~CfxControl_ZoomImage()
{
    freeX(_bitmap);
}

void CfxControl_ZoomImage::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_ZoomImage::DefineProperties(CfxFiler &F)
{
    CfxControl_ZoomImageBase::DefineProperties(F);
    F.DefineXBITMAP("Bitmap", &_bitmap);
}

void CfxControl_ZoomImage::DefinePropertiesUI(CfxFilerUI &F) 
{
}

bool CfxControl_ZoomImage::GetImageData(int *pWidth, int *pHeight)
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

void CfxControl_ZoomImage::DrawImage(CfxCanvas *pDstCanvas, FXRECT *pDstRect, FXRECT *pSrcRect)
{
    CfxResource *resource = GetResource();
    
    FXBITMAPRESOURCE *bitmap = resource->GetBitmap(this, _bitmap);
    if (bitmap)
    {
        pDstCanvas->StretchDraw(bitmap, pSrcRect, pDstRect, false, false);
        resource->ReleaseBitmap(bitmap);
    }
}
