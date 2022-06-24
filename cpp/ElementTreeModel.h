#pragma once
#include "pch.h"
#include "Element.h"

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
    ElementManager* m_elementManager = nullptr;
};
