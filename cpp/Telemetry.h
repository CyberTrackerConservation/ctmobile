#pragma once
#include "pch.h"

class Telemetry : public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY(QString, gaMeasurementId)
    QML_WRITABLE_AUTO_PROPERTY(QString, gaSecret)
    QML_WRITABLE_AUTO_PROPERTY(QString, aiKey)

public:
    explicit Telemetry(QObject* parent = nullptr);

    Q_INVOKABLE void gaSendEvent(const QString& name, const QVariantMap& params) const;
    Q_INVOKABLE void aiSendEvent(const QString& name, const QVariantMap& props = QVariantMap(), const QVariantMap& measures = QVariantMap()) const;

private:
    QString m_platform;
    QString m_sku;
    QString m_build;
};
