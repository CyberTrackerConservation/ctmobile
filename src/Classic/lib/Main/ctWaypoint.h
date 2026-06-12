#pragma once
#include "pch.h"

const int MAGIC_WAYPOINT = 'WAY2';

// Must be packed for compatibility with the Server
#pragma pack(push, 1) 

struct WAYPOINT
{
    int Magic = 0;
    GUID Id;
    GUID DeviceId;
    double DateCreated = 0.0;
    double DateCurrent = 0.0;
    double Latitude = 0.0;
    double Longitude = 0.0;
    double Altitude = 0.0;
    double Accuracy = 0.0;
    BYTE Deleted = 0;
};

#pragma pack(pop)
