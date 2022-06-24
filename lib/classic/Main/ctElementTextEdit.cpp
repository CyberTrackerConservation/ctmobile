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
#include "ctElementTextEdit.h"
#include "ctActions.h"
#include "ctDialog_TextEditor.h"

CfxControl *Create_Control_ElementTextEdit(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementTextEdit(pOwner, pParent);
}

CctControl_ElementTextEdit::CctControl_ElementTextEdit(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Memo(pOwner, pParent), _retainState(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTEDIT);
    InitLockProperties("Result Element");

    _required = FALSE;
    _oneLine = FALSE;
    _maxLength = 0;

    InitXGuid(&_elementId);
	 
    RegisterRetainState(this, &_retainState);

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementTextEdit::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementTextEdit::OnSessionNext);
}

CctControl_ElementTextEdit::~CctControl_ElementTextEdit()
{
    UnregisterRetainState(this);

    UnregisterSessionEvent(seCanNext, this);
    UnregisterSessionEvent(seNext, this);
}

VOID CctControl_ElementTextEdit::SetCaption(const CHAR *Caption)
{
    CfxControl_Memo::SetCaption(Caption);

    if (_oneLine)
    {
        StringReplace(&_caption, "\n", " ");
        StringReplace(&_caption, "\r", "");
    }

    if (_maxLength != 0)
    {
        if (strlen(_caption) > _maxLength)
        {
            _caption[_maxLength] = '\0';            
        }
    }

    UpdateScrollBar();
}

VOID CctControl_ElementTextEdit::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (_required && ((_caption == NULL) || (strlen(_caption) == 0)))
    {
        GetSession(this)->ShowMessage("Text is required");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_ElementTextEdit::OnSessionNext(CfxControl *pSender, VOID *pParams)
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
    else
    {
        ((CctSession *)pSender)->ShowMessage("No Result: Text not saved");
    }
}

VOID CctControl_ElementTextEdit::DefineProperties(CfxFiler &F)
{
    CfxControl_Memo::DefineProperties(F);
    _retainState.DefineProperties(F);

    F.DefineValue("MaxLength", dtInt32, &_maxLength, F_ZERO);
    F.DefineValue("OneLine", dtBoolean, &_oneLine, F_FALSE);
    F.DefineValue("Required", dtBoolean, &_required, F_FALSE);
    F.DefineXOBJECT("Element", &_elementId);

    HackFixResultElementLockProperties(&_lockProperties);
}

VOID CctControl_ElementTextEdit::ResetState()
{
    CfxControl_Memo::ResetState();

    MEM_FREE(_caption);
    _caption = NULL;
}

VOID CctControl_ElementTextEdit::DefineState(CfxFiler &F)
{
    CfxControl_Memo::DefineState(F);
    _retainState.DefineState(F);

    F.DefineValue("Caption", dtPText, &_caption);
}

VOID CctControl_ElementTextEdit::DefinePropertiesUI(CfxFilerUI &F)
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    _retainState.DefinePropertiesUI(F);
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,     &_font);
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Maximum length", dtInt32,  &_maxLength);
    F.DefineValue("One line only", dtBoolean, &_oneLine);
    F.DefineValue("Required",     dtBoolean,  &_required);
    F.DefineValue("Result Element", dtGuid,   &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Scroll width", dtInt32,    &_scrollBarWidth);
    F.DefineValue("Text color",   dtColor,    &_textColor);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

VOID CctControl_ElementTextEdit::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Memo::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineField(_elementId, dtText);
#endif
}

VOID CctControl_ElementTextEdit::OnPenClick(INT X, INT Y)
{
    TEXTEDITOR_PARAMS params;
    params.ControlId = GetId();
    params.Text = AllocString(_caption);
    memcpy(&params.Font, &_font, sizeof(XFONT));
    
    GetSession(this)->ShowDialog(&GUID_DIALOG_TEXTEDITOR, NULL, &params, sizeof(TEXTEDITOR_PARAMS)); 
}

VOID CctControl_ElementTextEdit::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if ((_caption == NULL) || (strlen(_caption) == 0))
    {
        if (!_transparent)
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->FillRect(pRect);
        }

        CfxResource *resource = GetResource(); 
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font)
        {
            pCanvas->State.TextColor = _textColor;
            pCanvas->DrawTextRect(font, pRect, TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER, "Tap to edit");
            resource->ReleaseFont(font);
        }
    }
    else
    {
        CfxControl_Memo::OnPaint(pCanvas, pRect);
    }
}

VOID CctControl_ElementTextEdit::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_Memo::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);
    _height = max(_height, (INT)ScaleHitSize);
}
