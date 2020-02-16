import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"
import "../../header"
import "../../controls/menu"

DefTextEdit {
  id: hoursEdit

  opacity: (brg.file.data.dataExpanded.world.other.playtime.clockMaxed)
           ? 0.50
           : 1.00
  maximumLength: 2
  width: 2 * font.pixelSize

  horizontalAlignment: Text.AlignRight

  onTextChanged: {
    if(text === "")
      return;

    var txtDec = parseInt(text, 10);
    if(txtDec === NaN)
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
    onHoursChanged: hoursEdit.text = brg.file.data.dataExpanded.world.other.playtime.hoursAdjusted
  }

  Component.onCompleted: hoursEdit.text = brg.file.data.dataExpanded.world.other.playtime.hoursAdjusted;
}
