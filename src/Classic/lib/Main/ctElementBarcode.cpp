#include "fxUtils.h"
#include "fxApplication.h"
#include "fxTranslate.h"
#include "ctElementBarcode.h"
#include "ctElement.h"
#include "ctActions.h"

CfxControl *Create_Control_ElementBarcode(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementBarcode(pOwner, pParent);
}

CctControl_ElementBarcode::CctControl_ElementBarcode(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Button(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTBARCODE);
    InitLockProperties("Result Element");

    InitGuid(&_sightingUniqueId);

    InitXGuid(&_elementId);
    _required = false;
    _waitForTaken = _pending = false;
    _filterBefore = _filterAfter = nullptr;
 
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementBarcode::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementBarcode::OnSessionNext);
}

CctControl_ElementBarcode::~CctControl_ElementBarcode()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    MEM_FREE(_filterBefore);
    _filterBefore = nullptr;

    MEM_FREE(_filterAfter);
    _filterAfter = nullptr;
}

void CctControl_ElementBarcode::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
	if (_required && (_caption == nullptr))
    {
        GetSession(this)->ShowMessage(fxTranslate("Barcode is required"));
        *((bool *)pParams) = false;
    }
}

void CctControl_ElementBarcode::OnSessionNext(CfxControl *pSender, void *pParams)
{
    GetCaption();

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId))
    {
        CctAction_MergeAttribute action(this);
        action.SetControlId(_id);

        if (_caption && (strlen(_caption) > 0))
        {
            action.SetupAdd(_id, _elementId, dtPText, &_caption);
        }

        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

void CctControl_ElementBarcode::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("Required", dtBoolean, &_required, F_FALSE);
    F.DefineValue("FilterBefore", dtPText, &_filterBefore);
    F.DefineValue("FilterAfter", dtPText, &_filterAfter);
  
    F.DefineXOBJECT("Element", &_elementId);

    // Null id is automatically assigned to static
}


void CctControl_ElementBarcode::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);

    F.DefineValue("SightingUniqueId", dtGuid, &_sightingUniqueId);
    F.DefineValue("WaitForTaken", dtBoolean, &_waitForTaken);
    F.DefineValue("Barcode", dtPTextAnsi, &_caption);
}

void CctControl_ElementBarcode::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_ElementBarcode::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementBarcode::OnPenClick(int X, int Y)
{
    _waitForTaken = false;

    CfxHost *host = GetHost(this);
    char *barCode = (char *)MEM_ALLOC(1024);
    _pending = true;

    if (host->ShowBarcodeDialog(barCode))
    {
        _pending = false;
        _waitForTaken = true;

        if (strlen(barCode) == 0)
        {
            GetSession(this)->SaveControlState(this);
        }
        else
        {
            OnBarcodeScan(barCode, true);
        }
    }
    else
    {
        _pending = false;
        Repaint();
    }

    MEM_FREE(barCode);
    barCode = nullptr;
}

void CctControl_ElementBarcode::OnBarcodeScan(char *pBarcode, bool Success)
{
    if (!_waitForTaken) return;
    _waitForTaken = false;
    if (!Success) return;

    char *barcode = pBarcode;
    if (_filterBefore) 
    {
        for (int i=0; i < (int)strlen(barcode) - (int)strlen(_filterBefore) - 1; i++)
        {
            if (strncmp(barcode + i, _filterBefore, strlen(_filterBefore)) == 0)
            {
                barcode = barcode + i + 1;
                break;
            }
        }
    }

    if (_filterAfter)
    {
        for (int i=0; i < (int)strlen(barcode) - (int)strlen(_filterAfter) - 1; i++)
        {
            if (strncmp(barcode + i, _filterAfter, strlen(_filterAfter)) == 0)
            {
                *(barcode + i) = '\0';
                break;
            }
        }
    }

    SetCaption(barcode);

    CctSession *session = GetSession(this);
    _sightingUniqueId = *session->GetCurrentSightingUniqueId();
    session->SaveControlState(this);

    Repaint();
}

void CctControl_ElementBarcode::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (IsLive())
    {
        CctSession *session = GetSession(this);
        session->LoadControlState(this);
        if (CompareGuid(&_sightingUniqueId, session->GetCurrentSightingUniqueId()) == false)
        {
            SetCaption(nullptr);
        }
    }

    char *oldCaption = _caption;

    if ((_caption == nullptr) || (strlen(_caption) == 0)) 
    {
        _caption = _pending ? (char *)"Please wait..." : (char *)"Tap to scan";
    }

    CfxControl_Button::OnPaint(pCanvas, pRect);

    _caption = oldCaption;
}

void CctControl_ElementBarcode::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _height = max(_height, (int)ScaleHitSize);
}
