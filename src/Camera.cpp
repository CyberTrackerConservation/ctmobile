#include "Camera.h"
#include "App.h"

namespace CyberTracker {

Camera::Camera(QQuickItem* parent): QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    start();
}

Camera::~Camera()
{
}

bool Camera::active() const
{
    return m_camera && m_camera->isActive();
}

int Camera::cameraIndex() const
{
    return App::instance()->settings()->cameraIndex();
}

QString Camera::cameraIcon() const
{
    if (m_camera)
    {
        switch (m_camera->cameraDevice().position())
        {
        case QCameraDevice::BackFace:
            return "qrc:/icons/camera_rear.svg";

        case QCameraDevice::UnspecifiedPosition:
        case QCameraDevice::FrontFace:
            return "qrc:/icons/camera_front.svg";
        }
    }

    return QString();
}

QCamera::FlashMode Camera::flashMode() const
{
    return (QCamera::FlashMode)(App::instance()->settings()->cameraFlashMode());
}

QString Camera::flashModeIcon() const
{
    switch (flashMode())
    {
    case QCamera::FlashOff:
        return "qrc:/icons/flash_off.svg";

    case QCamera::FlashOn:
        return "qrc:/icons/flash_on.svg";

    case QCamera::FlashAuto:
        return "qrc:/icons/flash_auto.svg";

    default:
        return QString();
    }
}

void Camera::switchFlashMode()
{
    auto nextMode = QCamera::FlashOff;
    switch (flashMode())
    {
    case QCamera::FlashOff:
        nextMode = QCamera::FlashOn;
        break;

    case QCamera::FlashOn:
        nextMode = QCamera::FlashAuto;
        break;

    case QCamera::FlashAuto:
        nextMode = QCamera::FlashOff;
        break;
    }

    App::instance()->settings()->set_cameraFlashMode((int)nextMode);

    if (m_camera)
    {
        m_camera->setFlashMode(nextMode);
    }

    emit flashModeChanged();
}

QCamera::WhiteBalanceMode Camera::whiteBalanceMode() const
{
    return (QCamera::WhiteBalanceMode)(App::instance()->settings()->cameraWhiteBalanceMode());
}

QString Camera::whiteBalanceIcon() const
{
    switch (whiteBalanceMode())
    {
    case QCamera::WhiteBalanceAuto:
        return "qrc:/icons/wb_auto.svg";

    case QCamera::WhiteBalanceSunlight:
        return "qrc:/icons/wb_sunny.svg";

    case QCamera::WhiteBalanceCloudy:
        return "qrc:/icons/wb_cloudy.svg";

    case QCamera::WhiteBalanceTungsten:
        return "qrc:/icons/wb_incandescent.svg";

    case QCamera::WhiteBalanceFluorescent:
        return "qrc:/icons/wb_iridescent.svg";

    default:
        return "qrc:/icons/wb_auto.svg";
    }
}

void Camera::switchWhiteBalanceMode()
{
    auto nextMode = QCamera::WhiteBalanceAuto;

    switch (whiteBalanceMode())
    {
    case QCamera::WhiteBalanceAuto:
        nextMode = QCamera::WhiteBalanceSunlight;
        break;

    case QCamera::WhiteBalanceSunlight:
        nextMode = QCamera::WhiteBalanceCloudy;
        break;

    case QCamera::WhiteBalanceCloudy:
        nextMode = QCamera::WhiteBalanceTungsten;
        break;

    case QCamera::WhiteBalanceTungsten:
        nextMode = QCamera::WhiteBalanceFluorescent;
        break;

    case QCamera::WhiteBalanceFluorescent:
        nextMode = QCamera::WhiteBalanceAuto;
        break;

    default:
        nextMode = QCamera::WhiteBalanceAuto;
        break;
    }

    App::instance()->settings()->set_cameraWhiteBalanceMode((int)nextMode);

    if (m_camera)
    {
        m_camera->setWhiteBalanceMode(nextMode);
    }

    emit whiteBalanceChanged();
}

float Camera::zoomFactor() const
{
    return m_camera ? m_camera->zoomFactor() : 1.0;
}

void Camera::setZoomFactor(float value)
{
    if (!m_camera)
    {
        return;
    }

    m_camera->setZoomFactor(value);

    emit zoomFactorChanged();
}

float Camera::maximumZoomFactor() const
{
    return m_camera ? m_camera->maximumZoomFactor() : 1.0;
}

void Camera::start()
{
    auto index = cameraIndex();
    auto cameras = QMediaDevices::videoInputs();
    if (index >= cameras.length())
    {
        qDebug() << "Error: bad camera index";
        m_captureSession.setCamera(nullptr);
        m_camera.reset();
        return;
    }

    m_camera.reset(new QCamera(cameras[index]));
    m_camera->setFocusMode(QCamera::FocusModeAuto);
    m_camera->setWhiteBalanceMode(whiteBalanceMode());
    m_camera->setFlashMode(flashMode());
    m_camera->setExposureMode(QCamera::ExposureMode::ExposureAuto);
    m_camera->setAutoExposureTime();
    m_camera->setAutoIsoSensitivity();
    m_camera->setZoomFactor(1.0);
    m_camera->start();

    m_captureSession.setCamera(m_camera.get());

    if (!m_imageCapture)
    {
        m_imageCapture.reset(new QImageCapture());
        m_imageCapture->setQuality(QImageCapture::Quality::VeryHighQuality);

        m_captureSession.setImageCapture(m_imageCapture.get());
        connect(m_imageCapture.get(), &QImageCapture::imageCaptured, this, &Camera::imageCaptured);
        connect(m_imageCapture.get(), &QImageCapture::imageSaved, this, &Camera::imageSaved);
    }

    emit cameraChanged();
    emit activeChanged();
}

void Camera::stop()
{
    if (m_camera)
    {
        m_camera->stop();
    }
}

void Camera::switchCamera()
{
    auto count = QMediaDevices::videoInputs().length();
    if (count == 1)
    {
        return;
    }

    auto nextIndex = (cameraIndex() + 1) % count;
    App::instance()->settings()->set_cameraIndex(nextIndex);

    start();
}

bool Camera::readyForCapture() const
{
    if (m_imageCapture && m_imageCapture->isReadyForCapture())
    {
        qDebug() << "Error: Image capture reports not ready";
    }

    return m_camera && m_camera->isActive() && m_imageCapture; //  && m_imageCapture->isReadyForCapture();
}

void Camera::capture() const
{
    if (!readyForCapture())
    {
        emit App::instance()->showError(tr("Not ready"));
        return;
    }

    m_imageCapture->setFileFormat(QImageCapture::FileFormat::JPEG);

    auto mediaFilename = App::instance()->getMediaFilename("jpg");
    auto mediaFilePath = App::instance()->getMediaFilePath(mediaFilename);

    m_imageCapture->captureToFile(mediaFilePath);
}

QObject* Camera::videoOutput() const
{
    return m_captureSession.videoOutput();
}

void Camera::setVideoOutput(QObject* value)
{
    m_captureSession.setVideoOutput(value);
}

void Camera::componentComplete()
{
    QQuickItem::componentComplete();
}

void Camera::imageCaptured(int id, const QImage& image)
{
}

void Camera::imageSaved(int id, const QString& filePath)
{
    QTimer::singleShot(10, this, std::bind(&Camera::photoCaptured, this, QFileInfo(filePath).fileName()));
}

} // namespace CyberTracker
