import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import CyberTracker

Loader {
    id: root

    signal accepted(var fileUrl)
    signal rejected()

    Component {
        id: nativeImagePicker

        NativeImagePicker {

            onAccepted: (fileUrl) => {
                root.accepted(fileUrl)
            }

            onRejected: {
                root.rejected()
            }
        }
    }

    Component {
        id: imagePicker

        FileDialog {
            id: picker

            currentFolder: {
                let paths = StandardPaths.standardLocations(StandardPaths.PicturesLocation)
                return paths[paths.length -1]
            }

            nameFilters: ["Image files (*.jpg *.jpeg *.png *.heic)"]

            onAccepted: {
                root.accepted(picker.selectedFile)
            }

            onRejected: {
                root.rejected()
            }
        }
    }

    function open() {
        sourceComponent = App.desktopOS ? imagePicker : nativeImagePicker
        item.open()
    }
}
