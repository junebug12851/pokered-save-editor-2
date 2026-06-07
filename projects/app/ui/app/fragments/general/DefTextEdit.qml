// DefTextEdit.qml -- the app's default single-line text field.
//
// A styled TextField with an optional left-side Label (labelEl alias), mouse
// selection, accent-colored selection highlight, and vertically centered text.
// The base input control reused across the editors (e.g. the Pokemart cart
// amount fields).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

TextField {
  property alias labelEl: label
  property color selColor: brg.settings.primaryColor

  hoverEnabled: true
  selectByMouse: true
  selectedTextColor: Qt.lighter(selColor, 2)
  selectionColor: selColor
  color: brg.settings.textColorDark
  font.letterSpacing: 2
  font.pixelSize: 14

  // Center the text vertically so it stays put when the field height changes.
  verticalAlignment: TextInput.AlignVCenter
  topPadding: 0
  bottomPadding: 0

  placeholderTextColor: Qt.lighter(brg.settings.textColorDark, 1.25)

  Label {
    id: label

    anchors.right: parent.left
    anchors.rightMargin: 7

    anchors.top: parent.top
    anchors.bottom: parent.bottom

    font.pixelSize: 14

    horizontalAlignment: Text.AlignRight
    verticalAlignment: Text.AlignVCenter
  }
}
