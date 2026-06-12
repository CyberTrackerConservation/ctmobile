#pragma once
#include "pch.h"
#include "fxTypes.h"
#include "fxLists.h"

//
// Utility APIs with platform specific implementations.
//

void FxOutputDebugString(char *pMessage);
int FxGetTickCount();
int FxGetTicksPerSecond();
void FxSleep(int Milliseconds);
bool FxCreateDirectory(const char *pPath);
bool FxDeleteDirectory(const char *pPath);
bool FxMoveFile(const char *pExisting, const char *pNew);
bool FxCopyFile(const char *pExisting, const char *pNew);
bool FxDeleteFile(const char *pFilename);
int FxGetFileSize(const char *pFilename);
bool FxDoesFileExist(const char *pFileName);
bool FxBuildFileList(const char *pPath, const char *pPattern, FXFILELIST *pFileList);
bool FxMapFile(const char *pFileName, FXFILEMAP *pFileMap);
void FxUnmapFile(FXFILEMAP *pFileMap);

//
// General utility APIs.
//

char *AllocString(const char *pValue);
char *AllocMaxPath(const char *pSource);
void AppendSlash(char *pPath);
void TrimSlash(char *pPath);
int StrLenUtf8(const char *pValue);

void Log(const char *pFormat, ...);

void Swap(int *A, int *B);
void Swap(INT16 *A, INT16 *B);
void Swap(double *A, double *B);
bool IsRectEmpty(const FXRECT *pRect);
bool IntersectRect(FXRECT *pDst, const FXRECT *pSrc1, const FXRECT *pSrc2);
void OffsetRect(FXRECT *pRect, int Dx, int Dy);
void InflateRect(FXRECT *pRect, int Dx, int Dy);
bool PtInRect(const FXRECT *pRect, const FXPOINT *pPoint);
bool PtInRect(const FXRECT *pRect, const int X, const int Y);
bool CompareRect(const FXRECT *pRect, const FXRECT *pRect2);

bool CompareGuid(const GUID *pGuid1, const GUID *pGuid2);
bool CompareGuid(const GUID Guid1, const GUID Guid2);
void InitGuid(GUID *pGuid);
void InitGuid(GUID *pDstGuid, const GUID *pSrcGuid);
bool IsNullGuid(GUID *pGuid);

void InitXGuid(XGUID *pGuid);
bool IsNullXGuid(const XGUID *pGuid);
bool CompareXGuid(const XGUID *pGuid1, const XGUID *pGuid2);

void InitGpsAccuracy(FXGPS_ACCURACY *pAccuracy);

GUID ConstructGuid(XGUID Id);

void GetBitmapRect(FXBITMAPRESOURCE *Bitmap, int Width, int Height, bool Center, bool Stretch, bool Proportional, FXRECT *pTargetRect);
void NumberToText(int Value, BYTE Digits, BYTE Decimals, BYTE Fraction, bool Negative, char *pOutValue, bool Spaces = false);

double EncodeDateTime(const FXDATETIME *pDateTime);
double EncodeTime(const UINT16 Hours, const UINT16 Minutes, const UINT16 Seconds, const UINT16 Milliseconds);
void DecodeDate(double EncodedDate, FXDATETIME *pDateTime);
void DecodeDateTime(double EncodedDateTime, FXDATETIME *pDateTime);

double CalcDistanceKm(double Lat1, double Lon1, double Lat2, double Lon2);
double CalcDistanceMi(double Lat1, double Lon1, double Lat2, double Lon2);
double CalcDistanceNaMi(double Lat1, double Lon1, double Lat2, double Lon2);
int CalcHeading(double Lat1, double Lon1, double Lat2, double Lon2);

double KnotsToKmHour(double Knots);

bool StringReplace(char **pString, const char *pFrom, const char *pTo);

bool TestGPS(FXGPS *pGPS, double Accuracy = GPS_DEFAULT_ACCURACY);
bool TestGPS(FXGPS *pGPS, FXGPS_ACCURACY *pAccuracy);
bool TestGPS(FXGPS_POSITION *pGPS, double Accuracy = GPS_DEFAULT_ACCURACY);
bool TestGPS(FXGPS_POSITION *pGPS, FXGPS_ACCURACY *pAccuracy);

double EncodeDouble(int IValue, int IPoint, bool INegative, double OutMin=-999999999, double OutMax=999999999);
void DecodeDouble(double Value, BYTE Decimals, int *pIValue, int *pIPoint, bool *pINegative, bool Pack = false);

void InitCOM(BYTE *pPortId, FX_COMPORT *pComPort);

char *AllocGuidName(GUID *pGuid, char *pExtension);

const char *GetFileExtension(const char *pFileName);
bool ChangeFileExtension(char *pFileName, char *pNewExtension);
char *ExtractFileName(char *pFullFileName);

int FileListGetTotalSize(FXFILELIST *pFileList);
void FileListClear(FXFILELIST *pFileList);
bool FileListRemove(FXFILELIST *pFileList, char *pFileName);

bool ClearDirectory(const char *pPath);

HANDLE FileOpen(const char *pFilename, const char *pMode);
void FileClose(HANDLE Handle);
bool FileRead(HANDLE Handle, void *pBuffer, int Length);
bool FileSeek(HANDLE Handle, int Position);
int FileGetSize(HANDLE Handle);
bool FileWrite(HANDLE Handle, void *pBuffer, int Length);
int FileGetLastError(HANDLE Handle);

char *AsciiToBase64(char *pSource);
bool Base64Encode(char *pSource, int SourceLen, CfxFileStream &Stream);

void FixJsonString(QString &String);

bool CompressFile(const char* pSrc_filename, const char *pDst_filename);

QString GUIDToString(GUID* guid);
