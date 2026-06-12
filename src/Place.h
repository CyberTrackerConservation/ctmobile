#pragma once
#include "pch.h"

namespace CyberTracker {

struct PlaceRow
{
    QStringList path;
    QString name;
    QString title;
    QString subtitle;
    QVariantList blocks;
    void load(const QVariantMap& source);
    QString hash() const;
};

class Place : public QObject
{
    Q_OBJECT

public:
    explicit Place(QObject *parent = nullptr);

    Q_INVOKABLE QVariantList buildModel(const QStringList& stack);

    Q_INVOKABLE bool isEmpty() const;

    Q_INVOKABLE void clear();

    Q_INVOKABLE bool search(double x, double y);

private:
    QStringList m_rows;
    QMap<QString, PlaceRow> m_items;
    QString m_packageLanguage;
    QString m_defaultLocale;
    void merge(const QString& filePath, const QStringList& featureIds);
};

} // namespace CyberTracker
