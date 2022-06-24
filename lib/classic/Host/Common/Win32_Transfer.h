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

struct TRANSFER_ITEM
{
    HWND hWnd;
    UINT Id;
    FXSENDDATA SendData;
    WCHAR SourcePath[MAX_PATH];
    WCHAR FileName[MAX_PATH];
};

class CTransferManager
{
protected:
    CRITICAL_SECTION _lock;
    TList<TRANSFER_ITEM> _pending;
    WCHAR *_outgoingMaxPath;
    
    UINT _idMax;
    HWND _hWnd;

    BOOL InternalSendFile(HWND hWnd, FXSENDDATA *pSendData, WCHAR *pSourcePath, WCHAR *pFileName);
    BOOL Complete(TRANSFER_ITEM *pItem);
    VOID ClearPending();
public:
    CTransferManager();
	~CTransferManager();

    VOID Terminate();

    BOOL FileRequested(TRANSFER_ITEM *pItem);
    BOOL FileRejected(TRANSFER_ITEM *pItem);
    BOOL FileCompleted(TRANSFER_ITEM *pItem);

    BOOL SendFiles(HWND hWnd, WCHAR *pSourcePath);
    BOOL SendFile(HWND hWnd, FXSENDDATA *pSendData, WCHAR *pSourcePath, WCHAR *pFileName);
    VOID Stop();

    VOID GetState(FXSENDSTATE *pState);

    BOOL IsIdle();
};

extern CTransferManager *g_TransferManager;
