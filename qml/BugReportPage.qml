import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import CyberTracker as C

C.ContentPage {
    id: page

    property bool useSharingOnMobile: true

    QtObject {
        id: internal
        property string reportId
        property string headerTitle: page.useSharingOnMobile ? qsTr("Share bug report") : qsTr("Save bug report")
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
        inputMethodHints: Qt.ImhNoPredictiveText

        Label {
            font.pixelSize: App.settings.font17
            anchors.fill: parent
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            text: {
                var line1 = qsTr("Describe the issue")
                var line2 = qsTr("Tap ✓ in the top right corner")
                var line3 = qsTr("Send the file")
                return line1 + "\n\r" + line2 + "\n\r" + line3
            }
            visible: description.text === ""
            color: Material.color(Material.Grey)
        }
    }

    FileDialog {
        id: fileDialog

        defaultSuffix: "zip"
        title: internal.headerTitle
        currentFolder: StandardPaths.writableLocation(StandardPaths.DesktopLocation)
        nameFilters: [ "Zip files (*.zip)" ]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            let success = Utils.copyFile(internal.reportId, Utils.urlToLocalFile(fileDialog.selectedFile))
            if (success) {
                showToast(qsTr("Report saved"))
            } else {
                showError(qsTr("Failed to copy report"))
            }

            Utils.removeFile(internal.reportId)
            popPageImmediate(page)
        }

        onRejected: {
            Utils.removeFile(internal.reportId)
            popPageImmediate(page)
        }
    }

    C.MessagePopup {
        id: popupReportComplete

        title: qsTr("Success")
        message1: qsTr("Download/BugReports")
        onClicked: {
            popPageImmediate(page)
        }
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
            popPageImmediate(page)
            return
        }

        if (App.copyToDownload(internal.reportId, "BugReports") !== "") {
            popupReportComplete.open()
        } else {
            showError(qsTr("Failed to copy bug report"))
        }
    }
}
