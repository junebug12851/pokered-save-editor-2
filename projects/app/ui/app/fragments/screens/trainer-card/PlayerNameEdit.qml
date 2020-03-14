import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"
import "../../header"

NameDisplay {
  id: top

  isPersonName: true
  isPlayerName: true

  onStrChanged: brg.file.data.dataExpanded.player.basics.playerName = str;

  Connections {
    target: brg.file.data.dataExpanded.player.basics
    onPlayerNameChanged: top.str = brg.file.data.dataExpanded.player.basics.playerName;
  }

  Component.onCompleted: top.str = brg.file.data.dataExpanded.player.basics.playerName;
}
