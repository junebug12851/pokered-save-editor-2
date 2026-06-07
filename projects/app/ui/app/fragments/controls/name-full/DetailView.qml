// DetailView.qml -- the right-side detail panel of the full keyboard.
//
// A Pane showing the selected character/code's color swatch, title, raw code, an
// optional divider, and a wrapped description. Fields are exposed via aliases
// (titleEl/codeEl/descEl/...) so the picker can fill them in as you browse.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"

Pane {
  property alias colorCodeEl: colorCode
  property alias titleEl: title
  property alias codeEl: code
  property alias descEl: desc
  property alias descDividerEl: descDivider

  Column {
    anchors.fill: parent

    Spacer {
      id: colorCode
      width: parent.width
      height: 10
    }

    Text {
      id: title
      font.pixelSize: 18
      font.bold: true
    }

    Text {
      id: code
      font.pixelSize: 14
      font.italic: true
      textFormat: Text.PlainText
    }

    Spacer {
      visible: false
      id: descDivider
      width: parent.width
      height: 1
      border.color: brg.settings.dividerColor
    }

    Text {
      id: desc
      font.pixelSize: 12
      width: parent.width
      wrapMode: Text.WordWrap
    }
  }
}
