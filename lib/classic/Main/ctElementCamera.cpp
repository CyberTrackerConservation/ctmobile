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

#include "fxUtils.h"
#include "fxApplication.h"
#include "ctElementCamera.h"
#include "ctActions.h"

CfxControl *Create_Control_ElementCamera(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementCamera(pOwner, pParent);
}

CctControl_ElementCamera::CctControl_ElementCamera(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTCAMERA);
    InitLockProperties("Result Element");

    InitGuid(&_sightingUniqueId);

    InitXGuid(&_elementId);
    _required = FALSE;
    _waitForTaken = _pending = FALSE;
    _cacheBitmap = NULL;
    _imageQuality = iqDefault;

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementCamera::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementCamera::OnSessionNext);
}

CctControl_ElementCamera::~CctControl_ElementCamera()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    MEM_FREE(_cacheBitmap);
    _cacheBitmap = NULL;
}

VOID CctControl_ElementCamera::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (GetHost(this)->IsCameraSupported() && _required && (_caption == NULL))
    {
        GetSession(this)->ShowMessage("Picture is required");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_ElementCamera::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    GetCaption();

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId))
    {
        CctAction_MergeAttribute action(this);
        action.SetControlId(_id);
        CHAR *fullFileName = NULL;

        if (_caption && (strlen(_caption) > 0))
        {
            fullFileName = (CHAR *)MEM_ALLOC(MAX_PATH + 8);
            if (fullFileName)
            {
                strcpy(fullFileName, MEDIA_PREFIX);
                strcat(fullFileName, _caption);
                action.SetupAdd(_id, _elementId, dtPText, &fullFileName);
            }
        }

        ((CctSession *)pSender)->ExecuteAction(&action);

        MEM_FREE(fullFileName);
    }
}

VOID CctControl_ElementCamera::DefineProperties(CfxFiler &F)
{
    if (F.IsReader())
    {
        MEM_FREE(_cacheBitmap);
        _cacheBitmap = NULL;
    }

    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("Required", dtBoolean, &_required, F_FALSE);
    F.DefineValue("ImageQuality", dtInt8, &_imageQuality, F_ZERO);

    F.DefineXOBJECT("Element", &_elementId);

    // Null id is automatically assigned to static
    #ifdef CLIENT_DLL
        if (IsNullXGuid(&_elementId))
        {
            _elementId = GUID_ELEMENT_PHOTO;
        }
    #endif

    HackFixResultElementLockProperties(&_lockProperties);
}

VOID CctControl_ElementCamera::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);

    F.DefineValue("SightingUniqueId", dtGuid, &_sightingUniqueId);
    F.DefineValue("WaitForTaken", dtBoolean, &_waitForTaken);
    F.DefineValue("FileName", dtPTextAnsi, &_caption);
}

VOID CctControl_ElementCamera::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Required",     dtBoolean,  &_required);
    F.DefineValue("Result Element", dtGuid,   &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Image quality", dtInt8,    &_imageQuality,"LIST(Default \"Very high\" High Medium Low \"Very low\" \"Ultra low\")");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_ElementCamera::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Panel::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineField(_elementId, dtGraphic);
#endif
}

VOID CctControl_ElementCamera::OnPenClick(INT X, INT Y)
{
    _waitForTaken = FALSE;

    CfxHost *host = GetHost(this);
    if (host->IsCameraSupported())
    {
        CHAR *fileName = GetSession(this)->AllocMediaFileName(NULL);
        _pending = TRUE;

        if (host->ShowCameraDialog(fileName, _imageQuality))
        {
            _pending = FALSE;
            _waitForTaken = TRUE;

            if (strlen(fileName) == 0)
            {
                GetSession(this)->SaveControlState(this);
            }
            else
            {
                OnPictureTaken(fileName, TRUE);
            }
        }
        else 
        {
            _pending = FALSE;
            Repaint();
        }

        MEM_FREE(fileName);
        fileName = NULL;
    }
    else if (IsLive())
    {
        GetSession(this)->ShowMessage("Camera not supported");
    }
}

VOID CctControl_ElementCamera::OnPictureTaken(CHAR *pFileName, BOOL Success)
{
    if (!_waitForTaken) return;
    _waitForTaken = FALSE;
    if (!Success) return;

    MEM_FREE(_cacheBitmap);
    _cacheBitmap = NULL;

    if (_caption != NULL)
    {
        GetSession(this)->DeleteMedia(_caption);
        SetCaption(NULL);
    }

    SetCaption(GetSession(this)->GetMediaName(pFileName));
    GetSession(this)->BackupMedia(_caption);

    CctSession *session = GetSession(this);
    _sightingUniqueId = *session->GetCurrentSightingUniqueId();
    session->SaveControlState(this);

    Repaint();
}

VOID CctControl_ElementCamera::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    if (IsLive())
    {
        CctSession *session = GetSession(this);
        session->LoadControlState(this);
        if (CompareGuid(&_sightingUniqueId, session->GetCurrentSightingUniqueId()) == FALSE)
        {
            SetCaption(NULL);
        }
    }

    if ((_caption == NULL) || (strlen(_caption) == 0))
    {
        CfxResource *resource = GetResource(); 
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            pCanvas->State.TextColor = _textColor;
            CHAR *pending = "Please wait...";
            CHAR *ready = "Tap to capture";
            pCanvas->DrawTextRect(font, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, _pending ? pending : ready);
            resource->ReleaseFont(font);
        }
    }
    else
    {
        if (!_cacheBitmap && IsLive())
        {
            CHAR *fileName = GetSession(this)->AllocMediaFileName(_caption);
            if (fileName)
            {
                _cacheBitmap = pCanvas->CreateBitmapFromFile(fileName, pRect->Right - pRect->Left + 1, pRect->Bottom - pRect->Top + 1);
                MEM_FREE(fileName);
            }
        }

        if (_cacheBitmap)
        {
            FXRECT rect;
            GetBitmapRect(_cacheBitmap, pRect->Right - pRect->Left + 1, pRect->Bottom - pRect->Top + 1, TRUE, TRUE, TRUE, &rect);
            pCanvas->StretchDraw(_cacheBitmap, pRect->Left + rect.Left, pRect->Top + rect.Top, rect.Right - rect.Left + 1 , rect.Bottom - rect.Top + 1, FALSE, FALSE);
        }
        else
        {
            SetCaption("Tap to capture");
            CfxControl_Panel::OnPaint(pCanvas, pRect);
            SetCaption("");
        }
    }
}

VOID CctControl_ElementCamera::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _height = max(_height, (INT)ScaleHitSize);
}
