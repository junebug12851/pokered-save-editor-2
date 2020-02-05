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

  property bool disableAcceptBtn: false
  property bool disableKeyboardBtn: false
  property bool disableMenu: false

  signal close();
  signal toggleFullKeyboard();
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
  IconBtnSquare {
    visible: !disableMenu
    id: menuBtn

    icon.width: 7
    icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

    anchors.top: parent.top
    anchors.topMargin: -5

    anchors.left: parent.right
    anchors.leftMargin: -7
    onClicked: menu.open();

    NameDisplayMenu {
      id: menu

      isPersonName: txtField.isPersonName
      hasBox: txtField.hasBox

      onChangeStr: txtField.text = val;
      onToggleExample: txtField.toggleExample();
      onReUpdateExample: txtField.reUpdateExample();
    }
  }

  IconBtnRound {
    visible: !disableAcceptBtn
    id: acceptBtn
    icon.source: "qrc:/assets/icons/fontawesome/check.svg"

    anchors.left: menuBtn.right
    anchors.leftMargin: -17

    anchors.bottom: menuBtn.bottom

    onClicked: close();
  }

  IconBtnRound {
    visible: !disableKeyboardBtn
    icon.source: "qrc:/assets/icons/fontawesome/keyboard.svg"
    icon.width: 20

    anchors.left: acceptBtn.right
    anchors.leftMargin: -14

    anchors.bottom: acceptBtn.bottom

    onClicked: toggleFullKeyboard();
  }
}
