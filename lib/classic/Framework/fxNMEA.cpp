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

#include "fxNMEA.h"
#include "fxUtils.h"

//
// IsValidNMEAStream
//

BOOL IsValidNMEAStream(BYTE *pBuffer, UINT Length)
{
    return (strstr((CHAR *)pBuffer, "$GP") != NULL) ||
           (strstr((CHAR *)pBuffer, "$PSR") != NULL);
}

//*************************************************************************************************
// CfxNMEAParser

#define FIELD_LENGTH 32      

CfxNMEAParser::CfxNMEAParser(): CfxGPSParser()
{
    Reset(); 
}

VOID CfxNMEAParser::Reset()
{
    CfxGPSParser::Reset();

    _ggaCount = _gsaCount = _gsvCount = _rmcCount = _zdaCount = 0;

    _state = npsStart;
    
    _sirf = FALSE;

    memset(&_gga, 0, sizeof(_gga));
    memset(&_gsa, 0, sizeof(_gsa));
    memset(&_gsv, 0, sizeof(_gsv));
    memset(&_rmc, 0, sizeof(_rmc));
    memset(&_zda, 0, sizeof(_zda));
    memset(&_satellites, 0, sizeof(_satellites));
}

BOOL CfxNMEAParser::GetGPS(FXGPS *pGPS, UINT MaxAgeInSeconds)
{
    pGPS->State = dsConnected;

    UINT currentTime = FxGetTickCount();
    UINT maxAgeInTicks = MaxAgeInSeconds * FxGetTicksPerSecond();

    if (currentTime - _gga.TimeStamp > maxAgeInTicks)
    {
        _ggaCount = 0;
        memset(&_gga, 0, sizeof(_gga));
    }

    if (currentTime - _gsa.TimeStamp > maxAgeInTicks)
    {
        _gsaCount = 0;
        memset(&_gsa, 0, sizeof(_gsa));
    }

    if (currentTime - _gsv.TimeStamp > maxAgeInTicks)
    {
        _gsvCount = 0;
        memset(&_gsv, 0, sizeof(_gsv));
    }

    if (currentTime - _rmc.TimeStamp > maxAgeInTicks)
    {
        _rmcCount = 0;
        memset(&_rmc, 0, sizeof(_rmc));
    }

    if (currentTime - _zda.TimeStamp > maxAgeInTicks)
    {
        _zdaCount = 0;
        memset(&_zda, 0, sizeof(_zda));
    }

    // Fill pGPS structure
    pGPS->TimeStamp = _lastDataTimeStamp;

    pGPS->Position.Latitude = _gga.Latitude;
    pGPS->Position.Longitude = _gga.Longitude;
    pGPS->Position.Altitude = _gga.Altitude;

    // Ensure valid accuracy
    DOUBLE accuracy;
    if (_gga.TimeStamp)
    {
        accuracy = _gga.HDOP;
    }
    else if (_gsa.TimeStamp)
    {
        accuracy = _gsa.HDOP;
    }
    else
    {
        accuracy = 50.0;
    }
    pGPS->Position.Accuracy = max(0.01, min(50.0, accuracy));  
    pGPS->Position.Speed = 0;

    // Fix Quality
    pGPS->Position.Quality = fqNone;
    if (_gga.TimeStamp != 0)
    {
        switch (_gga.Quality)
        {
        case 1:  pGPS->Position.Quality = fq3D;     break;
        case 2:  pGPS->Position.Quality = fqDiff3D; break;
        default: pGPS->Position.Quality = fqNone;   break;
        }

        // If GSA tells us it's 2D, we have to respect that 
        if ((_gsa.TimeStamp != 0) && (_gsa.FixMode == 2))
        {
            pGPS->Position.Quality = fq2D;
        }

        pGPS->DateTime.Hour = _gga.Hour;
        pGPS->DateTime.Minute = _gga.Minute;
        pGPS->DateTime.Second = _gga.Second;
    }

    pGPS->SatellitesInView = _gsv.SatellitesInView;

    memcpy(&pGPS->Satellites[0], &_satellites[0], sizeof(_satellites));
    for (UINT i=0; i<ARRAYSIZE(pGPS->Satellites); i++)
    {
        FXGPS_SATELLITE *satellite = &pGPS->Satellites[i];
        UINT16 prn = satellite->PRN;
        if (prn == 0) continue;

        for (UINT j=0; j<ARRAYSIZE(_gsa.SatellitesInSolution); j++)
	    {
            if (prn == _gsa.SatellitesInSolution[j])
            {
                satellite->UsedInSolution = TRUE;
                break;
            }
        }
	}

    // Last effort - Pick up RMC data
    if (_rmc.Valid && (_rmc.TimeStamp != 0))
    {
        if (pGPS->Position.Quality == fqNone)
        {
            pGPS->Position.Quality = fq2D;
            pGPS->Position.Latitude = _rmc.Latitude;
            pGPS->Position.Longitude = _rmc.Longitude;
        }

        pGPS->DateTime.Year = _rmc.Year;
        pGPS->DateTime.Month = _rmc.Month;
        pGPS->DateTime.Day = _rmc.Day;
        pGPS->DateTime.Hour = _rmc.Hour;
        pGPS->DateTime.Minute = _rmc.Minute;
        pGPS->DateTime.Second = _rmc.Second;
        pGPS->Heading = ((UINT)_rmc.Course % 360);
        pGPS->TimeStamp = _rmc.TimeStamp; 
        pGPS->Position.Speed = KnotsToKmHour(_rmc.GroundSpeed);
    }

    // Pick up the date and time.
    if (_zda.Valid && (_zda.TimeStamp != 0) && (_zda.TimeStamp > _rmc.TimeStamp))
    {
        pGPS->DateTime.Year = _zda.Year;
        pGPS->DateTime.Month = _zda.Month;
        pGPS->DateTime.Day = _zda.Day;
        pGPS->DateTime.Hour = _zda.Hour;
        pGPS->DateTime.Minute = _zda.Minute;
        pGPS->DateTime.Second = _zda.Second;
    }

    // Explicity prevent out of bounds accuracy 
    if ((pGPS->Position.Accuracy == 0.0) || (pGPS->Position.Accuracy >= 50.0))
    {
        pGPS->Position.Quality = fqNone;
    }

    // Prevent partial solutions, e.g. where RMC has occurred without GGA
    if ((_ggaCount < 2) && (_rmcCount < 2))
    {
        pGPS->Position.Quality = fqNone;
    }
    
    return (currentTime - _lastDataTimeStamp < maxAgeInTicks); 
}

VOID CfxNMEAParser::ParseBuffer(CHAR *pBuffer, UINT Length)
{
    CfxGPSParser::ParseBuffer(pBuffer, Length);

    for (UINT i=0; i<Length; i++)
    {
        ProcessNMEA(pBuffer[i]);
    }
}

VOID CfxNMEAParser::ProcessNMEA(CHAR Data)
{
    switch (_state)
    {
        // '$' is message header
        case npsStart:
            if (Data == '$')
            {
                _expectedChecksum = 0;  
                _index = 0;
                _state = npsCommand;
            }
            break;

        // Get Command
        case npsCommand:
            if ((Data != ',') && (Data != '*'))
            {
                _command[_index++] = Data;
                if (_index >= sizeof(_command))
                {
                    _state = npsStart;
                }
            }
            else
            {
                _command[_index] = '\0';   
                _index = 0;
                _state = npsData;     
            }
            _expectedChecksum ^= Data;
            break;

        // Get Data
        case npsData:
            // Detect checksum flag
            if (Data == '*') 
            {
                _data[_index] = '\0';
                _state = npsChecksum1;
            }
            else 
            {
                // Check for end of sentence with no checksum
                if (Data == '\r')
                {
                    _data[_index] = '\0';
                    ProcessCommand(_command, _data);
                    _state = npsStart;
                }
                else
                {
                    // Store data and calculate checksum
                    _expectedChecksum ^= Data;
                    _data[_index] = Data;
                    if (++_index >= sizeof(_data)) 
                    {
                        _state = npsStart;
                    }
                }
            }
            break;

        // Get NMEA checksum
        case npsChecksum1:
            if ((Data - '0') <= 9)
            {
                _receivedChecksum = (Data - '0') << 4;
            }
            else
            {
                _receivedChecksum = (Data - 'A' + 10) << 4;
            }

            _state = npsChecksum2;
            break;

        // Validate checksum
        case npsChecksum2:
            if ((Data - '0') <= 9)
            {
                _receivedChecksum |= (Data - '0');
            }
            else
            {
                _receivedChecksum |= (Data - 'A' + 10);
            }

            if (_expectedChecksum == _receivedChecksum)
            {
                ProcessCommand(_command, _data);
            }

            _state = npsStart;

            break;
        
        default: 
            FX_ASSERT(FALSE);
    }
}

VOID CfxNMEAParser::ProcessCommand(CHAR *pCommand, CHAR *pData)
{
    // GPGGA
    if (strcmp(pCommand, "GPGGA") == 0)
    {
        _ggaCount++;
        ProcessGGA(pData);
    }
    // GPGSA
    else if (strcmp(pCommand, "GPGSA") == 0)
    {
        _gsaCount++;
        ProcessGSA(pData);
    }
    // GPGSV
    else if (strcmp(pCommand, "GPGSV") == 0)
    {
        _gsvCount++;
        ProcessGSV(pData);
    }
    // GPRMC
    else if (strcmp(pCommand, "GPRMC") == 0)
    {
        _rmcCount++;
        ProcessRMC(pData);
    }
    // GPZDA
    else if (strcmp(pCommand, "GPZDA") == 0)
    {
        _zdaCount++;
        ProcessZDA(pData);
    }
    else if (strncmp(pCommand, "PSRF", 4) == 0)
    {
        _sirf = TRUE;
    }
}

BOOL CfxNMEAParser::GetPingStream(BYTE **ppBuffer, UINT *pLength)
{
    if (_sirf && (_rmcCount == 0))
    {
        // Turn on RMC message
        CHAR buffer[] = "$PSRF103,04,00,01,01*21\r\n";
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

BOOL CfxNMEAParser::GetField(CHAR *pData, CHAR *pField, UINT FieldNumber, UINT FieldLength)
{
    FX_ASSERT(pData != NULL);
    FX_ASSERT(pField != NULL);
    FX_ASSERT(FieldLength > 0);

    // Initialize pField
    pField[0] = '\0';

    // Skip to the right start location
    UINT i = 0;
    UINT field = 0;
    while ((field != FieldNumber) && pData[i])
    {
        if (pData[i] == ',')
        {
            field++;
        }

        i++;

        if (pData[i] == 0)
        {
            return FALSE;
        }
    }

    // No data
    if ((pData[i] == ',') || (pData[i] == '*'))
    {
        return FALSE;
    }

    // Copy to pField
    UINT j = 0;
    while ((pData[i] != ',') && (pData[i] != '*') && pData[i])
    {
        pField[j] = pData[i];
        j++; 
        i++;

        // Clip to max length
        if (j >= FieldLength)
        {
            j = FieldLength-1;
            break;
        }
    }
    pField[j] = '\0';

    return TRUE;
}

VOID CfxNMEAParser::ProcessGGA(CHAR *pData)
{
    memset(&_gga, 0, sizeof(_gga));
    
    CHAR field[FIELD_LENGTH];
    CHAR buffer[16];

    // Time
    if (GetField(pData, field, 0, FIELD_LENGTH))
    {
        // Hour
        buffer[0] = field[0];
        buffer[1] = field[1];
        buffer[2] = '\0';
        _gga.Hour = atoi(buffer);

        // minute
        buffer[0] = field[2];
        buffer[1] = field[3];
        buffer[2] = '\0';
        _gga.Minute = atoi(buffer);

        // Second
        buffer[0] = field[4];
        buffer[1] = field[5];
        buffer[2] = '\0';
        _gga.Second = atoi(buffer);
    }

    // Latitude
    if (GetField(pData, field, 1, FIELD_LENGTH))
    {
        _gga.Latitude = atof(field + 2) / 60.0;
        field[2] = '\0';
        _gga.Latitude += atof(field);

    }

    if (GetField(pData, field, 2, FIELD_LENGTH))
    {
        if (field[0] == 'S')
        {
            _gga.Latitude = -_gga.Latitude;
        }
    }

    // Longitude
    if (GetField(pData, field, 3, FIELD_LENGTH))
    {
        _gga.Longitude = atof(field + 3) / 60.0;
        field[3] = '\0';
        _gga.Longitude += atof(field);
    }

    if (GetField(pData, field, 4, FIELD_LENGTH))
    {
        if (field[0] == 'W')
        {
            _gga.Longitude = -_gga.Longitude;
        }
    }

    //
    // GPS quality: NOT the same as FXGPS_QUALITY - this number is 0, 1, or 2 and 
    // is interpreted correctly when making use of the _gga structure .
    //
    if (GetField(pData, field, 5, FIELD_LENGTH))
    {
        _gga.Quality = field[0] - '0';
    }

    // Satellites in use
    if (GetField(pData, field, 6, FIELD_LENGTH))
    {
        buffer[0] = field[0];
        buffer[1] = field[1];
        buffer[2] = '\0';
        _gga.SatellitesInUse = atoi(buffer);
    }

    // HDOP
    if (GetField(pData, field, 7, FIELD_LENGTH))
    {
        _gga.HDOP = atof(field);
    }
    
    // Altitude
    if (GetField(pData, field, 8, FIELD_LENGTH))
    {
        _gga.Altitude = atof(field);
    }

    _gga.TimeStamp = FxGetTickCount();
}

VOID CfxNMEAParser::ProcessGSA(CHAR *pData)
{
    memset(&_gsa, 0, sizeof(_gsa));

    CHAR field[FIELD_LENGTH];
    CHAR buffer[16];

    // Mode
    if (GetField(pData, field, 0, FIELD_LENGTH))
    {
        _gsa.AutoMode = field[0] == 'A';
    }

    // Fix Mode
    if (GetField(pData, field, 1, FIELD_LENGTH))
    {
        _gsa.FixMode = field[0] - '0';
    }

    // Active satellites
    for (INT i=0; i<12; i++)
    {
        if (GetField(pData, field, i + 2, FIELD_LENGTH))
        {
            buffer[0] = field[0];
            buffer[1] = field[1];
            buffer[2] = '\0';
            _gsa.SatellitesInSolution[i] = atoi(buffer);
        }
    }

    // PDOP
    if (GetField(pData, field, 14, FIELD_LENGTH))
    {
        _gsa.PDOP = atof(field);
    }

    // HDOP
    if (GetField(pData, field, 15, FIELD_LENGTH))
    {
        _gsa.HDOP = atof(field);
    }

    //
    // VDOP
    //
    if (GetField(pData, field, 16, FIELD_LENGTH))
    {
        _gsa.VDOP = atof(field);
    }

    _gsa.TimeStamp = FxGetTickCount();
}

VOID CfxNMEAParser::ProcessGSV(CHAR *pData)
{
    _gsv.TimeStamp = 0;
    CHAR field[FIELD_LENGTH];

    // Get total number of messages
    INT totalMessages = 0;
    if (GetField(pData, field, 0, FIELD_LENGTH))
    {
        totalMessages = atoi(field);
        if ((totalMessages < 1) || (totalMessages * 4 >= 36)) 
        {
            return;
        }
    }

    // Message number
    INT messageNumber = 0;
    if (GetField(pData, field, 1, FIELD_LENGTH))
    {
        messageNumber = atoi(field);

        // If this is the first message, clear all data
        if (messageNumber == 1)
        {
            memcpy(&_satellites[0], &_gsv.Satellites[0], sizeof(_satellites)); 
            memset(&_gsv, 0, sizeof(_gsv));
        }
        else if ((messageNumber < 1) || (messageNumber > 9))
        {
            // Bad message number
            return;
        }
    }

    // Total satellites in view
    if (GetField(pData, field, 2, FIELD_LENGTH))
    {
        _gsv.SatellitesInView = atoi(field);
    }

    // Get individual satellite data
    for (INT i=0; i<4; i++)
    {
        FXGPS_SATELLITE *satellite = &_gsv.Satellites[i + (messageNumber - 1) * 4];

        // Satellite ID
        if (GetField(pData, field, 4*i + 3, FIELD_LENGTH))
        {
            satellite->PRN = atoi(field);
        }

        // Elevation
        if (GetField(pData, field, 4*i + 4, FIELD_LENGTH))
        {
            satellite->Elevation = atoi(field);
        }

        // Azimuth
        if (GetField(pData, field, 4*i + 5, FIELD_LENGTH))
        {
            satellite->Azimuth = atoi(field);
        }

        // SNR
        if (GetField(pData, field, 4*i + 6, FIELD_LENGTH))
        {
            satellite->SignalQuality = min(GPS_MAX_SIGNAL_QUALITY, atoi(field));
        }
	}

    _gsv.TimeStamp = FxGetTickCount();
}

VOID CfxNMEAParser::ProcessRMC(CHAR *pData)
{
    memset(&_rmc, 0, sizeof(_rmc));

    CHAR field[FIELD_LENGTH];
    CHAR buffer[16];

    // Time
    if (GetField(pData, field, 0, FIELD_LENGTH))
    {
        // Hour
        buffer[0] = field[0];
        buffer[1] = field[1];
        buffer[2] = '\0';
        _rmc.Hour = atoi(buffer);

        // minute
        buffer[0] = field[2];
        buffer[1] = field[3];
        buffer[2] = '\0';
        _rmc.Minute = atoi(buffer);

        // Second
        buffer[0] = field[4];
        buffer[1] = field[5];
        buffer[2] = '\0';
        _rmc.Second = atoi(buffer);
    }

    // Data valid
    if (GetField(pData, field, 1, FIELD_LENGTH))
    {
        _rmc.Valid = field[0] == 'A';
    }

    // Latitude
    if (GetField(pData, field, 2, FIELD_LENGTH))
    {
        _rmc.Latitude = atof(field + 2) / 60.0;
        field[2] = '\0';
        _rmc.Latitude += atof(field);
    }

    if (GetField(pData, field, 3, FIELD_LENGTH))
    {
        if (field[0] == 'S')
        {
            _rmc.Latitude = -_rmc.Latitude;
        }
    }

    // Longitude
    if (GetField(pData, field, 4, FIELD_LENGTH))
    {
        _rmc.Longitude = atof(field+3) / 60.0;
        field[3] = '\0';
        _rmc.Longitude += atof(field);
    }

    if (GetField(pData, field, 5, FIELD_LENGTH))
    {
        if (field[0] == 'W')
        {
            _rmc.Longitude = -_rmc.Longitude;
        }
    }

    // Ground speed
    if (GetField(pData, field, 6, FIELD_LENGTH))
    {
        _rmc.GroundSpeed = atof(field);
    }

    // Course over ground, degrees true
    if (GetField(pData, field, 7, FIELD_LENGTH))
    {
        _rmc.Course = atof(field);
    }

    // Date
    if (GetField(pData, field, 8, FIELD_LENGTH))
    {
        // Day
        buffer[0] = field[0];
        buffer[1] = field[1];
        buffer[2] = '\0';
        _rmc.Day = atoi(buffer);

        // Month
        buffer[0] = field[2];
        buffer[1] = field[3];
        buffer[2] = '\0';
        _rmc.Month = atoi(buffer);

        // Year is only 2 digits
        buffer[0] = field[4];
        buffer[1] = field[5];
        buffer[2] = '\0';
        _rmc.Year = 2000 + atoi(buffer);
    }

    // Course over ground, degrees true
    if (GetField(pData, field, 9, FIELD_LENGTH))
    {
        _rmc.MagneticVariation = atof(field);
    }

    if (GetField(pData, field, 10, FIELD_LENGTH))
    {
        if (field[0] == 'W')
        {
            _rmc.MagneticVariation = -_rmc.MagneticVariation;
        }
    }

    _rmc.TimeStamp = FxGetTickCount();
}

VOID CfxNMEAParser::ProcessZDA(CHAR *pData)
{
    memset(&_zda, 0, sizeof(_zda));

    CHAR field[FIELD_LENGTH];
    CHAR buffer[16];

    // Time
    if (GetField(pData, field, 0, FIELD_LENGTH))
    {
        // Hour
        buffer[0] = field[0];
        buffer[1] = field[1];
        buffer[2] = '\0';
        _zda.Hour = atoi(buffer);

        // minute
        buffer[0] = field[2];
        buffer[1] = field[3];
        buffer[2] = '\0';
        _zda.Minute = atoi(buffer);

        // Second
        buffer[0] = field[4];
        buffer[1] = field[5];
        buffer[2] = '\0';
        _zda.Second = atoi(buffer);
    }

    // Day
    if (GetField(pData, field, 1, FIELD_LENGTH))
    {
        _zda.Day = atoi(field);
    }

    // Month
    if (GetField(pData, field, 2, FIELD_LENGTH))
    {
        _zda.Month = atoi(field);
    }

    // Year
    if (GetField(pData, field, 3, FIELD_LENGTH))
    {
        _zda.Year = atoi(field);
    }

    // Local hour
    if (GetField(pData, field, 4, FIELD_LENGTH))
    {
        _zda.LocalHour = atoi(field);
    }

    // Local minute
    if (GetField(pData, field, 5, FIELD_LENGTH))
    {
        _zda.LocalMinute = atoi(field);
    }

    _zda.Valid = (_zda.Year >= 2008) && (_zda.Month <= 12) && (_zda.Day <= 31) &&
                 (_zda.Hour < 24) && (_zda.Minute < 60) && (_zda.Second < 60);
    _zda.TimeStamp = FxGetTickCount();
}
