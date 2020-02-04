import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../../common/Style.js" as Style

TextField {
  signal close();

  hoverEnabled: true
  bottomInset: 8
  leftInset: 8
  rightInset: 8
  topInset: 8

  selectByMouse: true

  selectedTextColor: Style.textColorAccent
  selectionColor: Qt.darker(Style.primaryColorDark, 1.25)
  color: Style.textColorPrimary
  font.letterSpacing: 2
  font.pixelSize: 14

  placeholderText: "Enter a name"
  placeholderTextColor: Qt.lighter(Style.textColorPrimary, 1.25)

  background: Rectangle {
    color: "transparent"
  }

  AcceptButton {
    anchors.left: parent.right
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 5

    onClicked: close();
  }
}
