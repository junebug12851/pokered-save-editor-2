import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

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
