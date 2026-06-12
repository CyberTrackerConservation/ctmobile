import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    id: page

    QtObject {
        id: internal
        property int groupBy: 0
        property bool sortReverse: false
    }

    header: C.PageHeader {
        text: qsTr("Group by")

        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            Qt.inputMethod.hide()
            App.settings.sightingGroupBy = internal.groupBy
            App.settings.sightingSortReverse = internal.sortReverse
            popPageImmediate(page)
        }
    }

    Component.onCompleted: {
        internal.groupBy = App.settings.sightingGroupBy
        internal.sortReverse = App.settings.sightingSortReverse
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
                        text: qsTr("Group by")
                        font.pixelSize: App.settings.font14
                        font.capitalization: Font.MixedCase
                        font.bold: false
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        id: comboGroupBy
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pixelSize: App.settings.font14
                        textRole: "text"
                        valueRole: "value"
                        model: [
                            { text: qsTr("None"), value: C.SightingListProxyModel.None },
                            { text: App.alias_Project, value: C.SightingListProxyModel.ProjectTitle },
                            { text: qsTr("Patrol or survey"), value: C.SightingListProxyModel.GroupText },
                            { text: qsTr("Day"), value: C.SightingListProxyModel.Day },
                            { text: qsTr("Month"), value: C.SightingListProxyModel.Month },
                            { text: qsTr("Year"), value: C.SightingListProxyModel.Year },
                        ]

                        Component.onCompleted: {
                            currentIndex = indexOfValue(internal.groupBy)
                        }

                        onActivated: function (index) {
                            internal.groupBy = currentValue
                        }

                        delegate: ItemDelegate {
                            width: comboGroupBy.width
                            font.pixelSize: App.settings.font14
                            text: modelData.text
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
                        text: qsTr("Sort order")
                        font.pixelSize: App.settings.font14
                        font.capitalization: Font.MixedCase
                        font.bold: false
                        wrapMode: Label.WordWrap
                    }

                    ComboBox {
                        id: comboSortOrder
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pixelSize: App.settings.font14
                        textRole: "text"
                        valueRole: "value"
                        model: [
                            { text: qsTr("Oldest first"), value: false },
                            { text: qsTr("Newest first"), value: true },
                        ]

                        Component.onCompleted: {
                            currentIndex = indexOfValue(internal.sortReverse)
                        }

                        onActivated: function (index) {
                            internal.sortReverse = currentValue
                        }

                        delegate: ItemDelegate {
                            width: comboSortOrder.width
                            font.pixelSize: App.settings.font14
                            text: modelData.text
                        }
                    }
                }

                C.HorizontalDivider {}
            }
        }
    }
}
