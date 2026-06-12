#include "ProviderInterface.h"
#include "App.h"
#include "Form.h"

namespace CyberTracker {

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
        { "downloadUri", downloadUri },
    };
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Provider

Provider::Provider(Form* form): QObject(form)
{
    m_form = form;
    m_project = m_form->project();
    m_database = App::instance()->database();
    m_uploadQueue = App::instance()->uploadQueue();
    m_exportFile = App::instance()->exportFile();
    m_backupFile = App::instance()->backupFile();
}

Provider::~Provider()
{
}

QVariantMap Provider::saveState(QStringList* /*attachments*/)
{
    return {};
}

void Provider::loadState(const QVariantMap& /*data*/)
{
}

bool Provider::connectToProject()
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

bool Provider::requireGPSTime() const
{
    return false;
}

bool Provider::supportLocationTrackField() const
{
    return false;
}

bool Provider::supportLocationTrackService() const
{
    return false;
}

bool Provider::resetPageStackOnConnect() const
{
    return false;
}

QString Provider::getStartPage() const
{
    return QString();
}

QString Provider::getSchemaFilename() const
{
    return "Form" + m_form->stateSpace() + ".qml";
}

QString Provider::getProjectFileUrl(const QString& filename) const
{
    return App::instance()->projectManager()->getFileUrl(m_project->uid(), filename);
}

QString Provider::getProjectFilePath(const QString& filename) const
{
    return App::instance()->projectManager()->getFilePath(m_project->uid(), filename);
}

void Provider::auditAdd(const QString& description) const
{
    qobject_cast<const Form*>(parent())->auditAdd(description);
}

void Provider::setUsage(const QString& name, const QVariant& value) const
{
    Telemetry::setUsage(m_project->uid(), m_form->stateSpace(), name, value);
}

void Provider::registerExportFile(const QString& filePath, const QVariantMap& data) const
{
    m_exportFile->registerFile(filePath, data);
    auditAdd("Register export file");
}

void Provider::registerBackupFile(const QString& filePath) const
{
    m_backupFile->registerFile(m_project->title(), filePath);
}

void Provider::registerBackupData(const QByteArray& data, const QString& suffix) const
{
    m_backupFile->registerData(m_project->title(), data, m_project->connector() + "_", suffix);
}

bool Provider::notify(const QVariantMap& /*message*/)
{
    return true;
}

bool Provider::finalizePackage(const QString& /*packageFilesPath*/) const
{
    return true;
}

bool Provider::canSnapTrackToNewSighting() const
{
    return true;
}

bool Provider::canSubmitData() const
{
    return true;
}

void Provider::submitData()
{
    qDebug() << "Error: submitData not implemented";
}

void Provider::editSighting(const QString& stateSpace, const QString& sightingUid) const
{
    m_form->saveState();
    m_form->pushFormPage(QVariantMap {
        { "projectUid", m_form->project()->uid() },
        { "stateSpace", stateSpace },
        { "editSightingUid", sightingUid }
    });
}

void Provider::deleteSighting(const QString& /*sightingUid*/)
{
}

void Provider::finalizeSightingData(QVariantMap* /*sightingData*/) const
{
}

} // namespace CyberTracker
