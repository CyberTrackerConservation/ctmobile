#include "fxApplication.h"
#include "fxSound.h"
#include "fxUtils.h"
#include "fxMath.h"

//*************************************************************************************************
// CfxControl_SoundBase

CfxControl_SoundBase::CfxControl_SoundBase(CfxPersistent *pOwner, CfxControl *pParent): CfxControl(pOwner, pParent)
{
    InitBorder(ControlBorderStyle::Single, 1);

    _showPlay = _showStop = true;
    _buttonBorderStyle = ControlBorderStyle::None;
    _buttonBorderLineWidth = 1;
    _buttonBorderWidth = 1;
    _buttonWidth = 28;

    // Create buttons
    _playButton = new CfxControl_Button(pOwner, this);
    _playButton->SetComposite(true);
    _playButton->SetVisible(_showPlay);
    _playButton->SetOnClick(this, (NotifyMethod)&CfxControl_SoundBase::OnPlayClick);
    _playButton->SetOnPaint(this, (NotifyMethod)&CfxControl_SoundBase::OnPlayPaint);

    _stopButton = new CfxControl_Button(pOwner, this);
    _stopButton->SetComposite(true);
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

void CfxControl_SoundBase::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl::SetBounds(Left, Top, Width, Height);

    int th = GetClientHeight();
    int tx = _borderWidth;
    _playButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
    
    tx = Width - _borderWidth;
    _stopButton->SetBounds(tx - _buttonWidth, _borderWidth, _buttonWidth, th);
}

void CfxControl_SoundBase::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

    _buttonWidth = (_buttonWidth * ScaleX) / 100;
    _buttonWidth = max(16, _buttonWidth);
    _buttonWidth = max(ScaleHitSize, _buttonWidth);

    int t = _buttonBorderWidth;
    _buttonBorderWidth = (_buttonBorderWidth * ScaleBorder) / 100;
    if ((_buttonBorderWidth == 0) && (t > 0))
    {
        _buttonBorderWidth = 1;
    }

    _buttonBorderLineWidth = max(1, (_buttonBorderLineWidth * ScaleBorder) / 100);

    _height = max((int)ScaleHitSize, _height);
    SetBounds(_left, _top, _width, _height);
}

void CfxControl_SoundBase::DefineProperties(CfxFiler &F)
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

void CfxControl_SoundBase::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }
}

void CfxControl_SoundBase::OnPlayClick(CfxControl *pSender, void *pParams)
{
    FXSOUNDRESOURCE *soundData = GetSound();
    if (soundData)
    {
        GetHost(this)->PlaySound(soundData);
        ReleaseSound(soundData);
    }
}

void CfxControl_SoundBase::OnStopClick(CfxControl *pSender, void *pParams)
{
    GetHost(this)->StopSound();
}

void CfxControl_SoundBase::OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPlay(pParams->Rect, _color, _borderColor, _playButton->GetDown()); 
}

void CfxControl_SoundBase::OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams)
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

void CfxControl_Sound::DefineResources(CfxFilerResource &F)
{
}

void CfxControl_Sound::DefineProperties(CfxFiler &F)
{
    CfxControl_SoundBase::DefineProperties(F);
    F.DefineXSOUND("Sound", &_sound);
}

void CfxControl_Sound::DefinePropertiesUI(CfxFilerUI &F) 
{
}

FXSOUNDRESOURCE *CfxControl_Sound::GetSound()
{
    return GetResource()->GetSound(this, _sound);
}

void CfxControl_Sound::ReleaseSound(FXSOUNDRESOURCE *pSound)
{
    GetResource()->ReleaseSound(pSound);
}
