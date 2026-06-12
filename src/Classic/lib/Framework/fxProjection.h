#pragma once
#include "pch.h"
#include "fxTypes.h"

class CfxProjection
{
public:
    virtual void Project(double Longitude, double Latitude, double *pX, double *pY) = 0;
    virtual void Unproject(double X, double Y, double *pLongitude, double *pLatitude) = 0;
};

class CfxProjectionVirtualEarth: public CfxProjection
{
private:
    int _zoomLevel = 0;
    double _arc = 0.0, _k = 0.0;
 public:
    CfxProjectionVirtualEarth(int ZoomLevel);

    void Project(double Longitude, double Latitude, double *pX, double *pY);
    void Unproject(double X, double Y, double *pLongitude, double *pLatitude);
};

class CfxProjectionUTM: public CfxProjection
{
private:
    int _zone = 0;
    double _a = 0.0, _f = 0.0;
    double _centralMeridian = 0.0;
    double _scale = 0.0;
    double _falseEasting = 0.0, _falseNorthing = 0.0;
    double _b = 0.0, _es = 0.0, _ebs = 0.0, _ap = 0.0, _bp = 0.0, _cp = 0.0, _dp = 0.0, _ep = 0.0;
 public:
    CfxProjectionUTM(int Zone, const char *pDatum);

    void Project(double Longitude, double Latitude, double *pX, double *pY);
    void Unproject(double X, double Y, double *pLongitude, double *pLatitude);
};

void SetProjectionParams(FXPROJECTION Projection, BYTE UTMZone);
void GetProjectedParams(const char **ppLon, const char **ppLat, bool *pInvert);
void GetProjectedStrings(double Lon, double Lat, char *pLon, char *pLat);
