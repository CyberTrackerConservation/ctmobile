#pragma once
#include "pch.h"
#include "Schema.h"
#include "Project.h"

namespace CyberTracker {

class ClassicSchema : public QObject
{
    Q_OBJECT

public:
    static constexpr char TRACK_FIELD_UID[] = "trackFile";

public:
    explicit ClassicSchema(QObject *parent = nullptr);

    static bool parse(Project* project, const QString& formFilePath, Schema* schema);
};

} // namespace CyberTracker
