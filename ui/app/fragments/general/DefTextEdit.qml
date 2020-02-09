import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

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

  topPadding: 0

  placeholderTextColor: Qt.lighter(brg.settings.textColorDark, 1.25)

  Label {
    id: label

    anchors.right: parent.left
    anchors.rightMargin: 7

    anchors.top: parent.top
    anchors.bottom: parent.bottom

    font.pixelSize: 14

    horizontalAlignment: Text.AlignRight
  }
}
