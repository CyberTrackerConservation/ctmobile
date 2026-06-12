#pragma once

#include "pch.h"
#include "ReadBarcode.h"

namespace ZXingQt {

Q_NAMESPACE

enum class BarcodeFormat
{
    None            = 0,         ///< Used as a return value if no valid barcode has been detected
    Aztec           = (1 << 0),  ///< Aztec
    Codabar         = (1 << 1),  ///< Codabar
    Code39          = (1 << 2),  ///< Code39
    Code93          = (1 << 3),  ///< Code93
    Code128         = (1 << 4),  ///< Code128
    DataBar         = (1 << 5),  ///< GS1 DataBar, formerly known as RSS 14
    DataBarExpanded = (1 << 6),  ///< GS1 DataBar Expanded, formerly known as RSS EXPANDED
    DataMatrix      = (1 << 7),  ///< DataMatrix
    EAN8            = (1 << 8),  ///< EAN-8
    EAN13           = (1 << 9),  ///< EAN-13
    ITF             = (1 << 10), ///< ITF (Interleaved Two of Five)
    MaxiCode        = (1 << 11), ///< MaxiCode
    PDF417          = (1 << 12), ///< PDF417 or
    QRCode          = (1 << 13), ///< QR Code
    UPCA            = (1 << 14), ///< UPC-A
    UPCE            = (1 << 15), ///< UPC-E
    MicroQRCode     = (1 << 16), ///< Micro QR Code
    RMQRCode        = (1 << 17), ///< Rectangular Micro QR Code
    DXFilmEdge      = (1 << 18), ///< DX Film Edge Barcode
    DataBarLimited  = (1 << 19), ///< GS1 DataBar Limited

    LinearCodes = Codabar | Code39 | Code93 | Code128 | EAN8 | EAN13 | ITF | DataBar | DataBarExpanded | DataBarLimited | DXFilmEdge | UPCA | UPCE,
    MatrixCodes = Aztec | DataMatrix | MaxiCode | PDF417 | QRCode | MicroQRCode | RMQRCode,
};

enum class ContentType { Text, Binary, Mixed, GS1, ISO15434, UnknownECI };

using ZXing::ReaderOptions;
using ZXing::Binarizer;
using ZXing::BarcodeFormats;

Q_ENUM_NS(BarcodeFormat)
Q_ENUM_NS(ContentType)

class Position : public ZXing::Quadrilateral<QPoint>
{
    Q_GADGET

    Q_PROPERTY(QPoint topLeft READ topLeft)
    Q_PROPERTY(QPoint topRight READ topRight)
    Q_PROPERTY(QPoint bottomRight READ bottomRight)
    Q_PROPERTY(QPoint bottomLeft READ bottomLeft)

    using Base = ZXing::Quadrilateral<QPoint>;

public:
    using Base::Base;
};

class Barcode : private ZXing::Barcode
{
    Q_GADGET

    Q_PROPERTY(BarcodeFormat format READ format)
    Q_PROPERTY(QString formatName READ formatName)
    Q_PROPERTY(QString text READ text)
    Q_PROPERTY(QByteArray bytes READ bytes)
    Q_PROPERTY(bool isValid READ isValid)
    Q_PROPERTY(ContentType contentType READ contentType)
    Q_PROPERTY(QString contentTypeName READ contentTypeName)
    Q_PROPERTY(Position position READ position)

    QString _text;
    QByteArray _bytes;
    Position _position;

public:
    Barcode() = default;
    explicit Barcode(ZXing::Barcode&& r);

    BarcodeFormat format() const;
    ContentType contentType() const;
    QString formatName() const;
    QString contentTypeName() const;
    const QString& text() const;
    const QByteArray& bytes() const;
    const Position& position() const;
};

class BarcodeReader : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    QML_WRITABLE_AUTO_PROPERTY (int, formats)

    Q_PROPERTY(QVideoSink* videoSink MEMBER m_sink WRITE setVideoSink)
    Q_PROPERTY(QRectF factorsRect MEMBER m_factorsRect WRITE setFactorsRect NOTIFY factorsRectChanged)

public:
    BarcodeReader(QObject* parent = nullptr);
    ~BarcodeReader();

    void classBegin() override;
    void componentComplete() override;

    void setVideoSink(QVideoSink* sink);
    void setFactorsRect(const QRectF& factorsRect);

    Q_INVOKABLE void stop();

signals:
    void factorsRectChanged();
    void cameraChanged(QCamera* camera);
    void failedRead();
    void foundBarcode(ZXingQt::Barcode barcode);

private:
    QVideoSink* m_sink = nullptr;
    QRectF m_factorsRect;
    QRect m_captureRect;
    QThreadPool m_pool;
    QMediaCaptureSession m_capture;
    QPointer<QCamera> m_camera;
    ReaderOptions m_options;
    qint64 m_lastFoundTime = 0;

    void onVideoFrameChanged(const QVideoFrame& frame);
    ZXingQt::Barcode process(const QVideoFrame& image);
    QCameraFormat selectCameraFormat() const;
    QCamera* makeDefaultCamera();
    void setCamera(QCamera* camera);
};

void registerQmlAndMetaTypes();

} // namespace ZXingQt

Q_DECLARE_METATYPE(ZXingQt::Position)
Q_DECLARE_METATYPE(ZXingQt::Barcode)
