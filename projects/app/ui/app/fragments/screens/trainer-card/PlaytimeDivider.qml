// PlaytimeDivider.qml -- the ":" separator between playtime fields.
//
// A simple colon Text, dimmed when the clock is maxed out
// (playtime.clockMaxed), matching the sub-fields' disabled look.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

Text {
  text: ":"
  font.pixelSize: 14
  opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
           ? 0.50
           : 1.00
}
