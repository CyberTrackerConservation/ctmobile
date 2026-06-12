#pragma once
#include "pch.h"
#include "Renderer.h"

namespace CyberTracker {

class EarthRangerRenderer : public Renderer
{
    Q_OBJECT

public:
    static constexpr char RENDERER_NAME[] = "EarthRanger";

public:
    explicit EarthRangerRenderer(QObject* parent = nullptr);

    QString getSightingSummaryText(Sighting* sighting) const override;
    QString getSightingSummaryIcon(Sighting* sighting) const override;
    QString getSightingStatusIcon(Sighting* sighting, Database::DataFlags flags) const override;
    QByteArray getSightingWebhook(Project* project, Sighting* sighting) const override;
};

} // namespace CyberTracker