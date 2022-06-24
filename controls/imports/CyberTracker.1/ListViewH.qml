import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12

ListView {
    clip: true
    ScrollBar.horizontal: ScrollBar { interactive: false }
    orientation: ListView.Horizontal
}
