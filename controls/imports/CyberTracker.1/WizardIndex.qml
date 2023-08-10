import QtQuick 2.0
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

C.ListViewV {
    id: root

    property string recordUid
    property string fieldUid
    property var filterFieldUids: ([])
    property bool highlightInvalid: false

    highlightFollowsCurrentItem: true
    preferredHighlightBegin: 0
    preferredHighlightEnd: 16
    highlightRangeMode: ListView.StrictlyEnforceRange
    highlightMoveDuration: -1
    highlightMoveVelocity: -1

    QtObject {
        id: internal
        property int highlightIndex: -1
    }

    header: Rectangle {
        width: root.width
        height: form.wizard.banner === "" ? 0 : page.header.height
        color: Material.color(Material.Green, Material.Shade300)

        Label {
            anchors.fill: parent
            font.pixelSize: App.settings.font14
            fontSizeMode: Label.Fit
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            text: form.wizard.banner
        }
    }

    model: C.FieldListProxyModel {
        id: fieldListModel

        recordUid: root.recordUid
        filterFieldUids: root.filterFieldUids

        Component.onCompleted: {
            internal.highlightIndex = fieldListModel.indexOfField(root.fieldUid)

            for (let i = 0; i < fieldListModel.count; i++) {
                if (i === internal.highlightIndex) {
                    root.currentIndex = i > 0 ? i - 1 : i
                    break
                }

                let fieldUid = fieldListModel.get(i).fieldUid
                let recordUid = fieldListModel.get(i).recordUid
                if (!form.getFieldValueValid(recordUid, fieldUid)) {
                    root.currentIndex = i > 0 ? i - 1 : i
                    root.highlightInvalid = true
                    break
                }
            }
        }
    }

    delegate: HighlightDelegate {
        id: d
        visible: model.visible
        width: root.width
        highlighted: model.index === internal.highlightIndex

        FieldStateMarker {
            recordUid: model.recordUid
            fieldUid: model.fieldUid
            x: -d.padding + App.scaleByFontSize(2)
            y: -d.padding + App.scaleByFontSize(2)
            icon.width: App.scaleByFontSize(10)
            icon.height: App.scaleByFontSize(10)
            icon.color: fieldName.color
        }

        C.FieldBinding {
            id: fieldBinding
            recordUid: model.recordUid
            fieldUid: model.fieldUid
        }

        contentItem: RowLayout {
            Item { height: Style.minRowHeight }

            Column {
                Layout.fillWidth: true
                spacing: App.scaleByFontSize(4)
                height: fieldName.implicitHeight + fieldValue.implicitHeight

                C.FieldName {
                    id: fieldName
                    width: parent.width
                    recordUid: fieldBinding.recordUid
                    fieldUid: fieldBinding.fieldUid
                    color: (highlightInvalid && !fieldBinding.isValid) ? Style.colorInvalid : Material.foreground
                    font.bold: d.highlighted || (highlightInvalid && !fieldBinding.isValid) ? true : false
                    visible: model.titleVisible
                }

                Label {
                    id: fieldValue
                    width: parent.width
                    color: fieldBinding.isCalculated ? Style.colorCalculated : Material.foreground
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    wrapMode: Label.Wrap
                    text: fieldBinding.displayValue
                    visible: !fieldBinding.isEmpty && text !== ""
                }
            }

            SquareIcon {
                source: fieldBinding.fieldIcon
                size: Style.minRowHeight
            }

            ChevronRight {
                visible: fieldBinding.field.isRecordField && fieldBinding.field.group
            }
        }

        onClicked: {
            if (fieldBinding.field.isRecordField && fieldBinding.field.group && !fieldBinding.field.wizardFieldList) {
                form.pushPage(form.wizard.indexPageUrl, { recordUid: fieldBinding.value[0], fieldUid: root.fieldUid, topText: fieldBinding.fieldName })
            } else {
                form.wizard.gotoField(model.fieldUid)
            }
        }

        C.HorizontalDivider {}
    }

    Component.onCompleted: {
        if (form.wizard.banner !== "") {
            root.highlightRangeMode = ListView.NoHighlightRange
            root.positionViewAtBeginning()
            return
        }

        let highlightIndex = fieldListModel.indexOfField(root.fieldUid)
        if (highlightIndex !== -1) {
            root.positionViewAtIndex(highlightIndex, ListView.Contain)
        }
    }

    function gotoLastField() {
        // Look for first invalid field.
        for (let i = 0; i < fieldListModel.count; i++) {
            let fieldUid = fieldListModel.get(i).fieldUid
            let recordUid = fieldListModel.get(i).recordUid
            if (!form.getFieldValueValid(recordUid, fieldUid)) {
                root.highlightInvalid = true
                root.currentIndex = i > 0 ? i - 1 : i
                return
            }
        }

        // Get the last field.
        return form.wizard.gotoField(fieldListModel.get(fieldListModel.count - 1).fieldUid)
    }
}
