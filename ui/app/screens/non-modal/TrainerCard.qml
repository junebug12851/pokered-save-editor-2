import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"
import "../../fragments/screens/trainer-card"

Page {
  id: top

  Rectangle {
    id: cardFront
    anchors.centerIn: parent
    border.color: brg.settings.textColorMid
    color: "transparent"

    width: 500
    height: width / 2

    PlayerNameEdit {
      id: playerNameEdit

      anchors.left: parent.left
      anchors.top: parent.top
      anchors.leftMargin: 15
      anchors.topMargin: 33
    }

    PlayerIdEdit {
      id: playerIdEdit
      width: starterEdit.width

      anchors.top: playerNameEdit.top
      anchors.topMargin: -5
      anchors.left: playerNameEdit.right
      anchors.leftMargin: 150
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

    MoneyEdit {
      id: moneyEdit

      anchors.top: spacer.top
      anchors.topMargin: 25
      anchors.left: playerIdEdit.left

      width: starterEdit.width
    }

    CoinsEdit {
      id: coinsEdit

      anchors.top: moneyEdit.top
      anchors.topMargin: 40
      anchors.left: moneyEdit.left

      width: starterEdit.width
    }

    StarterEdit {
      id: starterEdit

      anchors.top: coinsEdit.top
      anchors.topMargin: 25

      anchors.left: coinsEdit.left
      width: font.pixelSize * 10 // Starter name is max of 10 chars
    }

    PlaytimeEdit {
      id: playtimeEdit

      anchors.bottom: cardFront.bottom
      anchors.bottomMargin: 2
      anchors.right: cardFront.right
      anchors.rightMargin: 82
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
