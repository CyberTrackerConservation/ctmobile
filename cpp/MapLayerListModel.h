#pragma once
#include "pch.h"
#include "Project.h"

class MapLayerListModel : public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

public:
    MapLayerListModel(QObject* parent = nullptr);
    ~MapLayerListModel();

    void classBegin() override;
    void componentComplete() override;

    Q_INVOKABLE int layerIdToIndex(const QString& layerId) const;

    Q_INVOKABLE void setBasemap(const QString& basemapId);
    Q_INVOKABLE void setProjectLayerState(const QString& layerId, bool checked);
    Q_INVOKABLE void setDataLayerState(const QString& layerId, bool checked);
    Q_INVOKABLE void removeGotoLayer(const QString& layerId);

signals:
    void changed();

private slots:
    void rebuild();

private:
    bool m_completed = false;
    bool m_blockRebuild = false;

    void setChecked(const QString& layerId, bool checked);

    QVariantList buildBaseLayers();
    QVariantList buildProjectLayers(ProjectManager* projectManager, const QString& filterProjectUid);
    QVariantList buildGotoLayers();
    QVariantList buildFormLayers();
};
