import QtQml 2.12
import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import CyberTracker 1.0 as C
import QtQuick.Controls.Material 2.12
import CyberTracker.Classic 1.0

Page {
    id: page

    Component.onCompleted: {
        form.readonly = true
    }

    ClassicSession {
        id: classicSession
        fillColor: "black"

        Component.onCompleted: {
            width = parent.width
            height = parent.height
            connectToProject(form.project.uid)
        }

        Connections {
            target: formPageStack

            function onDepthChanged() {
                classicSession.blockResize = formPageStack.depth > 1 || classicSession.width == 0 || classicSession.height == 0
            }
        }

        Rectangle { // Provide a background for the text editor, so it works in dark mode as well.
            color: Material.background
            x: textEdit.x
            y: textEdit.y
            width: textEdit.width
            height: textEdit.height
            visible: textEdit.visible
        }

        TextArea {
            id: textEdit
            width: 100
            height: 100
            visible: false
            padding: 4
            wrapMode: Text.Wrap
            text: classicSession.lastEditorText
            inputMethodHints: Qt.ImhNoPredictiveText
            onTextChanged: {
                deselect()
                if (visible) {
                    classicSession.lastEditorText = textEdit.text
                }
            }
        }

        onSetInputMethodVisible: {
            if (visible) {
                Qt.inputMethod.show()
            } else {
                Qt.inputMethod.hide()
            }
        }

        onShowEditControl: {
            textEdit.x = params.x
            textEdit.y = params.y
            textEdit.width = params.width
            textEdit.height = params.height
            textEdit.wrapMode = params.singleLine === true ? TextEdit.NoWrap : TextEdit.WordWrap
            textEdit.inputMethodHints = params.password === true ? Qt.ImhHiddenText : 0
            textEdit.visible = true
            textEdit.cursorVisible = true
            textEdit.forceActiveFocus()
        }

        onHideEditControl: {
            textEdit.focus = false
            textEdit.cursorVisible = false
            Qt.inputMethod.hide()
            textEdit.visible = false
            classicSession.forceActiveFocus()
        }

        onSetEditControlFont: {
            textEdit.font.bold = params.bold
            textEdit.font.italic = params.italic
            textEdit.font.family = params.family
            textEdit.font.pixelSize = params.height
        }

        onShowCameraDialog: {
            form.pushPage("qrc:/imports/CyberTracker.1/FieldCameraPage.qml", { photoIndex: 0, resolutionX: resolutionX, resolutionY: resolutionY } )
        }

        onShowBarcodeDialog: {
            form.pushPage("qrc:/imports/CyberTracker.1/FieldBarcodePage.qml")
        }

        onShowSkyplot: {
            skyplot.x = x + page.width / 16
            skyplot.y = y
            skyplot.width = width - (2 * page.width) / 16
            skyplot.height = height
            skyplot.visible = visible
            skyplot.active = visible
        }

        onShowExports: {
            form.pushPage("qrc:/ExportsPage.qml", { projectUid: form.project.uid })
        }

        onShutdown: {
            form.popPage();
        }
    }

    C.Skyplot {
        id: skyplot
        visible: false
        active: false
        legendVisible: true
        legendDepth: height / 16
        legendSpace: height / 16
        darkMode: true
    }
}
