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
#include "fxUtils.h"
#include "ctElementRecord.h"
#include "ctSighting.h"
#include "ctSession.h"
#include "ctActions.h"
#include "ctDialog_Confirm.h"

CfxControl *Create_Control_ElementRecord(CfxPersistent *pOwner, CfxControl *pParent)
{
    return new CctControl_ElementRecord(pOwner, pParent);
}

CctControl_ElementRecord::CctControl_ElementRecord(CfxPersistent *pOwner, CfxControl *pParent): CfxControl_Panel(pOwner, pParent)
{
    InitControl(&GUID_CONTROL_ELEMENTRECORD);
    InitLockProperties("Result Element;Maximum record time");

    InitXGuid(&_elementId);
    _required = FALSE;

    _buttonBorderStyle = bsNone;
    _buttonBorderLineWidth = 1;
    _buttonBorderWidth = 1;
    _buttonWidth = 28;

    _colorPlay = 0x00FF00;
    _colorRecord = 0x0000FF;
    _colorStop = 0;
    _confirmReplace = FALSE;

    _caption = NULL;
    _maxRecordTime = 60;
    _frequency = 8000;
    _deleteIfShort = 2;

    InitGuid(&_sightingUniqueId);
    _recordStartTickCount = _playStartTickCount = 0;
    _recordDuration = 0;

    // Create buttons
    _recordButton = new CfxControl_Button(pOwner, this);
    _recordButton->SetGroupId(1);
    _recordButton->SetComposite(TRUE);
    _recordButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementRecord::OnRecordClick);
    _recordButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementRecord::OnRecordPaint);

    _playButton = new CfxControl_Button(pOwner, this);
    _playButton->SetGroupId(1);
    _playButton->SetComposite(TRUE);
    _playButton->SetOnClick(this, (NotifyMethod)&CctControl_ElementRecord::OnPlayClick);
    _playButton->SetOnPaint(this, (NotifyMethod)&CctControl_ElementRecord::OnPlayPaint);

    _stopButton = new CfxControl_Button(pOwner, this);
    _stopButton->SetGroupId(1);
    _stopButton->SetComposite(TRUE);
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

VOID CctControl_ElementRecord::OnSessionCanNext(CfxControl *pSender, VOID *pParams)
{
    if (GetHost(this)->IsSoundSupported() && _required && (_caption == NULL))
    {
        GetSession(this)->ShowMessage("Recording is required");
        *((BOOL *)pParams) = FALSE;
    }
}

VOID CctControl_ElementRecord::OnSessionNext(CfxControl *pSender, VOID *pParams)
{
    GetCaption();

    // Add the Attribute for this Element
    if (!IsNullXGuid(&_elementId))
    {
        CctAction_MergeAttribute action(this);
        action.SetControlId(_id);
        CHAR *fullFileName = NULL;

        if (_caption && (strlen(_caption) > 0))
        {
            fullFileName = (CHAR *)MEM_ALLOC(MAX_PATH + 8);
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

VOID CctControl_ElementRecord::DefineState(CfxFiler &F)
{
    CfxControl_Panel::DefineState(F);

    F.DefineValue("SightingUniqueId", dtGuid, &_sightingUniqueId);
    F.DefineValue("FileName", dtPTextAnsi, &_caption);
    F.DefineValue("RecordDuration", dtInt32, &_recordDuration);
}

VOID CctControl_ElementRecord::DefineResources(CfxFilerResource &F)
{
#ifdef CLIENT_DLL
    CfxControl_Panel::DefineResources(F);
    F.DefineObject(_elementId, eaName);
    F.DefineField(_elementId, dtSound);
#endif
}

VOID CctControl_ElementRecord::DefineProperties(CfxFiler &F)
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
    #ifdef CLIENT_DLL
        if (IsNullXGuid(&_elementId))
        {
            _elementId = GUID_ELEMENT_SOUND;
        }
    #endif

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
            if (CompareGuid(&_sightingUniqueId, session->GetCurrentSightingUniqueId()) == FALSE)
            {
                SetCaption(NULL);
                _recordDuration = 0;
            }
        }
    }
}

VOID CctControl_ElementRecord::DefinePropertiesUI(CfxFilerUI &F) 
{
#ifdef CLIENT_DLL
    F.DefineValue("Border color", dtColor,   &_borderColor);
    F.DefineValue("Border line width", dtInt32, &_borderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Border style", dtInt8,    &_borderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Border width", dtInt32,   &_borderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button border", dtInt32,  &_buttonBorderStyle, "LIST(None Single Round1 Round2)");
    F.DefineValue("Button border line width", dtInt32, &_buttonBorderLineWidth, "MIN(1);MAX(100)");
    F.DefineValue("Button border width", dtInt32, &_buttonBorderWidth, "MIN(0);MAX(1000)");
    F.DefineValue("Button width", dtInt32,   &_buttonWidth, "MIN(20);MAX(1000)");
    F.DefineValue("Color",        dtColor,   &_color);
    F.DefineValue("Color play",   dtColor,   &_colorPlay);
    F.DefineValue("Color record", dtColor,   &_colorRecord);
    F.DefineValue("Color stop",   dtColor,   &_colorStop);
    F.DefineValue("Confirm replace", dtBoolean, &_confirmReplace);
    F.DefineValue("Delete if short time", dtInt32, &_deleteIfShort, "MIN(0);MAX(10)");
    F.DefineValue("Dock",          dtByte,   &_dock,        "LIST(None Top Bottom Left Right Fill)");
    F.DefineValue("Height",       dtInt32,   &_height);
    F.DefineValue("Left",         dtInt32,   &_left);
    F.DefineValue("Maximum record time", dtInt32, &_maxRecordTime, "MIN(5);MAX(300)");
    F.DefineValue("Required",     dtBoolean, &_required);
    F.DefineValue("Result Element", dtGuid,  &_elementId,   "EDITOR(ScreenElementsEditor);REQUIRED");
    F.DefineValue("Sample rate",  dtInt32,   &_frequency,   "LIST(\"8.0 kHz\"=8000 \"11.025 kHz\"=11025 \"22.05 kHz\"=22050 \"44.1 kHz\"=44100)");
    F.DefineValue("Top",          dtInt32,   &_top);
    F.DefineValue("Transparent",  dtBoolean, &_transparent);
    F.DefineValue("Width",        dtInt32,   &_width);
#endif
}

VOID CctControl_ElementRecord::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxControl_Panel::SetBounds(Left, Top, Width, Height);

    UINT th = GetClientHeight();
    INT tx;
    
    _recordButton->SetBounds(_borderWidth, _borderWidth, _buttonWidth, th);
    tx = Width - _borderWidth * 2 - _buttonWidth * 2;
    _playButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
    tx += _borderWidth + _buttonWidth;
    _stopButton->SetBounds(tx, _borderWidth, _buttonWidth, th);
}

VOID CctControl_ElementRecord::Scale(UINT ScaleX, UINT ScaleY, UINT ScaleBorder, UINT ScaleHitSize)
{
    CfxControl_Panel::Scale(ScaleX, ScaleY, ScaleBorder, ScaleHitSize);

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
}

VOID CctControl_ElementRecord::OnRecordClick(CfxControl *pSender, VOID *pParams)
{
    if (!GetHost(this)->IsSoundSupported())
    {
        GetSession(this)->ShowMessage("Sound not supported");
        return;
    }

    if (_caption != NULL)
    {
        if (_confirmReplace)
        {
            GetSession(this)->Confirm(this, "Confirm", "Delete existing recording?");
            return;
        }

        GetSession(this)->DeleteMedia(_caption);
        SetCaption(NULL);
    }

    SetPaintTimer(500);

    CHAR *fileName = GetSession(this)->AllocMediaFileName(NULL);

    GetEngine(this)->StartRecording(this, fileName, _maxRecordTime * 1000, _frequency);

    MEM_FREE(fileName);

    _recordStartTickCount = FxGetTickCount();

    Repaint();
}

VOID CctControl_ElementRecord::OnDialogResult(GUID *pDialogId, VOID *pParams)
{
    if ((BOOL)(UINT64)pParams)
    {
        GetSession(this)->DeleteMedia(_caption);
        SetCaption(NULL);
        _recordDuration = 0;
        Repaint();
    }
}

VOID CctControl_ElementRecord::OnPlayClick(CfxControl *pSender, VOID *pParams)
{
    SetPaintTimer(500);

    if (_caption != NULL)
    {
        CHAR *fileName = GetSession(this)->AllocMediaFileName(_caption);
        GetHost(this)->PlayRecording(fileName);
        MEM_FREE(fileName);
    }

    _playStartTickCount = FxGetTickCount();

    Repaint();
}

VOID CctControl_ElementRecord::OnStopClick(CfxControl *pSender, VOID *pParams)
{
    GetHost(this)->StopSound();
    GetEngine(this)->StopRecording();

    _recordStartTickCount = _playStartTickCount = 0;

    Repaint();
}

VOID CctControl_ElementRecord::OnRecordPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawRecord(pParams->Rect, _colorRecord, _color, _recordButton->GetDown()); 
}

VOID CctControl_ElementRecord::OnPlayPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawPlay(pParams->Rect, _playButton->GetEnabled() ? _colorPlay : 0x808080, _color, _playButton->GetDown()); 
}

VOID CctControl_ElementRecord::OnStopPaint(CfxControl *pSender, FXPAINTDATA *pParams)
{
    pParams->Canvas->DrawStop(pParams->Rect, _colorStop, _color, _stopButton->GetDown());
}

VOID CctControl_ElementRecord::OnRecordingComplete(CHAR *pFileName, UINT Duration)
{
    CctSession *session = GetSession(this);

    if (Duration < _deleteIfShort * 1000)
    {
        session->DeleteMedia(session->GetMediaName(pFileName));
        pFileName = NULL;
        Duration = 0;
    }

    _recordDuration = Duration;
    _recordStartTickCount = 0;

    SetCaption(session->GetMediaName(pFileName));
    session->BackupMedia(_caption);
    Repaint();

    _sightingUniqueId = *session->GetCurrentSightingUniqueId();
    session->SaveControlState(this);
}

VOID CctControl_ElementRecord::OnPaint(CfxCanvas *pCanvas, FXRECT *pRect)
{
    if (!_transparent)
    {
        pCanvas->State.BrushColor = _color;
        pCanvas->FillRect(pRect);
    }

    CfxHost *host = GetHost(this);
    BOOL isPlaying = (_playStartTickCount != 0);
    BOOL isRecording = (_recordStartTickCount != 0);

    if (isRecording)
    {
        FxResetAutoOffTimer();
    }

    if (isPlaying)
    {
        DOUBLE t;
        t = FxGetTickCount() - _playStartTickCount;
        t /= FxGetTicksPerSecond();
        t *= 1000;
        isPlaying = t < _recordDuration;
    }

    if (isPlaying)
    {
        _playButton->SetEnabled(FALSE);
        _playButton->SetDown(TRUE);
        _stopButton->SetEnabled(TRUE);
        _stopButton->SetDown(FALSE);
        _recordButton->SetEnabled(FALSE);
        _recordButton->SetDown(FALSE);
        SetPaintTimer(500);
    }
    else if (isRecording)
    {
        _playButton->SetEnabled(FALSE);
        _playButton->SetDown(FALSE);
        _stopButton->SetEnabled(TRUE);
        _stopButton->SetDown(FALSE);
        _recordButton->SetEnabled(FALSE);
        _recordButton->SetDown(TRUE);
        SetPaintTimer(500);
    }
    else if ((_caption != NULL) && (_recordDuration != 0))
    {
        _playButton->SetEnabled(TRUE);
        _playButton->SetDown(FALSE);
        _stopButton->SetEnabled(FALSE);
        _stopButton->SetDown(TRUE);
        _recordButton->SetEnabled(TRUE);
        _recordButton->SetDown(FALSE);
    } 
    else 
    {
        _playButton->SetEnabled(FALSE);
        _playButton->SetDown(FALSE);
        _stopButton->SetEnabled(FALSE);
        _stopButton->SetDown(TRUE);
        _recordButton->SetEnabled(TRUE);
        _recordButton->SetDown(FALSE);
    }

    FXRECT rect = *pRect;
    rect.Top += _oneSpace * 8;
    rect.Bottom -= _oneSpace * 8;
    rect.Left += _oneSpace + _borderWidth + _buttonWidth;
    rect.Right -= (_oneSpace + _borderWidth + _buttonWidth) * 2;
    
    pCanvas->State.LineWidth = _oneSpace;
    pCanvas->State.LineColor = _borderColor;
    pCanvas->State.BrushColor = _borderColor;

    UINT w = rect.Right - rect.Left + 1;
    UINT h = rect.Bottom - rect.Top + 1;
    DOUBLE p = 0.0;

    if ((_caption != NULL) && (_recordDuration != 0))
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

    pCanvas->FillRect(rect.Left, rect.Top, min(w, (UINT)(p * w)), h);

    if ((p == 1.0) && !isPlaying)
    {
        CfxResource *resource = GetResource(); 
        FXFONTRESOURCE *font = resource->GetFont(this, _font);
        if (font && ((pCanvas->FontHeight(font) - 2) < h))
        {
            CHAR caption[32];
            sprintf(caption, "%lu s", (_recordDuration + 500) / 1000);
            pCanvas->State.TextColor = _color;
            pCanvas->DrawTextRect(font, &rect, TEXT_ALIGN_VCENTER | TEXT_ALIGN_HCENTER, caption);
            resource->ReleaseFont(font);
        }
    }
}
