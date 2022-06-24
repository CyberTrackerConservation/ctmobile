pragma Singleton

import QtQuick 2.12

Item {
    // Must match with SMARTProvider.cpp.
    readonly property string modelTreeFieldUid: "ModelTree"
    readonly property string modelTreeElementUid: "ModelTree"
    readonly property string modelSamplingUnitFieldUid: "SMART_SamplingUnit"
    readonly property string modelLocationFieldUid: "Location"
    readonly property string modelDistanceFieldUid: "Distance"
    readonly property string modelBearingFieldUid: "Bearing"
    readonly property string modelIncidentGroupListFieldUid: "GroupList"
    readonly property string modelIncidentRecordFieldUid: "IncidentRecord"
    readonly property string modelIncidentGroupFieldUid: "SMART_SightingGroupId"
    readonly property string modelIncidentTypeFieldUid: "SMART_ObservationType"
    readonly property string modelObserverFieldUid: "SMART_Observer"
    readonly property string modelCollectUserFieldUid: "SMART_CollectUser"
    readonly property string modelPatrolLegDistance: "SMART_PatrolLegDistance"

    readonly property string formSpacePatrol: "Patrol"
    readonly property string formSpaceIncident: "Incident"
    readonly property string formSpaceCollect: "Collect"

    // For PAUSE/RESUME caption changes across forms.
    property int patrolChangeCount: 0
}
