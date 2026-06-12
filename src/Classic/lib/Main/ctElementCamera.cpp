#include "fxUtils.h"
#include "fxApplication.h"
#include "fxTranslate.h"
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
    _required = false;
    _waitForTaken = _pending = false;
    _cacheBitmap = nullptr;
    _imageQuality = FXIMAGE_QUALITY::Default;

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementCamera::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementCamera::OnSessionNext);
}

CctControl_ElementCamera::~CctControl_ElementCamera()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    MEM_FREE(_cacheBitmap);
    _cacheBitmap = nullptr;
}

void CctControl_ElementCamera::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if (_required && (_caption == nullptr))
    {
        GetSession(this)->ShowMessage(fxTranslate("Picture is required"));
        *((bool *)pParams) = false;
    }
}

void CctControl_ElementCamera::OnSessionNext(CfxControl *pSender, void *pParams)
{
    GetCaption();

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId))
    {
        CctAction_MergeAttribute action(this);
        action.SetControlId(_id);
        char *fullFileName = nullptr;

        if (_caption && (strlen(_caption) > 0))
        {
            fullFileName = (char *)MEM_ALLOC(MAX_PATH + 8);
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

void CctControl_ElementCamera::DefineProperties(CfxFiler &F)
{
    if (F.IsReader())
    {
        MEM_FREE(_cacheBitmap);
        _cacheBitmap = nullptr;
    }

    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("Required", dtBoolean, &_required, F_FALSE);
    F.DefineValue("ImageQuality", dtInt8, &_imageQuality, F_ZERO);

    F.DefineXOBJECT("Element", &_elementId);

    // Null id is automatically assigned to static

    HackFixResultElementLockProperties(&_lockProperties);
}

void CctControl_ElementCamera::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);

    F.DefineValue("SightingUniqueId", dtGuid, &_sightingUniqueId);
    F.DefineValue("WaitForTaken", dtBoolean, &_waitForTaken);
    F.DefineFileName("FileName", &_caption);
}

void CctControl_ElementCamera::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_ElementCamera::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementCamera::OnPenClick(int X, int Y)
{
    _waitForTaken = false;

    CfxHost *host = GetHost(this);
    char *fileName = GetSession(this)->AllocMediaFileName(nullptr);
    _pending = true;

    if (host->ShowCameraDialog(fileName, _imageQuality, GetSession(this)->GetResourceHeader()->SimpleCamera))
    {
        _pending = false;
        _waitForTaken = true;

        if (strlen(fileName) == 0)
        {
            GetSession(this)->SaveControlState(this);
        }
        else
        {
            OnPictureTaken(fileName, true);
        }
    }
    else
    {
        _pending = false;
        Repaint();
    }

    MEM_FREE(fileName);
    fileName = nullptr;
}

void CctControl_ElementCamera::OnPictureTaken(char *pFileName, bool Success)
{
    if (!_waitForTaken) return;
    _waitForTaken = false;
    if (!Success) return;

    MEM_FREE(_cacheBitmap);
    _cacheBitmap = nullptr;

    if (_caption != nullptr)
    {
        GetSession(this)->DeleteMedia(_caption);
        GetHost(this)->RemoveReplicatedMedia(_caption);
        SetCaption(nullptr);
    }

    SetCaption(GetSession(this)->GetMediaName(pFileName));
    GetSession(this)->BackupMedia(_caption);

    CctSession *session = GetSession(this);
    _sightingUniqueId = *session->GetCurrentSightingUniqueId();
    session->SaveCurrentScreen();
    session->SaveControlState(this);

    Repaint();
}

void CctControl_ElementCamera::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
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
        if (CompareGuid(&_sightingUniqueId, session->GetCurrentSightingUniqueId()) == false)
        {
            SetCaption(nullptr);
        }
    }

    if ((_caption == nullptr) || (strlen(_caption) == 0))
    {
        CfxResource *resource = GetResource(); 
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, fxTranslate(_pending ? "Please wait..." : "Tap to capture"));
            resource->ReleaseFont(font);
        }
    }
    else
    {
        if (!_cacheBitmap && IsLive())
        {
            char *fileName = GetSession(this)->AllocMediaFileName(_caption);
            if (fileName)
            {
                _cacheBitmap = pCanvas->CreateBitmapFromFile(fileName, pRect->Right - pRect->Left + 1, pRect->Bottom - pRect->Top + 1);
                MEM_FREE(fileName);
            }
        }

        if (_cacheBitmap)
        {
            FXRECT rect;
            GetBitmapRect(_cacheBitmap, pRect->Right - pRect->Left + 1, pRect->Bottom - pRect->Top + 1, true, true, true, &rect);
            pCanvas->StretchDraw(_cacheBitmap, pRect->Left + rect.Left, pRect->Top + rect.Top, rect.Right - rect.Left + 1 , rect.Bottom - rect.Top + 1, false, false);
        }
        else
        {
            SetCaption(fxTranslate("Tap to capture"));
            CfxControl_Panel::OnPaint(pCanvas, pRect);
            SetCaption("");
        }
    }
}

void CctControl_ElementCamera::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _height = max(_height, (int)ScaleHitSize);
}
