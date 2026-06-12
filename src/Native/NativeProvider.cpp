#include "NativeProvider.h"
#include "App.h"
#include "Form.h"

namespace CyberTracker {

NativeProvider::NativeProvider(Form* form) : Provider(form)
{
    update_name(PROVIDER_NAME);
}

bool NativeProvider::connectToProject()
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

QString NativeProvider::getStartPage() const
{
    if (m_form->editing())
    {
        return "qrc:/ODK/FormPage.qml";
    }

    auto startPageFilename = m_project->startPage();
    if (startPageFilename.isEmpty())
    {
        auto parameters = m_form->schema()->rootField()->parameters();
        startPageFilename = parameters.value("startPage").toString();
    }

    auto startPageFilePath = getProjectFilePath(startPageFilename);
    if (QFile::exists(startPageFilePath))
    {
        return QUrl::fromLocalFile(startPageFilePath).toString();
    }

    return "qrc:/ODK/StartPage.qml";
}

bool NativeProvider::finalizePackage(const QString& packageFilesPath) const
{
    if (QFile::exists(packageFilesPath + "/form.xlsx"))
    {
        QFile::remove(packageFilesPath + "/Form.qml");
    }

    return true;
}

bool NativeProvider::supportLocationTrackField() const
{
    return !m_project->trackFieldUid().isEmpty();
}

bool NativeProvider::canSubmitData() const
{
    return !m_project->connectorParams().value("submissionUrl").toString().isEmpty();
}

void NativeProvider::submitData()
{
    auto submissionUrl = m_project->getConnectorParam("submissionUrl").toString();
    if (submissionUrl.isEmpty())
    {
        App::instance()->showError(tr("No submission target"));
        return;
    }

    auto projectUid = m_project->uid();

    // Backup database.
    App::instance()->backupDatabase("Native: submit data");

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
            queueItem.url = submissionUrl;
            queueItem.payload = m_form->renderer()->getSightingWebhook(m_project, sighting.get());
            queueItem.webhook = queueItem.payload;
            queueItem.attachments = sighting->recordManager()->attachments();

            m_uploadQueue->addItem(queueItem);
            m_database->setDataFlags(projectUid, sightingUid, Database::DataFlag::DB_SUBMITTED_FLAG);

            registerBackupData(queueItem.payload, ".json");

            emit App::instance()->sightingFlagsChanged(projectUid, sightingUid);
        });

        return true;
    });

    m_uploadQueue->kick();
}

} // namespace CyberTracker