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
#include "fxUtils.h"

class CfxGPSParser
{
protected:
    UINT _lastDataTimeStamp;
public:
    CfxGPSParser() 
    { 
        Reset(); 
    }

    virtual VOID Reset()
    {
        ResetTimeOut();
    }
    
    virtual VOID ResetTimeOut()
    {
        _lastDataTimeStamp = FxGetTickCount();
    }

    virtual VOID ParseBuffer(CHAR *pBuffer, UINT Length) 
    {
        if (Length > 0)
        {
            _lastDataTimeStamp = FxGetTickCount();
        }
    }
    
    virtual BOOL GetGPS(FXGPS *pGPS, UINT MaxAgeInSeconds)
    {
        return (FxGetTickCount() - _lastDataTimeStamp < FxGetTicksPerSecond() * MaxAgeInSeconds); 
    }

    virtual BOOL GetPingStream(BYTE **ppBuffer, UINT *pLength)
    {
        *ppBuffer = NULL;
        *pLength = 0;
        return FALSE;
    }
};

