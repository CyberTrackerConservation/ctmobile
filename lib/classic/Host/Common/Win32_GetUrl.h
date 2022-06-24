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

#include "fxTypes.h"
#include "fxClasses.h"
#include "fxEngine.h"

struct GETURL_ITEM
{
    HWND hWnd;
    UINT Id;
    CHAR Url[1024];
    CHAR UserName[64];
    CHAR Password[64];
    CHAR FileName[MAX_PATH];
    BYTE *Data;
    UINT DataSize;
    UINT ErrorCode;
};

class CGetUrlManager
{
protected:
    CRITICAL_SECTION _lock;
    TList<GETURL_ITEM> _pending;
    HWND _hWnd;
    VOID ClearPending();
    UINT _idMax;
public:
    CGetUrlManager();
	~CGetUrlManager();

    VOID Terminate();
    VOID Stop();

    BOOL UrlRequested(GETURL_ITEM *pItem);
    VOID UrlCompleted(GETURL_ITEM *pItem);

    BOOL GetFile(HWND hWnd, CHAR *pUrl, CHAR *pUserName, CHAR *pPassword, CfxStream &Data, CHAR *pFileName, UINT *pId);
    VOID CancelPending(UINT Id);

    BOOL IsIdle();
};

extern CGetUrlManager *g_GetUrlManager;
