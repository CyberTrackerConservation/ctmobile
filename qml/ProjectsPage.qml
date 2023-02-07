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
        text: qsTr("Projects")
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
                        text: qsTr("Delete project?")
                        leftPadding: 20
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
                if (d.swipe.position !== 0) {
                    return
                }

                if (internal.launchMutex) {
                    return
                }

                internal.launchMutex = true
                try {
                    popupConfirmDelete.close()
                    App.clearComponentCache()

                    // Request permissions (for Android).
                    if (!App.requestPermissions(modelData.uid)) {
                        return
                    }

                    // Send telemetry.
                    App.telemetry.aiSendEvent("projectStart", { "connector": modelData.connector, "telemetry": modelData.telemetry })

                    // Ask for update on launch.
                    if (modelData.hasUpdate && modelData.loggedIn && App.updateAllowed()) {
                        popupConfirmUpdate.open({ projectUid: modelData.uid, projectTitle: modelData.title })
                        return
                    }

                    // Ensure valid provider.
                    if (modelData.provider === "") {
                        App.showError(qsTr("No provider"))
                        return
                    }

                    // Launch.
                    appPageStack.push(FormViewUrl, { projectUid: modelData.uid })

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

    C.PopupLoader {
        id: popupConfirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                property string projectUid
                property string projectTitle

                text: qsTr("Delete project?")
                subText: qsTr("The project and all of its data will be permanently removed.")
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

                text: qsTr("Update project?")
                subText: qsTr("A new version is available.")
                confirmText: qsTr("Yes, update now")
                confirmDelay: false
                cancelText: qsTr("No, update later")
                onConfirmed: {
                    busyCover.doWork = () => updateThenLaunch(projectUid)
                    busyCover.start()
                }

                onCancel: {
                    appPageStack.push(FormViewUrl, { projectUid: projectUid })
                }
            }
        }
    }

    function updateThenLaunch(projectUid) {
        if (Utils.networkAvailable()) {
            App.projectManager.update(projectUid, true)
        }

        appPageStack.push(FormViewUrl, { projectUid: projectUid })
    }
}
