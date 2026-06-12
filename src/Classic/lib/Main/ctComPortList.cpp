#include "fxApplication.h"
#include "fxUtils.h"
#include "ctComPortList.h"
#include "ctActions.h"

//*************************************************************************************************

CfxControl *Create_Control_ComPortList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ComPortList(pOwner, pParent);
}

//*************************************************************************************************
// CctControl_ComPortList

CctControl_ComPortList::CctControl_ComPortList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_COMPORTLIST);
    InitLockProperties("Font;ItemHeight;Port device type");

    for (int i = 0; i < ARRAYSIZE(_items); i++)
    {
        _items[i].Caption = AllocString(ComPortItems[i].Caption);
        _items[i].PortId = ComPortItems[i].PortId;
        _items[i].Enabled = true;
    }
    RebuildItemMap();

    _rightToLeft = false;
    _autoNext = false;
    _selectedIndex = -1;
    _defaultPort = 255;
    _itemHeight = 20;
    _portDeviceType = PortDeviceType::None;
    _paramsId = 1;

    _resultGlobalValue = nullptr;

    // Register for CanNext and Next events
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ComPortList::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ComPortList::OnSessionNext);
}

CctControl_ComPortList::~CctControl_ComPortList()
{
    MEM_FREE(_resultGlobalValue);
    _resultGlobalValue = nullptr;

    ClearItems();

    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);
}

void CctControl_ComPortList::ClearItems()
{
    _itemMapCount = 0;
    memset(&_itemMap, 0xFF, sizeof(_itemMap));

    for (int i = 0; i < ARRAYSIZE(_items); i++)
    {
        MEM_FREE((char*)_items[i].Caption);
        _items[i].Caption = nullptr;
    }

    memset(&_items, 0, sizeof(_items));
}

void CctControl_ComPortList::RebuildItemMap()
{
    _itemMapCount = 0;
    memset(&_itemMap, 0xFF, sizeof(_itemMap));

    for (int i = 0; i < ARRAYSIZE(_items); i++)
    {
        if (_items[i].Enabled)
        {
            _itemMap[_itemMapCount] = i;
            _itemMapCount++;
        }
    }
}

void CctControl_ComPortList::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if (_selectedIndex == -1)
    {
        *((bool *)pParams) = false;
    }
}

void CctControl_ComPortList::OnSessionNext(CfxControl *pSender, void *pParams)
{
    if (_selectedIndex == -1) return;

    // Set the global value
    if (_resultGlobalValue != nullptr)
    {
        GetSession(this)->SetGlobalValue(this, _resultGlobalValue, GetPortId());
    }

}

void CctControl_ComPortList::OnPenClick(int X, int Y)
{
    int index;
    int offsetX, offsetY;

    if (HitTest(X, Y, &index, &offsetX, &offsetY))
    {
        _selectedIndex = (index != -1) ? index : _selectedIndex;
        Repaint();
    }

    if (_autoNext && (_selectedIndex != -1))
    {
        GetEngine(this)->KeyPress(KEY_RIGHT, 100);
    }
}

void CctControl_ComPortList::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ComPortList::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    F.DefineValue("AutoNext", dtBoolean, &_autoNext, F_FALSE);
    F.DefineValue("RightToLeft", dtBoolean, &_rightToLeft, F_FALSE);
    F.DefineValue("PortDeviceType", dtByte, &_portDeviceType, F_ZERO);
    F.DefineValue("ParamsId", dtByte, &_paramsId, F_ONE);
    F.DefineValue("DefaultPort", dtByte, &_defaultPort, "FF");
    F.DefineValue("ResultGlobalValue", dtPText, &_resultGlobalValue, "");

    if (F.IsReader())
    {
        _paramsId = max(1, _paramsId);

        RebuildItemMap();

        _selectedIndex = -1;
        if ((_defaultPort >= 0) && (_defaultPort <= ARRAYSIZE(ComPortItems)))
        {
            _selectedIndex = _defaultPort;
        }

        UpdateScrollBar();
    }
}

void CctControl_ComPortList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
}

void CctControl_ComPortList::DefinePropertiesUI(CfxFilerUI &F) 
{
}

int CctControl_ComPortList::GetItemCount()
{
    return _itemMapCount;
}

void CctControl_ComPortList::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index)
{
    CfxResource *resource = GetResource();
    int itemHeight = pRect->Bottom - pRect->Top + 1;

    // Paint if selected
    bool selected = ((int)Index == _selectedIndex);
    if (selected)
    {
        pCanvas->State.BrushColor = pCanvas->InvertColor(_color);
        pCanvas->FillRect(pRect);
    }

    // Draw the caption
//    if (_showCaption)
    {
        FXRECT rect = *pRect;
        int flags = TEXT_ALIGN_VCENTER;

        if (_rightToLeft)
        {
            rect.Right -= 2 * _oneSpace;
            flags |= TEXT_ALIGN_RIGHT;
        }
        else
        {
            rect.Left += 2 * _oneSpace;
        }
        
        pCanvas->State.TextColor = pCanvas->InvertColor(_textColor, selected);
        pCanvas->DrawTextRect(pFont, &rect, flags, _items[_itemMap[Index]].Caption);
    }
}

BYTE CctControl_ComPortList::GetPortId()
{
    if (_selectedIndex != -1) 
    {
        return _items[_itemMap[_selectedIndex]].PortId;
    }
    else 
    {
        return 0;
    }
}

void CctControl_ComPortList::SetPortId(BYTE Value)
{
    if (Value == 0) 
    {
        _selectedIndex = -1;
    }
    else
    {
        _selectedIndex = Value;
    }
}
