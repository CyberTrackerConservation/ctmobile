import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item
{
    id: root

    property date now: new Date()
    property int year: now.getFullYear()
    property int month: now.getMonth()
    property int day: now.getDate()

    clip: true

    // titleRow
    Item {
        id: titleRow
        width: parent.width
        height: parent.height * 0.2

        Button {
            id: buttonDate
            anchors {
                left: parent.left
                leftMargin: parent.width * 0.05
                verticalCenter: parent.verticalCenter
            }
            width: Math.min(parent.width - buttonPrev.width - buttonNext.width - x * 2, dateLabel.paintedWidth + downArrow.width + padding * 2 + 4)
            height: parent.height
            flat: true
            contentItem: Item {
                clip: true

                Label {
                    id: dateLabel
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    width: titleRow.width - titleRow.width * 0.2 - downArrow.width - buttonPrev.width - buttonNext.width
                    text: (new Date(root.year, root.month, root.day)).toLocaleDateString(App.locale, "MMM yyyy")
                    font.pixelSize: App.scaleByFontSize(30)
                    fontSizeMode: Text.Fit
                    verticalAlignment: Text.AlignVCenter
                }

                SquareIcon {
                    id: downArrow
                    x: dateLabel.paintedWidth + 4
                    anchors {
                        verticalCenter: parent.verticalCenter
                    }
                    source: !calendarView.visible ? "qrc:/icons/chevron_up.svg" : "qrc:/icons/chevron_down.svg"
                    size: Style.iconSize32
                    opacity: 0.5
                }
            }
            onClicked: {
                calendarView.visible = !calendarView.visible
            }
        }

        ToolButton {
            id: buttonPrev
            anchors {
                right : buttonNext.left
                verticalCenter: buttonDate.verticalCenter
            }
            height: parent.height
            icon.source: "qrc:/icons/chevron_left.svg"
            icon.width: Style.iconSize48
            icon.height: Style.iconSize48
            opacity: 0.5
            visible: calendarView.visible
            onClicked: {
                let m = (root.month + 11) % 12;
                if (m === 11) {
                    root.year--
                    monthView.setCurrentIndex(11);
                } else {
                    monthView.decrementCurrentIndex();
                }
                root.month = m
            }
        }

        ToolButton {
            id: buttonNext
            anchors {
                right: parent.right
                rightMargin: parent.width * 0.05
                verticalCenter: buttonPrev.verticalCenter
            }
            height: parent.height
            icon.source: "qrc:/icons/chevron_right.svg"
            icon.width: Style.iconSize48
            icon.height: Style.iconSize48
            opacity: 0.5
            visible: calendarView.visible
            onClicked: {
                let m = (root.month + 1) % 12;
                if (m === 0) {
                    root.year++
                    monthView.setCurrentIndex(0);
                } else {
                    monthView.incrementCurrentIndex();
                }
                root.month = m
            }
        }
    }

    // calendarView
    Item {
        id: calendarView
        anchors.top: titleRow.bottom
        width: parent.width
        height: parent.height * 0.80

        DayOfWeekRow {
            id: dayOfWeekRow
            anchors {
                left: parent.left
                right: parent.right
            }

            height: parent.height * 0.20

            property int bestFontPixelSize: Utils.computeDayOfWeekFontSize(App.locale, width, font.family, font.bold)

            delegate: Label {
                text: model.shortName
                width: dayOfWeekRow.width / 7
                clip: true
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: dayOfWeekRow.bestFontPixelSize
                font.bold: true
                opacity: 0.75
            }
        }

        SwipeView {
            id: monthView
            anchors {
                top: dayOfWeekRow.bottom
                topMargin: 8
                left: dayOfWeekRow.left
                right: dayOfWeekRow.right
                bottom: calendarView.bottom
                bottomMargin: 8
            }

            currentIndex: root.month
            interactive: false

            Repeater {
                model: 12
                MonthGrid {
                    id: monthGrid
                    year: root.year
                    month: index

                    property int fontPixelSize: Utils.computeNumberFontSize(width / 7, 8888, font.family, font.bold)

                    delegate: RoundButton {
                        horizontalPadding: 0
                        verticalPadding: 0
                        radius: width
                        text: model.day
                        flat: true
                        font.pixelSize: monthGrid.fontPixelSize
                        highlighted: model.month === root.month && model.year === root.year && model.day === root.day
                        opacity: model.month === monthGrid.month ? 1 : 0.2
                        onClicked: {
                            root.year = model.year
                            root.month = model.month
                            root.day = model.day
                        }
                    }
                }
            }
        }
    }

    GridView {
        id: yearView
        anchors.fill: calendarView
        visible: !calendarView.visible
        cellWidth: width / 3
        cellHeight: height * 0.1666667
        clip: true
        ScrollBar.vertical: ScrollBar {}

        property int fontPixelSize: Utils.computeNumberFontSize(cellWidth / 2, 9999, font.family, font.bold)

        Component.onCompleted: {
            for (let i = 0; i < 200; i++) {
                yearModel.append({ "year": 1900 + i });
            }
            positionViewAtIndex(root.year - 1900, GridView.Beginning)
        }

        model: ListModel {
            id: yearModel
        }

        delegate: RoundButton {
            width: yearView.cellWidth
            height: yearView.cellHeight
            text: model.year
            flat: true
            highlighted: model.year === root.year
            font.pixelSize: yearView.fontPixelSize
            radius: Math.min(height, width) * 0.5
            onClicked: {
                root.year = model.year
                calendarView.visible = true
            }
        }
    }
}
