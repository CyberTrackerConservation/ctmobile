#pragma once
#include "pch.h"
#include "ProviderInterface.h"
#include "Element.h"
#include "Form.h"

namespace CyberTracker {

class SMARTProvider : public Provider
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (int, smartVersion)

    QML_READONLY_AUTO_PROPERTY (bool, hasPatrol)
    QML_READONLY_AUTO_PROPERTY (bool, patrolForm)
    QML_READONLY_AUTO_PROPERTY (QString, patrolUid)
    QML_READONLY_AUTO_PROPERTY (bool, patrolStarted)
    QML_READONLY_AUTO_PROPERTY (bool, patrolPaused)
    QML_READONLY_AUTO_PROPERTY (double, patrolPausedDistance)
    QML_READONLY_AUTO_PROPERTY (QVariantList, patrolLegs)

    QML_READONLY_AUTO_PROPERTY (bool, hasIncident)
    QML_READONLY_AUTO_PROPERTY (bool, incidentForm)
    QML_READONLY_AUTO_PROPERTY (bool, incidentStarted)

    QML_READONLY_AUTO_PROPERTY (bool, hasCollect)
    QML_READONLY_AUTO_PROPERTY (bool, collectForm)
    QML_READONLY_AUTO_PROPERTY (bool, collectStarted)
    QML_READONLY_AUTO_PROPERTY (QString, collectUser)

    QML_READONLY_AUTO_PROPERTY (bool, hasDataServer)
    QML_READONLY_AUTO_PROPERTY (int, uploadFrequencyMinutes)
    QML_READONLY_AUTO_PROPERTY (bool, manualUpload)
    QML_READONLY_AUTO_PROPERTY (QString, smartDataPath)

    QML_READONLY_AUTO_PROPERTY (QString, lastExportError)

    QML_READONLY_AUTO_PROPERTY (int, uploadObsCounter)
    QML_READONLY_AUTO_PROPERTY (QString, uploadLastTime)

    QML_READONLY_AUTO_PROPERTY (bool, surveyMode)
    QML_READONLY_AUTO_PROPERTY (bool, samplingUnits)
    QML_READONLY_AUTO_PROPERTY (bool, instantGPS)
    QML_READONLY_AUTO_PROPERTY (bool, allowManualGPS)
    QML_READONLY_AUTO_PROPERTY (bool, useMetric)
    QML_READONLY_AUTO_PROPERTY (bool, useDistanceAndBearing)
    QML_READONLY_AUTO_PROPERTY (bool, useGroupUI)
    QML_READONLY_AUTO_PROPERTY (bool, useObserver)
    QML_READONLY_AUTO_PROPERTY (bool, useIncidentType)
    QML_READONLY_AUTO_PROPERTY (bool, allowHistory)

public:
    static constexpr char PROVIDER_NAME[] = "SMART";
    static constexpr char VALUE_NEW_PATROL[]    = "NewPatrol";
    static constexpr char VALUE_STOP_PATROL[]   = "StopPatrol";
    static constexpr char VALUE_PAUSE_PATROL[]  = "PausePatrol";
    static constexpr char VALUE_RESUME_PATROL[] = "ResumePatrol";
    static constexpr char VALUE_CHANGE_PATROL[] = "ChangePatrol";
    static constexpr char VALUE_OBSERVATION[]   = "Observation";

public:
    explicit SMARTProvider(Form* form);

    Q_INVOKABLE QVariantList buildConfigModel();

    Q_INVOKABLE void startPatrol();
    Q_INVOKABLE void stopPatrol();
    Q_INVOKABLE void pausePatrol();
    Q_INVOKABLE void resumePatrol();
    Q_INVOKABLE bool isPatrolMetadataValid();
    Q_INVOKABLE void changePatrol();
    Q_INVOKABLE void setPatrolLegValue(const QString& name, const QVariant& value);

    Q_INVOKABLE void startIncident();
    Q_INVOKABLE void stopIncident(bool cancel = false);

    Q_INVOKABLE void startCollect();
    Q_INVOKABLE void stopCollect();
    Q_INVOKABLE void setCollectUser(const QString& user);

    Q_INVOKABLE bool exportData();
    Q_INVOKABLE bool hasUploadData() const;
    Q_INVOKABLE bool uploadData();
    Q_INVOKABLE bool clearCompletedData();
    Q_INVOKABLE bool recoverAndClearData();

    Q_INVOKABLE QVariant getProfileValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

    Q_INVOKABLE QString getPatrolText() const;
    Q_INVOKABLE QString getSightingTypeText(const QString& observationType) const;
    Q_INVOKABLE QVariantMap getPatrolLeg(int index) const;
    Q_INVOKABLE void snapLegDistance();

    bool initialize();
    bool connectToProject() override;

    QString getStartPage() const override;

    bool requireGPSTime() const override;

    void finalizeSightingData(QVariantMap* sightingData) const override;

    bool finalizePackage(const QString& packageFilesPath) const override;

    void setUsage(const QString& name, const QVariant& value) const override;

private:
    QVariantMap m_profile;
    QVariantMap m_pingData;
    QString m_pingUuid;
    QString m_patrolStartTimestamp = 0;
    QString m_patrolTimeZone;

    void loadProjectState();
    void saveProjectState();

    bool shouldUploadData() const;
    bool hasUploadDataLegacy() const;
    bool uploadDataLegacy();

    void savePatrolSighting(const QString& observationType, const QString& timestamp, bool snapTrack);
    void savePatrolLocation(Location* location);

    QString convertToZone(const QString& isoTimestamp, const QString& timeZoneId) const;
    QJsonObject locationToJson(const QString& locationUid, Location* location, const QVariantMap& extraProps = QVariantMap(), const QString& timeZoneOverride = QString()) const;
    QJsonObject addRecordToJson(const QJsonObject& baseData, Sighting* sighting, const QString& recordUid, bool childRecords = false, int attributeIndex = 0, bool skipAttachments = false) const;
    QJsonArray sightingToJson(Sighting* sighting, int* obsCounter, const QString& filterRecordUid = QString(), const QVariantMap& extraProps = QVariantMap()) const;

    void startTrack(double initialTrackDistance, int initialTrackCounter);
    void stopTrack();

    bool uploadPing(Location* location);
    bool uploadPing6(Location* location);
    bool uploadPing7(Location* location);
    bool uploadAlerts(Form* form, const QString& sightingUid);
    bool uploadAlerts6(Form* form, const QString& sightingUid);
    bool uploadAlerts7(Form* form, const QString& sightingUid);
    bool uploadSighting(Form* form, const QString& sightingUid, int* obsCounter);
    void uploadPendingLocations(Form* form);
    bool uploadLocation(Form* form, Location* location, const QString& locationUid);
    void addToUploadQueue(Form* form, Sighting* sighting, const QString& type, const QString& summary, const QVariantMap& dataServer, const QByteArray& payload) const;

    struct ExportEnumState
    {
        int obsCounter = 1;
        int first = 0;
        int maxFileSizeBytes = -1;
    };
    bool exportSightings(Form* form, QFile& exportFile, const std::function<bool()>& startFile, const std::function<bool()>& completeFile, const std::function<void(const QString&, const QVariantMap&, Database::DataFlags, int)>& doneOne, ExportEnumState* enumState);
    bool exportDataInternal(int maxFileSizeBytes, const std::function<bool(const QString& filePath, ExportFileInfo info)>& finalizeFile);

    QString processFieldIdentifier(const QString& fieldIdentifier) const;

signals:
    void exportProgress(int index, int count);
};

} // namespace CyberTracker
