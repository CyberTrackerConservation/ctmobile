#pragma once
#include "pch.h"
#include "fxList.h"

const GUID GUID_CONTROL_GPSTIMERLIST = {0xa65f0e62, 0x76a, 0x43b6, {0x80, 0xf2, 0xc3, 0x7a, 0x57, 0x4f, 0x91, 0xf3}};

struct GPSTIMERITEM
{
    const char *Caption = nullptr;
    int Timeout = 0;
    bool Enabled = false;
};

const GPSTIMERITEM GpsTimerItems[] = 
{
    { "Off",        0,    true },
    { "1 second",   1,    true },
    { "5 seconds",  5,    true },
    { "10 seconds", 10,   true },
    { "30 seconds", 30,   true },
    { "1 minute",   60,   true },
    { "2 minutes",  60*2, true },
    { "3 minutes",  60*3, true },
    { "5 minutes",  60*5, true }
};

//
// CctControl_GPSTimerList
//
class CctControl_GPSTimerList: public CfxControl_List
{
protected:
    bool _autoNext = false;
    bool _showCaption = false;
    int _selectedIndex = 0;
    XFONT _valueFont = 0;
    bool _rightToLeft = false;
    GPSTIMERITEM _items[ARRAYSIZE(GpsTimerItems)] = {};
    int _itemMap[ARRAYSIZE(GpsTimerItems)] = {};
    int _itemMapCount = 0;

    void RebuildItemMap();
    void ClearItems();
    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_GPSTimerList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_GPSTimerList();

    void OnPenClick(int X, int Y) override;

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    int GetItemCount() override;
    void PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index) override;
};

extern CfxControl *Create_Control_GPSTimerList(CfxPersistent *pOwner, CfxControl *pParent);

