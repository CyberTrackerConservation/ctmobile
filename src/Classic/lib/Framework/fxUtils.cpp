#include "fxMath.h"
#include "fxUtils.h"
#include "zlib.h"

char *AllocString(const char *pValue)
{
    if (pValue)
    {
        char *value = (char *)MEM_ALLOC(strlen(pValue) + 1);
        strcpy(value, pValue);
        return value;
    }
    else
    {
        return nullptr;
    }
}

char *AllocMaxPath(const char *pSource)
{
	char *result;
	int len = MAX_PATH * sizeof(char);

	result = (char *)MEM_ALLOC(len);
	if (result != nullptr)
	{
		memset(result, 0, len);
	}

	if (pSource != nullptr)
	{
        FX_ASSERT(strlen(pSource) < MAX_PATH);
		strcpy(result, pSource);
	}

	return result;
}

void AppendSlash(char *pPath)
{
	if (pPath)
    {
	    int len = strlen(pPath);
	    if ((len > 0) && (pPath[len-1] != PATH_SLASH_CHAR)) 
	    {
            char pathSlash[2] = {0};
            pathSlash[0] = PATH_SLASH_CHAR;
		    strcat(pPath, pathSlash);
	    }
    }
}

void TrimSlash(char *pPath)
{
	if (pPath)
    {
	    int len = strlen(pPath);
	    if ((len > 0) && (pPath[len-1] == PATH_SLASH_CHAR)) 
	    {
		    pPath[len-1] = '\0';
	    }
    }
}

int StrLenUtf8(const char *pValue) 
{
    int i = 0, j = 0;

    if (pValue == nullptr)
    {
        return 0;
    }
  
    while (pValue[i]) 
    {
        if ((pValue[i] & 0xc0) != 0x80)
        {
            j++;
        }

        i++;
    }

    return j;
}

void Log(const char *pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);

    char buffer[4096];
    vsprintf(buffer, pFormat, args);
    FxOutputDebugString(buffer);
    va_end(args);
}

bool IsWhiteSpace(char Value)
{
    return (Value == '\n') || (Value == '\r') || (Value == ' ') || (Value == '\t');
}

void TrimString(char *pValue)
{
    if (!pValue || !*pValue) 
    {
        return;
    }

    char *tail = pValue + strlen(pValue) - 1;
    
    char *head = pValue;
    while (head < tail)
    {
        if (IsWhiteSpace(*head))
        {
            head++;
        }
        else break;
    }

    while (tail > head)
    {
        if (IsWhiteSpace(*tail))
        {
            tail--;
        }
        else break;
    }
    tail++;

    char *curr = pValue;
    while (head < tail)
    {
        *curr = *head;
        curr++;
        head++;
    }
    *curr = '\0';

    if (pValue && IsWhiteSpace(*pValue))
    {
        *pValue = '\0';
    }
}

void Swap(int *A, int *B)
{
    int t = *A;
    *A = *B;
    *B = t;
}

void Swap(INT16 *A, INT16 *B)
{
    INT16 t = *A;
    *A = *B;
    *B = t;
}

void Swap(double *A, double *B)
{
    double t = *A;
    *A = *B;
    *B = t;
}

bool IsRectEmpty(const FXRECT *pRect)
{
    return pRect->Right < pRect->Left || pRect->Bottom < pRect->Top;
}

bool IntersectRect(FXRECT *pDst, const FXRECT *pSrc1, const FXRECT *pSrc2)
{
    #define MMAX(a, b) ((a) > (b) ? (a) : (b))
    #define MMIN(a, b) ((a) < (b) ? (a) : (b))

    if ((pSrc1->Left > pSrc2->Right) || (pSrc2->Left > pSrc1->Right) ||
        (pSrc1->Top > pSrc2->Bottom) || (pSrc2->Top > pSrc1->Bottom))
    {
        memset(pDst, 0, sizeof(FXRECT));
        return false;
    }

    pDst->Left   = MMAX(pSrc1->Left, pSrc2->Left );
    pDst->Right  = MMIN(pSrc1->Right, pSrc2->Right );
    pDst->Top    = MMAX(pSrc1->Top, pSrc2->Top );
    pDst->Bottom = MMIN(pSrc1->Bottom, pSrc2->Bottom );

    return true;
}

void OffsetRect(FXRECT *pRect, int Dx, int Dy)
{
    pRect->Left += Dx;
    pRect->Top  += Dy;
    pRect->Right += Dx;
    pRect->Bottom += Dy;
}

void InflateRect(FXRECT *pRect, int Dx, int Dy)
{
    pRect->Left -= Dx;
    pRect->Top -= Dy;
    pRect->Right += Dx;
    pRect->Bottom += Dy;
}

bool PtInRect(const FXRECT *pRect, const FXPOINT *pPoint)
{
    return (pPoint->X >= pRect->Left) && (pPoint->X < pRect->Right) &&
           (pPoint->Y >= pRect->Top) && (pPoint->Y < pRect->Bottom);
}

bool PtInRect(const FXRECT *pRect, const int X, const int Y)
{
    return (X >= pRect->Left) && (X < pRect->Right) &&
           (Y >= pRect->Top) && (Y < pRect->Bottom);
}

bool CompareRect(const FXRECT *pRect1, const FXRECT *pRect2)
{
    return memcmp(pRect1, pRect2, sizeof(FXRECT)) == 0;
}

bool CompareGuid(const GUID *pGuid1, const GUID *pGuid2)
{
    return memcmp(pGuid1, pGuid2, sizeof(GUID)) == 0;
}

bool CompareGuid(const GUID Guid1, const GUID Guid2)
{
    return memcmp(&Guid1, &Guid2, sizeof(GUID)) == 0;
}

void InitGuid(GUID *pGuid)
{
    memset(pGuid, 0, sizeof(GUID));
}

void InitGuid(GUID *pDstGuid, const GUID *pSrcGuid)
{
    memmove(pDstGuid, pSrcGuid, sizeof(GUID));
}

bool IsNullGuid(GUID *pGuid)
{
    return CompareGuid(pGuid, &GUID_0);
}

bool CompareXGuid(const XGUID *pGuid1, const XGUID *pGuid2)
{
    return memcmp(pGuid1, pGuid2, sizeof(XGUID)) == 0;
}

void InitXGuid(XGUID *pGuid)
{
    memset(pGuid, 0, sizeof(XGUID));
}

bool IsNullXGuid(const XGUID *pGuid)
{
    // NULL_GUID is big enough to handle both GUID and int sizes of XGUID
    return (memcmp(pGuid, &GUID_0, sizeof(XGUID)) == 0);
}

GUID ConstructGuid(XGUID Id)
{
    GUID result = {0};
    memcpy(&result, &Id, sizeof(Id));
    return result;
}

void InitGpsAccuracy(FXGPS_ACCURACY *pAccuracy)
{
    pAccuracy->DilutionOfPrecision = GPS_DEFAULT_ACCURACY;
    pAccuracy->MaximumSpeed = 10000;
}

void GetBitmapRect(FXBITMAPRESOURCE *Bitmap, int Width, int Height, bool Center, bool Stretch, bool Proportional, FXRECT *pTargetRect)
{
    int w = Bitmap->Width;
    int h = Bitmap->Height;
    int cw = Width;
    int ch = Height;

    float xyaspect;

    if (Stretch || (Proportional && ((w > cw) || (h > ch))))
    { 
        if (Proportional && (w > 0) && (h > 0))
        {
            xyaspect = (float)w / (float)h;
            if (w > h)
            {
                w = cw;
                h = (int)((float)cw / xyaspect);
                if (h > ch) // woops, too big
                {
                    h = ch;
                    w = (int)((float)ch * xyaspect);
                }
            }
            else
            {
                h = ch;
                w = (int)((float)ch * xyaspect);
                if (w > cw) // woops, too big
                {
                    w = cw;
                    h = (int)((float)cw / xyaspect);
                }
            }
        }
        else
        {
            w = cw;
            h = ch;
        }
    }

    pTargetRect->Left = 0;
    pTargetRect->Top = 0;
    pTargetRect->Right = max(0, (int)w-1);
    pTargetRect->Bottom = max(0, (int)h-1);

    if (Center)
    {
        OffsetRect(pTargetRect, (int)((cw - w) / 2), (int)((ch - h) / 2));
    }
}

void NumberToText(int Value, BYTE Digits, BYTE Decimals, BYTE Fraction, bool Negative, char *pOutValue, bool Spaces)
{
    int i;
    if (Fraction > 1)
    {
        double value = Value;
        for (i=0; i < Decimals; i++, value/=10) {}

        int intPart = (int)value;
        int fraPart = (int)((value - (double)intPart) * Fraction);

        itoa(intPart, pOutValue, 10);

        char fraText1[32];
        itoa(fraPart, fraText1, 10);

        char fraText2[32];
        itoa(Fraction, fraText2, 10);

        if ((fraPart > 0) && (fraPart < Fraction))
        {
            strcat(pOutValue, " ");
            strcat(pOutValue, fraText1);
            strcat(pOutValue, "/");
            strcat(pOutValue, fraText2);
        }
    }
    else
    {
        int divisor = 1;
        int runningCount = 0;
        int actualDigits = 0;
        int v = Value;
        while (v > 0)
        {
            actualDigits++;
            v /= 10;
        }

        actualDigits = max(Digits, actualDigits);            

        char digit[2] = "0";
        strcpy(pOutValue, "");

        for (i=0; i<actualDigits-1; i++, divisor*=10) {}

        for (i=0; i<actualDigits; i++)
        {
            int value = (Value - runningCount) / divisor;
            
            runningCount += (value * divisor);
            divisor /= 10;

            if (Spaces && (i > 0))
            {
                strcat(pOutValue, " ");
            }

            digit[0] = '0' + (char)value;
            strcat(pOutValue, digit);

            if ((i == actualDigits - Decimals - 1) && (i < actualDigits-1))
            {
                if (Spaces)
                {
                    strcat(pOutValue, " ");
                }
                strcat(pOutValue, ".");
            }
        }

        if (!Spaces)
        {
            // Skip leading zeros
            char *p = pOutValue;
            for (i=0; i<(int)strlen(pOutValue); i++)
            {
                if (pOutValue[i] != '0') break;
                p++;
            }

            // So we get "0."
            if (*p == '.')
            {
                p--;
            }

            if (strlen(p) == 0)
            {
                strcat(p, "0");
            }

            if (strchr(p, '.') == nullptr)
            {
                strcat(p, ".");
            }
            
            strcpy(pOutValue, p);
        }
    }

    if (Negative)
    {
        char *p = pOutValue + strlen(pOutValue) + 1;
        while (p > pOutValue)
        {
            *p = *(p-1);
            p--;
        }
        *p = '-';
    }
}

typedef BYTE DayTable[12];
const DayTable MonthDays[2] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
    };
const int DateDelta   = 693594;
const int HoursPerDay = 24;
const int MinsPerHour = 60;
const int SecsPerMin  = 60;
const int MSecsPerSec = 1000;
const int MinsPerDay  = HoursPerDay * MinsPerHour;
const int SecsPerDay  = MinsPerDay * SecsPerMin;
const int MSecsPerDay = SecsPerDay * MSecsPerSec;


bool IsLeapYear(int Year)
{
    return (Year % 4 == 0) && ((Year % 100 != 0) || (Year % 400 == 0));
}

double EncodeDateTime(const FXDATETIME *pDateTime)
{
    const DayTable *dayTable = &MonthDays[IsLeapYear(pDateTime->Year)];

    if ((pDateTime->Year >= 1) && (pDateTime->Year <= 9999) && (pDateTime->Month >= 1) && (pDateTime->Month <= 12) &&
        (pDateTime->Day >= 1) && (pDateTime->Day <= (*dayTable)[pDateTime->Month-1]))
    {
        // Encode Date
        int day = pDateTime->Day;
        for (int i=1; i<pDateTime->Month; i++)
        {
            day = day + (*dayTable)[i-1];
        }

        int days = pDateTime->Year - 1;
        days = days * 365 + days/4 - days/100 + days/400 + day - DateDelta;
 
        // Encode time
		UINT16 h  = min(pDateTime->Hour, (UINT16)HoursPerDay-1);
		UINT16 m  = min(pDateTime->Minute, (UINT16)MinsPerHour-1);
		UINT16 s  = min(pDateTime->Second, (UINT16)SecsPerMin-1);
		UINT16 ms = min(pDateTime->Milliseconds, (UINT16)MSecsPerSec-1);

        return (double)days + EncodeTime(h, m, s, ms);
    }
	else
	{
		//FX_ASSERT(false);    // Bad dates just fail silently.
		return 0;
	}
}

double EncodeTime(const UINT16 Hours, const UINT16 Minutes, const UINT16 Seconds, const UINT16 Milliseconds)
{
    return (double)((int)Hours * (MinsPerHour * SecsPerMin * MSecsPerSec) + 
                    (int)Minutes * (SecsPerMin * MSecsPerSec) +
                    (int)Seconds * MSecsPerSec +
                    (int)Milliseconds) / (double)MSecsPerDay;
}

void DecodeDate(double EncodedDate, FXDATETIME *pDateTime)
{
    const double DateDelta = 693594;

    memset(pDateTime, 0, sizeof(FXDATETIME));

    if (EncodedDate > -DateDelta)  
    {
        int di = (int)EncodedDate;
        double j = (((double)di + 693900) * 4) - 1;
        int ly = (int)(j / 146097);
        
        j = j - 146097 * ly;
        int ld = (int)(j / 4);
        j  = (ld * 4 + 3) / 1461;
        
        ld = (int)((ld * 4 + 7 - 1461 * j) / 4);
        double lm = (5 * ld - 3) / 153;
        ld = (int)((5 * ld + 2 - 153 * lm) / 5);
        ly = 100 * ly + (int)j;
        if (lm < 10) 
        {
            lm += 3;
        }
        else
        {
            lm -= 9;
            ly ++;
        }
    
        pDateTime->Year = (UINT16)ly;
        pDateTime->Month = (UINT16)lm;
        pDateTime->Day = (UINT16)ld;
    }
}

void DecodeDateTime(double EncodedDateTime, FXDATETIME *pDateTime)
{
    if (EncodedDateTime == 0.0)
    {
        memset(pDateTime, 0, sizeof(FXDATETIME));
        return;
    }

    DecodeDate(EncodedDateTime, pDateTime);

    int time;
    time = (int)((EncodedDateTime - (int)EncodedDateTime) * MSecsPerDay);

    int minCount, msecCount;
    minCount = time / (SecsPerMin * MSecsPerSec);
    msecCount = time % (SecsPerMin * MSecsPerSec);

    pDateTime->Hour = (UINT16)(minCount / MinsPerHour);
    pDateTime->Minute = (UINT16)(minCount % MinsPerHour);
    pDateTime->Second = (UINT16)(msecCount / MSecsPerSec);
    pDateTime->Milliseconds = (UINT16)(msecCount % MSecsPerSec);
}

double CalcDistance(double Lat1, double Lon1, double Lat2, double Lon2)
{
    if ((Lat1 == Lat2) && (Lon1 == Lon2))
    {
        return 0;
    }

    double rLat1 = DEG2RAD(Lat1);
    double rLon1 = DEG2RAD(Lon1);
    double rLat2 = DEG2RAD(Lat2);
    double rLon2 = DEG2RAD(Lon2);
    
    double dLon = rLon2 - rLon1;
    double dLat = rLat2 - rLat1;

    double dMid = sin(rLat1) * sin(rLat2) + cos(rLat1) * cos(rLat2) * cos(dLon);
    double result = 0.0;

    if ((dMid >= -1.0) && (dMid <= 1.0))
    {
        result = acos(dMid);
        if (result != result) //isnan(result))
        {
            result = 0;
        }
    }

    return result;
}

double CalcDistanceKm(double Lat1, double Lon1, double Lat2, double Lon2)
{
    return 6378.7 * CalcDistance(Lat1, Lon1, Lat2, Lon2);
}

double CalcDistanceMi(double Lat1, double Lon1, double Lat2, double Lon2)
{
    return 3963.0 * CalcDistance(Lat1, Lon1, Lat2, Lon2);
}

double CalcDistanceNaMi(double Lat1, double Lon1, double Lat2, double Lon2)
{
    return 3437.74667 * CalcDistance(Lat1, Lon1, Lat2, Lon2);
}

int CalcHeading(double Lat1, double Lon1, double Lat2, double Lon2)
{
    double rLat1 = DEG2RAD(Lat1);
    double rLon1 = DEG2RAD(Lon1);
    double rLat2 = DEG2RAD(Lat2);
    double rLon2 = DEG2RAD(Lon2);
    
    double dLon = rLon2 - rLon1;
    double dLat = rLat2 - rLat1;

    double y = sin(dLon) * cos(rLat2);
    double x = cos(rLat1) * sin(rLat2) - sin(rLat1) * cos(rLat2) * cos(dLon);

    int heading;

    if (y > 0)
    {
        if (x > 0) 
        {
            heading = (int)RAD2DEG(atan(y / x));
        }
        else if (x < 0) 
        {
            heading = 180 - (int)RAD2DEG(atan(-y / x));
        }
        else if (x == 0) 
        {
            heading = 90;
        }
    }
    else if (y < 0)
    {
        if (x > 0)
        {
            heading = (int)RAD2DEG(-atan(-y / x));
        }
        else if (x < 0)
        {
            heading = (int)RAD2DEG(atan(y / x)) - 180;
        }
        else if (x == 0) 
        {
            heading = 270;
        }
    }
    else if (y == 0)
    {
        if (x > 0)
        {
            heading = 0;
        }
        else if (x < 0) 
        {
            heading = 180;
        }
        else if (x == 0)
        {
            heading = 0; //[the 2 points are the same]
        }
    }

    return (heading + 360) % 360;
}

double KnotsToKmHour(double Knots)
{
	return Knots * 1.88;
}

char *replace(const char *str, const char *pSrc, const char *pDst) 
{ 
    char *ret, *r; 
    const char *p, *q; 
    int srclen = strlen(pSrc); 
    int count, retlen, dstlen = strlen(pDst); 

    if (srclen != dstlen) 
    { 
        for (count = 0, p = str; (q = strstr(p, pSrc)) != nullptr; p = q + srclen) 
        {
            count++; 
        }
        retlen = p - str + strlen(p) + count * (dstlen - srclen); 
    } 
    else 
    {
        retlen = strlen(str); 
    }

    ret = (char *)MEM_ALLOC(retlen + 1); 
    if (ret == nullptr)
    {
        return nullptr;
    }

    for (r = ret, p = str; (q = strstr(p, pSrc)) != nullptr; p = q + srclen) 
    { 
        int l = q - p; 
        memcpy(r, p, l); 
        r += l; 
        memcpy(r, pDst, dstlen); 
        r += dstlen; 
    } 

    strcpy(r, p); 

    return ret; 
} 

bool StringReplace(char **pString, const char *pFrom, const char *pTo)
{
    if (*pString == nullptr)
    {
        return true;
    }

    char *newStr;
    newStr = replace(*pString, pFrom, pTo);
    if (newStr == nullptr)
    {
        return false;
    }

    if (newStr == *pString)
    {
        return true;
    }

    MEM_FREE(*pString);
    *pString = newStr;

    return true;
}

bool TestGPS(FXGPS *pGPS, double Accuracy)
{
    return (pGPS->State == FXDEVICE_STATE::Connected) && 
           (pGPS->Position.Quality != FXGPS_QUALITY::None) &&
           (pGPS->Position.Accuracy <= Accuracy);
}

bool TestGPS(FXGPS *pGPS, FXGPS_ACCURACY *pAccuracy)
{
    return TestGPS(pGPS, pAccuracy->DilutionOfPrecision) &&
           (pGPS->Position.Speed < pAccuracy->MaximumSpeed);
}

bool TestGPS(FXGPS_POSITION *pGPS, double Accuracy)
{
    return (pGPS->Quality != FXGPS_QUALITY::None) &&
           (pGPS->Accuracy <= Accuracy) &&
           ((pGPS->Accuracy > 0) || (pGPS->Quality == FXGPS_QUALITY::User2D));
}

bool TestGPS(FXGPS_POSITION *pGPS, FXGPS_ACCURACY *pAccuracy)
{
    return TestGPS(pGPS, pAccuracy->DilutionOfPrecision) &&
           (pGPS->Speed < pAccuracy->MaximumSpeed);
}

double EncodeDouble(int IValue, int IPoint, bool INegative, double OutMin, double OutMax)
{
    double v = IValue;

    for (int i=0; i<IPoint; i++, v/=10) {}

    v = max(OutMin, v);
    v = min(OutMax, v);

    return INegative ? -v : v;
}

void DecodeDouble(double Value, BYTE Decimals, int *pIValue, int *pIPoint, bool *pINegative, bool Pack)
{
    int v = 0;
    int p = -1;
    int i = 0;
    double value = 0;

    if (Value == 0) 
    {
        goto Done;
    }

    value = FxAbs(Value);
    for (i=0; i<Decimals; i++, value*=10) {}
    value += 0.5;   // round properly

    v = (int)value;
    p = Decimals;

    if (Pack && (p > 0))
    {
        while ((v > 0) && (v % 10 == 0))
        {
            v /= 10;
            p--;
        }
    }

    if (p == 0)
    {
        p = -1;
    }

Done:

    *pIValue = v;
    *pIPoint = p;
    *pINegative = (Value < 0);
}

void InitCOM(BYTE *pPortId, FX_COMPORT *pComPort)
{
    *pPortId = 1;
    pComPort->Baud = CBR_19200;
    pComPort->Parity = NOPARITY;
    pComPort->Data = DATABITS_8;
    pComPort->Stop = ONESTOPBIT;
    pComPort->FlowControl = false;
}

char *AllocGuidName(GUID *pGuid, char *pExtension)
{
    char *result = nullptr;
    int len = 40 + ((pExtension != nullptr) ? strlen(pExtension) : 0);

    result = (char *)MEM_ALLOC(len);
    if (result == nullptr)
    {
        goto cleanup;
    }

    sprintf(result, 
            "%08lx%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x%s",
            pGuid->Data1, pGuid->Data2, pGuid->Data3, 
            pGuid->Data4[0], pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3], 
            pGuid->Data4[4], pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7],
            pExtension ? pExtension : ""); 

cleanup:

    return result;
}

bool ClearDirectory(const char *pPath)
{
    bool result = false;

    FXFILELIST fileList;
    char *path = AllocString(pPath);
    if (path == nullptr)
    {
        Log("Out of memory");
    	goto cleanup;
    }

    TrimSlash(path);

    if (FxBuildFileList(path, "*", &fileList))
    {
    	for (int i = 0; i < fileList.GetCount(); i++)
    	{
    		FxDeleteFile(fileList.GetPtr(i)->FullPath);
    	}
    }

    MEM_FREE(path);
    result = true;

  cleanup:

    return result;
}

const char *GetFileExtension(const char *pFileName)
{
    const char *pDot = strrchr(pFileName, '.');
    if (!pDot)
    {
        return nullptr;
    }
    else
    {
        return pDot;
    }
}

bool ChangeFileExtension(char *pFileName, char *pNewExtension)
{
    char *pDot = strrchr(pFileName, '.');
    if (!pDot) return false;

    if (pNewExtension)
    {
        strcpy(pDot + 1, pNewExtension);
    }
    else
    {
        *pDot = L'\0';
    }

    return true;
}

char *ExtractFileName(char *pFullFileName)
{
    char *pTemp = strrchr(pFullFileName, PATH_SLASH_CHAR);
    if (pTemp)
    {
        return pTemp + 1;
    }
    else
    {
        return nullptr;
    }
}

int FileListGetTotalSize(FXFILELIST *pFileList)
{
    int result = 0;
    for (int i = 0; i < pFileList->GetCount(); i++)
    {
        FXFILE file = pFileList->Get(i);
        result += file.Size;
    }

    return result;
}

void FileListClear(FXFILELIST *pFileList)
{
    for (int i = 0; i < pFileList->GetCount(); i++)
    {
        FXFILE file = pFileList->Get(i);
        MEM_FREE(file.Name);
        MEM_FREE(file.FullPath);
        if (file.FileHandle != 0)
        {
            FileClose(file.FileHandle);
        }
    }

    pFileList->Clear();
}

bool FileListRemove(FXFILELIST *pFileList, char *pFileName)
{
    bool Result = false;

    for (int i = 0; i < pFileList->GetCount(); i++)
    {
        FXFILE file = pFileList->Get(i);

        if ((stricmp(pFileName, file.FullPath) == 0) ||
            (stricmp(pFileName, file.Name) == 0))
        {
            MEM_FREE(file.Name);
            MEM_FREE(file.FullPath);
            if (file.FileHandle != 0)
            {
                FileClose(file.FileHandle);
            }

            pFileList->Delete(i);

            Result = true;
            break;
        }
    }

    return Result;
}

HANDLE FileOpen(const char *pFilename, const char *pMode)
{
	return (HANDLE)fopen(pFilename, pMode);
}

void FileClose(HANDLE Handle)
{
    if (Handle) 
    {
        fflush((FILE *)Handle);
        fclose((FILE *)Handle);
    }
}

bool FileRead(HANDLE Handle, void *pBuffer, int Length)
{
    return fread(pBuffer, 1, Length, (FILE *)Handle) == Length;
}

bool FileSeek(HANDLE Handle, int Position)
{
    return fseek((FILE *)Handle, Position, SEEK_SET) == 0;
}

bool FileWrite(HANDLE Handle, void *pBuffer, int Length)
{
    return fwrite(pBuffer, 1, Length, (FILE *)Handle) == Length;
}

int FileGetSize(HANDLE Handle)
{
    int result = 0;
    FILE *f = (FILE *)Handle;
    long original = ftell(f);

    if (fseek(f, 0, SEEK_END) == 0)
    {
        result = ftell(f);
        fseek(f, original, SEEK_SET);
    }

    return result;
}

int FileGetLastError(HANDLE Handle)
{
    return ferror((FILE *)Handle);
}

/*******************************/
/* ASCII TO BASE 64 (UUENCODE) */
/*******************************/

char t_64[64] = 
{
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

/*********************************************************/
/* convert a three character string to four char encoded */
/*********************************************************/

int a_to_b(char *a, int a_len, char *b) /* a[3], b[4] */
{
    int  len;
    int  i;
    long b32;
    char c[4];

    /*****************************************/
    /* convert string in "a" to 32 bit value */
    /*****************************************/
    
    if (a_len > 3)
        a_len = 3;

    if (a_len < 1)
        return(0);

    b32 = 0;

    for (i=0; i<a_len; ++i)
    {
        b32 <<= 8;
        b32 |= (a[i] & 0xff);
    }
    
    while(i < 3)
    {
        b32 <<= 8;
        ++i;
    }

    /****************************************/
    /* fill in "c" with 6 bit chunks of "a" */
    /****************************************/

    c[3] = b32 & 0x3f;
    b32 >>= 6;
    c[2] = b32 & 0x3f;
    b32 >>= 6;
    c[1] = b32 & 0x3f;
    b32 >>= 6;
    c[0] = b32 & 0x3f;


    /********************************************/
    /* fill in "b" with encoded values from "c" */
    /********************************************/

    len = 2;

    b[0] = t_64[c[0]];        /* one character encoding format */
    b[1] = t_64[c[1]];
    b[2] = '=';
    b[3] = '=';
    if (a_len > 1)            /* update to two character format */
    {
        b[2] = t_64[c[2]];
        ++len;
    }
    if (a_len > 2)            /* update to three character format */
    {
        b[3] = t_64[c[3]];
        ++len;
    }

    return(len);
}

/******************************************/
/* endcode a string in base 64 (UUENCODE) */
/******************************************/

char *AsciiToBase64(char *pSource)
{
    char B[4] = {0};
    int SourceLen = strlen(pSource);
    CfxStream stream;
   
    for (int i=0; i<(int)SourceLen; i+=3)
    {
        int len = a_to_b(&pSource[i], SourceLen - i, B);

        if (len > 0)
        {
            stream.Write(&B, 4);
        }

        if (len < 4)
        {
            break;
        }
    }

    B[0] = '\0';
    stream.Write(&B, 1);

    return AllocString((char *)stream.GetMemory());        
}

bool Base64Encode(char *pSource, int SourceLen, CfxFileStream &Stream)
{
    char B[4] = {0};
    CfxStream buffer;
    buffer.SetCapacity(256 * 1024);
   
    for (int i=0; i<(int)SourceLen; i+=3)
    {
        int len = a_to_b(&pSource[i], SourceLen - i, B);

        if (len > 0)
        {
            buffer.Write(&B, 4);
        }

        if (len < 4)
        {
            break;
        }

        if (buffer.GetPosition() > 256 * 1024 - 16)
        {
            if (!Stream.Write(buffer.GetMemory(), buffer.GetPosition()))
            {
                return false;
            }

            buffer.SetPosition(0);
        }
    }

    if (buffer.GetPosition() > 0)
    {
        if (!Stream.Write(buffer.GetMemory(), buffer.GetPosition()))
        {
            return false;
        }
    }

    return true;
}

void FixJsonString(QString &String)
{
    if (String.isEmpty()) return;

    QByteArray utf8 = String.toUtf8();
    const char *src = utf8.constData();
    int len = utf8.size();

    char c = 0;
    int i;
    CfxStream sb;

    for (i = 0; i < len; i += 1)
    {
        c = src[i];
        if (c == '\0') break;
        switch (c)
        {
        case '\\':
        case '"':
            sb.Write("\\");
            sb.Write(&c, 1);
            break;

        case '/':
            sb.Write("\\");
            sb.Write(&c, 1);
            break;

        case '\b':
            sb.Write("\\b");
            break;

        case '\t':
            sb.Write("\\t");
            break;

        case '\n':
            sb.Write("\\n");
            break;

        case '\f':
            sb.Write("\\f");
            break;

        case '\r':
            sb.Write("\\r");
            break;

        default:
            if (c < ' ')
            {
                char number[16];
                sprintf(number, "\\u%04ud", (unsigned int)((BYTE)c));
                sb.Write(number);
            }
            else
            {
                 sb.Write(&c, 1);
            }
        }
    }

    c = 0;
    sb.Write(&c, 1);
    String = QString::fromUtf8((char *)sb.GetMemory());
}

struct comp_options
{
    comp_options() :
        m_level(7),
        m_unbuffered_decompression(false),
        m_verify_compressed_data(false),
        m_randomize_params(false),
        m_randomize_buffer_sizes(false),
        m_z_strat(Z_DEFAULT_STRATEGY),
        m_random_z_flushing(false),
        m_write_zlib_header(true),
        m_archive_test(false),
        m_write_archives(false)
    {
    }

    int m_level;
    bool m_unbuffered_decompression;
    bool m_verify_compressed_data;
    bool m_randomize_params;
    bool m_randomize_buffer_sizes;
    int m_z_strat;
    bool m_random_z_flushing;
    bool m_write_zlib_header;
    bool m_archive_test;
    bool m_write_archives;
};

static comp_options g_comp_options;
#define TDEFLTEST_COMP_INPUT_BUFFER_SIZE 1024*1024*2
#define TDEFLTEST_COMP_OUTPUT_BUFFER_SIZE 1024*1024*2
#define RND_SHR3(x)  (x ^= (x << 17), x ^= (x >> 13), x ^= (x << 5))
static float s_max_small_comp_ratio, s_max_large_comp_ratio;

bool CompressFile(const char* pSrc_filename, const char *pDst_filename)
{
    FILE *pInFile = fopen(pSrc_filename, "rb");
    if (!pInFile)
    {
        Log("Unable to read file: %s\n", pSrc_filename);
        return false;
    }

    FILE *pOutFile = fopen(pDst_filename, "wb");
    if (!pOutFile)
    {
        Log("Unable to create file: %s\n", pDst_filename);
        return false;
    }

    fseek(pInFile, 0, SEEK_END);
    UINT64 src_file_size = ftell(pInFile);
    fseek(pInFile, 0, SEEK_SET);

    // Do not write header.
    /*fputc('D', pOutFile); fputc('E', pOutFile); fputc('F', pOutFile); fputc('0', pOutFile);
    fputc(g_comp_options.m_write_zlib_header, pOutFile);

    for (int i = 0; i < 8; i++)
    fputc(static_cast<int>((src_file_size >> (i * 8)) & 0xFF), pOutFile);
    */

    int cInBufSize = TDEFLTEST_COMP_INPUT_BUFFER_SIZE;
    int cOutBufSize = TDEFLTEST_COMP_OUTPUT_BUFFER_SIZE;
    if (g_comp_options.m_randomize_buffer_sizes)
    {
        cInBufSize = 1 + (rand() % 4096);
        cOutBufSize = 1 + (rand() % 4096);
    }

    BYTE *in_file_buf = static_cast<BYTE*>(malloc(cInBufSize));
    BYTE *out_file_buf = static_cast<BYTE*>(malloc(cOutBufSize));
    if ((!in_file_buf) || (!out_file_buf))
    {
        Log("Out of memory!\n");
        free(in_file_buf);
        free(out_file_buf);
        fclose(pInFile);
        fclose(pOutFile);
        return false;
    }

    UINT64 src_bytes_left = src_file_size;

    int in_file_buf_size = 0;
    int in_file_buf_ofs = 0;

    UINT64 total_output_bytes = 0;

    int start_time = FxGetTickCount();

    z_stream zstream;
    memset(&zstream, 0, sizeof(zstream));

    int init_start_time = FxGetTickCount();
    int status = deflateInit(&zstream, g_comp_options.m_level);//, Z_DEFLATED, g_comp_options.m_write_zlib_header ? Z_DEFAULT_WINDOW_BITS : -Z_DEFAULT_WINDOW_BITS, 9, g_comp_options.m_z_strat);
    int total_init_time = FxGetTickCount() - init_start_time;

    if (status != Z_OK)
    {
        Log("Failed initializing compressor!\n");
        free(in_file_buf);
        free(out_file_buf);
        fclose(pInFile);
        fclose(pOutFile);
        return false;
    }

    int x = max(1, (int)(src_file_size ^ (src_file_size >> 32)));

    for ( ; ; )
    {
        if (src_file_size)
        {
            double total_elapsed_time = (FxGetTickCount() - start_time) / FxGetTicksPerSecond();
            double total_bytes_processed = static_cast<double>(src_file_size - src_bytes_left);
            double comp_rate = (total_elapsed_time > 0.0f) ? total_bytes_processed / total_elapsed_time : 0.0f;
        }

        if (in_file_buf_ofs == in_file_buf_size)
        {
            in_file_buf_size = static_cast<int>(min(cInBufSize, src_bytes_left));

            if (fread(in_file_buf, 1, in_file_buf_size, pInFile) != in_file_buf_size)
            {
                Log("Failure reading from source file!\n");
                free(in_file_buf);
                free(out_file_buf);
                fclose(pInFile);
                fclose(pOutFile);
                deflateEnd(&zstream);
                return false;
            }

            src_bytes_left -= in_file_buf_size;
            in_file_buf_ofs = 0;
        }

        zstream.next_in = &in_file_buf[in_file_buf_ofs];
        zstream.avail_in = in_file_buf_size - in_file_buf_ofs;
        zstream.next_out = out_file_buf;
        zstream.avail_out = cOutBufSize;

        int flush = !src_bytes_left ? Z_FINISH : Z_NO_FLUSH;
        if ((flush == Z_NO_FLUSH) && (g_comp_options.m_random_z_flushing))
        {
            RND_SHR3(x);
            if ((x & 15) == 0)
            {
                RND_SHR3(x);
                flush = (x & 31) ? Z_SYNC_FLUSH : Z_FULL_FLUSH;
            }
        }
        status = deflate(&zstream, flush);

        int num_in_bytes = (in_file_buf_size - in_file_buf_ofs) - zstream.avail_in;
        int num_out_bytes = cOutBufSize - zstream.avail_out;
        if (num_in_bytes)
        {
            in_file_buf_ofs += (int)num_in_bytes;
            FX_ASSERT(in_file_buf_ofs <= in_file_buf_size);
        }

        if (num_out_bytes)
        {
            if (fwrite(out_file_buf, 1, static_cast<int>(num_out_bytes), pOutFile) != num_out_bytes)
            {
                Log("Failure writing to destination file!\n");
                free(in_file_buf);
                free(out_file_buf);
                fclose(pInFile);
                fclose(pOutFile);
                deflateEnd(&zstream);
                return false;
            }

            total_output_bytes += num_out_bytes;
        }

        if (status != Z_OK) break;
    }

    src_bytes_left += (in_file_buf_size - in_file_buf_ofs);

    int adler32 = zstream.adler;
    deflateEnd(&zstream);

    int end_time = FxGetTickCount();
    double total_time = (max(1, end_time - start_time)) / FxGetTicksPerSecond();

    UINT64 cmp_file_size = ftell(pOutFile);

    free(in_file_buf);
    in_file_buf = nullptr;
    free(out_file_buf);
    out_file_buf = nullptr;

    fclose(pInFile);
    pInFile = nullptr;
    fclose(pOutFile);
    pOutFile = nullptr;

    if (status != Z_STREAM_END)
    {
        Log("Compression failed with status %i\n", status);
        return false;
    }

    if (src_bytes_left)
    {
        Log("Compressor failed to consume entire input file!\n");
        return false;
    }

    //printf("Input file size: " QUAD_INT_FMT ", Compressed file size: " QUAD_INT_FMT ", Ratio: %3.2f%%\n", src_file_size, cmp_file_size, src_file_size ? ((1.0f - (static_cast<float>(cmp_file_size) / src_file_size)) * 100.0f) : 0.0f);
    //printf("Compression time: %3.6f\nConsumption rate: %9.1f bytes/sec, Emission rate: %9.1f bytes/sec\n", total_time, src_file_size / total_time, cmp_file_size / total_time);
    //printf("Input file adler32: 0x%08X\n", adler32);
    if (src_file_size)
    {
        if (src_file_size >= 256)
            s_max_large_comp_ratio = max(s_max_large_comp_ratio, cmp_file_size / (float)src_file_size);
        else
            s_max_small_comp_ratio = max(s_max_small_comp_ratio, cmp_file_size / (float)src_file_size);
    }

    return true;
}

QString GUIDToString(GUID* guid)
{
    return QUuid::fromRfc4122(QByteArray((char *)guid, sizeof(GUID))).toString(QUuid::StringFormat::Id128);
}

//*************************************************************************************************
// Platform-specific implementations (previously in QtUtils.cpp)
//*************************************************************************************************

#define LOG_TAG "Utils"

char *g_RootDirectory;

//
// Memory APIs: normal allocation APIs don't store size properly, so we have to hack the size
// into the first 4 bytes.
//

const int MEM_MAGIC = '1234';

void *FxAlloc(int Bytes)
{
    if (Bytes == 0) return nullptr;

    void *p = nullptr;
    int bytesAligned = (Bytes + 3) & ~3;

    int *data = (int *) malloc(bytesAligned + sizeof(int) * 3);
    if (data)
    {
        *data = MEM_MAGIC;
        *(data + 1) = Bytes;
        p = data + 2;
        memset(p, 0, bytesAligned);
        memcpy((BYTE *)p + bytesAligned, &MEM_MAGIC, sizeof(MEM_MAGIC));
        return p;
    }
    else
    {
        return nullptr;
    }
}

void *FxReAlloc(void *pData, int Bytes)
{
    void *newPtr = FxAlloc(Bytes);
    if (newPtr)
    {
        memcpy(newPtr, pData, min(FxSize(pData), Bytes));
        FxFree(pData);
    }

    return newPtr;
}

void FxFree(void *pData)
{
    if (!pData) return;

    if (!FxCheck(pData))
    {
        FX_ASSERT(false);
    }

    int *p = (int *)pData;
    int sizeFull = ((*(p - 1) + 3) & ~3) + sizeof(int) * 3;

    memset((void *)(p - 2), 0xFE, sizeFull);

    free((void *)(p - 2));
}

int FxSize(void *pData)
{
    if (!pData) return 0;

    if (!FxCheck(pData))
    {
        FX_ASSERT(false);
    }

    int *p = (int *)pData;
    return *((int *)pData - 1);
}

bool FxCheck(void *pData)
{
    if (pData == nullptr)
    {
        return true;
    }

    int *p = (int *)pData;
    int sizeAligned = (*(p - 1) + 3) & ~3;

    if (*(p-2) != MEM_MAGIC)
    {
        FX_ASSERT(false);
        return false;
    }

    if (memcmp((BYTE *)p + sizeAligned, &MEM_MAGIC, sizeof(MEM_MAGIC)) != 0)
    {
        FX_ASSERT(false);
        return false;
    }

    return true;
}

void FxHardFault(const char *pFile, int Line)
{
    LogError("Fault in %s, Line %d", pFile, Line);
    qDebug() << pFile << Line;
    qFatal("%s", pFile);
}

void FxOutputDebugString(char *pMessage)
{
    qDebug() << pMessage;
}

int FxGetTickCount()
{
    static QElapsedTimer s_timer;
    static bool s_first = true;
    if (s_first)
    {
        s_first = false;
        s_timer.start();
    }

    return static_cast<int>(s_timer.elapsed());

}

int FxGetTicksPerSecond()
{
    return 1000;
}

void FxSleep(int Milliseconds)
{
#ifdef Q_OS_WIN
    Sleep(uint(Milliseconds));
#else
    struct timespec ts = { (time_t)(Milliseconds / 1000), (time_t)((Milliseconds % 1000) * 1000 * 1000) };
    nanosleep(&ts, nullptr);
#endif
}

bool FxCreateDirectory(const char *pPath)
{
    QDir dir;

    bool result = dir.exists(pPath) || dir.mkdir(pPath);

    if (!result)
    {
        LogError("Failed to create directory %s", pPath);
    }

    return result;
}

bool FxDeleteDirectory(const char *pPath)
{
    QDir dir;

    bool result = dir.rmdir(pPath);

    if (!result)
    {
        LogError("Failed to delete directory %s", pPath);
    }

    return result;
}

bool FxMoveFile(const char *pExisting, const char *pNew)
{
    QFile file(pExisting);

    if (!file.exists())
    {
        LogError("MoveFile source not found %s", pExisting);
        return false;
    }

    if (!file.rename(pNew))
    {
        LogError("MoveFile could not rename to new file %s", pNew);
        return false;
    }

    return true;
}

bool FxCopyFile(const char *pExisting, const char *pNew)
{
    QFile fileFrom(pExisting);

    if (!fileFrom.exists())
    {
        LogError("CopyFile source not found %s", pExisting);
        return false;
    }

    if (!FxDeleteFile(pNew))
    {
        LogError("CopyFile could not remove file %s", pNew);
        return false;
    }

    if (!fileFrom.copy(pNew))
    {
        LogError("CopyFile could not write new file %s", pNew);
        return false;
    }

    return true;
}

bool FxDeleteFile(const char *pFileName)
{
    if (!FxDoesFileExist(pFileName))
    {
        return true;
    }

    if (!QFile::remove(pFileName))
    {
        LogError("Failed to remove %s", pFileName);
        return false;
    }

    return true;
}

int FxGetFileSize(const char *pFilename)
{
    QFile file(pFilename);

    if (file.exists())
    {
        return (int)file.size();
    }
    else
    {
        return 0;
    }
}

bool FxDoesFileExist(const char *pFileName)
{
    QFile file(pFileName);

    return file.exists();
}

bool FxBuildFileList(const char *pPath, const char *pPattern, FXFILELIST *pFileList)
{
    FileListClear(pFileList);

    QDir d(pPath);
    QFileInfoList files;
    QStringList filter;

    bool result = false;

    if (!d.exists())
    {
        LogError("opendir error");
        goto cleanup;
    }

    filter.append(pPattern);
    files = d.entryInfoList(filter, QDir::Files | QDir::NoDot | QDir::NoDotDot, QDir::NoSort);

    for (int i = 0; i < files.size(); i++)
    {
        const QFileInfo &fileInfo = files.at(i);
        FXFILE newFile = {0};

        newFile.FullPath = AllocString(fileInfo.absoluteFilePath().toStdString().c_str());
        newFile.Name = AllocString(fileInfo.fileName().toStdString().c_str());
        newFile.Size = (int)fileInfo.size();

        const QDateTime &created = fileInfo.birthTime();

        newFile.TimeStamp.Year = created.date().year();
        newFile.TimeStamp.Month = created.date().month();
        newFile.TimeStamp.Day = created.date().day();
        newFile.TimeStamp.Hour = created.time().hour();
        newFile.TimeStamp.Minute = created.time().minute();
        newFile.TimeStamp.Second = created.time().second();
        newFile.TimeStamp.Milliseconds = 0;

        pFileList->Add(newFile);

    }

    result = true;

  cleanup:

    return result;
}

bool FxMapFile(const char *pFileName, FXFILEMAP *pFileMap)
{
    bool Result = false;
    QFile *file;

    FxUnmapFile(pFileMap);

    file = new QFile(pFileName);
    pFileMap->FileHandle = (HANDLE)file;

    if (!file->open(QFile::ReadOnly))
    {
        LogError("Failed to open: %s", pFileName);
        goto cleanup;
    }

    pFileMap->FileSize = (int)file->size();
    pFileMap->BasePointer = file->map(0, pFileMap->FileSize);
    if (pFileMap->BasePointer == nullptr)
    {
        LogError("Failed to map file: %s", pFileName);
        goto cleanup;
    }

    Result = true;

  cleanup:

    if (Result == false)
    {
        FxUnmapFile(pFileMap);
    }

    return Result;
}

void FxUnmapFile(FXFILEMAP *pFileMap)
{
    QFile *file = (QFile *)pFileMap->FileHandle;

    if (pFileMap->BasePointer)
    {
        file->unmap((uchar *)pFileMap->BasePointer);
    }

    if (pFileMap->FileHandle != 0)
    {
        delete file;
    }

    memset(pFileMap, 0, sizeof(FXFILEMAP));
}
