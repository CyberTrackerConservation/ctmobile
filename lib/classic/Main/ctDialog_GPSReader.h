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
#include "fxList.h"
#include "fxGPS.h"
#include "fxTitleBar.h"
#include "fxNotebook.h"
#include "ctMovingMap.h"
#include "ctElementList.h"

//*************************************************************************************************

const GUID GUID_DIALOG_GPSREADER = {0xc3ede1fa, 0x971c, 0x4167, {0x98, 0x8d, 0x12, 0x49, 0x1e, 0x40, 0x3d, 0xe6}};

struct GPS_READER_CONTEXT
{
    XGUID SaveTarget;
    INT SkipTimeout;
};

enum GPS_READER_STAGE
{
    rsNormalGps = 0,
    rsManualGps,
    rsMovingMap
};

//
// CctDialog_GPSViewer
//

class CctDialog_GPSReader: public CfxDialog
{
protected:
    DBID _originalId;
    XGUID _commitTargetId;
    INT _skipTimeout;
    CfxControl_TitleBar *_titleBar;
    CfxControl_Notebook *_noteBook;
    CfxControl_Panel *_spacer;
    CctControl_MovingMap *_movingMap;
    CctControl_ElementList *_manualGps;
    CfxControl_Button *_manualButton;
    CfxControl_Button *_skipButton;
    CfxControl_Panel *_travelMode;

    UINT _fixCount;
    UINT _triangleTimeOut;
    FXGPS _gps;
    GPS_READER_STAGE _stage;
    BOOL _useManualGps;
    BOOL _useMovingMap;

    VOID ClearDialogControls();

    VOID SetStage(GPS_READER_STAGE Value);

    VOID CommitWithPosition(FXGPS_POSITION *pGPS, BOOL DoRepaint);
    BOOL FixSufficient(FXGPS *pGPS);
    VOID ConstructFix(FXGPS *pGPS);
    VOID OnSkipClick(CfxControl *pSender, VOID *pParams);
    VOID OnMovingMapFix(CfxControl *pSender, VOID *pParams);
    VOID OnManualClick(CfxControl *pSender, VOID *pParams);
    VOID OnNoteBookPageChange(CfxControl * pSender, VOID *pParams);

public:
    CctDialog_GPSReader(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctDialog_GPSReader();

    VOID Init(CfxControl *pSender, VOID *pParam, UINT ParamSize);
    VOID PostInit(CfxControl *pSender);

    VOID DefineState(CfxFiler &F);
    VOID DefineProperties(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
    VOID OnTimer();

    VOID OnCloseDialog(BOOL *pHandled);
};

extern CfxDialog *Create_Dialog_GPSReader(CfxPersistent *pOwner, CfxControl *pParent);
