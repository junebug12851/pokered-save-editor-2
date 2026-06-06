import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

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

    labelEl.text: "Coins"
    maximumLength: 4

    onTextChanged: {
      if(text === "")
        return;

      var txtDec = parseInt(text, 10);
      if(isNaN(txtDec))
        return;

      if(txtDec < 0 || txtDec > 9999)
        return;

      brg.file.data.dataExpanded.player.basics.coins = txtDec;
    }

    Component.onCompleted: child.text = brg.file.data.dataExpanded.player.basics.coins.toString()

    MainToolTip {
      text: "Casino Coins, 0 - 9,999"
    }

    Connections {
      target: brg.file.data.dataExpanded.player.basics
      function onCoinsChanged() { child.text = brg.file.data.dataExpanded.player.basics.coins.toString(); }
    }

    RandomMenu {
      id: menuBtn
      onRandomize: brg.file.data.dataExpanded.player.basics.randomizeCoins();
    }
  }
}