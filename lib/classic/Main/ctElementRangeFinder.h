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
#include "fxRangeFinder.h"
#include "fxButton.h"
#include "ctSession.h"

const GUID GUID_CONTROL_ELEMENTRANGEFINDER = {0x8f3f0fc9, 0x70cf, 0x4b7d, { 0xaa, 0xd2, 0x48, 0x66, 0x6a, 0x2, 0xcd, 0x6f}};

//
// CctControl_ElementRangeFinder
//
class CctControl_ElementRangeFinder: public CfxControl_RangeFinder
{
protected:
    BOOL _required, _autoNext;
    BOOL _clearOnEnter;
    BOOL _snapDateTime, _snapGPS;

    FXDATETIME _snapDateTimeValue;
    FXGPS _snapGPSValue;

    CHAR *_rangeFinderId;

    XGUID _elementRangeFinderId;
    XGUID _elementRangeId, _elementRangeUnitsId;
    XGUID _elementPolarBearingId, _elementPolarBearingModeId;
    XGUID _elementPolarInclinationId;
    XGUID _elementPolarRollId;
    XGUID _elementAzimuthId;
    XGUID _elementInclinationId;
    
    XGUID _nextScreenId;

    CHAR *_portGlobalValue;

    BOOL _showPortOverride;
    CfxControl_Button *_buttonPortSelect;
    VOID OnPortSelectPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnPortSelectClick(CfxControl *pSender, VOID *pParams);

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ElementRangeFinder(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementRangeFinder();

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);

    BOOL IsRangeLocked() { return IsLive() && _range.HasRange() && GetSession(this)->IsEditing(); }

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnDialogResult(GUID *pDialogId, VOID *pParams);
    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    
    VOID OnRange(FXRANGE *pRange);
};

extern CfxControl *Create_Control_ElementRangeFinder(CfxPersistent *pOwner, CfxControl *pParent);
