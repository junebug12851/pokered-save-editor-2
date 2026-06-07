import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../general"

// NameEdit.qml --
// Quick-edit row: the name field plus its trailing actions. The old ⋮ overflow
// menu is gone (poor UX — too much clicking); its actions are now explicit
// buttons. This row keeps Randomize-Name / Accept / Keyboard; example controls
// live outside (the popup / keyboard own those). Still dual-use: NameFullEdit
// instantiates it as a plain field with all buttons disabled.
RowLayout {
  id: root

  property alias text: txtField.text
  // Forwarded so callers that use NameEdit as just a field (NameFullEdit) can
  // still set the inner field's inset.
  property alias topInset: txtField.topInset
  property bool isPersonName: false

  property bool disableRandomize: false
  property bool disableAcceptBtn: false
  property bool disableKeyboardBtn: false

  property color selectedColor: brg.settings.primaryColor

  signal accepted()
  signal close()
  signal toggleFullKeyboard()

  spacing: 2

  TextField {
    id: txtField
    Layout.fillWidth: true

    hoverEnabled: true
    selectByMouse: true
    selectedTextColor: Qt.lighter(selectedColor, 2)
    selectionColor: selectedColor
    color: brg.settings.textColorDark
    font.letterSpacing: 2
    font.pixelSize: 14

    placeholderText: "Enter a name"
    placeholderTextColor: Qt.lighter(brg.settings.textColorDark, 1.25)

    onAccepted: root.accepted()

    background: Rectangle {
      color: "transparent"
      // Editable affordance: a baseline that brightens on hover/focus.
      Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: (txtField.activeFocus || txtField.hovered) ? 2 : 1
        color: (txtField.activeFocus || txtField.hovered)
               ? brg.settings.accentColor
               : Qt.lighter(brg.settings.textColorMid, 1.3)
      }
    }
  }

  // Randomize Name (replaces the old ⋮ menu).
  IconButtonSquare {
    visible: !disableRandomize
    Layout.alignment: Qt.AlignVCenter
    icon.width: 16
    icon.height: 16
    icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
    icon.color: brg.settings.textColorDark
    onClicked: txtField.text = root.isPersonName
               ? brg.randomPlayerName.randomExample()
               : brg.randomPokemonName.randomExample();

    MainToolTip { text: "Randomize the name" }
  }

  IconButtonRound {
    visible: !disableAcceptBtn
    Layout.alignment: Qt.AlignVCenter
    icon.source: "qrc:/assets/icons/fontawesome/check.svg"
    onClicked: root.close();
  }

  IconButtonRound {
    visible: !disableKeyboardBtn
    Layout.alignment: Qt.AlignVCenter
    icon.source: "qrc:/assets/icons/fontawesome/keyboard.svg"
    icon.width: 20
    onClicked: root.toggleFullKeyboard();
  }
}
