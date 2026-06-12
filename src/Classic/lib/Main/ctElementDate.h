#pragma once

#include "pch.h"
#include "fxControl.h"
#include "fxList.h"
#include "ctElement.h"
#include "ctSession.h"
#include "ctNavigate.h"

const GUID GUID_CONTROL_ELEMENTDATE = { 0x5dff7d37, 0x8ba4, 0x4b0a, { 0xa7, 0x92, 0x16, 0xbe, 0x53, 0x9c, 0xa6, 0xba } };

//
// CctControl_ElementDate
//
class CctControl_ElementDate: public CfxControl
{
protected:
    XGUID _elementId = 0;
    UINT16 _year = 0, _month = 0, _day = 0;
    XFONT _titleFont = 0;
    COLOR _titleTextColor = 0, _titleBackgroundColor = 0, _buttonTextColor = 0;

    CfxControl_Button *_backButton = nullptr, *_nextButton = nullptr;
    CfxControl_Button *_monthButtons[12] = {};
    CfxControl_Button *_dayButtons[31] = {};
    CfxControl_Button *_todayButton = nullptr;
    CfxControl_Panel *_panelYearTitle = nullptr, *_panelMonthTitle = nullptr, *_panelDayTitle = nullptr;
    CfxControl_Panel *_panelYear = nullptr, *_panelMonth = nullptr, *_panelDay = nullptr;

    void OnBackClick(CfxControl *pSender, void *pParams);
    void OnNextClick(CfxControl *pSender, void *pParams);
    void OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    void OnMonthClick(CfxControl *pSender, void *pParams);
    void OnDayClick(CfxControl *pSender, void *pParams);
    void OnTodayClick(CfxControl *pSender, void *pParams);

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);

    void Update();

public:
    CctControl_ElementDate(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementDate();

    void DefineProperties(CfxFiler &F) override;
    void DefineState(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;

    void SetBounds(int Left, int Top, int Width, int Height) override;
};

extern CfxControl *Create_Control_ElementDate(CfxPersistent *pOwner, CfxControl *pParent);
