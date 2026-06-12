#pragma once
#include "Renderer.h"

namespace CyberTracker {

class ClassicRenderer : public Renderer
{
    Q_OBJECT

public:
    static constexpr char RENDERER_NAME[] = "Classic";

    explicit ClassicRenderer(QObject* parent = nullptr);

    QVariantList buildSightingModel(Sighting* sighting) const override;

    QStringList getSightingIcons(Sighting* sighting) const override;
    bool getSightingCanDelete(Sighting* sighting, Database::DataFlags flags) const override;
};

} // namespace CyberTracker
