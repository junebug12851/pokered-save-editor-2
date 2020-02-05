import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/modal"
import "../../fragments/controls/name"
import "../../common/Style.js" as Style

Rectangle {
  id: top

  anchors.fill: root

  signal toggleExample();
  signal reUpdateExample();

  property string str: ""
  property bool isPersonName: false
  property bool hasBox: false

  color: Style.primaryColorLight

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    Rectangle {
      height: 75
      Layout.fillWidth: true
      border.color: Style.primaryColorDark
      color: Style.primaryColorLight

      NameEdit {
        anchors.centerIn: parent

        text: top.str
        onTextChanged: {
          if(fonts.countSizeOf(text) <= 10)
            top.str = text;
        }

        width: 100

        disableAcceptBtn: true
        disableKeyboardBtn: true
        disableMenu: true
      }
    }

    Rectangle {
      Layout.fillWidth: true
      Layout.fillHeight: true
      border.color: Style.primaryColorDark
      color: Style.primaryColorLight
    }

    Rectangle {
      height: 75
      Layout.fillWidth: true
      border.color: Style.primaryColorDark
      color: Style.primaryColorLight
    }
  }

  ModalClose {
    anchors.right: parent.right
    anchors.rightMargin: 7

    anchors.top: parent.top
    anchors.topMargin: 7

    onClicked: exitAnim.start()
  }

  // Entrace animation
  NumberAnimation on y {
    from: 1000
    to: 0
    duration: 250
  }

  NumberAnimation on opacity {
    from: 0
    to: 1
    duration: 100
  }

  // Exit animation
  ParallelAnimation {
    id: exitAnim
    NumberAnimation {target: top; property: "y"; to: 1000; duration: 250}
    NumberAnimation {target: top; property: "opacity"; to: 0; duration: 250}

    onFinished: visible = false;
  }
}
