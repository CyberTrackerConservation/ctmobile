import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

StackView {
    id: root

    QtObject {
        id: internal

        property bool isStartPatrol: !form.provider.patrolStarted
        property bool isResumePatrol: form.provider.patrolPaused
        property bool isChangePatrol: !isStartPatrol && !isResumePatrol
    }

    C.FieldListProxyModel {
        id: fieldListModel
        recordUid: form.rootRecordUid
        filterTagName: "metadata"
        filterTagValue: true
    }

    function setUsageTelemetry() {
        // Capture the patrol metadata fields the user is asked to enter,
        // annotated with (*) for required and (F) for fixed.
        let fieldNames = []
        for (let i = 0; i < fieldListModel.fieldUids.length; i++) {
            let fieldUid = fieldListModel.fieldUids[i]
            let field = form.getField(fieldUid)
            let name = form.getFieldName(form.rootRecordUid, fieldUid)

            let required = field.required
            if (required === true ||
                (typeof required === "string" && required.length > 0 && required !== "false()")) {
                name += " (*)"
            }

            if (field.tag.fixed === true) {
                name += " (F)"
            }

            fieldNames.push(name)
        }
        /// List of patrol metadata fields. If the field name contains "(*)" then it is required. If the field name contains "(F)" then it is fixed, meaning it cannot be changed once the patrol has been started.
        /// @category Data model
        /// @docs https://cybertrackerwiki.org/smart/user-manual/6#634-patrol-metadata-tab
        form.setUsage("patrolMetadataFields", fieldNames)
    }

    Component.onCompleted: {
        if (fieldListModel.count !== 0) {
            setUsageTelemetry()
            loader.sourceComponent = metadataPage
        } else if (internal.isStartPatrol) {
            loader.sourceComponent = locationStartPatrolPage
        } else if (internal.isResumePatrol) {
            loader.sourceComponent = locationResumePatrolPage
        } else if (internal.isChangePatrol) {
            loader.sourceComponent = locationChangePatrolPage
        } else {
            console.log("Error: unknown patrol state")
        }
    }

    initialItem: Loader {
        id: loader
    }

    Component {
        id: metadataPage

        C.ContentPage {
            property bool validate: true

            function hasChanged() {
                if (!form.provider.patrolStarted) {
                    return true
                }

                let oldState = JSON.parse(form.getControlState(form.rootRecordUid, "", Globals.metadataValues, "{}"))
                let newState = JSON.parse(listView.snapValues())
                return !Utils.compareVariantMaps(oldState, newState)
            }

            Component.onCompleted: {
                let oldState = form.getControlState(form.rootRecordUid, "", Globals.metadataValues, "{}")
                if (oldState === "{}") {
                    form.setControlState(form.rootRecordUid, "", Globals.metadataValues, listView.snapValues())
                }

                pageHeader.menuEnabled = hasChanged()
            }

            Connections {
                id: connections
                target: form

                function onFieldValueChanged(recordUid, fieldUid, oldValue, newValue) {
                    pageHeader.menuEnabled = hasChanged()
                }
            }

            header: C.PageHeader {
                id: pageHeader
                text: {
                    if (internal.isStartPatrol) {
                        return form.provider.getSightingTypeText("NewPatrol")
                    } else if (internal.isResumePatrol) {
                        return form.provider.getSightingTypeText("ResumePatrol")
                    } else if (internal.isChangePatrol) {
                        return form.provider.getSightingTypeText("ChangePatrol")
                    }
                }

                formBack: false
                onBackClicked: {
                    connections.enabled = false

                    let fieldValues = JSON.parse(form.getControlState(form.rootRecordUid, "", Globals.metadataValues, "{}"))
                    form.setFieldValues(form.rootRecordUid, fieldValues)
                    form.resetControlState(form.rootRecordUid, "", Globals.metadataValues)

                    popPage()
                }

                menuIcon: "qrc:/icons/check.svg"
                menuVisible: true
                menuEnabled: false

                onMenuClicked: {
                    if (internal.isChangePatrol && !hasChanged()) {
                        form.popPage()
                        return
                    }

                    if (!validate || listView.validate()) {
                        if (internal.isStartPatrol) {
                            root.push(locationStartPatrolPage, StackView.Immediate)
                        } else if (internal.isResumePatrol) {
                            root.push(locationResumePatrolPage, StackView.Immediate)
                        } else if (internal.isChangePatrol) {
                            root.push(locationChangePatrolPage, StackView.Immediate)
                        } else {
                            console.log("Error bad condition on metadata page")
                        }

                        Globals.patrolChangeCount++
                    }
                }
            }

            C.FieldListViewEditor {
                id: listView
                anchors.fill: parent

                recordUid: form.rootRecordUid
                filterTagName: "metadata"
                filterTagValue: true

                callbackEnabled: getFixedEnabled

                function getFixedEnabled(fieldBinding) {
                    if (form.provider.patrolStarted === false) return true
                    return fieldBinding.field.tag.fixed === true ? false : true
                }

                function snapValues() {
                    let result = ({})
                    for (let i = 0; i < listView.model.count; i++) {
                        let fieldUid = listView.model.get(i).fieldUid
                        result[fieldUid] = form.getFieldValue(listView.recordUid, fieldUid)
                    }
                    return JSON.stringify(result)
                }
            }
        }
    }

    Component {
        id: locationStartPatrolPage

        LocationPage {
            tag: "startPatrol"
            clean: true
        }
    }

    Component {
        id: locationResumePatrolPage

        LocationPage {
            tag: "resumePatrol"
            clean: true
        }
    }

    Component {
        id: locationChangePatrolPage

        LocationPage {
            tag: "changePatrol"
            clean: true
        }
    }
}
