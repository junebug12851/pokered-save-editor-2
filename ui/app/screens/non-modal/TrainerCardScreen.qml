import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../common/Style.js" as Style

Rectangle {

  color: Style.primaryColorLight

  NameDisplay {
    id: playerNameEdit

    anchors.left: parent.left
    anchors.top: parent.top
    anchors.leftMargin: 15
    anchors.topMargin: 43

    sizeMult: 2
    isPersonName: true
    isPlayerName: true

    str: file.data.dataExpanded.player.basics.playerName
    onStrChanged: file.data.dataExpanded.player.basics.playerName = str

    Connections {
      target: file.data.dataExpanded.player.basics
      onPlayerNameChanged: playerNameEdit.str = file.data.dataExpanded.player.basics.playerName
    }
  }
}
