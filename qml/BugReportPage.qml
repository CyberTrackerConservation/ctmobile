import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property bool useSharingOnMobile: true

    QtObject {
        id: internal
        property string reportId
        property string headerTitle: page.useSharingOnMobile ? qsTr("Share report") : qsTr("Save report")
    }

    header: C.PageHeader {
        text: internal.headerTitle
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            Qt.inputMethod.hide()
            busyCover.doWork = createReport
            busyCover.start()
        }
    }

    TextArea {
        id: description

        anchors.fill: parent
        anchors.margins: 10
        focus: true
        font.pixelSize: App.settings.font17
        wrapMode: Text.Wrap
        placeholderText: {
            var line1 = qsTr("Describe the issue")
            var line2 = qsTr("Tap ✓ in the top right corner")
            var line3 = qsTr("Send the file")
            return "1." + line1 + "\n\r" + "2." + line2 + "\n\r" + "3." + line3
        }

        placeholderTextColor: Material.color(Material.Grey)
        inputMethodHints: Qt.ImhNoPredictiveText
    }

    FileDialog {
        id: fileDialog

        defaultSuffix: "zip"
        title: internal.headerTitle
        folder: shortcuts.desktop
        nameFilters: [ "Zip files (*.zip)" ]
        selectExisting: false
        onAccepted: {
            let success = Utils.copyFile(internal.reportId, Utils.urlToLocalFile(fileDialog.fileUrl))
            if (success) {
                showToast(qsTr("Report saved"))
            } else {
                showError(qsTr("Failed to copy report"))
            }

            Utils.removeFile(internal.reportId)
            appPageStack.pop()
        }

        onRejected: {
            Utils.removeFile(internal.reportId)
            appPageStack.pop()
        }
    }

    C.MessagePopup {
        id: popupReportComplete

        title: qsTr("Success")
        message: {
            let result = internal.reportId
            let i = result.indexOf("/Android/data/")
            if (i !== -1) {
                result = result.slice(i, result.length)
            }
            return result
        }

        onClicked: appPageStack.pop()
    }

    function createReport() {
        internal.reportId = App.createBugReport(description.text)
        if (internal.reportId === "") {
            showError(qsTr("Unknown error"))
            return
        }

        if (App.desktopOS) {
            fileDialog.open()
            return
        }

        if (page.useSharingOnMobile) {
            App.sendFile(internal.reportId, "Bug report")
            appPageStack.pop()
            return
        }

        popupReportComplete.open()
    }
}

