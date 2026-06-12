import QtQuick
import CyberTracker as C
import CyberTracker.Classic 1.0

C.ContentPage {
    id: page

    header: Item {
        function sendBackClick(backConfirmed, androidBack) {
            popPageDefault(page)
        }
    }

    Component.onCompleted: {
        if (!form.project.loggedIn) {
            form.triggerLogin()
        }
    }

    ClassicView {
        id: classicView
        anchors.fill: parent
        fillColor: "black"
        initialWidth: parent.width
        initialHeight: parent.height

        onSetInputMethodVisible: function(visible) {
            if (visible) {
                Qt.inputMethod.show()
            } else {
                Qt.inputMethod.hide()
            }
        }

        onShowCameraDialog: function(resolutionX, resolutionY, simpleMode) {
            form.pushPage("qrc:/imports/CyberTracker/FieldCameraPage.qml", { photoIndex: 0, resolutionX: resolutionX, resolutionY: resolutionY, simpleMode: simpleMode } )
        }

        onShowBarcodeDialog: function() {
            form.pushPage("qrc:/imports/CyberTracker/FieldBarcodePage.qml")
        }

        onShowExports: function() {
            form.pushPage("qrc:/ExportsPage.qml", { projectUid: form.project.uid })
        }

        onShowUploadQueue: function() {
            form.pushPage("qrc:/UploadQueuePage.qml")
        }

        onShowMapPage: function() {
            form.pushPage("qrc:/MapsPage.qml")
        }

        onShowSightingEditorPage: function(isExportMode) {
            form.pushPage("qrc:/Classic/SightingEditorPage.qml", { isExportMode: isExportMode, historyPoints: classicView.historyPoints })
        }

        onShowTextEditorPage: function(controlId) {
            form.pushPage("qrc:/Classic/TextEditorPage.qml", { controlId: controlId })
        }

        onShowGPSReaderPage: function(saveTarget, skipTimeout, skipAllowed) {
            popupLocation.open({ skipTimeout: skipTimeout, skipAllowed: skipAllowed, contextData: { saveTarget: saveTarget } })
        }

        onShowConfirm: function(controlId, message) {
            confirmPopup.controlId = controlId
            confirmPopup.text = message
            confirmPopup.open()
        }

        onShutdown: function() {
            form.popPage();
        }

        onSubmitDataClicked: function() {
            form.submitData()
        }

        onShareData: function() {
            if (!form.hasData()) {
                App.showToast(qsTr("No new data"));
                return
            }

            confirmShare.open()
        }
    }

    C.PopupLoader {
        id: popupLocation
        popupComponent: Component {
            C.LocationPopup {
                showTriangle: false
            }
        }
    }

    C.ConfirmPopup {
        id: confirmShare
        text: qsTr("Share outstanding data?")
        confirmText: qsTr("Yes, share it")
        onConfirmed: {
            busyCover.doWork = shareData
            busyCover.start()
        }
    }

    C.ConfirmPopup {
        id: confirmPopup
        property int controlId: 0
        onConfirmed: classicView.confirmResult(controlId, true)
        onCancel: classicView.confirmResult(controlId, false)
    }

    function shareData() {
        let result = form.exportToCSV()
        if (result.success) {
            App.showToast(qsTr("Success"))
            form.pushPage("qrc:/ExportsPage.qml", { projectUid: form.project.uid })

        } else if (result.expected) {
            App.showToast(result.errorString)

        } else {
            App.showError(result.errorString)
        }
    }
}
