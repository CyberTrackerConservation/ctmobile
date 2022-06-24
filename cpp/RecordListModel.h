#pragma once
#include "pch.h"
#include "Form.h"

class RecordListModel: public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

    QML_WRITABLE_AUTO_PROPERTY (QString, recordUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, fieldUid)

public:
    RecordListModel(QObject* parent = nullptr);
    ~RecordListModel();

    void classBegin() override;
    void componentComplete() override;

private slots:
    void sightingModified(const QString& sightingUid);
    void recordCreated(const QString& recordUid);
    void recordDeleted(const QString& recordUid);
    void fieldValueChanged(const QString& recordUid, const QString& fieldUid, const QVariant& oldValue, const QVariant& newValue);
    void recordChildValueChanged(const QString& recordUid, const QString& fieldUid);
    void rebuild();

private:
    bool m_completed = false;
    Form* m_form = nullptr;
    QStringList m_recordUids;
    QVariant makeItem(const QString& recordUid);
};
