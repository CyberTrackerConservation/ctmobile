import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    C.PopupBase {
        id: popupExportComplete

        width: parent.width * 0.7
        height: width * 0.75
        property bool success
        property alias message: labelExportMessage.text

        contentItem: ColumnLayout {
            Component.onCompleted: formPageStack.setProjectColors(this)
            Label {
                Layout.fillWidth: true
                text: popupExportComplete.success ? qsTr("Success") : qsTr("Error")
                font.pixelSize: App.settings.font18
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Label.AlignHCenter
                id: labelExportMessage
                font.pixelSize: App.settings.font13
                wrapMode: Label.Wrap
            }

            RowLayout {
                spacing: 10
                Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom

                RoundButton {
                    Layout.alignment: Qt.AlignRight
                    icon.source: popupExportComplete.success ? "qrc:/icons/ok.svg" : "qrc:/icons/cancel.svg"
                    icon.color: Material.foreground
                    icon.width: 40
                    icon.height: 40
                    Material.background: popupExportComplete.success ? Material.accent : Material.color(Material.Red, Material.Shade300)
                    onClicked: popupExportComplete.close()
                }
            }
        }
    }

    header: C.PageHeader {
        text: form.project.title
        menuVisible: true
        menuEnabled: false
        menuIcon: App.batteryIcon
    }

    ColumnLayout {

        width: parent.width * 0.8
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 4

        onYChanged: {
            if (y < 0) {
                logoImage.visible = false
            }
        }

        Item {
            height: 32
        }

        Image {
            id: logoImage
            Layout.alignment: Qt.AlignHCenter
            source: form.project.icon !== "" ? App.projectManager.getFileUrl(form.project.uid, form.project.icon) : ""
            sourceSize.width: page.height * 0.25
        }

        Item {
            height: 8
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: C.Style.colorGroove
        }

        C.FormLanguageComboBox {
            Layout.fillWidth: true
        }

        TextField {
            id: userId
            Layout.fillWidth: true
            placeholderText: qsTr("Enter email or phone")
            font.pixelSize: App.settings.font16
            horizontalAlignment: TextField.AlignHCenter
            inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhLowercaseOnly
            text: form.provider.collectUser
            onDisplayTextChanged: form.provider.setCollectUser(userId.displayText)
        }

        C.ColorButton {
            id: startButton
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Start collecting")
            color: Material.accent
            font.pixelSize: App.settings.font16
            onClicked: form.pushFormPage({ projectUid: form.project.uid, stateSpace: Globals.formSpaceCollect })
            enabled: form.provider.collectUser.trim() !== ""
        }

        C.ColorButton {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Upload data")
            color: Material.color(Material.Green, Material.Shade300)
            font.pixelSize: App.settings.font16
            onClicked: {
                busyCover.doWork = completeData
                busyCover.start()
            }

            visible: {
                Globals.patrolChangeCount
                return form.hasData() && !form.provider.collectStarted
            }
        }
    }

    function completeData() {
        let success = App.taskManager.waitForTasks(form.project.uid)

        if (success) {
            form.provider.clearCompletedData()
        }

        let successMessage = qsTr("All data has been uploaded")
        let failureMessage = qsTr("Connection failed")

        popupExportComplete.success = success
        popupExportComplete.message = success ? successMessage : failureMessage
        popupExportComplete.open()

        Globals.patrolChangeCount++
    }
}
