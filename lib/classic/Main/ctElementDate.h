/*************************************************************************************************/
//                                                                                                *
//   Copyright (C) 2005              Software                                                     *
//                                                                                                *
//   Original author: Justin Steventon (justin@steventon.com)                                     *
//                                                                                                *
//   You may retrieve the latest version of this file via the              Software home page,    *
//   located at: http://www.            .com.                                                     *
//                                                                                                *
//   The contents of this file are used subject to the Mozilla Public License Version 1.1         *
//   (the "License"); you may not use this file except in compliance with the License. You may    *
//   obtain a copy of the License from http://www.mozilla.org/MPL.                                *
//                                                                                                *
//   Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY  *
//   OF ANY KIND, either express or implied. See the License for the specific language governing  *
//   rights and limitations under the License.                                                    *
//                                                                                                *
//************************************************************************************************/

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
    XGUID _elementId;
    UINT16 _year, _month, _day;
    XFONT _titleFont;
    COLOR _titleTextColor, _titleBackgroundColor, _buttonTextColor;

    CfxControl_Button *_backButton, *_nextButton;
    CfxControl_Button *_monthButtons[12];
    CfxControl_Button *_dayButtons[31];
    CfxControl_Button *_todayButton;
    CfxControl_Panel *_panelYearTitle, *_panelMonthTitle, *_panelDayTitle;
    CfxControl_Panel *_panelYear, *_panelMonth, *_panelDay;

    VOID OnBackClick(CfxControl *pSender, VOID *pParams);
    VOID OnNextClick(CfxControl *pSender, VOID *pParams);
    VOID OnBackPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnNextPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    VOID OnMonthClick(CfxControl *pSender, VOID *pParams);
    VOID OnDayClick(CfxControl *pSender, VOID *pParams);
    VOID OnTodayClick(CfxControl *pSender, VOID *pParams);

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);

    VOID Update();

public:
    CctControl_ElementDate(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementDate();

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
};

extern CfxControl *Create_Control_ElementDate(CfxPersistent *pOwner, CfxControl *pParent);
