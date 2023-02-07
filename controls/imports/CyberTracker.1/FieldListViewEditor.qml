import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ListViewV  {
    id: root

    property alias recordUid: fieldListModel.recordUid
    property alias filterElementUid: fieldListModel.filterElementUid
    property alias filterTagName: fieldListModel.filterTagName
    property alias filterTagValue: fieldListModel.filterTagValue
    property bool wizardMode: false
    property bool highlightInvalid: false
    property var callbackEnabled

    // Hack start: prevent the scroll position from changing when the list is resized (due to stackview Push/Pop events).
    width: parent.width
    Component.onCompleted: height = parent.height
    Connections {
        target: appWindow
        
        function onHeightChanged() {
            root.height = parent.height
        }
    }
    // Hack end.

    model: C.FieldListProxyModel {
        id: fieldListModel
    }

    Component {
        id: sectionHeading

        Item {
            width: root.width
            height: labelHeading.implicitHeight * 2

            Rectangle {
                anchors.fill: parent
                opacity: 0.5
                color: Material.primary
            }

            Label {
                id: labelHeading
                anchors.fill: parent
                text: section
                font.bold: true
                font.pixelSize: App.settings.font14
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    section.property: "section"
    section.criteria: ViewSection.FullString
    section.delegate: sectionHeading

    delegate: C.FieldEditorDelegate {
        recordUid: model.recordUid
        fieldUid: model.fieldUid
        showTitle: model.titleVisible
        highlightInvalid: root.highlightInvalid
        callbackEnabled: root.callbackEnabled
        wizardMode: root.wizardMode

        HorizontalDivider {}
    }

    function validate() {
        let oneErrorShown = false

        for (let i = 0; i < model.count; i++) {
            let fieldValue = model.get(i)
            if (fieldValue.valid) {
                continue
            }

            highlightInvalid = true
            console.log("Binding value invalid: " + fieldValue.fieldUid)
            root.positionViewAtIndex(i, ListView.Center)

            if (!oneErrorShown && fieldValue.constraintMessage !== "") {
                oneErrorShown = true
                showError(fieldValue.constraintMessage)
            }

            return false
        }

        highlightInvalid = false
        return true
    }
}
