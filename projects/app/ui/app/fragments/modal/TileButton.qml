// TileButton.qml -- a large bordered "tile" button used on the modal menus
// (NewFile, FileTools).
//
// Shows a centered icon, bold title, and an italic hotkey hint, with a full-tile
// flat Button capturing clicks (re-emitted as clicked()) and an InfoButton tooltip
// (infoTxt). Width/height are fractions of the parent (divider x sizeH) so several
// tiles pack into the modal grid.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

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
  property int titleSize: 18
  property int hotKeySize: 15

  width: contW * divider
  height: contH * sizeH
  border.color: brg.settings.textColorMid
  border.width: 1
  color: "transparent"

  clip: true

  ColumnLayout {
    anchors.centerIn: parent
    width: parent.width
    spacing: -3

    Image {
      id: image
      fillMode: Image.PreserveAspectFit
      Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
      width: iconSize
      height: iconSize
      opacity: titleOpacity

      sourceSize.height: iconSize
      sourceSize.width: iconSize
    }

    Text {
      id: title
      Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
      opacity: titleOpacity
      topPadding: 7
      bottomPadding: 5
      font.pixelSize: titleSize
      font.bold: true
    }

    Text {
      id: hotkey
      Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
      opacity: titleOpacity
      font.pixelSize: hotKeySize
      font.italic: true
    }
  }

  Button {
    anchors.fill: parent
    onClicked: parent.clicked()
    flat: true

    topInset: -5
    bottomInset: -5
  }

  InfoButton {
    id: infoBtn
  }
}
