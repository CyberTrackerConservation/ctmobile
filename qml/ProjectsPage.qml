import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: App.alias_Projects
        backVisible: false
    }

    QtObject {
        id: internal

        // If launching takes too long, it can cause longpress to fire.
        property bool launchMutex: false
    }

    C.ListViewV {
        id: listView
        anchors.fill: parent

        model: C.ProjectListModel {
            id: projectListModel
        }

        footer: Item {
            width: parent.width
            height: C.Style.iconSize64
        }

        currentIndex: -1

        delegate: SwipeDelegate {
            id: d
            width: ListView.view.width
            checkable: true
            checked: swipe.complete
            onCheckedChanged: if (!checked) swipe.close()

            swipe.right: Rectangle {
                width: parent.width
                height: parent.height
                color: C.Style.colorGroove

                RowLayout {
                    anchors.fill: parent

                    Label {
                        text: qsTr("Delete %1?").arg(App.alias_project)
                        leftPadding: App.scaleByFontSize(20)
                        font.pixelSize: App.settings.font16
                        fontSizeMode: Label.Fit
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                    }

                    ToolButton {
                        flat: true
                        text: qsTr("Yes")
                        onClicked: {
                            d.swipe.close()
                            popupConfirmDelete.open({ projectUid: modelData.uid, projectTitle: modelData.title })
                        }
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: parent.height
                        Layout.maximumWidth: parent.width / 5
                        font.pixelSize: App.settings.font14
                    }

                    ToolButton {
                        flat: true
                        text: qsTr("No")
                        onClicked: d.swipe.close()
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: parent.height
                        Layout.maximumWidth: parent.width / 5
                        font.pixelSize: App.settings.font14
                    }
                }
            }

            contentItem: C.ListRow {
                property bool hasUpdate: modelData.hasUpdate && modelData.loggedIn
                property bool hasAnnouncements: modelData.announcements.length > 0
                iconSource: {
                    let result = App.settings.darkTheme && modelData.iconDark !== "" ? modelData.iconDark : modelData.icon
                    return result !== "" ? App.projectManager.getFileUrl(modelData.uid, result) : ""
                }
                text: modelData.title
                subText: modelData.subtitle
                toolButtonVisible: hasUpdate || hasAnnouncements
                toolButtonIconSource: hasUpdate ? "qrc:/icons/autorenew.svg" : "qrc:/icons/message_alert_outline.svg"
                onToolButtonClicked: {
                    if (hasUpdate) {
                        if (!App.updateAllowed()) {
                            showError(qsTr("Offline"))
                            return
                        }

                        appWindow.updateProject(modelData.uid)
                        return
                    }

                    if (hasAnnouncements) {
                        popupAnnouncements.open({ announcements: modelData.announcements, announcementIndex: 0 })
                        return
                    }
                }
            }

            Rectangle {
                id: redDot
                color: modelData.hasStreamers ? "transparent" : "orange"
                x: App.scaleByFontSize(4)
                y: x
                radius: modelData.hasStreamers ? width / 2 : 0
                width: parent.height / 8
                height: width
                visible: {
                    if (d.swipe.position !== 0) {
                        return false
                    }

                    if (App.settings.patrolProjectUid === modelData.uid) {
                        return true
                    }

                    if (modelData.hasStreamers) {
                        return true
                    }

                    return false
                }

                Timer {
                    interval: 750
                    running: redDot.visible && modelData && modelData.hasStreamers
                    repeat: true
                    onTriggered: {
                        var c = App.locationLogger.stateColor
                        redDot.color = Qt.rgba(c.r, c.g, c.b, redDot.color.a ^ 1)
                    }
                }
            }

            C.HorizontalDivider {}

            onClicked: {
                popupConfirmDelete.close()

                if (d.swipe.position !== 0) {
                    return
                }

                if (internal.launchMutex) {
                    return
                }

                internal.launchMutex = true
                try {
                    // Ask for update on launch.
                    if (modelData.hasUpdate && modelData.loggedIn && App.updateAllowed()) {
                        popupConfirmUpdate.open({ projectUid: modelData.uid, projectTitle: modelData.title })
                        return
                    }

                    // Send telemetry.
                    App.telemetry.aiSendEvent("projectStart", { "connector": modelData.connector, "telemetry": modelData.telemetry, "title": modelData.title, "subtitle": modelData.subtitle, "usage": App.telemetry.getUsage(modelData.uid) })

                    // Log to event log.
                    App.auditAdd(modelData.title, qsTr("Open %1").arg(App.alias_Project))

                    changeToProject(modelData.uid, StackView.PushTransition)

                } finally {
                    internal.launchMutex = false
                }
            }

            onPressAndHold: {
                if (internal.launchMutex || busyCover.visible) {
                    return
                }

                appPageStack.push(Qt.resolvedUrl("ProjectDetailsPage.qml"), { project: modelData } )
            }
        }
    }

    Pane {
        id: pane
        anchors.fill: parent
        visible: projectListModel.count === 0

        Material.background: C.Style.colorContent
        topPadding: flickable.contentHeight < parent.height ? (parent.height - flickable.contentHeight) / 2 : App.scaleByFontSize(8)
        leftPadding: parent.width * 0.1
        rightPadding: parent.width * 0.1

        contentItem: Flickable {
            id: flickable

            flickableDirection: Flickable.VerticalFlick
            contentHeight: column.height
            contentWidth: column.width

            ColumnLayout {
                id: column
                width: pane.width * 0.8
                spacing: App.scaleByFontSize(16)
                opacity: 0.5

                Label {
                    Layout.fillWidth: true
                    font.pixelSize: App.settings.font18
                    font.bold: true
                    text: qsTr("No %1").arg(App.alias_projects)
                    horizontalAlignment: Label.AlignHCenter
                    wrapMode: Label.WordWrap
                }

                ItemDelegate {
                    Layout.fillWidth: true
                    contentItem: Label {
                        font.pixelSize: App.settings.font18
                        text: {
                            let result = ""
                            let scanQRCode = "**" + qsTr("Scan QR code") + "**"
                            if (addButton.visible) {
                                result = qsTr("%1 or tap the %2 button below to add a %3").arg(scanQRCode).arg("**+**").arg(App.alias_Project)
                            } else if (scanButton.visible) {
                                result = qsTr("%1 to add a %2").arg(scanQRCode).arg(App.alias_Project)
                            }

                            return result
                        }
                        textFormat: Label.MarkdownText
                        horizontalAlignment: Label.AlignHCenter
                        verticalAlignment: Label.AlignVCenter
                        wrapMode: Label.WordWrap
                        elide: Label.ElideRight
                    }
                    onClicked: {
                        pushConnectQRCodePage()
                    }
                }

                ToolButton {
                    Layout.alignment: Qt.AlignHCenter
                    font.pixelSize: App.settings.font14
                    icon.source: "qrc:/icons/help.svg"
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    text: qsTr("Getting started")
                    font.capitalization: Font.AllUppercase
                    visible: App.config.helpUrl !== ""
                    onClicked: Qt.openUrlExternally(App.config.helpUrl)
                }
            }
        }
    }

    RowLayout {
        anchors {
            right: parent.right
            rightMargin: App.scaleByFontSize(8)
            bottom: listView.bottom
            bottomMargin: App.scaleByFontSize(8)
        }

        C.CircleButton {
            id: scanButton
            radius: width / 12
            text: qsTr("Scan") + " "
            font.pixelSize: App.settings.font14
            font.capitalization: Font.MixedCase
            icon.source: "qrc:/icons/qrcode.svg"
            icon.width: C.Style.iconSize40
            icon.height: C.Style.iconSize40
            visible: App.config.showScanButton
            onClicked: {
                if (appWindow.videoMode) {
                    App.runLinkOnClipboard()
                    return
                }

                pushConnectQRCodePage()
            }
        }

        C.CircleButton {
            id: addButton
            icon.source: "qrc:/icons/plus.svg"
            icon.width: C.Style.iconSize40
            icon.height: C.Style.iconSize40
            visible: !App.config.showConnectPage && (App.config.connectPane !== "")
            onClicked: {
                appPageStack.push("qrc:/ConnectPage.qml")
            }
        }
    }

    C.PopupLoader {
        id: popupConfirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                property string projectUid
                property string projectTitle

                text: qsTr("Delete %1?").arg(App.alias_project)
                subText: qsTr("The %1 and all of its data will be permanently removed.").arg(App.alias_project)
                confirmText: qsTr("Yes, delete it")
                confirmDelay: true
                onConfirmed: {
                    App.backupDatabase("Project delete: " + projectUid)
                    App.projectManager.remove(projectUid)
                    showToast(qsTr("%1 deleted").arg("'" + projectTitle + "'"))
                }
            }
        }
    }

    C.PopupLoader {
        id: popupConfirmUpdate

        popupComponent: Component {
            C.ConfirmPopup {
                property string projectUid
                property string projectTitle

                text: qsTr("Update %1?").arg(App.alias_project)
                subText: qsTr("A new version is available.")
                confirmText: qsTr("Yes, update now")
                confirmDelay: false
                cancelText: qsTr("No, update later")
                onConfirmed: {
                    busyCover.doWork = () => updateThenLaunch(projectUid)
                    busyCover.start()
                }

                onCancel: {
                    changeToProject(projectUid, StackView.PushTransition)
                }
            }
        }
    }

    C.PopupLoader {
        id: popupAnnouncements

        popupComponent: Component {
            C.MessagePopup {
                property var announcements: []
                property int announcementIndex: 0

                icon: "qrc:/icons/message_alert_outline.svg"
                title: qsTr("Annoucement")
                message1: announcements[announcementIndex].message
                message2: " "
                buttons: [{ icon: "qrc:/icons/chevron_right.svg", text: qsTr("Next"), enabled: announcementIndex < announcements.length - 1 },
                          { icon: "qrc:/icons/ok.svg", text: qsTr("OK") }]
                onClicked: (index) => {
                    if (index === 0) {
                        popupAnnouncements.open({ announcements: announcements, announcementIndex: announcementIndex + 1 })
                    }
                }
            }
        }
    }

    function updateThenLaunch(projectUid) {
        if (Utils.networkAvailable()) {
            App.projectManager.update(projectUid, true)
        }

        changeToProject(projectUid, StackView.PushTransition)
    }
}
