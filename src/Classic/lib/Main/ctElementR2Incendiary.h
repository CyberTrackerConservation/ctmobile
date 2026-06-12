#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "ctElement.h"

//*************************************************************************************************
//
// CctControl_ElementR2Incendiary
//

const GUID GUID_CONTROL_ELEMENTR2INCENDIARY = {0xcf2a83ab, 0x6b19, 0x4932, {0xa9, 0xdb, 0x10, 0x10, 0x8c, 0x78, 0x6a, 0x45}};

enum class R2INCENDIARY_PARSE_STATE : int8_t {Start=0, Counter, Length, Data, Checksum};
enum class R2INCENDIARY_SAVE_ACTION : int8_t {None=0, Waypoint, Sighting};

class CctControl_ElementR2Incendiary: public CfxControl_Panel
{
protected:
    BYTE _portId = 0;
    FX_COMPORT _comPort;
    bool _connected = false, _lastConnected = false;
    bool _fastSave = false;

    R2INCENDIARY_PARSE_STATE _state = R2INCENDIARY_PARSE_STATE::Start;
    R2INCENDIARY_SAVE_ACTION _onCapsuleAction = R2INCENDIARY_SAVE_ACTION::None;
    R2INCENDIARY_SAVE_ACTION _onBlankAction = R2INCENDIARY_SAVE_ACTION::None;
    R2INCENDIARY_SAVE_ACTION _onStatusAction = R2INCENDIARY_SAVE_ACTION::None;

    BYTE _counter = 0;
    int _dataIndex = 0;
    BYTE _dataLength = 0;
    BYTE _data[256] = {};
    BYTE _expectedChecksum = 0;

    bool _hasLastCounter = false;
    int _lastCounter = 0;
    int _heartbeatCount = 0;

    char *_resultHeartbeatCount = nullptr;
    char *_resultDropRate = nullptr;
    char *_resultCapsuleCount = nullptr;
    char *_resultCapsuleRollCount = nullptr;
    char *_resultCapsuleFlightTotal = nullptr;
    char *_resultBlankCount = nullptr;
    char *_resultLostPackets = nullptr;
    char *_resultBadPackets = nullptr;
    char *_resultNoGPS = nullptr;

    XGUID _statusElementId = 0;

    void Reset();
    void IncGlobalValue(char *pName);
    void ParsePacket(BYTE *pData, int Length);
    void OnSessionNext(CfxControl *pSender, void *pParams);
    void ExecuteAction(R2INCENDIARY_SAVE_ACTION Action);
public:
    CctControl_ElementR2Incendiary(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementR2Incendiary();

    void OnPortData(BYTE PortId, BYTE *pData, int Length) override;

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
    void OnTimer() override;
};

extern CfxControl *Create_Control_ElementR2Incendiary(CfxPersistent *pOwner, CfxControl *pParent);

