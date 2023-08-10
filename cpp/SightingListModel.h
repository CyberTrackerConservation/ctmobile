#pragma once
#include "pch.h"
#include "Form.h"

class SightingListModel: public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)
    QML_READONLY_AUTO_PROPERTY(bool, canSubmit)
    QML_WRITABLE_AUTO_PROPERTY(bool, groupExported)

public:
    SightingListModel(QObject* parent = nullptr);
    ~SightingListModel();

    void classBegin() override;
    void componentComplete() override;

private:
    Form* m_form = nullptr;
    QStringList m_sightingUids;
    QVariant sightingToVariant(Sighting* sighting, int flags) const;
    QVariant sightingToVariant(const QString& sightingUid) const;
    void sightingChanged(const QString& sightingUid);
    void rebuild();
    void computeCanSubmit();
};
