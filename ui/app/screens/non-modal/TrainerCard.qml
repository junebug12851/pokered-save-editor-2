import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/header"

Page {
  id: top

  Rectangle {
    color: "transparent"

    NameDisplay {
      id: playerNameEdit

      anchors.left: parent.left
      anchors.top: parent.top
      anchors.leftMargin: 15
      anchors.topMargin: 43

      sizeMult: 2
      isPersonName: true
      isPlayerName: true

      str: brg.file.data.dataExpanded.player.basics.playerName
      onStrChanged: brg.file.data.dataExpanded.player.basics.playerName = str

      Connections {
        target: brg.file.data.dataExpanded.player.basics
        onPlayerNameChanged: playerNameEdit.str = brg.file.data.dataExpanded.player.basics.playerName
      }
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
