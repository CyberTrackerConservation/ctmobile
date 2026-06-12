#pragma once
#include "pch.h"
#include "Form.h"

namespace CyberTracker {

class SightingModel: public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)
    QML_WRITABLE_AUTO_PROPERTY (QString, sightingUid)
    QML_WRITABLE_AUTO_PROPERTY (Sighting*, sighting)

public:
    SightingModel(QObject* parent = nullptr);
    ~SightingModel();

    void classBegin() override;
    void componentComplete() override;

private slots:
    void rebuild();
    void changed(const QString& projectUid, const QString& sightingUid);
    void removed(const QString& projectUid, const QString& sightingUid);

private:
    Form* m_form = nullptr;
};

} // namespace CyberTracker