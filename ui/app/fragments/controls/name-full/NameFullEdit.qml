import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../name"
import "../../general"

Row {
  id: top

  signal toggleExample();
  signal reUpdateExample();

  property string str: ""
  property int chopLen: 0
  property int sizeMult: 0
  property bool isPersonName: false
  property bool hasBox: false

  spacing: -16

  onStrChanged: {
    nameEdit.text = top.str
  }

  NameEdit {
    id: nameEdit

    text: top.str
    onTextChanged: {
      if(brg.fonts.countSizeOf(text) <= 10)
        top.str = text;
    }

    selectedColor: brg.settings.accentColor

    width: top.chopLen * top.sizeMult * 8

    disableAcceptBtn: true
    disableKeyboardBtn: true
    disableMenu: true

    topInset: 52
  }

  // Allows taking extra actions
  IconButtonSquare {
    id: menuBtn

    icon.width: 7
    icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

    onClicked: menu.open();

    NameDisplayMenuNoTileset {
      id: menu

      isPersonName: top.isPersonName
      hasBox: top.hasBox

      onChangeStr: top.str = val;
      onToggleExample: top.toggleExample();
      onReUpdateExample: top.reUpdateExample();
    }
  }
}
