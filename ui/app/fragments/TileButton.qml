import QtQuick 2.14
import QtQuick.Controls 2.14

import "../fragments"
import "../common/Style.js" as Style

Rectangle {

  property alias title: title.text
  property alias imgSrc: image.source
  property alias hotKey: hotkey.text

  property real sizeH: parent.height
  property int contW: parent.width
  property int contH: parent.height

  property real divider: 0.50
  property real titleOpacity: 0.60
  property int iconSize: 50
  property int iconSizeTitlePadding: 25
  property int titleSize: 25
  property int hotKeyTitlePadding: 5
  property int hotKeySize: 15

  width: contW * divider
  height: contH * sizeH
  color: manualMouse.curColor
  border.color: Style.primaryColorDark
  border.width: 1

  Image {
    id: image
    x: (parent.width / 2) - (width / 2)
    y: (parent.height / 2) - (height / 2) - titleSize - iconSizeTitlePadding
    width: iconSize
    height: iconSize
    source: "qrc:/assets/fontawesome-icons/file.svg"
    opacity: titleOpacity
  }

  Text {
    id: title
    text: "New Blank File"
    color: Qt.darker(Style.textColorPrimary, 2)
    opacity: titleOpacity
    anchors.centerIn: parent
    font.pixelSize: titleSize
    font.bold: true
  }

  Text {
    id: hotkey
    x: (parent.width / 2) - (width / 2)
    y: ((parent.height / 2) - (height / 2)) + titleSize + hotKeyTitlePadding
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
  }
}
