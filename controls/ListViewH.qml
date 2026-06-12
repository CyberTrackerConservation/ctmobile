import QtQuick
import QtQuick.Controls.Material

ListView {
    clip: true
    ScrollBar.horizontal: ScrollBar { interactive: false }
    orientation: ListView.Horizontal
}
