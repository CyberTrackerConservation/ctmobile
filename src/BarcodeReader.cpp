#include "BarcodeReader.h"

namespace ZXingQt {

namespace {

QList<Barcode>ZXBarcodesToQBarcodes(ZXing::Barcodes&& zxres)
{
    QList<Barcode> res;
    for (auto&& r : zxres)
    {
        res.emplace_back(std::move(r));
    }

    return res;
}

QList<Barcode> ReadBarcodes(const QImage& img, const ReaderOptions& opts, const QRect& captureRect)
{
    using namespace ZXing;

    auto ImgFmtFromQImg = [](const QImage& img) {
        switch (img.format()) {
        case QImage::Format_ARGB32:
        case QImage::Format_RGB32:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        return ImageFormat::BGRA;
#else
        return ImageFormat::ARGB;
#endif
        case QImage::Format_RGB888: return ImageFormat::RGB;
        case QImage::Format_RGBX8888:
        case QImage::Format_RGBA8888: return ImageFormat::RGBA;
        case QImage::Format_Grayscale8: return ImageFormat::Lum;
        default: return ImageFormat::None;
        }
    };

    auto exec = [&](const QImage& img) {
        auto imageView = ZXing::ImageView(img.bits(), img.width(), img.height(), ImgFmtFromQImg(img), static_cast<int>(img.bytesPerLine()))
            .cropped(captureRect.left(), captureRect.top(), captureRect.width(), captureRect.height());

        return ZXBarcodesToQBarcodes(ZXing::ReadBarcodes(imageView, opts));
    };

    return ImgFmtFromQImg(img) == ImageFormat::None ? exec(img.convertToFormat(QImage::Format_Grayscale8)) : exec(img);
}

QList<Barcode> ReadBarcodes(const QVideoFrame& frame, const ReaderOptions& opts, const QRect& captureRect)
{
    using namespace ZXing;

    ImageFormat fmt = ImageFormat::None;
    int pixStride = 0;
    int pixOffset = 0;

#define FORMAT(F5, F6) QVideoFrameFormat::Format_##F6
#define FIRST_PLANE 0

    switch (frame.pixelFormat()) {
    case FORMAT(ARGB32, ARGB8888):
    case FORMAT(ARGB32_Premultiplied, ARGB8888_Premultiplied):
    case FORMAT(RGB32, RGBX8888):
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        fmt = ImageFormat::BGRA;
#else
        fmt = ImageFormat::ARGB;
#endif
        break;

    case FORMAT(BGRA32, BGRA8888):
    case FORMAT(BGRA32_Premultiplied, BGRA8888_Premultiplied):
    case FORMAT(BGR32, BGRX8888):
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        fmt = ImageFormat::RGBA;
#else
        fmt = ImageFormat::ABGR;
#endif
        break;

    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016: fmt = ImageFormat::Lum, pixStride = 1; break;

    case FORMAT(AYUV444, AYUV):
    case FORMAT(AYUV444_Premultiplied, AYUV_Premultiplied):
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        fmt = ImageFormat::Lum, pixStride = 4, pixOffset = 3;
#else
        fmt = ImageFormat::Lum, pixStride = 4, pixOffset = 2;
#endif
        break;

    case FORMAT(YUV420P, YUV420P):
    case FORMAT(NV12, NV12):
    case FORMAT(NV21, NV21):
    case FORMAT(IMC1, IMC1):
    case FORMAT(IMC2, IMC2):
    case FORMAT(IMC3, IMC3):
    case FORMAT(IMC4, IMC4):
    case FORMAT(YV12, YV12): fmt = ImageFormat::Lum; break;
    case FORMAT(UYVY, UYVY): fmt = ImageFormat::Lum, pixStride = 2, pixOffset = 1; break;
    case FORMAT(YUYV, YUYV): fmt = ImageFormat::Lum, pixStride = 2; break;

    case FORMAT(Y8, Y8): fmt = ImageFormat::Lum; break;
    case FORMAT(Y16, Y16): fmt = ImageFormat::Lum, pixStride = 2, pixOffset = 1; break;

    case FORMAT(ABGR32, ABGR8888):
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        fmt = ImageFormat::RGBA;
#else
        fmt = ImageFormat::ABGR;
#endif
        break;

    case FORMAT(YUV422P, YUV422P): fmt = ImageFormat::Lum; break;
    default: break;
    }

    if (fmt != ImageFormat::None) {
        auto img = frame; // shallow copy just get access to non-const map() function
        if (!img.isValid() || !img.map(QVideoFrame::ReadOnly)){
            qWarning() << "invalid QVideoFrame: could not map memory";
            return {};
        }
        QScopeGuard unmap([&] { img.unmap(); });

        auto imageView = ZXing::ImageView(img.bits(FIRST_PLANE) + pixOffset, img.width(), img.height(), fmt, img.bytesPerLine(FIRST_PLANE), pixStride)
                             .cropped(captureRect.left(), captureRect.top(), captureRect.width(), captureRect.height());

        return ZXBarcodesToQBarcodes(ZXing::ReadBarcodes(imageView, opts));
    }
    else {
        auto qimg = frame.toImage();
        if (qimg.format() != QImage::Format_Invalid)
            return ReadBarcodes(qimg, opts, captureRect);
        qWarning() << "failed to convert QVideoFrame to QImage";
        return {};
    }
}

inline Barcode ReadBarcode(const QVideoFrame& frame, const ReaderOptions& opts, const QRect& captureRect)
{
    auto res = ReadBarcodes(frame, ReaderOptions(opts).setMaxNumberOfSymbols(1), captureRect);
    return !res.isEmpty() ? res.takeFirst() : Barcode();
}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Barcode

Barcode::Barcode(ZXing::Barcode&& r) : ZXing::Barcode(std::move(r))
{
    _text = QString::fromStdString(ZXing::Barcode::text());
    _bytes = QByteArray(reinterpret_cast<const char*>(ZXing::Barcode::bytes().data()), Size(ZXing::Barcode::bytes()));
    auto& pos = ZXing::Barcode::position();
    auto qp = [&pos](int i) { return QPoint(pos[i].x, pos[i].y); };
    _position = {qp(0), qp(1), qp(2), qp(3)};
}

BarcodeFormat Barcode::format() const
{
    return static_cast<BarcodeFormat>(ZXing::Barcode::format());
}

ContentType Barcode::contentType() const
{
    return static_cast<ContentType>(ZXing::Barcode::contentType());
}

QString Barcode::formatName() const
{
    return QString::fromStdString(ZXing::ToString(ZXing::Barcode::format()));
}

QString Barcode::contentTypeName() const
{
    return QString::fromStdString(ZXing::ToString(ZXing::Barcode::contentType()));
}

const QString& Barcode::text() const
{
    return _text;
}

const QByteArray& Barcode::bytes() const
{
    return _bytes;
}

const Position& Barcode::position() const
{
    return _position;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BarcodeReader

BarcodeReader::BarcodeReader(QObject* parent) : QObject(parent)
{
    m_pool.setMaxThreadCount(2);

    m_options.setTryHarder(true);
    m_options.setTryInvert(true);
    m_options.setTryRotate(true);
    m_options.setTryDownscale(false);
    m_options.setTextMode(ZXing::TextMode::HRI);
    m_options.setIsPure(false);

    m_factorsRect = QRectF(0.05, 0.20, 0.9, 0.6);
}

BarcodeReader::~BarcodeReader()
{
    m_pool.setMaxThreadCount(0);
    m_pool.clear();
    m_pool.waitForDone();
}

void BarcodeReader::classBegin()
{
}

void BarcodeReader::componentComplete()
{
    if (m_camera.isNull())
    {
        setCamera(makeDefaultCamera());
    }
}

void BarcodeReader::stop()
{
    m_camera->stop();
    m_pool.clear();
    m_pool.waitForDone();
}

Barcode BarcodeReader::process(const QVideoFrame& image)
{
    auto t = QElapsedTimer();
    t.start();

    m_options.setFormats(static_cast<ZXing::BarcodeFormat>(m_formats));

    if (m_captureRect.isEmpty())
    {
        auto w = image.width();
        auto h = image.height();
        m_captureRect = QRect(w * m_factorsRect.x(), h * m_factorsRect.y(), w * m_factorsRect.width(), h * m_factorsRect.height());
    }

    auto result = ReadBarcode(image, m_options, m_captureRect);

    if (!result.text().isEmpty())
    {
        emit foundBarcode(result);
    }
    else
    {
        emit failedRead();
    }

    return result;
}

void BarcodeReader::setVideoSink(QVideoSink* sink)
{
    if (m_sink == sink)
    {
        return;
    }

    if (m_sink)
    {
        disconnect(m_sink, nullptr, this, nullptr);
    }

    m_sink = sink;

    m_capture.setVideoSink(sink);

    connect(m_sink, &QVideoSink::videoFrameChanged, this, &BarcodeReader::onVideoFrameChanged, Qt::DirectConnection);
}

void BarcodeReader::setFactorsRect(const QRectF& factorsRect)
{
    if (factorsRect == m_factorsRect)
    {
        return;
    }

    m_factorsRect = factorsRect;

    emit factorsRectChanged();

}

void BarcodeReader::onVideoFrameChanged(const QVideoFrame& frame)
{
    if (m_pool.activeThreadCount() >= m_pool.maxThreadCount())
    {
        return; // we are busy => skip the frame
    }

    m_pool.start([this, frame]() { process(frame); });
}

QCameraFormat BarcodeReader::selectCameraFormat() const
{
    auto formats = QMediaDevices::defaultVideoInput().videoFormats();

    // Square formats.
    for (auto formatIt = formats.constBegin(); formatIt != formats.constEnd(); formatIt++)
    {
        if (formatIt->resolution() == QSize(1440, 1440))
        {
            return *formatIt;
        }

        if (formatIt->resolution() == QSize(1200, 1200))
        {
            return *formatIt;
        }

        if (formatIt->resolution() == QSize(1080, 1080))
        {
            return *formatIt;
        }
    }

    // Non square formats.
    for (auto formatIt = formats.constBegin(); formatIt != formats.constEnd(); formatIt++)
    {
        if (formatIt->resolution() == QSize(1440, 1080))
        {
            return *formatIt;
        }

        if (formatIt->resolution() == QSize(2560, 1440))
        {
            return *formatIt;
        }
    }

    return QCameraFormat();
}

QCamera* BarcodeReader::makeDefaultCamera()
{
    auto defaultCamera = QMediaDevices::defaultVideoInput();
    if (defaultCamera.isNull())
    {
        qDebug() << "Error: no default camera";
        return nullptr;
    }

    auto camera = new QCamera(defaultCamera, this);
    if (camera->error())
    {
        qDebug() << "Error: default camera error";
        return nullptr;
    }

    camera->setFocusMode(QCamera::FocusModeAutoNear);

    camera->setCameraFormat(selectCameraFormat());

    return camera;
}

void BarcodeReader::setCamera(QCamera* camera)
{
    if (m_camera == camera)
    {
        return;
    }

    // Disconnect old camera if not already null.
    if (m_camera)
    {
        m_camera->stop();
        disconnect(m_camera, nullptr, this, nullptr);

        m_capture.setCamera(nullptr);
        delete m_camera;
        m_camera = nullptr;
    }

    if (camera)
    {
        connect(camera, &QCamera::errorOccurred, this, [](auto err, const auto& string)
        {
            qDebug() << "Camera error:" << string;
        });

        m_capture.setCamera(camera);
        m_camera = camera;
        m_camera->start();
    }

    emit cameraChanged(m_camera);
}

void registerQmlAndMetaTypes()
{
    qRegisterMetaType<ZXingQt::BarcodeFormat>("BarcodeFormat");
    qRegisterMetaType<ZXingQt::ContentType>("ContentType");
    qRegisterMetaType<ZXingQt::Position>("Position");
    qRegisterMetaType<ZXingQt::Barcode>("Barcode");

    // For access to enums.
    qmlRegisterUncreatableMetaObject(ZXingQt::staticMetaObject, "ZXing", 1, 0, "ZXing", "Access to enums & flags only");

    qmlRegisterType<ZXingQt::BarcodeReader>("ZXing", 1, 0, "BarcodeReader");
}

}
