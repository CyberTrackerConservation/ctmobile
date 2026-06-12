import QtQuick
import QtQuick.Controls.Material

Item
{
    id: root

    property date now: new Date()
    property int hour: now.getHours()
    property int minute: now.getMinutes()

    ToolButton {
        id: buttonPrev
        x: parent.width - width * 2 - parent.width * 0.1
        icon.source: "qrc:/icons/chevron_left.svg"
        icon.width: Style.iconSize32
        icon.height: Style.iconSize32
        opacity: 0.5
        enabled: !watchFace.hourSelection
        onClicked: {
            watchFace.hourSelection = true
        }
    }
    ToolButton {
        id: buttonNext
        anchors.left: buttonPrev.right
        icon.source: "qrc:/icons/chevron_right.svg"
        icon.width: Style.iconSize32
        icon.height: Style.iconSize32
        opacity: 0.5
        enabled: watchFace.hourSelection
        onClicked: {
            watchFace.hourSelection = false
        }
    }

    Item {
        anchors {
            top: buttonPrev.bottom
            left: parent.left
            right: parent.right
            bottom: buttonAM.top
        }

        Item {
             id: watchFace

             anchors.centerIn: parent
             width: Math.min(parent.width, parent.height)
             height: width

             property bool hourSelection: true

             property real angleHour: 360 * (root.hour % 12) / 12.0
             property real angleMinute: 360 * (root.minute % 60) / 60.0
             property real angle: hourSelection ? angleHour : angleMinute
             Behavior on angle { SmoothedAnimation { velocity: 180.0 } }
             property int buttonSize: 24

            Rectangle {
                id: centerDot
                anchors.centerIn: watchFace
                color: Material.accent
                width: watchFace.width * 0.05
                height: width
                radius: width
            }

            Rectangle {
                id: arm
                anchors.horizontalCenter: watchFace.horizontalCenter
                antialiasing: true
                color: centerDot.color
                width: watchFace.width * 0.02
                height: watchFace.width * 0.5
                radius: width
                transform: Rotation {
                    origin.x: arm.width * 0.5
                    origin.y: arm.height
                    angle: watchFace.angle
                }
            }

            Rectangle {
                id: armEnd
                color: centerDot.color
                width: watchFace.buttonSize
                height: width
                radius: width
                x: (width - watchFace.buttonSize) / 2 + (watchFace.width - watchFace.buttonSize) * (1.0 + Math.sin((watchFace.angle / 30.0) * Math.PI / 6.0)) * 0.5
                y: (height - watchFace.buttonSize) / 2 + (watchFace.width - watchFace.buttonSize) * (1.0 - Math.cos((watchFace.angle / 30.0) * Math.PI / 6.0)) * 0.5
            }

            Repeater {
                model: 12
                anchors.centerIn: watchFace
                delegate: RoundButton {
                    width: watchFace.buttonSize
                    height: width
                    flat: true
                    radius: width
                    x: (watchFace.width - width) * (1.0 + Math.sin(model.index * Math.PI / 6.0)) * 0.5
                    y: (watchFace.width - width) * (1.0 - Math.cos(model.index * Math.PI / 6.0)) * 0.5
                    text: watchFace.hourSelection ? ((model.index === 0) ? 12 : model.index) : model.index * 5
                    font.pixelSize: App.scaleByFontSize(20)
                    font.family: "Courier New"
                    Component.onCompleted: {
                        watchFace.buttonSize = Math.max(implicitWidth * 1.1, watchFace.buttonSize)
                    }
                }
            }

            MouseArea {
                anchors.fill: watchFace
                onClicked: mouse => {
                    if (Math.pow(mouse.x - watchFace.width * 0.5, 2.0) + Math.pow(mouse.y - watchFace.width * 0.5, 2.0) < Math.pow(watchFace.width * 0.5, 2.0)) {
                        if (watchFace.hourSelection) {
                            let angleHour = 90 + 180 * (Math.atan((mouse.y - watchFace.width * 0.5) / (mouse.x - watchFace.width * 0.5)) / Math.PI) + ((mouse.x - watchFace.width * 0.5 < 0) ? 180 : 0)
                            root.hour = Math.round(12.0 * angleHour / 360.0) % 12
                            watchFace.hourSelection = false
                        } else {
                            let angleMinute = 90 + 180 * (Math.atan((mouse.y - watchFace.width * 0.5) / (mouse.x - watchFace.width * 0.5)) / Math.PI) + ((mouse.x - watchFace.width * 0.5 < 0) ? 180 : 0)
                            root.minute = Math.round(60.0 * angleMinute / 360.0) % 60
                        }
                    }
                }
            }
        }
    }

    ToolButton {
        id: buttonAM
        anchors.bottom: parent.bottom
        width: parent.width / 2
        text: Qt.locale().pmText
        enabled: root.hour < 12
        contentItem: Label {
            font.pixelSize: Math.min(parent.width, parent.height) * 0.5
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            color: root.hour < 12 ? Material.accent : Material.foreground
            text: Qt.locale().amText
        }
        onClicked: {
            root.hour = 0
        }
    }

    ToolButton {
        anchors.bottom: parent.bottom
        x: parent.width / 2
        width: x
        text: Qt.locale().pmText
        enabled: root.hour < 12
        contentItem: Label {
            font.pixelSize: Math.min(parent.width, parent.height) * 0.5
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            color: root.hour >= 12 ? Material.accent : Material.foreground
            text: Qt.locale().pmText
        }
        onClicked: {
            root.hour = 12
        }
    }
}
