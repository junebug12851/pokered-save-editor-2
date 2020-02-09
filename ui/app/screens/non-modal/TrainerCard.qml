import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: top

  Rectangle {
    id: cardFront
    anchors.centerIn: parent
    border.color: brg.settings.textColorMid
    radius: 25
    color: "transparent"

    width: 500
    height: width / 2

    NameDisplay {
      id: playerNameEdit

      anchors.left: parent.left
      anchors.top: parent.top
      anchors.leftMargin: 15
      anchors.topMargin: 33

      isPersonName: true
      isPlayerName: true

      str: brg.file.data.dataExpanded.player.basics.playerName
      onStrChanged: brg.file.data.dataExpanded.player.basics.playerName = str;

      Connections {
        target: brg.file.data.dataExpanded.player.basics
        onPlayerNameChanged: playerNameEdit.str = brg.file.data.dataExpanded.player.basics.playerName;
      }
    }

    DefTextEdit {
      id: playerIdEdit

      anchors.top: playerNameEdit.top
      anchors.topMargin: -5
      anchors.left: playerNameEdit.right
      anchors.leftMargin: 150
      labelEl.text: "#"

      maximumLength: 4
      placeholderText: "ID"
      width: font.pixelSize * 6

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
        onPlayerIDChanged: playerIdEdit.text = brg.file.data.dataExpanded.player.basics.playerID.toString(16).toUpperCase();
      }
    }

    Spacer {
      id: spacer

      anchors.top: playerNameEdit.bottom
      anchors.topMargin: 25
      anchors.left: parent.left

      width: parent.width
      height: 1
      border.color: brg.settings.dividerColor
    }

    DefTextEdit {
      id: moneyEdit

      anchors.top: spacer.top
      anchors.topMargin: 25

      anchors.left: playerIdEdit.left
      labelEl.text: "Money"
      labelEl.rightPadding: 7

      maximumLength: 6
      placeholderText: "0"
      width: playerIdEdit.width

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

      Connections {
        target: brg.file.data.dataExpanded.player.basics
        onMoneyChanged: moneyEdit.text = brg.file.data.dataExpanded.player.basics.money
      }
    }

    DefTextEdit {
      id: coinsEdit

      anchors.top: moneyEdit.top
      anchors.topMargin: 35

      anchors.left: moneyEdit.left
      labelEl.text: "Coins"
      labelEl.rightPadding: 7

      maximumLength: 4
      placeholderText: "0"
      width: playerIdEdit.width

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

      Connections {
        target: brg.file.data.dataExpanded.player.basics
        onMoneyChanged: coinsEdit.text = brg.file.data.dataExpanded.player.basics.coins
      }
    }

    Image {
      source: "qrc:/assets/images/red-larger.png"

      anchors.left: parent.left
      anchors.leftMargin: 35

      anchors.top: spacer.bottom
      anchors.topMargin: 10

      anchors.bottom: parent.bottom
      anchors.bottomMargin: 10

      width: parent.width / 3

      fillMode: Image.PreserveAspectFit
    }
  }

  // 1 Button Footer, the Randomize Button
  footer: AppFooterBtn1 {
    icon1.source: "qrc:/assets/icons/fontawesome/dice.svg"
    text1: "Re-Roll"
    onBtn1Clicked: {
      // Most of the data on the screen
      brg.file.data.dataExpanded.player.basics.randomize();

      // Playtime
      brg.file.data.dataExpanded.world.other.randomizePlaytime();
    }
  }
}
