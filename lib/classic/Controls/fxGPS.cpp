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

#include "fxGPS.h"
#include "fxApplication.h"
#include "fxUtils.h"
#include "fxProjection.h"

#include "fxMath.h"

CHAR *GetHeadingText(INT Heading)
{
    struct HEADING
    {
        INT Start, End;
        CHAR *Caption;
    };

    static const HEADING headings[] =
    {
        {   0 , +15, "N"  },
        {  75 , 105, "E"  },
        { 165 , 195, "S"  },
        { 255 , 285, "W"  },
        {  15 ,  75, "NE" },
        { 105 , 165, "SE" },
        { 195 , 255, "SW" },
        { 285 , 345, "NW" },
        { 345 , 360, "N"  }
    };

    INT i;
    CHAR *headingText = NULL;

    Heading %= 360;
    for (i = 0; i < ARRAYSIZE(headings); i++)
    {
        if ((Heading >= headings[i].Start) && (Heading <= headings[i].End))
        {
            headingText = headings[i].Caption;
            break;
        }
   }

    return headingText;
}

VOID PropagateGPSAccuracy(CfxControl *pControl, const FXGPS_ACCURACY *pAccuracy)
{
    if (CompareGuid(pControl->GetTypeId(), &GUID_CONTROL_GPS))
    {
        ((CfxControl_GPS *)pControl)->SetAccuracy(pAccuracy);
    }

    for (UINT i=0; i<pControl->GetControlCount(); i++)
    {
        PropagateGPSAccuracy(pControl->GetControl(i), pAccuracy);
    }
}

CfxControl *Create_Control_GPS(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_GPS(pOwner, pParent);
}

CfxControl_GPS::CfxControl_GPS(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_GPS);
    InitLockProperties("Auto connect;Style");
    InitBorder(bsSingle, 1);
    
    _connected = FALSE;
    _caption = NULL;

    _autoConnect = FALSE;
    InitGpsAccuracy(&_accuracy);
    _extraData = FALSE;

    _onClick.Object = NULL;

    _autoHeight = FALSE;

    InitFont(F_FONT_DEFAULT_S);
    _textColor = _borderColor;

    _style = gcsData;
}

CfxControl_GPS::~CfxControl_GPS()
{
    if (_connected)
    {
        GetEngine(this)->UnlockGPS();
    }
}

VOID CfxControl_GPS::UpdateHeight()
{
    if (_autoHeight && ((_dock == dkTop) || (_dock == dkBottom) || (_dock == dkNone)))
    {
        DOUBLE rows = 0;

        switch (_style)
        {
        case gcsState:
            rows = 1;
            break;

        case gcsData:
            if (_extraData)
            {
                rows = 7;
            }
            else
            {
                rows = 4;
            }
            break;

        case gcsSky:
            rows = 0;
            break;

        case gcsBars:
            rows = 0;
            break;

        case gcsCompass:
        case gcsCompass2:
            rows = 0;
            break;

        case gcsHeading:
            rows = 1;
            break;

        case gcsTriangle:
            rows = 0;
            break;

        case gcsGotoData:
            rows = 3.6;
            break;

        case gcsGotoPointer:
            rows = 0;
            break;

        default:
            FX_ASSERT(FALSE);
        }

        if (rows != 0)
        {
            ResizeHeight(max(16, (UINT)(rows * GetDefaultFontHeight() + (UINT)_borderWidth * 2 + _borderLineWidth * 2 + 4)));
        }
    }
}

VOID CfxControl_GPS::OnPenClick(INT X, INT Y)
{
    if (_onClick.Object)
    {
        ExecuteEvent(&_onClick, this);
    }
}

VOID CfxControl_GPS::SetOnClick(CfxControl *pControl, NotifyMethod OnClick)
{
    _onClick.Object = pControl;
    _onClick.Method = OnClick;
}

VOID CfxControl_GPS::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    F.DefineFont(_font);
#endif
}

VOID CfxControl_GPS::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("AutoConnect",  dtBoolean,  &_autoConnect, F_FALSE);
    F.DefineValue("AutoHeight",   dtBoolean,  &_autoHeight, F_FALSE);
    F.DefineValue("ExtraData",    dtBoolean,  &_extraData, F_FALSE);
    F.DefineValue("TextColor",    dtColor,    &_textColor, F_COLOR_BLACK);
    F.DefineValue("Style",        dtInt8,     &_style, F_ONE); 

    if (F.IsReader())
    {
        UpdateHeight();
    }
}

VOID CfxControl_GPS::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Auto connect", dtBoolean,  &_autoConnect);
    F.DefineValue("Auto height",  dtBoolean,  &_autoHeight);
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Extra data",   dtBoolean,  &_extraData);
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Style",        dtByte,     &_style,       "LIST(State=0 Data=1 Sky=2 Bars=3 \"Compass simple\"=7 Compass=8 Heading=9 Triangle=4 \"Goto data\"=5 \"Goto pointer\"=6)");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CfxControl_GPS::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    UpdateHeight();
}

VOID CfxControl_GPS::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);
    UpdateHeight();
}

VOID CfxControl_GPS::PaintStyleState(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    CHAR caption[64]="";
    switch (pGpsData->State)
    {
    case dsNotFound:     strcpy(caption, "Not Found"); break;
    case dsDisconnected: strcpy(caption, "Off");       break;
    case dsDetecting:    strcpy(caption, "Detecting"); break;
    case dsConnected:
        if (TestGPS(pGpsData, GPS_DEFAULT_ACCURACY))
        {
            switch (pGpsData->Position.Quality)
            {
            case fqNone:   strcpy(caption, "Acquiring");    break;
            case fq2D:     strcpy(caption, "2D GPS");       break;
            case fq3D:     strcpy(caption, "3D GPS");       break;
            case fqDiff3D: strcpy(caption, "Differential"); break;
            case fqSim3D:  strcpy(caption, "Simulator");    break;
            default:
                strcpy(caption, "Unknown Fix");
            }
        }
        else
        {
            strcpy(caption, "Acquiring");
        }
        break;
    default:
        FX_ASSERT(FALSE);
    }

    pCanvas->State.TextColor = _textColor;
    pCanvas->DrawTextRect(pFont, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, caption);
}

VOID CfxControl_GPS::PaintStyleData(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    CHAR captionLat[32], captionLon[32];
    CHAR caption[32] = "000" DEGREE_SYMBOL_UTF8 "00'00.000\" W";
    CHAR captionF[16];
    BOOL invert;
    INT fontHeight = (INT)pCanvas->FontHeight(pFont);

    GetProjectedParams(NULL, NULL, &invert);
    GetProjectedStrings(pGpsData->Position.Longitude, pGpsData->Position.Latitude, captionLon, captionLat);

    BOOL useExtraData = _extraData && (8 * (fontHeight + 2) < (pRect->Bottom - pRect->Top));

    UINT rowCount = 4;
    rowCount += (useExtraData ? 3 : 0);
    rowCount += ((useExtraData && pGpsData->DateTime.Year) ? 1 : 0);
    UINT dy = _oneSpace * 4;
    UINT dataH = (fontHeight + dy) * rowCount;

    UINT dataW = pCanvas->TextWidth(pFont, caption) + fontHeight + 4;
    UINT w = pRect->Right - pRect->Left + 1;
    UINT h = pRect->Bottom - pRect->Top + 1;

    FXRECT dataRect = *pRect;
    InflateRect(&dataRect, w > dataW ? -(INT)(w - dataW)/2 : 0, h > dataH ? -(INT)(h - dataH)/2 : 0);

    UINT x = dataRect.Left + fontHeight;
    UINT y = dataRect.Top;
    UINT radius = fontHeight / 2;
    UINT xadd = _oneSpace * 4;
    pCanvas->State.TextColor = _textColor;
    pCanvas->State.LineColor = _borderColor;
    pCanvas->State.BrushColor = _borderColor;

    if (invert)
    {
        // Longitude
        pCanvas->Ellipse(dataRect.Left + radius, y + radius, radius, radius);
        pCanvas->Line(dataRect.Left + radius, y, dataRect.Left + radius, y + radius*2);
        pCanvas->DrawText(pFont, x + xadd, y, captionLon);
        y += fontHeight + dy;

        // Latitude
        pCanvas->Ellipse(dataRect.Left + radius, y + radius, radius, radius);
        pCanvas->Line(dataRect.Left, y + radius, dataRect.Left + radius*2, y + radius);
        pCanvas->DrawText(pFont, x + xadd, y, captionLat);
        y += fontHeight + dy;
    }
    else
    {
        // Latitude
        pCanvas->Ellipse(dataRect.Left + radius, y + radius, radius, radius);
        pCanvas->Line(dataRect.Left, y + radius, dataRect.Left + radius*2, y + radius);
        pCanvas->DrawText(pFont, x + xadd, y, captionLat);
        y += fontHeight + dy;

        // Longitude
        pCanvas->Ellipse(dataRect.Left + radius, y + radius, radius, radius);
        pCanvas->Line(dataRect.Left + radius, y, dataRect.Left + radius, y + radius*2);
        pCanvas->DrawText(pFont, x + xadd, y, captionLon);
        y += fontHeight + dy;
    }

    // Altitude
    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{radius, 0}, {radius*2, radius*2}, {0, radius*2}};
    POLYGON(pCanvas, triangle, dataRect.Left, y);
    sprintf(caption, "%ld m", (INT)pGpsData->Position.Altitude);
    pCanvas->DrawText(pFont, x + xadd, y, caption);
    y += fontHeight + dy;

    // Accuracy
    pCanvas->Ellipse(dataRect.Left + radius, y + radius, radius, radius);
    INT xc = dataRect.Left + radius;
    INT yc = y + radius;
    INT r2 = radius/2;
    pCanvas->Line(xc - r2, yc - r2, xc + r2, yc + r2);
    pCanvas->Line(xc - r2, yc + r2, xc + r2, yc - r2);
    
    DOUBLE accuracy = pGpsData->Position.Accuracy;
    if (pGpsData->Position.Quality == fqNone)
    {
        accuracy = 50.0;
    }
    PrintF(caption, accuracy, sizeof(caption)-1, 1);
    pCanvas->State.TextColor = (pGpsData->Position.Accuracy <= _accuracy.DilutionOfPrecision) ? _textColor : 0xFF;
    pCanvas->DrawText(pFont, x + xadd, y, caption);
    pCanvas->State.TextColor = _textColor;
    y += fontHeight + dy;

    // Extra data
    if (!useExtraData) return;

    DOUBLE dateTimeEncoded = EncodeDateTime(&pGpsData->DateTime);
    dateTimeEncoded += ((DOUBLE)GetEngine(this)->GetGPSTimeZone() / 100) / 24;
    FXDATETIME dateTime;
    DecodeDateTime(dateTimeEncoded, &dateTime);

    // Day
    if (pGpsData->DateTime.Year != 0)
    {
        pCanvas->FrameRect(dataRect.Left, y, radius*2 + 1, radius*2 + 1);
        pCanvas->MoveTo(dataRect.Left, y + radius);
        pCanvas->LineTo(dataRect.Left + radius*2, y + radius);
        pCanvas->MoveTo(dataRect.Left + radius, y);
        pCanvas->LineTo(dataRect.Left + radius, y + radius*2);

        CHAR *pDate = GetHost(this)->GetDateString(&dateTime);
        pCanvas->DrawText(pFont, x + xadd, y, pDate);
        MEM_FREE(pDate);

        y += fontHeight + dy;
    }

    // Time
    pCanvas->Ellipse(dataRect.Left + radius, y + radius, radius, radius);
    pCanvas->MoveTo(dataRect.Left + radius, y);
    pCanvas->LineTo(dataRect.Left + radius, y + radius);
    pCanvas->LineTo(dataRect.Left + radius * 2, y + radius);
    sprintf(caption, "%02d:%02d:%02d", dateTime.Hour, dateTime.Minute, dateTime.Second);
    pCanvas->DrawText(pFont, x + xadd, y, caption);
    y += fontHeight + dy;

    // Speed
    UINT ty = y - 1;
    UINT t = radius / 2;
    pCanvas->MoveTo(dataRect.Left, y + t);
    pCanvas->LineTo(dataRect.Left + radius*2, y + t);
    pCanvas->LineTo(dataRect.Left + radius*2 - t, y);
    pCanvas->MoveTo(dataRect.Left + radius*2, y + t);
    pCanvas->LineTo(dataRect.Left + radius*2 - t, y + radius);

    ty += t + 2;
    pCanvas->MoveTo(dataRect.Left, ty + t);
    pCanvas->LineTo(dataRect.Left + radius*2, ty + t);
    pCanvas->LineTo(dataRect.Left + radius*2 - t, ty);
    pCanvas->MoveTo(dataRect.Left + radius*2, ty + t);
    pCanvas->LineTo(dataRect.Left + radius*2 - t, ty + radius);

    PrintF(captionF, pGpsData->Position.Speed, sizeof(captionF)-1, 2);
    sprintf(caption, "%s km/h", captionF);
    pCanvas->State.TextColor = (pGpsData->Position.Speed <= _accuracy.MaximumSpeed) ? _textColor : 0xFF;
    pCanvas->DrawText(pFont, x + xadd, y, caption);
    pCanvas->State.TextColor = _textColor;
    y += fontHeight + dy;

    // Heading
    FXPOINT triangle2[] = {{radius/2, 0}, {radius, radius}, {0, radius}};
    POLYGON(pCanvas, triangle2, dataRect.Left + radius/2, y + radius/2);
    pCanvas->Ellipse(dataRect.Left + radius, y + radius, radius, radius);

    sprintf(caption, "%ld" DEGREE_SYMBOL_UTF8, (INT)pGpsData->Heading);
    pCanvas->DrawText(pFont, x + xadd, y, caption);
    y += fontHeight + dy;
}

VOID CfxControl_GPS::PaintStyleSky(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    UINT i;
    INT offsetY;
    UINT satelliteTextW = pCanvas->TextWidth(pFont, "00", 2);
    UINT satelliteTextH = pCanvas->TextHeight(pFont, "0", 0, &offsetY);

    FXRECT skyRect = *pRect;
    if (skyRect.Right - skyRect.Left > skyRect.Bottom - skyRect.Top)
    {
        skyRect.Right = skyRect.Left + (skyRect.Bottom - skyRect.Top + 1);
    }
    else
    {
        skyRect.Bottom = skyRect.Top + (skyRect.Right - skyRect.Left + 1);
    }
    UINT skyRadius = (skyRect.Right - skyRect.Left + 1)/2-1;
    OffsetRect(&skyRect, GetClientWidth()/2 - skyRadius, GetClientHeight()/2 - skyRadius);

    pCanvas->State.BrushColor = _borderColor;
    pCanvas->FillEllipse(skyRect.Left + skyRadius, skyRect.Top + skyRadius, skyRadius, skyRadius);

    pCanvas->State.LineColor = _color;
    pCanvas->FillEllipse(skyRect.Left + skyRadius, skyRect.Top + skyRadius, skyRadius/3, skyRadius/3);
    pCanvas->Ellipse(skyRect.Left + skyRadius, skyRect.Top + skyRadius, skyRadius/2, skyRadius/2);

    UINT spacer = satelliteTextH / 3;
    pCanvas->State.LineWidth = 1;
    pCanvas->State.TextColor = _textColor;
    for (i=0; i<12; i++)
    {
        FXGPS_SATELLITE *satellite = &pGpsData->Satellites[i];
        if (satellite->PRN != 0) 
        {
            CHAR satelliteId[8];
            sprintf(satelliteId, "%02d", (INT16)satellite->PRN);

            INT azimuth = (satellite->Azimuth + 360) % 360;
            DOUBLE elevation = ((90 - satellite->Elevation) * skyRadius) / 90;

            INT posX = skyRadius + (INT)(elevation * sin(D2R * azimuth));
            INT posY = skyRadius - (INT)(elevation * cos(D2R * azimuth));

            FXRECT sat = {posX - 2 * spacer, posY - 2 * spacer, posX+satelliteTextW + spacer, posY+satelliteTextH + 2 * spacer};
            OffsetRect(&sat, skyRect.Left - satelliteTextW / 2, skyRect.Top - satelliteTextW / 2);

            //INT xc = sat.Left + (sat.Right - sat.Left + 1) / 2 - pCanvas->State.LineWidth;
            //INT yc = sat.Top + (sat.Bottom - sat.Top + 1) / 2;
            //INT r = (satelliteTextW * 2) / 3 + pCanvas->State.LineWidth * 2;
            
            pCanvas->State.BrushColor = satellite->UsedInSolution ? _borderColor : _color;
            pCanvas->FillRect(&sat);
            //pCanvas->FillEllipse(xc, yc, r, r);

            pCanvas->State.TextColor = satellite->UsedInSolution ? _color : _borderColor;
            FXRECT satText = sat;
            satText.Top = sat.Top + 2 * spacer - offsetY;
            pCanvas->DrawTextRect(pFont, &satText, TEXT_ALIGN_HCENTER, satelliteId);
            //pCanvas->DrawText(pFont, sat.Left + 2 * spacer, sat.Top + 2 * spacer - offsetY, satelliteId);

            pCanvas->State.LineColor = _color;
            pCanvas->FrameRect(&sat);
            //pCanvas->Ellipse(xc, yc, r, r);
        }
    }
}

VOID CfxControl_GPS::PaintStyleBars(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    FXRECT barsRect = *pRect;

    UINT i;
    UINT satelliteTextW = pCanvas->TextWidth(pFont, "00");
    UINT satelliteTextH = pCanvas->FontHeight(pFont);

    UINT barsHeightB = satelliteTextH + 2;
    UINT barsHeightA = barsRect.Bottom - barsRect.Top - barsHeightB;

    UINT barsWidth = barsRect.Right - barsRect.Left + 1;

    pCanvas->State.BrushColor = _borderColor;
    for (i=0; i<5; i++)
    {
        pCanvas->FillRect(barsRect.Left, barsRect.Top + (i * barsHeightA) / 4, barsWidth, (UINT)_borderLineWidth);
    }
    pCanvas->FillRect(barsRect.Left, barsRect.Bottom, barsWidth, (UINT)_borderLineWidth);

    // Draw state
    UINT columnWidth = (barsWidth - 11) / 12;
    for (i=0; i<12; i++)
    {
        FXGPS_SATELLITE *satellite = &pGpsData->Satellites[i];
        
        FXRECT bar = {barsRect.Left + i * (columnWidth + 1), barsRect.Bottom - barsHeightB, columnWidth - 1, barsRect.Bottom - barsHeightB + _borderLineWidth / 2};
        bar.Right += bar.Left;

        CHAR satelliteId[8] = "--";
        if (satellite->PRN != 0) 
        {
            bar.Top -= min((satellite->SignalQuality * barsHeightA) / GPS_MAX_SIGNAL_QUALITY, barsHeightA);
            pCanvas->State.LineColor = _borderColor;
            pCanvas->State.BrushColor = satellite->UsedInSolution ? _borderColor : _color;

            if (satellite->SignalQuality != 0)
            {
                pCanvas->Rectangle(&bar);
            }
            sprintf(satelliteId, "%02d", (INT16)satellite->PRN);
        }

        pCanvas->State.TextColor = _textColor;
        pCanvas->DrawText(pFont, bar.Left + (columnWidth - satelliteTextW) / 2, barsRect.Bottom - barsHeightB + 1, satelliteId);
    }
}

VOID CfxControl_GPS::PaintStyleCompass(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    INT heading = IsLive() ? GetEngine(this)->GetGPSLastHeading() : 0;
    BOOL fixGood = TestGPS(&pGpsData->Position);
    pCanvas->State.LineWidth = (UINT)_borderLineWidth;
    pCanvas->DrawCompass(pRect, NULL, _color, fixGood ? _borderColor : 0x757575, FALSE, heading, TRUE);
}

VOID CfxControl_GPS::PaintStyleCompass2(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    INT heading = IsLive() ? GetEngine(this)->GetGPSLastHeading() : 0;
    BOOL fixGood = TestGPS(&pGpsData->Position);
    pCanvas->State.LineWidth = (UINT)_borderLineWidth;
    pCanvas->DrawCompass(pRect, pFont, _color, fixGood ? _borderColor : 0x757575, FALSE, heading, TRUE);
}

VOID CfxControl_GPS::PaintStyleHeading(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    INT heading = IsLive() ? GetEngine(this)->GetGPSLastHeading() : 0;
    BOOL fixGood = TestGPS(&pGpsData->Position);
    pCanvas->State.LineWidth = (UINT)_borderLineWidth;

    CHAR captionHeading[16];
    sprintf(captionHeading, "%ld" DEGREE_SYMBOL_UTF8 " (%s)", (INT)heading, GetHeadingText(heading));

    pCanvas->DrawTextRect(pFont, pRect, TEXT_ALIGN_VCENTER | TEXT_ALIGN_HCENTER, captionHeading);
}

VOID CfxControl_GPS::PaintStyleTriangle(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    FXRECT triRect = *pRect;
    if (triRect.Right - triRect.Left > triRect.Bottom - triRect.Top)
    {
        triRect.Right = triRect.Left + (triRect.Bottom - triRect.Top + 1);
    }
    else
    {
        triRect.Bottom = triRect.Top + (triRect.Right - triRect.Left + 1);
    }
    INT triRadius = (triRect.Right - triRect.Left + 1)/2-1;
    OffsetRect(&triRect, GetClientWidth()/2 - triRadius, GetClientHeight()/2 - triRadius);

    FXPOINTLIST polygon;
    FXPOINT triangle[] = {{triRadius, 0}, {triRadius*2, triRadius*2}, {0, triRadius*2}};

    if (TestGPS(pGpsData, GPS_DEFAULT_ACCURACY))
    {
        pCanvas->State.BrushColor = _borderColor;
        FILLPOLYGON(pCanvas, triangle, triRect.Left, triRect.Top);
    }
    else
    {
        pCanvas->State.LineColor = _borderColor;
        POLYGON(pCanvas, triangle, triRect.Left, triRect.Top);
    }
}

VOID CfxControl_GPS::PaintStyleGotoData(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    CHAR captionTitle[64];
    CHAR captionHeading[16];
    CHAR captionDistance[16];
    UINT fontHeight = pCanvas->FontHeight(pFont);

    UINT rowCount = 3;
    UINT space4 = 4 * _oneSpace;
    UINT dataH = (fontHeight + space4 - 1) * rowCount + space4;

    UINT dataW = pCanvas->TextWidth(pFont, "222" DEGREE_SYMBOL_UTF8 " (SW)") + fontHeight + 4;
    UINT w = pRect->Right - pRect->Left + 1;
    UINT h = pRect->Bottom - pRect->Top + 1;

    FXRECT dataRect = *pRect;
    InflateRect(&dataRect, w > dataW ? -(INT)(w - dataW)/2 : 0, h > dataH ? -(INT)(h - dataH)/2 : 0);

    UINT x = dataRect.Left + fontHeight;
    UINT y = dataRect.Top;
    UINT radius = fontHeight / 2;
    pCanvas->State.TextColor = _textColor;
    pCanvas->State.LineColor = _borderColor;
    pCanvas->State.BrushColor = _borderColor;

    // Get the goto
    FXGOTO *pGoto = NULL;
    if (IsLive())
    {
        pGoto = GetEngine(this)->GetCurrentGoto();
    }

    // Build the captions
    if (!pGoto || !pGoto->Title[0])
    {
        strcpy(captionTitle, "-");
        strcpy(captionHeading, "-");
        strcpy(captionDistance, "-");
    }
    else if (!TestGPS(pGpsData))
    {
        strcpy(captionTitle, pGoto->Title);
        strcpy(captionHeading, "No fix");
        strcpy(captionDistance, "No fix");
    }
    else
    {
        sprintf(captionTitle, "%s", pGoto->Title);
        
        INT heading = CalcHeading(pGpsData->Position.Latitude, 
                                  pGpsData->Position.Longitude,
                                  pGoto->Y,
                                  pGoto->X);

        sprintf(captionHeading, "%ld" DEGREE_SYMBOL_UTF8 " (%s)", (INT)heading, GetHeadingText(heading));

        DOUBLE distance = CalcDistanceKm(pGpsData->Position.Latitude, 
                                         pGpsData->Position.Longitude,
                                         pGoto->Y,
                                         pGoto->X);


		CHAR captionF[16];
		if ((INT)distance == 0)
		{
		    distance *= 1000;
            PrintF(captionF, distance, sizeof(captionF)-1, 2);
            sprintf(captionDistance, "%s m", captionF);
		}
		else 
		{
            PrintF(captionF, distance, sizeof(captionF)-1, distance < 10 ? 3 : 1);
		    sprintf(captionDistance, "%s km", captionF);
		}
    }        

    // Title
    FXRECT rect = *pRect;
    rect.Top = y;
    rect.Bottom = y + fontHeight;
    pCanvas->DrawTextRect(pFont, &rect, TEXT_ALIGN_VCENTER | TEXT_ALIGN_HCENTER, captionTitle);
    y += fontHeight + space4 + space4;

    // Heading
    FXPOINTLIST polygon;
    FXPOINT triangle2[] = {{radius/2, 0}, {radius, radius}, {0, radius}};
    POLYGON(pCanvas, triangle2, dataRect.Left + radius/2, y + radius/2);
    pCanvas->Ellipse(dataRect.Left + radius, y + radius, radius, radius);
    pCanvas->DrawText(pFont, x + space4, y, captionHeading);
    y += fontHeight + space4;

    // Distance
    rect = dataRect;
    rect.Top = y;
    rect.Bottom = y + fontHeight;
    pCanvas->DrawRange(&rect, _borderColor, _textColor, FALSE, TRUE);
    pCanvas->DrawText(pFont, x + space4, y, captionDistance);
    y += fontHeight + space4;
}

VOID CfxControl_GPS::PaintStyleGotoPointer(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    // Get the goto
    FXGOTO *pGoto = NULL;
    if (IsLive())
    {
        pGoto = GetEngine(this)->GetCurrentGoto();
    }

    // Build the captions
    if (!pGoto || !pGoto->Title[0])
    {
        pCanvas->DrawTextRect(pFont, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, "?");
    }
    else if (!TestGPS(pGpsData))
    {
        pCanvas->DrawTextRect(pFont, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, "?");
    }
    else
    {
        INT heading = CalcHeading(pGpsData->Position.Latitude, 
                                  pGpsData->Position.Longitude,
                                  pGoto->Y,
                                  pGoto->X);
        heading = (360 + heading - pGpsData->Heading) % 360;
		pCanvas->State.LineWidth = _oneSpace;
        pCanvas->DrawHeading(pRect, _borderColor, _textColor, FALSE, heading, TRUE);
    }
}

VOID CfxControl_GPS::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (IsLive() && _autoConnect && !_connected)
    {
        _connected = TRUE;
        GetEngine(this)->LockGPS();
    }

    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    // Get GPS Data
    FXGPS gps;
    if (IsLive())
    {
        SetPaintTimer(2000);
        gps = *(GetEngine(this)->GetGPS());
    }
    else
    {
        memset(&gps, 0, sizeof(gps));
    }

    // Get the font
    CfxResource *resource = GetResource(); 
    FXFONTRESOURCE *font = resource->GetFont(this, _font);
    pCanvas->State.LineWidth = (UINT)_borderLineWidth;

    switch (_style)
    {
    case gcsState:
        PaintStyleState(pCanvas, pRect, font, &gps);
        break;

    case gcsData:
        PaintStyleData(pCanvas, pRect, font, &gps);
        break;

    case gcsSky:
        PaintStyleSky(pCanvas, pRect, font, &gps);
        break;

    case gcsCompass:
        PaintStyleCompass(pCanvas, pRect, font, &gps);
        break;

    case gcsCompass2:
        PaintStyleCompass2(pCanvas, pRect, font, &gps);
        break;

    case gcsHeading:
        PaintStyleHeading(pCanvas, pRect, font, &gps);
        break;

    case gcsBars:
        PaintStyleBars(pCanvas, pRect, font, &gps);
        break;

    case gcsTriangle:
        PaintStyleTriangle(pCanvas, pRect, font, &gps);
        break;

    case gcsGotoData:
        PaintStyleGotoData(pCanvas, pRect, font, &gps);
        break;

    case gcsGotoPointer:
        PaintStyleGotoPointer(pCanvas, pRect, font, &gps);
        break;

    default:
        FX_ASSERT(FALSE);
    }

    resource->ReleaseFont(font);
}
