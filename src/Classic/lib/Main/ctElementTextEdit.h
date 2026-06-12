#pragma once
#include "pch.h"
#include "fxMemo.h"
#include "ctElement.h"
#include "ctSession.h"

const GUID GUID_CONTROL_ELEMENTEDIT = {0x49728018, 0xd9f6, 0x49b8, {0x86, 0xe4, 0x3e, 0xa4, 0x9a, 0x29, 0xf4, 0xbc}};

//
// CctControl_ElementTextEdit
//
class CctControl_ElementTextEdit: public CfxControl_Memo
{
protected:
    XGUID _elementId = 0;
    bool _required = false;
    bool _oneLine = false;
    int _maxLength = 0;

    CctRetainState _retainState;

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_ElementTextEdit(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementTextEdit();

    void SetCaption(const char *Caption) override;

    void ResetState() override;

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnPenClick(int X, int Y) override;
    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;

    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;
};

extern CfxControl *Create_Control_ElementTextEdit(CfxPersistent *pOwner, CfxControl *pParent);
