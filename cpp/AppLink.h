#pragma once
#include "pch.h"

class AppLink: public QObject
{
    Q_OBJECT

public:
    AppLink(QObject* parent = nullptr);

    QString encode(const QVariantMap& value);
    QVariantMap decode(const QString& value);

private:
    QStringList m_dictionary;
    QMap<QString, QString> m_shorteners;
};
