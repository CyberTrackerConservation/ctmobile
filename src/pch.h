#pragma once

// Qt
#include <QObject>
#include <QFlags>
#include <QtGlobal>
#include <QtMath>
#include <QVariant>
#include <QVariantMap>
#include <QList>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QTimeZone>
#include <QTranslator>
#include <QDir>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QUuid>
#include <QImageReader>
#include <QTransform>
#include <QScopeGuard>
#include <QLockFile>
#include <QFuture>
#include <QFutureWatcher>
#include <QPermission>

#include <QValidator>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QRegularExpressionValidator>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>
#include <QTextStream>
#include <QTextDocument>
#include <QJSValue>

#include <QStandardPaths>
#include <QTemporaryDir>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <QScreen>
#include <QQmlEngine>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlListProperty>
#include <QQmlFileSelector>
#include <QSettings>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <QFontDatabase>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QSslSocket>

#include <QRunnable>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>
#include <QThreadPool>
#include <QThreadStorage>
#include <QTcpServer>

#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QNmeaPositionInfoSource>

#include <QGeoSatelliteInfo>
#include <QGeoSatelliteInfoSource>

#include <QMutex>
#include <QMutexLocker>
#include <QAccelerometer>
#include <QAccelerometerFilter>
#include <QMagnetometer>
#include <QMagnetometerFilter>
#include <QGyroscope>
#include <QGyroscopeFilter>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <QBitmap>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QQuickPaintedItem>
#include <QVariantAnimation>

#include <QRandomGenerator>

#include <QXmlStreamWriter>

#include <QHttpPart>
#include <QHttpMultiPart>

#include <QMimeDatabase>
#include <QMimeType>

#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioSource>
#include <QMediaPlayer>
#include <QMediaCaptureSession>
#include <QImageCapture>
#include <QVideoSink>
#include <QVideoFrame>
#include <QMediaDevices>
#include <QCamera>
#include <QCameraDevice>

#include <QBuffer>
#include <QtSvg/QSvgGenerator>
#include <QtSvg/QSvgRenderer>

#include <QtPlugin>
#include <QtLogging>

// STL
#include <memory>
#include <functional>
#include <cfloat>
#include <cmath>
#include <limits>
#include <cstdio>
#include <exception>
#include <unordered_map>

// QtSuperMacros
#include "PropertyHelpers.h"

// Engine
#include "VariantListModel.h"
#include "Utils.h"

#if defined(Q_OS_WINDOWS)
#include <Windows.h>
#endif

#if defined(Q_OS_ANDROID)
#include <QtCore/private/qandroidextras_p.h>
#include <QJniEnvironment>
#include <QJniObject>
#include <android/log.h>
#include <unistd.h>
#endif

#define DEG2RAD(d) (((d) * M_PI) / 180.0)
#define RAD2DEG(r) (((r) * 180.0) / M_PI)

// Desktop/Mobile select macros.
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
#define OS_DESKTOP false
#else
#define OS_DESKTOP true
#endif

#if OS_DESKTOP
#include <QClipboard>
#endif
