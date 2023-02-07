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
#include "fxLists.h"

//
// Utility APIs with platform specific implementations.
//

VOID FxOutputDebugString(CHAR *pMessage);
VOID FxResetAutoOffTimer();
VOID FxSystemBeep();
UINT FxGetTickCount();
UINT FxGetTicksPerSecond();
VOID FxSleep(UINT Milliseconds);
BOOL FxCreateDirectory(const CHAR *pPath);
BOOL FxDeleteDirectory(const CHAR *pPath);
BOOL FxMoveFile(const CHAR *pExisting, const CHAR *pNew);
BOOL FxCopyFile(const CHAR *pExisting, const CHAR *pNew);
BOOL FxDeleteFile(const CHAR *pFilename);
UINT FxGetFileSize(const CHAR *pFilename);
BOOL FxDoesFileExist(const CHAR *pFileName);
BOOL FxBuildFileList(const CHAR *pPath, const CHAR *pPattern, FXFILELIST *pFileList);
BOOL FxMapFile(const CHAR *pFileName, FXFILEMAP *pFileMap);
VOID FxUnmapFile(FXFILEMAP *pFileMap);

//
// General utility APIs.
//

CHAR *AllocString(const CHAR *pValue);
CHAR *AllocMaxPath(const CHAR *pSource);
VOID AppendSlash(CHAR *pPath);
VOID TrimSlash(CHAR *pPath);
UINT StrLenUtf8(const CHAR *pValue);

VOID Log(const CHAR *pFormat, ...);

VOID Swap(INT *A, INT *B);
VOID Swap(INT16 *A, INT16 *B);
VOID Swap(DOUBLE *A, DOUBLE *B);
BOOL IsRectEmpty(const FXRECT *pRect);
BOOL IntersectRect(FXRECT *pDst, const FXRECT *pSrc1, const FXRECT *pSrc2);
VOID OffsetRect(FXRECT *pRect, INT Dx, INT Dy);
VOID InflateRect(FXRECT *pRect, INT Dx, INT Dy);
BOOL PtInRect(const FXRECT *pRect, const FXPOINT *pPoint);
BOOL PtInRect(const FXRECT *pRect, const INT X, const INT Y);
BOOL CompareRect(const FXRECT *pRect, const FXRECT *pRect2);

BOOL CompareGuid(const GUID *pGuid1, const GUID *pGuid2);
BOOL CompareGuid(const GUID Guid1, const GUID Guid2);
VOID InitGuid(GUID *pGuid);
VOID InitGuid(GUID *pDstGuid, const GUID *pSrcGuid);
BOOL IsNullGuid(GUID *pGuid);

VOID InitXGuid(XGUID *pGuid);
BOOL IsNullXGuid(const XGUID *pGuid);
BOOL CompareXGuid(const XGUID *pGuid1, const XGUID *pGuid2);

VOID InitGpsAccuracy(FXGPS_ACCURACY *pAccuracy);

GUID ConstructGuid(XGUID Id);

VOID GetBitmapRect(FXBITMAPRESOURCE *Bitmap, UINT Width, UINT Height, BOOL Center, BOOL Stretch, BOOL Proportional, FXRECT *pTargetRect);
VOID NumberToText(INT Value, BYTE Digits, BYTE Decimals, BYTE Fraction, BOOL Negative, CHAR *pOutValue, BOOL Spaces = FALSE);

DOUBLE EncodeDateTime(const FXDATETIME *pDateTime);
DOUBLE EncodeTime(const UINT16 Hours, const UINT16 Minutes, const UINT16 Seconds, const UINT16 Milliseconds);
VOID DecodeDate(DOUBLE EncodedDate, FXDATETIME *pDateTime);
VOID DecodeDateTime(DOUBLE EncodedDateTime, FXDATETIME *pDateTime);

DOUBLE CalcDistanceKm(DOUBLE Lat1, DOUBLE Lon1, DOUBLE Lat2, DOUBLE Lon2);
DOUBLE CalcDistanceMi(DOUBLE Lat1, DOUBLE Lon1, DOUBLE Lat2, DOUBLE Lon2);
DOUBLE CalcDistanceNaMi(DOUBLE Lat1, DOUBLE Lon1, DOUBLE Lat2, DOUBLE Lon2);
INT CalcHeading(DOUBLE Lat1, DOUBLE Lon1, DOUBLE Lat2, DOUBLE Lon2);

DOUBLE KnotsToKmHour(DOUBLE Knots);

BOOL StringReplace(CHAR **pString, const CHAR *pFrom, const CHAR *pTo);

BOOL TestGPS(FXGPS *pGPS, DOUBLE Accuracy = GPS_DEFAULT_ACCURACY);
BOOL TestGPS(FXGPS *pGPS, FXGPS_ACCURACY *pAccuracy);
BOOL TestGPS(FXGPS_POSITION *pGPS, DOUBLE Accuracy = GPS_DEFAULT_ACCURACY);
BOOL TestGPS(FXGPS_POSITION *pGPS, FXGPS_ACCURACY *pAccuracy);

DOUBLE EncodeDouble(INT IValue, INT IPoint, BOOL INegative, DOUBLE OutMin=-999999999, DOUBLE OutMax=999999999);
VOID DecodeDouble(DOUBLE Value, BYTE Decimals, INT *pIValue, INT *pIPoint, BOOL *pINegative, BOOL Pack = FALSE);

VOID InitCOM(BYTE *pPortId, FX_COMPORT *pComPort);

CHAR *AllocGuidName(GUID *pGuid, CHAR *pExtension);

CHAR *GetFileExtension(CHAR *pFileName);
BOOL ChangeFileExtension(CHAR *pFileName, CHAR *pNewExtension);
CHAR *ExtractFileName(CHAR *pFullFileName);

UINT FileListGetTotalSize(FXFILELIST *pFileList);
VOID FileListClear(FXFILELIST *pFileList);
BOOL FileListRemove(FXFILELIST *pFileList, CHAR *pFileName);

BOOL ClearDirectory(const CHAR *pPath);

HANDLE FileOpen(const CHAR *pFilename, const CHAR *pMode);
VOID FileClose(HANDLE Handle);
BOOL FileRead(HANDLE Handle, VOID *pBuffer, UINT Length);
BOOL FileSeek(HANDLE Handle, UINT Position);
UINT FileGetSize(HANDLE Handle);
BOOL FileWrite(HANDLE Handle, VOID *pBuffer, UINT Length);
UINT FileGetLastError(HANDLE Handle);

CHAR *AsciiToBase64(CHAR *pSource);
BOOL Base64Encode(CHAR *pSource, UINT SourceLen, CfxFileStream &Stream);

VOID FixJsonString(CfxString &String);

BOOL CompressFile(const char* pSrc_filename, const char *pDst_filename);

QString GUIDToString(GUID* guid);
