import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../name"
import "../../general"

// NameFullEdit.qml --
// The name row of the full keyboard, and the thing that decides which of the screen's
// two MODES you're in.
//
//   KEYBOARD MODE (the default): the field is a read-only display of what the deck is
//   building -- dark grey text with a soft caret pulsing at the end of it. The caret is
//   the invitation: it says "the cursor is here, go on, type". Backspace on the deck
//   removes a whole TILE. The pen button switches to...
//
//   EDIT MODE: it becomes an ordinary text field -- caret, selection, Ctrl+C/V/Z and a
//   character-by-character Backspace. It LIVE-UPDATES as you type, so the preview at the
//   bottom of the screen keeps up with you. The keyboard fades out and goes dead (it has
//   no say in what you type, so it shouldn't pretend to), and the pen is replaced by a
//   check (apply) and a cross (discard).
//
// Discard means discard: the value you had when you entered edit mode is kept, and the
// cross puts it back -- even though everything you typed was live along the way.
RowLayout {
  id: top

  property string str: ""
  // Kept for backwards compatibility with NameFullHeader's wiring; the field is
  // a fixed comfortable width rather than sized off the tile count.
  property int chopLen: 0
  property int sizeMult: 0
  property bool isPersonName: false
  property bool hasBox: false

  property bool editMode: false

  // What `str` was when edit mode began -- what the cross restores.
  property string strBeforeEdit: ""

  signal editStarted()
  signal editEnded()

  spacing: 5

  onStrChanged: {
    // Don't fight the caret: in edit mode the FIELD is the source and str follows it.
    if(!top.editMode)
      nameEdit.text = top.str;
  }

  function beginEdit() {
    if(top.editMode)
      return;

    top.strBeforeEdit = top.str;
    nameEdit.text = top.str;
    top.editMode = true;
    nameEdit.focusField();
    top.editStarted();
  }

  function applyEdit() {
    top.endEdit();
  }

  function discardEdit() {
    top.str = top.strBeforeEdit;
    nameEdit.text = top.strBeforeEdit;
    top.endEdit();
  }

  function endEdit() {
    top.editMode = false;
    top.editEnded();
  }

  // The name is full and the key you pressed won't fit. Refusing in SILENCE would
  // read as a broken key, so the field shakes -- the way a locked door rattles.
  //
  // It animates a Translate, NOT the field's x: x belongs to the RowLayout, and
  // fighting a layout over a position is how you get a control that never sits still.
  function reject() {
    rejectShake.restart();
  }

  SequentialAnimation {
    id: rejectShake
    NumberAnimation { target: shakeT; property: "x"; to: -5; duration: 45 }
    NumberAnimation { target: shakeT; property: "x"; to:  5; duration: 60 }
    NumberAnimation { target: shakeT; property: "x"; to: -3; duration: 50 }
    NumberAnimation { target: shakeT; property: "x"; to:  0; duration: 45 }
  }

  Item {
    id: fieldWrap

    Layout.preferredWidth: 268
    Layout.preferredHeight: 34
    Layout.alignment: Qt.AlignVCenter

    transform: Translate { id: shakeT }

    NameEdit {
      id: nameEdit
      anchors.fill: parent

      // Read-only in keyboard mode: the deck is what's typing, and a caret you can move
      // in a field you can't type into is a lie.
      readOnly: !top.editMode

      isPersonName: top.isPersonName
      selectedColor: brg.settings.accentColor

      // Grey, not black, while the deck owns the text -- it reads as "shown to you"
      // rather than "being edited by you", and it goes full-strength in edit mode.
      textColor: top.editMode
                 ? brg.settings.textColorDark
                 : Qt.lighter(brg.settings.textColorDark, 2.4)

      disableAcceptBtn: true
      disableKeyboardBtn: true
      disableRandomize: true

      // LIVE. Everything typed here reaches the name immediately, so the preview at the
      // bottom of the screen keeps up. Discard is handled by remembering what it was.
      onTextChanged: {
        if(top.editMode && brg.fonts.countSizeOf(nameEdit.text) <= 10)
          top.str = nameEdit.text;
      }

      onAccepted: top.applyEdit();   // Enter inside the field applies

      Keys.onEscapePressed: (event) => {
        top.discardEdit();
        event.accepted = true;
      }
    }

    // ---- The keyboard-mode caret ----
    // A soft pulse at the end of the text. It isn't a real caret (the field is read-only
    // and has none) -- it's the invitation to type, sitting exactly where the next key
    // will land.
    Rectangle {
      id: caret

      visible: !top.editMode
      width: 2
      height: 17
      radius: 1
      color: brg.settings.accentColor

      x: nameEdit.textEndX + 1
      anchors.verticalCenter: parent.verticalCenter
      anchors.verticalCenterOffset: -1

      SequentialAnimation on opacity {
        running: caret.visible
        loops: Animation.Infinite
        NumberAnimation { to: 0.15; duration: 620; easing.type: Easing.InOutSine }
        NumberAnimation { to: 0.95; duration: 620; easing.type: Easing.InOutSine }
      }
    }
  }

  // ---- The buttons ----
  // All of them carry the SAME chrome now. Before, some had a background and some had
  // none, so a row of five buttons read as two unrelated groups of controls.

  IconButtonSquare {
    visible: !top.editMode
    Layout.alignment: Qt.AlignVCenter
    trayed: true
    icon.width: 15
    icon.height: 15
    icon.source: "qrc:/assets/icons/fontawesome/pen.svg"
    onClicked: top.beginEdit();

    MainToolTip { text: "Edit the text directly (turns the keyboard off)" }
  }

  IconButtonSquare {
    visible: !top.editMode
    Layout.alignment: Qt.AlignVCenter
    trayed: true
    icon.source: "qrc:/assets/icons/fontawesome/backspace.svg"
    onClicked: top.str = "";

    MainToolTip { text: "Clear the name" }
  }

  IconButtonSquare {
    visible: !top.editMode
    Layout.alignment: Qt.AlignVCenter
    trayed: true
    icon.width: 16
    icon.height: 16
    icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
    onClicked: top.str = top.isPersonName
               ? brg.randomPlayerName.randomExample()
               : brg.randomPokemonName.randomExample();

    MainToolTip { text: "Randomize the name" }
  }

  IconButtonSquare {
    visible: top.editMode
    Layout.alignment: Qt.AlignVCenter
    trayed: true
    icon.width: 16
    icon.height: 16
    icon.source: "qrc:/assets/icons/fontawesome/check.svg"
    icon.color: brg.settings.accentColor
    onClicked: top.applyEdit();

    MainToolTip { text: "Done editing — keep what you typed" }
  }

  IconButtonSquare {
    visible: top.editMode
    Layout.alignment: Qt.AlignVCenter
    trayed: true
    icon.width: 12
    icon.height: 17
    icon.source: "qrc:/assets/icons/fontawesome/times.svg"
    icon.color: brg.settings.errorColor
    onClicked: top.discardEdit();

    MainToolTip { text: "Discard your edit and put the name back" }
  }
}
