#include "fxApplication.h"
#include "fxUtils.h"
#include "fxTranslate.h"
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

    _url = _userName = _password = nullptr;
    _waitingForResponse = _waitingInvert = false;
    _separator = nullptr;

    _autoNext = false;

    _textColor = _borderColor;

    _selectedIndex = -1;
    _rightToLeft = false;
    _centerText = false;

    _required = true;

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
    _listItems = nullptr;

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

void CctControl_ElementUrlList::OnTimer()
{
    _waitingInvert = !_waitingInvert;
    Repaint();
}

void CctControl_ElementUrlList::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (IsLive() && _waitingForResponse)
    {
        if (!_transparent)
        {
            pCanvas->State.BrushColor = _color;
            pCanvas->FillRect(pRect);
        }

        FXRECT rect = *pRect;
        int w = rect.Right - rect.Left + 1;
        int h = rect.Bottom - rect.Top + 1;
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

void CctControl_ElementUrlList::OnUrlReceived(const char *pFileName, int ErrorCode)
{
    FXFILEMAP fileMap = {0};
    CfxStream stream;
    char b = 0;

    if (!FxMapFile(pFileName, &fileMap))
    {
        SetCaption(fxTranslate("Download failed"));
        goto cleanup;
    }

    MEM_FREE(_listItems);
    _listItems = 0;

    if (fileMap.FileSize != 0)
    {
        stream.Write(fileMap.BasePointer, fileMap.FileSize);
        stream.Write1(&b);
        _listItems = AllocString((const char *)stream.GetMemory());
    }

cleanup:
    FxUnmapFile(&fileMap);
    ParseList();
    UpdateScrollBar();

    _waitingForResponse = false;
    Repaint();
}

void CctControl_ElementUrlList::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementUrlList::DefineProperties(CfxFiler &F)
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

        if (_url == nullptr)
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

void CctControl_ElementUrlList::DefineState(CfxFiler &F)
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

void CctControl_ElementUrlList::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_ElementUrlList::OnSessionEnter(CfxControl *pSender, void *pParams)
{
    CctSession *session = GetSession(this);
    if (CompareXGuid(session->GetCurrentScreenId(), session->GetFirstScreenId()) || !_listItems)
    {
        OnSessionEnterNext(pSender, nullptr);
    }
}

void CctControl_ElementUrlList::OnSessionEnterNext(CfxControl *pSender, void *pParams)
{
    if ((_url == nullptr) || (strlen(_url) == 0))
    {
        SetCaption("<No data: set \"Url\" property>");
    }
    else
    {
        CfxStream data;
        CctJsonBuilder::WriteSightingToStream(GetSession(this), GetSession(this)->GetCurrentSighting(), data);

        _waitingForResponse = true;
        SetTimer(500);
        char n[1] = "";
        GetEngine(this)->SetUrlRequest(this, _url ? _url : n, _userName ? _userName : n, _password ? _password : n, data);
    }
    Repaint();
}

void CctControl_ElementUrlList::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if (_required && _selectedIndex == -1)
    {
        GetSession(this)->ShowMessage(fxTranslate("Selection is required"));
        *((bool *)pParams) = false;
    }
}

void CctControl_ElementUrlList::OnSessionNext(CfxControl *pSender, void *pParams)
{
    bool selected = (_selectedIndex != -1);

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId)  && selected)
    {
        CctAction_MergeAttribute action(this);
        action.SetControlId(_id);

        char *pItem = GetItem(_selectedIndex);
        char *pText = AllocString(pItem);

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

int CctControl_ElementUrlList::GetItemCount()
{
    return _list.GetCount();
}

char *CctControl_ElementUrlList::GetItem(int Index)
{
    return _list.Get(Index);
}

void CctControl_ElementUrlList::PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index)
{
    int itemHeight = pItemRect->Bottom - pItemRect->Top + 1;

    // Paint if selected
    bool selected = ((int)Index == _selectedIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pItemRect);
    }

    // Draw the caption
    int flags = TEXT_ALIGN_VCENTER;
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

    bool boundary = false;
    char *pItem = GetItem(Index);
    char *pItemNoTab = AllocString(pItem);
    char *pItemTabLocation = strchr(pItemNoTab, '\t');
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

void CctControl_ElementUrlList::ParseList()
{
    _list.Clear();

    if (_listItems == nullptr)
        return;

    char* a_str = _listItems;
    char a_delim[3] = "\n;";

    if (_pListBuffer)
    {
        MEM_FREE(_pListBuffer);
        _pListBuffer = nullptr;
    }
    _pListBuffer = (char *)MEM_ALLOC(strlen(a_str) + 1);
    strcpy(_pListBuffer, _listItems);

    char* token = strtok(_pListBuffer, a_delim);
    while (token)
    {
        TrimString(token);
        if ((token != nullptr) && (*token != '\0'))
        {
            _list.Add(token);
        }
        token = strtok(0, a_delim);
    }
}

void CctControl_ElementUrlList::SetSelectedIndex(int Index)
{
    _selectedIndex = Index;
}

void CctControl_ElementUrlList::OnPenClick(int X, int Y)
{
    int index;
    int offsetX, offsetY;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        SetSelectedIndex(index);
        Repaint();
    }

    if (_autoNext && (_selectedIndex != -1))
    {
        GetEngine(this)->KeyPress(KEY_RIGHT, 100);
    }
}
