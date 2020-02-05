import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"
import "../../../common/Style.js" as Style

TextField {
  id: txtField

  property bool isPersonName: false
  property bool hasBox: false

  signal close();
  signal changeStr(string val);
  signal toggleExample();
  signal reUpdateExample();

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

  // Allows taking extra actions
  MenuButton {
    id: menuBtn
    anchors.top: parent.top
    anchors.topMargin: -5

    anchors.left: parent.right
    anchors.leftMargin: -7
    onClicked: menu.open();

    NameDisplayMenu {
      id: menu

      isPersonName: txtField.isPersonName
      hasBox: txtField.hasBox

      onChangeStr: txtField.str = val;
      onToggleExample: txtField.toggleExample();
      onReUpdateExample: txtField.reUpdateExample();
    }
  }

  AcceptButton {
    anchors.left: menuBtn.right
    anchors.leftMargin: -20

    anchors.bottom: menuBtn.bottom

    onClicked: close();
  }
}
