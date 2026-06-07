// ShadedBG.qml -- a small shaded backing rectangle for stat value cells.
//
// A light grey 110x35 Rectangle placed behind a stat value to set it off from the
// background in the stats tables.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

Rectangle {
  color: Qt.lighter(brg.settings.textColorMid, 1.75)
  width: 110
  height: 35
}
