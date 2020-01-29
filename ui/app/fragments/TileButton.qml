import QtQuick 2.14
import QtQuick.Controls 2.14

import "../fragments"
import "../common/Style.js" as Style

Rectangle {
  id: tile

  property alias title: title.text
  property alias imgSrc: image.source
  property alias hotKey: hotkey.text

  signal clicked()

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
  color: manualMouse.curColor
  border.color: Style.primaryColorDark
  border.width: 1

  Image {
    id: image
    x: (parent.width / 2) - (width / 2)
    anchors.bottom: title.top
    width: iconSize
    height: iconSize
    opacity: titleOpacity
  }

  Text {
    id: title
    color: Qt.darker(Style.textColorPrimary, 2)
    opacity: titleOpacity
    topPadding: 7
    bottomPadding: 5
    anchors.centerIn: parent
    font.pixelSize: titleSize
    font.bold: true
  }

  Text {
    id: hotkey
    x: (parent.width / 2) - (width / 2)
    anchors.top: title.bottom
    color: Qt.darker(Style.textColorPrimary, 2)
    opacity: titleOpacity
    font.pixelSize: hotKeySize
    font.italic: true
  }

  ManualButton {
    id: manualMouse
    refColor: "#7fefefef"
    refDefColor: "transparent"
    anchors.fill: parent
    onClicked: parent.clicked()
  }
}
