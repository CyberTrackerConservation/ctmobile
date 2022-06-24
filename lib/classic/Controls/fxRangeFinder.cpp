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

#include "fxRangeFinder.h"
#include "fxApplication.h"
#include "fxUtils.h"
#include "fxMath.h"
#include "fxLaserAtlanta.h"

CfxControl *Create_Control_RangeFinder(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_RangeFinder(pOwner, pParent);
}

CfxControl_RangeFinder::CfxControl_RangeFinder(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_RANGEFINDER);
    InitLockProperties("Style");
    InitBorder(bsSingle, 1);
    
    InitFont(F_FONT_DEFAULT_S);
    _textColor = _borderColor;

    memset(&_range, 0, sizeof(_range));

    _legacyMode = TRUE;
    _keepConnected = FALSE;
    _portId = _portIdOverride = _portIdConnected = 0;
    InitCOM(&_portId, &_comPort);
    _connected = _tryConnect = _hasData = _showPortState = FALSE;
    _parser = new CfxLaserAtlantaParser();

    _style = rcsRange;
}

CfxControl_RangeFinder::~CfxControl_RangeFinder()
{
    Reset(_keepConnected);

    delete _parser;
    _parser = NULL;
}

VOID CfxControl_RangeFinder::Reset(BOOL KeepConnected)
{
    if (_connected)
    {
        CfxEngine *engine = GetEngine(this);

        if (_legacyMode)
        {
            engine->UnlockRangeFinder();
        }
        else
        {
            engine->DisconnectPort(this, _portIdConnected, KeepConnected);
        }
    }

    KillTimer();

    _portIdConnected = 0;
    _connected = _tryConnect = _hasData = FALSE;
}

VOID CfxControl_RangeFinder::ResetRange()
{
    memset(&_range, 0, sizeof(_range));
}

VOID CfxControl_RangeFinder::OnPortData(BYTE PortId, BYTE *pData, UINT Length)
{
    if (PortId != _portIdConnected) return;
    if (IsRangeLocked()) return;

    /*
    CHAR * r = "$PLTIT,HV,34.2,F,176.8,D,6.52,D,34.5,F*59\r\n";
    pData = (BYTE*)r;
    Length = strlen(r);
    */

    _parser->ParseBuffer((CHAR *)pData, Length);

    UINT i;

    for (i = 0; i < Length; i++, pData++)
    {
        if ((*pData == 10) || (*pData == 13))
        {
            _hasData = TRUE;

            if (_parser->GetRange(&_range, 3))
            {
                OnRange(&_range);
                return;
            }

            Repaint();
            break;
        }
    }
}

VOID CfxControl_RangeFinder::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
#endif
}

VOID CfxControl_RangeFinder::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("KeepConnected", dtBoolean, &_keepConnected, F_FALSE);
    F.DefineValue("LegacyMode",   dtBoolean,  &_legacyMode, F_TRUE);
    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
    F.DefineValue("Style",        dtInt8,     &_style, F_ONE); 
    F.DefineValue("ShowPortState",dtBoolean,  &_showPortState, F_FALSE);

    F.DefineCOM(&_portId, &_comPort);

    if (F.IsReader() && IsLive() && !_connected)
    {
        _portIdOverride = _portIdConnected = 0;
        _tryConnect = TRUE;
    }
}

VOID CfxControl_RangeFinder::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("COM - Auto detect", dtBoolean, &_legacyMode);
    F.DefineCOM(&_portId, &_comPort);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Keep connected", dtBoolean, &_keepConnected);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Show port state", dtBoolean, &_showPortState);
    F.DefineValue("Style",        dtByte,     &_style,       "LIST(State Range All)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CfxControl_RangeFinder::PaintStyleState(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXRANGE *pRange)
{
    CHAR caption[64]="";
    switch (pRange->State)
    {
    case dsNotFound:     strcpy(caption, "-"); break;
    case dsDisconnected: strcpy(caption, "Off");       break;
    case dsDetecting:    strcpy(caption, "Detecting"); break;
    case dsConnected:
        if (pRange->TimeStamp > 0)
        {
            strcpy(caption, "Connected");
        }
        else
        {
            strcpy(caption, "Acquiring");
        }
        break;
    default:
        FX_ASSERT(FALSE);
    }

    if (IsLive() && !_legacyMode && (!GetEngine(this)->IsPortConnected(_portIdConnected)))
    {
        strcpy(caption, "Tap to retry");
    }

    pCanvas->State.TextColor = _textColor;
    pCanvas->DrawTextRect(pFont, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, caption);
}

VOID CfxControl_RangeFinder::PaintStyleRange(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXRANGE *pRange)
{
    CHAR caption[32]="999.9" DEGREE_SYMBOL_UTF8;
    CHAR captionF[16];
    UINT fontHeight = pCanvas->FontHeight(pFont);
    UINT dataH = fontHeight + 3;
    UINT dataW = pCanvas->TextWidth(pFont, caption) + fontHeight + 4;
    UINT w = pRect->Right - pRect->Left + 1;
    UINT h = pRect->Bottom - pRect->Top + 1;

    FXRECT dataRect = *pRect;
    InflateRect(&dataRect, w > dataW ? -(INT)(w - dataW)/2 : 0, h > dataH ? -(INT)(h - dataH) / 2 : 0);
    dataRect.Bottom = dataRect.Top + fontHeight;
    dataRect.Right = dataRect.Left + fontHeight;

    UINT x = dataRect.Right + 4;
    UINT y = dataRect.Top;
    
    pCanvas->DrawRange(&dataRect, _borderColor, _textColor, FALSE, TRUE);
    PrintF(captionF, pRange->Range.Range, sizeof(captionF) - 1, 2);
    sprintf(caption, "%s %c", captionF, pRange->Range.RangeUnits ? pRange->Range.RangeUnits : 'M');
    pCanvas->State.TextColor = _textColor;
    pCanvas->DrawText(pFont, x, y, caption);
}

VOID CfxControl_RangeFinder::PaintStyleAll(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXRANGE *pRange)
{
    // Count the number of fields shown
    UINT FlagMask = RANGE_FLAG_RANGE | 
                      RANGE_FLAG_POLAR_BEARING | RANGE_FLAG_POLAR_INCLINATION | RANGE_FLAG_POLAR_ROLL |
                      RANGE_FLAG_AZIMUTH | RANGE_FLAG_INCLINATION;
    UINT FieldCount = 0;
    
    FlagMask &= pRange->Range.Flags;
    if (FlagMask == 0)
    {
        FieldCount = 3;
        FlagMask |= (RANGE_FLAG_RANGE | RANGE_FLAG_AZIMUTH | RANGE_FLAG_INCLINATION);
        pRange->Range.RangeUnits = 'M';
    }
    else
    {
        UINT i;
        for (i = 0; i < 32; i++)
        {
            if (FlagMask & (1 << i))
            {
                FieldCount++;
            }
        }
    }

    CHAR caption[32]="999.9" DEGREE_SYMBOL_UTF8;
    CHAR captionF[16];

    UINT fontHeight = pCanvas->FontHeight(pFont);
    UINT dataH = (fontHeight + 3) * FieldCount;
    UINT dataW = pCanvas->TextWidth(pFont, caption) + fontHeight + 4;
    UINT w = pRect->Right - pRect->Left + 1;
    UINT h = pRect->Bottom - pRect->Top + 1;
    UINT dh = fontHeight + 4;
    
    FXRECT dataRect = *pRect;
    InflateRect(&dataRect, w > dataW ? -(INT)(w - dataW)/2 : 0, h > dataH ? -(INT)(h - dataH)/2 : 0);

    FXRECT iconRect = dataRect;
    iconRect.Bottom = iconRect.Top + fontHeight;
    FXRECT textRect = dataRect;
    textRect.Bottom = textRect.Top + fontHeight;

    textRect.Left = dataRect.Left + dh;
    iconRect.Right = dataRect.Left + fontHeight;

    pCanvas->State.TextColor = _textColor;

    // Range
    if (FlagMask & RANGE_FLAG_RANGE)
    {
        pCanvas->DrawRange(&iconRect, _borderColor, _textColor, FALSE, TRUE);

        PrintF(captionF, pRange->Range.Range, sizeof(captionF)-1, 2);
        sprintf(caption, "%s %c", captionF, pRange->Range.RangeUnits);
        pCanvas->DrawText(pFont, textRect.Left, textRect.Top, caption);

        OffsetRect(&textRect, 0, dh);
        OffsetRect(&iconRect, 0, dh);
    }

    // PolarBearing
    if (FlagMask & RANGE_FLAG_POLAR_BEARING)
    {
        pCanvas->DrawBearing(&iconRect, _borderColor, _textColor, FALSE, TRUE);

        PrintF(captionF, pRange->Range.PolarBearing, sizeof(captionF)-1, 1);
        sprintf(caption, "%s" DEGREE_SYMBOL_UTF8, captionF);
        switch (pRange->Range.PolarBearingMode)
        {
        case 'M': strcat(caption, " magn"); break;
        case 'T': strcat(caption, " true"); break;
        case 'E': strcat(caption, " enco"); break;
        case 'D': strcat(caption, " diff"); break;
        }
        pCanvas->DrawText(pFont, textRect.Left, textRect.Top, caption);

        OffsetRect(&textRect, 0, dh);
        OffsetRect(&iconRect, 0, dh);
    }

    // PolarInclination
    if (FlagMask & RANGE_FLAG_POLAR_INCLINATION)
    {
        pCanvas->DrawInclination(&iconRect, _borderColor, _textColor, FALSE, TRUE);

        PrintF(captionF, pRange->Range.PolarInclination, sizeof(captionF)-1, 1);
        sprintf(caption, "%s" DEGREE_SYMBOL_UTF8, captionF);

        pCanvas->DrawText(pFont, textRect.Left, textRect.Top, caption);

        OffsetRect(&textRect, 0, dh);
        OffsetRect(&iconRect, 0, dh);
    }

    // PolarRoll
    if (FlagMask & RANGE_FLAG_POLAR_ROLL)
    {
        pCanvas->DrawRoll(&iconRect, _borderColor, _textColor, FALSE, TRUE);
        PrintF(captionF, pRange->Range.PolarRoll, sizeof(captionF)-1, 1);
        sprintf(caption, "%s" DEGREE_SYMBOL_UTF8, captionF);

        pCanvas->DrawText(pFont, textRect.Left, textRect.Top, caption);

        OffsetRect(&textRect, 0, dh);
        OffsetRect(&iconRect, 0, dh);
    }

    // Azimuth
    if (FlagMask & RANGE_FLAG_AZIMUTH)
    {
        pCanvas->DrawBearing(&iconRect, _borderColor, _textColor, FALSE, TRUE);

        PrintF(captionF, pRange->Range.Azimuth, sizeof(captionF)-1, 1);
        sprintf(caption, "%s" DEGREE_SYMBOL_UTF8, captionF);
        pCanvas->DrawText(pFont, textRect.Left, textRect.Top, caption);

        OffsetRect(&textRect, 0, dh);
        OffsetRect(&iconRect, 0, dh);
    }

    // Inclination
    if (FlagMask & RANGE_FLAG_INCLINATION)
    {
        pCanvas->DrawInclination(&iconRect, _borderColor, _textColor, FALSE, TRUE);

        PrintF(captionF, pRange->Range.Inclination, sizeof(captionF)-1, 1);
        sprintf(caption, "%s" DEGREE_SYMBOL_UTF8, captionF);

        pCanvas->DrawText(pFont, textRect.Left, textRect.Top, caption);

        OffsetRect(&textRect, 0, dh);
        OffsetRect(&iconRect, 0, dh);
    }
}

VOID CfxControl_RangeFinder::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    if (_tryConnect && IsLive())
    {
        if (_legacyMode)
        {
            Reset();
            _connected = TRUE;
            GetEngine(this)->LockRangeFinder();
            SetTimer(1000);
        }
        else
        {
            Reset();
            if (_portIdOverride != 0) 
            {
                _portIdConnected = _portIdOverride;
            }
            else
            {
                _portIdConnected = _portId;
            }

            _connected = GetEngine(this)->ConnectPort(this, _portIdConnected, &_comPort);
        }
    }

    // Get the font
    CfxResource *resource = GetResource(); 
    FXFONTRESOURCE *font = resource->GetFont(this, _font);
    pCanvas->State.LineWidth = (UINT)_borderLineWidth;

    switch (_style)
    {
    case rcsState:
        PaintStyleState(pCanvas, pRect, font, &_range);
        break;

    case rcsRange:
        PaintStyleRange(pCanvas, pRect, font, &_range);
        break;

    case rcsAll:
        PaintStyleAll(pCanvas, pRect, font, &_range);
        break;

    default:
        FX_ASSERT(FALSE);
    }

    if (_showPortState)
    {
        UINT spacer = 4 * _borderLineWidth;
        UINT size = _borderLineWidth * 24;
        FXRECT r = {pRect->Left + spacer + _borderWidth, pRect->Top + spacer + _borderWidth, pRect->Left + size, pRect->Top + size};
        pCanvas->DrawPortState(&r, font, _portIdConnected, _color, _textColor, _hasData, _transparent);
    }

    resource->ReleaseFont(font);
}

VOID CfxControl_RangeFinder::OnTimer()
{
    FxResetAutoOffTimer();

    if (_connected && !IsRangeLocked() && _legacyMode)
    {
        UINT lastTimeStamp = _range.TimeStamp;
        ResetRange();
        _range = *GetEngine(this)->GetRange();
        if (_range.TimeStamp != lastTimeStamp)
        {
            OnRange(&_range);
            return;
        }
    }

    Repaint();
}

VOID CfxControl_RangeFinder::OnRange(FXRANGE *pRange) 
{ 
    GetEngine(this)->SetLastRange(pRange);
    Repaint();
}

VOID CfxControl_RangeFinder::OnPenClick(INT X, INT Y)
{
    if (!_connected) return;

    ResetRange();

    // Reset the COM port if not correct connected
    if (!_legacyMode && !GetEngine(this)->IsPortConnected(_portIdConnected))
    {
        Reset();
        _tryConnect = TRUE;
        Repaint();
        FxSleep(1000);
    }

    Repaint();
}
