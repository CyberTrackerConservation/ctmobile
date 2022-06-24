#pragma once
#include "pch.h"

#define CLIENT_BRAND_COMPANYA   "CyberTracker Software"
#define CLIENT_BRAND_COMPANYW   L"CyberTracker Software"
#define CLIENT_BRAND_PRODUCTA   "CyberTracker"
#define CLIENT_BRAND_PRODUCTW   L"CyberTracker"

#ifdef CLIENT_DLL
    #define CLIENT_REGISTRY_KEY L"Software\\CyberTracker Software\\CyberTracker\\Version3" 
#else
    #define CLIENT_REGISTRY_KEY L"Software\\CyberTracker"
#endif

#define CLIENT_RESOURCE_EXTENSION  ".CTS"
#define CLIENT_RESOURCE_EXTENSIONW L".CTS"
#define CLIENT_EXPORT_EXT          ".CTX"
#define CLIENT_EXPORT_EXTW         L".CTX"
#define CLIENT_EXPORT_JSON_EXT     ".JSON"
#define CLIENT_EXPORT_JSON_EXTW    L".JSON"
#define CLIENT_UPLOAD_MUTEX_NAMEW  L"Global\\CyberTrackerUploadOutgoing"

#define CLIENT_WINCE_TITLE         L"CyberTracker";            
#define CLIENT_WINCE_WNDCLASS      L"CT3WNDCLASS";      
#define CLIENT_WINCE_INSTALLKEY    L"Software\\Apps\\CyberTracker Client"
