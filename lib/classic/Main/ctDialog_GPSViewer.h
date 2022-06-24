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
#include "fxTitleBar.h"
#include "fxNotebook.h"
#include "fxGPS.h"
#include "ctMovingMap.h"
#include "ctGotos.h"

//*************************************************************************************************

const GUID GUID_DIALOG_GPSVIEWER = {0x2799b511, 0x0ed6, 0x4b5e, {0xa1, 0x47, 0x32, 0xa3, 0x2f, 0x4a, 0x22, 0x35}};

//
// CctDialog_GPSViewer
//
class CctDialog_GPSViewer: public CfxDialog
{
protected:
    DBID _originalId;
    CfxControl_TitleBar *_titleBar;
    CfxControl_Notebook *_noteBook;
    CctControl_MovingMap *_movingMap;
    CctControl_MapInspector *_mapInspector;
    CctControl_GotoList *_gotoList;
    CfxControl_Panel *_gotoData;
    CfxControl_Panel *_spacer;
    CfxControl_Panel *_travelMode;

    VOID OnSetGotoClick(CfxControl *pSender, VOID *pParams);
    VOID OnNewGotoClick(CfxControl *pSender, VOID *pParams);
    VOID OnNoteBookPageChange(CfxControl * pSender, VOID *pParams);

public:
    CctDialog_GPSViewer(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctDialog_GPSViewer();

    VOID Init(CfxControl *pSender, VOID *pParam, UINT ParamSize);
    VOID PostInit(CfxControl *pSender);

    VOID DefineProperties(CfxFiler &F);
    VOID DefineState(CfxFiler &F);
    VOID DefineResources(CfxFilerResource &F);

    VOID OnCloseDialog(BOOL *pHandled);
};

extern CfxDialog *Create_Dialog_GPSViewer(CfxPersistent *pOwner, CfxControl *pParent);

