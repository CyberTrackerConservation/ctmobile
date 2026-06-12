#include "pch.h"
#include "Log.h"
#include "Register.h"
#include "App.h"
#include "ArcGISRuntimeEnvironment.h"
#include "CoreTypes.h"
#include "LicenseResult.h"
#include "MapQuickView.h"
#include <QtWebView>

using namespace CyberTracker;
using namespace Esri::ArcGISRuntime;

#if defined(Q_OS_ANDROID)
#include "Location.h"

// On Android 13 with 6.8.2, shader cache corruption messes things up.
void hackClearShaderCache()
{
    auto cacheDir = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::CacheLocation);
    auto entries = QDir(cacheDir).entryInfoList();
    for (auto entryIt = entries.constBegin(); entryIt != entries.constEnd(); entryIt++)
    {
        if (entryIt->isDir() && entryIt->baseName().contains("qtshadercache"))
        {
            QDir(entryIt->filePath()).removeRecursively();
        }
    }
}
#endif

#if defined(Q_OS_IOS)
// On iOS we shipped a version of SMART Mobile which used the wrong root folder.
auto s_badRootPath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppDataLocation);
void hackRootFolder(const QString& rootPath)
{
    if (!QFile::exists(s_badRootPath + "/Projects.db"))
    {
        return;
    }

    auto hasProjects = false;
    Utils::enumFolders(rootPath + "/Projects", [&](const QFileInfo& fileInfo)
    {
        if (QFile::exists(fileInfo.filePath() + "/Project.qml"))
        {
            hasProjects = true;
        }
    });

    if (hasProjects)
    {
        return;
    }

    auto moveFile = [&](const QString& filename)
    {
        auto sourceFilePath = s_badRootPath + "/" + filename;
        auto targetFilePath = rootPath + "/" + filename;
        if (QFile::exists(sourceFilePath) && QFile::exists(targetFilePath))
        {
            QFile::remove(targetFilePath);
        }

        auto result = QFile::rename(sourceFilePath, targetFilePath);
        qDebug() << "Moved file: " << filename << result;
    };

    auto moveDir = [&](const QString& dir)
    {
        auto sourceDir = s_badRootPath + "/" + dir;
        auto targetDir = rootPath + "/" + dir;
        if (QDir(sourceDir).exists() && QDir(targetDir).exists())
        {
            QDir(targetDir).removeRecursively();
        }

        auto result = QDir().rename(sourceDir, targetDir);
        qDebug() << "Moved dir: " << dir << result;
    };

    moveFile("Settings.json");
    moveFile("Settings.ini");
    moveFile("Projects.db");
    moveFile("Media.json");
    moveFile("TrackLog.bin");
    moveDir("Projects");
    moveDir("Backup");
    moveDir("BugReports");
    moveDir("ExportData");
    moveDir("Goto");
    moveDir("Media");
    moveDir("OfflineMaps");
    moveDir("Archive");
    moveDir("Tracks");
    moveDir("Work");
}
#endif

int main(int argc, char *argv[])
{
    // Android location update service.
#if defined(Q_OS_ANDROID)
    if (argc > 1 && strcmp(argv[1], "-locationService") == 0)
    {
        auto service = QAndroidService(argc, argv);

        auto rootPath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppDataLocation);
        Log::init(rootPath);

        auto productSelect = QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("getFlavor", "()Ljava/lang/String;").toString();
        auto configs = Utils::variantMapFromJsonFile(":/config/config.json");
        auto config = configs[productSelect.isEmpty() ? "ct" : productSelect].toMap();
        auto app = App(rootPath, config);

        auto lockFile = QLockFile(app.getLocationServiceLockFilePath());
        qFatalIf(!lockFile.lock(), "Failed to get lock");

        auto locationWriter = LocationWriter(app.trackLogFilePath());

        auto result = service.exec();

        return result;
    }
#endif

#if defined(Q_OS_ANDROID)
    hackClearShaderCache();
#endif

    // Normal execution.
#if defined(SCREENSHOT_MODE)
    double scale = SCREENSHOT_SCALE;
    std::string scaleAsString = std::to_string(scale);
    QByteArray scaleAsQByteArray(scaleAsString.c_str(), scaleAsString.length());
    qputenv("QT_SCALE_FACTOR", scaleAsQByteArray);
#endif

    // Disable disk caching for hot-reloading.
    // BUGBUG: Caching causing issues on some Android devices.
#if OS_DESKTOP
    qputenv("QML_DISABLE_DISK_CACHE", "1");
#endif

    // Load the config.
    auto configs = Utils::variantMapFromJsonFile(":/config/config.json");
    auto productSelect = QString(CT_PRODUCT_SELECT).toLower();
#if defined(Q_OS_ANDROID)
    productSelect = QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("getFlavor", "()Ljava/lang/String;").toString();
    if (productSelect.isEmpty())
    {
        productSelect = "ct";
    }
#endif
    auto config = configs[productSelect].toMap();

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents);
    QtWebView::initialize();

    // Initialize application.
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName(config["title"].toString());
    QCoreApplication::setApplicationVersion(QString(CT_VERSION));
    QCoreApplication::setOrganizationName(config["organizationName"].toString());
    QCoreApplication::setOrganizationDomain(config["organizationDomain"].toString());

#if OS_DESKTOP
    auto rootPath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DesktopLocation) + "/CTProjects";
#else
    auto rootPath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppDataLocation);
#endif

#if defined(Q_OS_IOS)  // Remove after SM and GG have been out for a while,
    hackRootFolder(rootPath);
#endif

    Log::init(rootPath);
    qDebug() << "Initializing application";

    //dumpResources();

    QSettings::setDefaultFormat(QSettings::IniFormat);

    // ESRI license.
    ArcGISRuntimeEnvironment::setApiKey(config["esriApiKey"].toString());
    auto license = ArcGISRuntimeEnvironment::setLicense(config["esriLicense"].toString());
    qDebug() << "ArcGIS license valid: " << (license.licenseStatus() == LicenseStatus::Valid ? "true" : "false");
    qmlRegisterType<MapQuickView>("CyberTracker", 1, 0, "MapView");

    // Application engine.
    QQmlApplicationEngine appEngine;
    appEngine.addImportPath(":/imports");

    // Register components.
    registerEngine(rootPath, &app, &appEngine, config);
    registerConnectors(&appEngine);
    registerProviders(&appEngine);
    registerRenderers(&appEngine);

    // Start the UI.
    App::instance()->startUI();

    // Create paths that are shared with the desktop (via USB) on Android.
#if defined(Q_OS_ANDROID)
    auto androidShared = Utils::androidGetExternalFilesDir();
    qFatalIf(!Utils::ensurePath(androidShared), "Failed to create Android shared root");
    qFatalIf(!Utils::ensurePath(androidShared + "/SMARTdata"), "Failed to create Android shared SMARTdata folder");

    if (config.value("androidBootstrapClassic", false).toBool())
    {
        App::instance()->projectManager()->bootstrap("Classic", QVariantMap {{"no-telemetry", true }});
    }
#endif

    // Load the main QML file.
    appEngine.load(QUrl("qrc:/AppMain.qml"));

    // Dump SSL versions.
    qDebug() << "SSL: supportsSsl() = " << QSslSocket::supportsSsl();
    qDebug() << "SSL: versionString = " << QSslSocket::sslLibraryBuildVersionString();

#if defined(Q_OS_ANDROID)
    QNativeInterface::QAndroidApplication::hideSplashScreen(250);
#endif

    return app.exec();
}
