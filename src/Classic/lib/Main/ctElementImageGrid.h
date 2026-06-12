#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "ctElement.h"

//
// CctControl_ElementImageGrid
//
class CctControl_ElementImageGrid: public CfxControl
{
protected:
    int _rows = 0, _columns = 0;
    XBITMAP _bitmap = 0;
    bool _transparentImage = false;
    int _selectedX = 0, _selectedY = 0;
    bool GetItemRect(int X, int Y, FXRECT *pRect);
    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
    virtual bool IsValidItem(int X, int Y)=0;
    virtual void DoSessionNext(CfxControl *pSender)=0;
public:
    CctControl_ElementImageGrid(CfxPersistent *pOwner, CfxControl *pParent);
    virtual ~CctControl_ElementImageGrid();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    void OnPenDown(int X, int Y) override;
    void OnPenUp(int X, int Y) override;
};

//
// CctControl_ElementImageGrid1
//
const GUID GUID_CONTROL_ELEMENTIMAGEGRID1 = {0xd2bc4b92, 0xae3a, 0x4ace, {0x9c, 0xa4, 0xaf, 0xf1, 0x16, 0xbd, 0xb1, 0x66}};

enum class ELEMENTIMAGEGRIDFORMAT : int8_t {Excel=0, Number};

class CctControl_ElementImageGrid1: public CctControl_ElementImageGrid
{
protected:
    XGUIDLIST _elements;
    bool IsValidItem(int X, int Y) override;
    void DoSessionNext(CfxControl *pSender) override;
public:
    CctControl_ElementImageGrid1(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};
extern CfxControl *Create_Control_ElementImageGrid1(CfxPersistent *pOwner, CfxControl *pParent);

//
// CctControl_ElementImageGrid2
//
const GUID GUID_CONTROL_ELEMENTIMAGEGRID2 = {0xfb310e77, 0xa4ae, 0x4840, {0x93, 0x5b, 0x1f, 0x7e, 0x57, 0x87, 0xc1, 0xc3}};

class CctControl_ElementImageGrid2: public CctControl_ElementImageGrid
{
protected:
    XGUID _elementId = 0;
    ELEMENTIMAGEGRIDFORMAT _format = ELEMENTIMAGEGRIDFORMAT::Excel;
    bool IsValidItem(int X, int Y) override;
    void DoSessionNext(CfxControl *pSender) override;
public:
    CctControl_ElementImageGrid2(CfxPersistent *pOwner, CfxControl *pParent);

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};
extern CfxControl *Create_Control_ElementImageGrid2(CfxPersistent *pOwner, CfxControl *pParent);

