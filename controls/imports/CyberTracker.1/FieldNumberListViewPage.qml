import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import CyberTracker 1.0 as C

ContentPage {
    id: page

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property string listElementUid
    property int depth: 1

    C.FieldBinding {
        id: fieldBinding

        Component.onCompleted: {
            if (listElementUid === "") {
                listElementUid = fieldBinding.field.listElementUid
            }

            listView.currentIndex = fieldBinding.getControlState(listElementUid + "_CurrentIndex", -1)
        }
    }

    header: C.PageHeader {
        text: form.getFieldName(recordUid, fieldUid)
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPages(depth)
    }

    Label {
        id: colorLabel
        visible: false
    }

    Component {
        id: listDelegate
        
        Item {
            width: parent.width
            height: d.height

            ItemDelegate {
                id: d
                width: parent.width
                contentItem: RowLayout {
                    SquareIcon {
                        source: form.getElementIcon(modelData.uid)
                        size: controlLabel.implicitHeight * 1.3
                    }

                    Label {
                        id: controlLabel
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: form.getElementName(modelData.uid)
                        font.pixelSize: App.settings.font16
                        color: page.getItemColor(modelData.uid)
                    }

                    Label {
                        id: numberBox
                        font.pixelSize: App.settings.font16
                        visible: !form.getElement(modelData.uid).hasChildren
                        text: page.getItemText(modelData.uid)
                        color: page.getItemColor(modelData.uid)
                    }

                    C.ChevronRight {
                        visible: form.getElement(modelData.uid).hasChildren
                    }
                }

                onClicked: {
                    if (form.getElement(modelData.uid).hasChildren) {
                        form.pushPage(Qt.resolvedUrl("FieldNumberListViewPage.qml"),
                            { listElementUid: modelData.uid, recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid, depth: depth + 1 } )
                    } else {
                        form.pushPage(Qt.resolvedUrl("FieldKeypadPage.qml"),
                            { listElementUid: modelData.uid, recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
                    }
                }
            }

            C.HorizontalDivider {}
        }
    }

    C.ListViewV {
        id: listView
        anchors.fill: parent

        model: C.ElementListModel {
            elementUid: listElementUid
        }
        currentIndex: -1
        delegate: listDelegate
    }

    function getItemText(fieldUid) {
        let a = fieldBinding.value === undefined ? {} : fieldBinding.value
        let v = a[fieldUid]
        return v === undefined ? "-" : v.toString()
    }

    function getItemColor(fieldUid) {
        let a = fieldBinding.value === undefined ? {} : fieldBinding.value
        let v = a[fieldUid]
        return v === undefined || fieldBinding.field.checkSingleValue(v) ? colorLabel.color : "red"
    }
}
