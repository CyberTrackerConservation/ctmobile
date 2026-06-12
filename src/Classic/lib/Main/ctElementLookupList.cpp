#include "fxApplication.h"
#include "fxUtils.h"
#include "fxTranslate.h"
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

    _resultGlobalValue = nullptr;

    _autoNext = false;

    _textColor = _borderColor;

    _selectedIndex = -1;
    _rightToLeft = false;
    _centerText = false;

    _required = true;

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
    _listItems = nullptr;

    MEM_FREE(_resultGlobalValue);
	_resultGlobalValue = nullptr;

    UnregisterSessionEvent(seCanNext, this);
    UnregisterSessionEvent(seNext, this);
}

void CctControl_ElementLookupList::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementLookupList::DefineProperties(CfxFiler &F)
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

void CctControl_ElementLookupList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
    if (F.IsReader())
    {
        SetSelectedIndex(_selectedIndex);
    }
}

void CctControl_ElementLookupList::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_ElementLookupList::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if (_required && _selectedIndex == -1)
    {
        GetSession(this)->ShowMessage(fxTranslate("Selection is required"));
        *((bool *)pParams) = false;
    }
}

void CctControl_ElementLookupList::OnSessionNext(CfxControl *pSender, void *pParams)
{
    bool selected = (_selectedIndex != -1);

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId)  && selected)
    {
        CctAction_MergeAttribute action(this);
        action.SetControlId(_id);

        char *pItem = GetItem(_selectedIndex);

        char *pText = (char *)MEM_ALLOC(strlen(pItem) + 1);
        if (pText)
        {
            strcpy(pText, pItem);
            action.SetupAdd(_id, _elementId, dtPText, &pText);
            ((CctSession *)pSender)->ExecuteAction(&action);
            MEM_FREE(pText);
        }
    }
}

int CctControl_ElementLookupList::GetItemCount()
{
    return _list.GetCount();
}

char *CctControl_ElementLookupList::GetItem(int Index)
{
    return _list.Get(Index);
}

void CctControl_ElementLookupList::PaintItem(CfxCanvas *pCanvas, FXRECT *pItemRect, FXFONTRESOURCE *pFont, int Index)
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
    if (pItem)
    {
        pCanvas->DrawTextRect(pFont, &rect, flags, pItem);
    }
    else
    {
        pCanvas->DrawTextRect(pFont, &rect, flags, "Bad lookup item index");
    }
}

void CctControl_ElementLookupList::ParseLookupList()
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

void CctControl_ElementLookupList::SetSelectedIndex(int Index)
{
    _selectedIndex = Index;

    // Set the global value
    if (IsLive() && (_resultGlobalValue != nullptr))
    {
        GetSession(this)->SetGlobalValue(this, _resultGlobalValue, Index + 1);
    }
}

void CctControl_ElementLookupList::OnPenClick(int X, int Y)
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
