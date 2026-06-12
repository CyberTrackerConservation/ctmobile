import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    id: page

    C.SightingFilter {
        id: sightingFilter
    }

    header: C.PageHeader {
        text: qsTr("Sighting filter")

        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            Qt.inputMethod.hide()
            sightingFilter.searchText = searchText.text
            App.settings.sightingFilter = sightingFilter.save()
            popPageImmediate(page)
        }
    }

    Component.onCompleted: {
        sightingFilter.load(App.settings.sightingFilter)
    }

    Flickable {
        anchors.fill: parent
        ScrollBar.vertical: ScrollBar { interactive: false }
        contentWidth: parent.width
        contentHeight: column.height

        ColumnLayout {
            id: column
            width: parent.width
            spacing: 0

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Source")
                        font.pixelSize: App.settings.font14
                        font.capitalization: Font.MixedCase
                        font.bold: false
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        id: comboSource
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pixelSize: App.settings.font14
                        textRole: "text"
                        valueRole: "value"
                        model: [
                            { text: qsTr("All data"), value: 0, sourceArchive: true, sourceProject: true },
                            { text: qsTr("Archive only"), value: 1, sourceArchive: true, sourceProject: false },
                            { text: qsTr("%1 only").arg(App.alias_Projects), value: 2, sourceArchive: false, sourceProject: true }
                        ]
                        Component.onCompleted: {
                            for (let i = 0; i < model.length; i++) {
                                if (model[i].sourceArchive === sightingFilter.sourceArchive &&
                                    model[i].sourceProject === sightingFilter.sourceProject) {
                                    currentIndex = i
                                    return
                                }
                            }

                            currentIndex = 0
                        }

                        onActivated: function (index) {
                            sightingFilter.sourceArchive = model[currentValue].sourceArchive
                            sightingFilter.sourceProject = model[currentValue].sourceProject
                        }

                        delegate: ItemDelegate {
                            width: comboSource.width
                            contentItem: Label {
                                text: modelData.text
                                font.pixelSize: App.settings.font14
                                wrapMode: Text.WordWrap
                                width: parent.width
                                verticalAlignment: Qt.AlignVCenter
                            }
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Date range")
                        font.pixelSize: App.settings.font14
                        font.capitalization: Font.MixedCase
                        font.bold: false
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        id: comboDateRange
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pixelSize: App.settings.font14
                        textRole: "text"
                        valueRole: "value"
                        model: [
                            { text: qsTr("All"), value: 0 },
                            { text: qsTr("Today"), value: -1 },
                            { text: qsTr("Last 7 days"), value: -7 },
                            { text: qsTr("Last 30 days"), value: -30 },
                            { text: qsTr("Last 90 days"), value: -90 },
                            { text: qsTr("Last year"), value: -365 }
                        ]

                        Component.onCompleted: currentIndex = indexOfValue(sightingFilter.dateRangeDays)

                        onActivated: function (index) {
                            sightingFilter.dateRangeDays = currentValue
                        }

                        delegate: ItemDelegate {
                            width: comboDateRange.width
                            contentItem: Label {
                                text: modelData.text
                                font.pixelSize: App.settings.font14
                                wrapMode: Text.WordWrap
                                width: parent.width
                                verticalAlignment: Qt.AlignVCenter
                            }
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    TextField {
                        id: searchText
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search text...")
                        font.pixelSize: App.settings.font14
                        text: sightingFilter.searchText
                    }

                    ToolButton {
                        icon.width: C.Style.toolButtonSize
                        icon.height: C.Style.toolButtonSize
                        icon.source: "qrc:/icons/cancel.svg"
                        opacity: 0.5
                        onClicked: {
                            searchText.text = ""
                        }
                        enabled: searchText.text !== ""
                    }
                }

                C.HorizontalDivider {
                    height: C.Style.lineWidth2
                }
            }
        }
    }
}
