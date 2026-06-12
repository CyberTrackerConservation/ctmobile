#include "fxApplication.h"
#include "fxUtils.h"
#include "ctGPSTimerList.h"
#include "ctActions.h"

CfxControl *Create_Control_GPSTimerList(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_GPSTimerList(pOwner, pParent);
}

//*************************************************************************************************
// CctControl_GPSTimerList

CctControl_GPSTimerList::CctControl_GPSTimerList(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_List(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_GPSTIMERLIST);
    InitLockProperties("Columns;ItemHeight;Show caption");

    for (int i = 0; i < ARRAYSIZE(_items); i++)
    {
        _items[i].Caption = AllocString(GpsTimerItems[i].Caption);
        _items[i].Timeout = GpsTimerItems[i].Timeout;
        _items[i].Enabled = true;
    }
    RebuildItemMap();

    _rightToLeft = false;
    _autoNext = false;
    _showCaption = true;
    _selectedIndex = -1;
    InitFont(_valueFont, GPS_WAYPOINT_FONT);
   
    // Register for CanNext and Next events
    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_GPSTimerList::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_GPSTimerList::OnSessionNext);
}

CctControl_GPSTimerList::~CctControl_GPSTimerList()
{
    ClearItems();

    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);
}

void CctControl_GPSTimerList::ClearItems()
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

void CctControl_GPSTimerList::RebuildItemMap()
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

void CctControl_GPSTimerList::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if (_selectedIndex == -1)
    {
        *((bool *)pParams) = false;
    }
}

void CctControl_GPSTimerList::OnSessionNext(CfxControl *pSender, void *pParams)
{
    if (_selectedIndex == -1) return;

    CctAction_ConfigureGPS action(this);

    CctSession *session = (CctSession *)pSender;
    FXGPS_ACCURACY waypointAccuracy = {};
    action.Setup(false, session->GetSightingAccuracy(), session->GetSightingFixCount(),
                 &waypointAccuracy, _items[_itemMap[_selectedIndex]].Timeout);

    session->ExecuteAction(&action);
}

void CctControl_GPSTimerList::OnPenClick(int X, int Y)
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

void CctControl_GPSTimerList::DefineResources(CfxFilerResource &F)
{
}

void CctControl_GPSTimerList::DefineProperties(CfxFiler &F)
{
    CfxControl_List::DefineProperties(F);

    F.DefineValue("AutoNext", dtBoolean, &_autoNext, F_FALSE);
    F.DefineValue("RightToLeft", dtBoolean, &_rightToLeft, F_FALSE);
    F.DefineValue("ShowCaption", dtBoolean, &_showCaption, F_TRUE);
    F.DefineXFONT("ValueFont", &_valueFont, GPS_WAYPOINT_FONT);

    for (int i = 0; i < ARRAYSIZE(_items); i++)
    {
        char index[5], name[32];
        itoa(i + 1, index, 10);

        strcpy(name, "Caption_");
        strcat(name, index);
        F.DefineValue(name, dtPText, &_items[i].Caption);

        strcpy(name, "Timeout_");
        strcat(name, index);
        F.DefineValue(name, dtInt32, &_items[i].Timeout);

        strcpy(name, "Enabled_");
        strcat(name, index);
        F.DefineValue(name, dtBoolean, &_items[i].Enabled);
    }

    if (F.IsReader())
    {
        RebuildItemMap();

        _selectedIndex = -1;
        UpdateScrollBar();
    }
}

void CctControl_GPSTimerList::DefineState(CfxFiler &F)
{
    CfxControl_List::DefineState(F);
    F.DefineValue("SelectedIndex", dtInt32, &_selectedIndex);
}

void CctControl_GPSTimerList::DefinePropertiesUI(CfxFilerUI &F) 
{
}

int CctControl_GPSTimerList::GetItemCount()
{
    return _itemMapCount;
}

void CctControl_GPSTimerList::PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index)
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
    if (_showCaption)
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

    {
        FXRECT rect = *pRect;
        int itemHeight = rect.Bottom - rect.Top + 1;

        if (_showCaption)
        {
            if (_rightToLeft)
            {
                rect.Right = rect.Left + itemHeight;
            }
            else
            {
                rect.Left = rect.Right - itemHeight;
            }
        }
        else
        {
            rect.Left = rect.Left + (rect.Right - rect.Left - itemHeight) / 2;
            rect.Right = rect.Left + itemHeight;
        }

        InflateRect(&rect, -_oneSpace, -_oneSpace);

        FXFONTRESOURCE *font = resource->GetFont(this, _valueFont);
        if (font)
        {
            pCanvas->State.LineWidth = GetBorderLineWidth();
            DrawWaypointIcon(pCanvas, &rect, font, _items[_itemMap[Index]].Timeout, _color, _textColor, false, _items[_itemMap[Index]].Timeout == 0, selected);
            resource->ReleaseFont(font);
        }
    }
}
