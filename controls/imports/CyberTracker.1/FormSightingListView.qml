import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ListViewV {
    id: listView

    property C.Form sightingForm
    property string sightingUid

    onSightingFormChanged: sightingUid = ""

    onSightingUidChanged: {
        if (sightingForm !== null && sightingUid !== "") {
            listView.model = sightingForm.buildSightingView(sightingUid)
        } else {
            listView.model = undefined
        }
    }

    delegate: ItemDelegate {
        id: d
        width: ListView.view.width
        implicitHeight: wrapDelegate.implicitHeight

        Component {
            id: groupDelegateComponent
            Label {
                anchors.fill: parent
                text: modelData.caption
                font.bold: true
                font.pixelSize: App.settings.font14
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Component {
            id: elementDelegateComponent
            RowLayout {
                Label {
                    id: controlLabel
                    Layout.fillWidth: true
                    wrapMode: Label.WordWrap
                    text: sightingForm.getElementName(modelData.elementUid)
                    font.bold: true
                    font.pixelSize: App.settings.font14
                    verticalAlignment: Text.AlignVCenter
                }

                Image {
                    Layout.alignment: Qt.AlignRight
                    source: sightingForm.getElementIcon(modelData.elementUid, true)
                    sourceSize.height: controlLabel.height * 2
                }
            }
        }

        Component {
            id: timestampDelegateComponent
            Label {
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.bold: true
                font.pixelSize: App.settings.font14
                fontSizeMode: Label.Fit
                text: App.formatDateTime(modelData.createTime)
            }
        }

        Component {
            id: locationDelegateComponent
            Label {
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.bold: true
                font.pixelSize: App.settings.font14
                fontSizeMode: Label.Fit
                text: App.getLocationText(modelData.value.x, modelData.value.y, modelData.value.z)
            }
        }

        Component {
            id: recordHeaderDelegateComponent
            Label {
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                font.bold: true
                font.pixelSize: App.settings.font14
                fontSizeMode: Label.Fit
                text: modelData.text
            }
        }

        Component {
            id: fieldDelegateComponent
            RowLayout {
                Item {
                    width: 16
                }

                ColumnLayout {
                    Label {
                        Layout.preferredWidth: (listView.width / 2) - 32
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                        text: modelData.fieldName
                    }

                    Label {
                        Layout.preferredWidth: (listView.width / 2) - 32
                        font.pixelSize: App.settings.font12
                        wrapMode: Label.WordWrap
                        text: visible ? modelData.sectionName : ""
                        opacity: 0.5
                        visible: modelData.sectionName !== undefined
                    }
                }

                Label {
                    Layout.fillWidth: true
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                    text: modelData.fieldValue
                }
            }
        }

        Component {
            id: photosDelegateComponent
            Control {
                anchors.fill: parent
                implicitHeight: imageSize
                property var imageSize: 96

                C.ListViewH {
                    id: listViewPhotos
                    anchors.fill: parent
                    leftMargin: spacing
                    spacing: 8
                    model: modelData.filenames
                    delegate: ItemDelegate {
                        width: imageSize
                        height: imageSize
                        padding: 0
                        contentItem: Item {
                            Image {
                                id: image
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectFit
                                source: modelData !== "" ? App.getMediaFileUrl(modelData) : ""
                                sourceSize.width: imageSize
                                sourceSize.height: imageSize
                                autoTransform: true
                            }
                        }
                        enabled: modelData !== ""
                        onClicked: form.pushPage("qrc:/imports/CyberTracker.1/ImagePage.qml", { text: qsTr("Photo"), source: image.source } )
                    }
                }
            }
        }

        ItemDelegate {
            id: wrapDelegate
            anchors.fill: parent

            padding: 8

            function getComponent() {
                backColor.color = "transparent"
                switch (modelData.delegate) {
                case "group":
                    backColor.color = Material.primary
                    return groupDelegateComponent

                case "element":
                    return elementDelegateComponent

                case "timestamp":
                    return timestampDelegateComponent

                case "field":
                    return fieldDelegateComponent

                case "photos":
                    return photosDelegateComponent

                case "location":
                    return locationDelegateComponent

                case "recordHeader":
                    return recordHeaderDelegateComponent

                default:
                    return undefined
                }
            }

            contentItem: getComponent().createObject(wrapDelegate)

            Rectangle {
                x: leftPadding
                y: parent.height - 1
                width: contentItem.width
                height: 1
                color: C.Style.colorGroove
                visible: modelData.divider === true
            }
        }

        background: Rectangle {
            id: backColor
            anchors.fill: parent
            opacity: 0.5
        }
    }
}
