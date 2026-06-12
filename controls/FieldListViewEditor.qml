import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ListViewV {
    id: root

    property alias recordUid: fieldListModel.recordUid
    property alias filterElementUid: fieldListModel.filterElementUid
    property alias filterTagName: fieldListModel.filterTagName
    property alias filterTagValue: fieldListModel.filterTagValue
    property bool wizardMode: false
    property bool highlightInvalid: false
    property var callbackEnabled

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

    delegate: FieldEditorDelegate {
        recordUid: model.recordUid
        fieldUid: model.fieldUid
        showTitle: model.titleVisible
        highlightInvalid: root.highlightInvalid
        callbackEnabled: root.callbackEnabled
        wizardMode: root.wizardMode

        HorizontalDivider {}
    }

    TranscribeButton {
        id: transcribeButton
        anchors.right: parent.right
        anchors.rightMargin: App.scaleByFontSize(8)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: App.scaleByFontSize(8)
        fillRecordUid: root.recordUid
        fillFieldUids: fieldListModel.fieldUids
        visible: App.settings.formFiller
    }

    // Start: preserve contentY - note that stackview events cause the list size to change.
    Component.onCompleted: {
        loadContentY()
    }

    onContentYChanged: {
        if (!formPageStack.loading) {
            saveContentY()
        }
    }

    Connections {
        target: formPageStack

        function onLoadingChanged() {
            if (!formPageStack.loading) {
                loadContentY()
            }
        }
    }

    function saveContentY() {
        form.setControlState(root.recordUid, filterElementUid + filterTagName + filterTagValue, "contentY_" + root.height.toString(), root.contentY)
    }

    function loadContentY() {
        root.contentY = form.getControlState(root.recordUid, filterElementUid + filterTagName + filterTagValue, "contentY_" + root.height.toString(), 0)
    }
    // End: preserve contentY.

    function validate() {
        for (let i = 0; i < model.count; i++) {
            let fieldValue = model.get(i)
            if (fieldValue.valid) {
                continue
            }

            highlightInvalid = true
            console.log("Binding value invalid: " + fieldValue.fieldUid)
            root.positionViewAtIndex(i, ListView.Center)

            let message = qsTr("Value required")
            if (fieldValue.constraintMessage !== "") {
                message = fieldValue.constraintMessage
            } else if (fieldValue.requiredMessage !== "") {
                message = fieldValue.requiredMessage
            }
            showError(message)

            return false
        }

        highlightInvalid = false
        return true
    }
}
