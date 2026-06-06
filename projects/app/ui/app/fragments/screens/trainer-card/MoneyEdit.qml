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

    labelEl.text: "Money"
    maximumLength: 6

    onTextChanged: {
      if(text === "")
        return;

      var txtDec = parseInt(text, 10);
      if(isNaN(txtDec))
        return;

      if(txtDec < 0 || txtDec > 999999)
        return;

      brg.file.data.dataExpanded.player.basics.money = txtDec;
    }

    Component.onCompleted: child.text = brg.file.data.dataExpanded.player.basics.money.toString()

    MainToolTip {
      text: "Money, 0 - 999,999"
    }

    Connections {
      target: brg.file.data.dataExpanded.player.basics
      function onMoneyChanged() { child.text = brg.file.data.dataExpanded.player.basics.money.toString(); }
    }

    RandomMenu {
      id: menuBtn
      onRandomize: brg.file.data.dataExpanded.player.basics.randomizeMoney();
    }
  }
}