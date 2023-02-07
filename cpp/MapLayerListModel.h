#pragma once
#include "pch.h"
#include "Project.h"

class MapLayerListModel : public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES (QQmlParserStatus)

    QML_WRITABLE_AUTO_PROPERTY(bool, addTypeHeaders)
    QML_WRITABLE_AUTO_PROPERTY(bool, addBaseLayers)
    QML_WRITABLE_AUTO_PROPERTY(bool, addOfflineLayers)
    QML_WRITABLE_AUTO_PROPERTY(bool, addProjectLayers)
    QML_WRITABLE_AUTO_PROPERTY(bool, addFormLayers)

public:
    MapLayerListModel(QObject* parent = nullptr);
    ~MapLayerListModel();

    void classBegin() override;
    void componentComplete() override;

    Q_INVOKABLE int indexOfLayer(const QString& layerId) const;
    Q_INVOKABLE QVariantMap findExtent(const QString& layerId) const;
    Q_INVOKABLE QVariantList findPath(int pathId) const;

    Q_INVOKABLE void setBasemap(const QString& basemapId);
    Q_INVOKABLE void setLayerState(const QVariantMap& layer, bool checked);

signals:
    void changed();

private slots:
    void rebuild();

private:
    bool m_completed = false;
    bool m_blockRebuild = false;
    QVariantList m_paths;

    QVariantList buildBaseLayers() const;
    QVariantList buildOfflineLayers();
    QVariantList buildFormLayers(Project* project) const;
};
