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

#include "pch.h"

#include "QtUtils.h"
#include "QtHost.h"
#include "QtCanvas.h"
#include "MapResource.h"
#include "ChunkDatabase.h"
#include "Location.h"

#include <string.h>

constexpr char CLASSIC_ALARM_ID[] = "CLASSIC_ALARM_ID";

//*************************************************************************************************

CHost_Qt::CHost_Qt(CfxPersistent *pOwner, CtClassicSessionItem *pWindow, FXPROFILE *pProfile, const CHAR *pApplicationPath): CfxHost(pOwner, pProfile)
{
    _window = pWindow;
    _batteryLevel = _batteryState = _powerState = 0;
    _batteryState = batHigh;

    _gpsTimeStamp = 0;
    _gpsTimeOffset = 0;
    memset(&_gps, 0, sizeof(_gps));

    memset(&_enumFileMap, 0, sizeof(_enumFileMap));
    _enumFileIndex = 0;

    _applicationPath = AllocString(pApplicationPath);

    connect(&_trackStreamer, &LocationStreamer::locationUpdated, this, &CHost_Qt::onTrackTimer);

    _trackTimerHandler.setSingleShot(true);
    _trackTimerHandler.setInterval(1000);

    connect(&_trackTimerHandler, &QTimer::timeout, [&]()
    {
        auto Timeout = _trackTimeout * 1000;

        if (_trackStreamer.running())
        {
            if (_trackStreamer.updateInterval() == Timeout)
            {
                return;
            }

            _trackStreamer.stop();
        }

        if (Timeout > 0)
        {
            _trackStreamer.start(Timeout);
        }
    });

    _audioTimer.setSingleShot(true);
    connect(&_audioTimer, &QTimer::timeout, [this]()
    {
        this->StopRecording();
    });

    connect(App::instance(), &App::alarmFired, this, [&](const QString& alarmId)
    {
        if (alarmId == CLASSIC_ALARM_ID)
        {
            GetApplication(this)->GetEngine()->DoAlarm();
        }
    });
}

CHost_Qt::~CHost_Qt()
{
    qDeleteAll(_timers);
    disconnect(App::instance(), &App::alarmFired, 0, 0);

    SetGPS(false);

    StopSound();
    CancelRecording();

    FxUnmapFile(&_enumFileMap);
    _enumFileIndex = 0;
}

VOID CHost_Qt::SetBounds(INT Left, INT Top, UINT Width, UINT Height)
{
    CfxHost::SetBounds(Left, Top, Width, Height);
}

CfxCanvas *CHost_Qt::CreateCanvas(UINT Width, UINT Height)
{
    CCanvas_Qt *canvas = new CCanvas_Qt(this, _profile.ScaleFont);
    if (canvas && Width && Height)
    {
        canvas->SetSize((Width + 3) & ~3, Height);
    }

    return canvas;
}

CfxDatabase *CHost_Qt::CreateDatabase(CHAR *pDatabaseName)
{
    return new CChunkDatabase(this, pDatabaseName);
}

CfxResource *CHost_Qt::CreateResource(CHAR *pResourceName)
{
    return new CMapResource(this, pResourceName);
}

VOID CHost_Qt::RequestMediaScan(const CHAR *pFileName)
{
    Utils::mediaScan(pFileName);
}

VOID CHost_Qt::Paint()
{
    if (_window == nullptr)
    {
        return;
    }

    static bool s_mutex = false;
    if (!s_mutex)
    {
        s_mutex = true;
        GetApplication(this)->GetEngine()->Paint(_canvas);
        auto image = ((CCanvas_Qt *)_canvas)->GetImage();
        _window->setImage(image);
        s_mutex = false;
    }

    _window->update();
}

VOID CHost_Qt::Startup()
{
}

VOID CHost_Qt::Shutdown()
{
    if (_window == nullptr)
    {
        return;
    }

    emit _window->shutdown();
}

VOID CHost_Qt::SetKioskMode(BOOL /*Enabled*/)
{
}

VOID CHost_Qt::SetTrackTimer(INT Timeout)
{
    _trackTimeout = Timeout;
    _trackTimerHandler.start();
}

void CHost_Qt::onTrackTimer(Location* update)
{
    FXGPS *finalGps = GetGPSPtr();

    GPSDataReceived();

    finalGps->State = dsConnected;
    finalGps->TimeStamp = FxGetTickCount();
    finalGps->Position.Quality = fq3D;

    finalGps->Position.Latitude = update->y();
    finalGps->Position.Longitude = update->x();
    finalGps->Position.Altitude = update->z();

    auto accuracy = update->accuracy();
    if (std::isnan(accuracy))
    {
        accuracy = 100;
    }

    auto speed = update->speed();
    if (std::isnan(speed))
    {
        speed = 0;
    }

    auto direction = update->direction();
    if (std::isnan(direction))
    {
        direction = 0;
    }

    finalGps->Position.Accuracy = min(50, accuracy / 10);
    finalGps->Position.Speed = speed;
    finalGps->Heading = static_cast<UINT>(direction);
    finalGps->State = dsConnected;

    FXDATETIME *pDateTime = &finalGps->DateTime;

    memset(pDateTime, 0, sizeof(FXDATETIME));

    auto ts = App::instance()->timeManager()->currentDateTime();
    pDateTime->Second = static_cast<UINT16>(ts.time().second());
    pDateTime->Minute = static_cast<UINT16>(ts.time().minute());
    pDateTime->Hour = static_cast<UINT16>(ts.time().hour());
    pDateTime->Month = static_cast<UINT16>(ts.date().month());
    pDateTime->Year = static_cast<UINT16>(ts.date().year());
    pDateTime->Day = static_cast<UINT16>(ts.date().day());

    GetApplication(this)->GetEngine()->DoTrackTimer();
}

BOOL CHost_Qt::HasExternalCloseButton()
{
    return FALSE;
}

VOID CHost_Qt::ShowSIP()
{
    if (_window == nullptr)
    {
        return;
    }

    emit _window->setInputMethodVisible(true);
}

VOID CHost_Qt::HideSIP()
{
    if (_window == nullptr)
    {
        return;
    }

    emit _window->setInputMethodVisible(false);
}

BOOL CHost_Qt::GetDeviceId(GUID *pGuid)
{
    auto deviceId = App::instance()->settings()->deviceId();
    BYTE *guidBytes = (BYTE *)pGuid;

    while (deviceId.length() > 0)
    {
        auto two = deviceId.left(2);
        auto ok = false;
        auto value = two.toInt(&ok, 16);
        qFatalIf(!ok, "Bad deviceId");
        *guidBytes = static_cast<BYTE>(value);
        guidBytes++;
        deviceId.remove(0, 2);
    }

    return TRUE;
}

CHAR *CHost_Qt::GetDeviceName()
{
    auto username = App::instance()->settings()->username();
    if (username.isEmpty())
    {
        return AllocString("CyberTracker Classic");
    }
    else
    {
        return AllocString(username.toStdString().c_str());
    }
}

BOOL CHost_Qt::CreateGuid(GUID *pGuid)
{
    auto guid = QUuid::createUuid().toRfc4122();
    memcpy(pGuid, guid.data(), sizeof(GUID));
    return TRUE;
}

VOID CHost_Qt::GetDateTime(FXDATETIME *pDateTime)
{
    QDate nowDate = QDate::currentDate();
    QTime nowTime = QTime::currentTime();

    memset(pDateTime, 0, sizeof(FXDATETIME));

    pDateTime->Year = nowDate.year();
    pDateTime->Month = nowDate.month();
    pDateTime->Day = nowDate.day();
    pDateTime->Hour = nowTime.hour();
    pDateTime->Minute = nowTime.minute();
    pDateTime->Second = nowTime.second();
    pDateTime->Milliseconds = 0;

    DOUBLE d = EncodeDateTime(pDateTime) + _gpsTimeOffset;

    DecodeDateTime(d, pDateTime);
}

CHAR *CHost_Qt::GetDateStringUTC(FXDATETIME *pDateTime, BOOL FormatAsUTC)
{
    auto dateTime = QDateTime();
    dateTime.setDate(QDate(pDateTime->Year, pDateTime->Month, pDateTime->Day));
    dateTime.setTime(QTime(pDateTime->Hour, pDateTime->Minute, pDateTime->Second, pDateTime->Milliseconds));

    auto dateString = QString();

    if (FormatAsUTC)
    {
        dateString = Utils::encodeTimestamp(dateTime.toUTC());
    }
    else
    {
        dateString = App::instance()->timeManager()->formatDateTime(dateTime.toMSecsSinceEpoch());
    }

    return AllocString(dateString.toStdString().c_str());
}

VOID CHost_Qt::SetDateTime(FXDATETIME */*pDateTime*/)
{
    qDebug() << "SetDateTime not implemented";
}

VOID CHost_Qt::SetTimeZone(INT /*BiasInMinutes*/)
{
    qDebug() << "SetTimeZone not implemented";
}

VOID CHost_Qt::SetDateTimeFromGPS(FXDATETIME */*pDateTime*/)
{
    qDebug() << "SetDateTimeFromGPS not implemented";
}

CHAR *CHost_Qt::GetDateString(FXDATETIME *pDateTime)
{
    auto date = QDate(pDateTime->Year, pDateTime->Month, pDateTime->Day);
    auto dateString = App::instance()->locale().toString(date, QLocale::FormatType::ShortFormat);

    return AllocString(dateString.toStdString().c_str());
}

static CHAR *g_SoundFileWAV = "sound.wav";
static CHAR *g_SoundFileMP3 = "sound.mp3";

BOOL CHost_Qt::PlaySound(FXSOUNDRESOURCE *pSound)
{
    StopSound();

    UINT header;
    memcpy(&header, &pSound->Data[0], sizeof(UINT));
    CHAR *fullPath = AllocPathApplication((header == 'FFIR') ? g_SoundFileWAV : g_SoundFileMP3);

    HANDLE fileHandle = 0;
    fileHandle = FileOpen(fullPath, "wb");
    if (fileHandle == 0)
    {
        qDebug() << "Failed to write sound";
    	goto cleanup;
    }

    if (!FileWrite(fileHandle, &pSound->Data[0], pSound->Size))
    {
        qDebug() << "Failed to write file";
    	goto cleanup;
    }

	FileClose(fileHandle);
	fileHandle = 0;

    _mediaPlayer.setMedia(QUrl::fromLocalFile(fullPath));
    _mediaPlayer.play();

 cleanup:

    MEM_FREE(fullPath);

    if (fileHandle != 0)
    {
    	FileClose(fileHandle);
    }

    return FALSE;
}

BOOL CHost_Qt::StopSound()
{
    CHAR *fullPath;

    fullPath = AllocPathApplication(g_SoundFileWAV);
  	FxDeleteFile(fullPath);
  	MEM_FREE(fullPath);

    fullPath = AllocPathApplication(g_SoundFileMP3);
  	FxDeleteFile(fullPath);
  	MEM_FREE(fullPath);

    _mediaPlayer.stop();

	return TRUE;
}

BOOL CHost_Qt::IsSoundPlaying()
{
    return _mediaPlayer.state() == QMediaPlayer::PlayingState;
}

BOOL CHost_Qt::StartRecording(CHAR *pFileNameNoExt, UINT Duration, UINT Frequency)
{
    StopSound();
    CancelRecording();

    _recordingFileName = QString(pFileNameNoExt) + ".wav";
    _audioRecorder.start(_recordingFileName, Frequency);
    _audioTimer.start(Duration);

    return TRUE;
}

BOOL CHost_Qt::StopRecording()
{
    _audioTimer.stop();
    if (!_recordingFileName.isEmpty())
    {
        auto duration = _audioRecorder.stop();

        CfxApplication *application = GetApplication(this);
        application->GetEngine()->DoRecordingComplete((char *)_recordingFileName.toStdString().c_str(), duration);

        _recordingFileName.clear();
    }

    return TRUE;
}

BOOL CHost_Qt::PlayRecording(CHAR *pFileName)
{
    StopSound();
    CancelRecording();

    _mediaPlayer.setMedia(QUrl::fromLocalFile(pFileName));
    _mediaPlayer.play();

    return TRUE;
}

VOID CHost_Qt::CancelRecording()
{
    _audioTimer.stop();
    if (!_recordingFileName.isEmpty())
    {
        _audioRecorder.stop();
        QFile::remove(_recordingFileName);
        _recordingFileName.clear();
    }
}

BOOL CHost_Qt::IsRecording()
{
    return !_recordingFileName.isEmpty();
}

VOID CHost_Qt::SetTimer(UINT Elapse, UINT TimerId)
{
    KillTimer(TimerId);
    auto timer = new QTimer(nullptr);

    connect(timer, &QTimer::timeout, [this, TimerId]()
    {
        GetApplication(this)->GetEngine()->DoTimer(TimerId);
    });

    _timers.insert(TimerId, timer);
    timer->start(Elapse);
}

VOID CHost_Qt::KillTimer(UINT TimerId)
{
    if (!_timers.contains(TimerId)) {
        return;
    }

    delete _timers[TimerId];
    _timers.remove(TimerId);
}

VOID CHost_Qt::SetAlarm(UINT ElapseInSeconds)
{
    App::instance()->setAlarm(CLASSIC_ALARM_ID, ElapseInSeconds);
}

VOID CHost_Qt::KillAlarm()
{
    App::instance()->killAlarm(CLASSIC_ALARM_ID);
}

VOID CHost_Qt::RequestUrl(CHAR *pUrl, CHAR *pUserName, CHAR *pPassword, CfxStream &Data, CHAR *pFileName, UINT *pId)
{
    auto url = QUrl(pUrl);
    auto username = QString(pUserName);
    auto password = QString(pPassword);

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Content-Type", "application/json; charset=utf-8");

    if (!username.isEmpty() && !password.isEmpty())
    {
        auto auth = "Basic " + (username + ":" + password).toLocal8Bit().toBase64();
        request.setRawHeader("Authorization", auth);
    }

    auto reply = _networkAccessManager.post(request, QByteArray((char*)Data.GetMemory(), Data.GetSize() - 1));
    auto id = _networkRequestMaxId;
    _networkRequestMaxId++;
    _networkRequests.append(id);
    auto filename = QString(pFileName);

    QObject::connect(reply, &QNetworkReply::finished, [this, reply, id, filename]()
    {
        auto errorCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        auto success = reply->error() == QNetworkReply::NoError;

        if (_networkRequests.indexOf(id) != -1)
        {
            _networkRequests.removeOne(id);

            {
                QFile file(filename);
                file.open(QIODevice::WriteOnly);
                QTextStream streamFileOut(&file);
                streamFileOut.setCodec("UTF-8");
                streamFileOut << reply->readAll();
            }

            GetApplication(this)->GetEngine()->DoUrlRequestCompleted(id, filename.toStdString().c_str(), errorCode);
        }

        if (!success)
        {
            qDebug() << "HTTP error: " << reply->errorString();
        }

        reply->deleteLater();
    });

    *pId = id;
}

VOID CHost_Qt::CancelUrl(UINT Id)
{
    _networkRequests.removeOne(Id);
}

BOOL CHost_Qt::ConnectPort(BYTE /*PortId*/, FX_COMPORT */*pComPort*/)
{
    //return g_SerialPorts->ConnectPort(PortId, pComPort);
    return FALSE;
}

BOOL CHost_Qt::IsPortConnected(BYTE /*PortId*/)
{
    //return g_SerialPorts->IsConnected(PortId);
    return FALSE;
}

VOID CHost_Qt::DisconnectPort(BYTE /*PortId*/)
{
    //g_SerialPorts->DisconnectPort(PortId);
}

VOID CHost_Qt::WritePortData(BYTE /*PortId*/, BYTE */*pData*/, UINT /*Length*/)
{
    //g_SerialPorts->WritePortData(PortId, pData, Length);
}

void CHost_Qt::transferStateChanged(int state)
{
    _transferState.Mode = (FXSENDSTATEMODE)state;
    GetApplication(this)->GetEngine()->DoTransferEvent(_transferState.Mode);
}

VOID CHost_Qt::StartTransfer(CHAR *pOutgoingPath)
{
    if (!App::instance()->uploadAllowed())
    {
        transferStateChanged(FXSENDSTATEMODE::SS_NO_WIFI);
        return;
    }

    _transferState.Mode = FXSENDSTATEMODE::SS_CONNECTING;
    connect(&_transfer, &QtTransfer::stateChange, this, &CHost_Qt::transferStateChanged, Qt::QueuedConnection);
    _transfer.start(pOutgoingPath);
}

VOID CHost_Qt::CancelTransfer()
{
    _transfer.stop();
}

VOID CHost_Qt::GetTransferState(FXSENDSTATE *pState)
{
    *pState = _transferState;
}

VOID CHost_Qt::GetGPS(FXGPS *pGPS)
{
    if (_gpsTimeStamp == 0) 
    {
        _gps.State = dsDetecting;
    } 
    else if (FxGetTickCount() - _gpsTimeStamp < FxGetTicksPerSecond() * 10)
    {
        _gps.State = dsConnected;
    } 
    else 
    {
        memset(&_gps, 0, sizeof(_gps));
        _gps.State = dsDisconnected;
    }

    memcpy(pGPS, &_gps, sizeof(FXGPS));;
}

VOID CHost_Qt::SetGPS(BOOL OnOff)
{
    if (_gpsOn == OnOff)
    {
        return;
    }

    auto satelliteManager = App::instance()->satelliteManager();

    _gpsOn = OnOff;

    if (!OnOff)
    {
        memset(&_gps, 0, sizeof(_gps));

        satelliteManager->stop();
        disconnect(satelliteManager, &SatelliteManager::satellitesChanged, 0, 0);

        m_positionSource->stopUpdates();

        delete m_positionSource;
        m_positionSource = nullptr;

        m_simulateSocket.close();
        return;
    }

    m_positionSource = new PositionInfoSource(this);

    connect(m_positionSource, &QGeoPositionInfoSource::positionUpdated, this, [=](const QGeoPositionInfo& update)
    {
        Location location(update);

        FXGPS *finalGps = GetGPSPtr();
        finalGps->State = dsConnected;
        finalGps->TimeStamp = FxGetTickCount();
        finalGps->Position.Quality = fq3D;

        auto accuracy = location.accuracy();
        if (std::isnan(accuracy))
        {
            accuracy = 100;
        }

        auto speed = location.speed();
        if (std::isnan(speed))
        {
            speed = 0;
        }

        auto direction = location.direction();
        if (std::isnan(direction))
        {
            direction = 0;
        }

        finalGps->Position.Latitude = location.y();
        finalGps->Position.Longitude = location.x();
        finalGps->Position.Altitude = location.z();
        finalGps->Position.Accuracy = min(50, accuracy / 10);
        finalGps->Position.Speed = speed;
        finalGps->Heading = static_cast<UINT>(direction);

        FXDATETIME *pDateTime = &finalGps->DateTime;
        memset(pDateTime, 0, sizeof(FXDATETIME));

        auto ts = update.timestamp().toUTC();
        pDateTime->Second = static_cast<UINT16>(ts.time().second());
        pDateTime->Minute = static_cast<UINT16>(ts.time().minute());
        pDateTime->Hour = static_cast<UINT16>(ts.time().hour());
        pDateTime->Month = static_cast<UINT16>(ts.date().month());
        pDateTime->Year = static_cast<UINT16>(ts.date().year());
        pDateTime->Day = static_cast<UINT16>(ts.date().day());

        GPSDataReceived();

        auto app = App::instance();
        if (!app->lastLocationAccurate())
        {
            finalGps->Position.Quality = fqNone;
            emit app->showToast(app->locationAccuracyText());
            return;
        }
    });

    m_positionSource->setUpdateInterval(1000);
    m_positionSource->startUpdates();

    // Start the GPS in the "detecting" state.
    {
        FXGPS *finalGps = GetGPSPtr();
        finalGps->State = dsDetecting;
        finalGps->TimeStamp = FxGetTickCount();
        finalGps->Position.Quality = fqNone;
    }

    satelliteManager->start();
    connect(satelliteManager, &SatelliteManager::satellitesChanged, [this, satelliteManager]()
    {
        auto satellites = satelliteManager->snapSatellites();

        FXGPS *gps = GetGPSPtr();

        gps->SatellitesInView = qMin(satellites.count(), MAX_SATELLITES);
        memset(gps->Satellites, 0, sizeof(gps->Satellites));

        for (auto i = 0; i < gps->SatellitesInView; i++)
        {
            auto satellite = satellites[i];
            auto sat = &gps->Satellites[i];

            sat->PRN = satellite.id;
            sat->Azimuth = satellite.azimuth;
            sat->Elevation = satellite.elevation;
            sat->UsedInSolution = satellite.used;
            sat->SignalQuality = satellite.strength;
        }
    });
}

VOID CHost_Qt::ResetGPSTimeouts()
{
}

BYTE CHost_Qt::GetGPSPowerupTimeSeconds()
{
    return 0;
}

BOOL CHost_Qt::HasSmartGPSPower()
{
    return TRUE;
}

VOID CHost_Qt::GetRange(FXRANGE */*pRange*/)
{
    //g_RangeFinder->GetState(pRange);
}

VOID CHost_Qt::SetRangeFinder(BOOL /*OnOff*/)
{
    //g_RangeFinder->SetState(OnOff);
}

VOID CHost_Qt::ResetRangeFinderTimeouts()
{
    //g_RangeFinder->ResetTimeouts();
}

FXPOWERMODE CHost_Qt::GetPowerState()
{
	return powOn;
}

VOID CHost_Qt::SetPowerState(FXPOWERMODE /*Mode*/)
{
}

FXBATTERYSTATE CHost_Qt::GetBatteryState()
{
    auto level = GetBatteryLevel();
    auto charging = App::instance()->batteryCharging();

    if (charging) {
        return FXBATTERYSTATE::batCharge;
    }

    if (level < 5) {
        return FXBATTERYSTATE::batCritical;
    }

    if (level < 20) {
        return FXBATTERYSTATE::batLow;
    }

    return FXBATTERYSTATE::batHigh;
}

UINT8 CHost_Qt::GetBatteryLevel()
{
    return App::instance()->batteryLevel();
}

VOID CHost_Qt::SetBatteryState(UINT Level, UINT State, UINT PowerState)
{
    _batteryLevel = Level;
    _batteryState = State;
    _powerState = PowerState;
}

UINT CHost_Qt::GetFreeMemory()
{
    return 0xFFFFFF;
}

BOOL CHost_Qt::AppendRecord(CHAR *pFileName, VOID *pRecord, UINT RecordSize)
{
    BOOL success = FALSE;
    CHAR *fullPath = AllocPathApplication(pFileName);
    HANDLE fileHandle = NULL;

    if (fullPath)
    {
        fileHandle = FileOpen(fullPath, "ab");
    }

    if (fileHandle)
    {
        success = FileWrite(fileHandle, pRecord, RecordSize);
        FileClose(fileHandle);
    }

    RequestMediaScan(fullPath);

    MEM_FREE(fullPath);

    return success;
}

BOOL CHost_Qt::DeleteRecords(CHAR *pFileName)
{
    BOOL result = FALSE;
    CHAR *fullPath = AllocPathApplication(pFileName);

    if (fullPath) 
    {
        result = FxDeleteFile(fullPath);
        RequestMediaScan(fullPath);
        MEM_FREE(fullPath);
    }

    return result;
}

BOOL CHost_Qt::EnumRecordInit(CHAR *pFileName)
{
    FX_ASSERT(_enumFileMap.BasePointer == NULL);
    _enumFileIndex = 0;

    BOOL result = FALSE;
    CHAR *fullPath = AllocPathApplication(pFileName);

    if (fullPath && FxDoesFileExist(fullPath))
    {
        result = FxMapFile(fullPath, &_enumFileMap);
        MEM_FREE(fullPath);
    }

    return result;
}

BOOL CHost_Qt::EnumRecordSetIndex(UINT Index, UINT RecordSize)
{
    FX_ASSERT(_enumFileMap.BasePointer != NULL);

    UINT location = Index * RecordSize;

    if (location < _enumFileMap.FileSize) 
    {
        _enumFileIndex = location;
        return TRUE;
    } 
    else 
    {
        return FALSE;
    }
}

BOOL CHost_Qt::EnumRecordNext(VOID *pRecord, UINT RecordSize)
{
    FX_ASSERT(_enumFileMap.BasePointer != NULL);

    if (_enumFileIndex + RecordSize <= _enumFileMap.FileSize)
    {
        memcpy(pRecord, (BYTE *)_enumFileMap.BasePointer + _enumFileIndex, RecordSize);
        _enumFileIndex += RecordSize;
        return TRUE;
    } 
    else 
    {
        return FALSE;
    }
}

BOOL CHost_Qt::EnumRecordDone()
{
    FxUnmapFile(&_enumFileMap);
    _enumFileIndex = 0;
    return TRUE;
}

struct EDIT_CONTROL
{
    GUID Guid;
    BOOL SingleLine = FALSE;
    BOOL Password = FALSE;
    QString Text;
};

HANDLE CHost_Qt::CreateEditControl(BOOL SingleLine, BOOL Password)
{
    auto editControl = new EDIT_CONTROL();
    editControl->SingleLine = SingleLine;
    editControl->Password = Password;
    CreateGuid(&editControl->Guid);

    return (HANDLE)editControl;
}

VOID CHost_Qt::DestroyEditControl(HANDLE Control)
{
    if (_window == nullptr)
    {
        return;
    }

    auto editControl = (EDIT_CONTROL*)Control;
    emit _window->hideEditControl();
    delete editControl;
}

VOID CHost_Qt::MoveEditControl(HANDLE Control, INT Left, INT Top, UINT Width, UINT Height)
{
    if (_window == nullptr)
    {
        return;
    }

    auto editControl = (EDIT_CONTROL*)Control;
    auto params = QVariantMap
    {
        { "x", Left },
        { "y", Top },
        { "width", Width },
        { "height", Height },
        { "singleLine", editControl->SingleLine },
        { "password", editControl->Password },
    };

    emit _window->showEditControl(params);
}

VOID CHost_Qt::FocusEditControl(HANDLE /*Control*/)
{
}

CHAR *CHost_Qt::GetEditControlText(HANDLE /*Control*/)
{
    if (_window == nullptr)
    {
        return AllocString("");
    }

    return AllocString(_window->lastEditorText().toUtf8());
}

VOID CHost_Qt::SetEditControlText(HANDLE /*Control*/, CHAR *pValue)
{
    if (_window == nullptr)
    {
        return;
    }

    _window->set_lastEditorText(QString::fromUtf8(pValue ? pValue : ""));
}

VOID CHost_Qt::SetEditControlFont(HANDLE /*Control*/, FXFONTRESOURCE *pFont)
{
    if (_window == nullptr)
    {
        return;
    }

    auto params = QVariantMap
    {
        { "family", pFont->Name },
        { "bold", (pFont->Style & 1) != 0 },
        { "italic", (pFont->Style & 2) != 0 },
        { "height", pFont->Height },
    };

    emit _window->setEditControlFont(params);
}

BOOL CHost_Qt::IsCameraSupported()
{
    return TRUE;
}

BOOL CHost_Qt::ShowCameraDialog(CHAR *pFileNameNoExt, FXIMAGE_QUALITY ImageQuality)
{
    if (_window == nullptr)
    {
        return true;
    }

    *pFileNameNoExt = '\0';

    auto rx = -1;
    auto ry = -1;
    switch (ImageQuality)
    {
    case iqVeryHigh:
        rx = 2560;
        ry = 1920;
        break;

    case iqHigh:
    case iqDefault:
        rx = 1600;
        ry = 1200;
        break;

    case iqMedium:
        rx = 1280;
        ry = 960;
        break;

    case iqLow:
        rx = 800;
        ry = 600;
        break;

    case iqVeryLow:
        rx = 640;
        ry = 480;
        break;

    case iqUltraLow:
        rx = 320;
        ry = 240;
        break;

    default:
        break;
    };

    emit _window->showCameraDialog(rx, ry);

    QObject *context = new QObject(this);
    connect(App::instance(), &App::photoTaken, context, [&, context](const QString& filename)
    {
        delete context;

        auto filePath = App::instance()->getMediaFilePath(filename);
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists())
        {
            return;
        }

        CfxApplication *application = GetApplication(this);
        CctSession *session = (CctSession *)application->GetScreen()->GetControl(0);

        auto newFilePath = session->AllocMediaFileName((char*)filename.toStdString().c_str());
        if (QFile::rename(filePath, QString(newFilePath)))
        {
            Utils::mediaScan(newFilePath);
            application->GetEngine()->DoPictureTaken(application->GetScreen(), newFilePath, true);
        }
        MEM_FREE(newFilePath);
    });

    return true;
}

VOID CHost_Qt::ShowSkyplot(INT Left, INT Top, UINT Width, UINT Height, BOOL Visible)
{
    if (_window == nullptr)
    {
        return;
    }

    emit _window->showSkyplot(Left, Top, Width, Height, Visible);
}

VOID CHost_Qt::ShowToast(const CHAR* pMessage)
{
    emit App::instance()->showToast(pMessage);
}

VOID CHost_Qt::ShowExports()
{
    if (_window == nullptr)
    {
        return;
    }

    emit _window->showExports();
}

VOID CHost_Qt::ShareData()
{
    if (_window == nullptr)
    {
        return;
    }

    emit _window->shareData();
}

BOOL CHost_Qt::IsBarcodeSupported()
{
    return TRUE;
}

BOOL CHost_Qt::ShowBarcodeDialog(CHAR */*pBarcode*/)
{
    if (_window == nullptr)
    {
        return true;
    }

    emit _window->showBarcodeDialog();

    QObject *context = new QObject(this);
    connect(App::instance(), &App::barcodeScan, context, [&, context](const QString& barcode)
    {
        delete context;

        CfxApplication *application = GetApplication(this);
        application->GetEngine()->DoBarcodeScan(application->GetScreen(), (char*)(barcode.toStdString().c_str()), true);
    });

    return true;
}

BOOL CHost_Qt::IsEsriSupported()
{
    return TRUE;
}

BOOL CHost_Qt::TestEsriCredentials(CHAR *pUserName, CHAR *pPassword)
{
    auto username = QString(pUserName);
    auto password = QString(pPassword);

    if (username.isEmpty() || password.isEmpty())
    {
        return FALSE;
    }

    ArcGisClient arcGisClient;
    arcGisClient.set_username(username);
    arcGisClient.set_password(password);
    return arcGisClient.authorizeToken();
}

CHAR *CHost_Qt::AllocPathApplication(const CHAR *pFileName)
{
    if (pFileName && strchr(pFileName, PATH_SLASH_CHAR))
    {
        return AllocString(pFileName);
    }

    CHAR *result = AllocMaxPath(_applicationPath);

    if (result)
    {
        AppendSlash(result);
    }

    if (pFileName)
    {
        strcat(result, pFileName);
    }

    return result;
}

CHAR *CHost_Qt::AllocPathSD()
{
#if defined(Q_OS_ANDROID)
    auto sdPath = QtAndroid::androidActivity().callObjectMethod("getBestSDPath", "()Ljava/lang/String;").toString();
    if (sdPath.isEmpty())
    {
        return AllocPathApplication("Backup");
    }

    auto result = AllocMaxPath(sdPath.toStdString().c_str());
    AppendSlash(result);
    strcat(result, "cybertrackerdata");

    if (!FxCreateDirectory(result))
    {
        MEM_FREE(result);
        return nullptr;
    }

    Utils::mediaScan(QString(result));

    return result;
#else
    return AllocPathApplication("Backup");
#endif
}

VOID CHost_Qt::RegisterExportFile(CHAR *pExportFilePath, FXEXPORTFILEINFO* exportFileInfo)
{
    if (_window == nullptr)
    {
        return;
    }

    auto now = App::instance()->timeManager()->currentDateTime();

    auto project = App::instance()->projectManager()->loadPtr(_window->projectUid());
    auto fileInfo = ExportFileInfo { project->uid(), project->provider(), project->title(), now.toString("yyyy-MM-dd hh:mm:ss") };
    fileInfo.sightingCount = exportFileInfo->sightingCount;
    fileInfo.locationCount = exportFileInfo->waypointCount;

    auto makeDateTime = [&](const FXDATETIME* dt) -> QDateTime
    {
        return QDateTime(QDate(dt->Year, dt->Month, dt->Day), QTime(dt->Hour, dt->Minute, dt->Second));
    };

    fileInfo.startTime = App::instance()->timeManager()->formatDateTime(makeDateTime(&exportFileInfo->startTime));
    fileInfo.stopTime = App::instance()->timeManager()->formatDateTime(makeDateTime(&exportFileInfo->stopTime));

    App::instance()->registerExportFile(pExportFilePath, fileInfo.toMap());

    emit App::instance()->showToast(tr("Data exported"));
}

VOID CHost_Qt::ArchiveSighting(GUID* pId, FXDATETIME* dateTime, FXGPS_POSITION* gps, const QVariantMap& data)
{
    auto app = App::instance();

    auto sightingUid = GUIDToString(pId);
    auto sighting = _window->ownerForm()->createSightingPtr();
    sighting->recordManager()->rootRecord()->setRecordUid(sightingUid);
    sighting->recalculate();

    // Set the timestamp.
    auto timestamp = GetDateStringUTC(dateTime, false);
    auto dt = Utils::decodeTimestamp(timestamp);
    sighting->snapCreateTime(timestamp);
    sighting->snapUpdateTime(timestamp);
    MEM_FREE(timestamp);

    // Set the location.
    if (gps->Quality > fqNone)
    {
        sighting->setRootFieldValue("location", LocationField::createValue(gps->Longitude, gps->Latitude, gps->Altitude, gps->Accuracy * 10, gps->Speed));
    }

    // Set the field values.
    auto counter = 0;
    for (auto it = data.constKeyValueBegin(); it != data.constKeyValueEnd(); it++)
    {
        auto fieldUid = it->first;
        auto value = it->second;

        if (value.toString().startsWith("media://"))
        {
            auto filename = value.toString().replace("media://", "");
            auto fileInfo = QFileInfo(Utils::classicRoot() + "/Media/" + filename);

            auto mimeTypePrefix = Utils::mimeDatabase()->mimeTypeForFile(fileInfo).name().split("/").constFirst();
            auto newFilename = QString("%1-%2-%3.%4").arg(mimeTypePrefix, dt.toString("yyyyMMdd-hhmmss-zzz"), QString::number(counter++), fileInfo.suffix());
            auto newFilePath = app->mediaPath() + "/" + newFilename;
            QFile::copy(fileInfo.filePath(), newFilePath);

            if (mimeTypePrefix == "image")
            {
                value = PhotoField::createValue(newFilename);
            }
            else if (mimeTypePrefix == "audio")
            {
                value = AudioField::createValue(newFilename);
            }
            else
            {
                qDebug() << "Error: unknown media type - " << mimeTypePrefix;
            }
        }

        sighting->setRootFieldValue(fieldUid, value);
    }

    // Save the sighting to the database.
    sighting->recalculate();
    auto sightingAttachments = QStringList();
    auto sightingData = sighting->save(&sightingAttachments);
    app->database()->saveSighting(_window->projectUid(), "", sightingUid, Sighting::DB_SIGHTING_FLAG, sightingData, sightingAttachments);
}

VOID CHost_Qt::ArchiveWaypoint(GUID* pId, FXDATETIME* dateTime, FXGPS_POSITION* gps)
{
    auto app = App::instance();

    auto locationUid = GUIDToString(pId);
    Location location;
    location.set_x(gps->Longitude);
    location.set_y(gps->Latitude);
    location.set_z(gps->Altitude);
    location.set_accuracy(gps->Accuracy * 10);
    location.set_accuracyFilter(app->settings()->locationAccuracyMeters());
    location.set_distanceFilter(_trackStreamer.distanceFilter());
    location.set_interval(_trackStreamer.updateInterval() / 1000);
    location.set_counter(_trackStreamer.counter());
    location.set_deviceId(App::instance()->settings()->deviceId());

    auto timestamp = GetDateStringUTC(dateTime, false);
    location.set_timestamp(timestamp);
    MEM_FREE(timestamp);

    app->database()->saveSighting(_window->projectUid(), "", locationUid, Sighting::DB_LOCATION_FLAG, location.toMap(), QStringList());
}
