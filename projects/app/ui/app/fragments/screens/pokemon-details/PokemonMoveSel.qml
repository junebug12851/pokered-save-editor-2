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

// PokemonMoveSel.qml -- the inner controls of one move row in the Moves tab.
//
// Bound to a PokemonMove (monMove). Rendered inside MovesTab's grouped panel as a
// zebra row, so this component is just the controls (transparent background, no
// pill) following the General-tab design language: a type-COLOR accent strip +
// type chip carry the move's identity, then the move combo (SelectMove, fills the
// leftover width), an editable PP field, "/ maxPP", and a ⋮ overflow menu (PP-Up
// sub-menu, per-move restore/re-roll/correct, All-Moves bulk ops). The grip handle
// and drag-to-reorder live in MovesTab (the row owns the drag; this owns the
// controls). The getColor() type palette is from Bulbapedia (CC-BY-NC-SA) -- keep
// the attribution comment.
Item {
  id: top
  property PokemonMove monMove: null
  // The owning Pokemon -- needed for the All-Moves bulk ops and the species
  // change hook. Passed in by MovesTab (a separate component can't see the
  // parent file's properties by bare name, and PokemonMove::parentMon is a plain
  // C++ member, not a Q_PROPERTY, so it isn't reachable from QML).
  property PokemonBox boxData: null
  property int rowH: 40

  property bool filled: monMove !== null && monMove.moveID !== 0

  implicitHeight: rowH

  // Thanks to Bulbapedia
  // bulbapedia.bulbagarden.net/wiki/Category:Type_color_templates
  // Licensed: CC-BY-NC-SA 2.5 (creativecommons.org/licenses/by-nc-sa/2.5)
  function getColor() {
    if(monMove === null)
      return "#E5D6D0";
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
    anchors.leftMargin: 4
    anchors.rightMargin: 6
    spacing: 8

    // Type-color accent strip -- the move's type identity in the otherwise neutral
    // grouped panel (replaces the old full-row colored pill). Empty slots show a
    // faint neutral strip so the column edge stays aligned.
    Rectangle {
      Layout.alignment: Qt.AlignVCenter
      Layout.preferredWidth: 5
      Layout.preferredHeight: top.rowH - 12
      radius: 2.5
      color: top.filled ? top.getColor() : Qt.rgba(0, 0, 0, 0.10)
    }

    SelectMove {
      id: moveSelect
      // Fill the leftover width so the row fits the pane (keeps the ⋮ inside).
      Layout.fillWidth: true
      Layout.minimumWidth: 60
      Layout.preferredHeight: top.rowH

      onActivated: { if(monMove) monMove.moveID = currentValue; }
      Component.onCompleted: if(monMove) currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);

      Connections {
        target: monMove
        function onMoveIDChanged() { moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID); }
      }

      Connections {
        target: brg.moveSelectModel
        function onMonChanged() {
          if(monMove) moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);
        }
      }

      Connections {
        target: top.boxData
        ignoreUnknownSignals: true
        function onSpeciesChanged() {
          if(monMove) moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);
        }
      }

      MainToolTip {
        text: qsTr("Pokémon Move")
      }
    }

    // Type name chip -- a faint type-tinted pill so the type reads at a glance.
    Rectangle {
      visible: top.filled
      Layout.alignment: Qt.AlignVCenter
      Layout.preferredHeight: 20
      implicitWidth: typeChipText.implicitWidth + 16
      radius: 10
      color: Qt.lighter(top.getColor(), 1.35)

      Text {
        id: typeChipText
        anchors.centerIn: parent
        text: top.filled ? monMove.moveType : ""
        color: brg.settings.textColorDark
        font.pixelSize: 12
        font.capitalization: Font.Capitalize
      }
    }

    DefTextEdit {
      id: movePPEdit
      visible: top.filled

      Layout.alignment: Qt.AlignVCenter
      Layout.preferredHeight: top.rowH - 8
      horizontalAlignment: Text.AlignHCenter

      // PP is at most 2 digits; size for 2 chars + a little padding (trimmed).
      leftPadding: 8
      rightPadding: 8
      Layout.preferredWidth: 2 * font.pixelSize + leftPadding + rightPadding
      maximumLength: 2
      color: brg.settings.textColorDark

      onTextChanged: {
        if(text === "" || monMove === null)
          return;

        var idDec = parseInt(text, 10);
        if(isNaN(idDec))
          return;

        if(idDec < 0 || idDec > 0xFF)
          return;

        monMove.pp = idDec;
      }
      Component.onCompleted: text = monMove ? monMove.pp.toString(10) : "";

      Connections {
        target: monMove
        function onPpChanged() { movePPEdit.text = monMove.pp.toString(10); }
      }

      MainToolTip {
        text: qsTr("Pokémon PP")
      }
    }

    Text {
      visible: top.filled
      Layout.alignment: Qt.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      color: brg.settings.textColorMid
      text: "/"
      font.pixelSize: 13
    }

    Text {
      visible: top.filled
      Layout.alignment: Qt.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      color: brg.settings.textColorMid
      text: top.filled ? monMove.getMaxPP : ""
      font.pixelSize: 13
    }

    IconButtonSquare {
      visible: top.filled
      Layout.alignment: Qt.AlignVCenter
      Layout.leftMargin: 4

      icon.width: 7
      icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"
      icon.color: brg.settings.textColorDark

      onClicked: { moveSelMenu.open(); }

      Menu {
        id: moveSelMenu

        Menu {
          title: qsTr("PP Up")

          MenuItem { text: "Currently: " + (top.filled ? monMove.ppUp : 0) + "/3"}
          MenuSeparator { }
          MenuItem { text: "Max Out"; onTriggered: { if(monMove) monMove.maxPpUp(); } enabled: top.filled && monMove.ppUp < 3 }
          MenuItem { text: "Raise"; onTriggered: { if(monMove) monMove.raisePpUp(); } enabled: top.filled && monMove.ppUp < 3 }
          MenuItem { text: "Lower"; onTriggered: { if(monMove) monMove.lowerPpUp(); } enabled: top.filled && monMove.ppUp > 0 }
          MenuItem { text: "Reset"; onTriggered: { if(monMove) monMove.resetPpUp(); } enabled: top.filled && monMove.ppUp > 0 }
        }

        Menu {
          title: qsTr("All Moves")

          MenuItem { text: "Clear All"; onTriggered: { if(top.boxData) top.boxData.clearMoves(); } }
          MenuItem { text: "Re-Roll All"; onTriggered: { if(top.boxData) top.boxData.randomizeMoves(); } }
          MenuItem { text: "Correct All"; onTriggered: { if(top.boxData) top.boxData.correctMoves(); } }
          MenuItem { text: "Move Empty"; onTriggered: { if(top.boxData) top.boxData.cleanupMoves(); } }
          MenuItem { text: "Correct / Move"; onTriggered: {
              if(top.boxData) {
                top.boxData.correctMoves();
                top.boxData.cleanupMoves();
              }
            }
          }
        }

        MenuItem { text: "Restore PP"; onTriggered: { if(monMove) monMove.restorePP(); } enabled: top.filled && !monMove.isMaxPP }
        MenuItem { text: "Re-Roll"; onTriggered: { if(monMove) monMove.randomize(); } }
        MenuItem { text: "Correct Move"; onTriggered: { if(monMove) monMove.correctMove(); } }

        MenuSeparator { }
        MenuItem { text: "Close" }
      }
    }
  }
}
