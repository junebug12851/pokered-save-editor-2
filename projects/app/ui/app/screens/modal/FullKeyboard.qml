// FullKeyboard.qml -- the full-screen name-entry modal (the "big keyboard").
//
// Redesigned 2026-07-11 from a filter form + chip list into an actual KEYBOARD: an
// ASDF/QWERTY deck whose 36 alphanumeric keys each hold one game tile, with the
// physical key printed in the corner of every cap. Click a key, or just type it.
// Shift/Ctrl/Alt switch pages -- 255 tiles over 36 keys needs 8 pages, and three
// modifiers give exactly 8 combinations. The map is in C++ (`brg.keyboard`,
// FontKeyboard) and pinned by tst_font_keyboard; the full design is in
// notes/plans/full-keyboard-redesign.md.
//
// Layout: the colour legend rails the left (it explains what the key colours MEAN --
// there is nothing left to filter), the page strip + deck take the middle, and the
// detail split shows whatever key you're hovering on the right.
//
// The `str` property is the single source of truth, fanned out to header, deck and
// preview on change. The header carries the editable str; the footer previews the
// name -- either bare or inside a random example sentence (toggleExample /
// reUpdateExample pull samples from brg.randomExamplePlayer / randomExampleRival /
// randomExamplePokemon). The two longer // comments below explain why the example
// demo is locally owned and why the preview uses anchors (not a layout) -- keep them.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/modal"
import "../../fragments/controls/name"
import "../../fragments/controls/name-full"

Page {
  id: top

  signal preClose();

  property string placeholder: "%%"
  property string str: ""
  property bool hasBox: false
  property bool is2Line: false
  property bool isPersonName: false
  property bool isPlayerName: false

  // The example demo is OWNED here (local) — it never touches the row that
  // opened the keyboard.
  function toggleExample() {
    hasBox = !hasBox;
    reUpdateExample();
  }

  function reUpdateExample() {
    if(!hasBox) {
      placeholder = "%%";
      return;
    }

    if(isPersonName && isPlayerName)
      placeholder = brg.randomExamplePlayer.randomExample();
    else if(isPersonName && !isPlayerName)
      placeholder = brg.randomExampleRival.randomExample();
    else
      placeholder = brg.randomExamplePokemon.randomExample();
  }

  // A key was pressed (clicked or typed). The 10-tile ceiling is the same one the
  // name field enforces -- so check BEFORE writing rather than letting str and the
  // field drift apart. A refusal shakes the field; it is never silent.
  function insertCode(code) {
    var next = top.str.toString() + code;

    if(brg.fonts.countSizeOf(next) > 10) {
      header.reject();
      return;
    }

    top.str = next;
  }

  // Delete a whole <code>, never one character out of the middle of one -- that would
  // leave a string the codec can't round-trip. The rule lives in C++ with the map.
  function backspace() {
    top.str = brg.keyboard.chopLastToken(top.str.toString());
  }

  function commitAndClose() {
    top.preClose();
    brg.router.closeScreen();
  }

  onStrChanged: {
    header.str = top.str
    nameDisplay.str = str;
  }

  // Esc while the NAME FIELD has focus hands the keys back to the deck (rather than
  // closing the screen out from under someone who was only editing text). Esc on the
  // deck itself closes -- the deck handles that one.
  Keys.onEscapePressed: (event) => {
    deck.forceActiveFocus();
    event.accepted = true;
  }

  // Header toolbar
  header: NameFullHeader {
    id: header

    onPreClose: top.preClose();

    chopLen: nameDisplay.chopLen
    sizeMult: nameDisplay.sizeMult
    isPersonName: top.isPersonName
    hasBox: top.hasBox

    str: top.str
    onStrChanged: top.str = str;
  }

  Pane {
    anchors.fill: parent
    padding: 0

    ColumnLayout {
      anchors.fill: parent
      spacing: 0

      // ---- The pages, across the FULL width ----
      // Eight named buttons don't fit a narrow middle column at the app's default
      // 750px window -- they'd clip at both ends. So the strip spans the body.
      PageStrip {
        Layout.fillWidth: true
        Layout.topMargin: 8

        page: deck.page
        onPicked: (p) => {
          deck.setPage(p);
          deck.forceActiveFocus();
        }
      }

      RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 0

        // ---- Left rail: what the key colours mean ----
        ColorLegend {
          Layout.preferredWidth: 132
          Layout.fillHeight: true
          Layout.topMargin: 6
          Layout.leftMargin: 12
        }

        // ---- Middle: the deck ----
        KeyboardDeck {
          id: deck

          Layout.fillWidth: true
          Layout.fillHeight: true
          Layout.bottomMargin: 8

          onInsert: (code) => top.insertCode(code);
          onBackspace: top.backspace();
          onAccept: top.commitAndClose();
          onDismiss: top.commitAndClose();
          onDetail: (info) => detailView.info = info;
        }

        // ---- Right: the detail split ----
        DetailView {
          id: detailView

          Layout.preferredWidth: 212
          Layout.fillHeight: true
        }
      }
    }
  }

  // The deck, not the name field, gets the keys on open -- so you can walk in and
  // start typing. Clicking into the field takes them back (and dims the key legends
  // to show it).
  Component.onCompleted: deck.forceActiveFocus();

  footer: ToolBar {
    // Room for the Name/Example toggle row above the preview.
    height: ((top.hasBox) ? nameDisplay.height + 25 + (8 * 2) : 75) + 44
    Material.background: Qt.lighter(brg.settings.accentColor, 1.50)

    // Toggle the preview between just the Name and an Example sentence, with a
    // next-button to re-roll the example. Anchored (not in a layout) so the
    // NameDisplay below keeps its own width/height bindings — a layout would
    // override them and the box→name toggle would stay box-shaped/distorted.
    RowLayout {
      id: exampleControls
      anchors.top: parent.top
      anchors.topMargin: 6
      anchors.horizontalCenter: parent.horizontalCenter
      spacing: 4

      FlatToggle {
        text: top.hasBox ? "Example" : "Name"
        active: top.hasBox
        onClicked: top.toggleExample();

        MainToolTip {
          text: qsTr("Preview just the name, or the name inside an example sentence.")
        }
      }

      IconButtonSquare {
        visible: top.hasBox
        Layout.alignment: Qt.AlignVCenter
        icon.width: 16
        icon.source: "qrc:/assets/icons/fontawesome/angle-double-right.svg"
        onClicked: top.reUpdateExample();

        MainToolTip { text: "Show a different random example" }
      }
    }

    NameDisplay {
      id: nameDisplay
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.top: exampleControls.bottom
      anchors.topMargin: 8

      placeholder: top.placeholder
      str: top.str
      hasBox: top.hasBox
      is2Line: top.is2Line
      isPersonName: top.isPersonName
      isPlayerName: top.isPlayerName

      disableEditor: true
      disableAutoPlaceholder: true

      centerFeedback: true
      feedbackColorNormal: "#424242"
      feedbackColorWarning: "#ef6c00"
    }
  }
}
