import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ComboBox {
    id: root

    font.pixelSize: App.settings.font14

    model: form.project.languages
    displayText: currentIndex !== -1 ? model[currentIndex].name : ""

    currentIndex: form.project.languageIndex
    onCurrentIndexChanged: {
        if (currentIndex !== -1) {
            form.project.languageIndex = currentIndex
        }
    }

    delegate: ItemDelegate {
        width: root.width
        font.pixelSize: root.font.pixelSize
        text: modelData.name
    }
}
