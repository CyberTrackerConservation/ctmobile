#include "fxGPS.h"
#include "fxApplication.h"
#include "fxUtils.h"
#include "fxProjection.h"

#include "fxMath.h"

const char *GetHeadingText(int Heading)
{
    struct HEADING
    {
        int Start, End;
        const char *Caption;
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

    int i;
    const char *headingText = nullptr;

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

void PropagateGPSAccuracy(CfxControl *pControl, const FXGPS_ACCURACY *pAccuracy)
{
    if (CompareGuid(pControl->GetTypeId(), &GUID_CONTROL_GPS))
    {
        ((CfxControl_GPS *)pControl)->SetAccuracy(pAccuracy);
    }

    for (int i=0; i<pControl->GetControlCount(); i++)
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
    InitBorder(ControlBorderStyle::Single, 1);
    
    _connected = false;
    _caption = nullptr;

    _autoConnect = false;
    InitGpsAccuracy(&_accuracy);
    _extraData = false;

    _onClick.Object = nullptr;

    _autoHeight = false;

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

void CfxControl_GPS::UpdateHeight()
{
    if (_autoHeight && ((_dock == ControlDock::Top) || (_dock == ControlDock::Bottom) || (_dock == ControlDock::None)))
    {
        double rows = 0;

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
            FX_ASSERT(false);
        }

        if (rows != 0)
        {
            ResizeHeight(max(16, (int)(rows * GetDefaultFontHeight() + (int)_borderWidth * 2 + _borderLineWidth * 2 + 4)));
        }
    }
}

void CfxControl_GPS::OnPenClick(int X, int Y)
{
    if (_onClick.Object)
    {
        ExecuteEvent(&_onClick, this);
    }
}

void CfxControl_GPS::SetOnClick(CfxControl *pControl, NotifyMethod OnClick)
{
    _onClick.Object = pControl;
    _onClick.Method = OnClick;
}

void CfxControl_GPS::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_GPS::DefineProperties(CfxFiler &F)
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

void CfxControl_GPS::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CfxControl_GPS::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    UpdateHeight();
}

void CfxControl_GPS::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);
    UpdateHeight();
}

void CfxControl_GPS::PaintStyleState(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    char caption[64]="";
    switch (pGpsData->State)
    {
    case FXDEVICE_STATE::NotFound:     strcpy(caption, "Not Found"); break;
    case FXDEVICE_STATE::Disconnected: strcpy(caption, "Off");       break;
    case FXDEVICE_STATE::Detecting:    strcpy(caption, "Detecting"); break;
    case FXDEVICE_STATE::Connected:
        if (TestGPS(pGpsData, GPS_DEFAULT_ACCURACY))
        {
            switch (pGpsData->Position.Quality)
            {
            case FXGPS_QUALITY::None:   strcpy(caption, "Acquiring");    break;
            case FXGPS_QUALITY::Fix2D:     strcpy(caption, "2D GPS");       break;
            case FXGPS_QUALITY::Fix3D:     strcpy(caption, "3D GPS");       break;
            case FXGPS_QUALITY::Diff3D: strcpy(caption, "Differential"); break;
            case FXGPS_QUALITY::Sim3D:  strcpy(caption, "Simulator");    break;
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
        FX_ASSERT(false);
    }

    pCanvas->State.TextColor = _textColor;
    pCanvas->DrawTextRect(pFont, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, caption);
}

void CfxControl_GPS::PaintStyleData(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    char captionLat[32], captionLon[32];
    char caption[32] = "000" DEGREE_SYMBOL_UTF8 "00'00.000\" W";
    char captionF[16];
    bool invert;
    int fontHeight = (int)pCanvas->FontHeight(pFont);

    GetProjectedParams(nullptr, nullptr, &invert);
    GetProjectedStrings(pGpsData->Position.Longitude, pGpsData->Position.Latitude, captionLon, captionLat);

    bool useExtraData = _extraData && (8 * (fontHeight + 2) < (pRect->Bottom - pRect->Top));

    int rowCount = 4;
    rowCount += (useExtraData ? 3 : 0);
    rowCount += ((useExtraData && pGpsData->DateTime.Year) ? 1 : 0);
    int dy = _oneSpace * 4;
    int dataH = (fontHeight + dy) * rowCount;

    int dataW = pCanvas->TextWidth(pFont, caption) + fontHeight + 4;
    int w = pRect->Right - pRect->Left + 1;
    int h = pRect->Bottom - pRect->Top + 1;

    FXRECT dataRect = *pRect;
    InflateRect(&dataRect, w > dataW ? -(int)(w - dataW)/2 : 0, h > dataH ? -(int)(h - dataH)/2 : 0);

    int x = dataRect.Left + fontHeight;
    int y = dataRect.Top;
    int radius = (int)fontHeight / 2;
    int xadd = _oneSpace * 4;
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
    sprintf(caption, "%d m", (int)pGpsData->Position.Altitude);
    pCanvas->DrawText(pFont, x + xadd, y, caption);
    y += fontHeight + dy;

    // Accuracy
    pCanvas->Ellipse(dataRect.Left + radius, y + radius, radius, radius);
    int xc = dataRect.Left + radius;
    int yc = y + radius;
    int r2 = radius/2;
    pCanvas->Line(xc - r2, yc - r2, xc + r2, yc + r2);
    pCanvas->Line(xc - r2, yc + r2, xc + r2, yc - r2);
    
    double accuracy = pGpsData->Position.Accuracy;
    if (pGpsData->Position.Quality == FXGPS_QUALITY::None)
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

    double dateTimeEncoded = EncodeDateTime(&pGpsData->DateTime);
    dateTimeEncoded += ((double)GetEngine(this)->GetGPSTimeZone() / 100) / 24;
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

        char *pDate = GetHost(this)->GetDateString(&dateTime);
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
    int ty = y - 1;
    int t = radius / 2;
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

    sprintf(caption, "%ld" DEGREE_SYMBOL_UTF8, (int)pGpsData->Heading);
    pCanvas->DrawText(pFont, x + xadd, y, caption);
    y += fontHeight + dy;
}

void CfxControl_GPS::PaintStyleSky(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    int i;
    int offsetY;
    int satelliteTextW = pCanvas->TextWidth(pFont, "00", 2);
    int satelliteTextH = pCanvas->TextHeight(pFont, "0", 0, &offsetY);

    FXRECT skyRect = *pRect;
    if (skyRect.Right - skyRect.Left > skyRect.Bottom - skyRect.Top)
    {
        skyRect.Right = skyRect.Left + (skyRect.Bottom - skyRect.Top + 1);
    }
    else
    {
        skyRect.Bottom = skyRect.Top + (skyRect.Right - skyRect.Left + 1);
    }
    int skyRadius = (skyRect.Right - skyRect.Left + 1)/2-1;
    OffsetRect(&skyRect, GetClientWidth()/2 - skyRadius, GetClientHeight()/2 - skyRadius);

    pCanvas->State.BrushColor = _borderColor;
    pCanvas->FillEllipse(skyRect.Left + skyRadius, skyRect.Top + skyRadius, skyRadius, skyRadius);

    pCanvas->State.LineColor = _color;
    pCanvas->FillEllipse(skyRect.Left + skyRadius, skyRect.Top + skyRadius, skyRadius/3, skyRadius/3);
    pCanvas->Ellipse(skyRect.Left + skyRadius, skyRect.Top + skyRadius, skyRadius/2, skyRadius/2);

    int spacer = (int)satelliteTextH / 3;
    pCanvas->State.LineWidth = 1;
    pCanvas->State.TextColor = _textColor;
    for (i=0; i<12; i++)
    {
        FXGPS_SATELLITE *satellite = &pGpsData->Satellites[i];
        if (satellite->PRN != 0) 
        {
            char satelliteId[8];
            sprintf(satelliteId, "%02d", (INT16)satellite->PRN);

            int azimuth = (satellite->Azimuth + 360) % 360;
            double elevation = ((90 - satellite->Elevation) * skyRadius) / 90;

            int posX = skyRadius + (int)(elevation * sin(D2R * azimuth));
            int posY = skyRadius - (int)(elevation * cos(D2R * azimuth));

            FXRECT sat = {posX - 2 * spacer, posY - 2 * spacer, posX + (int)satelliteTextW + spacer, posY + (int)satelliteTextH + 2 * spacer};
            OffsetRect(&sat, skyRect.Left - satelliteTextW / 2, skyRect.Top - satelliteTextW / 2);

            //int xc = sat.Left + (sat.Right - sat.Left + 1) / 2 - pCanvas->State.LineWidth;
            //int yc = sat.Top + (sat.Bottom - sat.Top + 1) / 2;
            //int r = (satelliteTextW * 2) / 3 + pCanvas->State.LineWidth * 2;
            
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

void CfxControl_GPS::PaintStyleBars(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    FXRECT barsRect = *pRect;

    int i;
    int satelliteTextW = pCanvas->TextWidth(pFont, "00");
    int satelliteTextH = pCanvas->FontHeight(pFont);

    int barsHeightB = satelliteTextH + 2;
    int barsHeightA = barsRect.Bottom - barsRect.Top - barsHeightB;

    int barsWidth = barsRect.Right - barsRect.Left + 1;

    pCanvas->State.BrushColor = _borderColor;
    for (i=0; i<5; i++)
    {
        pCanvas->FillRect(barsRect.Left, barsRect.Top + (i * barsHeightA) / 4, barsWidth, (int)_borderLineWidth);
    }
    pCanvas->FillRect(barsRect.Left, barsRect.Bottom, barsWidth, (int)_borderLineWidth);

    // Draw state
    int columnWidth = (barsWidth - 11) / 12;
    for (i=0; i<12; i++)
    {
        FXGPS_SATELLITE *satellite = &pGpsData->Satellites[i];
        
        FXRECT bar = {barsRect.Left + i * ((int)columnWidth + 1), barsRect.Bottom - (int)barsHeightB, (int)columnWidth - 1, barsRect.Bottom - (int)barsHeightB + (int)_borderLineWidth / 2};
        bar.Right += bar.Left;

        char satelliteId[8] = "--";
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

void CfxControl_GPS::PaintStyleCompass(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    int heading = IsLive() ? GetEngine(this)->GetGPSLastHeading() : 0;
    bool fixGood = TestGPS(&pGpsData->Position);
    pCanvas->State.LineWidth = (int)_borderLineWidth;
    pCanvas->DrawCompass(pRect, nullptr, _color, fixGood ? _borderColor : 0x757575, false, heading, true);
}

void CfxControl_GPS::PaintStyleCompass2(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    int heading = IsLive() ? GetEngine(this)->GetGPSLastHeading() : 0;
    bool fixGood = TestGPS(&pGpsData->Position);
    pCanvas->State.LineWidth = (int)_borderLineWidth;
    pCanvas->DrawCompass(pRect, pFont, _color, fixGood ? _borderColor : 0x757575, false, heading, true);
}

void CfxControl_GPS::PaintStyleHeading(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    int heading = IsLive() ? GetEngine(this)->GetGPSLastHeading() : 0;
    bool fixGood = TestGPS(&pGpsData->Position);
    pCanvas->State.LineWidth = (int)_borderLineWidth;

    char captionHeading[16];
    sprintf(captionHeading, "%ld" DEGREE_SYMBOL_UTF8 " (%s)", (int)heading, GetHeadingText(heading));

    pCanvas->DrawTextRect(pFont, pRect, TEXT_ALIGN_VCENTER | TEXT_ALIGN_HCENTER, captionHeading);
}

void CfxControl_GPS::PaintStyleTriangle(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
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
    int triRadius = (triRect.Right - triRect.Left + 1)/2-1;
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

void CfxControl_GPS::PaintStyleGotoData(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    char captionTitle[64];
    char captionHeading[16];
    char captionDistance[16];
    int fontHeight = pCanvas->FontHeight(pFont);

    int rowCount = 3;
    int space4 = 4 * _oneSpace;
    int dataH = (fontHeight + space4 - 1) * rowCount + space4;

    int dataW = pCanvas->TextWidth(pFont, "222" DEGREE_SYMBOL_UTF8 " (SW)") + fontHeight + 4;
    int w = pRect->Right - pRect->Left + 1;
    int h = pRect->Bottom - pRect->Top + 1;

    FXRECT dataRect = *pRect;
    InflateRect(&dataRect, w > dataW ? -(int)(w - dataW)/2 : 0, h > dataH ? -(int)(h - dataH)/2 : 0);

    int x = dataRect.Left + fontHeight;
    int y = dataRect.Top;
    int radius = (int)fontHeight / 2;
    pCanvas->State.TextColor = _textColor;
    pCanvas->State.LineColor = _borderColor;
    pCanvas->State.BrushColor = _borderColor;

    // Get the goto
    FXGOTO *pGoto = nullptr;
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
        
        int heading = CalcHeading(pGpsData->Position.Latitude, 
                                  pGpsData->Position.Longitude,
                                  pGoto->Y,
                                  pGoto->X);

        sprintf(captionHeading, "%d" DEGREE_SYMBOL_UTF8 " (%s)", (int)heading, GetHeadingText(heading));

        double distance = CalcDistanceKm(pGpsData->Position.Latitude, 
                                         pGpsData->Position.Longitude,
                                         pGoto->Y,
                                         pGoto->X);


		char captionF[16];
		if ((int)distance == 0)
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
    pCanvas->DrawRange(&rect, _borderColor, _textColor, false, true);
    pCanvas->DrawText(pFont, x + space4, y, captionDistance);
    y += fontHeight + space4;
}

void CfxControl_GPS::PaintStyleGotoPointer(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, FXGPS *pGpsData)
{
    // Get the goto
    FXGOTO *pGoto = nullptr;
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
        int heading = CalcHeading(pGpsData->Position.Latitude, 
                                  pGpsData->Position.Longitude,
                                  pGoto->Y,
                                  pGoto->X);
        heading = (360 + heading - pGpsData->Heading) % 360;
		pCanvas->State.LineWidth = _oneSpace;
        pCanvas->DrawHeading(pRect, _borderColor, _textColor, false, heading, true);
    }
}

void CfxControl_GPS::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (IsLive() && _autoConnect && !_connected)
    {
        _connected = true;
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
    pCanvas->State.LineWidth = (int)_borderLineWidth;

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
        FX_ASSERT(false);
    }

    resource->ReleaseFont(font);
}
