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

#include "fxLaserAtlanta.h"

//
// IsValidLaserAtlantaStream
//

BOOL IsValidLaserAtlantaStream(BYTE *pBuffer, UINT Length)
{
    return (strstr((CHAR *)pBuffer, "$LA") != NULL) ||
		   (strstr((CHAR *)pBuffer, "$PLTIT") != NULL) ||
           (strstr((CHAR *)pBuffer, "$PMDLA") != NULL) ||
           (strstr((CHAR *)pBuffer, "$PMDLB") != NULL);
}

//*************************************************************************************************
// CfxLaserAtlantaParser

#define FIELD_LENGTH 32      

CfxLaserAtlantaParser::CfxLaserAtlantaParser(): CfxRangeFinderParser()
{
    Reset(); 
}

VOID CfxLaserAtlantaParser::Reset()
{
    CfxRangeFinderParser::Reset();

    _la1kaCount = _la1kcCount = _la1kdCount = _pltithvCount = _pltithtCount = 0;

    _state = lpsStart;

    memset(&_la1ka, 0, sizeof(_la1ka));
    memset(&_la1kc, 0, sizeof(_la1kc));
    memset(&_la1kd, 0, sizeof(_la1kd));
	memset(&_pltithv, 0, sizeof(_pltithv));
	memset(&_pltitht, 0, sizeof(_pltitht));
}

BOOL CfxLaserAtlantaParser::GetRange(FXRANGE *pRange, UINT MaxAgeInSeconds)
{
    pRange->State = dsConnected;

    UINT currentTime = FxGetTickCount();
    UINT maxAgeInTicks = MaxAgeInSeconds * FxGetTicksPerSecond();

    if (currentTime - _la1ka.TimeStamp > maxAgeInTicks)
    {
        _la1kaCount = 0;
        memset(&_la1ka, 0, sizeof(_la1ka));
    }

    if (currentTime - _la1kc.TimeStamp > maxAgeInTicks)
    {
        _la1kcCount = 0;
        memset(&_la1kc, 0, sizeof(_la1kc));
    }

    if (currentTime - _la1kd.TimeStamp > maxAgeInTicks)
    {
        _la1kdCount = 0;
        memset(&_la1kd, 0, sizeof(_la1kd));
    }

	if (currentTime - _pltithv.TimeStamp > maxAgeInTicks)
	{
		_pltithvCount = 0;
		memset(&_pltithv, 0, sizeof(_pltithv));
	}

	if (currentTime - _pltitht.TimeStamp > maxAgeInTicks)
	{
		_pltithtCount = 0;
		memset(&_pltitht, 0, sizeof(_pltitht));
	}

    if (_la1ka.TimeStamp > 0)
    {
        pRange->TimeStamp = _lastDataTimeStamp;
        pRange->Range.Flags = 0;
        
        if (_la1ka.Flags & LAFLAG_RANGE)
        {
            pRange->Range.Flags |= RANGE_FLAG_RANGE;
            pRange->Range.Range = _la1ka.Range.Range;
            pRange->Range.RangeUnits = _la1ka.Range.Units;
        }

        if (_la1ka.Flags & LAFLAG_POLAR_BEARING)
        {
            pRange->Range.Flags |= RANGE_FLAG_POLAR_BEARING;
            pRange->Range.PolarBearing = _la1ka.Polar.Bearing;
            pRange->Range.PolarBearingMode = _la1ka.Polar.BearingMode;
        }

        if (_la1ka.Flags & LAFLAG_POLAR_INCLINATION)
        {
            pRange->Range.Flags |= RANGE_FLAG_POLAR_INCLINATION;
            pRange->Range.PolarInclination = _la1ka.Polar.Inclination;
        }

        if (_la1ka.Flags & LAFLAG_POLAR_ROLL)
        {
            pRange->Range.Flags |= RANGE_FLAG_POLAR_ROLL;
            pRange->Range.PolarRoll = _la1ka.Polar.Roll;
        }
    }
    else if (_la1kc.TimeStamp > 0)
    {
        pRange->TimeStamp = _lastDataTimeStamp;
        pRange->Range.Flags = 0;

        if (_la1kc.Flags & LAFLAG_RANGE)
        {
            pRange->Range.Flags |= RANGE_FLAG_RANGE;
            pRange->Range.Range = _la1kc.Range.Range;
            pRange->Range.RangeUnits = _la1kc.Range.Units;
        }
    }
    else if (_la1kd.TimeStamp > 0)
    {
        pRange->TimeStamp = _lastDataTimeStamp;
        pRange->Range.Flags = 0;

        if (_la1kd.Flags & LAFLAG_RANGE)
        {
            pRange->Range.Flags |= RANGE_FLAG_RANGE;
            pRange->Range.Range = _la1kd.Range.Range;
            pRange->Range.RangeUnits = _la1kd.Range.Units;
        }

        if (_la1kd.Flags & LAFLAG_POLAR_BEARING)
        {
            pRange->Range.Flags |= RANGE_FLAG_POLAR_BEARING;
            pRange->Range.PolarBearing = _la1kd.Polar.Bearing;
            pRange->Range.PolarBearingMode = _la1kd.Polar.BearingMode;
        }

        if (_la1kd.Flags & LAFLAG_POLAR_INCLINATION)
        {
            pRange->Range.Flags |= RANGE_FLAG_POLAR_INCLINATION;
            pRange->Range.PolarInclination = _la1kd.Polar.Inclination;
        }

        if (_la1kd.Flags & LAFLAG_POLAR_ROLL)
        {
            pRange->Range.Flags |= RANGE_FLAG_POLAR_ROLL;
            pRange->Range.PolarRoll = _la1kd.Polar.Roll;
        }
    }
	else if (_pltithv.TimeStamp > 0)
	{
		pRange->TimeStamp = _lastDataTimeStamp;
        pRange->Range.Flags = 0;

        if (_pltithv.Flags & TPFLAG_SLOPE_DISTANCE)
        {
            pRange->Range.Flags |= RANGE_FLAG_RANGE;
            pRange->Range.Range = _pltithv.SDvalue;
            pRange->Range.RangeUnits = _pltithv.SDunits;
        }

        if (_pltithv.Flags & TPFLAG_AZIMUTH)
        {
            pRange->Range.Flags |= RANGE_FLAG_AZIMUTH;
            pRange->Range.Azimuth = _pltithv.AZvalue;
        }

        if (_pltithv.Flags & TPFLAG_INCLINATION)
        {
            pRange->Range.Flags |= RANGE_FLAG_INCLINATION;
            pRange->Range.Inclination = _pltithv.INCvalue;
        }

        if (_pltithv.Flags & TPFLAG_HORIZONTAL_DISTANCE)
        {
            pRange->Range.Flags |= RANGE_FLAG_HORIZONTAL_DISTANCE;
            pRange->Range.HorizontalDistance = _pltithv.HDvalue;
            pRange->Range.HorizontalDistanceUnits = _pltithv.HDunits;
        }
	}

    return (currentTime - _lastDataTimeStamp < maxAgeInTicks); 
}

VOID CfxLaserAtlantaParser::ParseBuffer(CHAR *pBuffer, UINT Length)
{
    CfxRangeFinderParser::ParseBuffer(pBuffer, Length);

    for (UINT i=0; i<Length; i++)
    {
        ProcessLaserAtlanta(pBuffer[i]);
    }
}

VOID CfxLaserAtlantaParser::ProcessLaserAtlanta(CHAR Data)
{
    switch (_state)
    {
        // '$' is message header
        case lpsStart:
            if (Data == '$')
            {
                _expectedChecksum = 0;  
                _index = 0;
                _state = lpsCommand;
            }
            break;

        // Get Command
        case lpsCommand:
            if ((Data != ',') && (Data != '*'))
            {
                _command[_index++] = Data;
                if (_index >= sizeof(_command))
                {
                    _state = lpsStart;
                }
            }
            else
            {
                _command[_index] = '\0';   
                _index = 0;
                _state = lpsData;     
            }
            _expectedChecksum ^= Data;
            break;

        // Get Data
        case lpsData:
            // Detect checksum flag
            if (Data == '*') 
            {
                _data[_index] = '\0';
                _state = lpsChecksum1;
            }
            else 
            {
                // Check for end of sentence with no checksum
                if (Data == '\r')
                {
                    _data[_index] = '\0';
                    ProcessCommand(_command, _data);
                    _state = lpsStart;
                }
                else
                {
                    // Store data and calculate checksum
                    _expectedChecksum ^= Data;
                    _data[_index] = Data;
                    if (++_index >= sizeof(_data)) 
                    {
                        _state = lpsStart;
                    }
                }
            }
            break;

        // Get LaserAtlanta checksum
        case lpsChecksum1:
            if ((Data - '0') <= 9)
            {
                _receivedChecksum = (Data - '0') << 4;
            }
            else
            {
                _receivedChecksum = (Data - 'A' + 10) << 4;
            }

            _state = lpsChecksum2;
            break;

        // Validate checksum
        case lpsChecksum2:
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

            _state = lpsStart;

            break;
        
        default: 
            FX_ASSERT(FALSE);
    }
}

VOID CfxLaserAtlantaParser::ProcessCommand(CHAR *pCommand, CHAR *pData)
{
    _la1ka.TimeStamp = _la1kc.TimeStamp = _la1kd.TimeStamp = 0;

    // LA1KA
    if (strcmp(pCommand, "LA1KA") == 0)
    {
        _la1kaCount++;
        ProcessLA1KA(pData, &_la1ka);
    }
    // LA1KC
    else if (strcmp(pCommand, "LA1KC") == 0)
    {
        _la1kcCount++;
        ProcessLA1KC(pData, &_la1kc);
    }
    // LA1KD
    else if (strcmp(pCommand, "LA1KD") == 0)
    {
        _la1kdCount++;
        ProcessLA1KD(pData, &_la1kd);
    }
	// PLTIT,HV and PLTIT,HT, PMDLA,HV
	else if ((strcmp(pCommand, "PLTIT") == 0) || 
             (strcmp(pCommand, "PMDLA") == 0) ||
             (strcmp(pCommand, "PMDLB") == 0))
	{
		if (strncmp(pData, "HV", 2) == 0)
		{
			_pltithvCount++;
			ProcessPLTITHV(pData, &_pltithv);
		}
		else if (strncmp(pData, "HT", 2) == 0)
		{
			_pltithtCount++;
			ProcessPLTITHT(pData, &_pltitht);
		}
	}
}

BOOL CfxLaserAtlantaParser::GetField(CHAR *pData, CHAR *pField, UINT FieldNumber, UINT FieldLength)
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

VOID CfxLaserAtlantaParser::ProcessLA1KA(CHAR *pData, LASERATLANTA_LA1KA *pla1ka)
{
    // BUGBUG: remove these 2 lines
    //CHAR szSentence[] = "631.8,F,215.0,D,M,090.3,D,090.7,D";
    //pData = szSentence;

    // Initialize structure
    CHAR field[FIELD_LENGTH] = "";
    memset(pla1ka, 0, sizeof(LASERATLANTA_LA1KA));

    // Range 
    if (GetField(pData, field, 0, FIELD_LENGTH))
    {
        pla1ka->Range.Range = atof(field);

        // Range units
        if (GetField(pData, field, 1, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'F') || (field[0] == 'M') || (field[0] == 'Y') || (field[0] == '2'));
            pla1ka->Range.Units = field[0];
            pla1ka->Flags |= LAFLAG_RANGE;
        }
    }

    // Bearing
    if (GetField(pData, field, 2, FIELD_LENGTH))
    {
        pla1ka->Polar.Bearing = atof(field);

        // Bearing mode: Magnetic, True, Encoder
        if (GetField(pData, field, 4, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'M') || (field[0] == 'T') || (field[0] == 'E'));
            pla1ka->Polar.BearingMode = field[0];
            pla1ka->Flags |= LAFLAG_POLAR_BEARING;
        }
    }

    // Inclination
    if (GetField(pData, field, 5, FIELD_LENGTH))
    {
        pla1ka->Polar.Inclination = atof(field);

        // Must be in Degrees
        if (GetField(pData, field, 6, FIELD_LENGTH))
        {
            FX_ASSERT(field[0] == 'D');
            pla1ka->Flags |= LAFLAG_POLAR_INCLINATION;
        }
    }

    // Roll
    if (GetField(pData, field, 7, FIELD_LENGTH))
    {
        pla1ka->Polar.Roll = atof(field);

        // Must be in Degrees
        if (GetField(pData, field, 8, FIELD_LENGTH))
        {
            FX_ASSERT(field[0] == 'D');
            pla1ka->Flags |= LAFLAG_POLAR_ROLL;
        }
    }

    pla1ka->TimeStamp = FxGetTickCount();
}

VOID CfxLaserAtlantaParser::ParsePolar(CHAR *pData, UINT BaseIndex, LASERATLANTA_POLAR *pPolar, UINT *pFlags)
{
    CHAR field[FIELD_LENGTH];
    memset(pPolar, 0, sizeof(LASERATLANTA_POLAR));

    // Bearing
    if (GetField(pData, field, BaseIndex, FIELD_LENGTH))
    {
        pPolar->Bearing = atof(field);

        // Bearing units
        if (GetField(pData, field, BaseIndex + 1, FIELD_LENGTH))
        {
            FX_ASSERT(field[0] == 'D');

            // Bearing mode
            if (GetField(pData, field, BaseIndex + 2, FIELD_LENGTH))
            {
                FX_ASSERT((field[0] == 'M') || (field[0] == 'T') || (field[0] == 'E'));
                pPolar->BearingMode = field[0];
                *pFlags |= LAFLAG_POLAR_BEARING;
            }
        }
    }
    
    // Inclination
    if (GetField(pData, field, BaseIndex + 3, FIELD_LENGTH))
    {
        pPolar->Inclination = atof(field);

        // Inclination units
        if (GetField(pData, field, BaseIndex + 4, FIELD_LENGTH))
        {
            FX_ASSERT(field[0] == 'D');

            // Inclination source
            if (GetField(pData, field, BaseIndex + 5, FIELD_LENGTH))
            {
                FX_ASSERT(field[0] == 'E');
                pPolar->InclinationSource = field[0];
            }

            *pFlags |= LAFLAG_POLAR_INCLINATION;
        }
    }

    // Roll
    if (GetField(pData, field, BaseIndex + 6, FIELD_LENGTH))
    {
        pPolar->Roll = atof(field);

        // Roll units
        if (GetField(pData, field, BaseIndex + 7, FIELD_LENGTH))
        {
            FX_ASSERT(field[0] == 'D');
            *pFlags |= LAFLAG_POLAR_ROLL;
        }
    }
}

VOID CfxLaserAtlantaParser::ParseCartesian(CHAR *pData, UINT BaseIndex, LASERATLANTA_CARTESIAN *pCartesian, UINT *pFlags)
{
    CHAR field[FIELD_LENGTH];
    memset(pCartesian, 0, sizeof(LASERATLANTA_CARTESIAN));

    // X Axis Range (East)
    if (GetField(pData, field, BaseIndex, FIELD_LENGTH))
    {
        pCartesian->XAxisRange = atof(field);

        // X Units
        if (GetField(pData, field, BaseIndex + 1, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'F') || (field[0] == 'M') || (field[0] == 'Y'));
            pCartesian->XUnits = field[0];
            *pFlags |= LAFLAG_CARTESIAN_X;
        }
    }

    // Y Axis Range (North)
    if (GetField(pData, field, BaseIndex + 2, FIELD_LENGTH))
    {
        pCartesian->YAxisRange = atof(field);

        // Y Units
        if (GetField(pData, field, BaseIndex + 3, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'F') || (field[0] == 'M') || (field[0] == 'Y'));
            pCartesian->YUnits = field[0];
            *pFlags |= LAFLAG_CARTESIAN_Y;
        }
    }

    // Z Axis Range (Up)
    if (GetField(pData, field, BaseIndex + 4, FIELD_LENGTH))
    {
        pCartesian->ZAxisRange = atof(field);

        // Z Units
        if (GetField(pData, field, BaseIndex + 5, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'F') || (field[0] == 'M') || (field[0] == 'Y'));
            pCartesian->ZUnits = field[0];
            *pFlags |= LAFLAG_CARTESIAN_Z;
        }
    }

    // Horizontal Range
    if (GetField(pData, field, BaseIndex + 6, FIELD_LENGTH))
    {
        pCartesian->HRange = atof(field);

        // H Units
        if (GetField(pData, field, BaseIndex + 7, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'F') || (field[0] == 'M') || (field[0] == 'Y'));
            pCartesian->HUnits = field[0];
            *pFlags |= LAFLAG_CARTESIAN_HORIZONTAL;
        }
    }

    // Vertical Range
    if (GetField(pData, field, BaseIndex + 8, FIELD_LENGTH))
    {
        pCartesian->VRange = atof(field);

        // V Units
        if (GetField(pData, field, BaseIndex + 9, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'F') || (field[0] == 'M') || (field[0] == 'Y'));
            pCartesian->VUnits = field[0];
            *pFlags |= LAFLAG_CARTESIAN_VERTICAL;
        }
    }
}

VOID CfxLaserAtlantaParser::ParseGolfTournament(CHAR *pData, UINT BaseIndex, LASERATLANTA_GOLFTOURNAMENT *pGolfTournament, UINT *pFlags)
{
    CHAR field[FIELD_LENGTH];
    memset(pGolfTournament, 0, sizeof(LASERATLANTA_GOLFTOURNAMENT));

    // Bearing
    if (GetField(pData, field, BaseIndex, FIELD_LENGTH))
    {
        pGolfTournament->Bearing = atof(field);

        // Bearing units
        if (GetField(pData, field, BaseIndex + 1, FIELD_LENGTH))
        {
            FX_ASSERT(field[0] == 'D');

            // Bearing mode
            if (GetField(pData, field, BaseIndex + 2, FIELD_LENGTH))
            {
                FX_ASSERT((field[0] == 'M') || (field[0] == 'T') || (field[0] == 'E') || (field[0] == 'D'));
                pGolfTournament->BearingMode = field[0];
                *pFlags |= LAFLAG_GOLFTOURNAMENT_BEARING;
            }
        }
    }
    
    // Vertical Missing-Line Mode Range
    if (GetField(pData, field, BaseIndex + 3, FIELD_LENGTH))
    {
        pGolfTournament->VMissingLineModeRange = atof(field);

        // Vertical units
        if (GetField(pData, field, BaseIndex + 4, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'F') || (field[0] == 'M') || (field[0] == 'Y'));
            pGolfTournament->VUnits = field[0];

            if (GetField(pData, field, BaseIndex + 5, FIELD_LENGTH))
            {
                pGolfTournament->VModeId[0] = field[0];
                pGolfTournament->VModeId[1] = field[1];
                *pFlags |= LAFLAG_GOLFTOURNAMENT_VRANGE;
            }
        }
    }

    // Horizontal Missing-Line Mode Range
    if (GetField(pData, field, BaseIndex + 6, FIELD_LENGTH))
    {
        pGolfTournament->HMissingLineModeRange = atof(field);

        // Vertical units
        if (GetField(pData, field, BaseIndex + 7, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'F') || (field[0] == 'M') || (field[0] == 'Y'));
            pGolfTournament->HUnits = field[0];

            if (GetField(pData, field, BaseIndex + 8, FIELD_LENGTH))
            {
                pGolfTournament->HModeId[0] = field[0];
                pGolfTournament->HModeId[1] = field[1];
                *pFlags |= LAFLAG_GOLFTOURNAMENT_HRANGE;
            }
        }
    }
}

VOID CfxLaserAtlantaParser::ProcessLA1KC(CHAR *pData, LASERATLANTA_LA1KC *pla1kc)
{
    // BUGBUG: remove these 2 lines
    //CHAR szSentence[] = "HV,69.1,F,212.9,D,21.4,D,74.3,F";
    //pData = szSentence;

    // Initialize structure
    CHAR field[FIELD_LENGTH] = "";
    memset(pla1kc, 0, sizeof(LASERATLANTA_LA1KC));

    // Range 
    if (GetField(pData, field, 7, FIELD_LENGTH))
    {
        pla1kc->Range.Range = atof(field);

        // Range units
        if (GetField(pData, field, 8, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'F') || (field[0] == 'M') || (field[0] == 'Y'));
            pla1kc->Range.Units = field[0];
            pla1kc->Flags |= LAFLAG_RANGE;
        }
    }
    
    pla1kc->TimeStamp = FxGetTickCount();
}

VOID CfxLaserAtlantaParser::ProcessLA1KD(CHAR *pData, LASERATLANTA_LA1KD *pla1kd)
{
    /*#define T_POLAR     "0,249.7,F,,245.6,D,M,84.1,D,,91.9,D,,,,,04/01/20,10:54:38.69,,5,0,6.3,V,249.7,F,FM,48,72"
    #define T_CARTESIAN "1,249.7,F,,-226.14,F,-102.58,F,-25.79,F,248.32,F,-25.79,F,,,,04/01/20,10:54:38.69,,5,0,6.2,V,249.7,F,FM,48,72"
    #define T_GOLF      "2,249.7,F,,59.4,D,D,-29.72,F,Vm,218.97,F,Hm,,,,04/01/20,10:54:38.69,,5,0,6.2,V,249.7,F,FM,48,72"
    #define T_BOTH      "3,249.7,F,,59.4,D,D,-29.72,F,Vm,218.97,F,Hm,-226.14,F,-102.58,F,-25.79,F,248.32,F,-25.79,F,,,,04/01/20,10:54:38.69,,5,0,6.2,V,249.7,F,FM,48,72"

    CHAR szSentence[] = T_POLAR;
    pData = szSentence;*/
    
    CHAR field[FIELD_LENGTH];
    memset(pla1kd, 0, sizeof(LASERATLANTA_LA1KD));

    // Id packet
    if (GetField(pData, field, 0, FIELD_LENGTH))
    {
        pla1kd->PositionId = (LA1KD_ID_PACKET)(field[0] - '0');
    }

    // Range packet
    if (GetField(pData, field, 1, FIELD_LENGTH))
    {
        pla1kd->Range.Range = atof(field);

        // Range units
        if (GetField(pData, field, 2, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'F') || (field[0] == 'M') || (field[0] == 'Y') || (field[0] == '2'));
            pla1kd->Range.Units = field[0];
            pla1kd->Flags |= LAFLAG_RANGE;
        }
    }

    UINT baseIndex = 4;

    // Position packet
    switch (pla1kd->PositionId)
    {
    case ladPolar:
        ParsePolar(pData, baseIndex, &pla1kd->Polar, &pla1kd->Flags); 
        baseIndex += 9;
        break;
    
    case ladCartesian:
        ParseCartesian(pData, baseIndex, &pla1kd->Cartesian, &pla1kd->Flags); 
        baseIndex += 10;
        break;

    case ladGolfTournament:
        ParseGolfTournament(pData, baseIndex, &pla1kd->GolfTournament, &pla1kd->Flags);
        baseIndex += 9;
        break;

    case ladBoth:
        ParseGolfTournament(pData, baseIndex, &pla1kd->GolfTournament, &pla1kd->Flags);
        baseIndex += 9;
        ParseCartesian(pData, baseIndex, &pla1kd->Cartesian, &pla1kd->Flags);
        baseIndex += 10;
        break;
    }

    // Speed packet
    if (GetField(pData, field, baseIndex, FIELD_LENGTH))
    {
        pla1kd->Speed.Speed = atof(field);

        // Speed units: M=mph, K=kph, N=knots
        if (GetField(pData, field, baseIndex + 1, FIELD_LENGTH))
        {
            FX_ASSERT((field[0] == 'M') || (field[0] == 'K') || (field[0] == 'N'));
            pla1kd->Speed.Units = field[0];

            if (GetField(pData, field, baseIndex + 2, FIELD_LENGTH))
            {
                FX_ASSERT((field[0] == 'M') || (field[0] == 'P'));
                pla1kd->Speed.Mode = field[0];
            }

            pla1kd->Flags |= LAFLAG_SPEED;
        }
    }

    pla1kd->TimeStamp = FxGetTickCount();
}

VOID CfxLaserAtlantaParser::ProcessPLTITHV(CHAR *pData, TRUPULSE_PLTITHV *ppltithv)
{
    CHAR field[FIELD_LENGTH] = "";
    memset(ppltithv, 0, sizeof(TRUPULSE_PLTITHV));

    // HDvalue
    if (GetField(pData, field, 1, FIELD_LENGTH))
    {
        ppltithv->HDvalue = atof(field);

        // Units
        if (GetField(pData, field, 2, FIELD_LENGTH))
        {
            if ((field[0] != 'F') && (field[0] != 'M') && (field[0] != 'Y') &&
                (field[0] != 'f') && (field[0] != 'm') && (field[0] != 'y')) 
            {
                goto baddata;
            }

            ppltithv->HDunits = field[0];
            ppltithv->Flags |= TPFLAG_HORIZONTAL_DISTANCE;
        }
    }

    // Bearing
    if (GetField(pData, field, 3, FIELD_LENGTH))
    {
        ppltithv->AZvalue = atof(field);

        // Units: must be in degrees
        if (GetField(pData, field, 4, FIELD_LENGTH))
        {
            if ((field[0] != 'D') && (field[0] != 'd')) 
            {
                goto baddata;
            }
            
            ppltithv->AZunits = field[0];
            ppltithv->Flags |= TPFLAG_AZIMUTH;
        }
    }

    // Inclination
    if (GetField(pData, field, 5, FIELD_LENGTH))
    {
        ppltithv->INCvalue = atof(field);

        // Units: must be in degrees
        if (GetField(pData, field, 6, FIELD_LENGTH))
        {
            if ((field[0] != 'D') && (field[0] != 'd')) 
            {
                goto baddata;
            }

            ppltithv->INCunits = field[0];
            ppltithv->Flags |= TPFLAG_INCLINATION;
        }
    }


    // Slope distance
    if (GetField(pData, field, 7, FIELD_LENGTH))
    {
        ppltithv->SDvalue = atof(field);

        // Units
        if (GetField(pData, field, 8, FIELD_LENGTH))
        {
            if ((field[0] != 'F') && (field[0] != 'M') && (field[0] != 'Y') &&
                (field[0] != 'f') && (field[0] != 'm') && (field[0] != 'y')) 
            {
                goto baddata;
            }
            ppltithv->SDunits = field[0];
            ppltithv->Flags |= TPFLAG_SLOPE_DISTANCE;
        }
    }

    ppltithv->TimeStamp = FxGetTickCount();
    return;

  baddata:

    Reset();
}

VOID CfxLaserAtlantaParser::ProcessPLTITHT(CHAR *pData, TRUPULSE_PLTITHT *ppltitht)
{
    CHAR field[FIELD_LENGTH] = "";
    memset(ppltitht, 0, sizeof(TRUPULSE_PLTITHT));

    // Height
    if (GetField(pData, field, 1, FIELD_LENGTH))
    {
        ppltitht->HTvalue = atof(field);

        // Height units
        if (GetField(pData, field, 2, FIELD_LENGTH))
        {
            if ((field[0] != 'F') && (field[0] != 'M') && (field[0] != 'Y') &&
                (field[0] != 'f') && (field[0] != 'm') && (field[0] != 'y')) goto baddata;
            ppltitht->HTunits = field[0];
            ppltitht->Flags |= TPFLAG_CALCULATED_HEIGHT;
        }
    }

    ppltitht->TimeStamp = FxGetTickCount();
    return;

  baddata:

    Reset();
}

BOOL CfxLaserAtlantaParser::GetPingStream(BYTE **ppBuffer, UINT *pLength)
{
    CfxRangeFinderParser::GetPingStream(ppBuffer, pLength);

    CHAR szFire[] = "KF\n\r";

    *pLength = sizeof(szFire);
    *ppBuffer = (BYTE *)MEM_ALLOC(*pLength);
    memcpy(*ppBuffer, &szFire[0], *pLength);

    return TRUE;
}
