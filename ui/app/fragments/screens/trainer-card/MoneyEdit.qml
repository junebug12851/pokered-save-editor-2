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

    labelEl.text: "Money"
    maximumLength: 6

    text: brg.file.data.dataExpanded.player.basics.money
    onTextChanged: {
      if(text === "")
        return;

      var txtDec = parseInt(text, 10);
      if(txtDec === NaN)
        return;

      if(txtDec < 0 || txtDec > 999999)
        return;

      brg.file.data.dataExpanded.player.basics.money = txtDec;
    }

    MainToolTip {
      text: "Player Money, 0 - 999,999"
    }

    Connections {
      target: brg.file.data.dataExpanded.player.basics
      onMoneyChanged: child.text = brg.file.data.dataExpanded.player.basics.money
    }

    RandomMenu {
      id: menuBtn
      onRandomize: brg.file.data.dataExpanded.player.basics.randomizeMoney();
    }
  }
}
