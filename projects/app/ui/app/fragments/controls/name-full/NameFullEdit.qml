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
//   KEYBOARD MODE (the default): the field is read-only and just shows what the deck
//   is building. Backspace on the deck removes a whole TILE. The pen button switches
//   to...
//
//   EDIT MODE: the field becomes a normal text field -- you get the caret, selection,
//   Ctrl+C/V/Z and a character-by-character Backspace, exactly as a text field should
//   behave. The keyboard fades out and goes dead while you're in here (it has no say
//   in what you type, so it shouldn't pretend to). The pen is replaced by a check
//   (apply) and a cross (discard) -- so an edit is a thing you commit or throw away,
//   never a thing that half-happened.
//
// Nothing typed in edit mode reaches `str` until the check is pressed; the cross puts
// the field back to `str`. That's what makes discard actually mean discard.
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

  signal editStarted()
  signal editEnded()

  spacing: 4

  onStrChanged: {
    // Never stomp on what someone is typing mid-edit.
    if(!top.editMode)
      nameEdit.text = top.str;
  }

  function beginEdit() {
    if(top.editMode)
      return;

    nameEdit.text = top.str;
    top.editMode = true;
    nameEdit.focusField();
    top.editStarted();
  }

  // Apply the edit -- but only if it actually fits. Silently truncating (or silently
  // refusing) someone's typing would be worse than saying no.
  function applyEdit() {
    if(brg.fonts.countSizeOf(nameEdit.text) > 10) {
      top.reject();
      return;
    }

    top.str = nameEdit.text;
    top.endEdit();
  }

  function discardEdit() {
    nameEdit.text = top.str;
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

  NameEdit {
    id: nameEdit

    Layout.preferredWidth: 260
    Layout.alignment: Qt.AlignVCenter

    transform: Translate { id: shakeT }

    // Read-only in keyboard mode: the deck is what's typing, and a caret blinking in a
    // field you can't type into is a lie.
    readOnly: !top.editMode

    isPersonName: top.isPersonName
    selectedColor: brg.settings.accentColor

    disableAcceptBtn: true
    disableKeyboardBtn: true
    disableRandomize: true

    onAccepted: top.applyEdit();   // Enter inside the field applies

    Keys.onEscapePressed: (event) => {
      top.discardEdit();
      event.accepted = true;
    }
  }

  // ---- Keyboard mode: edit / clear / randomize ----

  IconButtonSquare {
    visible: !top.editMode
    Layout.alignment: Qt.AlignVCenter
    icon.width: 15
    icon.height: 15
    icon.source: "qrc:/assets/icons/fontawesome/pen.svg"
    onClicked: top.beginEdit();

    MainToolTip { text: "Edit the text directly (turns the keyboard off)" }
  }

  IconButtonSquare {
    visible: !top.editMode
    Layout.alignment: Qt.AlignVCenter
    icon.source: "qrc:/assets/icons/fontawesome/backspace.svg"
    onClicked: top.str = "";

    MainToolTip { text: "Clear the name" }
  }

  IconButtonSquare {
    visible: !top.editMode
    Layout.alignment: Qt.AlignVCenter
    icon.width: 16
    icon.height: 16
    icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
    onClicked: top.str = top.isPersonName
               ? brg.randomPlayerName.randomExample()
               : brg.randomPokemonName.randomExample();

    MainToolTip { text: "Randomize the name" }
  }

  // ---- Edit mode: apply / discard ----

  IconButtonSquare {
    visible: top.editMode
    Layout.alignment: Qt.AlignVCenter
    icon.width: 16
    icon.height: 16
    icon.source: "qrc:/assets/icons/fontawesome/check.svg"
    icon.color: brg.settings.accentColor
    onClicked: top.applyEdit();

    MainToolTip { text: "Apply your edit and go back to the keyboard" }
  }

  IconButtonSquare {
    visible: top.editMode
    Layout.alignment: Qt.AlignVCenter
    icon.width: 12
    icon.height: 17
    icon.source: "qrc:/assets/icons/fontawesome/times.svg"
    icon.color: brg.settings.errorColor
    onClicked: top.discardEdit();

    MainToolTip { text: "Discard your edit" }
  }
}
