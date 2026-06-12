#include "Register.h"
#include "Log.h"
#include "App.h"
#include "Settings.h"
#include "Vault.h"
#include "Element.h"
#include "Fields.h"
#include "Project.h"
#include "HasUpdate.h"
#include "UploadQueue.h"
#include "Form.h"
#include "FieldBinding.h"
#include "Sighting.h"
#include "Database.h"
#include "WaveFileRecorder.h"
#include "VoiceRecorder.h"
#include "Location.h"
#include "OfflineMap.h"
#include "Place.h"
#include "Schema.h"

// Models.
#include "FieldListModel.h"
#include "UploadQueueModel.h"
#include "ElementListModel.h"
#include "ElementTreeModel.h"
#include "MapLayerListModel.h"
#include "ProjectListModel.h"
#include "RecordListModel.h"
#include "SightingListModel.h"
#include "SightingModel.h"
#include "MapController.h"

// Controls.
#include "Camera.h"
#include "Skyplot.h"
#include "SketchPad.h"
#include "SquareImage.h"
#include "BarcodeReader.h"
#include "NativeImagePicker.h"
#include "ClassicItem.h"

// Connectors.
#include "Native/NativeConnector.h"
#include "EarthRanger/EarthRangerConnector.h"
#include "SMART/SMARTConnector.h"
#include "Classic/ClassicConnector.h"
#include "CTO/CTOConnector.h"
#include "Esri/EsriConnector.h"
#include "KoBo/KoBoConnector.h"
#include "ODK/ODKConnector.h"
#include "Google/GoogleConnector.h"

// Providers.
#include "Native/NativeProvider.h"
#include "Classic/ClassicProvider.h"
#include "Classic/ClassicView.h"
#include "SMART/SMARTProvider.h"
#include "EarthRanger/EarthRangerProvider.h"
#include "Esri/EsriProvider.h"
#include "ODK/ODKProvider.h"
#include "Google/GoogleProvider.h"

// Renderers.
#include "Renderer.h"
#include "Classic/ClassicRenderer.h"
#include "EarthRanger/EarthRangerRenderer.h"
#include "SMART/SMARTRenderer.h"

namespace CyberTracker {

// We have to do this because otherwise Qt intellisense doesn't work.
#define ENGINE_URI "CyberTracker"
#define UNCREATABLE_MESSAGE "Uncreatable"

void registerEngine(const QString& rootPath, QGuiApplication* guiApplication, QQmlApplicationEngine* qmlEngine, const QVariantMap& config)
{
    const int maj = 1;
    const int min = 0;

    qmlRegisterUncreatableType<App>(ENGINE_URI, maj, min, "Application", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<PlatformUtils>(ENGINE_URI, maj, min, "PlatformUtils", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Settings>(ENGINE_URI, maj, min, "Settings", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Vault>(ENGINE_URI, maj, min, "Vault", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Telemetry>(ENGINE_URI, maj, min, "Telemetry", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<TimeManager>(ENGINE_URI, maj, min, "TimeManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<ProjectManager>(ENGINE_URI, maj, min, "ProjectManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<UploadQueue>(ENGINE_URI, maj, min, "UploadQueue", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<HasUpdate>(ENGINE_URI, maj, min, "HasUpdate", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<OfflineMapManager>(ENGINE_URI, maj, min, "OfflineMapManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<LocationStreamer>(ENGINE_URI, maj, min, "LocationStreamer", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<LocationLogger>(ENGINE_URI, maj, min, "LocationLogger", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Database>(ENGINE_URI, maj, min, "Database", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<AppLink>(ENGINE_URI, maj, min, "AppLink", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Sighting>(ENGINE_URI, maj, min, "Sighting", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Location>(ENGINE_URI, maj, min, "Location", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<GotoManager>(ENGINE_URI, maj, min, "GotoManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<GotoManager>(ENGINE_URI, maj, min, "GotoManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Compass>(ENGINE_URI, maj, min, "Compass", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<SatelliteManager>(ENGINE_URI, maj, min, "SatelliteManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Place>(ENGINE_URI, maj, min, "Place", UNCREATABLE_MESSAGE);

    qmlRegisterUncreatableType<RecordManager>(ENGINE_URI, maj, min, "RecordManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Record>(ENGINE_URI, maj, min, "Record", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<FieldValue>(ENGINE_URI, maj, min, "FieldValue", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<BaseField>(ENGINE_URI, maj, min, "BaseField", UNCREATABLE_MESSAGE);

    qmlRegisterType<Project>(ENGINE_URI, maj, min, "Project");
    qmlRegisterType<Element>(ENGINE_URI, maj, min, "Element");

    qmlRegisterType<RecordField>(ENGINE_URI, maj, min, "RecordField");
    qmlRegisterType<StaticField>(ENGINE_URI, maj, min, "StaticField");
    qmlRegisterType<StringField>(ENGINE_URI, maj, min, "StringField");
    qmlRegisterType<NumberField>(ENGINE_URI, maj, min, "NumberField");
    qmlRegisterType<LocationField>(ENGINE_URI, maj, min, "LocationField");
    qmlRegisterType<LineField>(ENGINE_URI, maj, min, "LineField");
    qmlRegisterType<AreaField>(ENGINE_URI, maj, min, "AreaField");
    qmlRegisterType<CheckField>(ENGINE_URI, maj, min, "CheckField");
    qmlRegisterType<AcknowledgeField>(ENGINE_URI, maj, min, "AcknowledgeField");
    qmlRegisterType<DateField>(ENGINE_URI, maj, min, "DateField");
    qmlRegisterType<DateTimeField>(ENGINE_URI, maj, min, "DateTimeField");
    qmlRegisterType<TimeField>(ENGINE_URI, maj, min, "TimeField");
    qmlRegisterType<ListField>(ENGINE_URI, maj, min, "ListField");
    qmlRegisterType<PhotoField>(ENGINE_URI, maj, min, "PhotoField");
    qmlRegisterType<AudioField>(ENGINE_URI, maj, min, "AudioField");
    qmlRegisterType<SketchField>(ENGINE_URI, maj, min, "SketchField");
    qmlRegisterType<FileField>(ENGINE_URI, maj, min, "FileField");
    qmlRegisterType<CalculateField>(ENGINE_URI, maj, min, "CalculateField");

    qmlRegisterType<Schema>(ENGINE_URI, maj, min, "Schema");

    qmlRegisterType<FieldBinding>(ENGINE_URI, maj, min, "FieldBinding");

    qmlRegisterType<Form>(ENGINE_URI, maj, min, "Form");

    qmlRegisterType<WaveFileRecorder>(ENGINE_URI, maj, min, "WaveFileRecorder");
    qmlRegisterType<VoiceRecorder>(ENGINE_URI, maj, min, "VoiceRecorder");

    // Models.
    qmlRegisterUncreatableType<VariantListModel>(ENGINE_URI, maj, min, "VariantListModel", UNCREATABLE_MESSAGE);
    qmlRegisterType<UploadQueueModel>(ENGINE_URI, maj, min, "UploadQueueModel");
    qmlRegisterType<ElementListModel>(ENGINE_URI, maj, min, "ElementListModel");
    qmlRegisterType<ElementTreeModel>(ENGINE_URI, maj, min, "ElementTreeModel");
    qmlRegisterType<FieldListModel>(ENGINE_URI, maj, min, "FieldListModel");
    qmlRegisterType<FieldListProxyModel>(ENGINE_URI, maj, min, "FieldListProxyModel");
    qmlRegisterType<MapLayerListModel>(ENGINE_URI, maj, min, "MapLayerListModel");
    qmlRegisterType<ProjectListModel>(ENGINE_URI, maj, min, "ProjectListModel");
    qmlRegisterType<RecordListModel>(ENGINE_URI, maj, min, "RecordListModel");
    qmlRegisterType<SightingListModel>(ENGINE_URI, maj, min, "SightingListModel");
    qmlRegisterType<SightingFilter>(ENGINE_URI, maj, min, "SightingFilter");
    qmlRegisterType<SightingListProxyModel>(ENGINE_URI, maj, min, "SightingListProxyModel");
    qmlRegisterType<SightingModel>(ENGINE_URI, maj, min, "SightingModel");
    qmlRegisterType<MapController>(ENGINE_URI, maj, min, "MapController");

    // Misc.
    qmlRegisterUncreatableType<Audit>(ENGINE_URI, maj, min, "Audit", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<BackupFile>(ENGINE_URI, maj, min, "BackupFile", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<ExportFile>(ENGINE_URI, maj, min, "ExportFile", UNCREATABLE_MESSAGE);

    // Controls.
    qmlEngine->rootContext()->setContextProperty("FormViewUrl", "qrc:/imports/CyberTracker/FormView.qml");
    qmlRegisterType<Camera>(ENGINE_URI, maj, min, "CameraManager");
    qmlRegisterType<Skyplot>(ENGINE_URI, maj, min, "Skyplot");
    qmlRegisterType<SketchPad>(ENGINE_URI, maj, min, "SketchPad");
    qmlRegisterType<SquareImage>(ENGINE_URI, maj, min, "SquareImage");
    qmlRegisterType<NativeImagePicker>(ENGINE_URI, maj, min, "NativeImagePicker");

    // Classic.
    qmlRegisterUncreatableMetaObject(ClassicEnums::staticMetaObject, ENGINE_URI, maj, min, "ClassicEnums", UNCREATABLE_MESSAGE);
    qmlRegisterType<ClassicItem>(ENGINE_URI, maj, min, "ClassicItem");

    ZXingQt::registerQmlAndMetaTypes();

    // Utils.
    auto utils = new Utils::Api(guiApplication);
    utils->set_networkAccessManager(qmlEngine->networkAccessManager());
    qmlEngine->rootContext()->setContextProperty("Utils", utils);

    // App and MBTilesReader.
    auto app = new App(rootPath, config, guiApplication, qmlEngine);
    qmlEngine->rootContext()->setContextProperty("App", app);
    auto log = Log::instance();
    qmlEngine->rootContext()->setContextProperty("Log", log);

    // Create a never connected form so that intellisense works in QML.
    qmlEngine->rootContext()->setContextProperty("form", new Form(app));
}

//=================================================================================================
// Connectors.

Connector* createNativeConnector(QObject *parent)
{
    return new NativeConnector(parent);
}

Connector* createEarthRangerConnector(QObject *parent)
{
    return new EarthRangerConnector(parent);
}

Connector* createSMARTConnector(QObject *parent)
{
    return new SMARTConnector(parent);
}

Connector* createClassicConnector(QObject *parent)
{
    return new ClassicConnector(parent);
}

Connector* createCTOConnector(QObject *parent)
{
    return new CTOConnector(parent);
}

Connector* createEsriConnector(QObject *parent)
{
    return new EsriConnector(parent);
}

Connector* createKoBoConnector(QObject *parent)
{
    return new KoBoConnector(parent);
}

Connector* createODKConnector(QObject *parent)
{
    return new ODKConnector(parent);
}

Connector* createGoogleConnector(QObject *parent)
{
    return new GoogleConnector(parent);
}

void registerConnectors(QQmlEngine* /*engine*/)
{
    auto app = App::instance();
    app->registerConnector(NativeConnector::CONNECTOR_NAME, &createNativeConnector);
    app->registerConnector(EarthRangerConnector::CONNECTOR_NAME, &createEarthRangerConnector);
    app->registerConnector(SMARTConnector::CONNECTOR_NAME, &createSMARTConnector);
    app->registerConnector(ClassicConnector::CONNECTOR_NAME, &createClassicConnector);
    app->registerConnector(CTOConnector::CONNECTOR_NAME, &createCTOConnector);
    app->registerConnector(EsriConnector::CONNECTOR_NAME, &createEsriConnector);
    app->registerConnector(KoBoConnector::CONNECTOR_NAME, &createKoBoConnector);
    app->registerConnector(ODKConnector::CONNECTOR_NAME, &createODKConnector);
    app->registerConnector(GoogleConnector::CONNECTOR_NAME, &createGoogleConnector);
}

//=================================================================================================
// Providers.

Provider* createNativeProvider(Form* form)
{
    return new NativeProvider(form);
}

Provider* createClassicProvider(Form* form)
{
    return new ClassicProvider(form);
}

Provider* createSMARTProvider(Form* form)
{
    return new SMARTProvider(form);
}

Provider* createEarthRangerProvider(Form* form)
{
    return new EarthRangerProvider(form);
}

Provider* createEsriProvider(Form* form)
{
    return new EsriProvider(form);
}

Provider* createODKProvider(Form* form)
{
    return new ODKProvider(form);
}

Provider* createGoogleProvider(Form* form)
{
    return new GoogleProvider(form);
}

void registerProviders(QQmlEngine* /*engine*/)
{
    qmlRegisterType<ClassicView>("CyberTracker.Classic", 1, 0, "ClassicView");

    auto app = App::instance();
    app->registerProvider(NativeProvider::PROVIDER_NAME, &createNativeProvider);
    app->registerProvider(ClassicProvider::PROVIDER_NAME, &createClassicProvider);
    app->registerProvider(SMARTProvider::PROVIDER_NAME, &createSMARTProvider);
    app->registerProvider(EarthRangerProvider::PROVIDER_NAME, &createEarthRangerProvider);
    app->registerProvider(EsriProvider::PROVIDER_NAME, &createEsriProvider);
    app->registerProvider(ODKProvider::PROVIDER_NAME, &createODKProvider);
    app->registerProvider(GoogleProvider::PROVIDER_NAME, &createGoogleProvider);
}

//=================================================================================================
// Renderers.

Renderer* createClassicRenderer(QObject *parent)
{
    return new ClassicRenderer(parent);
}

Renderer* createEarthRangerRenderer(QObject *parent)
{
    return new EarthRangerRenderer(parent);
}

Renderer* createSMARTRenderer(QObject *parent)
{
    return new SMARTRenderer(parent);
}

void registerRenderers(QQmlEngine* /*engine*/)
{
    auto app = App::instance();
    app->registerRenderer(ClassicRenderer::RENDERER_NAME, &createClassicRenderer);
    app->registerRenderer(EarthRangerRenderer::RENDERER_NAME, &createEarthRangerRenderer);
    app->registerRenderer(SMARTRenderer::RENDERER_NAME, &createSMARTRenderer);
}

} // namespace CyberTracker
