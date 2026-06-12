#pragma once

#include "pch.h"
#include "fxPanel.h"
#include "fxButton.h"
#include "fxScrollBar.h"
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

    XGUID _elementId = 0;
    bool _required = false;
    bool _confirmReplace = false;
    int _maxRecordTime = 0;
    int _frequency = 0;
    int _deleteIfShort = 0;
    int _recordStartTickCount = 0, _playStartTickCount = 0, _recordDuration = 0;

    CfxControl_Button *_recordButton = nullptr, *_playButton = nullptr, *_stopButton = nullptr;
    int _buttonWidth = 0, _buttonBorderWidth = 0, _buttonBorderLineWidth = 0;
    ControlBorderStyle _buttonBorderStyle = {};

    COLOR _colorRecord = 0, _colorPlay = 0, _colorStop = 0;

    void OnRecordClick(CfxControl *pSender, void *pParams);
    void OnPlayClick(CfxControl *pSender, void *pParams);
    void OnStopClick(CfxControl *pSender, void *pParams);
    void OnRecordPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    void OnSessionCanNext(CfxControl *pSender, void *pParams);
    void OnSessionNext(CfxControl *pSender, void *pParams);
public:
    CctControl_ElementRecord(CfxPersistent *pOwner, CfxControl *pParent);
    ~CctControl_ElementRecord();

    void DefineState(CfxFiler &F) override;
    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
    
    void SetBounds(int Left, int Top, int Width, int Height) override;
    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
    void OnRecordingComplete(char *pFileName, int Duration) override;

    void OnDialogResult(GUID *pDialogId, void *pParams) override;
};

extern CfxControl *Create_Control_ElementRecord(CfxPersistent *pOwner, CfxControl *pParent);
