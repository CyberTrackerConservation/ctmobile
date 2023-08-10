#pragma once
#include "pch.h"
#include "ProviderInterface.h"
#include "Element.h"
#include "Field.h"
#include "Form.h"

constexpr char SMART_PROVIDER[] = "SMART";

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
    QML_READONLY_AUTO_PROPERTY (QString, uploadSightingLastTime)

    QML_READONLY_AUTO_PROPERTY (bool, surveyMode)
    QML_READONLY_AUTO_PROPERTY (bool, samplingUnits)
    QML_READONLY_AUTO_PROPERTY (bool, instantGPS)
    QML_READONLY_AUTO_PROPERTY (bool, allowManualGPS)
    QML_READONLY_AUTO_PROPERTY (bool, useDistanceAndBearing)
    QML_READONLY_AUTO_PROPERTY (bool, useGroupUI)
    QML_READONLY_AUTO_PROPERTY (bool, useObserver)

public:
    SMARTProvider(QObject *parent = nullptr);

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

    Q_INVOKABLE QString getPatrolText();
    Q_INVOKABLE QString getSightingTypeText(const QString& observationType) const;
    Q_INVOKABLE QUrl getSightingTypeIcon(const QString& observationType) const;
    Q_INVOKABLE QVariantMap getPatrolLeg(int index);

    bool initialize();
    bool connectToProject(bool newBuild, bool* formChangedOut) override;

    QUrl getStartPage() override;
    void getElements(ElementManager* elementManager) override;
    void getFields(FieldManager* fieldManager) override;

    QVariantList buildMapDataLayers() const override;
    bool requireGPSTime() const override;

    bool supportSightingEdit() const override;
    bool supportSightingDelete() const override;

    bool canEditSighting(Sighting* sighting, int flags) const override;
    void finalizeSighting(QVariantMap& sightingMap) override;

    QVariantList buildSightingView(Sighting* sighting) const override;

    QString getFieldName(const QString& fieldUid) const override;
    bool getFieldTitleVisible(const QString& fieldUid) const override;
    QString getSightingSummaryText(Sighting* sighting) const override;
    QUrl getSightingSummaryIcon(Sighting* sighting) const override;
    QUrl getSightingStatusIcon(Sighting* sighting, int flags) const override;

    bool finalizePackage(const QString& packageFilesPath) const override;

private:
    QVariantMap m_profile;
    QVariantList m_extraData;
    QVariantMap m_pingData;
    QString m_pingUuid;
    QString m_patrolStartTimestamp = 0;

    void loadState();
    void saveState();

    bool shouldUploadData() const;

    void savePatrolSighting(const QString& observationType, const QString& timestamp);
    void savePatrolLocation(Location* location, const QString& timestamp);

    QJsonObject locationToJson(const QString& sightingUid, Location* location, const QVariantMap& extraProps = QVariantMap());
    QJsonObject addRecordToJson(const QJsonObject& baseData, Form* form, Sighting* sighting, const QString& recordUid, bool childRecords = false, int attributeIndex = 0, bool skipAttachments = false);
    QJsonArray sightingToJson(Form* form, const QVariantMap& data, int* obsCounter, const QString& filterRecordUid = QString(), const QVariantMap& extraProps = QVariantMap());

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

    struct ExportEnumState
    {
        int obsCounter = 1;
        int first = 0;
        int maxFileSizeBytes = -1;
    };
    bool exportSightings(Form* form, QFile& exportFile, const std::function<bool()>& startFile, const std::function<bool()>& completeFile, const std::function<void(const QString&, const QVariantMap&, uint)>& doneOne, ExportEnumState* enumState);
    bool exportDataInternal(int maxFileSizeBytes, const std::function<bool(const QString& filePath, const ExportFileInfo& info)>& finalizeFile);

    PhotoField* createPhotoField(const QString& fieldUidSuffix, bool required) const;
    AudioField* createAudioField(const QString& fieldUidSuffix) const;

    QVariantMap parseProject();
    void addMetadataItem(const QString& fieldName, const QVariantMap& fieldValues, ElementManager* elementManager, FieldManager* fieldManager, QVariantMap* settingsOut);
    void parseMetadata(const QString& fileName, ElementManager* elementManager, FieldManager* fieldManager, QVariantMap* settingsOut);
    void parseModel(const QString& fileName, ElementManager* elementManager, FieldManager* fieldManager, QVariantMap* settingsOut, QVariantList* extraDataOut);
    void postProcessModel(ElementManager* elementManager, FieldManager* fieldManager, RecordField* modelField, QVariantMap* settings);

    QVariantMap getSightingMapSymbol(Form* form, Sighting* sighting, const QColor& color) const;

signals:
    void exportProgress(int index, int count);
};

class SMARTModelHandler : public QXmlDefaultHandler
{
public:
    SMARTModelHandler(FieldManager* fieldManager, RecordField* modelField, ElementManager* elementManager, QVariantMap* settingsOut, QVariantList* extraDataOut);
    ~SMARTModelHandler() override;

    bool startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& attributes) override;
    bool endElement(const QString& namespaceURI, const QString& localName, const QString& qName) override;
    bool characters(const QString& str) override;
    bool fatalError(const QXmlParseException& exception) override;
    QString errorString() const override;

private:
    FieldManager* m_fieldManager = nullptr;
    RecordField* m_modelField = nullptr;
    ElementManager* m_elementManager = nullptr;
    QVariantMap* m_settings = nullptr;
    QVariantList* m_extraData = nullptr;

    QStringList m_stack;            // XML node stack
    QList<Element*> m_nodeStack;    // "node" stack
    QList<Element*> m_elementStack; // Current element stack
    QVariantMap m_attribute;        // Current attribute
    QStringList m_languages;        // Language list
    QString m_defaultLanguage;      // Default language
    bool m_instantGPS = false;      // Instant GPS
    QVariantMap m_lastAttribute;    // Last attribute for node
    QMultiMap<QString, BaseField*> m_remapDefaultValue; // Remap list+tree dmUuid->uuid for default values

    void elementFromXml(Element* element, const QXmlAttributes& attributes, const QString& exportUidPrefix = QString());
    bool matchParent(const QString& parentAttributeName);
    Element* getCurrentElement();
};
