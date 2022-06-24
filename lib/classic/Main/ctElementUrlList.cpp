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
#include "ctElementUrlList.h"
#include "ctSession.h"
#include "ctActions.h"

//*************************************************************************************************
// CctControl_ElementUrlList

CfxControl *Create_Control_ElementUrlList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementUrlList(pOwner, pParent);
}

CctControl_ElementUrlList::CctControl_ElementUrlList(CfxPersistent *pOwner, CfxControl *pParent)
    : CfxControl_List(pOwner, pParent),
    _list()
{
    InitControl(&GUID_CONTROL_ELEMENTURLLIST);
    InitLockProperties("Result Element;Url;Url - user name;Url - password");

    InitXGuid(&_elementId);

    _url = _userName = _password = NULL;
    _waitingForResponse = _waitingInvert = FALSE;
    _separator = NULL;

    _autoNext = FALSE;

    _textColor = _borderColor;

    _selectedIndex = -1;
    _rightToLeft = FALSE;
    _centerText = FALSE;

    _required = TRUE;

    _listItems = 0;
    _pListBuffer = 0;

    // Register for session events
    RegisterSessionEvent(seEnter, this, (NotifyMethod)&CctControl_ElementUrlList::OnSessionEnter);
    RegisterSessionEvent(seEnterNext, this, (NotifyMethod)&CctControl_ElementUrlList::OnSessionEnterNext);
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementUrlList::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementUrlList::OnSessionNext);
}

CctControl_ElementUrlList::~CctControl_ElementUrlList()
{
    MEM_FREE(_listItems);
    MEM_FREE(_pListBuffer);
    _listItems = NULL;

    MEM_FREE(_url);
    MEM_FREE(_userName);
    MEM_FREE(_password);
    MEM_FREE(_separator);

    if (IsLive())
    {
        GetEngine(this)->CancelUrlRequest(this);
        KillTimer();
    }

    UnregisterSessionEvent(seCanNext, this);
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seEnterNext, this);
    UnregisterSessionEvent(seEnter, this);
}

VOID CctControl_ElementUrlList::OnTimer()
{
    _waitingInvert = !_waitingInvert;
    Repaint();
}

VOID CctControl_ElementUrlList::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (IsLive() && _waitingForResponse)
    {
        if (!_transparent)
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->FillRect(pRect);
        }

        FXRECT rect = *pRect;
        INT w = rect.Right - rect.Left + 1;
        INT h = rect.Bottom - rect.Top + 1;
        rect.Left = rect.Right = rect.Left + w / 2;
        rect.Top = rect.Bottom = rect.Top + h / 2;
        InflateRect(&rect, 16 * _oneSpace, 16 * _oneSpace);
        
        pCanvas->DrawBusy(&rect, _color, _textColor, _waitingInvert);
    }
    else
    {
        CfxControl_List::OnPaint(pCanvas, pRect);
    }
}

VOID CctControl_ElementUrlList::OnUrlReceived(const CHAR *pFileName, UINT ErrorCode)
{
    FXFILEMAP fileMap = {0};
    CfxStream stream;
    CHAR b = 0;

    if (!FxMapFile(pFileName, &fileMap))
    {
        SetCaption("Download failed");
        goto cleanup;
    }

    MEM_FREE(_listItems);
    _listItems = 0;

    if (fileMap.FileSize != 0)
    {
        stream.Write(fileMap.BasePointer, fileMap.FileSize);
        stream.Write1(&b);
        _listItems = AllocString((const CHAR *)stream.GetMemory());
    }

cleanup:
    FxUnmapFile(&fileMap);
    ParseList();
    UpdateScrollBar();

    _waitingForResponse = FALSE;
    Repaint();
}

VOID CctControl_ElementUrlList::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_List::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineField(_elementId, dtText);
#endif
}

VOID CctControl_ElementUrlList::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    F.DefineValue("AutoNext", dtBoolean, &_autoNext, F_FALSE);
    F.DefineValue("LookupList", dtPText, &_listItems, F_NULL);
    F.DefineValue("Required", dtBoolean, &_required, F_FALSE);
    F.DefineValue("RightToLeft", dtBoolean, &_rightToLeft, F_FALSE);
    F.DefineValue("CenterText", dtBoolean, &_centerText, F_FALSE);
    F.DefineValue("Url", dtPText, &_url, F_NULL);
    F.DefineValue("UserName", dtPText, &_userName, F_NULL);
    F.DefineValue("Password", dtPText, &_password, F_NULL);
    F.DefineValue("Separator", dtPText, &_separator, F_NULL);

    F.DefineXOBJECT("Element", &_elementId);

    if (F.IsReader())
    {
        SetSelectedIndex(-1);

        if (_url == NULL)
        {
            SetCaption("<No data: set \"Url\" property>");
        }
        else
        {
            SetCaption("Web List");
        }

        UpdateScrollBar();
    }
}

VOID CctControl_ElementUrlList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
    F.DefineValue("ListItems", dtPText, &_listItems);
    if (F.IsReader())
    {
        ParseList();
        SetSelectedIndex(_selectedIndex);
    }
}

VOID CctControl_ElementUrlList::DefinePropertiesUI(CfxFilerUI &F) 
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
    F.DefineValue("Minimum item height", dtInt32, &_minItemHeight,  "MIN(16);MAX(320)");
    F.DefineValue("Minimum item width",  dtInt32, &_minItemWidth,   "MIN(32);MAX(640)");
    F.DefineValue("Required",     dtBoolean,  &_required);
    F.DefineValue("Result Element", dtGuid,   &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Right to left",dtBoolean,   &_rightToLeft);
    F.DefineValue("Scale for touch", dtBoolean,&_scaleForHitSize);
    F.DefineValue("Scroll width", dtInt32,     &_scrollBarWidth,    "MIN(7);MAX(40)");
    F.DefineValue("Separator",    dtPText,     &_separator);
    F.DefineValue("Show lines",   dtBoolean,   &_showLines);
    F.DefineValue("Text color",   dtColor,     &_textColor);
    F.DefineValue("Top",          dtInt32,     &_top);
    F.DefineValue("Transparent",  dtBoolean,   &_transparent);
    F.DefineValue("Url",             dtPText, &_url, F_NULL);
    F.DefineValue("Url - user name", dtPText,  &_userName, F_NULL);
    F.DefineValue("Url - password",  dtPText,  &_password, F_NULL);
    F.DefineValue("Width",        dtInt32,     &_width);    
#endif
}

VOID CctControl_ElementUrlList::OnSessionEnter(CfxControl *pSender, VOID *pParams)
{
    CctSession *session = GetSession(this);
    if (CompareXGuid(session->GetCurrentScreenId(), session->GetFirstScreenId()) || !_listItems)
    {
        OnSessionEnterNext(pSender, NULL);
    }
}

VOID CctControl_ElementUrlList::OnSessionEnterNext(CfxControl *pSender, VOID *pParams)
{
    if ((_url == NULL) || (strlen(_url) == 0))
    {
        SetCaption("<No data: set \"Url\" property>");
    }
    else
    {
        CfxStream data;
        CctJsonBuilder::WriteSightingToStream(GetSession(this), GetSession(this)->GetCurrentSighting(), data);

        _waitingForResponse = TRUE;
        SetTimer(500);
        CHAR n[1] = "";
        GetEngine(this)->SetUrlRequest(this, _url ? _url : n, _userName ? _userName : n, _password ? _password : n, data);
    }
    Repaint();
}

VOID CctControl_ElementUrlList::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (_required && _selectedIndex == -1)
    {
        GetSession(this)->ShowMessage("Selection is required.");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_ElementUrlList::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    BOOL selected = (_selectedIndex != -1);

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId)  && selected)
    {
        CctAction_MergeAttribute action(this);
        action.SetControlId(_id);

        CHAR *pItem = GetItem(_selectedIndex);
        CHAR *pText = AllocString(pItem);

        if (pText)
        {
            StringReplace(&pText, "\t", (_separator && _separator[0]) ? _separator : "_");
            strcpy(pText, pItem);
            action.SetupAdd(_id, _elementId, dtPText, &pText);
            ((CctSession *)pSender)->ExecuteAction(&action);
            MEM_FREE(pText);
        }
    }
}

UINT CctControl_ElementUrlList::GetItemCount()
{
    return _list.GetCount();
}

CHAR *CctControl_ElementUrlList::GetItem(UINT Index)
{
    return _list.Get(Index);
}

VOID CctControl_ElementUrlList::PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, UINT Index)
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
    CHAR *pItemNoTab = AllocString(pItem);
    CHAR *pItemTabLocation = strchr(pItemNoTab, '\t');
    if (pItemTabLocation)
    {
        *pItemTabLocation = '\0';
    }

    if (pItem)
    {
        pCanvas->DrawTextRect(pFont, &rect, flags, pItemNoTab);
    }
    else
    {
        pCanvas->DrawTextRect(pFont, &rect, flags, "Bad lookup item index");
    }

    MEM_FREE(pItemNoTab);
}

VOID CctControl_ElementUrlList::ParseList()
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

VOID CctControl_ElementUrlList::SetSelectedIndex(INT Index)
{
    _selectedIndex = Index;
}

VOID CctControl_ElementUrlList::OnPenDown(INT X, INT Y)
{
    INT index;
    INT offsetX, offsetY;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        SetSelectedIndex(index);

        Repaint();
    }
}

VOID CctControl_ElementUrlList::OnPenClick(INT X, INT Y)
{
    if (_autoNext && (_selectedIndex != -1))
    {
        GetEngine(this)->KeyPress(KEY_RIGHT);
    }
}
