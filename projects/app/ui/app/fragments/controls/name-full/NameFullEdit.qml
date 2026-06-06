import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../name"
import "../../general"

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

  NameEdit {
    id: nameEdit

    Layout.preferredWidth: 260
    Layout.alignment: Qt.AlignVCenter

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
