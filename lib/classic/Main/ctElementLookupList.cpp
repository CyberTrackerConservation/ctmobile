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
#include "ctElementLookupList.h"
#include "ctSession.h"
#include "ctActions.h"

//*************************************************************************************************
// CctControl_ElementLookupList

CfxControl *Create_Control_ElementLookupList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementLookupList(pOwner, pParent);
}

CctControl_ElementLookupList::CctControl_ElementLookupList(CfxPersistent *pOwner, CfxControl *pParent)
    : CfxControl_List(pOwner, pParent),
    _list()
{
    InitControl(&GUID_CONTROL_ELEMENTLOOKUPLIST);
    InitLockProperties("List items;List name;Result Element");

    InitXGuid(&_elementId);

    _resultGlobalValue = NULL;

    _autoNext = FALSE;

    _textColor = _borderColor;

    _selectedIndex = -1;
    _rightToLeft = FALSE;
    _centerText = FALSE;

    _required = TRUE;

    _listItems = 0;
    _pListBuffer = 0;
    _listName = 0;

    SetCaption("<No data: set \"List items\" property>");

    // Register for CanNext and Next events
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementLookupList::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementLookupList::OnSessionNext);
}

CctControl_ElementLookupList::~CctControl_ElementLookupList()
{
    MEM_FREE(_listItems);
    MEM_FREE(_listName);
    MEM_FREE(_pListBuffer);
    _listItems = NULL;

    MEM_FREE(_resultGlobalValue);
	_resultGlobalValue = NULL;

    UnregisterSessionEvent(seCanNext, this);
    UnregisterSessionEvent(seNext, this);
}

VOID CctControl_ElementLookupList::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_List::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineField(_elementId, dtText);
#endif
}

VOID CctControl_ElementLookupList::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    F.DefineValue("AutoNext", dtBoolean, &_autoNext, F_FALSE);
    F.DefineValue("LookupList", dtPText, &_listItems, F_NULL);
    F.DefineValue("LookupListName", dtPText, &_listName, F_NULL);
    F.DefineValue("Required", dtBoolean, &_required, F_FALSE);
    F.DefineValue("RightToLeft", dtBoolean, &_rightToLeft, F_FALSE);
    F.DefineValue("CenterText", dtBoolean, &_centerText, F_FALSE);
    F.DefineValue("ResultGlobalValue", dtPText, &_resultGlobalValue);

    F.DefineXOBJECT("Element", &_elementId);

    if (F.IsReader())
    {
        SetSelectedIndex(-1);

        //Update our list
        ParseLookupList();
        UpdateScrollBar();
    }
}

VOID CctControl_ElementLookupList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
    if (F.IsReader())
    {
        SetSelectedIndex(_selectedIndex);
    }
}

VOID CctControl_ElementLookupList::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Auto next",    dtBoolean,   &_autoNext);
    F.DefineValue("Border color", dtColor,     &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,      &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,     &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Center text",  dtBoolean,   &_centerText);
    F.DefineValue("Color",        dtColor,     &_color);
    F.DefineValue("Columns",      dtInt32,     &_columnCount, "MIN(1);MAX(8)");
    F.DefineValue("Dock",         dtByte,      &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Font",         dtFont,      &_font);
    F.DefineValue("Height",       dtInt32,     &_height);
    F.DefineValue("Item height",  dtInt32,     &_itemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Left",         dtInt32,     &_left);
    F.DefineValue("List items",   dtPText,     &_listItems, "MEMO;ROWHEIGHT(100)");
    F.DefineValue("List name",    dtPText,     &_listName);
    F.DefineValue("Minimum item height", dtInt32, &_minItemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Minimum item width",  dtInt32, &_minItemWidth,   "MIN(32);MAX(640)");
    F.DefineValue("Required",     dtBoolean,  &_required);
    F.DefineValue("Result Element", dtGuid,   &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Result global value", dtPText, &_resultGlobalValue);
    F.DefineValue("Right to left",dtBoolean,   &_rightToLeft);
    F.DefineValue("Scale for touch", dtBoolean,&_scaleForHitSize);
    F.DefineValue("Scroll width", dtInt32,     &_scrollBarWidth,    "MIN(7);MAX(40)");
    F.DefineValue("Show lines",   dtBoolean,   &_showLines);
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Width",        dtInt32,     &_width);    
#endif
}

VOID CctControl_ElementLookupList::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (_required && _selectedIndex == -1)
    {
        GetSession(this)->ShowMessage("Selection is required.");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_ElementLookupList::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    BOOL selected = (_selectedIndex != -1);

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId)  && selected)
    {
        CctAction_MergeAttribute action(this);
        action.SetControlId(_id);

        CHAR *pItem = GetItem(_selectedIndex);

        CHAR *pText = (CHAR *)MEM_ALLOC(strlen(pItem) + 1);
        if (pText)
        {
            strcpy(pText, pItem);
            action.SetupAdd(_id, _elementId, dtPText, &pText);
            ((CctSession *)pSender)->ExecuteAction(&action);
            MEM_FREE(pText);
        }
    }
}

UINT CctControl_ElementLookupList::GetItemCount()
{
    return _list.GetCount();
}

CHAR *CctControl_ElementLookupList::GetItem(UINT Index)
{
    return _list.Get(Index);
}

VOID CctControl_ElementLookupList::PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index)
{
    INT itemHeight = pItemRect->Bottom - pItemRect->Top + 1;

    // Paint if selected
    BOOL selected = ((INT)Index == _selectedIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);
    }

    // Draw the caption
    UINT flags = TEXT_ALIGN_VCENTER;
    FXRECT rect = *pItemRect;
    rect.Right -= 2 * _oneSpace;
    rect.Left += 2 * _oneSpace;

    if (_rightToLeft)
    {
        flags |= TEXT_ALIGN_RIGHT;
    }
    else if (_centerText)
    {
        flags |= TEXT_ALIGN_HCENTER;
    }

    pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);

    BOOL boundary = FALSE;
    CHAR *pItem = GetItem(Index);
    if (pItem)
    {
        pCanvas->DrawTextRect(pFont, &rect, flags, pItem);
    }
    else
    {
        pCanvas->DrawTextRect(pFont, &rect, flags, "Bad lookup item index");
    }
}

VOID CctControl_ElementLookupList::ParseLookupList()
{
    
    _list.Clear();

    if (_listItems == NULL)
        return;

    char* a_str = _listItems;
    char a_delim[3] = "\n;";

    if (_pListBuffer)
    {
        MEM_FREE(_pListBuffer);
        _pListBuffer = NULL;
    }
    _pListBuffer = (CHAR *)MEM_ALLOC(strlen(a_str) + 1);
    strcpy(_pListBuffer, _listItems);

    char* token = strtok(_pListBuffer, a_delim);
    while (token)
    {
        TrimString(token);
        if ((token != NULL) && (*token != '\0'))
        {
            _list.Add(token);
        }
        token = strtok(0, a_delim);
    }
}

VOID CctControl_ElementLookupList::SetSelectedIndex(INT Index)
{
    _selectedIndex = Index;

    // Set the global value
    if (IsLive() && (_resultGlobalValue != NULL))
    {
        GetSession(this)->SetGlobalValue(this, _resultGlobalValue, Index + 1);
    }
}

VOID CctControl_ElementLookupList::OnPenDown(INT X, INT Y)
{
    INT index;
    INT offsetX, offsetY;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        SetSelectedIndex(index);

        Repaint();
    }
}

VOID CctControl_ElementLookupList::OnPenClick(INT X, INT Y)
{
    if (_autoNext && (_selectedIndex != -1))
    {
        GetEngine(this)->KeyPress(KEY_RIGHT);
    }
}
