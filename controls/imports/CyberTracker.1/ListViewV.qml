import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12

ListView {
    clip: true
    ScrollBar.vertical: ScrollBar { interactive: false }
    orientation: ListView.Vertical
}
