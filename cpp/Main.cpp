#include "pch.h"

#ifdef QT_WEBENGINE_LIB
#include <QtWebEngine>
#endif

#ifdef QT_WEBVIEW_LIB
#include <QtWebView>
#endif

#include <QZXing.h>
#include <ControlsPlugin.h>
#include <duperagent.h>
#include <promisemodule.h>
#include <imageutils.h>

#include "Register.h"
#include "App.h"

//=================================================================================================

#define kArgShowName        "show"
#define kArgShowValueName   "showOption"
#define kArgShowDescription "Show option maximized | minimized | fullscreen | normal | default"
#define kArgShowDefault     "show"

#define kShowMaximized      "maximized"
#define kShowMinimized      "minimized"
#define kShowFullScreen     "fullscreen"
#define kShowNormal         "normal"
#define STRINGIZE(x)        #x
#define QUOTE(x)            STRINGIZE(x)

//=================================================================================================

int main(int argc, char *argv[])
{
#if defined(SCREENSHOT_MODE)
    double scale = SCREENSHOT_SCALE;
    std::string scaleAsString = std::to_string(scale);
    QByteArray scaleAsQByteArray(scaleAsString.c_str(), scaleAsString.length());
    qputenv("QT_SCALE_FACTOR", scaleAsQByteArray);
#endif

    // Fix font corruption for bold fonts on OSX.
#if defined(Q_OS_MACOS)
    qputenv("QML_USE_GLYPHCACHE_WORKAROUND", "1");
#endif

    // Disable disk caching for hot-reloading.
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    qputenv("QML_DISABLE_DISK_CACHE", "");
#endif

    qSetMessagePattern("[%{time yyyyMMdd h:mm:ss.zzz} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{message}");
    qDebug() << "Initializing application";

    //dumpResources();

    // Load the config.
    auto configs = Utils::variantMapFromJsonFile(":/config.json");
    auto productSelect = QString(CT_PRODUCT_SELECT).toLower();
#if defined(Q_OS_ANDROID)
    productSelect = QtAndroid::androidActivity().callObjectMethod("getFlavor", "()Ljava/lang/String;").toString();
    if (productSelect.isEmpty())
    {
        productSelect = "ct";
    }
#endif
    auto config = configs[productSelect].toMap();

    auto applicationName = config["title"].toString();
    auto applicationVersion = QString(CT_VERSION);
    auto organizationName = config["organizationName"].toString();
    auto organizationDomain = config["organizationDomain"].toString();
    auto esriLicense = config["esriLicense"].toString();

    // Initialize Qt.
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // Initialize web engine.
    // Order of the selectors is important on macOS, which supports both QtWebEngine and QtWebView.
    // Currently, macOS does not support QtWebView well, regardless of whether the QtWebEngine or
    // native backend is chosen. The webengine selector comes first and so is prioritized over
    // webview.
    auto extraSelectors = QStringList();
#ifdef QT_WEBENGINE_LIB
    extraSelectors.append(QStringLiteral("webengine"));
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
     // QtWebEngine::initialize() is init before QML engine is created and
     // will prevent debugging under QT_FATAL_WARNINGS env variable
    QtWebEngine::initialize();
#endif
#ifdef QT_WEBVIEW_LIB
    extraSelectors.append(QStringLiteral("webview"));
    QtWebView::initialize();
#endif

    // Initialze application.
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Material");

    QCoreApplication::setApplicationName(applicationName);
    QCoreApplication::setApplicationVersion(applicationVersion);
    QCoreApplication::setOrganizationName(organizationName);
#ifdef Q_OS_MACOS
    QCoreApplication::setOrganizationDomain(organizationName);
#else
    QCoreApplication::setOrganizationDomain(organizationDomain);
#endif
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // Initialize license.
    QCoreApplication::instance()->setProperty("Esri.ArcGISRuntime.license", esriLicense);

    // Intialize application window.
    QQmlApplicationEngine appEngine;
    QQmlFileSelector::get(&appEngine)->setExtraSelectors(extraSelectors);
    appEngine.addImportPath(QDir(QCoreApplication::applicationDirPath()).filePath("qml"));

    QString arcGISRuntimeImportPath = QUOTE(ARCGIS_RUNTIME_IMPORT_PATH);
    QString arcGISToolkitImportPath = QUOTE(ARCGIS_TOOLKIT_IMPORT_PATH);

#if defined(LINUX_PLATFORM_REPLACEMENT)
    // on some linux platforms the string 'linux' is replaced with 1
    // fix the replacement paths which were created
    QString replaceString = QUOTE(LINUX_PLATFORM_REPLACEMENT);
    arcGISRuntimeImportPath = arcGISRuntimeImportPath.replace(replaceString, "linux", Qt::CaseSensitive);
    arcGISToolkitImportPath = arcGISToolkitImportPath.replace(replaceString, "linux", Qt::CaseSensitive);
#endif

    // Add the Runtime and Extras path.
    appEngine.addImportPath(arcGISRuntimeImportPath);

    // Add the Toolkit path.
    appEngine.addImportPath(arcGISToolkitImportPath);

    // Register QZXing.
    QZXing::registerQMLTypes();
    QZXing::registerQMLImageProvider(appEngine);

    // Register components.
    registerEngine(&app, &appEngine, config);
    registerControls(&appEngine);
    registerConnectors(&appEngine);
    registerProviders(&appEngine);

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

    // Register duperagent.
    appEngine.rootContext()->setContextProperty("HttpRequest", new com::cutehacks::duperagent::Request(&appEngine));
    appEngine.rootContext()->setContextProperty("Promise", new com::cutehacks::duperagent::PromiseModule(&appEngine));
    appEngine.rootContext()->setContextProperty("HttpImageUtils", new com::cutehacks::duperagent::ImageUtils(&appEngine));

    // Load the main QML file.
    QString mainQmlFile("qrc:/AppMain.qml");
    appEngine.load(mainQmlFile);

    // Initialize SSL.
    qDebug() << "SSL: supportsSsl() = " << QSslSocket::supportsSsl();
    qDebug() << "SSL: versionString = " << QSslSocket::sslLibraryBuildVersionString();

    auto topLevelObject = appEngine.rootObjects().value(0);
    qDebug() << Q_FUNC_INFO << topLevelObject;
    qDebug() << QSslSocket::supportsSsl()
             << QSslSocket::sslLibraryBuildVersionString()
             << QSslSocket::sslLibraryVersionString();
    auto window = qobject_cast<QQuickWindow *>(topLevelObject);
    if (!window)
    {
        qCritical("Error: Your root item has to be a Window.");
        return -1;
    }

#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
    // Process command line.
    QCommandLineOption showOption(kArgShowName, kArgShowDescription, kArgShowValueName, kArgShowDefault);

    QCommandLineParser commandLineParser;

    commandLineParser.setApplicationDescription(config["title"].toString());
    commandLineParser.addOption(showOption);
    commandLineParser.addHelpOption();
    commandLineParser.addVersionOption();
    commandLineParser.process(app);

    // Show app window.
    auto showValue = commandLineParser.value(kArgShowName).toLower();

    if (showValue.compare(kShowMaximized) == 0)
    {
        window->showMaximized();
    }
    else if (showValue.compare(kShowMinimized) == 0)
    {
        window->showMinimized();
    }
    else if (showValue.compare(kShowFullScreen) == 0)
    {
        window->showFullScreen();
    }
    else if (showValue.compare(kShowNormal) == 0)
    {
        window->showNormal();
    }
    else
    {
        window->show();
    }
#else
    window->show();
#endif

#if defined(Q_OS_ANDROID)
    QtAndroid::hideSplashScreen();
#endif

    auto exitCode = app.exec();

#if defined(Q_OS_WIN)
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(exitCode));
#endif

    return exitCode;
}
