#pragma once
#include "pch.h"
#include "fxTitleBar.h"
#include "ctElementKeypad.h"

struct NUMBEREDITOR_PARAMS
{
    int ControlId = 0;
    int Index = 0;
    BYTE Digits = 0;
    BYTE Decimals = 0;
    BYTE Fraction = 0;
    double Value = 0.0;
    int MinValue = 0, MaxValue = 0;
    bool FormulaMode = false;
    char Title[64] = {};
};

//*************************************************************************************************

const GUID GUID_DIALOG_NUMBEREDITOR = {0x62b46c4c, 0xf815, 0x45b1, {0xb0, 0x9a, 0x6a, 0x1f, 0x5b, 0x7a, 0xb0, 0x53}};

//
// CctDialog_NumberEditor
//
class CctDialog_NumberEditor: public CfxDialog
{
protected:
    NUMBEREDITOR_PARAMS _source;

    CfxControl_TitleBar *_titleBar = nullptr;
    CctControl_ElementKeypad *_keypad = nullptr;
public:
    CctDialog_NumberEditor(CfxPersistent *pOwner, CfxControl *pParent);

    void Init(CfxControl *pSender, void *pParam, int ParamSize) override;

    void DefineProperties(CfxFiler &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void OnCloseDialog(bool *pHandled) override;
};

extern CfxDialog *Create_Dialog_NumberEditor(CfxPersistent *pOwner, CfxControl *pParent);
