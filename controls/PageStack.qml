import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    property alias initialItem: _stackView.initialItem

    StackView {
        id: _stackView
        anchors.fill: parent
    }
}
