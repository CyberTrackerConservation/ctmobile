#pragma once
#include "Renderer.h"

namespace CyberTracker {

class SMARTRenderer : public Renderer
{
    Q_OBJECT

public:
    static constexpr char RENDERER_NAME[] = "SMART";

public:
    explicit SMARTRenderer(QObject *parent = nullptr);

    QString getPatrolText(bool surveyMode) const;
    QString getSightingTypeText(const QString& observationType, bool surveyMode) const;
    QString getSightingTypeIcon(const QString& observationType) const;

    QString getSightingGroupText(Sighting* sighting) const override;
    QString getSightingSummaryText(Sighting* sighting) const override;
    QString getSightingSummaryIcon(Sighting* sighting) const override;
    QString getSightingStatusIcon(Sighting* sighting, Database::DataFlags flags) const override;
    QStringList getSightingIcons(Sighting* sighting) const override;
    bool getSightingCanEdit(Sighting* sighting, Database::DataFlags flags) const override;
    bool getSightingCanDelete(Sighting* sighting, Database::DataFlags flags) const override;
    QByteArray getSightingWebhook(Project* project, Sighting* sighting) const override;

    MapSymbol getSightingMapSymbol(Sighting* sighting) const override;

    QVariantList buildSightingModel(Sighting* sighting) const override;
};

} // namespace CyberTracker