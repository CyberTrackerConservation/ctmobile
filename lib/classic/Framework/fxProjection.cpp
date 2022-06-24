/*************************************************************************************************/
//                                                                                                *
//   Copyright (C) 2005              Software                                                     *
//                                                                                                *
//   Original author: Justin Steventon (justin@steventon.com)                                     *
//                                                                                                *
//   You may retrieve the latest version of this file via the              Software home page, *
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

#include "fxProjection.h"
#include "fxUtils.h"
#include "fxMath.h"

#define MapTileSize      (256)
#define MapEarthCirc     (6378137.0 * 2.0 * PI)
#define MapEarthRadius   (6378137.0)
#define MapEarthHalfCirc (6378137.0 * PI)

CfxProjectionVirtualEarth::CfxProjectionVirtualEarth(UINT ZoomLevel)
{
    _zoomLevel = ZoomLevel;
    _arc = MapEarthCirc / ((1 << ZoomLevel) * MapTileSize);
    _k = (256 * (1 << ZoomLevel));
}

VOID CfxProjectionVirtualEarth::Project(DOUBLE Longitude, DOUBLE Latitude, DOUBLE *pX, DOUBLE *pY)
{
    DOUBLE metersX = MapEarthRadius * D2R * Longitude;
    *pX = (metersX + MapEarthHalfCirc) / _arc;

    DOUBLE sinLat = sin(D2R * Latitude);
    DOUBLE metersY = (MapEarthRadius / 2.0) * log((1.0 + sinLat) / (1.0 - sinLat));
    *pY = _k - (MapEarthHalfCirc - metersY) / _arc;
}

VOID CfxProjectionVirtualEarth::Unproject(DOUBLE X, DOUBLE Y, DOUBLE *pLongitude, DOUBLE *pLatitude)
{
    DOUBLE metersX = (X * _arc) - MapEarthHalfCirc;
    *pLongitude = R2D * (metersX / MapEarthRadius);

    Y = _k - Y;
    DOUBLE metersY = MapEarthHalfCirc - (Y * _arc);
    DOUBLE a = exp(metersY * 2.0 / MapEarthRadius);
    *pLatitude = R2D * (asin((a - 1) / (a + 1)));
}

//*************************************************************************************************

struct ELLIPSOID 
{
    CHAR *Code;
    DOUBLE SemiMajor, Flattening;
};

static const ELLIPSOID Ellipsoids[] = 
{
    // "World Geodetic System 1984 (WGS 84)", "WGS_1984",
    { "WE", 6378137.000, 298.257223563 },
    //"World Geodetic System 1972 (WGS 72)", "WGS_1972",
    { "WD", 6378135.000, 298.260000000 },
    // "Airy (1930)", "Airy_1830",
    { "AA", 6377563.396, 299.324964600 },
    // "Australian National", "Australian",
    { "AN", 6378160.000, 298.250000000 },
    // "Bessel 1841", "Bessel_1841",
    { "BR", 6377397.155, 299.152812800 },
    // "Bessel 1841: Namibia", "Bessel_Namibia",
    { "BN", 6377483.865, 299.152812800 },
    // "Bessel Modified: Norway & Sweden", "Bessel_Modified",
    { "BM", 6377492.018, 299.152812800 },
    // "Clarke 1866", "Clarke_1866",
    { "CC", 6378206.400, 294.978698200 },
    // "Clarke 1880", "Clarke_1880",
    { "CD", 6378249.138, 293.466307656 },
    // "Everest: Brunei & E. Malasia (Sabah & Sarawak)", "Everest_Definition_1967",
    { "EB", 6377298.556, 300.8017000 },
    // "Everest: India 1830", "Everest_1830",
    { "EA", 6377276.345, 300.801700000 },
    // "Everest: India 1956", "Everest_Definition_1975",
    { "EC", 6377301.243, 300.801700000 },
    // "Everest: W. Malasia and Singapore 1948", "Everest_Modified",
    { "EE", 6377304.063, 300.801740000 },
    // "Everest: W. Malasia 1969", "Everest_Modified_1969",
    { "ED", 6377295.664, 300.801700000 },
    // "Geodetic Reference System 1980 (GRS 80)", "GRS_1980",
    { "RF", 6378137.000, 298.257222101 },
    // "Helmert 1906", "Helmert_1906",
    { "HE", 6378200.000, 298.300000000 },
    // "Hough 1960", "Hough_1960",
    { "HO", 6378270.000, 297.000000000 },
    // "Indonesian 1974", "Indonesian",
    { "ID", 6378160.000, 298.247000000 },
    // "International 1924", "International_1924",
    { "IN", 6378388.000, 297.000000000 },
    // "Krassovsky 1940", "Krasovsky_1940",
    { "KA", 6378245.000, 298.300000000 },
    // "Modified Airy", "Airy_Modified",
    { "AM", 6377340.189, 299.324964600 },
    // "Modified Fischer 1960 (South Asia)", "Fischer_Modified",
    { "FA", 6378155.000, 298.300000000 },
    // "South American 1969", "GRS_1967_Truncated",
    { "SA", 6378160.000, 298.250000000 }
};

struct DATUM
{
    CHAR *Code;
    CHAR *EllipsoidCode;
    DOUBLE K1, K2, K3, K4, K5, K6, K7, K8, K9, K10;
};

static const DATUM Datums[] =
{
    // "World Geodetic System 1984", "WGS_1984",
    { "WGS84", "WE", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { "WGE", "WE", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

    // "World Geodetic System 1972", "WGS_1972",
    { "WGD", "WD", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    // "ADINDAN, Mean", "?",
    { "ADI-M", "CD", -166, 5, -15, 5, 204, 3, -5, 31, 15, 55 },
    // "ADINDAN, Ethiopia", "?",
    { "ADI-A", "CD", -165, 3, -11, 3, 206, 3, -3, 25, 26, 50 },
    // "ADINDAN, Sudan", "?",
    { "ADI-B", "CD", -161, 3, -14, 5, 205, 3, -3, 31, 15, 45 },
    // "ADINDAN, Mali", "?",
    { "ADI-C", "CD", -123, 25, -20, 25, 220, 25, 3, 31, -20, 11 },
    // "ADINDAN, Senegal", "?",
    { "ADI-D", "CD", -128, 25, -18, 25, 224, 25, 5, 23, -24, -5 },
    // "ADINDAN, Burkina Faso", "?",
    { "ADI-E", "CD", -118, 25, -14, 25, 218, 25, 4, 22, -5, 8 },
    // "ADINDAN, Cameroon", "?",
    { "ADI-F", "CD", -134, 25, -2, 25, 210, 25, -4, 19, 3, 23 },
    // "AFGOOYE, Somalia", "?",
    { "AFG", "KA", -43, 25, -163, 25, 45, 25, -8, 19, 35, 60 },
    // "ANTIGUA ISLAND ASTRO 1943", "?",
    { "AIA", "CD", -270, 25, 13, 25, 62, 25, 16, 20, -65, -61 },
    // "AIN EL ABD 1970, Bahrain", "?",
    { "AIN-A", "IN", -150, 25, -250, 25, -1, 25, 24, 28, 49, 53 },
    // "AIN EL ABD 1970, Saudi Arabia", "?",
    { "AIN-B", "IN", -143, 10, -236, 10, 7, 10, 8, 38, 28, 62 },
    // "AMERICAN SAMOA 1962", "Samoa_1962",
    { "AMA", "CC", -115, 25, 118, 25, 426, 25, -19, -9, -174, -165 },
    // "ANNA 1 ASTRO 1965, Cocos Is.", "Anna_1_1965",
    { "ANO", "AN", -491, 25, -22, 25, 435, 25, -14, -10, 94, 99 },
    // "ARC 1950, Mean", "?",
    { "ARF-M", "CD", -143, 20, -90, 33, -294, 20, -36, 10, 4, 42 },
    // "ARC 1950, Botswana", "?",
    { "ARF-A", "CD", -138, 3, -105, 5, -289, 3, -33, -13, 13, 36 },
    // "ARC 1950, Lesotho", "?",
    { "ARF-B", "CD", -125, 3, -108, 3, -295, 8, -36, -23, 21, 35 },
    // "ARC 1950, Malawi", "?",
    { "ARF-C", "CD", -161, 9, -73, 24, -317, 8, -21, -3, 26, 42 },
    // "ARC 1950, Swaziland", "?",
    { "ARF-D", "CD", -134, 15, -105, 15, -295, 15, -33, -20, 25, 40 },
    // "ARC 1950, Zaire", "?",
    { "ARF-E", "CD", -169, 25, -19, 25, -278, 25, -21, 10, 4, 38 },
    // "ARC 1950, Zambia", "?",
    { "ARF-F", "CD", -147, 21, -74, 21, -283, 27, -24, -1, 15, 40 },
    // "ARC 1950, Zimbabwe", "?",
    { "ARF-G", "CD", -142, 5, -96, 8, -293, 11, -29, -9, 19, 39 },
    // "ARC 1950, Burundi", "?",
    { "ARF-H", "CD", -153, 20, -5, 20, -292, 20, -11, 4, 21, 37 },
    // "ARC 1960, Kenya & Tanzania", "?",
    { "ARS-M", "CD", -160, 20, -6, 20, -302, 20, -18, 8, 23, 47 },
    // "ARC 1960, Kenya", "?",
    { "ARS-A", "CD", -157, 4, -2, 3, -299, 3, -11, 8, 28, 47 },
    // "ARC 1960, Tanzania", "?",
    { "ARS-B", "CD", -175, 6, -23, 9, -303, 10, -18, 5, 23, 47 },
    // "ASCENSION ISLAND 1958", "Ascension_Island_1958",
    { "ASC", "IN", -205, 25, 107, 25, 53, 25, -9, -6, -16, -13 },
    // "MONTSERRAT ISLAND ASTRO 1958", "?",
    { "ASM", "CD", 174, 25, 359, 25, 365, 25, 15, 18, -64, -61 },
    // "ASTRO STATION 1952, Marcus Is.", "Astro_1952",
    { "ASQ", "IN", 124, 25, -234, 25, -25, 25, 22, 26, 152, 156 },
    // "ASTRO BEACON E 1945, Iwo Jima", "Beacon_E_1945",
    { "ATF", "IN", 145, 25, 75, 25, -272, 25, 22, 26, 140, 144 },
    // "AUSTRALIAN GEODETIC 1966", "Australian_1966",
    { "AUA", "AN", -133, 3, -48, 3, 148, 3, -46, -4, 109, 161 },
    // "AUSTRALIAN GEODETIC 1984", "Australian_1984",
    { "AUG", "AN", -134, 2, -48, 2, 149, 2, -46, -4, 109, 161 },
    // "DJAKARTA, INDONESIA", "?",
    { "BAT", "BR", -377, 3, 681, 3, -50, 3, -16, 11, 89, 146 },
    // "BISSAU, Guinea-Bissau", "Bissau",
    { "BID", "IN", -173, 25, 253, 25, 27, 25, 5, 19, -23, -7 },
    // "BERMUDA 1957, Bermuda Islands", "Bermuda_1957",
    { "BER", "CC", -73, 20, 213, 20, 296, 20, 31, 34, -66, -63 },
    // "BOGOTA OBSERVATORY, Columbia", "Bogota",
    { "BOO", "IN", 307, 6, 304, 5, -318, 6, -10, 16, -85, -61 },
    // "BUKIT RIMPAH, Banka & Belitung", "Bukit_Rimpah",
    { "BUR", "BR", -384, -1, 664, -1, -48, -1, -6, 0, 103, 110 },
    // "CAPE CANAVERAL, Fla & Bahamas", "Cape_Canaveral",
    { "CAC", "CC", -2, 3, 151, 3, 181, 3, 15, 38, -94, -12 },
    // "CAMPO INCHAUSPE 1969, Arg.", "Campo_Inchauspe",
    { "CAI", "IN", -148, 5, 136, 5, 90, 5, -58, -27, -72, -51 },
    // "CANTON ASTRO 1966, Phoenix Is.", "Canton_1966",
    { "CAO", "IN", 298, 15, -304, 15, -375, 15, -13, 3, -180, -165 },
    // "CAPE, South Africa", "?",
    { "CAP", "CD", -136, 3, -108, 6, -292, 6, -43, -15, 10, 40 },
    // "CAMP AREA ASTRO, Camp McMurdo", "Camp_Area",
    { "CAZ", "IN", -104, -1, -129, -1, 239, -1, -85, -70, 135, 180 },
    // "S-JTSK, Czech Republic", "S_JTSK",
    { "CCD", "BR", 589, 4, 76, 2, 480, 3, 43, 56, 6, 28 },
    // "CARTHAGE, Tunisia", "?",
    { "CGE", "CD", -263, 6, 6, 9, 431, 8, 24, 43, 2, 18 },
    // "CHATHAM ISLAND ASTRO 1971, NZ", "Chatham_Island_1971",
    { "CHI", "IN", 175, 15, -38, 15, 113, 15, -46, -42, -180, -174 },
    // "CHUA ASTRO, Paraguay", "Chua",
    { "CHU", "IN", -134, 6, 229, 9, -29, 5, -33, -14, -69, -49 },
    // "CORREGO ALEGRE, Brazil", "Corrego_Alegre",
    { "COA", "IN", -206, 5, 172, 3, -6, 5, -39, -2, -80, -29 },
    // "DABOLA, Guinea", "?",
    { "DAL", "CD", -83, 15, 37, 15, 124, 15, 1, 19, 12, 11 },
    // "DECEPTION ISLAND", "?",
    { "DID", "CD", 260, 20, 12, 20, -147, 20, -65, -62, 58, 62 },
    // "GUX 1 ASTRO, Guadalcanal Is.", "GUX_1",
    { "DOB", "IN", 252, 25, -209, 25, -751, 25, -12, -8, 158, 163 },
    // "EASTER ISLAND 1967", "Easter_Island_1967",
    { "EAS", "IN", 211, 25, 147, 25, 111, 25, -29, -26, -111, -108 },
    // "WAKE-ENIWETOK 1960", "Wake_Eniwetok_1960",
    { "ENW", "HO", 102, 3, 52, 3, -38, 3, 1, 16, 159, 175 },
    // "ESTONIA, 1937", "Estonia_1937",
    { "EST", "BR", 374, 2, 150, 3, 588, 3, 52, 65, 16, 34 },
    // "EUROPEAN 1950, Mean (3 Param)", "?",
    { "EUR-M", "IN", -87, 3, -98, 8, -121, 5, 30, 80, 5, 33 },
    // "EUROPEAN 1950, Western Europe", "?",
    { "EUR-A", "IN", -87, 3, -96, 3, -120, 3, 30, 78, -15, 25 },
    // "EUROPEAN 1950, Greece", "?",
    { "EUR-B", "IN", -84, 25, -95, 25, -130, 25, 30, 48, 14, 34 },
    // "EUROPEAN 1950, Norway & Finland", "?",
    { "EUR-C", "IN", -87, 3, -95, 5, -120, 3, 52, 80, -2, 38 },
    // "EUROPEAN 1950, Portugal & Spain", "?",
    { "EUR-D", "IN", -84, 5, -107, 6, -120, 3, 30, 49, -15, 10 },
    // "EUROPEAN 1950, Cyprus", "?",
    { "EUR-E", "IN", -104, 15, -101, 15, -140, 15, 33, 37, 31, 36 },
    // "EUROPEAN 1950, Egypt", "?",
    { "EUR-F", "IN", -130, 6, -117, 8, -151, 8, 16, 38, 19, 42 },
    // "EUROPEAN 1950, England, Channel", "?",
    { "EUR-G", "IN", -86, 3, -96, 3, -120, 3, 48, 62, -10, 3 },
    // "EUROPEAN 1950, Iran", "?",
    { "EUR-H", "IN", -117, 9, -132, 12, -164, 11, 19, 47, 37, 69 },
    // "EUROPEAN 1950, Sardinia(Italy)", "?",
    { "EUR-I", "IN", -97, 25, -103, 25, -120, 25, 37, 43, 6, 12 },
    // "EUROPEAN 1950, Sicily(Italy)", "?",
    { "EUR-J", "IN", -97, 20, -88, 20, -135, 20, 35, 40, 10, 17 },
    // "EUROPEAN 1950, England, Ireland", "?",
    { "EUR-K", "IN", -86, 3, -96, 3, -120, 3, 48, 62, -12, 3 },
    // "EUROPEAN 1950, Malta", "?",
    { "EUR-L", "IN", -107, 25, -88, 25, -149, 25, 34, 38, 12, 16 },
    // "EUROPEAN 1950, Iraq, Israel", "?",
    { "EUR-S", "IN", -103, -1, -106, -1, -141, -1, -38, -4, 36, 57 },
    // "EUROPEAN 1950, Tunisia", "?",
    { "EUR-T", "IN", -112, 25, -77, 25, -145, 25, 24, 43, 2, 18 },
    // "EUROPEAN 1979", "European_1979",
    { "EUS", "IN", -86, 3, -98, 3, -119, 3, 30, 80, -15, 24 },
    // "OMAN", "?",
    { "FAH", "CD", -346, 3, -1, 3, 224, 9, 10, 32, 46, 65 },
    // "OBSERVATORIO MET. 1939, Flores", "Observ_Meteorologico_1939",
    { "FLO", "IN", -425, 20, -169, 20, 81, 20, 38, 41, -33, -30 },
    // "FORT THOMAS 1955, Leeward Is.", "?",
    { "FOT", "CD", -7, 25, 215, 25, 225, 25, 16, 19, -64, -61 },
    // "GAN 1970, Rep. of Maldives", "Gan_1970",
    { "GAA", "IN", -133, 25, -321, 25, 50, 25, -2, 9, 71, 75 },
    // "GEODETIC DATUM 1949, NZ", "New_Zealand_1949",
    { "GEO", "IN", 84, 5, -22, 3, 209, 5, -48, -33, 165, 180 },
    // "DOS 1968, Gizo Island", "DOS_1968",
    { "GIZ", "IN", 230, 25, -199, 25, -752, 25, -10, -7, 155, 158 },
    // "GRACIOSA BASE SW 1948, Azores", "Graciosa_Base_SW_1948",
    { "GRA", "IN", -104, 3, 167, 3, -38, 3, 37, 41, -30, -26 },
    // "GUAM 1963", "Guam_1963",
    { "GUA", "CC", -100, 3, -248, 3, 259, 3, 12, 15, 143, 146 },
    // "GUNUNG SEGARA, Indonesia", "Gunung_Segara",
    { "GSE", "BR", -403, -1, 684, -1, 41, -1, -6, 9, 106, 121 },
    // "HERAT NORTH, Afghanistan", "Herat_North",
    { "HEN", "IN", -333, -1, -222, -1, 114, -1, 23, 44, 55, 81 },
    // "HERMANNSKOGEL, old Yugoslavia", "Hermannskogel",
    { "HER", "BR", 682, -1, -203, -1, 480, -1, 35, 52, 7, 29 },
    // "PROVISIONAL SOUTH CHILEAN 1963", "?",
    { "HIT", "IN", 16, 25, 196, 25, 93, 25, -64, -25, -83, -60 },
    // "HJORSEY 1955, Iceland", "Hjorsey_1955",
    { "HJO", "IN", -73, 3, 46, 3, -86, 6, 61, 69, -24, -11 },
    // "HONG KONG 1963", "Hong_Kong_1963",
    { "HKD", "IN", -156, 25, -271, 25, -189, 25, 21, 24, 112, 116 },
    // "HU-TZU-SHAN, Taiwan", "?",
    { "HTN", "IN", -637, 15, -549, 15, -203, 15, 20, 28, 117, 124 },
    // "BELLEVUE (IGN), Efate Is.", "Bellevue_IGN",
    { "IBE", "IN", -127, 20, -769, 20, 472, 20, -20, -16, 167, 171 },
    // "INDONESIAN 1974", "Indonesian_1974",
    { "IDN", "ID", -24, 25, -15, 25, 5, 25, -16, 11, 89, 146 },
    // "INDIAN, Bangladesh", "?",
    { "IND-B", "EA", 282, 10, 726, 8, 254, 12, 15, 33, 80, 100 },
    // "INDIAN, India & Nepal", "?",
    { "IND-I", "EC", 295, 12, 736, 10, 257, 15, 2, 44, 62, 105 },
    // "INDIAN, Pakistan", "?",
    { "IND-P", "EA", 283, -1, 682, -1, 231, -1, 17, 44, 55, 81 },
    // "INDIAN 1954, Thailand", "Indian_1954",
    { "INF-A", "EA", 217, 15, 823, 6, 299, 12, 0, 27, 91, 111 },
    // "INDIAN 1960, Vietnam 16N", "?",
    { "ING-A", "EA", 198, 25, 881, 25, 317, 25, 11, 23, 101, 115 },
    // "INDIAN 1960, Con Son Island", "?",
    { "ING-B", "EA", 182, 25, 915, 25, 344, 25, 6, 11, 104, 109 },
    // "INDIAN 1975, Thailand", "Indian_1975",
    { "INH-A", "EA", 209, 12, 818, 10, 290, 12, 0, 27, 91, 111 },
    // "IRELAND 1965", "?",
    { "IRL", "AM", 506, 3, -122, 3, 611, 3, 50, 57, -12, -4 },
    // "ISTS 061 ASTRO 1968, S Georgia", "ISTS_061_1968",
    { "ISG", "IN", -794, 25, 119, 25, -298, 25, -56, -52, -38, -34 },
    // "ISTS 073 ASTRO 1969, Diego Garc", "ISTS_073_1969",
    { "IST", "IN", 208, 25, -435, 25, -229, 25, -10, -4, 69, 75 },
    // "JOHNSTON ISLAND 1961", "Johnston_Island_1961",
    { "JOH", "IN", 189, 25, -79, 25, -202, 25, -46, -43, -76, -73 },
    // "KANDAWALA, Sri Lanka", "Kandawala",
    { "KAN", "EA", -97, 20, 787, 20, 86, 20, 4, 12, 77, 85 },
    // "KERGUELEN ISLAND 1949", "Kerguelen_Island_1949",
    { "KEG", "IN", 145, 25, -187, 25, 103, 25, -81, -74, 139, 180 },
    // "KERTAU 1948, W Malaysia & Sing.", "Kertau",
    { "KEA", "EE", -11, 10, 851, 8, 5, 6, -5, 12, 94, 112 },
    // "KUSAIE ASTRO 1951, Caroline Is.", "Kusaie_1951",
    { "KUS", "IN", 647, 25, 1777, 25, -1124, 25, -1, 12, 134, 167 },
    // "L.C. 5 ASTRO 1961, Cayman Brac", "LC5_1961",
    { "LCF", "CC", 42, 25, 124, 25, 147, 25, 18, 21, -81, -78 },
    // "LEIGON, Ghana", "?",
    { "LEH", "CD", -130, 2, 29, 3, 364, 2, -1, 17, -9, 7 },
    // "LIBERIA 1964", "?",
    { "LIB", "CD", -90, 15, 40, 15, 88, 15, -1, 14, -17, -1 },
    // "LUZON, Phillipines", "?",
    { "LUZ-A", "CC", -133, 8, -77, 11, -51, 9, 3, 23, 115, 128 },
    // "LUZON, Mindanao Island", "?",
    { "LUZ-B", "CC", -133, 25, -79, 25, -72, 25, 4, 12, 120, 128 },
    // "MASSAWA, Ethiopia", "Massawa",
    { "MAS", "BR", 639, 25, 405, 25, 60, 25, 7, 25, 37, 53 },
    // "MERCHICH, Morocco", "?",
    { "MER", "CD", 31, 5, 146, 3, 47, 3, 22, 42, -19, 5 },
    // "MIDWAY ASTRO 1961, Midway Is.", "Midway_1961",
    { "MID", "IN", 912, 25, -58, 25, 1227, 25, 25, 30, -180, -169 },
    // "MAHE 1971, Mahe Is.", "?",
    { "MIK", "CD", 41, 25, -220, 25, -134, 25, -6, -3, 54, 57 },
    // "MINNA, Cameroon", "?",
    { "MIN-A", "CD", -81, 25, -84, 25, 115, 25, -4, 19, 3, 23 },
    // "MINNA, Nigeria", "?",
    { "MIN-B", "CD", -92, 3, -93, 6, 122, 5, -1, 21, -4, 20 },
    // "ROME 1940, Sardinia", "?",
    { "MOD", "IN", -225, 25, -65, 25, 9, 25, 37, 43, 6, 12 },
    // "M""PORALOKO, Gabon", "?",
    { "MPO", "CD", -74, 25, -130, 25, 42, 25, -10, 8, 3, 20 },
    // "VITI LEVU 1916, Viti Levu Is.", "?",
    { "MVS", "CD", 51, 25, 391, 25, -36, 25, -20, -16, 176, 180 },
    // "NAHRWAN, Masirah Island (Oman)", "?",
    { "NAH-A", "CD", -247, 25, -148, 25, 369, 25, 19, 22, 57, 60 },
    // "NAHRWAN, United Arab Emirates", "?",
    { "NAH-B", "CD", -249, 25, -156, 25, 381, 25, 17, 32, 45, 62 },
    // "NAHRWAN, Saudi Arabia", "?",
    { "NAH-C", "CD", -243, 25, -192, 25, 477, 20, 8, 38, 28, 62 },
    // "NAPARIMA, Trinidad & Tobago", "?",
    { "NAP", "IN", -10, 15, 375, 15, 165, 15, 8, 13, -64, -59 },
    // "NORTH AMERICAN 1927, Eastern US", "?",
    { "NAS-A", "CC", -9, 5, 161, 5, 179, 8, 18, 55, -102, -60 },
    // "NORTH AMERICAN 1927, Western US", "?",
    { "NAS-B", "CC", -8, 5, 159, 3, 175, 3, 19, 55, -132, -87 },
    // "NORTH AMERICAN 1927, CONUS", "?",
    { "NAS-C", "CC", -8, 5, 160, 5, 176, 6, 15, 60, -135, -60 },
    // "NORTH AMERICAN 1927, Alaska", "?",
    { "NAS-D", "CC", -5, 5, 135, 9, 172, 5, 47, 78, -175, -157 },
    // "NORTH AMERICAN 1927, Canada", "?",
    { "NAS-E", "CC", -10, 15, 158, 11, 187, 6, 18, 55, -150, -50 },
    // "NORTH AMERICAN 1927, Alberta/BC", "?",
    { "NAS-F", "CC", -7, 8, 162, 8, 188, 6, 43, 65, -145, -105 },
    // "NORTH AMERICAN 1927, E. Canada", "?",
    { "NAS-G", "CC", -22, 6, 160, 6, 190, 3, 38, 68, -85, -45 },
    // "NORTH AMERICAN 1927, Man/Ont", "?",
    { "NAS-H", "CC", -9, 9, 157, 5, 184, 5, 36, 63, -108, -69 },
    // "NORTH AMERICAN 1927, NW Terr.", "?",
    { "NAS-I", "CC", 4, 5, 159, 5, 188, 3, 43, 90, -144, -55 },
    // "NORTH AMERICAN 1927, Yukon", "?",
    { "NAS-J", "CC", -7, 5, 139, 8, 181, 3, 53, 75, -147, -117 },
    // "NORTH AMERICAN 1927, Mexico", "?",
    { "NAS-L", "CC", -12, 8, 130, 6, 190, 6, 10, 38, -122, -80 },
    // "NORTH AMERICAN 1927, C. America", "?",
    { "NAS-N", "CC", 0, 8, 125, 3, 194, 5, 3, 25, -98, -77 },
    // "NORTH AMERICAN 1927, Canal Zone", "?",
    { "NAS-O", "CC", 0, 20, 125, 20, 201, 20, 3, 15, -86, -74 },
    // "NORTH AMERICAN 1927, Caribbean", "?",
    { "NAS-P", "CC", -3, 3, 142, 9, 183, 12, 8, 29, -87, -58 },
    // "NORTH AMERICAN 1927, Bahamas", "?",
    { "NAS-Q", "CC", -4, 5, 154, 3, 178, 5, 19, 29, -83, -71 },
    // "NORTH AMERICAN 1927, San Salvador", "?",
    { "NAS-R", "CC", 1, 25, 140, 25, 165, 25, 23, 26, -75, -74 },
    // "NORTH AMERICAN 1927, Cuba", "?",
    { "NAS-T", "CC", -9, 25, 152, 25, 178, 25, 18, 25, -87, -78 },
    // "NORTH AMERICAN 1927, Greenland", "?",
    { "NAS-U", "CC", 11, 25, 114, 25, 195, 25, 74, 81, 74, 56 },
    // "NORTH AMERICAN 1927, Aleutian E", "?",
    { "NAS-V", "CC", -2, 6, 152, 8, 149, 10, 50, 58, -180, -161 },
    // "NORTH AMERICAN 1927, Aleutian W", "?",
    { "NAS-W", "CC", 2, 10, 204, 10, 105, 10, 50, 58, 169, 180 },
    // "NORTH AMERICAN 1983, Alaska", "?",
    { "NAR-A", "RF", 0, 2, 0, 2, 0, 2, 48, 78, -175, -135 },
    // "NORTH AMERICAN 1983, Canada", "?",
    { "NAR-B", "RF", 0, 2, 0, 2, 0, 2, 36, 90, -150, -50 },
    // "NORTH AMERICAN 1983, CONUS", "?",
    { "NAR-C", "RF", 0, 2, 0, 2, 0, 2, 15, 60, -135, -60 },
    // "NORTH AMERICAN 1983, Mexico", "?",
    { "NAR-D", "RF", 0, 2, 0, 2, 0, 2, 28, 11, -122, -72 },
    // "NORTH AMERICAN 1983, Aleutian", "?",
    { "NAR-E", "RF", -2, 5, 0, 2, 4, 5, 51, 74, -180, 180 },
    // "NORTH AMERICAN 1983, Hawai""i", "?",
    { "NAR-H", "RF", 1, 2, 1, 2, -1, 2, 17, 24, -164, -153 },
    // "NORTH SAHARA 1959, Algeria", "?",
    { "NSD", "CD", -186, 25, -93, 25, 310, 25, 13, 43, -15, 11 },
    // "OLD EGYPTIAN 1907", "Egypt_1907",
    { "OEG", "HE", -130, 3, 110, 6, -13, 8, 16, 38, 19, 42 },
    // "ORDNANCE GB 1936, Mean (3 Para)", "?",
    { "OGB-M", "AA", 375, 10, -111, 10, 431, 15, 44, 66, -14, 7 },
    // "ORDNANCE GB 1936, England", "?",
    { "OGB-A", "AA", 371, 5, -112, 5, 434, 6, 44, 61, -12, 7 },
    // "ORDNANCE GB 1936, Eng., Wales", "?",
    { "OGB-B", "AA", 371, 10, -111, 10, 434, 15, 44, 61, -12, 7 },
    // "ORDNANCE GB 1936, Scotland", "?",
    { "OGB-C", "AA", 384, 10, -111, 10, 425, 10, 49, 66, -14, 4 },
    // "ORDNANCE GB 1936, Wales", "?",
    { "OGB-D", "AA", 370, 20, -108, 20, 434, 20, 46, 59, -11, 3 },
    // "OLD HAWAI""IAN (CC), Mean", "?",
    { "OHA-M", "CC", 61, 25, -285, 20, -181, 20, 17, 24, -164, -153 },
    // "OLD HAWAI""IAN (CC), Hawai""i", "?",
    { "OHA-A", "CC", 89, 25, -279, 25, -183, 25, 17, 22, -158, -153 },
    // "OLD HAWAI""IAN (CC), Kauai", "?",
    { "OHA-B", "CC", 45, 20, -290, 20, -172, 20, 20, 24, -161, -158 },
    // "OLD HAWAI""IAN (CC), Maui", "?",
    { "OHA-C", "CC", 65, 25, -290, 25, -190, 25, 19, 23, -158, -154 },
    // "OLD HAWAI""IAN (CC), Oahu", "?",
    { "OHA-D", "CC", 58, 10, -283, 6, -182, 6, 20, 23, -160, -156 },
    // "OLD HAWAI""IAN (IN), Mean", "?",
    { "OHI-M", "IN", 201, 25, -228, 20, -346, 20, 17, 24, -164, -153 },
    // "OLD HAWAI""IAN (IN), Hawai""i", "?",
    { "OHI-A", "IN", 229, 25, -222, 25, -348, 25, 17, 22, -158, -153 },
    // "OLD HAWAI""IAN (IN), Kauai", "?",
    { "OHI-B", "IN", 185, 20, -233, 20, -337, 20, 20, 24, -161, -158 },
    // "OLD HAWAI""IAN (IN), Maui", "?",
    { "OHI-C", "IN", 205, 25, -233, 25, -355, 25, 19, 23, -158, -154 },
    // "OLD HAWAI""IAN (IN), Oahu", "?",
    { "OHI-D", "IN", 198, 10, -226, 6, -347, 6, 20, 23, -160, -156 },
    // "AYABELLA LIGHTHOUSE, Bjibouti", "?",
    { "PHA", "CD", -79, 25, -129, 25, 145, 25, 5, 20, 36, 49 },
    // "PITCAIRN ASTRO 1967", "?",
    { "PIT", "IN", 185, 25, 165, 25, 42, 25, -27, -21, -134, -119 },
    // "PICO DE LAS NIEVES, Canary Is.", "?",
    { "PLN", "IN", -307, 25, -92, 25, 127, 25, 26, 31, -20, -12 },
    // "PORTO SANTO 1936, Madeira Is.", "International_1924",
    { "POS", "IN", -499, 25, -249, 25, 314, 25, 31, 35, -18, -15 },
    // "PROV. S AMERICAN 1956, Bolivia", "?",
    { "PRP-A", "IN", -270, 5, 188, 11, -388, 14, -28, -4, -75, -51 },
    // "PROV. S AMERICAN 1956, N Chile", "?",
    { "PRP-B", "IN", -270, 25, 183, 25, -390, 25, -45, -12, -83, -60 },
    // "PROV. S AMERICAN 1956, S Chile", "?",
    { "PRP-C", "IN", -305, 20, 243, 20, -442, 20, -64, -20, -83, -60 },
    // "PROV. S AMERICAN 1956, Colombia", "?",
    { "PRP-D", "IN", -282, 15, 169, 15, -371, 15, -10, 16, -85, -61 },
    // "PROV. S AMERICAN 1956, Ecuador", "?",
    { "PRP-E", "IN", -278, 3, 171, 5, -367, 3, -11, 7, -85, -70 },
    // "PROV. S AMERICAN 1956, Guyana", "?",
    { "PRP-F", "IN", -298, 6, 159, 14, -369, 5, -4, 14, -67, -51 },
    // "PROV. S AMERICAN 1956, Peru", "?",
    { "PRP-G", "IN", -279, 6, 175, 8, -379, 12, -24, 5, -87, -63 },
    // "PROV. S AMERICAN 1956, Venez", "?",
    { "PRP-H", "IN", -295, 9, 173, 14, -371, 15, -5, 18, -79, -54 },
    // "PROV. S AMERICAN 1956, Mean", "?",
    { "PRP-M", "IN", -288, 17, 175, 27, -376, 27, -64, 18, -87, -51 },
    // "POINT 58, Burkina Faso & Niger", "?",
    { "PTB", "CD", -106, 25, -129, 25, 165, 25, 0, 10, -15, 25 },
    // "POINT NOIRE 1948", "?",
    { "PTN", "CD", -148, 25, 51, 25, -291, 25, -11, 10, 5, 25 },
    // "PULKOVO 1942, Russia", "Pulkovo_1942",
    { "PUK", "KA", 28, -1, -130, -1, -95, -1, 36, 89, -180, 180 },
    // "PUERTO RICO & Virgin Is.", "Puerto_Rico",
    { "PUR", "CC", 11, 3, 72, 3, -101, 3, 16, 20, -69, -63 },
    // "QATAR NATIONAL", "Qatar",
    { "QAT", "IN", -128, 20, -283, 20, 22, 20, 19, 32, 45, 57 },
    // "QORNOQ, South Greenland", "Qornoq",
    { "QUO", "IN", 164, 25, 138, 25, -189, 32, 57, 85, -77, -7 },
    // "REUNION, Mascarene Is.", "Reunion",
    { "REU", "IN", 94, 25, -948, 25, -1262, 25, -27, -12, 47, 65 },
    // "SANTO (DOS) 1965", "Santo_DOS_1965",
    { "SAE", "IN", 170, 22, 42, 25, 84, 25, -17, -13, 160, 169 },
    // "SAO BRAZ, Santa Maria Is.", "Sao_Braz",
    { "SAO", "IN", -203, 25, 141, 25, 53, 25, 35, 39, -27, -23 },
    // "SAPPER HILL 1943, E Falkland Is", "Sapper_Hill_1943",
    { "SAP", "IN", -355, 1, 21, 1, 72, 1, -54, -50, -61, -56 },
    // "SOUTH AMERICAN 1969, Mean", "?",
    { "SAN-M", "SA", -57, 15, 1, 6, -41, 9, -65, -50, -90, -25 },
    // "SOUTH AMERICAN 1969, Argentina", "?",
    { "SAN-A", "SA", -62, 5, -1, 5, -37, 5, -62, -23, -76, -47 },
    // "SOUTH AMERICAN 1969, Bolivia", "?",
    { "SAN-B", "SA", -61, 15, 2, 15, -48, 15, -28, -4, -75, -51 },
    // "SOUTH AMERICAN 1969, Brazil", "?",
    { "SAN-C", "SA", -60, 3, -2, 5, -41, 5, -39, -2, -80, -29 },
    // "SOUTH AMERICAN 1969, Chile", "?",
    { "SAN-D", "SA", -75, 15, -1, 8, -44, 11, -64, -12, -83, -60 },
    // "SOUTH AMERICAN 1969, Colombia", "?",
    { "SAN-E", "SA", -44, 6, 6, 6, -36, 5, -10, 16, -85, -61 },
    // "SOUTH AMERICAN 1969, Ecuador", "?",
    { "SAN-F", "SA", -48, 3, 3, 3, -44, 3, -11, 7, -85, -70 },
    // "SOUTH AMERICAN 1969, Guyana", "?",
    { "SAN-G", "SA", -53, 9, 3, 5, -47, 5, -4, 14, -67, -51 },
    // "SOUTH AMERICAN 1969, Paraguay", "?",
    { "SAN-H", "SA", -61, 15, 2, 15, -33, 15, -33, -14, -69, -49 },
    // "SOUTH AMERICAN 1969, Peru", "?",
    { "SAN-I", "SA", -58, 5, 0, 5, -44, 5, -24, 5, -87, -63 },
    // "SOUTH AMERICAN 1969, Baltra", "?",
    { "SAN-J", "SA", -47, 25, 26, 25, -42, 25, -2, 1, -92, -89 },
    // "SOUTH AMERICAN 1969, Trinidad", "?",
    { "SAN-K", "SA", -45, 25, 12, 25, -33, 25, 4, 17, -68, -55 },
    // "SOUTH AMERICAN 1969, Venezuela", "?",
    { "SAN-L", "SA", -45, 3, 8, 6, -33, 3, -5, 18, -79, -54 },
    // "SCHWARZECK, Namibia", "Schwarzeck",
    { "SCK", "BN", 616, 20, 97, 20, -251, 20, -35, -11, 5, 31 },
    // "SELVAGEM GRANDE 1938, Salvage Is", "Selvagem_Grande_1938",
    { "SGM", "IN", -289, 25, -124, 25, 60, 25, 28, 32, -18, -14 },
    // "ASTRO DOS 71/4, St. Helena Is.", "DOS_71_4",
    { "SHB", "IN", -320, 25, 550, 25, -494, 25, -18, -14, -7, -4 },
    // "SOUTH ASIA, Singapore", "South_Asia_Singapore",
    { "SOA", "FA", 7, 25, -10, 25, -26, 25, 0, 3, 102, 106 },
    // "S-42 (PULKOVO 1942), Hungary", "?",
    { "SPK-A", "KA", 28, 2, -121, 2, -77, 2, 40, 54, 11, 29 },
    // "S-42 (PULKOVO 1942), Poland", "?",
    { "SPK-B", "KA", 23, 4, -124, 2, -82, 4, 43, 60, 8, 30 },
    // "S-41 (PK42) Former Czechoslov.", "?",
    { "SPK-C", "KA", 26, 3, -121, 3, -78, 2, 42, 57, 6, 28 },
    // "S-41 (PULKOVO 1942), Latvia", "?",
    { "SPK-D", "KA", 24, 2, -124, 2, -82, 2, 50, 64, 15, 34 },
    // "S-41 (PK 1942), Kazakhstan", "?",
    { "SPK-E", "KA", 15, 25, -130, 25, -84, 25, 35, 62, 41, 93 },
    // "S-41 (PULKOVO 1942), Albania", "?",
    { "SPK-F", "KA", 24, 3, -130, 3, -92, 3, 34, 48, 14, 26 },
    // "S-41 (PULKOVO 1942), Romania", "?",
    { "SPK-G", "KA", 28, 3, -121, 5, -77, 3, 38, 54, 15, 35 },
    // "SIERRA LEONE 1960", "?",
    { "SRL", "CD", -88, 15, 4, 15, 101, 15, 1, 16, -19, -4 },
    // "TANANARIVE OBSERVATORY 1925", "Tananarive_1925",
    { "TAN", "IN", -189, -1, -242, -1, -91, -1, -34, -8, 40, 53 },
    // "TRISTAN ASTRO 1968", "Tristan_1968",
    { "TDC", "IN", -632, 25, 438, 25, -609, 25, -39, -36, -14, -11 },
    // "TIMBALAI 1948, Brunei & E Malay", "Timbalai_1948",
    { "TIL", "EB", -679, 10, 669, 10, -48, 12, -5, 15, 101, 125 },
    // "TOKYO, Japan", "?",
    { "TOY-A", "BR", -148, 8, 507, 5, 685, 8, 19, 51, 119, 156 },
    // "TOKYO, South Korea", "?",
    { "TOY-B", "BR", -146, 8, 507, 5, 687, 8, 27, 45, 120, 139 },
    // "TOKYO, Okinawa", "?",
    { "TOY-C", "BR", -158, 20, 507, 5, 676, 20, 19, 31, 119, 134 },
    // "TOKYO, Mean", "?",
    { "TOY-M", "BR", -148, 20, 507, 5, 685, 20, 23, 53, 120, 155 },
    // "ASTRO TERN ISLAND (FRIG) 1961", "Tern_Island_1961",
    { "TRN", "IN", 114, 25, -116, 25, -333, 25, 22, 26, -166, -166 },
    // "VOIROL 1874, Algeria", "?",
    { "VOI", "CD", -73, -1, -247, -1, 227, -1, 13, 43, -15, 11 },
    // "VOIROL 1960, Algeria", "?",
    { "VOR", "CD", -123, 25, -206, 25, 219, 25, 13, 43, -15, 11 },
    // "WAKE ISLAND ASTRO 1952", "Wake_Island_1952",
    { "WAK", "IN", 276, 25, -57, 25, 149, 25, 17, 21, -176, -171 },
    // "YACARE, Uruguay", "Yacare",
    { "YAC", "IN", -155, -1, 171, -1, 37, -1, -40, -25, -65, -47 },
    // "ZANDERIJ, Suriname", "Zanderij",
    { "ZAN", "IN", -265, 5, 120, 5, -358, 8, -10, 20, -76, -47 },
    // "EUROPEAN 1950, Mean (7 Param)", "European_1950",
    { "EUR-7", "IN", -102, -102, -129, 0.413, -0.184, 0.385, 0.0000024664, 0, 0, 0 },
    // "ORDNANCE GB 1936, Mean (7 Param)", "OSGB_1936",
    { "OGB-7", "AA", 446, -99, 544, -0.945, -0.261, 0.435, -0.0000208927, 0, 0, 0 },
    // "Switzerland CH-1903", "CH1903",
    { "CH1903", "BR", 674.374, 15.056, 405.346, 0, 0, 0, 0, 0, 0, 0 }
};

CfxProjectionUTM::CfxProjectionUTM(INT Zone, CHAR *pDatum)
{
    UINT i, j;
    DOUBLE tn, tn2, tn3, tn4, tn5;

    _b = _es = _ebs = _ap = _bp = _cp = _dp = _ep = 0;

    _zone = Zone;
    _a = Ellipsoids[0].SemiMajor;
    _f = 1 / Ellipsoids[0].Flattening;

    for (i = 0; i < ARRAYSIZE(Datums); i++)
    {
        if (stricmp(pDatum, Datums[i].Code) == 0)
        {
            for (j = 0; j < ARRAYSIZE(Ellipsoids); j++)
            {
                if (stricmp(Ellipsoids[j].Code, Datums[i].EllipsoidCode) == 0)
                {
                    _a = Ellipsoids[j].SemiMajor;
                    _f = 1 / Ellipsoids[j].Flattening;
                    break;
                }
            }
        }
    }

    _falseEasting = 500000.0;
    _falseNorthing = 0;
    _scale = 0.9996;

    if (_zone >= 31)
    {
        _centralMeridian = (6 * _zone - 183) * PI / 180.0;
    }
    else
    {
        _centralMeridian = (6 * _zone + 177) * PI / 180.0;
    }

    if (_centralMeridian > PI)
    {
        _centralMeridian -= (2 * PI);
    }

    _es = 2 * _f - (_f * _f);
    _ebs = (1 / (1 - _es)) - 1;
    _b = _a * (1 - _f);

    tn = (_a - _b) / (_a + _b);
    tn2 = tn * tn;
    tn3 = tn2 * tn;
    tn4 = tn3 * tn;
    tn5 = tn4 * tn;

    _ap = _a * (1.e0 - tn + 5.e0 * (tn2 - tn3) / 4.e0 + 81.e0 * (tn4 - tn5) / 64.e0);
    _bp = 3.e0 * _a * (tn - tn2 + 7.e0 * (tn3 - tn4) / 8.e0 + 55.e0 * tn5 / 64.e0 ) / 2.e0;
    _cp = 15.e0 * _a * (tn2 - tn3 + 3.e0 * (tn4 - tn5) / 4.e0) / 16.0;
    _dp = 35.e0 * _a * (tn3 - tn4 + 11.e0 * tn5 / 16.e0) / 48.e0;
    _ep = 315.e0 * _a * (tn4 - tn5) / 512.e0;
}

#define SPHTMD(Latitude) ((DOUBLE) (_ap * Latitude \
      - _bp * sin(2.e0 * Latitude) + _cp * sin(4.e0 * Latitude) \
      - _dp * sin(6.e0 * Latitude) + _ep * sin(8.e0 * Latitude) ) )

#define SPHSN(Latitude) ((DOUBLE) (_a / sqrt( 1.e0 - _es * pow(sin(Latitude), 2))))
#define SPHSR(Latitude) ((DOUBLE) (_a * (1.e0 - _es) / pow(DENOM(Latitude), 3)))
#define DENOM(Latitude) ((DOUBLE) (sqrt(1.e0 - _es * pow(sin(Latitude), 2))))

VOID CfxProjectionUTM::Project(DOUBLE Longitude, DOUBLE Latitude, DOUBLE *pX, DOUBLE *pY)
{
    _falseNorthing = (Latitude < 0) ? 10000000 : 0;
    Latitude *= D2R;
    Longitude *= D2R;

    if (Longitude > PI)
    {
        Longitude -= (2 * PI);
    }

    DOUBLE c, c2, c3, c5, c7;
    DOUBLE dlam, eta, eta2, eta3, eta4;
    DOUBLE s, sn;
    DOUBLE t, tan2, tan3, tan4, tan5, tan6;
    DOUBLE t1, t2, t3, t4, t5, t6, t7, t8, t9;
    DOUBLE tmd, tmdo;

    dlam = Longitude - _centralMeridian;

    if (dlam > PI)
    {
        dlam -= (2 * PI);
    }
    else if (dlam < -PI)
    {
        dlam += (2 * PI);
    }

    if (fabs(dlam) < 2.e-10)
    {
        dlam = 0.0;
    }

    s = sin(Latitude);
    c = cos(Latitude);
    c2 = c * c;
    c3 = c2 * c;
    c5 = c3 * c2;
    c7 = c5 * c2;
    t = s / c; //tan(Latitude);
    tan2 = t * t;
    tan3 = tan2 * t;
    tan4 = tan3 * t;
    tan5 = tan4 * t;
    tan6 = tan5 * t;
    eta = _ebs * c2;
    eta2 = eta * eta;
    eta3 = eta2 * eta;
    eta4 = eta3 * eta;

    // Radius of curvature in prime vertical
    sn = SPHSN(Latitude);

    // True Meridianal Distances
    tmd = SPHTMD(Latitude);

    //  Origin
    tmdo = SPHTMD(0);

    // Northing
    t1 = (tmd - tmdo) * _scale;
    t2 = sn * s * c * _scale / 2.e0;
    t3 = sn * s * c3 * _scale * (5.e0 - tan2 + 9.e0 * eta + 4.e0 * eta2) / 24.e0; 

    t4 = sn * s * c5 * _scale * (61.e0 - 58.e0 * tan2
        + tan4 + 270.e0 * eta - 330.e0 * tan2 * eta + 445.e0 * eta2
        + 324.e0 * eta3 -680.e0 * tan2 * eta2 + 88.e0 * eta4 
        -600.e0 * tan2 * eta3 - 192.e0 * tan2 * eta4) / 720.e0;

    t5 = sn * s * c7 * _scale * (1385.e0 - 3111.e0 * tan2 + 543.e0 * tan4 - tan6) / 40320.e0;

    *pY = _falseNorthing + t1 + pow(dlam, 2.e0) * t2
        + pow(dlam,4.e0) * t3 + pow(dlam, 6.e0) * t4
        + pow(dlam,8.e0) * t5; 

    // Easting
    t6 = sn * c * _scale;
    t7 = sn * c3 * _scale * (1.e0 - tan2 + eta ) / 6.e0;
    t8 = sn * c5 * _scale * (5.e0 - 18.e0 * tan2 + tan4
        + 14.e0 * eta - 58.e0 * tan2 * eta + 13.e0 * eta2 + 4.e0 * eta3 
        - 64.e0 * tan2 * eta2 - 24.e0 * tan2 * eta3 )/ 120.e0;
    t9 = sn * c7 * _scale * ( 61.e0 - 479.e0 * tan2 + 179.e0 * tan4 - tan6 ) / 5040.e0;

    *pX = _falseEasting + dlam * t6 + pow(dlam,3.e0) * t7 
        + pow(dlam,5.e0) * t8 + pow(dlam,7.e0) * t9;
}

VOID CfxProjectionUTM::Unproject(DOUBLE X, DOUBLE Y, DOUBLE *pLongitude, DOUBLE *pLatitude)
{
    DOUBLE c;
    DOUBLE de;      // Delta easting - Difference in Easting (Easting-Fe)    
    DOUBLE dlam;    // Delta longitude - Difference in Longitude       
    DOUBLE eta;     // constant - TranMerc_ebs *c *c                   
    DOUBLE eta2;
    DOUBLE eta3;
    DOUBLE eta4;
    DOUBLE ftphi;   // Footpoint latitude                              
    INT  i;       // Loop iterator                   
    DOUBLE s;       // Sine of latitude                        
    DOUBLE sn;      // Radius of curvature in the prime vertical       
    DOUBLE sr;      // Radius of curvature in the meridian             
    DOUBLE t;       // Tangent of latitude                             
    DOUBLE tan2;
    DOUBLE tan4;
    DOUBLE t10;     // Term in coordinate conversion formula - GP to Y 
    DOUBLE t11;     // Term in coordinate conversion formula - GP to Y 
    DOUBLE t12;     // Term in coordinate conversion formula - GP to Y 
    DOUBLE t13;     // Term in coordinate conversion formula - GP to Y 
    DOUBLE t14;     // Term in coordinate conversion formula - GP to Y 
    DOUBLE t15;     // Term in coordinate conversion formula - GP to Y 
    DOUBLE t16;     // Term in coordinate conversion formula - GP to Y 
    DOUBLE t17;     // Term in coordinate conversion formula - GP to Y 
    DOUBLE tmd;     // True Meridional distance                        
    DOUBLE tmdo;    // True Meridional distance for latitude of origin 
    DOUBLE Latitude, Longitude;

    // True Meridional Distances for latitude of origin 
    tmdo = SPHTMD(0);

    //  Origin  
    tmd = tmdo +  (Y - _falseNorthing) / _scale; 

    // First Estimate 
    sr = SPHSR(0.e0);
    ftphi = tmd / sr;

    for (i = 0; i < 5 ; i++)
    {
        t10 = SPHTMD(ftphi);
        sr = SPHSR(ftphi);
        ftphi = ftphi + (tmd - t10) / sr;
    }

    // Radius of Curvature in the meridian 
    sr = SPHSR(ftphi);

    // Radius of Curvature in the meridian 
    sn = SPHSN(ftphi);

    // Sine Cosine terms 
    s = sin(ftphi);
    c = cos(ftphi);

    // Tangent Value  
    t = tan(ftphi);
    tan2 = t * t;
    tan4 = tan2 * tan2;
    eta = _ebs * pow(c,2);
    eta2 = eta * eta;
    eta3 = eta2 * eta;
    eta4 = eta3 * eta;
    de = X - _falseEasting;
    if (fabs(de) < 0.0001)
    {
        de = 0.0;
    }

    // Latitude 
    t10 = t / (2.e0 * sr * sn * pow(_scale, 2));
    t11 = t * (5.e0  + 3.e0 * tan2 + eta - 4.e0 * pow(eta, 2)
               - 9.e0 * tan2 * eta) / (24.e0 * sr * pow(sn, 3) * pow(_scale, 4));
    t12 = t * (61.e0 + 90.e0 * tan2 + 46.e0 * eta + 45.E0 * tan4
               - 252.e0 * tan2 * eta  - 3.e0 * eta2 + 100.e0 
               * eta3 - 66.e0 * tan2 * eta2 - 90.e0 * tan4
               * eta + 88.e0 * eta4 + 225.e0 * tan4 * eta2
               + 84.e0 * tan2* eta3 - 192.e0 * tan2 * eta4)
          / ( 720.e0 * sr * pow(sn,5) * pow(_scale, 6) );
    t13 = t * ( 1385.e0 + 3633.e0 * tan2 + 4095.e0 * tan4 + 1575.e0 
                * pow(t, 6)) / (40320.e0 * sr * pow(sn, 7) * pow(_scale, 8));
    Latitude = ftphi - pow(de,2) * t10 + pow(de,4) * t11 - pow(de,6) * t12 
                + pow(de,8) * t13;

    t14 = 1.e0 / (sn * c * _scale);

    t15 = (1.e0 + 2.e0 * tan2 + eta) / (6.e0 * pow(sn,3) * c * pow(_scale,3));

    t16 = (5.e0 + 6.e0 * eta + 28.e0 * tan2 - 3.e0 * eta2
           + 8.e0 * tan2 * eta + 24.e0 * tan4 - 4.e0 
           * eta3 + 4.e0 * tan2 * eta2 + 24.e0 
           * tan2 * eta3) / (120.e0 * pow(sn, 5) * c  * pow(_scale, 5));

    t17 = (61.e0 +  662.e0 * tan2 + 1320.e0 * tan4 + 720.e0 
           * pow(t,6)) / (5040.e0 * pow(sn,7) * c * pow(_scale, 7));

    // Difference in Longitude 
    dlam = de * t14 - pow(de,3) * t15 + pow(de,5) * t16 - pow(de,7) * t17;

    // Longitude 
    Longitude = _centralMeridian + dlam;
    while (Latitude > (90.0 * PI / 180.0))
    {
        Latitude = PI - Latitude;
        Longitude += PI;
        if (Longitude > PI)
        {
            Longitude -= (2 * PI);
        }
    }

    while (Latitude < (-90.0 * PI / 180.0))
    {
        Latitude = - (Latitude + PI);
        Longitude += PI;
        if (Longitude > PI)
        {
            Longitude -= (2 * PI);
        }
    }

    if (Longitude > (2 * PI))
    {
        Longitude -= (2 * PI);
    }

    if (Longitude < -PI)
    {
        Longitude += (2 * PI);
    }

    *pLatitude = Latitude * R2D;
    *pLongitude = Longitude * R2D;
}

FXPROJECTION g_DefaultProjection = PR_DEGREES_MINUTES_SECONDS;
BYTE g_DefaultUTMZone = 0;

VOID SetProjectionParams(FXPROJECTION Projection, BYTE UTMZone)
{
    g_DefaultProjection = Projection;
    g_DefaultUTMZone = UTMZone;
}

VOID GetProjectedParams(CHAR **ppLon, CHAR **ppLat, BOOL *pInvert)
{
    CHAR *pLon = NULL;
    CHAR *pLat = NULL;
    BOOL invert = FALSE;

    switch (g_DefaultProjection) {
    case PR_UTM:
        pLon = "Easting";
        pLat = "Northing";
        invert = TRUE;
        break;

    default:
        pLon = "Longitude";
        pLat = "Latitude";
        invert = FALSE;
        break;
    }

    if (ppLon)
    {
        *ppLon = pLon;
    }

    if (ppLat)
    {
        *ppLat = pLat;
    }

    if (pInvert)
    {
        *pInvert = invert;
    }
}

VOID GetProjectedStrings(DOUBLE Lon, DOUBLE Lat, CHAR *pLon, CHAR *pLat)
{
    FXPROJECTION projection = g_DefaultProjection;
    CHAR captionF[16];

    if ((projection == PR_UTM) && (g_DefaultUTMZone == 0))
    {
        projection = PR_DEGREES_MINUTES_SECONDS;
    }
    
    *pLat = '\0';
    *pLon = '\0';

    switch (projection) {
    case PR_DEGREES_MINUTES_SECONDS:
        {
            INT16 deg, min;
            DOUBLE fra, sec;

            fra = fabs(Lat);
            deg = (INT16)fra;
            fra = fra - deg;
            min = (INT16) (fra * 60);
            sec = (fra * 60 - min) * 60;
            PrintF(captionF, sec, sizeof(captionF)-1, 3);
            sprintf(pLat, "%02d" DEGREE_SYMBOL_UTF8 "%02d'%s\" %s", deg, min, captionF, Lat < 0 ? "S" : "N");

            fra = fabs(Lon);
            deg = (INT16)fra;
            fra = fra - deg;
            min = (INT16) (fra * 60);
            sec = (fra * 60 - min) * 60;
            PrintF(captionF, sec, sizeof(captionF)-1, 3);
            sprintf(pLon, "%03d" DEGREE_SYMBOL_UTF8 "%02d'%s\" %s", deg, min, captionF, Lon < 0 ? "W" : "E");
        }
        break;

    case PR_DECIMAL_DEGREES:
        PrintF(pLon, Lon, 31, 6);
        PrintF(pLat, Lat, 31, 6);
        break;

    case PR_UTM:
        if (g_DefaultUTMZone != 0)
        {
            DOUBLE x = 0, y = 0;
            CfxProjectionUTM projection(g_DefaultUTMZone, "WGE");
            if ((Lon != 0.0) || (Lat != 0.0))
            {
                projection.Project(Lon, Lat, &x, &y);
            }

            PrintF(captionF, x, 31, 2);
            sprintf(pLon, "%s E", captionF);
            PrintF(captionF, y, 31, 2);
            sprintf(pLat, "%s N", captionF);
        }
        break;

    default:
        FX_ASSERT(FALSE);
    }

}
