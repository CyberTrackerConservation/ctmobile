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
#include "fxTypes.h"

class CfxProjection
{
public:
    virtual VOID Project(DOUBLE Longitude, DOUBLE Latitude, DOUBLE *pX, DOUBLE *pY) = 0;
    virtual VOID Unproject(DOUBLE X, DOUBLE Y, DOUBLE *pLongitude, DOUBLE *pLatitude) = 0;
};

class CfxProjectionVirtualEarth: public CfxProjection
{
private:
    UINT _zoomLevel;
    DOUBLE _arc, _k;
 public:
    CfxProjectionVirtualEarth(UINT ZoomLevel);

    VOID Project(DOUBLE Longitude, DOUBLE Latitude, DOUBLE *pX, DOUBLE *pY);
    VOID Unproject(DOUBLE X, DOUBLE Y, DOUBLE *pLongitude, DOUBLE *pLatitude);
};

class CfxProjectionUTM: public CfxProjection
{
private:
    INT _zone;
    DOUBLE _a, _f;
    DOUBLE _centralMeridian;
    DOUBLE _scale;
    DOUBLE _falseEasting, _falseNorthing;
    DOUBLE _b, _es, _ebs, _ap, _bp, _cp, _dp, _ep;
 public:
    CfxProjectionUTM(INT Zone, CHAR *pDatum);

    VOID Project(DOUBLE Longitude, DOUBLE Latitude, DOUBLE *pX, DOUBLE *pY);
    VOID Unproject(DOUBLE X, DOUBLE Y, DOUBLE *pLongitude, DOUBLE *pLatitude);
};

VOID SetProjectionParams(FXPROJECTION Projection, BYTE UTMZone);
VOID GetProjectedParams(CHAR **ppLon, CHAR **ppLat, BOOL *pInvert);
VOID GetProjectedStrings(DOUBLE Lon, DOUBLE Lat, CHAR *pLon, CHAR *pLat);
