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

#define MAX_REPORT_SIZE  256
#define MAX_CHANNEL      12
#define DLE              0x10
#define ETX              0x03

enum TSIP_PARSE_STATE {tpsEmpty, tpsFull, tpsData, tpsDLE1, tpsDLE2};

// TSIP_PACKET
struct TSIP_PACKET 
{
    INT16 Counter, Length;
    TSIP_PARSE_STATE State;
    BYTE Code;
    BYTE Data[MAX_REPORT_SIZE];
};

// TSIP_REPORT_8F20 - gives us lat, lon, alt
struct TSIP_REPORT_8F20
{
	UINT TimeStamp;
    BYTE SubpacketId;
	BYTE FixFlags;
	DOUBLE Latitude, Longitude, Altitude;
	DOUBLE Velocity[3];
	DOUBLE TimeOfFix;
	UINT16 WeekNumber;
	UINT16 DatumIndex;
	BYTE NSVs;
	BYTE SV_PRN[MAX_CHANNEL];
	UINT16 SV_IODC[MAX_CHANNEL];
};

// TSIP_REPORT_6D - gives us PDOP
struct TSIP_REPORT_6D
{
	UINT TimeStamp;
	BYTE Mode;
	BYTE NSVs;
	BYTE SV_PRN[16];
	DOUBLE PDOP, HDOP, VDOP, TDOP;
};

// TSIP_REPORT_5C - gives us satellite status
struct TSIP_REPORT_5C
{
	UINT TimeStamp;

    BYTE PRN;
	BYTE Slot;
	BYTE Channel;
	BYTE AcquisitionFlag;
	BYTE EphemerisFlag;
	FLOAT SignalLevel;
	FLOAT TimeOfLastMeasurement;
	FLOAT Elevation;
	FLOAT Azimuth;
	BYTE OldMeasurementFlag;
	BYTE IntegerMillisecondFlag;
	BYTE BadDataFlag;
	BYTE DataCollectFlag;

    FXGPS_SATELLITE Satellites[MAX_SATELLITES];
};

//
// CfxTSIPParser
//
class CfxTSIPParser: public CfxGPSParser
{
private:
    TSIP_PACKET _packet;
    INT16 _lastMessage;

    TSIP_REPORT_8F20 _report8F20;
    TSIP_REPORT_6D _report6D;
    TSIP_REPORT_5C _report5C;

    UINT _8F20Count, _6DCount, _5CCount;
    FXGPS_SATELLITE _satellites[MAX_SATELLITES];

    VOID ParseReport(TSIP_PACKET *pReport);

    BOOL Parse5C(TSIP_PACKET *pReport);
    BOOL Parse6D(TSIP_PACKET *pReport);
    BOOL Parse8F20(TSIP_PACKET *pReport);
public:
    CfxTSIPParser();

    VOID Reset();

    VOID ParseBuffer(CHAR *pBuffer, UINT Length);
    BOOL GetGPS(FXGPS *pGPS, UINT MaxAgeInSeconds);
    BOOL GetPingStream(BYTE **ppBuffer, UINT *pLength);

    static VOID ParseByte(TSIP_PACKET *pPacket, BYTE InByte);
};

BOOL IsValidTSIPStream(BYTE *pBuffer, UINT Length);
