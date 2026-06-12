#include "ODKProvider.h"
#include "App.h"
#include "Form.h"

namespace CyberTracker {

ODKProvider::ODKProvider(Form* form) : Provider(form)
{
    update_name(PROVIDER_NAME);
}

bool ODKProvider::connectToProject()
{
    auto projectUid = m_project->uid();

    // Delete uploaded sightings after 2 seconds.
    connect(m_form, &Form::sightingFlagsChanged, this, [=](const QString& sightingUid)
    {
        if (!m_database->testData(projectUid, sightingUid))
        {
            return;
        }

        auto flags = m_database->getDataFlags(projectUid, sightingUid);
        if (!flags.testFlag(Database::DataFlag::DB_UPLOADED_FLAG))
        {
            return;
        }

        QTimer::singleShot(2000, m_form, &Form::deleteUploadedSightings);
    });

    // Cleanup uploaded data.
    connect(m_form, &Form::connectedChanged, this, [=]()
    {
        m_form->deleteUploadedSightings();
    });

    return true;
}

bool ODKProvider::requireUsername() const
{
    return m_form->schema()->getSetting("requireUsername").toBool() &&
           m_project->username().isEmpty() &&
           App::instance()->settings()->username().isEmpty();
}

bool ODKProvider::supportLocationTrackField() const
{
    return true;
}

QString ODKProvider::getStartPage() const
{
    if (m_form->editing())
    {
        return "qrc:/ODK/FormPage.qml";
    }

    auto parameters = m_form->schema()->rootField()->parameters();
    if (parameters.contains("startPage"))
    {
        auto startPagePath = getProjectFilePath(parameters["startPage"].toString());
        if (QFile::exists(startPagePath))
        {
            return QUrl::fromLocalFile(startPagePath).toString();
        }

        qDebug() << "Failed to load start page: " << startPagePath;
    }

    return "qrc:/ODK/StartPage.qml";
}

bool ODKProvider::canSubmitData() const
{
    return !m_project->connectorParams().value("submissionUrl").toString().isEmpty();
}

void ODKProvider::submitData()
{
    auto projectUid = m_project->uid();

    // Backup database.
    App::instance()->backupDatabase("ODK: submit data");

    // Submit data to the queue.
    m_database->transaction([&]() -> bool
    {
        m_database->enumData(projectUid, m_form->stateSpace(), Database::DataFlag::DB_SIGHTING_FLAG | Database::DataFlag::DB_COMPLETED_FLAG /* ON */, Database::DataFlag::DB_SUBMITTED_FLAG /* OFF */,
            [&](const QString& /*projectUid*/, const QString& sightingUid, const Database::DbData& dbSighting)
        {
            auto sighting = m_form->createSightingPtr(dbSighting.data);

            auto queueItem = Database::DbQueueItem();
            queueItem.projectUid = projectUid;
            queueItem.dataUid = sightingUid;
            queueItem.summary = m_form->renderer()->getSightingSummaryText(sighting.get());

            auto formId = m_project->getConnectorParam("exportId").toString();
            auto formVersion = m_project->getConnectorParam("version").toString();
            auto formhubUuid= m_project->getConnectorParam("formhubUuid").toString();
            queueItem.url = m_project->getConnectorParam("submissionUrl").toString();
            queueItem.payload = sighting->toXml(formId, formVersion, formhubUuid, &queueItem.attachments).toUtf8();
            queueItem.webhook = m_form->renderer()->getSightingWebhook(m_project, sighting.get());

            m_uploadQueue->addItem(queueItem);
            m_database->setDataFlags(projectUid, sightingUid, Database::DataFlag::DB_SUBMITTED_FLAG);

            registerBackupData(queueItem.payload, ".xml");

            emit App::instance()->sightingFlagsChanged(projectUid, sightingUid);
        });

        return true;
    });

    m_uploadQueue->kick();
}

bool ODKProvider::finalizePackage(const QString& packageFilesPath) const
{
    // Cleanup cached files.
    QFile::remove(packageFilesPath + "/Form.qml");

    return true;
}

} // namespace CyberTracker