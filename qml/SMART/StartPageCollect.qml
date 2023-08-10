import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: form.project.title
        menuVisible: Qt.platform.os !== "ios"
        menuIcon: App.batteryIcon
        onMenuClicked: App.showToast(App.batteryText)
    }

    ColumnLayout {

        width: parent.width * 0.8
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: App.scaleByFontSize(4)

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
            Layout.preferredHeight: C.Style.minRowHeight
        }

        TextField {
            id: userId
            Layout.fillWidth: true
            placeholderText: qsTr("Enter email or phone")
            font.pixelSize: App.settings.font16
            horizontalAlignment: TextField.AlignHCenter
            inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhPreferLowercase
            text: form.provider.collectUser
        }

        C.ColorButton {
            id: startButton
            Layout.fillWidth: true
            Layout.preferredHeight: Style.minRowHeight
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Start collecting")
            font.capitalization: Font.MixedCase
            color: Material.accent
            font.pixelSize: App.settings.font16
            onClicked: {
                form.provider.setCollectUser(userId.text.trim())
                form.pushFormPage({ projectUid: form.project.uid, stateSpace: Globals.formSpaceCollect })
            }
            enabled: userId.text.trim() !== ""
        }

        C.ColorButton {
            Layout.fillWidth: true
            Layout.preferredHeight: Style.minRowHeight
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Upload data")
            font.capitalization: Font.MixedCase
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

    C.MessagePopup {
        id: popupExportComplete
    }

    function completeData() {
        let success = App.taskManager.waitForTasks(form.project.uid)

        if (success) {
            form.provider.clearCompletedData()
        }

        let successMessage = qsTr("All data has been uploaded.")
        let failureMessage = qsTr("Connection failed. Please try again later.")

        popupExportComplete.icon = success ? "qrc:/icons/cloud_done.svg" : "qrc:/icons/cloud_off_outline.svg"
        popupExportComplete.title = success ? qsTr("Success") : qsTr("Error")
        popupExportComplete.message1 = success ? successMessage : failureMessage
        popupExportComplete.open()

        Globals.patrolChangeCount++
    }
}
