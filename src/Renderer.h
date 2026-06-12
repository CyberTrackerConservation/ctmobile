#pragma once
#include "pch.h"
#include "Project.h"
#include "Sighting.h"

namespace CyberTracker {

class Renderer : public QObject
{
    Q_OBJECT

public:
    explicit Renderer(QObject* parent = nullptr);

    virtual QString getSightingGroupText(Sighting* sighting) const;
    virtual QString getSightingSummaryText(Sighting* sighting) const;
    virtual QString getSightingSummaryIcon(Sighting* sighting) const;
    virtual QString getSightingStatusIcon(Sighting* sighting, Database::DataFlags flags) const;
    virtual QStringList getSightingIcons(Sighting* sighting) const;
    virtual bool getSightingCanEdit(Sighting* sighting, Database::DataFlags flags) const;
    virtual bool getSightingCanDelete(Sighting* sighting, Database::DataFlags flags) const;
    virtual QByteArray getSightingWebhook(Project* project, Sighting* sighting) const;

    struct MapSymbol
    {
        QString projectTitle;
        QColor color;
        QString filename;
        QString url;
    };
    virtual MapSymbol getSightingMapSymbol(Sighting* sighting) const;

    virtual void buildRecordModel(Sighting* sighting, QVariantList* recordModel, const std::function<bool(const FieldValue& fieldValue, int depth)>& filter) const;
    virtual QVariantList buildSightingModel(Sighting* sighting) const;
    virtual QByteArray sightingToJson(Project* project, Sighting* sighting, const std::function<bool(const FieldValue& fieldValue)>& filter) const;
};

} // namespace CyberTracker