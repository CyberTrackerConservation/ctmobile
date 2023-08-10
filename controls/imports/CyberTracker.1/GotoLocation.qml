import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0

RowLayout {
    spacing: App.scaleByFontSize(8)

    Item {
        width: 96
        height: 96
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

        Image {
            id: image
            anchors.fill: parent
            rotation: App.gotoManager.gotoHeading
            source: "qrc:/icons/arrow_up_circle_outline.svg"
            sourceSize.width: width
            sourceSize.height: height
            opacity: 0.75
        }

        layer {
            enabled: true
            effect: ColorOverlay {
                color: Material.foreground
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: App.gotoManager.gotoTitleText
            font.pixelSize: App.settings.font18
            wrapMode: Label.WordWrap
            opacity: 0.75
        }

        Label {
            Layout.fillWidth: true
            text: App.gotoManager.gotoLocationText
            font.pixelSize: App.settings.font13
            fontSizeMode: Label.Fit
            opacity: 0.75
        }

        Label {
            Layout.fillWidth: true
            text: App.gotoManager.gotoDistanceText
            font.pixelSize: App.settings.font13
            opacity: 0.75
        }

        Label {
            Layout.fillWidth: true
            text: App.gotoManager.gotoHeadingText
            font.pixelSize: App.settings.font13
            opacity: 0.75
        }

        Flow {
            Layout.fillWidth: true
            spacing: 0
            ToolButton {
                icon.source: "qrc:/icons/edit_location_outline.svg"
                text: qsTr("Location")
                font.pixelSize: App.settings.font12
                font.capitalization: Font.AllUppercase
                onClicked: {
                    let params = { setGotoTarget: true }
                    if (typeof(formPageStack) !== "undefined") {
                        form.pushPage(Qt.resolvedUrl("LocationPage.qml"), params)
                    } else {
                        appPageStack.push(Qt.resolvedUrl("LocationPage.qml"), params)
                    }
                }
                visible: App.gotoManager.gotoPath.length === 0
                opacity: 0.75
            }

            ToolButton {
                icon.source: "qrc:/icons/stop.svg"
                text: qsTr("Stop")
                font.pixelSize: App.settings.font12
                font.capitalization: Font.AllUppercase
                onClicked: App.gotoManager.setTarget("", [], 0)
                visible: App.gotoManager.gotoPath.length !== 0
                opacity: 0.75
            }

            Item {
                width: 16
            }

            ToolButton {
                icon.source: "qrc:/icons/fast_rewind.svg"
                font.pixelSize: App.settings.font12
                font.capitalization: Font.AllUppercase
                text: qsTr("Back")
                onClicked: App.gotoManager.setTarget(App.gotoManager.gotoTitle, App.gotoManager.gotoPath, App.gotoManager.gotoPathIndex - 1)
                visible: App.gotoManager.gotoPath.length > 1
                enabled: App.gotoManager.gotoPathIndex > 0
                opacity: 0.75
            }

            ToolButton {
                icon.source: "qrc:/icons/fast_forward.svg"
                font.pixelSize: App.settings.font12
                font.capitalization: Font.AllUppercase
                text: qsTr("Next")
                onClicked: App.gotoManager.setTarget(App.gotoManager.gotoTitle, App.gotoManager.gotoPath, App.gotoManager.gotoPathIndex + 1)
                visible: App.gotoManager.gotoPath.length > 1
                enabled: App.gotoManager.gotoPathIndex < App.gotoManager.gotoPath.length - 1
                opacity: 0.75
            }
        }
    }
}
