#pragma once
#include "pch.h"
#include "fxList.h"

//*************************************************************************************************
// CctControl_ComPortList

const GUID GUID_CONTROL_COMPORTLIST = {0x945aed3d, 0x3df6, 0x4aec, {0xa3, 0xba, 0xe9, 0x6d, 0x26, 0x74, 0xaf, 0x0}};

struct COMPORTITEM
{
    const char *Caption = nullptr;
    BYTE PortId = 0;
    bool Enabled = false;
};

const COMPORTITEM ComPortItems[] = 
{
    { "Detect",  0, true },
    { "COM 1",   1, true },
    { "COM 2",   2, true },
    { "COM 3",   3, true },
    { "COM 4",   4, true },
    { "COM 5",   5, true },
    { "COM 6",   6, true },
    { "COM 7",   7, true },
    { "COM 8",   8, true },
    { "COM 9",   9, true },
};

enum class PortDeviceType : int8_t {None=0, Gps, RangeFinder};

//
// CctControl_ComPortList
//
class CctControl_ComPortList: public CfxControl_List
{
protected:
    bool _autoNext = false;
    int _selectedIndex = 0;
    BYTE _defaultPort = 0;
    bool _rightToLeft = false;
    COMPORTITEM _items[ARRAYSIZE(ComPortItems)] = {};
    int _itemMap[ARRAYSIZE(ComPortItems)] = {};
    int _itemMapCount = 0;
    BYTE _paramsId = 0;
    PortDeviceType _portDeviceType = PortDeviceType::None;
    char *_resultGlobalValue = nullptr;

    void RebuildItemMap();
    void ClearItems();
    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_ComPortList(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ComPortList();

    void OnPenClick(int X, int Y) override;

    BYTE GetPortId();
    void SetPortId(BYTE Value);

    BYTE GetParamsId()           { return _paramsId;  }
    void SetParamsId(BYTE Value) { _paramsId = Value; }

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    int GetItemCount() override;
    void PaintItem(CfxCanvas *pCanvas, FXRECT *pRect, FXFONTRESOURCE *pFont, int Index) override;
};

extern CfxControl *Create_Control_ComPortList(CfxPersistent *pOwner, CfxControl *pParent);

