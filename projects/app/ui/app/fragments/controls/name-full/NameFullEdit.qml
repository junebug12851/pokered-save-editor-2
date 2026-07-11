import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../name"
import "../../general"

// NameFullEdit.qml --
// The name input row for the full keyboard: a wide, centered text field with
// Clear and Randomize-Name buttons beside it. (The old ⋮ overflow menu and the
// example actions moved out — example is a toggle above the preview now.)
RowLayout {
  id: top

  property string str: ""
  // Kept for backwards compatibility with NameFullHeader's wiring; the field is
  // a fixed comfortable width rather than sized off the tile count.
  property int chopLen: 0
  property int sizeMult: 0
  property bool isPersonName: false
  property bool hasBox: false

  spacing: 4

  onStrChanged: nameEdit.text = top.str

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

    text: top.str
    onTextChanged: {
      if(brg.fonts.countSizeOf(text) <= 10)
        top.str = text;
    }

    isPersonName: top.isPersonName
    selectedColor: brg.settings.accentColor

    disableAcceptBtn: true
    disableKeyboardBtn: true
    disableRandomize: true
  }

  // Clear the whole field.
  IconButtonSquare {
    id: clearBtn
    Layout.alignment: Qt.AlignVCenter
    icon.source: "qrc:/assets/icons/fontawesome/backspace.svg"
    onClicked: top.str = ""

    MainToolTip { text: "Clear the name" }
  }

  // Randomize the name (replaces the old ⋮ menu).
  IconButtonSquare {
    id: randomizeBtn
    Layout.alignment: Qt.AlignVCenter
    icon.width: 16
    icon.height: 16
    icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
    onClicked: top.str = top.isPersonName
               ? brg.randomPlayerName.randomExample()
               : brg.randomPokemonName.randomExample();

    MainToolTip { text: "Randomize the name" }
  }
}
