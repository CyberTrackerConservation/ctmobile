#pragma once

// Qt
#include <QObject>
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
#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlListProperty>
#include <QQmlFileSelector>
#include <QQuickStyle>
#include <QSettings>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <QFontDatabase>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QSslSocket>

#include <QRunnable>
#include <QTimer>
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
#include <QAccelerometer>
#include <QAccelerometerFilter>
#include <QMagnetometer>
#include <QMagnetometerFilter>
#include <QGyroscope>
#include <QGyroscopeFilter>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <QPainter>
#include <QPainterPath>
#include <QQuickPaintedItem>
#include <QVariantAnimation>

#include <QRandomGenerator>

#include <QXmlDefaultHandler>
#include <QXmlStreamWriter>

#include <QHttpPart>
#include <QHttpMultiPart>

#include <QMimeDatabase>
#include <QMimeType>

#include <QAudioInput>
#include <QAudioRecorder>

#include <QBuffer>
#include <QtSvg/QSvgGenerator>
#include <QtSvg/QSvgRenderer>

#include <QtPlugin>

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
#include <QClipboard>
#endif

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

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#if defined(Q_OS_ANDROID)
#include <QtAndroid>
#include <jni.h>
#include <QtAndroid>
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#endif

#define DEG2RAD(d) (((d) * M_PI) / 180.0)
#define RAD2DEG(r) (((r) * 180.0) / M_PI)

// Desktop/Mobile select macros.
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
#define OS_DESKTOP true
#define OS_MOBILE false
#else
#define OS_DESKTOP false
#define OS_MOBILE true
#endif
