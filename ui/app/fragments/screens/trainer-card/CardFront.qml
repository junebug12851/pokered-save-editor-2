import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"
import "../../header"
import "../../controls/menu"

Rectangle {
  id: cardFront
  border.color: brg.settings.textColorMid
  color: "transparent"

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

    anchors.bottom: spacer.top
    anchors.bottomMargin: 13
    anchors.right: parent.right
    anchors.rightMargin: 65
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

    anchors.top: starterEdit.bottom
    anchors.topMargin: 5
    anchors.right: starterEdit.right
    anchors.rightMargin: 0
  }

  Image {
    source: "qrc:/assets/images/red-larger.png"

    anchors.left: parent.left
    anchors.leftMargin: 75

    anchors.top: spacer.bottom
    anchors.topMargin: 10

    anchors.bottom: parent.bottom
    anchors.bottomMargin: 10

    width: parent.width / 3

    fillMode: Image.PreserveAspectFit
  }

  ListBadges {
    id: badges

    anchors.left: parent.left
    anchors.leftMargin: 10

    anchors.top: spacer.bottom
    anchors.topMargin: 20

    cellSize: 35
    width: cellSize * 2
    height: cellSize * 4
  }
}
