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
// Bound to a PokemonMove (monMove) + its owning PokemonBox (boxData) + slot index
// (moveIndex). Rendered inside MovesTab's grouped panel as a zebra row, so this is
// just the controls (transparent background). Layout, left→right:
//   [type strip] [move name combo, fills] [type chip, FIXED width]
//   [ |←  current(PP|PP-Up) textbox  →| ]  [ / max ]   [ dice | check-double | trash ]
// The columns after the name are fixed-width so rows line up regardless of move
// name / type length. A tab-level PP / PP-Ups view (showPpUps) switches what the
// editable box edits: current/max PP, or current/max PP-Ups (max 3). The |← / →|
// arrow-to-line buttons set the value to its min / max. The per-row action group
// is randomize-this-move / make-this-move-valid / delete-this-move (delete
// compacts the list). The ⋮ overflow menu is gone -- bulk ops live in the tab's
// top bar. getColor() type palette is Bulbapedia (CC-BY-NC-SA); keep the credit.
Item {
  id: root
  property PokemonMove monMove: null
  // The owning Pokemon + this move's slot index -- needed for delete (which
  // compacts the list). A separate component can't see the parent file's
  // properties by bare name, and PokemonMove::parentMon is a plain C++ member
  // (not a Q_PROPERTY), so neither is reachable without passing them in.
  property PokemonBox boxData: null
  property int moveIndex: 0
  property int rowH: 44

  // Tab-level view: false = edit current/max PP, true = edit current/max PP-Ups.
  property bool showPpUps: false

  property bool filled: monMove !== null && monMove.moveID !== 0

  // The current value + its max for the active view.
  readonly property int curVal: !filled ? 0 : (showPpUps ? monMove.ppUp : monMove.pp)
  readonly property int maxVal: !filled ? 0 : (showPpUps ? 3 : monMove.getMaxPP)

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

  // Point the move combo at monMove's current move. DEFERRED via Qt.callLater
  // because brg.moveSelectModel is per-mon and rebuilt asynchronously
  // (PokemonDetails.onCompleted -> monFromBox switches it from the general list to
  // this mon's specific list): syncing inline can compute an index against the
  // wrong model state, leaving the combo showing a different move's NAME while the
  // real type/PP are correct. Deferring runs the lookup after the model settles.
  function syncMoveCombo() {
    if(monMove)
      moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);
  }

  onMonMoveChanged: Qt.callLater(syncMoveCombo)

  // Set the active value to its minimum (0) / maximum (cap), honoring the view.
  function setMin() {
    if(!monMove) return;
    if(showPpUps) monMove.resetPpUp();
    else monMove.pp = 0;
  }
  function setMax() {
    if(!monMove) return;
    if(showPpUps) monMove.maxPpUp();
    else monMove.restorePP();
  }
  // Re-seat the editable box's text from the model for the active view.
  function reseatVal() {
    valEdit.text = filled ? curVal.toString(10) : "";
  }
  onShowPpUpsChanged: reseatVal();

  // A flat icon button used as one segment of a connected, bordered "combo"
  // group (matches the DV/EV tab's SegBtn). Hairline left divider unless first.
  component RowBtn: Button {
    property bool first: false
    property string tip: ""
    flat: true
    display: AbstractButton.IconOnly
    topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
    padding: 5
    icon.color: brg.settings.textColorDark
    Layout.fillHeight: true
    Layout.minimumHeight: 0
    background: Rectangle {
      color: parent.down ? Qt.rgba(0, 0, 0, 0.16)
             : parent.hovered ? Qt.rgba(0, 0, 0, 0.08)
             : "transparent"
      Rectangle {
        visible: !parent.parent.first
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        width: 1
        color: Qt.rgba(0, 0, 0, 0.15)
      }
    }
    MainToolTip { text: tip }
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 4
    anchors.rightMargin: 4
    spacing: 6

    // Type-color accent strip -- the move's type identity. Empty slots show a
    // faint neutral strip so the column edge stays aligned.
    Rectangle {
      Layout.alignment: Qt.AlignVCenter
      Layout.preferredWidth: 5
      Layout.preferredHeight: root.rowH - 14
      radius: 2.5
      color: root.filled ? root.getColor() : Qt.rgba(0, 0, 0, 0.10)
    }

    // Move name combo -- fills the leftover width (so every row's name column is
    // the same width, since everything to its right is fixed).
    SelectMove {
      id: moveSelect
      Layout.fillWidth: true
      Layout.minimumWidth: 64
      Layout.preferredHeight: root.rowH - 8

      onActivated: { if(monMove) monMove.moveID = currentValue; }
      Component.onCompleted: root.syncMoveCombo();

      Connections {
        target: monMove
        function onMoveIDChanged() { root.syncMoveCombo(); }
      }
      Connections {
        target: brg.moveSelectModel
        function onMonChanged() { Qt.callLater(root.syncMoveCombo); }
      }
      Connections {
        target: root.boxData
        ignoreUnknownSignals: true
        function onSpeciesChanged() { Qt.callLater(root.syncMoveCombo); }
      }

      MainToolTip { text: qsTr("Pokémon Move") }
    }

    // Type chip -- FIXED width so the columns after it line up across rows.
    Rectangle {
      visible: root.filled
      Layout.alignment: Qt.AlignVCenter
      Layout.preferredWidth: 58
      Layout.preferredHeight: 20
      radius: 10
      color: Qt.lighter(root.getColor(), 1.35)

      Text {
        anchors.centerIn: parent
        width: parent.width - 8
        text: root.filled ? monMove.moveType : ""
        color: brg.settings.textColorDark
        font.pixelSize: 11
        font.capitalization: Font.Capitalize
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
      }
    }

    // PP / PP-Up editor: |← (min) · current textbox · →| (max), in one bordered
    // group. The middle box is typed; the arrow-to-line buttons set it to its
    // min (0) / max (cap or 3).
    Rectangle {
      visible: root.filled
      Layout.alignment: Qt.AlignVCenter
      Layout.preferredHeight: root.rowH - 12
      implicitWidth: ppGrp.implicitWidth
      radius: 4; color: "transparent"
      border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
      clip: true

      RowLayout {
        id: ppGrp
        anchors.fill: parent
        spacing: 0

        RowBtn {
          first: true
          icon.width: 15; icon.height: 15
          icon.source: "qrc:/assets/icons/fontawesome/arrow-left-to-line.svg"
          enabled: root.curVal > 0
          onClicked: root.setMin();
          tip: root.showPpUps ? qsTr("Set PP-Ups to 0.") : qsTr("Set PP to 0.")
        }

        DefTextEdit {
          id: valEdit
          Layout.alignment: Qt.AlignVCenter
          Layout.preferredHeight: root.rowH - 14
          horizontalAlignment: Text.AlignHCenter
          leftPadding: 6
          rightPadding: 6
          // Reserve room for 2 digits in both views so widths don't jump when the
          // PP / PP-Ups toggle flips.
          Layout.preferredWidth: 2 * font.pixelSize + leftPadding + rightPadding
          maximumLength: root.showPpUps ? 1 : 2
          color: brg.settings.textColorDark
          background: Item {}

          onTextChanged: {
            if(text === "" || !monMove)
              return;
            var v = parseInt(text, 10);
            if(isNaN(v))
              return;
            if(root.showPpUps) {
              if(v < 0 || v > 3) return;
              monMove.ppUp = v;
            } else {
              if(v < 0 || v > 0xFF) return;
              monMove.pp = v;
            }
          }
          Component.onCompleted: root.reseatVal();

          Connections {
            target: monMove
            function onPpChanged()   { if(!root.showPpUps) root.reseatVal(); }
            function onPpUpChanged() { if(root.showPpUps)  root.reseatVal(); }
          }

          MainToolTip { text: root.showPpUps ? qsTr("Current PP-Ups (0–3).") : qsTr("Current PP.") }
        }

        RowBtn {
          icon.width: 15; icon.height: 15
          icon.source: "qrc:/assets/icons/fontawesome/arrow-right-to-line.svg"
          enabled: root.curVal < root.maxVal
          onClicked: root.setMax();
          tip: root.showPpUps ? qsTr("Set PP-Ups to the max (3).") : qsTr("Restore PP to the max.")
        }
      }
    }

    // " / max " for the active view (max PP, or 3 PP-Ups).
    Text {
      visible: root.filled
      Layout.alignment: Qt.AlignVCenter
      Layout.preferredWidth: 28
      horizontalAlignment: Text.AlignHCenter
      color: brg.settings.textColorMid
      text: "/ " + root.maxVal
      font.pixelSize: 12
    }

    // Per-move action group: randomize this move · make this move valid · delete
    // this move (delete compacts the list).
    Rectangle {
      visible: root.filled
      Layout.alignment: Qt.AlignVCenter
      Layout.preferredHeight: root.rowH - 12
      implicitWidth: actGrp.implicitWidth
      radius: 4; color: "transparent"
      border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
      clip: true

      RowLayout {
        id: actGrp
        anchors.fill: parent
        spacing: 0

        RowBtn {
          first: true
          icon.width: 15; icon.height: 15
          icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
          onClicked: { if(monMove) monMove.randomize(); }
          tip: qsTr("Replace this move with a random valid one.")
        }
        RowBtn {
          icon.width: 15; icon.height: 15
          icon.source: "qrc:/assets/icons/fontawesome/check-double.svg"
          onClicked: { if(monMove) monMove.correctMove(); }
          tip: qsTr("Make this move valid for the Pokémon (fix an illegal move / PP).")
        }
        RowBtn {
          icon.width: 14; icon.height: 15
          icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
          onClicked: { if(root.boxData) root.boxData.deleteMoveAt(root.moveIndex); }
          tip: qsTr("Delete this move (the rest slide up to fill the gap).")
        }
      }
    }
  }
}
