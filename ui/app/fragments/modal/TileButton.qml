import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../general"

Rectangle {
  id: tile

  signal clicked()

  property alias title: title.text
  property alias imgSrc: image.source
  property alias hotKey: hotkey.text
  property alias infoTxt: infoBtn.toolTipText

  property real sizeH: parent.height
  property int contW: parent.width
  property int contH: parent.height

  property real divider: 0.50
  property real titleOpacity: 0.60
  property int iconSize: 40
  property int titleSize: 20
  property int hotKeySize: 15
  property int minFullSize: 300

  width: contW * divider
  height: contH * sizeH
  border.color: brg.settings.primaryColorDark
  border.width: 1

  clip: true

  ColumnLayout {
    anchors.centerIn: parent

    Image {
      id: image
      width: iconSize
      height: iconSize
      opacity: titleOpacity
    }

    Text {
      id: title
      opacity: titleOpacity
      topPadding: 7
      bottomPadding: 5
      anchors.centerIn: parent
      font.pixelSize: titleSize
      font.bold: true
    }

    Text {
      id: hotkey
      opacity: titleOpacity
      font.pixelSize: hotKeySize
      font.italic: true
    }
  }

  AbstractButton {
    anchors.fill: parent
    onClicked: parent.clicked()
  }

  InfoButton {
    id: infoBtn
  }
}
