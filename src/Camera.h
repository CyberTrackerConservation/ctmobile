#pragma once
#include "pch.h"

namespace CyberTracker {

class Camera : public QQuickItem
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(int cameraIndex READ cameraIndex NOTIFY cameraChanged)
    Q_PROPERTY(QString cameraIcon READ cameraIcon NOTIFY cameraChanged)
    Q_PROPERTY(float maximumZoomFactor READ maximumZoomFactor NOTIFY cameraChanged)

    Q_PROPERTY(QString flashModeIcon READ flashModeIcon NOTIFY flashModeChanged)
    Q_PROPERTY(QString whiteBalanceIcon READ whiteBalanceIcon NOTIFY whiteBalanceChanged)

    Q_PROPERTY(float zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)

    Q_PROPERTY(QObject* videoOutput READ videoOutput WRITE setVideoOutput CONSTANT)

public:
    explicit Camera(QQuickItem* parent = nullptr);
    ~Camera() override;

    void componentComplete() override;

    Q_INVOKABLE void switchFlashMode();

    Q_INVOKABLE void switchWhiteBalanceMode();

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void switchCamera();

    Q_INVOKABLE bool readyForCapture() const;

    Q_INVOKABLE void capture() const;

signals:
    void activeChanged();
    void cameraChanged();
    void flashModeChanged();
    void whiteBalanceChanged();
    void zoomFactorChanged();
    void photoCaptured(const QString& mediaFilename);

private slots:
    void imageCaptured(int id, const QImage& image);
    void imageSaved(int id, const QString& filePath);

private:
    std::unique_ptr<QCamera> m_camera;
    std::unique_ptr<QImageCapture> m_imageCapture;
    QMediaCaptureSession m_captureSession;

    bool active() const;
    int cameraIndex() const;
    QString cameraIcon() const;

    QCamera::FlashMode flashMode() const;
    QString flashModeIcon() const;

    QCamera::WhiteBalanceMode whiteBalanceMode() const;
    QString whiteBalanceIcon() const;

    float zoomFactor() const;
    void setZoomFactor(float value);
    float maximumZoomFactor() const;

    QObject* videoOutput() const;
    void setVideoOutput(QObject* value);
};

} // namespace CyberTracker