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

public:
    Provider(QObject* parent = nullptr);
    virtual ~Provider();

    virtual void save(QVariantMap& data);
    virtual void load(const QVariantMap& data);

    virtual bool connectToProject(bool newBuild);
    virtual void disconnectFromProject();

    virtual bool canEditSighting(Sighting* sighting, int flags);
    virtual void finalizeSighting(QVariantMap& sightingMap);

    virtual QVariantList buildSightingView(Sighting* sighting);

    virtual bool requireUsername() const;

    virtual QUrl getStartPage();
    virtual void getElements(ElementManager* elementManager);
    virtual void getFields(FieldManager* fieldManager);

    virtual QVariantList buildMapDataLayers();

    virtual bool getUseGPSTime();
    
    virtual QString getFieldName(const QString& fieldUid);
    virtual QUrl getFieldIcon(const QString& fieldUid);

    virtual bool getFieldTitleVisible(const QString& fieldUid);

    virtual bool notify(const QVariantMap& message);

    virtual bool finalizePackage(const QString& packageFilesPath) const;

protected:
    Project* m_project = nullptr;
    ElementManager* m_elementManager = nullptr;
    FieldManager* m_fieldManager = nullptr;
    TaskManager* m_taskManager = nullptr;
    Database* m_database = nullptr;

    QUrl getProjectFileUrl(const QString& filename) const;
    QString getProjectFilePath(const QString& filename) const;
};
