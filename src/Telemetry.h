#pragma once
#include "pch.h"

namespace CyberTracker {

class Telemetry : public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY(QString, gaMeasurementId)
    QML_WRITABLE_AUTO_PROPERTY(QString, gaSecret)
    QML_WRITABLE_AUTO_PROPERTY(QString, aiKey)

public:
    explicit Telemetry(const QString& filePath, QObject* parent = nullptr);

    Q_INVOKABLE void gaSendEvent(const QString& name, const QVariantMap& params, bool block = false) const;
    Q_INVOKABLE void aiSendEvent(const QString& name, const QVariantMap& props = QVariantMap(), const QVariantMap& measures = QVariantMap(), bool block = false) const;

    // Cumulative per-project usage, persisted to <rootPath>/Telemetry.json.
    // Thread-safe; guarded by an internal mutex. If stateSpace is empty the
    // value is stored on the base project map; otherwise it is nested under a
    // sub-map keyed by stateSpace. getUsage always returns the whole project
    // object (including any stateSpace sub-maps).
    static void setUsage(const QString& projectUid, const QString& stateSpace, const QString& name, const QVariant& value);
    Q_INVOKABLE QVariantMap getUsage(const QString& projectUid) const;

private:
    QString m_platform;
    QString m_sku;
    QString m_build;
    QString m_filePath;
    QVariantMap m_usage;
    mutable QMutex m_usageMutex;
    void replyFinished();
};

} // namespace CyberTracker