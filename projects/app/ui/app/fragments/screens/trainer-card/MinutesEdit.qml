import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"
import "../../controls/menu"

DefTextEdit {
  id: minutesEdit

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

    if(txtDec < 0 || txtDec > 59)
      return;

    brg.file.data.dataExpanded.world.other.playtime.minutes = txtDec;
  }

  MainToolTip {
    text: "Minutes played, 0-59"
  }

  Connections {
    target: brg.file.data.dataExpanded.world.other.playtime
    function onMinutesChanged() { minutesEdit.text = brg.file.data.dataExpanded.world.other.playtime.minutes.toString(); }
  }

  Component.onCompleted: minutesEdit.text = brg.file.data.dataExpanded.world.other.playtime.minutes.toString()
}