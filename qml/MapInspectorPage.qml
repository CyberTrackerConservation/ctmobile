import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var inspectList: []
    property var currentIndex: 0
    property var currentItem: ({})

    property var forms: ({})

    Component.onCompleted: {
        page.rebuild(0)
    }

    function rebuild(indexDelta) {
        currentIndex += indexDelta
        if (currentIndex < 0 || currentIndex >= inspectList.length) {
            console.log("Error bad index")
            return
        }

        currentItem = inspectList[currentIndex]

        switch (currentItem.overlayId) {
        case "sighting":
            if (forms[currentItem.stateSpace] === undefined) {
                forms[currentItem.stateSpace] = App.createForm(form.project.uid, currentItem.stateSpace, true, page)
            }

            listViewSighting.sightingForm = forms[currentItem.stateSpace]
            listViewSighting.sightingUid = currentItem.sightingUid
            title.menuVisible = listViewSighting.sightingForm.canEditSighting(currentItem.sightingUid)
            break
        case "gotoPoints":
            title.menuVisible = false
            break
        case "gotoLines":
            title.menuVisible = false
            break
        }
    }

    C.FormSightingListView {
        id: listViewSighting
        anchors.fill: parent
        visible: page.currentItem.overlayId === "sighting"
    }

    ColumnLayout {
        id: listViewGoto
        width: page.width
        spacing: 0
        clip: true

        ItemDelegate {
            Layout.fillWidth: true
            contentItem: Label {
                text: page.currentItem.source
                font.pixelSize: App.settings.font18
                horizontalAlignment: Label.AlignHCenter
            }
        }

        ItemDelegate {
            Layout.fillWidth: true
            contentItem: Label {
                text: {
                    var location = page.currentItem.path[page.currentItem.pathIndex]
                    return location === undefined ? "" : App.getLocationText(location[0], location[1])
                }
                font.pixelSize: App.settings.font16
                horizontalAlignment: Label.AlignHCenter
            }
        }

        ItemDelegate {
            Layout.fillWidth: true
            contentItem: Label {
                text: page.currentItem.name
                font.pixelSize: App.settings.font16
                horizontalAlignment: Label.AlignHCenter
            }
        }

        visible: page.currentItem.overlayId !== "sighting"
    }

    header: C.PageHeader {
        id: title
        text: qsTr("Identify") + " - " + currentItem.name
        menuIcon: "qrc:/icons/edit.svg"
        onMenuClicked: {
            form.saveState()
            form.pushFormPage({ projectUid: form.project.uid, stateSpace: listViewSighting.sightingForm.stateSpace, editSightingUid: listViewSighting.sightingUid })
        }
    }

    Label {
        anchors.fill: parent
        text: qsTr("No selection")
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        visible: page.inspectList.length === 0
        opacity: 0.5
        font.pixelSize: App.settings.font20
    }

    footer: ColumnLayout {
        spacing: 0
        width: parent.width

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
            opacity: Material.theme === Material.Dark ? 0.12 : 0.12
        }

        RowLayout {
            id: buttonRow
            spacing: 0
            Layout.fillWidth: true
            property int buttonCount: 3
            property int buttonWidth: page.width / buttonCount
            property var buttonColor: Material.theme === Material.Dark ? Material.foreground : Material.primary

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: "Set goto"
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/arrow_goto.svg"
                Material.foreground: buttonRow.buttonColor
                onClicked: {
                    App.gotoManager.setTarget(page.currentItem.name, page.currentItem.path, page.currentItem.pathIndex)
                    if (typeof(formPageStack) !== "undefined") {
                        form.popPage()
                    } else {
                        appPageStack.pop()
                    }

                }
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Previous")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/menu_left.svg"
                Material.foreground: buttonRow.buttonColor
                enabled: page.currentIndex > 0
                onClicked: page.rebuild(-1)
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Next")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/menu_right.svg"
                Material.foreground: buttonRow.buttonColor
                enabled: page.currentIndex < page.inspectList.length - 1
                onClicked: page.rebuild(+1)
            }
        }
    }
}
