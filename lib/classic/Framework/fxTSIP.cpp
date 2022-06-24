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

#include "fxTSIP.h"
#include "fxUtils.h"

#ifdef BYTESWAP
INT16 GetInt16(BYTE *pData)
{
    INT16 value;
    Swap16((BYTE *)&value, (BYTE *)pData);
    return value;
}

INT GetInt32(BYTE *pData)
{
    INT value;
    Swap32((BYTE *)&value, (BYTE *)pData);
    return value;
}

UINT GetUInt32(BYTE *pData)
{
    UINT value;
    Swap32((BYTE *)&value, (BYTE *)pData);
    return value;
}

FLOAT GetFloat(BYTE *pData)
{
    FLOAT value;
    Swap32((BYTE *)&value, (BYTE *)pData);
    return value;
}
#else
INT16 GetInt16(BYTE *pData)
{
    INT16 value;
    Get16((BYTE *)&value, (BYTE *)pData);
    return value;
}

INT GetInt32(BYTE *pData)
{
    INT value;
    Get32((BYTE *)&value, (BYTE *)pData);
    return value;
}

UINT GetUInt32(BYTE *pData)
{
    UINT value;
    Get32((BYTE *)&value, (BYTE *)pData);
    return value;
}

FLOAT GetFloat(BYTE *pData)
{
    FLOAT value;
    Get32((BYTE *)&value, (BYTE *)pData);
    return value;
}
#endif

//
// IsValidTSIPStream
//

BOOL IsValidTSIPStream(BYTE *pBuffer, UINT Length)
{
    TSIP_PACKET packet = {0};
   
    for (UINT i=0; i<Length; i++)
    {
        CfxTSIPParser::ParseByte(&packet, pBuffer[i]);
        if (packet.State == tpsFull)
        {
            return TRUE;
        }
    }

    return FALSE;
}

//*************************************************************************************************
// CfxTSIPParser

CfxTSIPParser::CfxTSIPParser(): CfxGPSParser()
{
    Reset(); 
}

VOID CfxTSIPParser::Reset()
{
    CfxGPSParser::Reset();
    
    _lastMessage = 0;

    _8F20Count = _6DCount = _5CCount = 0;

    memset(&_packet, 0, sizeof(_packet));
    _packet.State = tpsEmpty;

    memset(&_report8F20, 0, sizeof(_report8F20));
    memset(&_report6D, 0, sizeof(_report6D));
    memset(&_report5C, 0, sizeof(_report5C));

    memset(&_satellites, 0, sizeof(_satellites));
}

BOOL CfxTSIPParser::GetPingStream(BYTE **ppBuffer, UINT *pLength)
{
    CfxGPSParser::GetPingStream(ppBuffer, pLength);

    if (_lastMessage != 0x5C)
    {
        BYTE buffer[] = {
            DLE, 0x24, DLE, ETX,
            DLE, 0x3C, 0x00, DLE, ETX,
            DLE, 0x8E, 0x20, DLE, ETX,
        };

        *ppBuffer = (BYTE *)MEM_ALLOC(sizeof(buffer));
        *pLength = sizeof(buffer);
        memcpy(*ppBuffer, &buffer[0], sizeof(buffer));

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL CfxTSIPParser::Parse8F20(TSIP_PACKET *pReport)
{
    memset(&_report8F20, 0, sizeof(_report8F20));

    if (pReport->Length != 56)
    {   
        return FALSE;
    }

	BYTE *buffer = pReport->Data;

    _report8F20.SubpacketId = buffer[0];

	DOUBLE velocityScale = (buffer[24] & 1) ? 0.020 : 0.005;

    _report8F20.Velocity[0] = GetInt16(buffer + 2) * velocityScale;
	_report8F20.Velocity[1] = GetInt16(buffer + 4) * velocityScale;
	_report8F20.Velocity[2] = GetInt16(buffer + 6) * velocityScale;

    _report8F20.TimeOfFix = GetUInt32(buffer + 8) * 0.001;

	INT lt = GetInt32(buffer + 12);
    _report8F20.Latitude = lt * (PI / MAX_INT32);

    UINT ult = GetUInt32(buffer + 16);
    _report8F20.Longitude = ult * (PI / MAX_INT32);
	if (_report8F20.Longitude > PI)
    {
        _report8F20.Longitude -= 2.0 * PI;
    }

    _report8F20.Altitude = GetInt32(buffer + 20) * 0.001;
	
    /* 25 blank; 29 = UTC */
    _report8F20.DatumIndex = (INT16)buffer[26] - 1;
    _report8F20.FixFlags = buffer[27];
    _report8F20.NSVs = buffer[28];
    _report8F20.WeekNumber = GetInt16(&buffer[30]);

	for (UINT i=0; i<8; i++) 
    {
		BYTE prnx = buffer[32 + 2*i];
        _report8F20.SV_PRN[i] = prnx & 0x3F;
		_report8F20.SV_IODC[i] = buffer[32 + 2*i + 1] + (INT16)(prnx - _report8F20.SV_PRN[i]) * 4;
	}

    _report8F20.TimeStamp = FxGetTickCount();

	return TRUE;
}

BOOL CfxTSIPParser::Parse6D(TSIP_PACKET *pReport)
{
    memset(&_report6D, 0, sizeof(_report6D));

	BYTE *buffer = pReport->Data;

    _report6D.NSVs = (buffer[0] & 0xF0) >> 4;

    if (pReport->Length != (17 + _report6D.NSVs)) 
    {
        return FALSE;
    }

    _report6D.Mode = buffer[0];

    _report6D.PDOP = GetFloat(&buffer[1]);
	_report6D.HDOP = GetFloat(&buffer[5]);
	_report6D.VDOP = GetFloat(&buffer[9]);
	_report6D.TDOP = GetFloat(&buffer[13]);

    for (UINT i=0; i<_report6D.NSVs; i++)
    {
        _report6D.SV_PRN[i] = buffer[i + 17];
    }

    _report6D.TimeStamp = FxGetTickCount();

	return TRUE;
}

BOOL CfxTSIPParser::Parse5C(TSIP_PACKET *pReport)
{
	BYTE *buffer = pReport->Data;

    if (pReport->Length != 24) 
    {
        return FALSE;
    }

    BYTE channel = buffer[1] >> 3;
    if (channel == 0x10)
    {
        channel = 1;
    }

    if (channel == 0)
    {
        memcpy(&_satellites[0], &_report5C.Satellites, sizeof(_satellites));
        memset(&_report5C, 0, sizeof(_report5C));
    }

    _report5C.PRN = buffer[0];
    _report5C.Slot = (buffer[1] & 0x07) + 1;
    _report5C.Channel = channel + 1;

    _report5C.AcquisitionFlag = buffer[2];
    _report5C.EphemerisFlag = buffer[3];
    _report5C.SignalLevel = (FLOAT)(20 * log(GetFloat(&buffer[4])) + 27);
    _report5C.TimeOfLastMeasurement = GetFloat(&buffer[8]);
    _report5C.Elevation = max(0, GetFloat(&buffer[12])); 
    _report5C.Azimuth = max(0, GetFloat(&buffer[16]));
    _report5C.OldMeasurementFlag = buffer[20];
    _report5C.IntegerMillisecondFlag = buffer[21];
    _report5C.BadDataFlag = buffer[22];
    _report5C.DataCollectFlag = buffer[23];

    UINT sz = sizeof(FLOAT);

    FXGPS_SATELLITE *satellite = &_report5C.Satellites[channel];
    satellite->PRN = _report5C.PRN;
    satellite->UsedInSolution = FALSE;
    satellite->Elevation = (INT16)(R2D * _report5C.Elevation); 
    satellite->Azimuth = (INT16)(R2D * _report5C.Azimuth);
    satellite->SignalQuality = (UINT16)_report5C.SignalLevel;

    _report5C.TimeStamp = FxGetTickCount();

	return TRUE;
}

VOID CfxTSIPParser::ParseReport(TSIP_PACKET *pReport)
{
    switch (pReport->Code)
    {
    case 0x13:
        {
            INT x=0;
        } 
        break;
    case 0x5C: 
        Parse5C(pReport); 
        _5CCount++;
        break;

    case 0x6D: 
        Parse6D(pReport); 
        _6DCount++;
        break;

    case 0x8F:
        switch (pReport->Data[0]) 
        {
        case 0x20: 
            Parse8F20(pReport); 
            _8F20Count++;
            break;
        }
        break;
    }

    _lastMessage = pReport->Code;
}

VOID CfxTSIPParser::ParseByte(TSIP_PACKET *pPacket, BYTE InByte)
{
    switch (pPacket->State)
	{
	case tpsDLE1:
		switch (InByte)
		{
		case 0:
		case ETX:   // illegal TSIP IDs 
            pPacket->Length = 0;
            pPacket->State = tpsEmpty;
			break;

		case DLE:   // try normal message start again 
            pPacket->Length = 0;
            pPacket->State = tpsDLE1;
			break;

		default:    // legal TSIP ID; start message 
            pPacket->Code = InByte;
            pPacket->Length = 0;
            pPacket->State = tpsData;
			break;
		}
		break;

	case tpsData:
		switch (InByte) 
        {
		case DLE:   // expect DLE or ETX next
            pPacket->State = tpsDLE2;
			break;

		default:    // normal data byte
            pPacket->Data[pPacket->Length] = InByte;
            pPacket->Length++;
            // no change in State
			break;
		}
		break;

	case tpsDLE2:
		switch (InByte) 
        {
		case DLE:   // normal data byte
            pPacket->Data[pPacket->Length] = InByte;
            pPacket->Length++;
			pPacket->State = tpsData;
			break;

		case ETX:   // end of message; return TRUE here
			pPacket->State = tpsFull;
			break;

		default:    // error: treat as TSIP_PARSED_DLE_1; start new report packet
            pPacket->Code = InByte;
            pPacket->Length = 0;
			pPacket->State = tpsData;
		}
		break;

	case tpsFull:
	case tpsEmpty:
	default:
		switch (InByte) 
        {
		case DLE:   // normal message start
            pPacket->Length = 0;
            pPacket->State = tpsDLE1;
			break;

		default:    // error: ignore newbyte
            pPacket->Length = 0;
			pPacket->State = tpsEmpty;
		}
		break;
	}

    if (pPacket->Length > MAX_REPORT_SIZE) 
    {
		// error: start new report packet
		pPacket->State = tpsEmpty;
        pPacket->Length = 0;
	}
}

VOID CfxTSIPParser::ParseBuffer(CHAR *pBuffer, UINT Length)
{
    CfxGPSParser::ParseBuffer(pBuffer, Length);

    for (UINT i=0; i<Length; i++)
    {
        ParseByte(&_packet, pBuffer[i]);
        if (_packet.State == tpsFull)
        {
            ParseReport(&_packet);
        }
    }
}

BOOL CfxTSIPParser::GetGPS(FXGPS *pGPS, UINT MaxAgeInSeconds)
{
    memset(pGPS, 0, sizeof(FXGPS));

    UINT currentTime = FxGetTickCount();
    UINT maxAgeInTicks = MaxAgeInSeconds * FxGetTicksPerSecond();

    // Clear out old data
    if (currentTime - _report6D.TimeStamp > maxAgeInTicks)
    {
        _6DCount = 0;
        memset(&_report6D, 0, sizeof(_report6D));
    }

    if (currentTime - _report5C.TimeStamp > maxAgeInTicks)
    {
        _5CCount = 0;
        memset(&_report5C, 0, sizeof(_report5C));
    }

    if (currentTime - _report8F20.TimeStamp > maxAgeInTicks)
    {
        _8F20Count = 0;
        memset(&_report8F20, 0, sizeof(_report8F20));
    }

    //
    // Fill in the FXGPS structure
    //

    pGPS->TimeStamp = currentTime;

    // Report 6D: HDOP and fix quality
    if ((_report6D.NSVs > 0) && (_report6D.TimeStamp != 0))
    {
        pGPS->Position.Accuracy = _report6D.HDOP;
    }

    // Fill in the satellites structure
    if (_report5C.TimeStamp != 0)
    {
        memcpy(&pGPS->Satellites[0], &_satellites[0], sizeof(_satellites));
    }

    // Report 8F20
    if ((_report8F20.NSVs > 0) && (_report8F20.TimeStamp != 0))
    {
        pGPS->Position.Latitude = R2D * _report8F20.Latitude;
        pGPS->Position.Longitude = R2D * _report8F20.Longitude;
        pGPS->Position.Altitude = _report8F20.Altitude;

        BYTE fixFlags = _report8F20.FixFlags;
        if (fixFlags & 1)
        {
            pGPS->Position.Quality = fqNone;
        } 
        else if (fixFlags & 2)
        {
            pGPS->Position.Quality = fqDiff3D;
        }
        else if (fixFlags & 4)
        {
            pGPS->Position.Quality = fq2D;
        }
        else
        {
            pGPS->Position.Quality = fq3D;
        }

        // Fill in the satellites that are being used
        if (_report5C.TimeStamp != 0)
        {
            for (UINT i=0; i<ARRAYSIZE(pGPS->Satellites); i++)
            {
                FXGPS_SATELLITE *satellite = &pGPS->Satellites[i];
                UINT16 prn = satellite->PRN;
                if (prn == 0) continue;

                pGPS->SatellitesInView++;
                for (UINT j=0; j<ARRAYSIZE(_report8F20.SV_PRN); j++)
                {
                    if (prn == _report8F20.SV_PRN[j])
                    {
                        satellite->UsedInSolution = TRUE;
                        break;
                    }
                }
            }
        }
    }

    // HDOP max is 50.0
    if ((pGPS->Position.Accuracy == 0.0) || (pGPS->Position.Accuracy > 50.0))
    {
        pGPS->Position.Accuracy = 50.0;
    }

    // Explicity prevent out of bounds accuracy 
    if ((pGPS->Position.Accuracy == 0.0) || (pGPS->Position.Accuracy >= 50.0))
    {
        pGPS->Position.Quality = fqNone;
    }

    // Need at least 2 sentences to have a decent fix
    if ((_6DCount < 2) && (_8F20Count < 2))
    {
        pGPS->Position.Quality = fqNone;
    }
    
    return (currentTime - _lastDataTimeStamp < maxAgeInTicks); 
}
