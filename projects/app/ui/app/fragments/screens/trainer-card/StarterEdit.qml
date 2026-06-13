// StarterEdit.qml -- the player's-starter field on the trainer card.
//
// A flat ComboBox over brg.starterModel bound to player.basics.playerStarter, with
// a hover RandomMenu (randomizeStarter). This records which starter the player
// chose (per the tooltip, likely cosmetic in-game).
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

  ComboBox {
    id: child
    anchors.fill: parent

    textRole: "monName"
    valueRole: "monInd"

    font.capitalization: Font.Capitalize
    font.pixelSize: 14
    flat: true
    model: brg.starterModel

    onActivated: brg.file.data.dataExpanded.player.basics.playerStarter = currentValue;
    Component.onCompleted: currentIndex = brg.starterModel.valToIndex(brg.file.data.dataExpanded.player.basics.playerStarter);

    MainToolTip {
      text: qsTr("Set based on starter you chose, I don't think this is ever used in gameplay.")
    }

    Connections {
      target: brg.file.data.dataExpanded.player.basics
      function onPlayerStarterChanged() { child.currentIndex = brg.starterModel.valToIndex(brg.file.data.dataExpanded.player.basics.playerStarter); }
    }

    Label {
      anchors.right: parent.left
      anchors.rightMargin: 7
      anchors.top: parent.top
      anchors.bottom: parent.bottom

      font.pixelSize: 14

      horizontalAlignment: Text.AlignRight
      verticalAlignment: Text.AlignVCenter

      text: qsTr("Starter")
    }

    RandomMenu {
      id: menuBtn
      anchors.top: parent.top
      anchors.left: parent.right
      anchors.bottom: parent.bottom
      anchors.topMargin: 0

      onRandomize: brg.file.data.dataExpanded.player.basics.randomizeStarter();
     }
  }
}
