import QtQuick
import QtQuick.Controls.Material

ListView {
    clip: true
    ScrollBar.vertical: ScrollBar { interactive: false }
    orientation: ListView.Vertical

    footer: FooterSpacer {}
}
