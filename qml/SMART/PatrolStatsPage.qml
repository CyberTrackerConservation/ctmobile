import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        id: headerBar
        text: qsTr("Statistics")
    }

    ListModel {
        id: listModel

        function rebuild() {
            let patrolForm = form.provider.patrolForm ? form : App.createForm(form.project.uid, Globals.formSpacePatrol, true, page)
            let legs = patrolForm.provider.patrolLegs
            if (legs.length !== listModel.count) {
                for (let i = 0; i < legs.length; i++) {
                    listModel.append({})
                }
            }

            for (let i = 0; i < legs.length; i++) {
                listModel.set(i, patrolForm.provider.getPatrolLeg(i))
            }
        }
    }

    Component.onCompleted: {
        listModel.rebuild()
    }

    Connections {
        target: form.provider
        function onPatrolLegsChanged() {
            listModel.rebuild()
        }
    }

    C.ListViewV {
        anchors.fill: parent

        model: listModel
        delegate: ItemDelegate {
            width: parent.width
            contentItem: GridLayout {
                id: grid
                columns: 3
                columnSpacing: 0
                rowSpacing: 8
                property var buttonWidth: (grid.width - 16) / 2
                property var color: model.active ? (App.settings.darkTheme ? "lightsteelblue" : "steelblue") : Material.foreground

                // Leg id
                Label {
                    Layout.fillWidth: true
                    Layout.columnSpan: 3
                    horizontalAlignment: Label.AlignHCenter
                    font.underline: true
                    font.bold: true
                    font.pixelSize: App.settings.font16
                    color: grid.color
                    text: model.id
                }

                // Date.
                Label {
                    Layout.preferredWidth: grid.buttonWidth
                    color: grid.color
                    text: qsTr("Start date")
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignRight
                }

                Item { width: 16 }

                Label {
                    Layout.preferredWidth: grid.buttonWidth
                    color: grid.color
                    text: model.startDate
                    font.pixelSize: App.settings.font14
                    font.bold: true
                }

                // Time.
                Label {
                    Layout.preferredWidth: grid.buttonWidth
                    color: grid.color
                    text: qsTr("Start time")
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignRight
                }

                Item { width: 16 }

                Label {
                    Layout.preferredWidth: grid.buttonWidth
                    color: grid.color
                    text: model.startTime
                    font.pixelSize: App.settings.font14
                    font.bold: true
                }

                // Distance.
                Label {
                    Layout.preferredWidth: grid.buttonWidth
                    color: grid.color
                    text: qsTr("Distance")
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignRight
                }

                Item { width: 16 }

                Label {
                    Layout.preferredWidth: grid.buttonWidth
                    color: grid.color
                    text: model.distance
                    font.pixelSize: App.settings.font14
                    font.bold: true
                }

                // Average speed.
                Label {
                    Layout.preferredWidth: grid.buttonWidth
                    color: grid.color
                    text: qsTr("Average speed")
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignRight
                }

                Item { width: 16 }

                Label {
                    Layout.preferredWidth: grid.buttonWidth
                    color: grid.color
                    text: model.speed
                    font.pixelSize: App.settings.font14
                    font.bold: true
                }

                // Rate.
                Label {
                    Layout.preferredWidth: grid.buttonWidth
                    color: grid.color
                    text: qsTr("Track recorder")
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignRight
                }

                Item { width: 16 }

                Label {
                    Layout.preferredWidth: grid.buttonWidth
                    color: grid.color
                    text: model.rate
                    font.pixelSize: App.settings.font14
                    font.bold: true
                }
            }

            C.HorizontalDivider {
                visible: model.active
            }
        }
    }
}
