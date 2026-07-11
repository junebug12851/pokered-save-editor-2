// DaysEdit.qml -- the "days" field of the playtime clock.
//
// A 2-digit DefTextEdit bound to playtime.days (clamped 0-10), dimmed when the
// clock is maxed. One of the PlaytimeEdit sub-fields; they all share this pattern
// (live write on valid input + a Connections sync-back).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

DefTextEdit {
  id: daysEdit

  opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
           ? 0.50
           : 1.00
  maximumLength: 2
  width: 2 * font.pixelSize + leftPadding + rightPadding

  horizontalAlignment: Text.AlignHCenter

  onTextChanged: {
    if(text === "")
      return;

    var txtDec = parseInt(text, 10);
    if(isNaN(txtDec))
      return;

    if(txtDec < 0 || txtDec > 10)
      return;

    brg.file.data.dataExpanded.world.other.playtime.days = txtDec;
  }

  MainToolTip {
    text: qsTr("Days played, 0-10")
  }

  Connections {
    target: brg.file.data.dataExpanded.world.other.playtime
    function onDaysChanged() { daysEdit.text = brg.file.data.dataExpanded.world.other.playtime.days.toString(); }
  }

  Component.onCompleted: daysEdit.text = brg.file.data.dataExpanded.world.other.playtime.days.toString()
}