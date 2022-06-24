#include "ProviderInterface.h"
#include "App.h"
#include "Form.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExportFileInfo

void ExportFileInfo::reset()
{
    sightingCount = locationCount = 0;
    startTime.clear();
    stopTime.clear();
}

QVariantMap ExportFileInfo::toMap() const
{
    return QVariantMap
    {
        { "projectUid", projectUid },
        { "filter", filter },
        { "projectTitle", projectTitle },
        { "name", name },
        { "sightingCount", sightingCount },
        { "locationCount", locationCount },
        { "startTime", startTime },
        { "stopTime", stopTime },
    };
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Provider

Provider::Provider(QObject* parent): QObject(parent)
{
    auto form = qobject_cast<Form*>(parent);

    m_project = form->project();
    m_elementManager = form->elementManager();
    m_fieldManager = form->fieldManager();
    m_taskManager = App::instance()->taskManager();
    m_database = App::instance()->database();
}

Provider::~Provider()
{
}

void Provider::save(QVariantMap& /*data*/)
{
}

void Provider::load(const QVariantMap& /*data*/)
{
}

bool Provider::connectToProject(bool /*newBuild*/)
{
    return true;
}

void Provider::disconnectFromProject()
{

}

bool Provider::requireUsername() const
{
    return false;
}

QUrl Provider::getStartPage()
{
    return QUrl();
}

void Provider::getElements(ElementManager* /*elementManager*/)
{
}

void Provider::getFields(FieldManager* /*fieldManager*/)
{
}

QUrl Provider::getProjectFileUrl(const QString& filename) const
{
    return App::instance()->projectManager()->getFileUrl(m_project->uid(), filename);
}

QString Provider::getProjectFilePath(const QString& filename) const
{
    return App::instance()->projectManager()->getFilePath(m_project->uid(), filename);
}

bool Provider::notify(const QVariantMap& /*message*/)
{
    return true;
}

bool Provider::finalizePackage(const QString& /*packageFilesPath*/) const
{
    return true;
}

QVariantList Provider::buildMapDataLayers()
{
    auto result = QVariantList();
    auto form = qobject_cast<Form*>(parent());

    // Sighting layer.
    auto getSightingSymbol = [](const Sighting* /*sighting*/) -> QVariantMap
    {
        auto symbol = QVariantMap();
        symbol["symbolType"] = "PictureMarkerSymbol";
        symbol["angle"] = 0.0;
        symbol["type"] = "esriPMS";
        symbol["url"] = "qrc:/icons/map_marker.png";
        symbol["width"] = 24;
        symbol["height"] = 24;
        symbol["yoffset"] = 12.0;

        return symbol;
    };
    result.append(form->buildSightingMapLayer("Sighting", getSightingSymbol));

    // Track layer.
    auto getTrackSymbol = []() -> QVariantMap
    {
        auto symbol = QVariantMap();
        symbol["symbolType"] = "SimpleLineSymbol";

        auto symbolColor = QVariantList();
        symbolColor.append(0);
        symbolColor.append(0);
        symbolColor.append(255);
        symbolColor.append(200);

        symbol["color"] = symbolColor;
        symbol["style"] = "esriSLSDot";
        symbol["type"] = "esriSLS";
        symbol["width"] = 1.75;

        return symbol;
    };
    result.append(form->buildTrackMapLayer("Tracks", getTrackSymbol()));

    return result;
}

bool Provider::canEditSighting(Sighting* /*sighting*/, int /*flags*/)
{
    return false;
}

void Provider::finalizeSighting(QVariantMap& /*sightingMap*/)
{
}

QVariantList Provider::buildSightingView(Sighting* sighting)
{
    auto recordModel = QVariantList();
    auto modelIndex = 1;

    // Add timestamp.
    recordModel.append(QVariantMap {
        { "delegate", "timestamp" },
        { "createTime", sighting->createTime() } });
    modelIndex++;

    // Add top-level location.
    auto locationFieldValue = sighting->location();
    if (!locationFieldValue.isEmpty())
    {
        recordModel.append(QVariantMap {
            { "delegate", "location" },
            { "value", locationFieldValue } });
        modelIndex++;
    }

    // Add a title.
    recordModel.append(QVariantMap {
        { "delegate", "group" },
        { "caption", tr("Data") }
    });
    modelIndex++;

    sighting->buildRecordModel(&recordModel, [&](const FieldValue& fieldValue, int /*depth*/) -> bool
    {
        // Skip location fields in the root, we already have this at the top.
        if (fieldValue.recordUid() == sighting->rootRecordUid() && qobject_cast<const LocationField*>(fieldValue.field()))
        {
            return false;
        }

        return true;
    });

    // Fancy logic to add headers and dividers to make complex records more readable.
    while (modelIndex <= recordModel.count() - 1)
    {
        auto lastIndex = modelIndex - 1;
        auto lastMap = recordModel.at(lastIndex).toMap();
        auto lastDepth = lastMap.value("depth", 0).toInt();
        auto lastRecordUid = lastMap.value("recordUid").toString();

        auto currMap = recordModel.at(modelIndex).toMap();
        auto currDepth = currMap.value("depth", 0).toInt();
        auto currRecordUid = currMap.value("recordUid").toString();

        auto addDivider = false;
        addDivider = addDivider || (currDepth > 0 && lastMap.value("depth") != currMap.value("depth"));
        addDivider = addDivider || (currDepth > 0 && lastMap.value("recordUid") != currMap.value("recordUid"));
        addDivider = addDivider || (lastDepth > currDepth && lastDepth > 0);
        if (addDivider)
        {
            lastMap["divider"] = true;
            recordModel.replace(lastIndex, lastMap);
        }

        auto addHeader = currDepth > 0 && (lastRecordUid != currRecordUid);
        if (addHeader)
        {
            // For groups, remove the record counter.
            if (lastMap.value("group").toBool())
            {
                recordModel.removeAt(lastIndex);
                modelIndex--;

                if (modelIndex > 0)
                {
                    auto lastMap = recordModel.at(modelIndex - 1).toMap();
                    lastMap["divider"] = true;
                    recordModel.replace(modelIndex - 1, lastMap);
                }
            }

            recordModel.insert(modelIndex, QVariantMap {
                { "delegate", "recordHeader" },
                { "text", sighting->getRecord(currRecordUid)->name() },
                { "divider", true } });

            modelIndex++;
        }

        modelIndex++;
    }

    return recordModel;
}

bool Provider::getUseGPSTime()
{
    return false;
}

QString Provider::getFieldName(const QString& /*fieldUid*/)
{
    return QString();
}

QUrl Provider::getFieldIcon(const QString& /*fieldUid*/)
{
    return QUrl();
}

bool Provider::getFieldTitleVisible(const QString& /*fieldUid*/)
{
    return true;
}
