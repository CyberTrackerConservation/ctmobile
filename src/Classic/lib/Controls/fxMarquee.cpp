#include "fxApplication.h"
#include "fxMarquee.h"
#include "fxUtils.h"

CfxControl *Create_Control_Marquee(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Marquee(pOwner, pParent);
}

CfxControl_Marquee::CfxControl_Marquee(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_MARQUEE, true);
    InitLockProperties("Caption;Scroll Rate");
    
    _caption = 0;

    InitFont(F_FONT_DEFAULT_M);
    _textColor = _borderColor;

    _timeDatum = 0;
    _rate = 15;
}

CfxControl_Marquee::~CfxControl_Marquee()
{
    MEM_FREE(_caption);
    _caption = nullptr;
}

void CfxControl_Marquee::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_Marquee::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
    F.DefineValue("Caption",      dtPText,    &_caption);
    F.DefineValue("ScrollRate",   dtInt32,    &_rate, "15");
}

void CfxControl_Marquee::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CfxControl_Marquee::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    SetPaintTimer(50);

    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    CfxResource *resource = GetResource(); 

    if (_caption && (strlen(_caption) > 0))
    {
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            int w = pRect->Right - pRect->Left + 1;
            int h = pRect->Bottom - pRect->Top + 1;
            int x = (w - pCanvas->TextWidth(font, _caption)) / 2;
            int y = (h - pCanvas->FontHeight(font)) / 2;

            if (IsLive())
            {
                int time = FxGetTickCount();
                if (_timeDatum == 0)
                {
                    _timeDatum = time;
                }

                int pixelsPerSecond = _rate;
                x = w - (int)(((time - _timeDatum) * pixelsPerSecond / FxGetTicksPerSecond()) % (w + pCanvas->TextWidth(font, _caption)));
            }

            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawText(font, pRect->Left + x, pRect->Top + y, _caption);
            resource->ReleaseFont(font);
        }
    }
}
