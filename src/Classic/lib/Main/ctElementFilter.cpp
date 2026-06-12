#include "fxApplication.h"
#include "fxUtils.h"
#include "ctSession.h"
#include "ctElementFilter.h"
#include "ctElementList.h"

CfxControl *Create_Control_ElementFilter(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementFilter(pOwner, pParent);
}

CctControl_ElementFilter::CctControl_ElementFilter(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTFILTER, false);
    InitLockProperties("Filter screen");
    InitBorder(ControlBorderStyle::Single, 1);

    _caption = 0;
    _minHeight = 0;
    _minWidth = 30;

    _textColor = _borderColor;

    _filtered = false;
    _penDown = _penOver = false;

    InitXGuid(&_filterScreenId);

    SetCaption("??");
}

CctControl_ElementFilter::~CctControl_ElementFilter()
{
    MEM_FREE(_caption);
}

void CctControl_ElementFilter::SetCaption(const char *pCaption)
{
    MEM_FREE(_caption);
    _caption = nullptr;

    if (pCaption)
    {
        _caption = (char *) MEM_ALLOC(strlen(pCaption) + 1);
        strcpy(_caption, pCaption);
    }
}

void CctControl_ElementFilter::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementFilter::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("TextColor",    dtColor,    &_textColor);
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("MinHeight",    dtInt32,    &_minHeight);
    F.DefineValue("MinWidth",     dtInt32,    &_minWidth);

    F.DefineXOBJECT("FilterScreenId", &_filterScreenId);

    _filtered = false;
}

void CctControl_ElementFilter::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_ElementFilter::ExecuteFilter()
{
    CfxResource *resource = GetResource(); 
    FXSCREENDATA *screenData = (FXSCREENDATA *)resource->GetObject(this, _filterScreenId);
    if (screenData)
    {
        FX_ASSERT(screenData->Magic == MAGIC_SCREEN);

        CfxStream stream(&screenData->Data[0]);
        CfxReader reader(&stream);

        CfxScreen screen(GetOwner(), this);
        screen.DefineProperties(reader);
        
        CctControl_ElementList *elementList = (CctControl_ElementList *)screen.FindControlByType(&GUID_CONTROL_ELEMENTLIST);
        if (elementList)
        {
            char value[16];
            itoa(elementList->GetItemCount(), value, 10);
            SetCaption(value);
        }
        resource->ReleaseObject(screenData);
    }

    _filtered = true;
}

void CctControl_ElementFilter::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
	bool invert = _penDown && _penOver;
    
    _color = pCanvas->InvertColor(_color, invert);
    _textColor = pCanvas->InvertColor(_textColor, invert);

    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    if (!_filtered)
    {
        ExecuteFilter();
    }

    FXRECT rect = *pRect;
    rect.Left = rect.Right - _oneSpace * 10;
    pCanvas->DrawNextArrow(&rect, _color, _textColor, false);

    if (_caption && (strlen(_caption) > 0))
    {
        CfxResource *resource = GetResource(); 
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            pCanvas->State.TextColor = _textColor;
            rect.Left = pRect->Left;
            rect.Right -= 4 * _oneSpace;
            pCanvas->DrawTextRect(font, &rect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, _caption);
            resource->ReleaseFont(font);
        }
    }

    _color = pCanvas->InvertColor(_color, invert);
    _textColor = pCanvas->InvertColor(_textColor, invert);
}

void CctControl_ElementFilter::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _height = max((int)_minHeight, _height);
    _width = max((int)_minWidth, _width);
}

void CctControl_ElementFilter::OnPenDown(int X, int Y)
{
	_penDown = _penOver = true;
    Repaint();
}

void CctControl_ElementFilter::OnPenUp(int X, int Y)
{
    if (!_penDown) return;
    _penDown = false;

    Repaint();
}

void CctControl_ElementFilter::OnPenMove(int X, int Y)
{
    if (!_penDown) return;

    if (_penOver != ((X >= 0) && (X < _width) && (Y >= 0) && (Y < _height)))
    {
		_penOver = !_penOver;
        Repaint();
	}
}

void CctControl_ElementFilter::OnPenClick(int X, int Y)
{
    if (!IsNullXGuid(&_filterScreenId))
    {
        GetSession(this)->DoSkipToScreen(_filterScreenId);
    }
}
