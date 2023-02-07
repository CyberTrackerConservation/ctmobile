#pragma once
#include "pch.h"
#include "Element.h"
#include "Field.h"
#include "Project.h"
#include "Task.h"
#include "Sighting.h"
#include "Database.h"

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

    void reset();
    QVariantMap toMap() const;
};

class Provider: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (QString, name)

    Q_PROPERTY (QUrl startPage READ getStartPage CONSTANT)

public:
    Provider(QObject* parent = nullptr);
    virtual ~Provider();

    virtual void save(QVariantMap& data);
    virtual void load(const QVariantMap& data);

    virtual bool connectToProject(bool newBuild, bool* formChangedOut);
    virtual void disconnectFromProject();

    virtual bool canEditSighting(Sighting* sighting, int flags) const;
    virtual void finalizeSighting(QVariantMap& sightingMap);

    virtual QVariantList buildSightingView(Sighting* sighting) const;

    virtual bool requireUsername() const;
    virtual bool requireGPSTime() const;

    virtual bool supportLocationPoint() const;
    virtual bool supportLocationTrack() const;
    virtual bool supportSightingEdit() const;
    virtual bool supportSightingDelete() const;

    virtual QUrl getStartPage();
    virtual void getElements(ElementManager* elementManager);
    virtual void getFields(FieldManager* fieldManager);

    virtual QVariantList buildMapDataLayers() const;

    virtual QString getFieldName(const QString& fieldUid) const;
    virtual QUrl getFieldIcon(const QString& fieldUid) const;
    virtual bool getFieldTitleVisible(const QString& fieldUid) const;
    virtual QString getSightingSummaryText(Sighting* sighting) const;
    virtual QUrl getSightingSummaryIcon(Sighting* sighting) const;
    virtual QUrl getSightingStatusIcon(Sighting* sighting, int flags) const;

    virtual bool notify(const QVariantMap& message);

    virtual bool finalizePackage(const QString& packageFilesPath) const;

    virtual bool canSubmitData() const;
    virtual void submitData();

protected:
    Project* m_project = nullptr;
    ElementManager* m_elementManager = nullptr;
    FieldManager* m_fieldManager = nullptr;
    TaskManager* m_taskManager = nullptr;
    Database* m_database = nullptr;

    QUrl getProjectFileUrl(const QString& filename) const;
    QString getProjectFilePath(const QString& filename) const;
};
