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
#include "fxRangeFinderParse.h"

enum LA1KD_ID_PACKET {ladPolar = 0, ladCartesian, ladGolfTournament, ladBoth};

const UINT LAFLAG_RANGE                  = 0x00000001;
const UINT LAFLAG_SPEED                  = 0x00000002;
const UINT LAFLAG_POLAR_BEARING          = 0x00000004;
const UINT LAFLAG_POLAR_INCLINATION      = 0x00000008;
const UINT LAFLAG_POLAR_ROLL             = 0x00000010;
const UINT LAFLAG_CARTESIAN_X            = 0x00000020;
const UINT LAFLAG_CARTESIAN_Y            = 0x00000040;
const UINT LAFLAG_CARTESIAN_Z            = 0x00000080;
const UINT LAFLAG_CARTESIAN_HORIZONTAL   = 0x00000100;
const UINT LAFLAG_CARTESIAN_VERTICAL     = 0x00000200;
const UINT LAFLAG_GOLFTOURNAMENT_BEARING = 0x00000400;
const UINT LAFLAG_GOLFTOURNAMENT_VRANGE  = 0x00000800;
const UINT LAFLAG_GOLFTOURNAMENT_HRANGE  = 0x00001000;

const UINT TPFLAG_HORIZONTAL_DISTANCE    = 0x00010000;
const UINT TPFLAG_AZIMUTH                = 0x00020000;
const UINT TPFLAG_INCLINATION            = 0x00040000;
const UINT TPFLAG_SLOPE_DISTANCE         = 0x00080000;
const UINT TPFLAG_CALCULATED_HEIGHT      = 0x00100000;

struct LASERATLANTA_RANGE
{
    DOUBLE Range;
    CHAR Units;
};

struct LASERATLANTA_SPEED
{
    DOUBLE Speed;
    CHAR Units;
    CHAR Mode;
};

struct LASERATLANTA_POLAR
{
    DOUBLE Bearing;         // Bearing in degrees
    CHAR BearingMode;       // M=Magnetic, T=True, E=Encoder
    DOUBLE Inclination;     // Inclination in degrees
    CHAR InclinationSource; // 0 = tilt sensor, E=Encoder
    DOUBLE Roll;            // Roll in degrees
};

struct LASERATLANTA_CARTESIAN
{
    DOUBLE XAxisRange;  // East
    CHAR XUnits;        // F=feet, M=meters, Y=yards
    DOUBLE YAxisRange;  // North
    CHAR YUnits;        // F=feet, M=meters, Y=yards
    DOUBLE ZAxisRange;  // Up
    CHAR ZUnits;        // F=feet, M=meters, Y=yards
    DOUBLE HRange;      // Up
    CHAR HUnits;        // F=feet, M=meters, Y=yards
    DOUBLE VRange;      // Up
    CHAR VUnits;        // F=feet, M=meters, Y=yards
};

struct LASERATLANTA_GOLFTOURNAMENT
{
    DOUBLE Bearing;     // Bearing in degrees
    CHAR BearingMode;   // M=Magnetic, T=True, E=Encoder
    DOUBLE VMissingLineModeRange;
    CHAR VUnits;        // F=feet, M=meters, Y=yards
    CHAR VModeId[2];    // VModeId
    DOUBLE HMissingLineModeRange;
    CHAR HUnits;        // F=feet, M=meters, Y=yards
    CHAR HModeId[2];    // HModeId
};

// LA1KA
struct LASERATLANTA_LA1KA
{
    UINT TimeStamp;
    UINT Flags;

    LASERATLANTA_RANGE Range;
    LASERATLANTA_POLAR Polar;
};

// LA1KC
struct LASERATLANTA_LA1KC
{
    UINT TimeStamp;
    UINT Flags;

    LASERATLANTA_RANGE Range;
};

// LA1KD
struct LASERATLANTA_LA1KD
{
    UINT TimeStamp;
    UINT Flags;           // Flag saying which are valid

    LASERATLANTA_RANGE Range;
    LASERATLANTA_SPEED Speed;

    LA1KD_ID_PACKET PositionId;
    LASERATLANTA_POLAR Polar;
    LASERATLANTA_CARTESIAN Cartesian;
    LASERATLANTA_GOLFTOURNAMENT GolfTournament;
};

struct TRUPULSE_PLTITHV
{
	UINT TimeStamp;
	UINT Flags;

    DOUBLE HDvalue;
	CHAR HDunits;
	DOUBLE AZvalue;
	CHAR AZunits;
	DOUBLE INCvalue;
	CHAR INCunits;
	DOUBLE SDvalue;
	CHAR SDunits;
};

struct TRUPULSE_PLTITHT
{
	UINT TimeStamp;
	UINT Flags;

    DOUBLE HTvalue;
	CHAR HTunits;
};

// CfxLaserAtlantaParser
enum LASERATLANTA_PARSE_STATE {lpsStart, lpsCommand, lpsData, lpsChecksum1, lpsChecksum2};
class CfxLaserAtlantaParser: public CfxRangeFinderParser
{
private:
    LASERATLANTA_PARSE_STATE _state;         
    BYTE _expectedChecksum;          // Expected NMEA sentence checksum
    BYTE _receivedChecksum;          // Received NMEA sentence checksum (if exists)
    UINT16 _index;                   // Index used for command and data
    CHAR _command[8];                // NMEA command
    CHAR _data[256];                 // NMEA data

    LASERATLANTA_LA1KA _la1ka;
    LASERATLANTA_LA1KC _la1kc;
    LASERATLANTA_LA1KD _la1kd;
	TRUPULSE_PLTITHV _pltithv;
	TRUPULSE_PLTITHT _pltitht;

    UINT _la1kaCount, _la1kcCount, _la1kdCount, _pltithvCount, _pltithtCount;

    BOOL GetField(CHAR *pData, CHAR *pField, UINT FieldNumber, UINT FieldLength);
    VOID ProcessCommand(CHAR *pCommand, CHAR *pData);
    VOID ProcessLaserAtlanta(CHAR Data);

    VOID ProcessLA1KA(CHAR *pData, LASERATLANTA_LA1KA *pla1ka);
    VOID ParsePolar(CHAR *pData, UINT BaseIndex, LASERATLANTA_POLAR *pPolar, UINT *pFlags); 
    VOID ParseCartesian(CHAR *pData, UINT BaseIndex, LASERATLANTA_CARTESIAN *pCartesian, UINT *pFlags);
    VOID ParseGolfTournament(CHAR *pData, UINT BaseIndex, LASERATLANTA_GOLFTOURNAMENT *pGolfTournament, UINT *pFlags);
    VOID ProcessLA1KC(CHAR *pData, LASERATLANTA_LA1KC *pla1kc);
    VOID ProcessLA1KD(CHAR *pData, LASERATLANTA_LA1KD *pla1kd);
	VOID ProcessPLTITHV(CHAR *pData, TRUPULSE_PLTITHV *ppltithv);
	VOID ProcessPLTITHT(CHAR *pData, TRUPULSE_PLTITHT *ppltitht);
 public:
    CfxLaserAtlantaParser();
    VOID Reset();
    VOID ParseBuffer(CHAR *pBuffer, UINT Length);
    BOOL GetRange(FXRANGE *pRange, UINT MaxAgeInSeconds);
    BOOL GetPingStream(BYTE **ppBuffer, UINT *pLength);
};

BOOL IsValidLaserAtlantaStream(BYTE *pBuffer, UINT Length);

