#include "fxApplication.h"
#include "fxUtils.h"
#include "fxMemo.h"
#include "ctSession.h"
#include "ctActions.h"
#include "ctElement.h"
#include "ctDialog_NumberEditor.h"
#include "ctElementList.h"

//*************************************************************************************************
// CctDialog_NumberEditor

CfxDialog *Create_Dialog_NumberEditor(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctDialog_NumberEditor(pOwner, pParent);
}

CctDialog_NumberEditor::CctDialog_NumberEditor(CfxPersistent *pOwner, CfxControl *pParent): CfxDialog(pOwner, pParent)
{
    InitControl(&GUID_DIALOG_NUMBEREDITOR);

    _dock = ControlDock::Fill;

    memset(&_source, 0, sizeof(_source));

    _titleBar = new CfxControl_TitleBar(pOwner, this);
    _titleBar->SetComposite(true);
    _titleBar->SetCaption("Number Editor");
    _titleBar->SetShowBattery(false);
    _titleBar->SetShowExit(false);
    _titleBar->SetShowTime(false);
    _titleBar->SetShowOkay(true);
    _titleBar->SetDock(ControlDock::Top);
    _titleBar->SetBounds(0, 0, 20, 24);

    _keypad = new CctControl_ElementKeypad(pOwner, this);
    _keypad->SetComposite(true);
    _keypad->SetBorderStyle(ControlBorderStyle::None);
    _keypad->SetBorderWidth(0);
    _keypad->SetDock(ControlDock::Fill);
    _keypad->SetupDisplay(60, 0xFFFFFF);
    _keypad->SetupButtons(0, 0, 1, ControlBorderStyle::Single, 0xFFFFFF, 0);
    _keypad->SetButtonFont(F_FONT_DEFAULT_XXLB);
    _keypad->SetFont(F_FONT_DEFAULT_XXLB);
}

void CctDialog_NumberEditor::Init(CfxControl *pSender, void *pParam, int ParamSize)
{ 
    _source = *(NUMBEREDITOR_PARAMS *)pParam;
    FX_ASSERT(_source.ControlId > 0);

    _titleBar->SetCaption(_source.Title);

    if (_source.FormulaMode)
    {
        _keypad->SetupDisplay(50, 0xFFFFFF);
        _keypad->SetupButtons(0, 0, 1, ControlBorderStyle::Single, 0xFFFFFF, 0);
    }
    else
    {
        _keypad->SetupDisplay(60, 0xFFFFFF);
        _keypad->SetupButtons(0, 0, 1, ControlBorderStyle::Single, 0xFFFFFF, 0);
    }

    _keypad->Setup(_source.Digits, _source.Decimals, _source.Fraction, _source.MinValue, _source.MaxValue, _source.FormulaMode, false);
    _keypad->SetValue(_source.Value);
}

void CctDialog_NumberEditor::OnCloseDialog(bool *pHandled)
{
    CctSession *session = GetSession(this);

    *pHandled = true;

    if (_keypad->FinalizeValue() && _keypad->TestValueValid())
    {
        int sourceControlId = _source.ControlId;
        int sourceIndex = _source.Index;
        int value, valuePoint;
        bool valueNegative;

        DecodeDouble(_keypad->GetValue(), _source.Decimals, &value, &valuePoint, &valueNegative);

        if (valueNegative)
        {
            value = -value;
        }

        session->Reconnect(QString(), false);

        CctControl_ElementList *sourceControl = (CctControl_ElementList *)(session->GetWindow()->FindControl(sourceControlId));
        if (sourceControl)
        {
            sourceControl->SetItemValue(sourceIndex, value);
        }
    }

    session->Repaint();
}

void CctDialog_NumberEditor::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    if (F.DefineBegin("Title"))
    {
        _titleBar->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.DefineBegin("Keypad"))
    {
        _keypad->DefineProperties(F);
        F.DefineEnd();
    }

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);
    }
}

void CctDialog_NumberEditor::DefineResources(CfxFilerResource &F)
{
}

