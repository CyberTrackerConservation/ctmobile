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
#include "fxPanel.h"
#include "fxButton.h"
#include "fxScrollbar.h"
#include "ctElement.h"
#include "ctSession.h"

//*************************************************************************************************
//
// CctControl_ElementRecord
//

const GUID GUID_CONTROL_ELEMENTRECORD = {0x408bf31c, 0xa146, 0x4f59, {0x93, 0xbe, 0x84, 0xf3, 0x1b, 0xf1, 0xa7, 0x67}};

class CctControl_ElementRecord: public CfxControl_Panel
{
protected:
    GUID _sightingUniqueId;

    XGUID _elementId;
    BOOL _required;
    BOOL _confirmReplace;
    UINT _maxRecordTime;
    UINT _frequency;
    UINT _deleteIfShort;
    UINT _recordStartTickCount, _playStartTickCount, _recordDuration;

    CfxControl_Button *_recordButton, *_playButton, *_stopButton;
    UINT _buttonWidth, _buttonBorderWidth, _buttonBorderLineWidth;
    ControlBorderStyle _buttonBorderStyle;

    COLOR _colorRecord, _colorPlay, _colorStop;

    VOID OnRecordClick(CfxControl *pSender, VOID *pParams);
    VOID OnPlayClick(CfxControl *pSender, VOID *pParams);
    VOID OnStopClick(CfxControl *pSender, VOID *pParams);
    VOID OnRecordPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    VOID OnSessionCanNext(CfxControl *pSender, VOID *pParams);
    VOID OnSessionNext(CfxControl *pSender, VOID *pParams);
public:
    CctControl_ElementRecord(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementRecord();

    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
    
    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID OnRecordingComplete(CHAR *pFileName, UINT Duration);

    VOID OnDialogResult(GUID *pDialogId, VOID *pParams);
};

extern CfxControl *Create_Control_ElementRecord(CfxPersistent *pOwner, CfxControl *pParent);
