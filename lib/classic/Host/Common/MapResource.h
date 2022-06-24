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

#include "pch.h"
#include "fxControl.h"

//
// CMapResource
//

class CMapResource: public CfxResource
{
protected:
    FXFILEMAP _fileMap;
    UINT64 PadOffset(UINT64 Value) { return (Value + 3) & ~3; }
public:
    CMapResource(CfxPersistent *pOwner, CHAR *pResourceName);
    ~CMapResource();

    BOOL Verify();
    
    FXPROFILE *GetProfile(CfxControl *pControl, UINT Index);
    VOID ReleaseProfile(FXPROFILE *pProfile);
    FXFONTRESOURCE *GetFont(CfxControl *pControl, XFONT Font);
    VOID ReleaseFont(FXFONTRESOURCE *pFontData);
    FXDIALOG *GetDialog(CfxControl *pControl, const GUID *pObject);
    VOID ReleaseDialog(FXDIALOG *pDialog);
    VOID *GetObject(CfxControl *pControl, XGUID Object, BYTE Attribute=0);
    VOID ReleaseObject(VOID *pObject);
    FXSOUNDRESOURCE *GetSound(CfxControl *pControl, XSOUND Sound);
    VOID ReleaseSound(FXSOUNDRESOURCE *pSoundData);
    FXBITMAPRESOURCE *GetBitmap(CfxControl *pControl, XBITMAP Bitmap);
    VOID ReleaseBitmap(FXBITMAPRESOURCE *pBitmapData);
    FXBINARYRESOURCE *GetBinary(CfxControl *pControl, XBINARY Binary);
    VOID ReleaseBinary(FXBINARYRESOURCE *pBinaryData);
    FXGOTO *GetGoto(CfxControl *pControl, UINT Index);
    VOID ReleaseGoto(FXGOTO *pGoto);
    BOOL GetNextScreen(CfxControl *pControl, XGUID ScreenId, XGUID *pNextScreenId);
	FXMOVINGMAP *GetMovingMap(CfxControl *pControl, UINT Index);
	VOID ReleaseMovingMap(FXMOVINGMAP *pMovingMap);
};

