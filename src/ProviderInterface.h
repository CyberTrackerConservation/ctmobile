#pragma once
#include "pch.h"
#include "Project.h"
#include "Sighting.h"
#include "Database.h"
#include "UploadQueue.h"
#include "ExportFile.h"
#include "BackupFile.h"

namespace CyberTracker {

class Form;

struct ExportFileInfo
{
    QString projectUid;
    QString filter;
    QString projectTitle;
    QString name;
    int sightingCount = 0;
    int locationCount = 0;
    QString startTime = QString();
    QString stopTime = QString();
    QString downloadUri = QString();

    void reset();
    QVariantMap toMap() const;
};

class Provider: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (QString, name)

    Q_PROPERTY (QString startPage READ getStartPage CONSTANT)

public:
    Provider(Form* form);
    virtual ~Provider();

    virtual QVariantMap saveState(QStringList* attachments);
    virtual void loadState(const QVariantMap& data);

    virtual bool connectToProject();
    virtual void disconnectFromProject();

    virtual void editSighting(const QString& stateSpace, const QString& sightingUid) const;

    virtual void deleteSighting(const QString& sightingUid);

    virtual void finalizeSightingData(QVariantMap* sightingData) const;

    virtual bool requireUsername() const;
    virtual bool requireGPSTime() const;

    virtual bool supportLocationTrackField() const;
    virtual bool supportLocationTrackService() const;

    virtual bool resetPageStackOnConnect() const;
    virtual QString getStartPage() const;
    virtual QString getSchemaFilename() const;

    virtual bool notify(const QVariantMap& message);

    virtual bool finalizePackage(const QString& packageFilesPath) const;

    virtual bool canSnapTrackToNewSighting() const;

    virtual bool canSubmitData() const;
    virtual void submitData();

    virtual void setUsage(const QString& name, const QVariant& value) const;

protected:
    Form* m_form = nullptr;
    Project* m_project = nullptr;
    Database* m_database = nullptr;
    UploadQueue* m_uploadQueue = nullptr;
    ExportFile* m_exportFile = nullptr;
    BackupFile* m_backupFile = nullptr;

    QString getProjectFileUrl(const QString& filename) const;
    QString getProjectFilePath(const QString& filename) const;
    void auditAdd(const QString& description) const;
    void registerExportFile(const QString& filePath, const QVariantMap& data) const;
    void registerBackupFile(const QString& filePath) const;
    void registerBackupData(const QByteArray& data, const QString& suffix) const;
};

} // namespace CyberTracker
