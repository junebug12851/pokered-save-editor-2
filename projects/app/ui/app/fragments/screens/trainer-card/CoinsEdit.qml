// CoinsEdit.qml -- the casino coins field on the trainer card.
//
// A 4-digit DefTextEdit bound to player.basics.coins (clamped 0-9,999) with a
// RandomButton (randomizeCoins). Persists on every valid keystroke. Mirrors MoneyEdit.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"
import "../../header"

MouseArea {
  hoverEnabled: true
  width: child.implicitWidth
  height: child.implicitHeight

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
      text: qsTr("Casino Coins, 0 - 9,999")
    }

    Connections {
      target: brg.file.data.dataExpanded.player.basics
      function onCoinsChanged() { child.text = brg.file.data.dataExpanded.player.basics.coins.toString(); }
    }

    RandomButton {
      tip: qsTr("Randomize the coins.")
      onRandomize: brg.file.data.dataExpanded.player.basics.randomizeCoins();
    }
  }
}
