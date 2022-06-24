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

#include "fxHost.h"
#include "fxUtils.h"
#include "MapResource.h"

CMapResource::CMapResource(CfxPersistent *pOwner, CHAR *pResourceName): CfxResource(pOwner), _fileMap()
{
    _header = NULL;

    CfxHost *host = (CfxHost *)pOwner;
    CHAR *fileName = host->AllocPathApplication(pResourceName);
    CHAR *fullName = AllocMaxPath(fileName);
    strcat(fullName, CLIENT_RESOURCE_EXTENSION);

    if (FxMapFile(fullName, &_fileMap))
    {
        _header = (FXRESOURCEHEADER *)_fileMap.BasePointer;
    }

    MEM_FREE(fileName);
    MEM_FREE(fullName);
}

CMapResource::~CMapResource()
{
    FxUnmapFile(&_fileMap);
	_header = NULL;
}

BOOL CMapResource::Verify()
{ 
    return (_header != NULL) && (_header->Magic == MAGIC_RESOURCE); 
}

FXPROFILE *CMapResource::GetProfile(CfxControl *pControl, UINT Index)
{
    if (Index < _header->ProfileCount)
    {
        return (FXPROFILE *)((UINT64)_fileMap.BasePointer + _header->FirstProfile + Index * (sizeof(FXPROFILE) + 4));
    }
    else
    {
        return NULL;
    }
}

VOID CMapResource::ReleaseProfile(FXPROFILE *pProfile)
{
}

FXFONTRESOURCE *CMapResource::GetFont(CfxControl *pControl, XFONT Font)
{
    return (Font == 0) ? NULL : (FXFONTRESOURCE *)((UINT64)_fileMap.BasePointer + (UINT64)Font);
}

VOID CMapResource::ReleaseFont(FXFONTRESOURCE *pFontData)
{
}

FXBITMAPRESOURCE *CMapResource::GetBitmap(CfxControl *pControl, XBITMAP Bitmap)
{
    return (Bitmap == 0) ? NULL : (FXBITMAPRESOURCE *)((UINT64)_fileMap.BasePointer + (UINT64)Bitmap);
}

VOID CMapResource::ReleaseBitmap(FXBITMAPRESOURCE *pBitmapData)
{
}

FXSOUNDRESOURCE *CMapResource::GetSound(CfxControl *pControl, XSOUND Sound)
{
    return (Sound == 0) ? NULL : (FXSOUNDRESOURCE *)((UINT64)_fileMap.BasePointer + (UINT64)Sound);
}

VOID CMapResource::ReleaseSound(FXSOUNDRESOURCE *pSoundData)
{
}

FXDIALOG *CMapResource::GetDialog(CfxControl *pControl, const GUID *pObject)
{
    UINT64 offset = (UINT64)_fileMap.BasePointer + _header->FirstDialog;
	
    UINT i = 0;
    while (i < _header->DialogCount)
    {
        FXDIALOG *data = (FXDIALOG *)(offset + sizeof(FXOBJECTHEADER));
        FX_ASSERT(data->Magic == MAGIC_SCREEN);

        if (CompareGuid(pObject, &data->Guid))
        {
            return data;
        }
        i++;

        UINT resourceSize = *(UINT *)(offset - sizeof(UINT));
        offset = PadOffset(offset + resourceSize + sizeof(UINT));
    }

    return NULL;
}

VOID CMapResource::ReleaseDialog(FXDIALOG *pDialog)
{
}

VOID *CMapResource::GetObject(CfxControl *pControl, XGUID Object, BYTE Attribute)
{
    FX_ASSERT(Verify());

    if (Object == 0)
    {
        return NULL;
    }

    // Tracks the resource file offset
    UINT64 offset = (UINT64)_fileMap.BasePointer + Object;

    while (1)
    {
        FXOBJECTHEADER *header = (FXOBJECTHEADER *)offset;

        // Done searching: resource not found
        if (header->Magic != MAGIC_OBJECT)
        {
            break;
        }

        // Check for a match
        if (header->Attribute == Attribute)
        {
            return (VOID *)(offset + sizeof(FXOBJECTHEADER));
        }

        if (header->Attribute > Attribute)
        {
            break;
        }

        // No match, so skip ahead
        offset = PadOffset(offset + sizeof(FXOBJECTHEADER) + header->Size);
    }

    // Failed
    return NULL;
}

VOID CMapResource::ReleaseObject(VOID *pObject)
{
}

FXBINARYRESOURCE *CMapResource::GetBinary(CfxControl *pControl, XBINARY Binary)
{
    return (Binary == 0) ? NULL : (FXBINARYRESOURCE *)((UINT64)_fileMap.BasePointer + (UINT64)Binary);
}

VOID CMapResource::ReleaseBinary(FXBINARYRESOURCE *pBinaryData)
{
}

FXGOTO *CMapResource::GetGoto(CfxControl *pControl, UINT Index)
{
    return (_header->GotosOffset == 0) ? NULL : (FXGOTO *)((UINT64)_fileMap.BasePointer + (UINT64)_header->GotosOffset + Index * sizeof(FXGOTO));
}

VOID CMapResource::ReleaseGoto(FXGOTO *pGoto)
{
}

FXMOVINGMAP *CMapResource::GetMovingMap(CfxControl *pControl, UINT Index)
{
    return (_header->MovingMapsOffset == 0) ? NULL : (FXMOVINGMAP *)((UINT64)_fileMap.BasePointer + (UINT64)_header->MovingMapsOffset + Index * sizeof(FXMOVINGMAP));
}

VOID CMapResource::ReleaseMovingMap(FXMOVINGMAP *pMovingMap)
{
}

BOOL CMapResource::GetNextScreen(CfxControl *pControl, XGUID ScreenId, XGUID *pNextScreenId)
{
	UINT i = 0;
    UINT *currId = (UINT *)((UINT64)_fileMap.BasePointer + _header->NextScreenList);
	
	*pNextScreenId = 0;

	while (i < _header->NextScreenCount)
	{
		if (*currId == ScreenId)
		{
			*pNextScreenId = *(currId + 1);

			return TRUE;
		}
		
		i += 2;
		currId += 2;
	}

	return FALSE;
}
