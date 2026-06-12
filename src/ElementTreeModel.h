#pragma once
#include "pch.h"
#include "Schema.h"

namespace CyberTracker {

class ElementTreeModel: public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

    QML_WRITABLE_AUTO_PROPERTY (QString, elementUid)

public:
    ElementTreeModel(QObject* parent = nullptr);
    ~ElementTreeModel();

    void classBegin() override;
    void componentComplete() override;

private slots:
    void rebuild();

private:
    Schema* m_schema = nullptr;
};

} // namespace CyberTracker