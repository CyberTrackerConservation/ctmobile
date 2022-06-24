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

#include "fxPlatform.h"
#include "fxUtils.h"

CHAR *ToAnsi(const WCHAR *pValue);
CHAR *ToUtf8(const WCHAR *pValue);
WCHAR *ToUnicodeFromAnsi(const CHAR *pValue);
WCHAR *ToUnicodeFromUtf8(const CHAR *pValue);
WCHAR *DupStringW(const WCHAR *pValue);
WCHAR *AllocMaxPathW(const WCHAR *pSource);
VOID AppendSlashW(WCHAR *pPath);
VOID TrimSlashW(WCHAR *pPath);
WCHAR *ExtractFileNameW(WCHAR *pFullFileName);

WCHAR *AllocMaxPathApplicationW();

UINT GetFileSizeW(WCHAR *pFileName);
BOOL FileExistsW(WCHAR *pFileName);

BOOL GetRegValueString(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue, WCHAR **pValue);
BOOL SetRegValueString(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue, const WCHAR *pValue);
BOOL GetRegValueDword(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue, DWORD *pValue);
BOOL SetRegValueDword(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue, const DWORD Value);
BOOL GetRegValueBool(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue);
VOID SetRegValueBool(const HKEY hKeyBase, const WCHAR *lpKey, const WCHAR *lpValue, const BOOL Value);

CHAR *GetDateStringUTC(DOUBLE Value, BOOL FormatAsUTC);
