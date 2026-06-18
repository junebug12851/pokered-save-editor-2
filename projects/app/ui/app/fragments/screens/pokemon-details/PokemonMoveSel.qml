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
// just the controls. Layout, left→right:
//   [type strip] [move name combo, fills] [type chip, FIXED] [ value(PP|PP-Up) →| ] [ / max ]
// and a per-row action group (randomize / make-valid / delete) that is HIDDEN at
// rest and SLIDES IN from the right edge on hover (an overlay, so it never reflows
// the row).
//
// PP vs PP-Ups is a tab-level view (showPpUps). CRUCIAL: each view has its OWN
// independent text box (ppBox / ppUpBox) bound to its OWN field. They are never
// the same widget, so toggling the view, editing, or hitting "max" can NEVER write
// the wrong field (the earlier single-shared-box design desynced PP and PP-Ups,
// esp. when the maxLength flip truncated the text into a cross-field write).
// getColor() type palette is Bulbapedia (CC-BY-NC-SA); keep the credit.
Item {
  id: root
  property PokemonMove monMove: null
  property PokemonBox boxData: null
  property int moveIndex: 0
  property int rowH: 44

  // Tab-level view: false = edit current/max PP, true = edit current/max PP-Ups.
  property bool showPpUps: false
  // Whether this is an alternate (zebra-tinted) row -- so the hover reveal's
  // backing matches the row exactly.
  property bool altRow: false

  property bool filled: monMove !== null && monMove.moveID !== 0
  readonly property color rowColor: altRow ? Qt.darker(brg.settings.textColorLight, 1.04)
                                           : brg.settings.textColorLight

  clip: true
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
  // because brg.moveSelectModel is per-mon and rebuilt asynchronously: syncing
  // inline can compute an index against the wrong model state.
  function syncMoveCombo() {
    if(monMove)
      moveSelect.currentIndex = brg.moveSelectModel.moveToListIndex(monMove.moveID);
  }

  // When the slot points at a different move object, re-sync the combo and re-seat
  // BOTH value boxes from their own fields.
  onMonMoveChanged: {
    Qt.callLater(syncMoveCombo);
    ppBox.reseat();
    ppUpBox.reseat();
  }

  // A flat icon button -- one segment of a connected, bordered "combo" group
  // (matches the DV/EV tab's SegBtn). Hairline left divider unless first.
  component RowBtn: Button {
    id: rbtn
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
      color: rbtn.down ? Qt.rgba(0, 0, 0, 0.16)
             : rbtn.hovered ? Qt.rgba(0, 0, 0, 0.08)
             : "transparent"
      Rectangle {
        visible: !rbtn.first
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        width: 1
        color: Qt.rgba(0, 0, 0, 0.15)
      }
    }
    MainToolTip { text: rbtn.tip }
  }

  RowLayout {
    id: mainRow
    anchors.fill: parent
    anchors.leftMargin: 4
    // Reserve a lane on the right for the tool-reveal handle so the value box is
    // never under it at rest.
    anchors.rightMargin: 26
    spacing: 6

    // Type-color accent strip. Empty slots show a faint neutral strip.
    Rectangle {
      Layout.alignment: Qt.AlignVCenter
      Layout.preferredWidth: 5
      Layout.preferredHeight: root.rowH - 14
      radius: 2.5
      color: root.filled ? root.getColor() : Qt.rgba(0, 0, 0, 0.10)
    }

    // Move name combo -- fills the remainder (every row's name column is the same
    // width since everything to its right is fixed).
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

    // Value editor: an independent box per view (PP / PP-Up), swapped by a
    // StackLayout, plus the →| "set to max" button. No min/reset button.
    Rectangle {
      visible: root.filled
      Layout.alignment: Qt.AlignVCenter
      Layout.preferredHeight: root.rowH - 12
      implicitWidth: ppStack.implicitWidth
      radius: 4; color: "transparent"
      border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
      clip: true

      StackLayout {
        id: ppStack
        anchors.fill: parent
        // 0 = PP view, 1 = PP-Ups view.
        currentIndex: root.showPpUps ? 1 : 0

        // ---- PP editor (own field: monMove.pp) ----
        RowLayout {
          spacing: 0
          DefTextEdit {
            id: ppBox
            objectName: "movePP" + root.moveIndex
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: root.rowH - 14
            horizontalAlignment: Text.AlignHCenter
            leftPadding: 6; rightPadding: 6
            Layout.preferredWidth: 2 * font.pixelSize + leftPadding + rightPadding
            maximumLength: 2
            color: brg.settings.textColorDark
            background: Item {}

            function reseat() { ppBox.text = root.filled ? monMove.pp.toString(10) : ""; }
            Component.onCompleted: reseat();

            onTextChanged: {
              if(text === "" || !monMove)
                return;
              var v = parseInt(text, 10);
              if(isNaN(v) || v < 0 || v > 0xFF)
                return;
              monMove.pp = v;
            }
            Connections {
              target: monMove
              function onPpChanged() { ppBox.reseat(); }
            }
            MainToolTip { text: qsTr("Current PP.") }
          }
          RowBtn {
            first: true
            icon.width: 15; icon.height: 15
            icon.source: "qrc:/assets/icons/fontawesome/arrow-right-to-line.svg"
            enabled: root.filled && monMove.pp < monMove.getMaxPP
            onClicked: { if(monMove) monMove.restorePP(); }
            tip: qsTr("Restore PP to the max.")
          }
        }

        // ---- PP-Ups editor (own field: monMove.ppUp) ----
        RowLayout {
          spacing: 0
          DefTextEdit {
            id: ppUpBox
            objectName: "movePPUp" + root.moveIndex
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: root.rowH - 14
            horizontalAlignment: Text.AlignHCenter
            leftPadding: 6; rightPadding: 6
            // Same width as the PP box so the cluster doesn't resize on toggle.
            Layout.preferredWidth: 2 * font.pixelSize + leftPadding + rightPadding
            maximumLength: 1
            color: brg.settings.textColorDark
            background: Item {}

            function reseat() { ppUpBox.text = root.filled ? monMove.ppUp.toString(10) : ""; }
            Component.onCompleted: reseat();

            onTextChanged: {
              if(text === "" || !monMove)
                return;
              var v = parseInt(text, 10);
              if(isNaN(v) || v < 0 || v > 3)
                return;
              monMove.ppUp = v;
            }
            Connections {
              target: monMove
              function onPpUpChanged() { ppUpBox.reseat(); }
            }
            MainToolTip { text: qsTr("Current PP-Ups (0–3).") }
          }
          RowBtn {
            first: true
            icon.width: 15; icon.height: 15
            icon.source: "qrc:/assets/icons/fontawesome/arrow-right-to-line.svg"
            enabled: root.filled && monMove.ppUp < 3
            onClicked: { if(monMove) monMove.maxPpUp(); }
            tip: qsTr("Set PP-Ups to the max (3).")
          }
        }
      }
    }

    // " / max " for the active view (max PP, or 3 PP-Ups). Display only -- never
    // writes a field, so switching its text by view is safe.
    Text {
      visible: root.filled
      Layout.alignment: Qt.AlignVCenter
      Layout.preferredWidth: 26
      horizontalAlignment: Text.AlignHCenter
      color: brg.settings.textColorMid
      text: "/ " + (root.showPpUps ? 3 : (root.filled ? monMove.getMaxPP : 0))
      font.pixelSize: 12
    }
  }

  // ---- Per-row tool reveal. At REST a small chevron HANDLE sits in the reserved
  // right lane (the value box stays fully visible + editable). Hovering the handle
  // (or the panel) slides a panel in from the right that covers the value cluster.
  // The panel's background is the ROW colour (it just hides the value box behind
  // it), and the tools are the SAME bordered icon group as the tab's top bar
  // (`validate · random · delete`, dark icons) so they read as part of the screen,
  // not a foreign accent bar. It's an OVERLAY (not in mainRow's flow) + root is
  // clipped, so it's hidden off-right at rest and revealing it never reflows. ----
  Item {
    id: toolReveal
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    // Wide enough to completely cover the value editor + max + "/ max".
    width: 132
    visible: root.filled
    z: 50

    // Revealed when the pointer is on the handle OR on the slid-in panel.
    property bool revealed: (handleHover.hovered || panelHover.hovered) && root.filled

    // Rest-state handle: a subtle tab with a left chevron, in the reserved lane at
    // the far right. Fades out as the panel covers it.
    Rectangle {
      id: handle
      anchors.right: parent.right
      anchors.rightMargin: 2
      anchors.verticalCenter: parent.verticalCenter
      width: 22
      height: root.rowH - 12
      radius: 4
      color: handleHover.hovered ? Qt.rgba(0, 0, 0, 0.10) : Qt.rgba(0, 0, 0, 0.05)
      opacity: toolReveal.revealed ? 0 : 1
      Behavior on opacity { NumberAnimation { duration: 110 } }

      Image {
        anchors.centerIn: parent
        width: 9; height: 14
        sourceSize.width: width; sourceSize.height: height
        source: "qrc:/assets/icons/fontawesome/angle-left.svg"
        fillMode: Image.PreserveAspectFit
        opacity: 0.55
      }

      HoverHandler { id: handleHover }
    }

    // The slid-in panel: a plain ROW-coloured backing that simply hides the value
    // box behind it (no edge/border line), with the bordered tool group at the
    // right. Slides x from off-right (width) to flush (0).
    Item {
      id: panel
      anchors.top: parent.top
      anchors.bottom: parent.bottom
      width: parent.width
      x: toolReveal.revealed ? 0 : width
      Behavior on x { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

      HoverHandler { id: panelHover }

      Rectangle {
        anchors.fill: parent
        color: root.rowColor
      }

      // Bordered icon group, identical styling to the tab's top-bar group. The
      // rightMargin matches the top bar's (10) so the buttons line up in the same
      // columns as the bulk buttons above.
      Rectangle {
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        height: root.rowH - 12
        implicitWidth: toolGrp.implicitWidth
        radius: 4; color: "transparent"
        border.width: 1; border.color: Qt.rgba(0, 0, 0, 0.18)
        clip: true

        RowLayout {
          id: toolGrp
          anchors.fill: parent
          spacing: 0

          RowBtn {
            first: true
            padding: 7
            icon.width: 18; icon.height: 18
            icon.source: "qrc:/assets/icons/fontawesome/file-circle-check.svg"
            onClicked: { if(monMove) monMove.correctMove(); }
            tip: qsTr("Make this move valid for the Pokémon.")
          }
          RowBtn {
            padding: 7
            icon.width: 18; icon.height: 18
            icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
            onClicked: { if(monMove) monMove.randomize(); }
            tip: qsTr("Replace this move with a random valid one.")
          }
          RowBtn {
            padding: 7
            icon.width: 17; icon.height: 18
            icon.source: "qrc:/assets/icons/fontawesome/trash-alt.svg"
            onClicked: { if(root.boxData) root.boxData.deleteMoveAt(root.moveIndex); }
            tip: qsTr("Delete this move (the rest slide up to fill the gap).")
          }
        }
      }
    }
  }
}
