import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"
import "../../header"
import "../../controls/menu"

DefTextEdit {
  id: minutesEdit

  opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
           ? 0.50
           : 1.00
  maximumLength: 2
  placeholderText: "00"
  width: 2 * font.pixelSize

  horizontalAlignment: Text.AlignRight

  onTextChanged: {
    if(text === "")
      return;

    var txtDec = parseInt(text, 10);
    if(txtDec === NaN)
      return;

    if(txtDec < 0 || txtDec > 59)
      return;

    brg.file.data.dataExpanded.world.other.playtime.minutes = txtDec;
  }

  Connections {
    target: brg.file.data.dataExpanded.world.other.playtime
    onMinutesChanged: minutesEdit.text = brg.file.data.dataExpanded.world.other.playtime.minutes
  }

  Component.onCompleted: minutesEdit.text = brg.file.data.dataExpanded.world.other.playtime.minutes;
}
