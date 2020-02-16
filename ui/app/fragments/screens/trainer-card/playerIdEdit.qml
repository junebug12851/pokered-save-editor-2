import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"
import "../../header"
import "../../controls/menu"

MouseArea {
  id: mouseArea
  hoverEnabled: true
  width: child.implicitWidth
  height: child.implicitHeight

  DefTextEdit {
    id: child
    anchors.fill: parent
    labelEl.text: "ID"
    maximumLength: 4

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

    MainToolTip {
      text: "Determines if your Pokemon are traded or not. 4 Characters each 0-9 or A-F."
    }

    Connections {
      target: brg.file.data.dataExpanded.player.basics
      onPlayerIDChanged: child.text = brg.file.data.dataExpanded.player.basics.playerID.toString(16).toUpperCase();
    }

    RandomMenu {
      id: menuBtn
      visible: mouseArea.containsMouse
      onRandomize: brg.file.data.dataExpanded.player.basics.randomizeID();
    }
  }
}
