// HeaderText.qml -- a right-aligned row label for the stats tables.
//
// A small Text filling its parent cell, right-aligned and vertically centered,
// used as the leading label in the StatsGroup / DV-EV layouts.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

Text {
  height: parent.height
  width: parent.width
  horizontalAlignment: Text.AlignRight
  verticalAlignment: Text.AlignVCenter
  anchors.fill: parent
  font.pixelSize: 12
  font.letterSpacing: 1
}
