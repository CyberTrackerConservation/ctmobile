import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

GridView {
    property var params: ({})
    property int padding: App.scaleByFontSize(params.padding === undefined ? 8 : params.padding)
    property int columns: params.columns === undefined ? 2 : params.columns
    property bool lines: params.lines !== undefined ? params.lines : true
    property bool sideBorder: true
    property int borderWidth: params.borderWidth === undefined ? Style.lineWidth1 : App.scaleByFontSize(params.borderWidth)
    property string style: (params.style === undefined ? "TextOnly" : params.style).toLowerCase()
    property int fontSize: params.fontSize === undefined ? App.settings.font16 : App.scaleByFontSize(params.fontSize)
    property bool fontBold: params.fontBold === undefined ? false : params.fontBold
    property int itemHeight: App.scaleByFontSize(params.itemHeight === undefined ? 48 : params.itemHeight)
}
