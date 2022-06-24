//
// This comes from the World Magnetic Model distro:
//   https://www.ngdc.noaa.gov/geomag/WMM
//
// It is good through 2025, although it degrades gracefully there-after.
//
// License and copyright:
//
//   The WMM source code is in the public domain and not licensed or under copyright.
//   The information and software may be used freely by the public.
//   As required by 17 U.S.C. 403, third parties producing copyrighted works consisting predominantly of the
//   material produced by U.S. government agencies must provide notice with such work(s) identifying the U.S.
//   Government material incorporated and stating that such material is not subject to copyright protection.
//

#include "geomag.h"

#include <memory>
#include <QDateTime>
#include <QFile>
#include <QTemporaryFile>

extern "C" {
#include "GeomagnetismHeader.h"
}

bool getMagneticDeclination(double latitude, double longitude, double altitude, float* result)
{
    MAGtype_MagneticModel * MagneticModels[1], *TimedMagneticModel;
    MAGtype_Ellipsoid Ellip;
    MAGtype_CoordSpherical CoordSpherical;
    MAGtype_CoordGeodetic CoordGeodetic;
    MAGtype_Date UserDate;
    MAGtype_GeoMagneticElements GeoMagneticElements, Errors;
    MAGtype_Geoid Geoid;
    int NumTerms, nMax = 0;
    int epochs = 1;

    *result = 0;

    QFile wmmFile(":/lib/geomag/WMM.COF");
    if (!wmmFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    auto tmpFile = std::unique_ptr<QTemporaryFile>(QTemporaryFile::createNativeFile(wmmFile));
    auto tmpFilename = QFile::encodeName(tmpFile->fileName());

    /* Memory allocation */
    if (!MAG_robustReadMagModels(tmpFilename.data(), (MAGtype_MagneticModel *(*)[])(&MagneticModels), epochs)) {
        return false;
    }

    if (nMax < MagneticModels[0]->nMax) nMax = MagneticModels[0]->nMax;
    NumTerms = ((nMax + 1) * (nMax + 2) / 2);
    TimedMagneticModel = MAG_AllocateModelMemory(NumTerms); /* For storing the time modified WMM Model parameters */
    if (MagneticModels[0] == NULL || TimedMagneticModel == NULL)
    {
        MAG_Error(2);
    }

    MAG_SetDefaults(&Ellip, &Geoid); /* Set default values and constants */
                                     /* Check for Geographic Poles */
                                     /* Set EGM96 Geoid parameters */
    Geoid.GeoidHeightBuffer = nullptr;  //GeoidHeightBuffer;
    Geoid.Geoid_Initialized = 1;

    CoordGeodetic.phi = latitude;
    CoordGeodetic.lambda = longitude;
    CoordGeodetic.HeightAboveEllipsoid = altitude / 1000;
    CoordGeodetic.HeightAboveGeoid = altitude / 1000;
    CoordGeodetic.UseGeoid = 0;

    auto now = QDateTime::currentDateTime().date();
    UserDate.Year = now.year();
    UserDate.Month = now.month();
    UserDate.Day = now.day();
    MAG_DateToYear(&UserDate, nullptr);

    /* Set EGM96 Geoid parameters END */
    MAG_GeodeticToSpherical(Ellip, CoordGeodetic, &CoordSpherical); /*Convert from geodetic to Spherical Equations: 17-18, WMM Technical report*/
    MAG_TimelyModifyMagneticModel(UserDate, MagneticModels[0], TimedMagneticModel); /* Time adjust the coefficients, Equation 19, WMM Technical report */
    MAG_Geomag(Ellip, CoordSpherical, CoordGeodetic, TimedMagneticModel, &GeoMagneticElements); /* Computes the geoMagnetic field elements and their time change*/
    MAG_CalculateGridVariation(CoordGeodetic, &GeoMagneticElements);
    MAG_WMMErrorCalc(GeoMagneticElements.H, &Errors);

    MAG_FreeMagneticModelMemory(TimedMagneticModel);
    MAG_FreeMagneticModelMemory(MagneticModels[0]);

    *result = GeoMagneticElements.Decl;

    return true;
}
