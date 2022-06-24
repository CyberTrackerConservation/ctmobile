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
#include "fxGPSParse.h"

// GGA
struct NMEA_GGA
{
    UINT TimeStamp;
    BYTE Hour, Minute, Second;
    DOUBLE Latitude, Longitude, Altitude;
    BYTE Quality;               
    BYTE SatellitesInUse;
    DOUBLE HDOP;
};

// GSA
struct NMEA_GSA
{
    UINT TimeStamp;
    BOOL AutoMode;                 
    BYTE FixMode;
    DOUBLE PDOP, HDOP, VDOP;
    UINT16 SatellitesInSolution[MAX_SATELLITES]; 
};

// GSV
struct NMEA_GSV
{
    UINT TimeStamp;
    BYTE SatellitesInView;      
    FXGPS_SATELLITE Satellites[MAX_SATELLITES];
};

// RMC
struct NMEA_RMC
{
    UINT TimeStamp;
	BYTE Hour, Minute, Second;
    BOOL Valid;  
	DOUBLE Latitude, Longitude;
	DOUBLE GroundSpeed;
	DOUBLE Course;
	BYTE Day, Month;
    UINT16 Year;
	DOUBLE MagneticVariation;
};

// ZDA
struct NMEA_ZDA
{
    UINT TimeStamp;
    BOOL Valid;  

    BYTE Hour, Minute, Second;
	BYTE Day, Month;
    UINT16 Year;

    BYTE LocalHour, LocalMinute;
};

// CfxNMEAParser
enum NMEA_PARSE_STATE {npsStart, npsCommand, npsData, npsChecksum1, npsChecksum2};
class CfxNMEAParser: public CfxGPSParser
{
private:
    NMEA_PARSE_STATE _state;         
    BYTE _expectedChecksum;          // Expected NMEA sentence checksum
    BYTE _receivedChecksum;          // Received NMEA sentence checksum (if exists)
    UINT16 _index;                   // Index used for command and data
    CHAR _command[8];                // NMEA command
    CHAR _data[256];                 // NMEA data

    BOOL _sirf;

    FXGPS_SATELLITE _satellites[MAX_SATELLITES];

    NMEA_GGA _gga;
    NMEA_GSA _gsa;
    NMEA_GSV _gsv;
    NMEA_RMC _rmc;
    NMEA_ZDA _zda;

    UINT _ggaCount, _gsaCount, _gsvCount, _rmcCount, _zdaCount;

    BOOL GetField(CHAR *pData, CHAR *pField, UINT FieldNumber, UINT FieldLength);
    VOID ProcessCommand(CHAR *pCommand, CHAR *pData);
    VOID ProcessNMEA(CHAR Data);
 public:
    CfxNMEAParser();

    VOID Reset();

    VOID ProcessGGA(CHAR *pData);
    VOID ProcessGSA(CHAR *pData);
    VOID ProcessGSV(CHAR *pData);
    VOID ProcessRMC(CHAR *pData);
    VOID ProcessZDA(CHAR *pData);

    VOID ParseBuffer(CHAR *pBuffer, UINT Length);

    BOOL GetPingStream(BYTE **ppBuffer, UINT *pLength);

    BOOL GetGPS(FXGPS *pGPS, UINT MaxAgeInSeconds);
};

BOOL IsValidNMEAStream(BYTE *pBuffer, UINT Length);

