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

//
// CfxControl_SoundBase
//
class CfxControl_SoundBase: public CfxControl
{
protected:
	BOOL _showPlay, _showStop;
    CfxControl_Button *_playButton, *_stopButton;
    UINT _buttonWidth, _buttonBorderWidth, _buttonBorderLineWidth;
    ControlBorderStyle _buttonBorderStyle;

    VOID OnPlayClick(CfxControl *pSender, VOID *pParams);
    VOID OnStopClick(CfxControl *pSender, VOID *pParams);
    VOID OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams);
    VOID OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams);

    virtual FXSOUNDRESOURCE *GetSound()=0;
    virtual VOID ReleaseSound(FXSOUNDRESOURCE *pSound)=0;
public:
    CfxControl_SoundBase(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_SoundBase();
    
    VOID DefineProperties(CfxFiler &F);

    VOID SetBounds(INT Left, INT Top, UINT Width, UINT Height);
    VOID Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize);

    VOID OnPaint(CfxCanvas *pCanvas, FXRECT *pRect);
};

//
// CfxControl_Sound
//
const GUID GUID_CONTROL_SOUND = {0xa864d253, 0x774, 0x4f6d, { 0xab, 0x88, 0x5, 0xf4, 0x9e, 0xf1, 0x58, 0xcc}};

class CfxControl_Sound: public CfxControl_SoundBase
{
protected:
    XSOUND _sound;
    FXSOUNDRESOURCE *GetSound();
    VOID ReleaseSound(FXSOUNDRESOURCE *pSound);
public:
    CfxControl_Sound(CfxPersistent *pOwner, CfxControl *pParent);
    ~CfxControl_Sound();

    VOID DefineProperties(CfxFiler &F);
    VOID DefinePropertiesUI(CfxFilerUI &F);
    VOID DefineResources(CfxFilerResource &F);
};

extern CfxControl *Create_Control_Sound(CfxPersistent *pOwner, CfxControl *pParent);
