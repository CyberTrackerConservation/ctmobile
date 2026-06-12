#include "fxUtils.h"
#include "fxApplication.h"
#include "ctElementPhoneNumber.h"
#include "ctActions.h"

CfxControl *Create_Control_ElementPhoneNumber(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementPhoneNumber(pOwner, pParent);
}

CctControl_ElementPhoneNumber::CctControl_ElementPhoneNumber(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTPHONENUMBER);
    InitLockProperties("Result Element");

    InitXGuid(&_elementId);
    _waitForTaken = _pending = false;

    _defaultCaption = nullptr;

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementPhoneNumber::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementPhoneNumber::OnSessionNext);
}

CctControl_ElementPhoneNumber::~CctControl_ElementPhoneNumber()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    MEM_FREE(_defaultCaption);
	_defaultCaption = nullptr;
}

void CctControl_ElementPhoneNumber::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
}

void CctControl_ElementPhoneNumber::OnSessionNext(CfxControl *pSender, void *pParams)
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

void CctControl_ElementPhoneNumber::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

	F.DefineValue("DefaultCaption", dtPText, &_defaultCaption, F_NULL);
    F.DefineXOBJECT("Element", &_elementId);
    F.DefineValue("Required", dtBoolean, &_required);
}

void CctControl_ElementPhoneNumber::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);

    F.DefineValue("WaitForTaken", dtBoolean, &_waitForTaken);
    F.DefineValue("Caption", dtPTextAnsi, &_caption);
}

void CctControl_ElementPhoneNumber::DefinePropertiesUI(CfxFilerUI &F)
{
}

void CctControl_ElementPhoneNumber::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementPhoneNumber::OnPenClick(int X, int Y)
{
    if (IsLive())
    {
        GetSession(this)->ShowMessage("Phone number not supported");
    }
}

void CctControl_ElementPhoneNumber::OnPhoneNumberTaken(char *pPhoneNumber, bool Success)
{
    if (!_waitForTaken) return;
    _waitForTaken = false;
    if (!Success) return;

    SetCaption(pPhoneNumber);

    GetSession(this)->SaveControlState(this);

    Repaint();
}

void CctControl_ElementPhoneNumber::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    if (_caption && strlen(_caption))
    {
        CfxControl_Panel::OnPaint(pCanvas, pRect);
        return;
    }

    CfxResource *resource = GetResource(); 
    FXFONTRESOURCE *font = resource->GetFont(this, _font);
    if (font)
    {
        pCanvas->State.TextColor = _textColor;
        const char *pending = "...";
        const char *ready = "Phone number";
        pCanvas->DrawTextRect(font, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, _pending ? pending : (_defaultCaption ? _defaultCaption : ready));
        resource->ReleaseFont(font);
    }
}
