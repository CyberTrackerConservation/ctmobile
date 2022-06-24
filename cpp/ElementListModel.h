#pragma once
#include "pch.h"
#include "Element.h"

class ElementListModel: public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

    QML_WRITABLE_AUTO_PROPERTY (QString, elementUid)
    QML_WRITABLE_AUTO_PROPERTY (bool, flatten)
    QML_WRITABLE_AUTO_PROPERTY (bool, sorted)
    QML_WRITABLE_AUTO_PROPERTY (bool, other)
    QML_WRITABLE_AUTO_PROPERTY (QString, searchFilter)
    QML_WRITABLE_AUTO_PROPERTY (QVariant, searchFilterIgnore)
    QML_WRITABLE_AUTO_PROPERTY (QString, filterByFieldRecordUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, filterByField)
    QML_WRITABLE_AUTO_PROPERTY (QString, filterByTag)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, filter)

public:
    ElementListModel(QObject* parent = nullptr);
    ~ElementListModel();

    void classBegin() override;
    void componentComplete() override;

    Q_INVOKABLE void init(const QString& elementUid, bool flatten, bool sorted, bool useFilter, const QVariantMap& filter);

private slots:
    void rebuild();

private:
    ElementManager* m_elementManager = nullptr;
};
