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
// TWO MODES, and the screen never leaves you guessing which one you're in:
//   KEYBOARD MODE -- the deck is live and the name field is a read-only display of
//   what it's building. Backspace removes a whole TILE.
//   EDIT MODE -- the pen button turns the field into an ordinary text field (caret,
//   selection, Ctrl+C/V/Z, character-by-character Backspace) and the keyboard FADES
//   OUT and goes dead, because it has no say in what you type. Check applies the edit,
//   cross discards it; the header says which mode you're in in words.
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

    // Entering edit mode drops the hover detail: the deck is about to go dead, and a
    // pane still describing the key the cursor happens to be parked on would be
    // describing something you can no longer press.
    onEditStarted: detailView.info = null;

    // Leaving it hands the keys straight back to the deck, so you can carry on typing
    // without having to click anything to "re-arm" it.
    onEditEnded: deck.forceActiveFocus();
  }

  Pane {
    anchors.fill: parent
    padding: 0

    ColumnLayout {
      id: body

      anchors.fill: parent
      spacing: 0

      // ALL of this -- strip, legend, deck, detail -- is "the keyboard". In edit mode
      // it fades out and goes dead as one thing. A keyboard that stayed bright and
      // clickable while it has no say in what you're typing would be lying about who's
      // listening; disabling it without fading it would just look broken.
      readonly property bool kbOn: !header.editMode

      enabled: kbOn
      opacity: kbOn ? 1.0 : 0.25
      Behavior on opacity { NumberAnimation { duration: 160 } }

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
        // Kept tight: every pixel here and in the detail pane comes straight off the
        // deck's key size (the deck is width-limited at the app's default window).
        ColorLegend {
          Layout.preferredWidth: 118
          Layout.fillHeight: true
          Layout.topMargin: 6
          Layout.leftMargin: 10
          // Breathing room between the legend's ⓘ dots and the keyboard's edge.
          Layout.rightMargin: 12
        }

        // ---- Middle: the Simulated bar (tile pages only), the deck, its description ----
        ColumnLayout {
          Layout.fillWidth: true
          Layout.fillHeight: true
          spacing: 4

          // The tileset controls sit with the thing they affect. They're only meaningful
          // on the picture pages, so that's the only place they appear -- and the row
          // ALWAYS holds its height, so showing/hiding them can't resize the keyboard.
          Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 30

            SimulatedBar {
              anchors.fill: parent
              visible: deck.isTilePage
            }
          }

          KeyboardDeck {
            id: deck

            Layout.fillWidth: true
            Layout.fillHeight: true

            onInsert: (code) => top.insertCode(code);
            onBackspace: top.backspace();
            onAccept: top.commitAndClose();
            onDismiss: top.commitAndClose();
            onDetail: (info) => detailView.info = info;
          }

          // A quiet line saying what this page IS -- the page strip gives it a name and a
          // chord; this says what it's for. Fixed height so switching pages never nudges
          // the deck.
          Text {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            Layout.bottomMargin: 4

            text: brg.keyboard.pageDescription(deck.page)
            font.pixelSize: 10
            color: brg.settings.textColorMid
            opacity: 0.9
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
          }
        }

        // ---- Right: the detail split ----
        DetailView {
          id: detailView

          Layout.preferredWidth: 190
          Layout.fillHeight: true
        }
      }
    }
  }

  // The deck, not the name field, gets the keys on open -- so you can walk in and
  // start typing immediately. The field only takes them in edit mode, deliberately.
  Component.onCompleted: deck.forceActiveFocus();

  footer: ToolBar {
    // FIXED HEIGHT -- it must not depend on Name-vs-Example. Sizing it to the preview
    // meant flipping that toggle grew the footer, which shrank the body, which shrank
    // the KEYBOARD: the whole deck re-flowed under your hands because you asked to see an
    // example sentence. The footer now reserves the taller of the two states once, and
    // the preview sits inside it.
    //
    // (The height also has to clear the length feedback -- "Using 3 out of 10 bytes"
    // hangs BELOW nameDisplay.height, so anything sized to that alone clips it off the
    // bottom of the window.)
    //
    // The number: the toggle row (~26) + the Example box preview (8 * 6 * 1.4 = 67) +
    // the feedback line + margins.
    height: 134
    Material.background: brg.settings.textColorLight

    Rectangle {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: parent.top
      height: 1
      color: brg.settings.dividerColor
    }

    // Toggle the preview between just the Name and an Example sentence, with a
    // next-button to re-roll the example. Anchored (not in a layout) so the
    // NameDisplay below keeps its own width/height bindings — a layout would
    // override them and the box→name toggle would stay box-shaped/distorted.
    // Anchored UP from the bottom, not down from the top. The footer's height is FIXED
    // (so Name/Example can't re-flow the deck), but the two previews are very different
    // heights -- and hung off the top, the short one leaves a slab of white beneath it
    // with the byte counter floating in the middle of nowhere. Bottom-up, the counter
    // sits just off the bottom edge in BOTH modes, and the toggle simply rides higher
    // when the example box needs the room.
    RowLayout {
      id: exampleControls
      anchors.bottom: nameDisplay.top
      anchors.bottomMargin: 8
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
      // The 24 is the length feedback, which hangs BELOW this item's height.
      anchors.bottom: parent.bottom
      anchors.bottomMargin: 24

      placeholder: top.placeholder
      str: top.str
      hasBox: top.hasBox
      is2Line: top.is2Line
      isPersonName: top.isPersonName
      isPlayerName: top.isPlayerName

      // Smaller than the default 2. The Example (box) preview is `8 * 6 * sizeMult` tall
      // -- 96px at the default -- and the footer has to reserve that height CONSTANTLY
      // (it must not resize when you flip Name/Example, or the whole keyboard re-flows).
      // At 1.75 the box fits a footer that isn't a slab.
      sizeMult: 1.4

      disableEditor: true
      disableAutoPlaceholder: true

      centerFeedback: true
      feedbackColorNormal: "#424242"
      feedbackColorWarning: "#ef6c00"
    }
  }
}
