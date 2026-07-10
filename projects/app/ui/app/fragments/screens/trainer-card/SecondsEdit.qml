// SecondsEdit.qml -- the "seconds" field of the playtime clock.
//
// A 2-digit DefTextEdit bound to playtime.seconds (clamped 0-59), dimmed when the
// clock is maxed. A PlaytimeEdit sub-field (see DaysEdit.qml for the shared
// pattern).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

DefTextEdit {
  id: secondsEdit

  opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
           ? 0.50
           : 1.00
  maximumLength: 2
  // reserve room for the digits PLUS the field's horizontal padding
  // (the old `2 * font.pixelSize` ignored padding, clipping the value)
  width: 2 * font.pixelSize + leftPadding + rightPadding

  horizontalAlignment: Text.AlignRight

  onTextChanged: {
    if(text === "")
      return;

    var txtDec = parseInt(text, 10);
    if(isNaN(txtDec))
      return;

    if(txtDec < 0 || txtDec > 59)
      return;

    brg.file.data.dataExpanded.world.other.playtime.seconds = txtDec;
  }

  MainToolTip {
    text: qsTr("Seconds played, 0-59")
  }

  Connections {
    target: brg.file.data.dataExpanded.world.other.playtime
    function onSecondsChanged() { secondsEdit.text = brg.file.data.dataExpanded.world.other.playtime.seconds.toString(); }
  }

  Component.onCompleted: secondsEdit.text = brg.file.data.dataExpanded.world.other.playtime.seconds.toString()
}