// DetailToolTip.qml -- the app's "detailed" (help-mode) tooltip.
//
// An overhauled tooltip with a header row (a small info badge + a bold title) over a
// wrapped body. Drop one inside a hoverable row and feed it `title` + `text`; bind
// `hovered` to the row's hover state. It only shows while hovered AND global tooltips
// are on (the header "?" toggle, brg.settings.infoBtnPressed) AND there is body text,
// so rows with no info stay quiet.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

ToolTip {
  id: tip

  property string title: ""
  property bool hovered: false
  // When false the tooltip ignores the global help toggle (always shows on hover).
  property bool followGlobalSetting: true

  visible: tip.hovered
           && tip.text !== ""
           && (!tip.followGlobalSetting || brg.settings.infoBtnPressed)

  delay: 300
  padding: 0

  Material.foreground: brg.settings.textColorLight

  background: Rectangle {
    color: brg.settings.textColorDark
    radius: 6
  }

  contentItem: ColumnLayout {
    spacing: 0

    // Header: a small info badge + the title.
    RowLayout {
      visible: tip.title !== ""
      Layout.fillWidth: true
      Layout.leftMargin: 10
      Layout.rightMargin: 10
      Layout.topMargin: 8
      spacing: 7

      Rectangle {
        Layout.alignment: Qt.AlignVCenter
        implicitWidth: 15
        implicitHeight: 15
        radius: 8
        color: "transparent"
        border.color: Qt.rgba(1, 1, 1, 0.65)
        border.width: 1
        Text {
          anchors.centerIn: parent
          text: "i"
          font.pixelSize: 10
          font.bold: true
          color: brg.settings.textColorLight
        }
      }

      Text {
        Layout.fillWidth: true
        text: tip.title
        font.pixelSize: 13
        font.bold: true
        color: brg.settings.textColorLight
        elide: Text.ElideRight
      }
    }

    Rectangle {
      visible: tip.title !== ""
      Layout.fillWidth: true
      Layout.topMargin: 8
      implicitHeight: 1
      color: Qt.rgba(1, 1, 1, 0.15)
    }

    Text {
      Layout.fillWidth: true
      Layout.maximumWidth: 300
      Layout.margins: 10
      text: tip.text
      textFormat: Text.PlainText
      wrapMode: Text.WordWrap
      lineHeight: 1.15
      font.pixelSize: 12
      color: brg.settings.textColorLight
    }
  }
}
