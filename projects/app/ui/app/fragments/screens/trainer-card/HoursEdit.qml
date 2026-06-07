// HoursEdit.qml -- the "hours" field of the playtime clock.
//
// A 2-digit DefTextEdit bound to playtime.hoursAdjusted (0-23, or 0-15 when days
// is 10), dimmed when the clock is maxed. A PlaytimeEdit sub-field (see
// DaysEdit.qml for the shared pattern).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"
import "../../controls/menu"

DefTextEdit {
  id: hoursEdit

  opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
           ? 0.50
           : 1.00
  maximumLength: 2
  width: 2 * font.pixelSize + leftPadding + rightPadding

  horizontalAlignment: Text.AlignRight

  onTextChanged: {
    if(text === "")
      return;

    var txtDec = parseInt(text, 10);
    if(isNaN(txtDec))
      return;

    if(txtDec < 0 || txtDec > 15)
      return;

    brg.file.data.dataExpanded.world.other.playtime.hoursAdjusted = txtDec;
  }

  MainToolTip {
    text: "Hours played, 0-23. If days is 10 then 0-15"
  }

  Connections {
    target: brg.file.data.dataExpanded.world.other.playtime
    function onHoursChanged() { hoursEdit.text = brg.file.data.dataExpanded.world.other.playtime.hoursAdjusted.toString(); }
  }

  Component.onCompleted: hoursEdit.text = brg.file.data.dataExpanded.world.other.playtime.hoursAdjusted.toString()
}