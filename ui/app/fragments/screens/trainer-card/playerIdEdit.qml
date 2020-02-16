import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"
import "../../header"
import "../../controls/menu"

MouseArea {
  hoverEnabled: true
  width: child.implicitWidth
  height: child.implicitHeight
  onContainsMouseChanged: menuBtn.visible = containsMouse

  DefTextEdit {
    id: child
    anchors.fill: parent
    labelEl.text: "ID"
    maximumLength: 4
    placeholderText: "0000"

    text: brg.file.data.dataExpanded.player.basics.playerID.toString(16).toUpperCase()
    onTextChanged: {
      if(text === "")
        return;

      var idDec = parseInt(text, 16);
      if(idDec === NaN)
        return;

      if(idDec < 0 || idDec > 0xFFFF)
        return;

      brg.file.data.dataExpanded.player.basics.playerID = idDec;
    }

    Connections {
      target: brg.file.data.dataExpanded.player.basics
      onPlayerIDChanged: child.text = brg.file.data.dataExpanded.player.basics.playerID.toString(16).toUpperCase();
    }

    RandomMenu {
      id: menuBtn
      onRandomize: brg.file.data.dataExpanded.player.basics.randomizeID();
    }
  }
}
