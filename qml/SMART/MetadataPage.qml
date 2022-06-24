import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property bool validate: true

    property bool isStartPatrol: !form.provider.patrolStarted
    property bool isResumePatrol: form.provider.patrolPaused
    property bool isChangePatrol: !isStartPatrol && !isResumePatrol

    property var beforeFieldValues

    function hasChanged() {
        delete beforeFieldValues[Globals.modelLocationFieldUid]
        delete beforeFieldValues[Globals.modelPatrolLegDistance]

        let afterFieldValues = form.getRecordData(form.rootRecordUid).fieldValues
        delete afterFieldValues[Globals.modelLocationFieldUid]
        delete afterFieldValues[Globals.modelPatrolLegDistance]

        return JSON.stringify(beforeFieldValues) !== JSON.stringify(afterFieldValues)
    }

    Component.onCompleted: {
        // Snap the record state to check if it changes.
        beforeFieldValues = form.getRecordData(form.rootRecordUid).fieldValues
    }

    header: C.PageHeader {
        id: pageHeader
        text: {
            if (isStartPatrol) {
                return form.provider.getSightingTypeText("NewPatrol")
            } else if (isResumePatrol) {
                return form.provider.getSightingTypeText("ResumePatrol")
            } else if (isChangePatrol) {
                return form.provider.getSightingTypeText("ChangePatrol")
            }
        }

        formBack: false
        onBackClicked: {
            if (isChangePatrol) {
                menuClicked()
            } else {
                form.popPage()
            }
        }

        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true

        onMenuClicked: {
            if (isChangePatrol && !hasChanged()) {
                form.popPage()
                return
            }

            if (!validate || listView.validate()) {
                if (isStartPatrol) {
                    form.provider.startPatrol()
                    showToast(form.provider.surveyMode ? qsTr("Survey started") : qsTr("Patrol started"))
                    form.replaceLastPage("qrc:/SMART/SightingHomePage.qml")
                } else if (isResumePatrol) {
                    form.provider.resumePatrol()
                    showToast(form.provider.surveyMode ? qsTr("Survey resumed") : qsTr("Patrol resumed"))
                    form.replaceLastPage("qrc:/SMART/SightingHomePage.qml")
                } else if (isChangePatrol) {
                    form.provider.changePatrol()
                    showToast(form.provider.surveyMode ? qsTr("Survey changed") : qsTr("Patrol changed"))
                    form.popPage()
                } else {
                    console.log("Error bad condition on metadata page")
                }

                Globals.patrolChangeCount++
            }
        }
    }

    C.FieldListViewEditor {
        id: listView

        recordUid: rootRecordUid
        filterTagName: "metadata"
        filterTagValue: true

        callbackEnabled: getFixedEnabled

        function getFixedEnabled(fieldBinding) {
            if (form.provider.patrolStarted === false) return true
            return fieldBinding.field.tag.fixed === true ? false : true
        }
    }
}
