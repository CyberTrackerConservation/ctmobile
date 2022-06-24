#pragma once
#include "pch.h"
#include "Database.h"
#include "TimeManager.h"
#include "Project.h"
#include "Element.h"
#include "Field.h"
#include "Location.h"
#include "ProviderInterface.h"
#include "Sighting.h"
#include "Wizard.h"

class Form: public QQuickItem
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

    QML_READONLY_AUTO_PROPERTY (Project*, project)
    QML_READONLY_AUTO_PROPERTY (int, depth)
    QML_READONLY_AUTO_PROPERTY (bool, connected)
    QML_READONLY_AUTO_PROPERTY (bool, editing)

    QML_READONLY_AUTO_PROPERTY (Database*, database)
    QML_READONLY_AUTO_PROPERTY (TimeManager*, timeManager)
    QML_READONLY_AUTO_PROPERTY (ProjectManager*, projectManager)
    QML_READONLY_AUTO_PROPERTY (ElementManager*, elementManager)
    QML_READONLY_AUTO_PROPERTY (FieldManager*, fieldManager)
    QML_READONLY_AUTO_PROPERTY (LocationStreamer*, trackStreamer)
    QML_READONLY_AUTO_PROPERTY (LocationStreamer*, pointStreamer)
    QML_READONLY_AUTO_PROPERTY (Wizard*, wizard)

    QML_READONLY_AUTO_PROPERTY (Sighting*, sighting)

    QML_WRITABLE_AUTO_PROPERTY (QString, projectUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, editSightingUid)

    QML_WRITABLE_AUTO_PROPERTY (QString, stateSpace)
    QML_WRITABLE_AUTO_PROPERTY (bool, readonly)
    QML_READONLY_AUTO_PROPERTY (QString, languageCode)
    QML_READONLY_AUTO_PROPERTY (bool, useGPSTime)

    QML_WRITABLE_AUTO_PROPERTY (bool, tracePageChanges)

    QML_READONLY_AUTO_PROPERTY (bool, requireUsername)
    QML_READONLY_AUTO_PROPERTY (QUrl, startPage)

    Q_PROPERTY (QString rootRecordUid READ rootRecordUid NOTIFY sightingModified)

    // Expose the provider as a variant, so QML can use it dynamically.
    Q_PROPERTY (QVariant provider READ providerAsVariant NOTIFY projectChanged)

public:
    Form(QObject *parent = nullptr);
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
    Q_INVOKABLE QUrl getPage(const QString& pageName) const;

    // Elements.
    Q_INVOKABLE Element* getElement(const QString& elementUid) const;
    Q_INVOKABLE QString getElementName(const QString& elementUid) const;
    Q_INVOKABLE QUrl getElementIcon(const QString& elementUid, bool walkBack = false) const;
    Q_INVOKABLE QColor getElementColor(const QString& elementUid) const;
    Q_INVOKABLE QVariant getElementTag(const QString& elementUid, const QString& key) const;
    Q_INVOKABLE QStringList getElementFieldList(const QString& elementUid) const;

    // Lists and views.
    Q_INVOKABLE QVariantList buildSightingView(const QString& sightingUid);

    // Map layers.
    QVariantMap buildSightingMapLayer(const QString& layerUid, const std::function<QVariantMap(const Sighting* sighting)>& getSymbol);
    Q_INVOKABLE QVariantMap buildTrackMapLayer(const QString& layerUid, const QVariantMap& symbol, bool filterToLastSegment = false);

    // Sighting save, load, edit, next, prev.
    void saveSighting(Sighting* sighting, bool autoSaveTrack = true);
    Q_INVOKABLE void saveSighting();
    Q_INVOKABLE void loadSighting(const QString& sightingUid);
    Q_INVOKABLE bool canEditSighting(const QString& sightingUid) const;
    Q_INVOKABLE void newSighting(bool copyFromCurrent = false);
    Q_INVOKABLE void removeSighting(const QString& sightingUid);
    Q_INVOKABLE void prevSighting();
    Q_INVOKABLE void nextSighting();

    Q_INVOKABLE void snapCreateTime();
    Q_INVOKABLE void snapUpdateTime();

    Q_INVOKABLE bool hasData() const;
    Q_INVOKABLE bool isSightingValid(const QString& sightingUid) const; // no callers
    Q_INVOKABLE bool isSightingExported(const QString& sightingUid) const; // no callers
    Q_INVOKABLE bool isSightingUploaded(const QString& sightingUid) const; // no callers
    Q_INVOKABLE bool isSightingCompleted(const QString& sightingUid) const; // no callers

    Q_INVOKABLE void markSightingCompleted();
    Q_INVOKABLE void removeExportedSightings();
    Q_INVOKABLE void removeUploadedSightings();

    // Project-scope variables.
    Q_INVOKABLE QVariant getSetting(const QString& name, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setSetting(const QString& name, const QVariant& value = QVariant());

    // Sighting-scope variables.
    Q_INVOKABLE QVariant getGlobal(const QString&, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setGlobal(const QString&, QVariant value = QVariant());
    
    Q_INVOKABLE QVariant getControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& value);

    // Fields.
    Q_INVOKABLE BaseField* getField(const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldType(const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldName(const QString& recordUid, const QString& fieldUid) const;
    Q_INVOKABLE QUrl getFieldIcon(const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldHint(const QString& recordUid, const QString& fieldUid) const;
    Q_INVOKABLE QUrl getFieldHintIcon(const QString& fieldUid) const;
    Q_INVOKABLE QUrl getFieldHintLink(const QString& fieldUid) const;
    Q_INVOKABLE QString getFieldConstraintMessage(const QString& recordUid, const QString& fieldUid) const;

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

    // Field values.
    Q_INVOKABLE QVariant getFieldParameter(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE QVariant getFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE QVariant getFieldValue(const QString& fieldUid, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& value);
    Q_INVOKABLE void setFieldValue(const QString& fieldUid, const QVariant& value);
    Q_INVOKABLE void resetFieldValue(const QString& recordUid, const QString& fieldUid);
    Q_INVOKABLE void resetFieldValue(const QString& fieldUid);
    Q_INVOKABLE void resetRecord(const QString& recordUid);

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
    Q_INVOKABLE void pushWizardPage(const QString& recordUid, const QStringList& fieldUids = QStringList(), int transition = /*StackView.Immediate*/ 0);
    Q_INVOKABLE void popPage(int transition = /*StackView.PopTransition*/ 3);
    Q_INVOKABLE void popPages(int count);
    Q_INVOKABLE void popPagesToStart();
    Q_INVOKABLE void popPagesToParent();
    Q_INVOKABLE void replaceLastPage(const QString& pageUrl, const QVariantMap& params = QVariantMap(), int transition = 0);
    Q_INVOKABLE void loadPages();
    Q_INVOKABLE void dumpPages(const QString& description);
    Q_INVOKABLE QVariantList getPageStack();
    Q_INVOKABLE void setPageStack(const QVariantList& value);

    // Persist state.
    Q_INVOKABLE void saveState();

    // Evaluator.
    Q_INVOKABLE QVariant evaluate(const QString& expression, const QString& contextRecordUid, const QString& contextFieldUid, const QVariantMap& variables = QVariantMap()) const;

    Provider* provider() const;
    QString rootRecordUid() const;
    const QVariantMap* globals() const;

private:
    void loadState();

    QVariantMap m_globals;
    std::unique_ptr<Provider> m_provider;
    bool m_recalculating = false;
    bool m_saveTrack = true;

    QVariant providerAsVariant();
    Record* getRecord(const QString& recordUid = QString()) const;

signals:
    void projectSettingChanged(const QString& name, const QVariant& value);
    void loggedInChanged(bool loggedIn);

    void sightingsChanged();
    void sightingSaved(const QString& sightingUid);
    void sightingRemoved(const QString& sightingUid);
    void sightingModified(const QString& sightingUid);

    void locationTrack(const QVariantMap& locationMap, const QString& locationUid);
    void locationPoint(const QVariantMap& locationMap);

    void providerEvent(const QString& name, const QVariant& value);

    void fieldValueChanged(const QString& recordUid, const QString& fieldUid, const QVariant& oldValue, const QVariant& newValue);
    void recordChildValueChanged(const QString& recordUid, const QString& fieldUid);

    void recordCreated(const QString& recordUid);
    void recordDeleted(const QString& recordUid);
    void recordComplete(const QString& recordUid);

    void pagePush(const QString& pageUrl, const QVariantMap& params, int transition);
    void pagePop(int transition);
    void pageReplaceLast(const QString& pageUrl, const QVariantMap& params, int transition);
    void pagesPopToParent(int transition);
    void pagesLoad(const QVariantList& pageStack);

    void triggerKioskPopup();
    void triggerLogin();

    void highlightInvalid();
};
