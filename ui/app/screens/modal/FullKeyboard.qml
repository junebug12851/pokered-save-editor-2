import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../fragments/modal"
import "../../fragments/controls/name"

Page {
  id: top

  signal toggleExample();
  signal reUpdateExample();
  signal preClose();

  property string placeholder: "%%"
  property string str: ""
  property bool hasBox: false
  property bool is2Line: false
  property bool isPersonName: false
  property bool isPlayerName: false

  // Doesn't update for some reason on it's on. Used to, doesn't anymore, no
  // idea why.
  onStrChanged: {
    nameDisplay.str = str;
  }

  header: ToolBar {
    height: 75
    Material.background: brg.settings.textColorLight

    Row {
      anchors.centerIn: parent

      NameEdit {
        id: nameEdit
        text: top.str
        onTextChanged: {
          if(brg.fonts.countSizeOf(text) <= 10)
            top.str = text;
        }

        width: 10 * 8 * 2

        disableAcceptBtn: true
        disableKeyboardBtn: true
        disableMenu: true

        // Allows taking extra actions
        IconButtonSquare {
          id: menuBtn

          icon.width: 7
          icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

          anchors.top: parent.top
          anchors.topMargin: -5

          anchors.left: parent.right
          anchors.leftMargin: -7

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
    height: (top.hasBox) ? nameDisplay.height + (8 * 2) : 75
    Material.background: brg.settings.textColorLight

    Row {
      anchors.centerIn: parent

      NameDisplay {
        id: nameDisplay

        placeholder: top.placeholder
        str: top.str
        hasBox: top.hasBox
        is2Line: top.is2Line
        isPersonName: top.isPersonName
        isPlayerName: top.isPlayerName

        disableEditor: true
        disableAutoPlaceholder: true
      }
    }
  }
}
