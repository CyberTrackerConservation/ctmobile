#include "Audit.h"
#include "App.h"
#include "qtcsv/stringdata.h"
#include "qtcsv/writer.h"

namespace CyberTracker {

Audit::Audit(QObject* parent): QObject(parent)
{
    auto app = App::instance();

    connect(app->settings(), &Settings::webhookUrlChanged, this, [&]()
    {
        auto webhookUrl = App::instance()->settings()->webhookUrl();
        add(webhookUrl.isEmpty() ? "Webhook cleared" : "Webhook set: " + webhookUrl);
    });

    connect(app->projectManager(), &ProjectManager::projectUpdated, this, [&](const QString& projectUid, const QVariantMap& result)
    {
        if (!ApiResult::fromMap(result).success)
        {
            return;
        }

        auto project = App::instance()->projectManager()->loadPtr(projectUid);
        if (project)
        {
            add(project->title(), "Project updated");
        }
    });

    connect(app->projectManager(), &ProjectManager::projectInstalled, this, [&](const QString& projectUid, const QVariantMap& result)
    {
        if (!ApiResult::fromMap(result).success)
        {
            return;
        }

        auto project = App::instance()->projectManager()->loadPtr(projectUid);
        if (project)
        {
            add(project->title(), "Project installed");
        }
    });

    connect(app->projectManager(), &ProjectManager::projectDeleted, this, [&](const QString& projectTitle)
    {
        add(projectTitle, "Project deleted");
    });

    connect(app->offlineMapManager(), &OfflineMapManager::layerAdded, this, [&](const QString& layerName)
    {
        add("Add map layer: " + layerName);
    });

    connect(app->offlineMapManager(), &OfflineMapManager::layerDeleted, this, [&](const QString& layerName)
    {
        add("Delete map layer: " + layerName);
    });

    connect(app, &App::sightingSaved, this, [&](const QString& projectUid, const QString& sightingUid)
    {
        auto project = App::instance()->projectManager()->loadPtr(projectUid);
        if (project)
        {
            add(project->title(), "Sighting saved");
        }
    });

    connect(app, &App::sightingDeleted, this, [&](const QString& projectUid, const QString& sightingUid)
    {
        auto project = App::instance()->projectManager()->loadPtr(projectUid);
        if (project)
        {
            add(project->title(), "Sighting deleted");
        }
    });

    connect(app, &App::auditAdd, this, &Audit::internalAdd);
}

void Audit::add(const QString& description)
{
    App::instance()->audit()->internalAdd("", description);
}

void Audit::add(const QString& projectTitle, const QString& description)
{
    App::instance()->audit()->internalAdd(projectTitle, description);
}

void Audit::internalAdd(const QString& projectTitle, const QString& description) const
{
    auto auditItem = Database::DbAuditItem();
    auditItem.timestamp = App::instance()->timeManager()->currentDateTimeISO();
    auditItem.projectTitle = projectTitle;
    auditItem.description = description;
    App::instance()->database()->addAudit(auditItem);
}

void Audit::clear() const
{
    App::instance()->database()->clearAudit();
} 

QVariantList Audit::buildModel() const
{
    auto result = QVariantList();

    App::instance()->database()->enumAudit([&](const Database::DbAuditItem& auditItem)
    {
        if (auditItem.internal)
        {
            return;
        }
        result.append(QVariantMap
        {
            { "timestamp", auditItem.timestamp },
            { "projectTitle", auditItem.projectTitle },
            { "description", auditItem.description },
        });
    });

    return result;
}

QString Audit::createCSV() const
{
    QtCSV::StringData csvData;
    
    // Add data rows.
    App::instance()->database()->enumAudit([&](const Database::DbAuditItem& entry)
    {
        auto dataRow = QStringList { entry.timestamp, entry.projectTitle, entry.description };
        csvData.addRow(dataRow);
    });
    
    // Write CSV file.
    auto now = App::instance()->timeManager()->currentDateTime();
    auto csvFilePath = QString("%1/activity-log-%2.csv").arg(App::instance()->tempPath(), now.toString("yyyyMMdd-hhmmss"));
    auto headerRow = QStringList { "Timestamp", "Project", "Description" };
    auto success = QtCSV::Writer::write(csvFilePath, csvData, ",", "\"", QtCSV::Writer::WriteMode::REWRITE, headerRow);
    if (!success)
    {
        qDebug() << "Failed to write activity log CSV file";
        return QString();
    }
    
    return csvFilePath;
}

} // namespace CyberTracker
