import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtQuick.Dialogs 1.2
import CyberTracker 1.0 as C

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

        currentIndex: -1

        ButtonGroup {
            buttons: listView.contentItem.children
        }

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
                iconSource: {
                    let result = App.settings.darkTheme && modelData.iconDark !== "" ? modelData.iconDark : modelData.icon
                    return result !== "" ? App.projectManager.getFileUrl(modelData.uid, result) : ""
                }
                text: modelData.title
                subText: modelData.subtitle
                toolButton: modelData.hasUpdate && modelData.loggedIn
                toolButtonIconSource: "qrc:/icons/autorenew.svg"
                onToolButtonClicked: {
                    if (!App.updateAllowed()) {
                        showError(qsTr("Offline"))
                        return
                    }

                    appWindow.updateProject(modelData.uid)
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
                    App.telemetry.aiSendEvent("projectStart", { "connector": modelData.connector, "telemetry": modelData.telemetry })

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
                        appPageStack.push("qrc:/ConnectQRCodePage.qml")
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

    C.CircleButton {
        id: addButton
        anchors {
            right: parent.right
            rightMargin: App.scaleByFontSize(8)
            bottom: listView.bottom
            bottomMargin: App.scaleByFontSize(8)
        }
        icon.source: "qrc:/icons/plus.svg"
        icon.width: C.Style.iconSize48
        icon.height: C.Style.iconSize48
        visible: !App.config.showConnectPage && (App.config.connectPane !== "")
        onClicked: {
            appPageStack.push("qrc:/ConnectPage.qml")
        }
    }

    C.CircleButton {
        id: scanButton
        anchors {
            right: parent.right
            rightMargin: App.scaleByFontSize(8)
            bottom: listView.bottom
            bottomMargin: App.scaleByFontSize(8)
        }
        radius: width / 8
        icon.source: "qrc:/icons/qrcode_scan.svg"
        icon.width: C.Style.iconSize48
        icon.height: C.Style.iconSize48
        visible: !App.config.showConnectPage && (App.config.connectPane === "")
        onClicked: {
            if (appWindow.videoMode) {
                App.runLinkOnClipboard()
                return
            }

            appPageStack.push("qrc:/ConnectQRCodePage.qml")
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

    function updateThenLaunch(projectUid) {
        if (Utils.networkAvailable()) {
            App.projectManager.update(projectUid, true)
        }

        changeToProject(projectUid, StackView.PushTransition)
    }
}
