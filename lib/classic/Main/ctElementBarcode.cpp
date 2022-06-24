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
    _required = FALSE;
    _waitForTaken = _pending = FALSE;
    _filterBefore = _filterAfter = NULL;
 
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementBarcode::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementBarcode::OnSessionNext);
}

CctControl_ElementBarcode::~CctControl_ElementBarcode()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    MEM_FREE(_filterBefore);
    _filterBefore = NULL;

    MEM_FREE(_filterAfter);
    _filterAfter = NULL;
}

VOID CctControl_ElementBarcode::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
	if (GetHost(this)->IsBarcodeSupported() && _required && (_caption == NULL))
    {
        GetSession(this)->ShowMessage("Barcode is required");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_ElementBarcode::OnSessionNext(CfxControl *pSender, VOID *pParams)
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

VOID CctControl_ElementBarcode::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("Required", dtBoolean, &_required, F_FALSE);
    F.DefineValue("FilterBefore", dtPText, &_filterBefore);
    F.DefineValue("FilterAfter", dtPText, &_filterAfter);
  
    F.DefineXOBJECT("Element", &_elementId);

    // Null id is automatically assigned to static
    #ifdef CLIENT_DLL
        if (IsNullXGuid(&_elementId))
        {
            _elementId = GUID_ELEMENT_BARCODE;
        }
    #endif
}


VOID CctControl_ElementBarcode::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);

    F.DefineValue("SightingUniqueId", dtGuid, &_sightingUniqueId);
    F.DefineValue("WaitForTaken", dtBoolean, &_waitForTaken);
    F.DefineValue("Barcode", dtPTextAnsi, &_caption);
}

VOID CctControl_ElementBarcode::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Filter text before", dtPText, &_filterBefore);
    F.DefineValue("Filter text after", dtPText, &_filterAfter);
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

VOID CctControl_ElementBarcode::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Panel::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineField(_elementId, dtText);
#endif
}

VOID CctControl_ElementBarcode::OnPenClick(INT X, INT Y)
{
    _waitForTaken = FALSE;

    CfxHost *host = GetHost(this);
    if (host->IsBarcodeSupported())
    {
        CHAR *barCode = (CHAR *)MEM_ALLOC(1024);
        _pending = TRUE;

        if (host->ShowBarcodeDialog(barCode))
        {
            _pending = FALSE;
            _waitForTaken = TRUE;

            if (strlen(barCode) == 0)
            {
                GetSession(this)->SaveControlState(this);
            }
            else
            {
                OnBarcodeScan(barCode, TRUE);
            }
        }
        else 
        {
            _pending = FALSE;
            Repaint();
        }

        MEM_FREE(barCode);
        barCode = NULL;
    }
    else if (IsLive())
    {
        GetSession(this)->ShowMessage("Barcode not supported");
    }
}

VOID CctControl_ElementBarcode::OnBarcodeScan(CHAR *pBarcode, BOOL Success)
{
    if (!_waitForTaken) return;
    _waitForTaken = FALSE;
    if (!Success) return;

    CHAR *barcode = pBarcode;
    if (_filterBefore) 
    {
        for (INT i=0; i < (INT)strlen(barcode) - (INT)strlen(_filterBefore) - 1; i++)
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
        for (INT i=0; i < (INT)strlen(barcode) - (INT)strlen(_filterAfter) - 1; i++)
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

VOID CctControl_ElementBarcode::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (IsLive())
    {
        CctSession *session = GetSession(this);
        session->LoadControlState(this);
        if (CompareGuid(&_sightingUniqueId, session->GetCurrentSightingUniqueId()) == FALSE)
        {
            SetCaption(NULL);
        }
    }

    CHAR *oldCaption = _caption;

    if ((_caption == NULL) || (strlen(_caption) == 0)) 
    {
        _caption = _pending ? (CHAR *)"Please wait..." : (CHAR *)"Tap to scan";
    }

    CfxControl_Button::OnPaint(pCanvas, pRect);

    _caption = oldCaption;
}

VOID CctControl_ElementBarcode::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _height = max(_height, (INT)ScaleHitSize);
}
