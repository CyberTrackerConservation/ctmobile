#pragma once
#include "pch.h"

namespace CyberTracker {

class Audit : public QObject
{
    Q_OBJECT

public:
    explicit Audit(QObject* parent = nullptr);

    static void add(const QString& description);
    static void add(const QString& projectTitle, const QString& description);

    Q_INVOKABLE void clear() const;

    Q_INVOKABLE QVariantList buildModel() const;

    Q_INVOKABLE QString createCSV() const;

private:
    void internalAdd(const QString& projectTitle, const QString& description) const;
};

} // namespace CyberTracker
