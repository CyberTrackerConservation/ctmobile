#pragma once

#include "pch.h"
#include "Schema.h"

namespace CyberTracker {

class EarthRangerSchema : public QObject
{
    Q_OBJECT

public:
    static constexpr char FIELD_LOCATION[] = "__location";
    static constexpr char FIELD_PHOTOS[]   = "__photos";
    static constexpr char FIELD_NOTES[]    = "__notes";
    static constexpr char ELEMENT_REPORTED_BY[] = "__reportedBy";

public:
    EarthRangerSchema(QObject *parent = nullptr);
    static bool parse(const QString& path, Schema* schema);

private:
    int m_runningUid = 0;
    QString m_path;
    void parseReportedBy(Schema* schema);
    void parseEventTypes(Schema* schema);
    bool parseField(Schema* schema, const QString& reportUid, RecordField* recordField, const QJsonObject& jsonField, QString* keyOut);
    QJsonObject resolveForm(const QJsonValue& form, const QJsonObject& props);
    void parseReport(Schema* schema, Element* reportElement);
};

} // namespace CyberTracker