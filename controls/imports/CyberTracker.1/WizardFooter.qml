import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

ColumnLayout {
    id: root

    property string recordUid
    property string fieldUid
    property var params

    spacing: 0
    clip: true
    visible: !getParam(params, "hidden", false)
    height: visible ? implicitHeight : 0

    QtObject {
        id: internal

        property bool clickReady: false
        property var buttonColor: getParamColor(params, "buttonColor")
        property int buttonCount: buttons.length
        property int buttonWidth: root.width / buttonCount

        property var buttonScale: {
            let result = getParam(params, "buttonScale", 1.0)
            if (result !== 1.0) {
                let maxButtonWidth = root.width / 6
                if (Style.toolButtonSize * result > maxButtonWidth) {
                    result = maxButtonWidth / Style.toolButtonSize
                }
            }
            return result
        }
    }

    Rectangle {
        Layout.fillWidth: true
        height: Style.lineWidth2
        color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
        opacity: 0.12
    }

    property var buttons: {
        if (form.editing) {
            return [ "back", "index", "next" ]
        }

        let buttonList = getParam(params, "buttons", "")
        if (buttonList !== "") {
            let result = Utils.stringToList(buttonList)
            if (result.length !== 0) {
                return result
            }
        }

        if (form.project.immersive) {
            return [ "back", "options", "nextOrSave" ]
        }

        return [ "back", "index", "nextOrSave" ]
    }

    RowLayout {
        spacing: 0
        Layout.fillWidth: true

        Repeater {
            model: root.buttons

            Loader {
                Layout.minimumWidth: internal.buttonWidth
                Layout.fillWidth: true
                sourceComponent: {
                    switch (modelData.toLowerCase()) {
                    case "space":
                        return emptySpaceComponent

                    case "home":
                        return homeButtonComponent

                    case "back":
                        return backButtonComponent

                    case "next":
                        return nextButtonComponent

                    case "save":
                        return saveButtonComponent

                    case "nextorsave":
                        return nextOrSaveButtonComponent

                    case "index":
                        return indexButtonComponent

                    case "options":
                        return form.wizard.immersive ? optionsButtonComponent : indexButtonComponent

                    case "map":
                        return mapButtonComponent
                    }

                    return emptySpaceComponent
                }
            }
        }
    }

    Timer {
        interval: 250
        repeat: false
        running: true
        onTriggered: {
            internal.clickReady = true
        }
    }

    Component {
        id: emptySpaceComponent

        Item {
            width: internal.buttonWidth
            height: 16
        }
    }

    // Home.
    Component {
        id: homeButtonComponent

        FooterButton {
            separatorTop: false
            icon.source: findIcon("home", Style.homeIconSource)
            extraScale: internal.buttonScale
            colorOverride: internal.buttonColor
            onClicked: {
                if (!page.canHome()) {
                    return
                }

                if (form.wizard.immersive && form.editing) {
                    form.popPagesToParent()
                    return
                }

                if (!form.wizard.immersive && !form.wizard.autoSave && form.hasRecordChanged(form.wizard.recordUid)) {
                    popupConfirmHome.open()
                    return
                }

                form.wizard.home()
            }
        }
    }

    // Back.
    Component {
        id: backButtonComponent

        FooterButton {
            enabled: form.wizard.canBack
            opacity: enabled ? 1.0 : 0
            separatorTop: false
            icon.source: findIcon("back", Style.backIconSource)
            extraScale: internal.buttonScale
            colorOverride: internal.buttonColor
            onClicked: {
                if (page.canBack()) {
                    form.wizard.back()
                }
            }
        }
    }

    // Next.
    Component {
        id: nextButtonComponent

        FooterButton {
            enabled: !form.wizard.lastPage
            opacity: enabled ? 1.0 : 0
            separatorTop: false
            icon.source: findIcon("next", Style.nextIconSource)
            extraScale: internal.buttonScale
            colorOverride: internal.buttonColor
            onClicked: {
                if (!internal.clickReady) {
                    return
                }

                if (page.canNext()) {
                    form.wizard.next(root.recordUid, root.fieldUid)
                }
            }
        }
    }

    // Save.
    Component {
        id: saveButtonComponent

        FooterButton {
            enabled: !form.editing
            opacity: enabled ? 1.0 : 0
            separatorTop: false
            icon.source: findIcon("save", Style.saveIconSource)
            extraScale: internal.buttonScale
            colorOverride: internal.buttonColor
            onClicked: {
                if (page.canSave()) {
                    saveStart()
                }
            }
        }
    }

    // NextOrSave.
    Component {
        id: nextOrSaveButtonComponent

        FooterButton {
            property bool saveMode: form.wizard.canSave && form.wizard.lastPage
            icon.source: saveMode ? findIcon("save", Style.saveIconSource) : findIcon("next", Style.nextIconSource)
            colorOverride: internal.buttonColor
            extraScale: internal.buttonScale
            enabled: form.wizard.canNext || saveMode
            opacity: enabled ? 1.0 : 0.5
            separatorTop: false

            onClicked: {
                if (!internal.clickReady) {
                    return
                }

                if (saveMode && page.canSave()) {
                    saveStart()

                } else if (page.canNext()) {
                    form.wizard.next(root.recordUid, root.fieldUid)
                }
            }
        }
    }

    // Index.
    Component {
        id: indexButtonComponent

        FooterButton {
            icon.source: findIcon("index", Style.formIconSource)
            colorOverride: internal.buttonColor
            extraScale: internal.buttonScale
            separatorTop: false
            onClicked: {
                form.wizard.index(root.recordUid, root.fieldUid)
            }
        }
    }


    // Options.
    Component {
        id: optionsButtonComponent

        FooterButton {
            icon.source: findIcon("options", Style.swapIconSource)
            extraScale: internal.buttonScale
            colorOverride: internal.buttonColor
            separatorTop: false
            onClicked: {
                form.wizard.options(root.recordUid, root.fieldUid)
            }
        }
    }

    // Map.
    Component {
        id: mapButtonComponent

        FooterButton {
            enabled: !form.editing || true
            opacity: (!form.editing && enabled) ? 1.0 : 0
            separatorTop: false
            icon.source: findIcon("map", Style.mapIconSource)
            extraScale: internal.buttonScale
            colorOverride: internal.buttonColor
            onClicked: {
                form.pushPage("qrc:/MapsPage.qml")
            }
        }
    }

    function findIcon(name, defaultIcon) {
        let result = getParam(params, name + "Icon")
        if (result === undefined) {
            return defaultIcon
        }

        return App.projectManager.getFileUrl(form.projectUid, result)
    }
}
