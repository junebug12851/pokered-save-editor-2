import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/modal"
import "../../fragments/controls/name"

Page {
  id: top

  signal toggleExample();
  signal reUpdateExample();
  signal preClose();

  property string str: ""
  property bool isPersonName: false
  property bool hasBox: false

  header: ToolBar {
    height: 75
    Material.background: brg.settings.textColorLight

    Row {
      anchors.centerIn: parent

      NameEdit {
        text: top.str
        onTextChanged: {
          if(brg.fonts.countSizeOf(text) <= 10)
            top.str = text;
        }

        width: 100

        disableAcceptBtn: true
        disableKeyboardBtn: true
        disableMenu: true
      }
    }

    ModalClose {
      onClicked: {
        preClose();
        brg.router.closeScreen();
      }
      anchors.topMargin: 3
      anchors.rightMargin: 3
      icon.width: 28
      icon.height: 28
    }
  }

  footer: ToolBar {
    height: 75
    Material.background: brg.settings.textColorLight

    RowLayout {
      spacing: 0
      anchors.fill: parent
    }
  }
}
