#pragma once
#include "pch.h"
#include "Form.h"

class SightingListModel: public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

public:
    SightingListModel(QObject* parent = nullptr);
    ~SightingListModel();

    void classBegin() override;
    void componentComplete() override;

private:
    Form* m_form = nullptr;
    QStringList m_sightingUids;
    QVariant sightingToVariant(const QString& sightingUid);
    void rebuild();
};
