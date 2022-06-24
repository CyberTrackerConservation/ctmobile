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

#include "fxApplication.h"
#include "fxUtils.h"
#include "ctNumberList.h"
#include "ctActions.h"

CfxControl *Create_Control_NumberList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_NumberList(pOwner, pParent);
}

//*************************************************************************************************
// CctControl_NumberList

CctControl_NumberList::CctControl_NumberList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent), _retainState(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_NUMBERLIST);
    InitLockProperties("Result Element;First value;Last value");

    _autoNext = FALSE;
    _startValue = 1;
    _endValue = 50;
    _interval = 1;
    _selectedIndex = -1;
    InitXGuid(&_elementId);

    _minItemWidth = 8;
    _autoSelectIndex = 0;

    // Global result
    _resultGlobalValue = NULL;

    // Register for next event
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_NumberList::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_NumberList::OnSessionNext);

    RegisterRetainState(this, &_retainState);
}

CctControl_NumberList::~CctControl_NumberList()
{
    UnregisterRetainState(this);

    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    MEM_FREE(_resultGlobalValue);
	_resultGlobalValue = NULL;
}

VOID CctControl_NumberList::SetSelectedIndex(INT Index)
{
    _selectedIndex = Index;

    // Set the global value
    if (IsLive() && (_resultGlobalValue != NULL))
    {
        GetSession(this)->SetGlobalValue(this, _resultGlobalValue, (Index == -1) ? 0 : Index + _startValue);
    }
}

VOID CctControl_NumberList::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (_selectedIndex == -1)
    {
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_NumberList::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    if (_selectedIndex == -1) return;

    //
    // Add the Attribute for this Element
    //
    INT value = _selectedIndex * _interval + _startValue;
    if (!IsNullXGuid(&_elementId) && (_selectedIndex >= 0))
    {
        CctAction_MergeAttribute action(this);
        action.SetupAdd(_id, _elementId, dtInt32, &value);
        ((CctSession *)pSender)->ExecuteAction(&action);
    }
}

VOID CctControl_NumberList::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_List::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineObject(_elementId, eaIcon32);   // for history list
    F.DefineField(_elementId, dtInt32);
#endif
}

VOID CctControl_NumberList::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);
    _retainState.DefineProperties(F);

    F.DefineValue("AutoNext",        dtBoolean, &_autoNext, F_FALSE);
    F.DefineValue("AutoSelectIndex", dtInt32,   &_autoSelectIndex, F_ZERO);
    F.DefineValue("StartValue",      dtInt32,   &_startValue, F_ONE);
    F.DefineValue("EndValue",        dtInt32,   &_endValue, "50");
    F.DefineValue("Interval",        dtInt32,   &_interval, "1");
    F.DefineValue("ResultGlobalValue", dtPText, &_resultGlobalValue);

    F.DefineXOBJECT("Element", &_elementId);

    if (F.IsReader())
    {
        _minItemWidth = 8;
        _endValue = max(_startValue + 1, _endValue);
        UpdateScrollBar();

        if ((_autoSelectIndex > 0) && (_autoSelectIndex <= GetItemCount()))
        {
            SetSelectedIndex(_autoSelectIndex - 1);
        }

        HackFixResultElementLockProperties(&_lockProperties);
    }
}

VOID CctControl_NumberList::ResetState()
{
    CfxControl_List::ResetState();
    _selectedIndex = -1;
}

VOID CctControl_NumberList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    _retainState.DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
}

VOID CctControl_NumberList::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Auto next",    dtBoolean,   &_autoNext);
    F.DefineValue("Auto select index", dtInt32, &_autoSelectIndex, "MIN(0);MAX(10000)");
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Columns",      dtInt32,     &_columnCount, "MIN(1);MAX(10)");
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("First value",  dtInt32,     &_startValue,  "MIN(0);MAX(499)");
    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Last value",   dtInt32,     &_endValue,    "MIN(1);MAX(500)");
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("Interval",     dtInt32,     &_interval,    "MIN(1);MAX(100)");
    F.DefineValue("Item height",  dtInt32,     &_itemHeight,  "MIN(16);MAX(65)");
    F.DefineValue("Minimum item height", dtInt32, &_minItemHeight,  "MIN(12);MAX(320)");
    F.DefineValue("Result Element", dtGuid,    &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Result global value", dtPText, &_resultGlobalValue);
    _retainState.DefinePropertiesUI(F);
    F.DefineValue("Scale for touch", dtBoolean, &_scaleForHitSize);
    F.DefineValue("Scroll width", dtInt32,     &_scrollBarWidth);
    F.DefineValue("Show lines",   dtBoolean,   &_showLines);
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Width",        dtInt32,     &_width);
#endif
}

VOID CctControl_NumberList::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl_List::SetBounds(Left, Top, Width, Height);

    UpdateScrollBar();
}

VOID CctControl_NumberList::OnPenDown(INT X, INT Y)
{
    INT index;
    INT offsetX, offsetY;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        SetSelectedIndex((index != -1) ? index : _selectedIndex);
        Repaint();
    }
}

VOID CctControl_NumberList::OnPenClick(INT X, INT Y)
{
    if (_autoNext && (_selectedIndex != -1))
    {
        GetEngine(this)->KeyPress(KEY_RIGHT);
    }
}

UINT CctControl_NumberList::GetItemCount()
{
    return (_endValue - _startValue + 1) / max(1, _interval);
}

VOID CctControl_NumberList::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, UINT Index)
{
    CHAR caption[10];
    BOOL selected = ((INT)Index == _selectedIndex);
    sprintf(caption, "%ld", (INT)(_startValue + Index * _interval));

    pCanvas->State.BrushColor = pCanvas->InvertColor(_color, selected);
    pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);

    if (selected)
    {
        pCanvas->FillRect(pRect);
    }

    UINT w = pRect->Right - pRect->Left + 1;
    UINT h = pRect->Bottom - pRect->Top + 1;
    INT x = (w - pCanvas->TextWidth(pFont, caption)) / 2;
    INT y = (h - pCanvas->FontHeight(pFont)) / 2;

    pCanvas->DrawText(pFont, pRect->Left + x, pRect->Top + y, caption);
}
