import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

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
        height: root.height
        font.pixelSize: App.settings.font14
        text: modelData.name
    }
}
