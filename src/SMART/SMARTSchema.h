#pragma once
#include "pch.h"
#include "Schema.h"
#include "Project.h"

namespace CyberTracker {

class SMARTSchema : public QObject
{
    Q_OBJECT

public:
    static constexpr char FIELD_MODEL[]               = "IncidentRecord";
    static constexpr char FIELD_MODEL_TREE[]          = "ModelTree";
    static constexpr char FIELD_GROUPS[]              = "GroupList";
    static constexpr char FIELD_LOCATION[]            = "Location";
    static constexpr char FIELD_DISTANCE[]            = "Distance";
    static constexpr char FIELD_BEARING[]             = "Bearing";
    static constexpr char FIELD_PHOTOS[]              = "Photos";
    static constexpr char FIELD_PHOTOS_REQUIRED[]     = "PhotosRequired";
    static constexpr char FIELD_AUDIO[]               = "Audio";

    static constexpr char FIELD_EMPLOYEES[]           = "SMART_Employees";
    static constexpr char FIELD_DATA_TYPE[]           = "SMART_DataType";
    static constexpr char FIELD_SIGHTING_GROUP_ID[]   = "SMART_SightingGroupId";
    static constexpr char FIELD_OBSERVATION_TYPE[]    = "SMART_ObservationType";
    static constexpr char FIELD_PATROL_ID[]           = "SMART_PatrolID";
    static constexpr char FIELD_PATROL_START_DATE[]   = "SMART_PatrolStartDate";
    static constexpr char FIELD_PATROL_START_TIME[]   = "SMART_PatrolStartTime";
    static constexpr char FIELD_PATROL_LEG_COUNTER[]  = "SMART_PatrolLegCounter";
    static constexpr char FIELD_PATROL_LEG_DISTANCE[] = "SMART_PatrolLegDistance";
    static constexpr char FIELD_OBS_COUNTER[]         = "SMART_ObsCounter";
    static constexpr char FIELD_NEW_WAYPOINT[]        = "SMART_NewWaypoint";
    static constexpr char FIELD_COLLECT_USER[]        = "SMART_CollectUser";
    static constexpr char FIELD_SAMPLING_UNIT[]       = "SMART_SamplingUnit";
    static constexpr char FIELD_PATROL_TRANSPORT[]    = "SMART_PatrolTransport";
    static constexpr char FIELD_OBSERVER[]            = "SMART_Observer";
    static constexpr char FIELD_INCIDENT_TYPE[]       = "SMART_IncidentType";
    static constexpr char FIELD_PHOTO_PREFIX[]        = "SMART_Photo";
    static constexpr char FIELD_AUDIO_PREFIX[]        = "SMART_Audio";

    static constexpr char ATTACHMENT_TAGS[]           = "AttachmentTags";

    static constexpr char TYPE_INCIDENT[]             = "Incident";
    static constexpr char TYPE_PATROL[]               = "Patrol";
    static constexpr char TYPE_COLLECT[]              = "Collect";

    static constexpr int DEFAULT_PHOTO_RESOLUTION_X   = 1600;
    static constexpr int DEFAULT_PHOTO_RESOLUTION_Y   = 1200;

    explicit SMARTSchema(QObject* parent = nullptr);

    bool parse(Project* project, const QString& path, QVariantList* languagesOut, int* languageIndexOut, QVariantMap* samplingGeoJsonOut);

private:
    Schema* m_schema = nullptr;
    QVariantMap m_profile;
    QVariantMap m_project;
    int m_smartVersion;

    PhotoField* createPhotoField(const QString& fieldUidSuffix, bool required) const;
    AudioField* createAudioField(const QString& fieldUidSuffix) const;

    void parseOptions(const QString& optionPrefix, const QVariantList& options, Element* baseElement, QStringList* optionUidsOut) const;
    void addMetadataItem(const QString& fieldName, const QVariantMap& fieldValues) const;
    void parseMetadata(const QString& fileName, QVariantMap* samplingGeoJsonOut) const;
    void parseModel(const QString& fileName) const;
    void postProcessModel(RecordField* modelField) const;
};

} // namespace CyberTracker