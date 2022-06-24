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

#include "fxApplication.h"
#include "fxSound.h"
#include "fxUtils.h"
#include "fxMath.h"

//*************************************************************************************************
// CfxControl_SoundBase

CfxControl_SoundBase::CfxControl_SoundBase(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitBorder(bsSingle, 1);

    _showPlay = _showStop = TRUE;
    _buttonBorderStyle = bsNone;
    _buttonBorderLineWidth = 1;
    _buttonBorderWidth = 1;
    _buttonWidth = 28;

    // Create buttons
    _playButton = new CfxControl_Button(pOwner, this);
    _playButton->SetComposite(TRUE);
    _playButton->SetVisible(_showPlay);
    _playButton->SetOnClick(this, (NotifyMethod)&CfxControl_SoundBase::OnPlayClick);
    _playButton->SetOnPaint(this, (NotifyMethod)&CfxControl_SoundBase::OnPlayPaint);

    _stopButton = new CfxControl_Button(pOwner, this);
    _stopButton->SetComposite(TRUE);
    _stopButton->SetVisible(_showStop);
    _stopButton->SetOnClick(this, (NotifyMethod)&CfxControl_SoundBase::OnStopClick);
    _stopButton->SetOnPaint(this, (NotifyMethod)&CfxControl_SoundBase::OnStopPaint);
}

CfxControl_SoundBase::~CfxControl_SoundBase()
{
    if (IsLive())
    {
        GetHost(this)->StopSound();
    }
}

VOID CfxControl_SoundBase::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    UINT th = GetClientHeight();
    UINT tx = _borderWidth;
    _playButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
    
    tx = Width - _borderWidth;
    _stopButton->SetBounds(tx - _buttonWidth, _borderWidth, _buttonWidth, th);
}

VOID CfxControl_SoundBase::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _buttonWidth = (_buttonWidth * ScaleX) / 100;
    _buttonWidth = max(16, _buttonWidth);
    _buttonWidth = max(ScaleHitSize, _buttonWidth);

    UINT t = _buttonBorderWidth;
    _buttonBorderWidth = (_buttonBorderWidth * ScaleBorder) / 100;
    if ((_buttonBorderWidth == 0) && (t > 0))
    {
        _buttonBorderWidth = 1;
    }

    _buttonBorderLineWidth = max(1, (_buttonBorderLineWidth * ScaleBorder) / 100);

    _height = max((INT)ScaleHitSize, _height);
    SetBounds(_left, _top, _width, _height);
}

VOID CfxControl_SoundBase::DefineProperties(CfxFiler &F)
{
    CfxControl::DefineProperties(F);

    F.DefineValue("ButtonWidth",           dtInt32,   &_buttonWidth, "28");
    F.DefineValue("ButtonBorderStyle",     dtInt8,    &_buttonBorderStyle, F_ZERO);
    F.DefineValue("ButtonBorderWidth",     dtInt32,   &_buttonBorderWidth, F_ONE);
    F.DefineValue("ButtonBorderLineWidth", dtInt32,   &_buttonBorderLineWidth, F_ONE);

    F.DefineValue("ShowPlay",              dtBoolean, &_showPlay, F_TRUE);
    F.DefineValue("ShowStop",              dtBoolean, &_showStop, F_TRUE);

    F.DefineValue("Transparent",           dtBoolean, &_transparent, F_FALSE);

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);

        _playButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
        _playButton->SetColor(_color);
        _playButton->SetVisible(_showPlay);

        _stopButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
        _stopButton->SetColor(_color);
        _stopButton->SetVisible(_showStop);
    }
}

VOID CfxControl_SoundBase::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }
}

VOID CfxControl_SoundBase::OnPlayClick(CfxControl *pSender, VOID *pParams)
{
    FXSOUNDRESOURCE *soundData = GetSound();
    if (soundData)
    {
        GetHost(this)->PlaySound(soundData);
        ReleaseSound(soundData);
    }
}

VOID CfxControl_SoundBase::OnStopClick(CfxControl *pSender, VOID *pParams)
{
    GetHost(this)->StopSound();
}

VOID CfxControl_SoundBase::OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPlay(pParams->Rect, _color, _borderColor, _playButton->GetDown()); 
}

VOID CfxControl_SoundBase::OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawStop(pParams->Rect, _color, _borderColor, _stopButton->GetDown()); 
}

//*************************************************************************************************
// CfxControl_Sound

CfxControl *Create_Control_Sound(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CfxControl_Sound(pOwner, pParent);
}

CfxControl_Sound::CfxControl_Sound(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_SoundBase(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_SOUND);
    InitLockProperties("Sound");

    _sound = 0;
}

CfxControl_Sound::~CfxControl_Sound()
{
    freeX(_sound);
}

VOID CfxControl_Sound::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl::DefineResources(F);
    F.DefineSound(_sound);
#endif
}

VOID CfxControl_Sound::DefineProperties(CfxFiler &F)
{
    CfxControl_SoundBase::DefineProperties(F);
    F.DefineXSOUND("Sound", &_sound);
}

VOID CfxControl_Sound::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,    &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,     &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,    &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button border", dtInt32,   &_buttonBorderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Button border line width", dtInt32, &_buttonBorderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Button border width", dtInt32, &_buttonBorderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button width",  dtInt32,   &_buttonWidth, "MIN(20) MAX(80)");
    F.DefineValue("Color",        dtColor,    &_color);
    F.DefineValue("Dock",         dtByte,     &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Height",       dtInt32,    &_height);
    F.DefineValue("Left",         dtInt32,    &_left);
    F.DefineValue("Show play",    dtBoolean,  &_showPlay);
    F.DefineValue("Show stop",    dtBoolean,  &_showStop);
    F.DefineValue("Sound",        dtPSound,   &_sound);
    F.DefineValue("Top",          dtInt32,    &_top);
    F.DefineValue("Transparent",  dtBoolean,  &_transparent);
    F.DefineValue("Width",        dtInt32,    &_width);
#endif
}

FXSOUNDRESOURCE *CfxControl_Sound::GetSound()
{
    return GetResource()->GetSound(this, _sound);
}

VOID CfxControl_Sound::ReleaseSound(FXSOUNDRESOURCE *pSound)
{
    GetResource()->ReleaseSound(pSound);
}
