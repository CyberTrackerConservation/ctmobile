#include "pch.h"

#include "fxApplication.h"
#include "ctSession.h"
#include "fxHost.h"
#include "fxUtils.h"
#include "UtilsAudio.h"
#include "App.h"
#include "Location.h"
#include "Classic/ClassicConnector.h"
#include "Classic/ClassicView.h"

#include <string.h>

constexpr char CLASSIC_ALARM_ID[] = "CLASSIC_ALARM_ID";

//*************************************************************************************************

CfxHost::CfxHost(CfxPersistent *pOwner, ClassicView *pWindow, FXPROFILE *pProfile, const char *pApplicationPath): CfxPersistent(pOwner)
{
    _canvas = 0;
    _left = _top = 0;
    _useResourceScale = true;
    _gpsTimeOffset = 0;
    memcpy(&_profile, pProfile, sizeof(FXPROFILE));

    _window = pWindow;
    _batteryLevel = _batteryState = _powerState = 0;
    _batteryState = (int)FXBATTERYSTATE::High;

    memset(&_gps, 0, sizeof(_gps));

    memset(&_enumFileMap, 0, sizeof(_enumFileMap));
    _enumFileIndex = 0;

    _applicationPath = AllocString(pApplicationPath);

    _mediaPlayer.setAudioOutput(new QAudioOutput(this));

    _audioTimer.setSingleShot(true);
    connect(&_audioTimer, &QTimer::timeout, this, [this]()
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

    m_positionSource = new PositionInfoSource(this);
    m_positionSource->setUpdateInterval(1000);

    connect(m_positionSource, &QGeoPositionInfoSource::positionUpdated, this, [&](const QGeoPositionInfo& update)
    {
        Location location(update);

        _gps.State = FXDEVICE_STATE::Connected;
        _gps.TimeStamp = FxGetTickCount();
        _gps.Position.Quality = FXGPS_QUALITY::Fix3D;

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

        _gps.Position.Latitude = location.y();
        _gps.Position.Longitude = location.x();
        _gps.Position.Altitude = std::isnan(location.z()) ? 0 : location.z();
        _gps.Position.Accuracy = min(50, accuracy / 10);
        _gps.Position.Speed = speed;
        _gps.Heading = static_cast<int>(direction);

        FXDATETIME *pDateTime = &_gps.DateTime;
        memset(pDateTime, 0, sizeof(FXDATETIME));

        auto ts = update.timestamp().toUTC();
        pDateTime->Second = static_cast<UINT16>(ts.time().second());
        pDateTime->Minute = static_cast<UINT16>(ts.time().minute());
        pDateTime->Hour = static_cast<UINT16>(ts.time().hour());
        pDateTime->Month = static_cast<UINT16>(ts.date().month());
        pDateTime->Year = static_cast<UINT16>(ts.date().year());
        pDateTime->Day = static_cast<UINT16>(ts.date().day());

        auto app = App::instance();
        if (!app->lastLocationAccurate())
        {
            _gps.Position.Quality = FXGPS_QUALITY::None;
            emit app->showToast(app->locationAccuracyText());
            return;
        }
    });

    connect(&_audioRecorder, &WaveFileRecorder::startFailed, this, [&](const QString&)
    {
        StopRecording();
    });

    connect(App::instance(), &App::photoTaken, this, [&](const QString& filename)
    {
        auto filePath = App::instance()->getMediaFilePath(filename);
        if (!QFileInfo::exists(filePath))
        {
            return;
        }

        auto filePathStd = filePath.toStdString();
        auto application = GetApplication(this);
        application->GetEngine()->DoPictureTaken(application->GetScreen(), (char *)filePathStd.c_str(), true);
    });

    connect(App::instance(), &App::barcodeScan, this, [&](const QString& barcode)
    {
        CfxApplication *application = GetApplication(this);
        application->GetEngine()->DoBarcodeScan(application->GetScreen(), (char*)(barcode.toStdString().c_str()), true);
    });

    connect(App::instance(), &App::locationAcquired, this, [&](double x, double y, double altitude, double accuracy, const QString& timestamp, const QVariantMap& contextData)
    {
        if (!contextData.contains("saveTarget"))
        {
            return;
        }

        CfxApplication *application = GetApplication(this);
        CctSession *session = (CctSession *)application->GetScreen()->GetControl(0);

        FXGPS_POSITION *sightingPosition = session->GetCurrentSighting()->GetGPS();

        if (_gps.Position.Quality != FXGPS_QUALITY::None)
        {
            *sightingPosition = _gps.Position;
        }
        else
        {
            sightingPosition->Quality = FXGPS_QUALITY::User2D;
            sightingPosition->Latitude = y;
            sightingPosition->Longitude = x;
            sightingPosition->Altitude = altitude;
            sightingPosition->Accuracy = accuracy;
            sightingPosition->Speed = 0.0;
        }

        auto saveTarget = contextData.value("saveTarget", 0).toInt();
        session->DoCommit(saveTarget);
    });
}

CfxHost::~CfxHost()
{
    qDeleteAll(_timers);
    disconnect(App::instance(), &App::alarmFired, 0, 0);

    SetGPS(false);

    StopSound();
    CancelRecording();

    FxUnmapFile(&_enumFileMap);
    _enumFileIndex = 0;

    _window = nullptr;

    delete _canvas;
}

CfxCanvas *CfxHost::GetCanvas()
{
    if (!_canvas)
    {
        _canvas = CreateCanvas(0, 0);
    }
    return _canvas;
}

void CfxHost::GetBounds(FXRECT *pRect)
{
    CfxCanvas *canvas = GetCanvas();

    pRect->Left = (int)_left;
    pRect->Top = (int)_top;
    pRect->Right = (int)(_left + canvas->GetWidth() - 1);
    pRect->Bottom = (int)(_top + + canvas->GetHeight() - 1);
}

void CfxHost::SetBounds(int Left, int Top, int Width, int Height)
{
    _left = Left;
    _top = Top;
    GetCanvas()->SetSize(Width, Height);
}


CfxCanvas *CfxHost::CreateCanvas(int Width, int Height)
{
    CfxCanvas *canvas = new CfxCanvas(this, _profile.ScaleFont);
    if (canvas && Width && Height)
    {
        canvas->SetSize((Width + 3) & ~3, Height);
    }

    return canvas;
}

CfxDatabase *CfxHost::CreateDatabase(char *pDatabaseName)
{
    char *fileName = AllocPathApplication(pDatabaseName);
    CfxDatabase *database = new CfxDatabase(this, fileName);
    MEM_FREE(fileName);
    return database;
}

CfxResource *CfxHost::CreateResource(char *pResourceName)
{
    char *fileName = AllocPathApplication(pResourceName);
    char *fullName = AllocMaxPath(fileName);
    strcat(fullName, CLIENT_RESOURCE_EXTENSION);

    CfxResource *resource = new CfxResource(this, fullName);

    MEM_FREE(fileName);
    MEM_FREE(fullName);

    return resource;
}

void CfxHost::RequestMediaScan(const char *pFileName)
{
    Utils::mediaScan(pFileName);
}

void CfxHost::Paint()
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
        auto image = _canvas->GetImage();
        _window->setImage(image);
        s_mutex = false;
    }

    _window->update();
}

void CfxHost::Startup()
{
}

void CfxHost::Shutdown()
{
    if (_window == nullptr)
    {
        return;
    }

    emit _window->shutdown();
}

bool CfxHost::GetTrackTimerRunning() const
{
    return _window->form()->trackStreamer()->running();
}

int CfxHost::GetTrackTimer() const
{
    auto project = _window->form()->project();
    return project->locationTrackInterval();
}

void CfxHost::SetTrackTimer(int Timeout)
{
    if (!_window)
    {
        return;
    }

    auto project = _window->form()->project();
    project->set_locationTrackInterval(Timeout);
    project->set_locationStreamingEnabled(Timeout > 0);
}

bool CfxHost::HasExternalCloseButton()
{
    return false;
}

bool CfxHost::HasExternalStateData()
{
    return false;
}

bool CfxHost::GetDeviceId(GUID *pGuid)
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

    return true;
}

char *CfxHost::GetDeviceName()
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

bool CfxHost::CreateGuid(GUID *pGuid)
{
    auto guid = QUuid::createUuid().toRfc4122();
    memcpy(pGuid, guid.data(), sizeof(GUID));
    return true;
}

void CfxHost::GetDateTime(FXDATETIME *pDateTime)
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

    double d = EncodeDateTime(pDateTime) + _gpsTimeOffset;

    DecodeDateTime(d, pDateTime);
}

char *CfxHost::GetDateStringUTC(FXDATETIME *pDateTime, bool FormatAsUTC)
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

void CfxHost::SetDateTime(FXDATETIME */*pDateTime*/)
{
}

void CfxHost::SetTimeZone(int /*BiasInMinutes*/)
{
}

char *CfxHost::GetDateString(FXDATETIME *pDateTime)
{
    auto date = QDate(pDateTime->Year, pDateTime->Month, pDateTime->Day);
    auto dateString = App::instance()->locale().toString(date, QLocale::FormatType::ShortFormat);

    return AllocString(dateString.toStdString().c_str());
}

bool CfxHost::IsSoundSupported()
{
    return true;
}

const static char *g_SoundFileWAV = "sound.wav";
const static char *g_SoundFileMP3 = "sound.mp3";

bool CfxHost::PlaySound(FXSOUNDRESOURCE *pSound)
{
    StopSound();

    int header;
    memcpy(&header, &pSound->Data[0], sizeof(int));
    char *fullPath = AllocPathApplication((header == 'FFIR') ? g_SoundFileWAV : g_SoundFileMP3);

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

    _mediaPlayer.setSource(QUrl());
    _mediaPlayer.setSource(QUrl::fromLocalFile(fullPath));
    _mediaPlayer.play();

 cleanup:

    MEM_FREE(fullPath);

    if (fileHandle != 0)
    {
        FileClose(fileHandle);
    }

    return false;
}

bool CfxHost::StopSound()
{
    char *fullPath;

    fullPath = AllocPathApplication(g_SoundFileWAV);
    FxDeleteFile(fullPath);
    MEM_FREE(fullPath);

    fullPath = AllocPathApplication(g_SoundFileMP3);
    FxDeleteFile(fullPath);
    MEM_FREE(fullPath);

    _mediaPlayer.stop();

    return true;
}

bool CfxHost::IsPlaying()
{
    return _mediaPlayer.isPlaying();
}

bool CfxHost::RequestPermissionRecordAudio()
{
    App::instance()->requestPermissionMicrophone(QVariant());
    auto permission = QMicrophonePermission();
    return qApp->checkPermission(permission) == Qt::PermissionStatus::Granted;
}

bool CfxHost::StartRecording(char *pFileNameNoExt, int Duration, int /*Frequency*/)
{
    StopSound();
    CancelRecording();

    _recordingFileName = QString(pFileNameNoExt) + ".wav";
    _audioRecorder.start(_recordingFileName/*, Frequency*/);
    _audioTimer.start(Duration);

    return true;
}

bool CfxHost::StopRecording()
{
    _audioTimer.stop();
    if (!_recordingFileName.isEmpty())
    {
        auto duration = _audioRecorder.stop();

        CfxApplication *application = GetApplication(this);
        application->GetEngine()->DoRecordingComplete((char *)_recordingFileName.toStdString().c_str(), duration);

        _recordingFileName.clear();
    }

    return true;
}

bool CfxHost::PlayRecording(char *pFileName)
{
    StopSound();
    CancelRecording();

    _mediaPlayer.setSource(QUrl::fromLocalFile(pFileName));
    _mediaPlayer.play();

    return true;
}

void CfxHost::CancelRecording()
{
    _audioTimer.stop();
    if (!_recordingFileName.isEmpty())
    {
        _audioRecorder.stop();
        QFile::remove(_recordingFileName);
        _recordingFileName.clear();
    }
}

bool CfxHost::IsRecording()
{
    return !_recordingFileName.isEmpty();
}

void CfxHost::SetTimer(int Elapse, int TimerId)
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

void CfxHost::KillTimer(int TimerId)
{
    if (!_timers.contains(TimerId)) {
        return;
    }

    delete _timers[TimerId];
    _timers.remove(TimerId);
}

void CfxHost::SetAlarm(int ElapseInSeconds)
{
    App::instance()->setAlarm(CLASSIC_ALARM_ID, ElapseInSeconds);
}

void CfxHost::KillAlarm()
{
    App::instance()->killAlarm(CLASSIC_ALARM_ID);
}

void CfxHost::RequestUrl(char *pUrl, char *pUserName, char *pPassword, CfxStream &Data, char *pFileName, int *pId)
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
        auto success = reply->isFinished() && reply->error() == QNetworkReply::NoError;

        if (_networkRequests.indexOf(id) != -1)
        {
            _networkRequests.removeOne(id);

            {
                QFile file(filename);
                file.open(QIODevice::WriteOnly);
                QTextStream streamFileOut(&file);
                streamFileOut.setEncoding(QStringConverter::Encoding::Utf8);
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

void CfxHost::CancelUrl(int Id)
{
    _networkRequests.removeOne(Id);
}

bool CfxHost::ConnectPort(BYTE /*PortId*/, FX_COMPORT */*pComPort*/)
{
    return false;
}

bool CfxHost::IsPortConnected(BYTE /*PortId*/)
{
    return false;
}

void CfxHost::DisconnectPort(BYTE /*PortId*/)
{
}

void CfxHost::WritePortData(BYTE /*PortId*/, BYTE */*pData*/, int /*Length*/)
{
}


void CfxHost::GetGPS(FXGPS *pGPS)
{
    if (m_positionSource->updateInterval() != 0)
    {
        _gps.State = FXDEVICE_STATE::Connected;
    }
    else
    {
        memset(&_gps, 0, sizeof(_gps));
        _gps.State = FXDEVICE_STATE::Disconnected;
    }

    memcpy(pGPS, &_gps, sizeof(FXGPS));;
}

void CfxHost::SetGPS(bool OnOff)
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

        return;
    }

    m_positionSource->startUpdates();

    // Start the GPS in the "detecting" state.
    {
        FXGPS *finalGps = GetGPSPtr();
        finalGps->State = FXDEVICE_STATE::Connected;
        finalGps->TimeStamp = FxGetTickCount();
        finalGps->Position.Quality = FXGPS_QUALITY::None;
    }

    satelliteManager->start();
    connect(satelliteManager, &SatelliteManager::satellitesChanged, this, [this, satelliteManager]()
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

void CfxHost::ResetGPSTimeouts()
{
}

void CfxHost::SetupGPSSimulator(double Latitude, double Longitude, int Radius)
{
}

BYTE CfxHost::GetGPSPowerupTimeSeconds()
{
    return 0;
}

bool CfxHost::HasSmartGPSPower()
{
    return true;
}

FXPOWERMODE CfxHost::GetPowerState()
{
    return FXPOWERMODE::On;
}

void CfxHost::SetPowerState(FXPOWERMODE /*Mode*/)
{
}

FXBATTERYSTATE CfxHost::GetBatteryState()
{
    auto level = GetBatteryLevel();
    auto charging = App::instance()->batteryCharging();

    if (charging) {
        return FXBATTERYSTATE::Charge;
    }

    if (level < 5) {
        return FXBATTERYSTATE::Critical;
    }

    if (level < 20) {
        return FXBATTERYSTATE::Low;
    }

    return FXBATTERYSTATE::High;
}

int CfxHost::GetBatteryLevel()
{
    return App::instance()->batteryLevel();
}

void CfxHost::SetBatteryState(int Level, int State, int PowerState)
{
    _batteryLevel = Level;
    _batteryState = State;
    _powerState = PowerState;
}

int CfxHost::GetFreeMemory()
{
    return 0xFFFFFF;
}

bool CfxHost::AppendRecord(char *pFileName, void *pRecord, int RecordSize)
{
    bool success = false;
    char *fullPath = AllocPathApplication(pFileName);
    HANDLE fileHandle = nullptr;

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

bool CfxHost::DeleteRecords(char *pFileName)
{
    bool result = false;
    char *fullPath = AllocPathApplication(pFileName);

    if (fullPath)
    {
        result = FxDeleteFile(fullPath);
        RequestMediaScan(fullPath);
        MEM_FREE(fullPath);
    }

    return result;
}

bool CfxHost::EnumRecordInit(char *pFileName)
{
    FX_ASSERT(_enumFileMap.BasePointer == nullptr);
    _enumFileIndex = 0;

    bool result = false;
    char *fullPath = AllocPathApplication(pFileName);

    if (fullPath && FxDoesFileExist(fullPath))
    {
        result = FxMapFile(fullPath, &_enumFileMap);
        MEM_FREE(fullPath);
    }

    return result;
}

bool CfxHost::EnumRecordSetIndex(int Index, int RecordSize)
{
    FX_ASSERT(_enumFileMap.BasePointer != nullptr);

    int location = Index * RecordSize;

    if (location < _enumFileMap.FileSize)
    {
        _enumFileIndex = location;
        return true;
    }
    else
    {
        return false;
    }
}

bool CfxHost::EnumRecordNext(void *pRecord, int RecordSize)
{
    FX_ASSERT(_enumFileMap.BasePointer != nullptr);

    if (_enumFileIndex + RecordSize <= _enumFileMap.FileSize)
    {
        memcpy(pRecord, (BYTE *)_enumFileMap.BasePointer + _enumFileIndex, RecordSize);
        _enumFileIndex += RecordSize;
        return true;
    }
    else
    {
        return false;
    }
}

bool CfxHost::EnumRecordDone()
{
    FxUnmapFile(&_enumFileMap);
    _enumFileIndex = 0;
    return true;
}

void CfxHost::RemoveReplicatedMedia(char *pFileName)
{
    App::instance()->removeMedia(pFileName);
}

bool CfxHost::IsCameraSupported()
{
    return true;
}

bool CfxHost::ShowCameraDialog(char *pFileNameNoExt, FXIMAGE_QUALITY ImageQuality, bool SimpleMode)
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
    case FXIMAGE_QUALITY::VeryHigh:
        rx = 2560;
        ry = 1920;
        break;

    case FXIMAGE_QUALITY::High:
    case FXIMAGE_QUALITY::Default:
        rx = 1600;
        ry = 1200;
        break;

    case FXIMAGE_QUALITY::Medium:
        rx = 1280;
        ry = 960;
        break;

    case FXIMAGE_QUALITY::Low:
        rx = 800;
        ry = 600;
        break;

    case FXIMAGE_QUALITY::VeryLow:
        rx = 640;
        ry = 480;
        break;

    case FXIMAGE_QUALITY::UltraLow:
        rx = 320;
        ry = 240;
        break;

    default:
        break;
    };

    emit _window->showCameraDialog(rx, ry, SimpleMode);

    return true;
}

void CfxHost::ShowToast(const char* pMessage)
{
    emit App::instance()->showToast(pMessage);
}

bool CfxHost::IsUploadQueueBusy()
{
    return App::instance()->uploadQueue()->processing();
}

bool CfxHost::IsBarcodeSupported()
{
    return true;
}

bool CfxHost::ShowBarcodeDialog(char */*pBarcode*/)
{
    if (_window == nullptr)
    {
        return true;
    }

    emit _window->showBarcodeDialog();

    return true;
}

bool CfxHost::IsPhoneNumberSupported()
{
    return false;
}

bool CfxHost::ShowPhoneNumberDialog()
{
    return false;
}

bool CfxHost::IsEsriSupported()
{
    return true;
}


char *CfxHost::AllocPathApplication(const char *pFileName)
{
    if (pFileName && strchr(pFileName, PATH_SLASH_CHAR))
    {
        return AllocString(pFileName);
    }

    char *result = AllocMaxPath(_applicationPath);

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

char *CfxHost::AllocPathSD()
{
#if defined(Q_OS_ANDROID)
    auto sdPath = QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("getBestSDPath", "()Ljava/lang/String;").toString();
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

bool CfxHost::IsUpdateSupported()
{
    return false;
}

void CfxHost::CheckForUpdate(GUID AppId, GUID StampId, char *pWebUpdateUrl, char *pTargetPath)
{
}

void CfxHost::RegisterExportFile(char *pExportFilePath, FXEXPORTFILEINFO* exportFileInfo)
{
    if (_window == nullptr)
    {
        return;
    }

    auto now = App::instance()->timeManager()->currentDateTime();

    auto project = App::instance()->projectManager()->loadPtr(GetClassicView(this)->projectUid());
    auto fileInfo = ExportFileInfo { project->uid(), project->provider(), project->title(), now.toString("yyyy-MM-dd hh:mm:ss") };
    fileInfo.sightingCount = exportFileInfo->sightingCount;
    fileInfo.locationCount = exportFileInfo->waypointCount;

    auto makeDateTime = [&](const FXDATETIME* dt) -> QDateTime
    {
        return QDateTime(QDate(dt->Year, dt->Month, dt->Day), QTime(dt->Hour, dt->Minute, dt->Second));
    };

    fileInfo.startTime = App::instance()->timeManager()->formatDateTime(makeDateTime(&exportFileInfo->startTime));
    fileInfo.stopTime = App::instance()->timeManager()->formatDateTime(makeDateTime(&exportFileInfo->stopTime));

#if defined(Q_OS_ANDROID)
    fileInfo.downloadUri = App::instance()->copyToDownload(pExportFilePath, "CyberTrackerData");
    if (!fileInfo.downloadUri.isEmpty())
    {
        qDebug() << "Exported file from " << pExportFilePath << " to " << fileInfo.downloadUri;
    }
    else
    {
        qDebug() << "Failed to export file to download folder";
    }
#endif

    App::instance()->exportFile()->registerFile(pExportFilePath, fileInfo.toMap());
    App::instance()->auditAdd(project->title(), "Register export file");
    App::instance()->backupFile()->registerFile(project->title(), pExportFilePath);

    emit App::instance()->showToast(tr("Data exported"));
}

void CfxHost::AddToUploadQueue(const QStringList& files, FXSENDDATA *sendData, FXEXPORTFILEINFO* exportFileInfo)
{
    auto queueItem = Database::DbQueueItem();
    queueItem.projectUid = GetClassicView(this)->projectUid();
    queueItem.url = sendData->Url;

    if (exportFileInfo)
    {
        queueItem.summary = QString("%1 %2, %3 %4").arg(exportFileInfo->sightingCount).arg(tr("Sightings")).arg(exportFileInfo->waypointCount).arg(tr("Points"));
    }
    else
    {
        queueItem.summary = tr("Data");
    }

    switch (sendData->Protocol)
    {
    case FXSENDDATA_PROTOCOL_AZURE:
        queueItem.type = ClassicConnector::UPLOAD_AZURE;
        break;

    case FXSENDDATA_PROTOCOL_FTP:
        queueItem.type =  ClassicConnector::UPLOAD_FTP;
        break;

    case FXSENDDATA_PROTOCOL_HTTP:
    case FXSENDDATA_PROTOCOL_HTTPS:
    case FXSENDDATA_PROTOCOL_HTTPZ:
        queueItem.type = ClassicConnector::UPLOAD_HTTP;
        break;

    case FXSENDDATA_PROTOCOL_ESRI:
        queueItem.type = ClassicConnector::UPLOAD_ESRI;
        break;

    default:
        qFatalIf(true, "Not yet implemented");
        return;
    };

    queueItem.payload = Utils::variantMapToJson(QVariantMap
    {
        { "username", QString(sendData->UserName) },
        { "password", QString(sendData->Password) },
        { "deflate", sendData->Protocol == FXSENDDATA_PROTOCOL_HTTPZ },
    });

    queueItem.attachments = files;

    // Backup CTX files.
    auto project = App::instance()->projectManager()->loadPtr(queueItem.projectUid);
    for (auto fileIt = files.constBegin(); fileIt != files.constEnd(); fileIt++)
    {
        if (fileIt->contains(".ctx", Qt::CaseInsensitive))
        {
            App::instance()->backupFile()->registerFile(project ? project->title() : "", App::instance()->getMediaFilePath(*fileIt));
        }
    }

    App::instance()->uploadQueue()->addItem(queueItem);
    App::instance()->uploadQueue()->kick();
}
