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

    labelEl.text: "Coins"
    maximumLength: 4

    text: brg.file.data.dataExpanded.player.basics.coins
    onTextChanged: {
      if(text === "")
        return;

      var txtDec = parseInt(text, 10);
      if(txtDec === NaN)
        return;

      if(txtDec < 0 || txtDec > 9999)
        return;

      brg.file.data.dataExpanded.player.basics.coins = txtDec;
    }

    MainToolTip {
      text: "Your casino coins"
    }

    Connections {
      target: brg.file.data.dataExpanded.player.basics
      onCoinsChanged: child.text = brg.file.data.dataExpanded.player.basics.coins
    }

    RandomMenu {
      id: menuBtn
      onRandomize: brg.file.data.dataExpanded.player.basics.randomizeCoins();
    }
  }
}
