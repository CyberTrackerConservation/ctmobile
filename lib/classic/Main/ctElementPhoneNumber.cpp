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
    _waitForTaken = _pending = FALSE;

    _defaultCaption = NULL;

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementPhoneNumber::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementPhoneNumber::OnSessionNext);
}

CctControl_ElementPhoneNumber::~CctControl_ElementPhoneNumber()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    MEM_FREE(_defaultCaption);
	_defaultCaption = NULL;
}

VOID CctControl_ElementPhoneNumber::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (GetHost(this)->IsPhoneNumberSupported() && _required && (_caption == NULL))
    {
        GetSession(this)->ShowMessage("Phone number is required");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_ElementPhoneNumber::OnSessionNext(CfxControl *pSender, VOID *pParams)
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

VOID CctControl_ElementPhoneNumber::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

	F.DefineValue("DefaultCaption", dtPText, &_defaultCaption, F_NULL);
    F.DefineXOBJECT("Element", &_elementId);
    F.DefineValue("Required", dtBoolean, &_required);
}

VOID CctControl_ElementPhoneNumber::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);

    F.DefineValue("WaitForTaken", dtBoolean, &_waitForTaken);
    F.DefineValue("Caption", dtPTextAnsi, &_caption);
}

VOID CctControl_ElementPhoneNumber::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Default caption", dtPText, &_defaultCaption);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Required",     dtBoolean,  &_required);
    F.DefineValue("Result Element", dtGuid,   &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_ElementPhoneNumber::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Panel::DefineResources(F);
    F.DefineObject(_elementId, eaName);
#endif
}

VOID CctControl_ElementPhoneNumber::OnPenClick(INT X, INT Y)
{
    _waitForTaken = FALSE;

    CfxHost *host = GetHost(this);
    if (host->IsPhoneNumberSupported())
    {
        _pending = TRUE;

        if (host->ShowPhoneNumberDialog())
        {
            _pending = FALSE;
            _waitForTaken = TRUE;

            GetSession(this)->SaveControlState(this);
        }
        else 
        {
            _pending = FALSE;
            Repaint();
        }
    }
    else if (IsLive())
    {
        GetSession(this)->ShowMessage("Phone number not supported");
    }
}

VOID CctControl_ElementPhoneNumber::OnPhoneNumberTaken(CHAR *pPhoneNumber, BOOL Success)
{
    if (!_waitForTaken) return;
    _waitForTaken = FALSE;
    if (!Success) return;

    SetCaption(pPhoneNumber);

    GetSession(this)->SaveControlState(this);

    Repaint();
}

VOID CctControl_ElementPhoneNumber::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
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
        CHAR *pending = "...";
        CHAR *ready = "Phone number";
        pCanvas->DrawTextRect(font, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, _pending ? pending : (_defaultCaption ? _defaultCaption : ready));
        resource->ReleaseFont(font);
    }
}
