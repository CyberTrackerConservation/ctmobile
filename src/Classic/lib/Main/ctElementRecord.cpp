#include "fxApplication.h"
#include "fxUtils.h"
#include "fxTranslate.h"
#include "ctElementRecord.h"
#include "ctSighting.h"
#include "ctSession.h"
#include "ctActions.h"

CfxControl *Create_Control_ElementRecord(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementRecord(pOwner, pParent);
}

CctControl_ElementRecord::CctControl_ElementRecord(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTRECORD);
    InitLockProperties("Result Element;Maximum record time");

    InitXGuid(&_elementId);
    _required = false;

    _buttonBorderStyle = ControlBorderStyle::None;
    _buttonBorderLineWidth = 1;
    _buttonBorderWidth = 1;
    _buttonWidth = 28;

    _colorPlay = 0x00FF00;
    _colorRecord = 0x0000FF;
    _colorStop = 0;
    _confirmReplace = false;

    _caption = nullptr;
    _maxRecordTime = 60;
    _frequency = 8000;
    _deleteIfShort = 2;

    InitGuid(&_sightingUniqueId);
    _recordStartTickCount = _playStartTickCount = 0;
    _recordDuration = 0;

    // Create buttons
    _recordButton = new CfxControl_Button(pOwner, this);
    _recordButton->SetGroupId(1);
    _recordButton->SetComposite(true);
    _recordButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementRecord::OnRecordClick);
    _recordButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementRecord::OnRecordPaint);

    _playButton = new CfxControl_Button(pOwner, this);
    _playButton->SetGroupId(1);
    _playButton->SetComposite(true);
    _playButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementRecord::OnPlayClick);
    _playButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementRecord::OnPlayPaint);

    _stopButton = new CfxControl_Button(pOwner, this);
    _stopButton->SetGroupId(1);
    _stopButton->SetComposite(true);
    _stopButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementRecord::OnStopClick);
    _stopButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementRecord::OnStopPaint);

    RegisterSessionEvent(seCanNext, this, (NotifyMethod)&CctControl_ElementRecord::OnSessionCanNext);
    RegisterSessionEvent(seNext, this, (NotifyMethod)&CctControl_ElementRecord::OnSessionNext);
}

CctControl_ElementRecord::~CctControl_ElementRecord()
{
    UnregisterSessionEvent(seNext, this);
    UnregisterSessionEvent(seCanNext, this);

    if (IsLive())
    {
        GetEngine(this)->CancelRecording();
    }

    KillTimer();
}

void CctControl_ElementRecord::OnSessionCanNext(CfxControl *pSender, void *pParams)
{
    if (GetHost(this)->IsSoundSupported() && _required && (_caption == nullptr))
    {
        GetSession(this)->ShowMessage(fxTranslate("Recording is required"));
        *((bool *)pParams) = false;
    }
}

void CctControl_ElementRecord::OnSessionNext(CfxControl *pSender, void *pParams)
{
    GetCaption();

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId))
    {
        CctAction_MergeAttribute action(this);
        action.SetControlId(_id);
        char *fullFileName = nullptr;

        if (_caption && (strlen(_caption) > 0))
        {
            fullFileName = (char *)MEM_ALLOC(MAX_PATH + 8);
            if (fullFileName)
            {
                strcpy(fullFileName, MEDIA_PREFIX);
                strcat(fullFileName, _caption);
                action.SetupAdd(_id, _elementId, dtPText, &fullFileName);
            }
        }

        ((CctSession *)pSender)->ExecuteAction(&action);

        MEM_FREE(fullFileName);
    }
}

void CctControl_ElementRecord::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);

    F.DefineValue("SightingUniqueId", dtGuid, &_sightingUniqueId);
    F.DefineFileName("FileName", &_caption);
    F.DefineValue("RecordDuration", dtInt32, &_recordDuration);
}

void CctControl_ElementRecord::DefineResources(CfxFilerResource &F)
{
}

void CctControl_ElementRecord::DefineProperties(CfxFiler &F)
{
    CfxControl_Panel::DefineProperties(F);

    F.DefineValue("ButtonWidth",           dtInt32,   &_buttonWidth, "28");
    F.DefineValue("ButtonBorderStyle",     dtInt8,    &_buttonBorderStyle, F_ZERO);
    F.DefineValue("ButtonBorderWidth",     dtInt32,   &_buttonBorderWidth, F_ONE);
    F.DefineValue("ButtonBorderLineWidth", dtInt32,   &_buttonBorderLineWidth, F_ONE);
    F.DefineValue("ColorPlay",             dtColor,   &_colorPlay, F_COLOR_LIME);
    F.DefineValue("ColorRecord",           dtColor,   &_colorRecord, F_COLOR_RED);
    F.DefineValue("ColorStop",             dtColor,   &_colorStop, F_COLOR_BLACK);
    F.DefineValue("ConfirmReplace",        dtBoolean, &_confirmReplace, F_FALSE);
    F.DefineValue("DeleteIfShort",         dtInt32,   &_deleteIfShort, F_TWO);
    
    F.DefineValue("MaxRecordTime",         dtInt32,   &_maxRecordTime, "60");
    F.DefineValue("Frequency",             dtInt32,   &_frequency);
    F.DefineValue("Required",              dtBoolean, &_required, F_FALSE);

    F.DefineValue("Transparent",           dtBoolean, &_transparent, F_FALSE);

    F.DefineXOBJECT("Element", &_elementId);

    // Null id is automatically assigned to static

    if (F.IsReader())
    {
        SetBounds(_left, _top, _width, _height);

        _playButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
        _playButton->SetColor(_color);

        _recordButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
        _recordButton->SetColor(_color);

        _stopButton->SetBorder(_buttonBorderStyle, _buttonBorderWidth, _buttonBorderLineWidth, _borderColor);
        _stopButton->SetColor(_color);

        if (IsLive())
        {
            CctSession *session = GetSession(this);
            session->LoadControlState(this);
            if (CompareGuid(&_sightingUniqueId, session->GetCurrentSightingUniqueId()) == false)
            {
                SetCaption(nullptr);
                _recordDuration = 0;
            }
        }
    }
}

void CctControl_ElementRecord::DefinePropertiesUI(CfxFilerUI &F) 
{
}

void CctControl_ElementRecord::SetBounds(int Left, int Top, int Width, int Height)
{
    CfxControl_Panel::SetBounds(Left, Top, Width, Height);

    int th = GetClientHeight();
    int tx;
    
    _recordButton->SetBounds(_borderWidth, _borderWidth, _buttonWidth, th);
    tx = Width - _borderWidth * 2 - _buttonWidth * 2;
    _playButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
    tx += _borderWidth + _buttonWidth;
    _stopButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
}

void CctControl_ElementRecord::Scale(int ScaleX, int ScaleY, int ScaleBorder, int ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

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
}

void CctControl_ElementRecord::OnRecordClick(CfxControl *pSender, void *pParams)
{
    if (!GetHost(this)->RequestPermissionRecordAudio())
    {
        GetSession(this)->ShowMessage(fxTranslate("Recording disabled"));
        return;
    }

    if (_caption != nullptr)
    {
        if (_confirmReplace)
        {
            GetSession(this)->ShowConfirm(GetId(), fxTranslate("Delete existing recording?"));
            return;
        }

        GetSession(this)->DeleteMedia(_caption);
        SetCaption(nullptr);
    }

    SetPaintTimer(500);

    _recordStartTickCount = FxGetTickCount();

    char *fileName = GetSession(this)->AllocMediaFileName(nullptr);

    GetEngine(this)->StartRecording(this, fileName, _maxRecordTime * 1000, _frequency);

    MEM_FREE(fileName);

    Repaint();
}

void CctControl_ElementRecord::OnDialogResult(GUID *pDialogId, void *pParams)
{
    if ((bool)(UINT64)pParams)
    {
        GetSession(this)->DeleteMedia(_caption);
        SetCaption(nullptr);
        _recordDuration = 0;
        Repaint();
    }
}

void CctControl_ElementRecord::OnPlayClick(CfxControl *pSender, void *pParams)
{
    SetPaintTimer(500);

    if (_caption != nullptr)
    {
        char *fileName = GetSession(this)->AllocMediaFileName(_caption);
        GetHost(this)->PlayRecording(fileName);
        MEM_FREE(fileName);
    }

    _playStartTickCount = FxGetTickCount();

    Repaint();
}

void CctControl_ElementRecord::OnStopClick(CfxControl *pSender, void *pParams)
{
    GetHost(this)->StopSound();
    GetEngine(this)->StopRecording();

    _recordStartTickCount = _playStartTickCount = 0;

    Repaint();
}

void CctControl_ElementRecord::OnRecordPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawRecord(pParams->Rect, _colorRecord, _color, _recordButton->GetDown()); 
}

void CctControl_ElementRecord::OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPlay(pParams->Rect, _playButton->GetEnabled() ? _colorPlay : 0x808080, _color, _playButton->GetDown()); 
}

void CctControl_ElementRecord::OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawStop(pParams->Rect, _colorStop, _color, _stopButton->GetDown());
}

void CctControl_ElementRecord::OnRecordingComplete(char *pFileName, int Duration)
{
    CctSession *session = GetSession(this);

    if (Duration < _deleteIfShort * 1000)
    {
        session->DeleteMedia(session->GetMediaName(pFileName));
        pFileName = nullptr;
        Duration = 0;
    }

    _recordDuration = Duration;
    _recordStartTickCount = 0;

    SetCaption(session->GetMediaName(pFileName));
    session->BackupMedia(_caption);

    _sightingUniqueId = *session->GetCurrentSightingUniqueId();
    session->SaveCurrentScreen();
    session->SaveControlState(this);

    Repaint();
}

void CctControl_ElementRecord::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    bool isPlaying = (_playStartTickCount != 0);
    bool isRecording = (_recordStartTickCount != 0);


    if (isPlaying)
    {
        double t;
        t = FxGetTickCount() - _playStartTickCount;
        t /= FxGetTicksPerSecond();
        t *= 1000;
        isPlaying = t < _recordDuration;
    }

    if (isPlaying)
    {
        _playButton->SetEnabled(false);
        _playButton->SetDown(true);
        _stopButton->SetEnabled(true);
        _stopButton->SetDown(false);
        _recordButton->SetEnabled(false);
        _recordButton->SetDown(false);
        SetPaintTimer(500);
    }
    else if (isRecording)
    {
        _playButton->SetEnabled(false);
        _playButton->SetDown(false);
        _stopButton->SetEnabled(true);
        _stopButton->SetDown(false);
        _recordButton->SetEnabled(false);
        _recordButton->SetDown(true);
        SetPaintTimer(500);
    }
    else if ((_caption != nullptr) && (_recordDuration != 0))
    {
        _playButton->SetEnabled(true);
        _playButton->SetDown(false);
        _stopButton->SetEnabled(false);
        _stopButton->SetDown(true);
        _recordButton->SetEnabled(true);
        _recordButton->SetDown(false);
    } 
    else 
    {
        _playButton->SetEnabled(false);
        _playButton->SetDown(false);
        _stopButton->SetEnabled(false);
        _stopButton->SetDown(true);
        _recordButton->SetEnabled(true);
        _recordButton->SetDown(false);
    }

    FXRECT rect = *pRect;
    rect.Top += _oneSpace * 8;
    rect.Bottom -= _oneSpace * 8;
    rect.Left += _oneSpace + _borderWidth + _buttonWidth;
    rect.Right -= (_oneSpace + _borderWidth + _buttonWidth) * 2;
    
    pCanvas->State.LineWidth = _oneSpace;
    pCanvas->State.LineColor = _borderColor;
    pCanvas->State.BrushColor = _borderColor;

    int w = rect.Right - rect.Left + 1;
    int h = rect.Bottom - rect.Top + 1;
    double p = 0.0;

    if ((_caption != nullptr) && (_recordDuration != 0))
    {
        if (isPlaying)
        {
            p = FxGetTickCount() - _playStartTickCount;
            p /= FxGetTicksPerSecond();
            p /= (_recordDuration / 1000);
        }
        else
        {
            p = 1.0;
        }
    }
    else if (isRecording)
    {
        p = FxGetTickCount() - _recordStartTickCount;
        p /= FxGetTicksPerSecond();
        p /= _maxRecordTime;
    }

    pCanvas->State.LineColor = _borderColor;
    pCanvas->FrameRect(&rect);

    pCanvas->FillRect(rect.Left, rect.Top, min(w, (int)(p * w)), h);

    if ((p == 1.0) && !isPlaying)
    {
        CfxResource *resource = GetResource(); 
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font && ((pCanvas->FontHeight(font) - 2) < h))
        {
            char caption[32];
            sprintf(caption, "%lu s", (_recordDuration + 500) / 1000);
            pCanvas->State.TextColor = _color;
            pCanvas->DrawTextRect(font, &rect, TEXT_ALIGN_VCENTER | TEXT_ALIGN_HCENTER, caption);
            resource->ReleaseFont(font);
        }
    }
}
