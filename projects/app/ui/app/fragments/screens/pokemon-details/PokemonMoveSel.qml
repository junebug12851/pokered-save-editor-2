import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import App.PokemonBox
import App.PokemonParty
import App.PokemonMove

import "../../general"
import "../../header"
import "../../controls/selection"

// PokemonMoveSel.qml -- one of the four move rows in the Moves tab.
//
// Bound to a PokemonMove (monMove). A type-colored pill holding the move combo
// (SelectMove), the move's type, an editable PP field, "/ maxPP", and an overflow
// menu (PP-Up sub-menu, All-Moves bulk ops, restore PP, re-roll, correct). The
// getColor() type palette is from Bulbapedia (CC-BY-NC-SA) -- keep the
// attribution comment.
//
// One move row: a colored (by type) pill containing the move selector, its
// type, PP / max-PP, and a ⋮ menu. Compact height; the move combo fills the
// leftover width so the row always fits the pane and the trailing items (incl.
// the ⋮ menu) stay inside it instead of overflowing onto the stats pane.
Rectangle {
  id: top
  property PokemonMove monMove: null
  property int rowH: 34

  radius: 0
  implicitHeight: rowH
  color: getColor()

  // Thanks to Bulbapedia
  // bulbapedia.bulbagarden.net/wiki/Category:Type_color_templates
  // Licensed: CC-BY-NC-SA 2.5 (creativecommons.org/licenses/by-nc-sa/2.5)
  function getColor() {
    if(monMove.moveType === "Normal")
      return "#A8A878";
    else if(monMove.moveType === "Fighting")
      return Qt.lighter("#C03028", 1.50);
    else if(monMove.moveType === "Flying")
      return "#A890F0";
    else if(monMove.moveType === "Poison")
      return Qt.lighter("#A040A0", 1.50);
    else if(monMove.moveType === "Ground")
      return "#E0C068";
    else if(monMove.moveType === "Rock")
      return "#B8A038";
    else if(monMove.moveType === "Bug")
      return "#A8B820";
    else if(monMove.moveType === "Ghost")
      return Qt.lighter("#705898", 1.50);
    else if(monMove.moveType === "Fire")
      return "#F08030";
    else if(monMove.moveType === "Water")
      return "#6890F0";
    else if(monMove.moveType === "Grass")
      return "#78C850";
    else if(monMove.moveType === "Electric")
      return "#F8D030";
    else if(monMove.moveType === "Psychic")
      return "#F85888";
    else if(monMove.moveType === "Ice")
      return "#98D8D8";
    else if(monMove.moveType === "Dragon")
      return Qt.lighter("#7038F8", 1.50);

    // Used TCG Colorless for glitch color
    return "#E5D6D0";
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 10
    anchors.rightMargin: 6
    spacing: 4

    SelectMove {
      id: moveSelect
      // Fill the leftover width so the row fits the pane (keeps the ⋮ inside).
      Layout.fillWidth: true
      Layout.minimumWidth: 60
      Layout.preferredHeight: top.rowH

      onActivated: { monMove.moveID = currentValue; }
      Component.onCompleted: currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);

      Connections {
        target: monMove
        function onMoveIDChanged() { moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID); }
      }

      Connections {
        target: brg.moveSelectModel
        function onMonChanged() {
          moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);
        }
      }

      Connections {
        target: boxData
        function onSpeciesChanged() {
          moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);
        }
      }

      MainToolTip {
        text: qsTr("Pokémon Move")
      }
    }

    Text {
      visible: monMove.moveID !== 0

      Layout.alignment: Qt.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      Layout.preferredWidth: font.pixelSize * 4
      color: brg.settings.textColorDark

      text: monMove.moveType
      font.pixelSize: 13
      font.capitalization: Font.Capitalize
    }

    DefTextEdit {
      id: movePPEdit
      visible: monMove.moveID !== 0

      Layout.alignment: Qt.AlignVCenter
      Layout.preferredHeight: top.rowH
      horizontalAlignment: Text.AlignHCenter

      // PP is at most 2 digits; size for 2 chars + a little padding (trimmed).
      leftPadding: 8
      rightPadding: 8
      Layout.preferredWidth: 2 * font.pixelSize + leftPadding + rightPadding
      maximumLength: 2
      color: brg.settings.textColorDark

      onTextChanged: {
        if(text === "")
          return;

        var idDec = parseInt(text, 10);
        if(isNaN(idDec))
          return;

        if(idDec < 0 || idDec > 0xFF)
          return;

        monMove.pp = idDec;
      }
      Component.onCompleted: text = monMove.pp.toString(10);

      Connections {
        target: monMove
        function onPpChanged() { movePPEdit.text = monMove.pp.toString(10); }
      }

      MainToolTip {
        text: qsTr("Pokémon PP")
      }
    }

    Text {
      visible: monMove.moveID !== 0
      Layout.alignment: Qt.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      color: brg.settings.textColorDark
      text: "/"
      font.pixelSize: 13
      font.capitalization: Font.Capitalize
    }

    Text {
      visible: monMove.moveID !== 0
      Layout.alignment: Qt.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      color: brg.settings.textColorDark
      text: monMove.getMaxPP
      font.pixelSize: 13
      font.capitalization: Font.Capitalize
    }

    IconButtonSquare {
      visible: monMove.moveID !== 0
      Layout.alignment: Qt.AlignVCenter
      Layout.leftMargin: 8

      icon.width: 7
      icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
      icon.color: brg.settings.textColorDark

      onClicked: { moveSelMenu.open(); }

      Menu {
        id: moveSelMenu

        Menu {
          title: qsTr("PP Up")

          MenuItem { text: "Currently: " + monMove.ppUp + "/3"}
          MenuSeparator { }
          MenuItem { text: "Max Out"; onTriggered: { monMove.maxPpUp(); } enabled: monMove.ppUp < 3 }
          MenuItem { text: "Raise"; onTriggered: { monMove.raisePpUp(); } enabled: monMove.ppUp < 3 }
          MenuItem { text: "Lower"; onTriggered: { monMove.lowerPpUp(); } enabled: monMove.ppUp > 0 }
          MenuItem { text: "Reset"; onTriggered: { monMove.resetPpUp(); } enabled: monMove.ppUp > 0 }
        }

        Menu {
          title: qsTr("All Moves")

          MenuItem { text: "Clear All"; onTriggered: { boxData.clearMoves(); } }
          MenuItem { text: "Re-Roll All"; onTriggered: { boxData.randomizeMoves(); } }
          MenuItem { text: "Correct All"; onTriggered: { boxData.correctMoves(); } }
          MenuItem { text: "Move Empty"; onTriggered: { boxData.cleanupMoves(); } }
          MenuItem { text: "Correct / Move"; onTriggered: {
              boxData.correctMoves();
              boxData.cleanupMoves();
            }
          }
        }

        MenuItem { text: "Restore PP"; onTriggered: { monMove.restorePP(); } enabled: !monMove.isMaxPP }
        MenuItem { text: "Re-Roll"; onTriggered: { monMove.randomize(); } }
        MenuItem { text: "Correct Move"; onTriggered: { monMove.correctMove(); } }

        MenuSeparator { }
        MenuItem { text: "Close" }
      }
    }
  }
}
