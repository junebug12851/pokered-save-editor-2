// StarterEdit.qml -- the player's-starter field on the trainer card.
//
// A flat ComboBox over brg.starterModel bound to player.basics.playerStarter, with a
// RandomButton (randomizeStarter). This records which starter the player chose (per
// the tooltip, likely cosmetic in-game).
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

    RandomButton {
      tip: qsTr("Randomize the starter.")
      onRandomize: brg.file.data.dataExpanded.player.basics.randomizeStarter();
    }
  }
}
