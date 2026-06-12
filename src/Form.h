#pragma once
#include "pch.h"
#include "Database.h"
#include "TimeManager.h"
#include "Project.h"
#include "Element.h"
#include "Fields.h"
#include "Schema.h"
#include "Location.h"
#include "ProviderInterface.h"
#include "Renderer.h"
#include "Sighting.h"
#include "Wizard.h"
#include "TrackFile.h"
#include "TranscribeQueue.h"

namespace CyberTracker {

class Form: public QQuickItem
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

    QML_READONLY_AUTO_PROPERTY (Project*, project)
    QML_READONLY_AUTO_PROPERTY (int, depth)
    QML_READONLY_AUTO_PROPERTY (bool, connected)
    QML_READONLY_AUTO_PROPERTY (bool, editing)
    QML_READONLY_AUTO_PROPERTY (QString, sessionId)

    QML_READONLY_AUTO_PROPERTY (Database*, database)
    QML_READONLY_AUTO_PROPERTY (TimeManager*, timeManager)
    QML_READONLY_AUTO_PROPERTY (ProjectManager*, projectManager)
    QML_READONLY_AUTO_PROPERTY (TranscribeQueue*, transcribeQueue)
    QML_READONLY_AUTO_PROPERTY (Schema*, schema)
    QML_READONLY_AUTO_PROPERTY (LocationStreamer*, trackStreamer)
    QML_READONLY_AUTO_PROPERTY (LocationStreamer*, pointStreamer)
    QML_READONLY_AUTO_PROPERTY (LocationStreamer*, fieldStreamer)
    QML_READONLY_AUTO_PROPERTY (Wizard*, wizard)

    QML_READONLY_AUTO_PROPERTY (Sighting*, sighting)

    QML_WRITABLE_AUTO_PROPERTY (QString, projectUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, editSightingUid)

    QML_WRITABLE_AUTO_PROPERTY (QString, stateSpace)
    QML_WRITABLE_AUTO_PROPERTY (bool, readonly)
    QML_READONLY_AUTO_PROPERTY (QString, languageCode)

    QML_WRITABLE_AUTO_PROPERTY (QStringList, attachmentTags)

    QML_READONLY_AUTO_PROPERTY (bool, requireUsername)
    QML_READONLY_AUTO_PROPERTY (bool, requireGPSTime)

    QML_WRITABLE_AUTO_PROPERTY (bool, tracePageChanges)
    QML_WRITABLE_AUTO_PROPERTY (QVariantList, mapLayers)

    QML_READONLY_AUTO_PROPERTY (QString, startPage)

    Q_PROPERTY (QString rootRecordUid READ rootRecordUid NOTIFY sightingModified)

    Q_PROPERTY (bool allowUserTrackChange READ allowUserTrackChange NOTIFY trackSettingsChanged)

    // Expose the provider as a variant, so QML can use it dynamically.
    Q_PROPERTY (QVariant provider READ providerAsVariant CONSTANT)

public:
    explicit Form(QObject *parent = nullptr);
    ~Form();

    void componentComplete() override;

    static Form* parentForm(const QObject* obj);

    std::unique_ptr<Sighting> createSightingPtr(const QVariantMap& data) const;
    std::unique_ptr<Sighting> createSightingPtr(const QString& sightingUid = QString()) const;

    Q_INVOKABLE bool notifyProvider(const QVariantMap& message);

    // Connect and disconnect.
    Q_INVOKABLE void connectToProject(const QString& projectUid, const QString& editSightingUid = QString());
    Q_INVOKABLE void disconnectFromProject();

    // Get page url.
    Q_INVOKABLE QString getPage(const QString& pageName) const;

    // Elements.
    Q_INVOKABLE Element* getElement(const QString& elementUid) const;
    Q_INVOKABLE QString getElementName(const QString& elementUid) const;
    Q_INVOKABLE QString getElementIcon(const QString& elementUid, bool walkBack = false) const;
    Q_INVOKABLE QColor getElementColor(const QString& elementUid) const;
    Q_INVOKABLE QVariant getElementTag(const QString& elementUid, const QString& key) const;
    Q_INVOKABLE QStringList getElementFieldList(const QString& elementUid) const;
    Q_INVOKABLE QVariantMap getElementTreeInfo(const QString& elementUid) const;

    // Sighting save, load, edit, next, prev.
    void saveSighting(Sighting* sighting, bool updateTime = true);
    Q_INVOKABLE void saveSighting();
    Q_INVOKABLE void loadSighting(const QString& sightingUid);
    Q_INVOKABLE void editSighting(const QString& stateSpace, const QString& sightingUid) const;
    Q_INVOKABLE void newSighting(bool copyFromCurrent = false);
    Q_INVOKABLE void deleteSighting(const QString& sightingUid);
    Q_INVOKABLE void prevSighting();
    Q_INVOKABLE void nextSighting();

    Q_INVOKABLE void newSessionId();

    Q_INVOKABLE void snapCreateTime();
    Q_INVOKABLE void snapUpdateTime();

    Q_INVOKABLE bool hasData() const;
    Q_INVOKABLE bool isSightingValid(const QString& sightingUid) const; // no callers
    Q_INVOKABLE bool isSightingExported(const QString& sightingUid) const; // no callers
    Q_INVOKABLE bool isSightingUploaded(const QString& sightingUid) const; // no callers
    Q_INVOKABLE bool isSightingCompleted(const QString& sightingUid) const; // no callers
    Q_INVOKABLE bool isSightingTranscribing(const QString& sightingUid) const;

    Q_INVOKABLE void setSightingFlag(const QString& sightingUid, Database::DataFlags flags, bool on = true);
    Q_INVOKABLE void markSightingCompleted();
    Q_INVOKABLE void deleteExportedSightings() const;
    Q_INVOKABLE void deleteUploadedSightings() const;

    // Project-scope variables.
    Q_INVOKABLE QVariant getSetting(const QString& name, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setSetting(const QString& name, const QVariant& value = QVariant());

    // Per-project cumulative telemetry usage. No-op until connected.
    Q_INVOKABLE void setUsage(const QString& name, const QVariant& value) const;

    // Form-scope variables.
    Q_INVOKABLE QVariant getGlobal(const QString&, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setGlobal(const QString&, QVariant value = QVariant());

    // Sighting-scope variables.
    Q_INVOKABLE QVariant getSightingVariable(const QString&, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setSightingVariable(const QString&, QVariant value = QVariant());
    Q_INVOKABLE QVariant getControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& value);
    Q_INVOKABLE void resetControlState(const QString& recordUid, const QString& fieldUid, const QString& name);

    // Fields.
    Q_INVOKABLE BaseField* getField(const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldType(const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldName(const QString& recordUid, const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldIcon(const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldHint(const QString& recordUid, const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldHintIcon(const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldHintLink(const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldConstraintMessage(const QString& recordUid, const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldRequiredMessage(const QString& recordUid, const QString& fieldUid) const;

    // Records.
    Q_INVOKABLE QString getRecordName(const QString& recordUid) const;
    Q_INVOKABLE QString getRecordIcon(const QString& recordUid) const;
    Q_INVOKABLE QVariantMap getRecordData(const QString& recordUid) const;
    Q_INVOKABLE QString getRecordFieldUid(const QString& recordUid) const;
    Q_INVOKABLE QString getParentRecordUid(const QString& recordUid) const;
    Q_INVOKABLE QString getRecordSummary(const QString& recordUid) const;
    Q_INVOKABLE QString newRecord(const QString& parentRecordUid, const QString& recordFieldUid);
    Q_INVOKABLE void deleteRecord(const QString& recordUid);
    Q_INVOKABLE bool hasRecord(const QString& recordUid) const;
    Q_INVOKABLE bool hasRecordChanged(const QString& recordUid) const;

    // Field values.
    Q_INVOKABLE QVariant getFieldParameter(const QString& recordUid, const QString& fieldUid, const QString& key, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE QVariant getFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE QVariant getFieldValue(const QString& fieldUid, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& value);
    Q_INVOKABLE void setFieldValue(const QString& fieldUid, const QVariant& value);
    Q_INVOKABLE void setFieldValues(const QString& recordUid, const QVariantMap& values);
    Q_INVOKABLE void setFieldValues(const QVariantMap& values);
    Q_INVOKABLE void resetFieldValue(const QString& recordUid, const QString& fieldUid);
    Q_INVOKABLE void resetFieldValue(const QString& fieldUid);
    Q_INVOKABLE void setFieldState(const QString& recordUid, const QString& fieldUid, int state);
    Q_INVOKABLE void setFieldState(const QString& fieldUid, int state);
    Q_INVOKABLE void resetRecord(const QString& recordUid);

    // Field streamer.
    Q_INVOKABLE void startFieldStreamer(const QString& recordUid, const QString& fieldUid, int updateInterval, double distanceFilter = 0);
    Q_INVOKABLE void stopFieldStreamer();

    // Display text.
    Q_INVOKABLE QString getFieldDisplayValue(const QString& recordUid, const QString& fieldUid, const QString& defaultValue = QString()) const;

    // Computed field properties.
    Q_INVOKABLE bool getFieldRequired(const QString& recordUid, const QString& fieldUid) const;
    Q_INVOKABLE bool getFieldVisible(const QString& recordUid, const QString& fieldUid) const;
    Q_INVOKABLE bool getFieldTitleVisible(const QString& fieldUid) const;
    Q_INVOKABLE bool getFieldValueValid(const QString& recordUid, const QString& fieldUid) const;
    Q_INVOKABLE bool getFieldValueCalculated(const QString& recordUid, const QString& fieldUid) const;
    Q_INVOKABLE bool getRecordValid(const QString& recordUid) const;

    // Pages.
    Q_INVOKABLE void pushPage(const QString& pageUrl, const QVariantMap& params = QVariantMap(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void pushFormPage(const QVariantMap& params = QVariantMap(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void pushWizardPage(const QString& recordUid, const QVariantMap& rules = QVariantMap(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void pushWizardIndexPage(const QString& recordUid, const QVariantMap& rules = QVariantMap(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void popPage(int transition = /*StackView.PopTransition*/ 3);
    Q_INVOKABLE void popPageBack(int transition = /*StackView.PopTransition*/ 3);
    Q_INVOKABLE void popPages(int count);
    Q_INVOKABLE void popPagesToStart();
    Q_INVOKABLE void popPagesToParent();
    Q_INVOKABLE void replaceLastPage(const QString& pageUrl, const QVariantMap& params = QVariantMap(), int transition = 0);
    Q_INVOKABLE void loadPages();
    Q_INVOKABLE void dumpPages(const QString& description);
    Q_INVOKABLE QVariantList getPageStack() const;
    Q_INVOKABLE void setPageStack(const QVariantList& value);
    Q_INVOKABLE QString firstPageUrl() const;
    Q_INVOKABLE QString lastPageUrl() const;

    // Commands to execute on sighting save.
    Q_INVOKABLE QVariantMap getSaveCommands(const QString& recordUid, const QString& fieldUid) const;
    Q_INVOKABLE void applyTrackCommand();
    Q_INVOKABLE void setTrackStreamer(int updateIntervalSeconds, int distanceFilterMeters = 0);
    Q_INVOKABLE void startTrackStreamer(int updateIntervalSeconds, int distanceFilterMeters = 0);
    Q_INVOKABLE void stopTrackStreamer();
    Q_INVOKABLE void pushTrackLocation(const QVariantMap& locationMap);

    // Enable or disable immersive mode.
    Q_INVOKABLE void setImmersive(bool value);

    // Save track data to an aggregate sighting.
    Q_INVOKABLE QStringList trackFieldCandidates() const;

    // Provider submit.
    Q_INVOKABLE bool canSubmitData() const;
    Q_INVOKABLE void submitData();
    Q_INVOKABLE void processAutoSubmit();
    Q_INVOKABLE int getPendingUploadCount() const;

    // Persist state.
    Q_INVOKABLE void saveState() const;

    // Evaluator.
    Q_INVOKABLE QVariant evaluate(const QString& expression, const QString& contextRecordUid, const QString& contextFieldUid, const QVariantMap& variables = QVariantMap()) const;

    // Archive data.
    Q_INVOKABLE QVariantMap exportToCSV(bool cleanup = true);

    Q_INVOKABLE QString resolveProjectUrl(const QString& projectFilename) const;

    Q_INVOKABLE void auditAdd(const QString& description) const;

    Provider* provider() const;
    Renderer* renderer() const;

    Q_INVOKABLE bool supportLocationTrackField() const;
    Q_INVOKABLE bool supportLocationTrackService() const;

    QString rootRecordUid() const;
    const QVariantMap* globals() const;

    TrackFile* trackFile();
    bool snapTrack(Sighting* sighting);
    bool snapTrackToNewSighting();
    Q_INVOKABLE bool snapTrack();

    Q_INVOKABLE QString buildSpeechPrompt(const QString& recordUid, const QStringList& fieldUids) const;
    Q_INVOKABLE QString buildSpeechSchema(const QString& recordUid, const QStringList& fieldUids) const;

    Q_INVOKABLE void requestTranscribe(const QString& filename, const QString& prompt = QString(), const QString& schema = QString(), const QString& fillRecordUid = QString(), const QStringList& fillFieldUids = QStringList()) const;

    void processTranscribeResult(const Database::DbTranscribeItem& transcribeItem);

private:
    QVariantMap m_globals;
    QString m_schemaHash;
    std::unique_ptr<Provider> m_provider;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<TrackFile> m_trackFile;
    bool m_recalculating = false;
    QString m_submitAlarmId;
    QString m_fieldStreamerRecordUid, m_fieldStreamerFieldUid;

    bool allowTrackCommands() const;
    bool allowUserTrackChange() const;

    void resetSessionId();
    void loadState(bool formChanged);
    QVariant providerAsVariant();
    Record* getRecord(const QString& recordUid = QString()) const;
    void updateLocationStreamers();
    void replayLocationStreamers();

signals:
    void projectSettingChanged(const QString& name, const QVariant& value);
    void loggedInChanged(bool loggedIn);

    void sightingsChanged();
    void sightingSaved(const QString& sightingUid);
    void sightingDeleted(const QString& sightingUid);
    void sightingModified(const QString& sightingUid);
    void sightingFlagsChanged(const QString& sightingUid);

    void locationSaved(CyberTracker::Location* location, const QString& locationUid);
    void locationPoint(CyberTracker::Location* location);

    void providerEvent(const QString& name, const QVariant& value);

    void fieldValueChanged(const QString& recordUid, const QString& fieldUid, const QVariant& oldValue, const QVariant& newValue);
    void recordChildValueChanged(const QString& recordUid, const QString& fieldUid);

    void recordCreated(const QString& recordUid);
    void recordDeleted(const QString& recordUid);

    void pagePush(const QString& pageUrl, const QVariantMap& params, int transition);
    void pagePop(int transition);
    void pagePopBack();
    void pageReplaceLast(const QString& pageUrl, const QVariantMap& params, int transition);
    void pagesPopToParent(int transition);
    void pagesLoad(const QVariantList& pageStack);

    void triggerKioskPopup();
    void triggerLogin();

    void highlightInvalid();

    void trackSettingsChanged();
    void beforeDisconnect();
};

} // namespace CyberTracker
