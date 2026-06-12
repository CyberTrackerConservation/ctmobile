#pragma once
#include "pch.h"
#include "fxPanel.h"
#include "fxButton.h"

//
// CfxControl_SoundBase
//
class CfxControl_SoundBase: public CfxControl
{
protected:
	bool _showPlay = false, _showStop = false;
    CfxControl_Button *_playButton = nullptr, *_stopButton = nullptr;
    int _buttonWidth = 0, _buttonBorderWidth = 0, _buttonBorderLineWidth = 0;
    ControlBorderStyle _buttonBorderStyle = {};

    void OnPlayClick(CfxControl *pSender, void *pParams);
    void OnStopClick(CfxControl *pSender, void *pParams);
    void OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    void OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    virtual FXSOUNDRESOURCE *GetSound()=0;
    virtual void ReleaseSound(FXSOUNDRESOURCE *pSound)=0;
public:
    CfxControl_SoundBase(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_SoundBase();
    
    void DefineProperties(CfxFiler &F) override;

    void SetBounds(int Left, int Top, int Width, int Height) override;
    void Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize) override;

    void OnPaint(CfxCanvas *pCanvas, FXRECT *pRect) override;
};

//
// CfxControl_Sound
//
const GUID GUID_CONTROL_SOUND = {0xa864d253, 0x774, 0x4f6d, { 0xab, 0x88, 0x5, 0xf4, 0x9e, 0xf1, 0x58, 0xcc}};

class CfxControl_Sound: public CfxControl_SoundBase
{
protected:
    XSOUND _sound = 0;
    FXSOUNDRESOURCE *GetSound() override;
    void ReleaseSound(FXSOUNDRESOURCE *pSound) override;
public:
    CfxControl_Sound(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Sound();

    void DefineProperties(CfxFiler &F) override;
    void DefinePropertiesUI(CfxFilerUI &F) override;
    void DefineResources(CfxFilerResource &F) override;
};

extern CfxControl *Create_Control_Sound(CfxPersistent *pOwner, CfxControl *pParent);
