import QtQuick 2.0
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    id: page

    property alias recordUid: fieldListModel.recordUid
    property string fieldUid: ""
    property alias filterFieldUids: fieldListModel.filterFieldUids
    property alias topText: pageHeader.topText
    property bool highlightInvalid: false

    header: PageHeader {
        id: pageHeader
        text: form.project.title
        menuVisible: fieldListModel.recordUid === form.rootRecordUid
        menuIcon: "qrc:/icons/skip_next.svg"
        onMenuClicked: {
            // Look for first invalid field.
            for (let i = 0; i < fieldListModel.count; i++) {
                let fieldUid = fieldListModel.get(i).fieldUid
                let recordUid = fieldListModel.get(i).recordUid
                if (!form.getFieldValueValid(recordUid, fieldUid)) {
                    highlightInvalid = true
                    listView.currentIndex = i > 0 ? i - 1 : i
                    return
                }
            }

            // Go to last page.
            form.wizard.gotoField(fieldListModel.get(fieldListModel.count - 1).fieldUid)
        }
    }

    C.ListViewV {
        id: listView
        anchors.fill: parent

        highlightFollowsCurrentItem: true
        preferredHighlightBegin: 0
        preferredHighlightEnd: 16
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: -1
        highlightMoveVelocity: -1

        property int highlightIndex: -1

        model: C.FieldListProxyModel {
            id: fieldListModel
            Component.onCompleted: {
                listView.highlightIndex = fieldListModel.indexOfField(page.fieldUid)

                for (let i = 0; i < fieldListModel.count; i++) {
                    if (i === listView.highlightIndex) {
                        listView.currentIndex = i > 0 ? i - 1 : i
                        break
                    }

                    let fieldUid = fieldListModel.get(i).fieldUid
                    let recordUid = fieldListModel.get(i).recordUid
                    if (!form.getFieldValueValid(recordUid, fieldUid)) {
                        listView.currentIndex = i > 0 ? i - 1 : i
                        highlightInvalid = true
                        break
                    }
                }
            }
        }

        delegate: ItemDelegate {
            id: d
            visible: model.visible
            width: listView.width
            highlighted: model.index === listView.highlightIndex

            Binding {
                target: background;
                property: "color";
                value: d.highlighted ? Utils.changeAlpha(Style.colorHighlighted, 50) : Style.colorContent
            }

            FieldStateMarker {
                recordUid: model.recordUid
                fieldUid: model.fieldUid
                x: -d.padding + 4
                y: -d.padding + 4
                icon.width: 12
                icon.height: 12
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
                    spacing: 4
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

                Image {
                    fillMode: Image.PreserveAspectFit
                    source: fieldBinding.fieldIcon
                    sourceSize.width: Style.minRowHeight
                    sourceSize.height: Style.minRowHeight
                    visible: fieldBinding.fieldIcon.toString() !== ""
                    verticalAlignment: Image.AlignVCenter
                }
            }

            onClicked: {
                if (fieldBinding.field.isRecordField && fieldBinding.field.group && !fieldBinding.field.wizardFieldList) {
                    form.pushPage(form.wizard.indexPageUrl, { recordUid: fieldBinding.value[0], fieldUid: page.fieldUid, topText: fieldBinding.fieldName })
                } else {
                    form.wizard.gotoField(model.fieldUid)
                }
            }

            C.HorizontalDivider {}
        }
    }
}
