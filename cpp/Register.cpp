#include "Register.h"
#include "App.h"
#include "Settings.h"
#include "Element.h"
#include "Field.h"
#include "Project.h"
#include "Form.h"
#include "FieldBinding.h"
#include "Sighting.h"
#include "Database.h"
#include "MBTilesReader.h"
#include "WaveFileRecorder.h"
#include "Location.h"

// Models.
#include "FieldListModel.h"
#include "ElementListModel.h"
#include "ElementTreeModel.h"
#include "MapLayerListModel.h"
#include "ProjectListModel.h"
#include "RecordListModel.h"
#include "SightingListModel.h"

// Connectors.
#include "Native/NativeConnector.h"
#include "EarthRanger/EarthRangerConnector.h"
#include "SMART/SMARTConnector.h"
#include "Classic/ClassicConnector.h"
#include "Esri/EsriConnector.h"
#include "KoBo/KoBoConnector.h"
#include "ODK/ODKConnector.h"

// Providers.
#include "Native/NativeProvider.h"
#include "Classic/ClassicProvider.h"
#include "QtSessionItem.h"
#include "SMART/SMARTProvider.h"
#include "EarthRanger/EarthRangerProvider.h"
#include "Esri/EsriProvider.h"
#include "ODK/ODKProvider.h"

// We have to do this because otherwise Qt intellisense doesn't work.
#define ENGINE_URI "CyberTracker"
#define UNCREATABLE_MESSAGE "Uncreatable"

void registerEngine(QGuiApplication* guiApplication, QQmlApplicationEngine* qmlEngine, const QVariantMap& config)
{
    const int maj = 1;
    const int min = 0;

    qmlRegisterUncreatableType<App>(ENGINE_URI, maj, min, "App", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Settings>(ENGINE_URI, maj, min, "Settings", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Telemetry>(ENGINE_URI, maj, min, "Telemetry", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<TimeManager>(ENGINE_URI, maj, min, "TimeManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<TaskManager>(ENGINE_URI, maj, min, "TaskManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<ProjectManager>(ENGINE_URI, maj, min, "ProjectManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<ElementManager>(ENGINE_URI, maj, min, "ElementManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<LocationStreamer>(ENGINE_URI, maj, min, "LocationStreamer", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<FieldManager>(ENGINE_URI, maj, min, "FieldManager", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Database>(ENGINE_URI, maj, min, "Database", UNCREATABLE_MESSAGE);
    qmlRegisterUncreatableType<Sighting>(ENGINE_URI, maj, min, "Sighting", UNCREATABLE_MESSAGE);
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
    qmlRegisterType<CalculateField>(ENGINE_URI, maj, min, "CalculateField");

    qmlRegisterType<FieldBinding>(ENGINE_URI, maj, min, "FieldBinding");

    qmlRegisterType<Form>(ENGINE_URI, maj, min, "Form");

    qmlRegisterType<MBTilesReader>(ENGINE_URI, maj, min, "MBTilesReader");
    qmlRegisterType<WaveFileRecorder>(ENGINE_URI, maj, min, "WaveFileRecorder");

    // Models.
    qmlRegisterUncreatableType<VariantListModel>(ENGINE_URI, maj, min, "VariantListModel", UNCREATABLE_MESSAGE);
    qmlRegisterType<ElementListModel>(ENGINE_URI, maj, min, "ElementListModel");
    qmlRegisterType<ElementTreeModel>(ENGINE_URI, maj, min, "ElementTreeModel");
    qmlRegisterType<FieldListModel>(ENGINE_URI, maj, min, "FieldListModel");
    qmlRegisterType<FieldListProxyModel>(ENGINE_URI, maj, min, "FieldListProxyModel");
    qmlRegisterType<MapLayerListModel>(ENGINE_URI, maj, min, "MapLayerListModel");
    qmlRegisterType<ProjectListModel>(ENGINE_URI, maj, min, "ProjectListModel");
    qmlRegisterType<RecordListModel>(ENGINE_URI, maj, min, "RecordListModel");
    qmlRegisterType<SightingListModel>(ENGINE_URI, maj, min, "SightingListModel");

    // Utils.
    auto utils = new Utils::Api(guiApplication);
    utils->set_networkAccessManager(qmlEngine->networkAccessManager());
    qmlEngine->rootContext()->setContextProperty("Utils", utils);

    // App and MBTilesReader.
    auto app = new App(guiApplication, qmlEngine, config);
    qmlEngine->rootContext()->setContextProperty("App", app);
    qmlEngine->rootContext()->setContextProperty("MBTilesReader", app->mbTilesReader());

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

void registerConnectors(QQmlEngine* /*engine*/)
{
    auto app = App::instance();
    app->registerConnector(NATIVE_CONNECTOR, &createNativeConnector);
    app->registerConnector(EARTH_RANGER_CONNECTOR, &createEarthRangerConnector);
    app->registerConnector(SMART_CONNECTOR, &createSMARTConnector);
    app->registerConnector(CLASSIC_CONNECTOR, &createClassicConnector);
    app->registerConnector(ESRI_CONNECTOR, &createEsriConnector);
    app->registerConnector(KOBO_CONNECTOR, &createKoBoConnector);
    app->registerConnector(ODK_CONNECTOR, &createODKConnector);
}

//=================================================================================================
// Providers.

Provider* createNativeProvider(QObject *parent)
{
    return new NativeProvider(parent);
}

Provider* createClassicProvider(QObject *parent)
{
    return new ClassicProvider(parent);
}

Provider* createSMARTProvider(QObject *parent)
{
    return new SMARTProvider(parent);
}

Provider* createEarthRangerProvider(QObject *parent)
{
    return new EarthRangerProvider(parent);
}

Provider* createEsriProvider(QObject *parent)
{
    return new EsriProvider(parent);
}

Provider* createODKProvider(QObject *parent)
{
    return new ODKProvider(parent);
}

void registerProviders(QQmlEngine* /*engine*/)
{
    qmlRegisterType<CtClassicSessionItem>("CyberTracker.Classic", 1, 0, "ClassicSession");

    auto app = App::instance();
    app->registerProvider(NATIVE_PROVIDER, &createNativeProvider);
    app->registerProvider(CLASSIC_PROVIDER, &createClassicProvider);
    app->registerProvider(SMART_PROVIDER, &createSMARTProvider);
    app->registerProvider(EARTH_RANGER_PROVIDER, &createEarthRangerProvider);
    app->registerProvider(ESRI_PROVIDER, &createEsriProvider);
    app->registerProvider(ODK_PROVIDER, &createODKProvider);
}
