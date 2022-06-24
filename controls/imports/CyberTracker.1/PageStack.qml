import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Item {
    property alias initialItem: _stackView.initialItem

    StackView {
        id: _stackView
        anchors.fill: parent
    }
}
