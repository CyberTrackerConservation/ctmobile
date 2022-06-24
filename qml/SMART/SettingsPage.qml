import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Settings")
    }

    property var halfWidth: (width - 20) / 2
    property var routes
    property var fontSize: App.settings.font16

    Flickable {
        anchors.fill: parent
        ScrollBar.vertical: ScrollBar { interactive: false }
        contentWidth: parent.width
        contentHeight: column.height

        ColumnLayout {
            id: column
            spacing: 0
            width: parent.width

            ItemDelegate {
                id: row0
                Layout.fillWidth: true
                contentItem: RowLayout {
                    Label {
                        Layout.preferredWidth: page.width / 3
                        font.pixelSize: page.fontSize
                        text: qsTr("Language")
                        wrapMode: Label.WordWrap
                    }

                    C.FormLanguageComboBox {
                        Layout.fillWidth: true
                        font.pixelSize: page.fontSize
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    spacing: 0
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Dark theme")
                        font.pixelSize: page.fontSize
                        wrapMode: Label.WordWrap
                    }

                    Switch {
                        id: checkTheme
                        Layout.alignment: Qt.AlignRight
                        font.pixelSize: page.fontSize
                        checked: App.settings.darkTheme
                        onClicked: App.settings.darkTheme = checkTheme.checked
                    }
                }

                onClicked: {
                    if (App.config.fullRowCheck) {
                        checkTheme.checked = !checkTheme.checked
                        checkTheme.clicked()
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                id: selectRoutes
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
                enabled: form.project.connectorParams.navigation_url !== undefined && form.project.connectorParams.api_key !== undefined
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: page.fontSize
                        wrapMode: Label.WordWrap
                        text: qsTr("Select routes")
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    busyCover.doWork = getRoutes
                    busyCover.start()
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: page.fontSize
                        text: qsTr("Change project")
                        wrapMode: Label.WordWrap
                    }

                    C.ChevronRight {}
                }

                onClicked: form.pushPage("qrc:/SMART/ProjectChangePage.qml")

                C.HorizontalDivider {}
            }

            ItemDelegate {
                id: showExports
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: page.fontSize
                        text: qsTr("Exported data")
                        wrapMode: Label.WordWrap
                    }

                    C.ChevronRight {}
                }

                onClicked: form.pushPage("qrc:/ExportsPage.qml")

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: page.fontSize
                        text: qsTr("Configuration")
                        wrapMode: Label.WordWrap
                    }

                    C.ChevronRight {}
                }

                onClicked: form.pushPage("qrc:/imports/CyberTracker.1/DetailsPage.qml", { title: qsTr("Configuration"), model: form.provider.buildConfigModel() } )

                C.HorizontalDivider {}
            }

            ItemDelegate {
                id: recoverData
                Layout.fillWidth: true
                Layout.minimumHeight: row0.height

                enabled: {
                    Globals.patrolChangeCount
                    return form.provider.hasDataServer && form.hasData() && !form.provider.patrolStarted
                }

                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: page.fontSize
                        text: qsTr("Recover Connect data")
                        wrapMode: Label.WordWrap
                    }

                    C.ChevronRight {}
                }

                onClicked: form.pushPage("qrc:/SMART/RecoverDataPage.qml")

                C.HorizontalDivider {}
            }
        }
    }

    function getRoutes() {
        var response = Utils.httpGet(form.project.connectorParams.navigation_url + "?api_key=" + form.project.connectorParams.api_key)

        switch (response.status) {
        case 200:
            page.routes = JSON.parse(response.data)
            if (page.routes.length > 0) {
                page.routes.sort(sortCompareByName)
                form.pushPage("qrc:/SMART/ConnectRoutesPage.qml", { routes: page.routes } )
                return true
            } else {
                showToast(qsTr("No routes found"))
            }

            break
        default:
            console.log("Failed to get navigation packages: " + response.status)
            break
        }

        return false
    }
}
