#include "fxSystem.h"
#include "fxUtils.h"

CfxControl *Create_Control_System(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_System(pOwner, pParent);
}

CfxControl_System::CfxControl_System(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SYSTEM, true);
    InitBorder(ControlBorderStyle::Single, 1);
    
    _caption = nullptr;
    _minHeight = 0;
    _alignment = TEXT_ALIGN_HCENTER;

    _style = SystemControlStyle::Time;

    _textColor = _borderColor;
}

CfxControl_System::~CfxControl_System()
{
    MEM_FREE(_caption);
	_caption = nullptr;
}

void CfxControl_System::SetCaption(char *pCaption)
{
    MEM_FREE(_caption);
    _caption = nullptr;
    _caption = AllocString(pCaption);
}

void CfxControl_System::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_System::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("Alignment",    dtInt32,    &_alignment, F_TWO);
    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
	F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("MinHeight",    dtInt32,    &_minHeight, F_ZERO);
    F.DefineValue("Style",        dtInt8,     &_style, F_ONE);
}

void CfxControl_System::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CfxControl_System::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    CfxResource *resource = GetResource(); 

    switch (_style)
    {
    case SystemControlStyle::Battery:
        pCanvas->State.LineWidth = max(1, _borderLineWidth);
        pCanvas->DrawBattery(GetHost(this)->GetBatteryLevel(), pRect, _alignment, _color, _textColor, false, _transparent);
        break;

    case SystemControlStyle::Time:
        {
            FXFONTRESOURCE *font = resource->GetFont(this, _font);
            if (font)
            {
                FXDATETIME dateTime;
                GetHost(this)->GetDateTime(&dateTime);
                pCanvas->DrawTime(&dateTime, pRect, font, _alignment | TEXT_ALIGN_VCENTER, _color, _textColor, false, _transparent);
                resource->ReleaseFont(font);
            }
        }
        break;
    }
}

void CfxControl_System::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _height = max((int)_minHeight, _height);
}

//*************************************************************************************************
