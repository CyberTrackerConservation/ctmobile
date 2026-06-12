#pragma once
#include "pch.h"
#include "fxPanel.h"

//*************************************************************************************************
//
// CfxControl_Button
//
const GUID GUID_CONTROL_BUTTON = {0x1758d8b1, 0xc5c4, 0x4b81, {0xa9, 0x58, 0x42, 0xbf, 0xb7, 0xd0, 0x9a, 0xfe}};

class CfxControl_Button: public CfxControl_Panel
{
protected:
	bool _penDown = false, _penOver = false, _down = false;
    BYTE _groupId = 0;
public:
    CfxControl_Button(CfxPersistent *pOwner, CfxControl *pParent);

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

	void OnPenDown(int X, int Y) override;
    void OnPenUp(int X, int Y) override;
    void OnPenMove(int X, int Y) override;

    bool GetDown()                    { return _down || (_penDown && _penOver); }
    void SetDown(const bool Value);
    BYTE GetGroupId()                 { return _groupId;  }
    void SetGroupId(const BYTE Value) { _groupId = Value; }

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
};

extern CfxControl *Create_Control_Button(CfxPersistent *pOwner, CfxControl *pParent);
