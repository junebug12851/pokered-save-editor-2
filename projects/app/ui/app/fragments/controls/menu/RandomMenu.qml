import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../general"

IconButtonSquare {
  id: top
  visible: false

  signal randomize();

  anchors.top: parent.top
  anchors.topMargin: -12
  anchors.left: parent.right

  icon.width: 7
  icon.source: "qrc:/assets/icons/fontawesome/ellipsis-v.svg"

  onClicked: menu.open();

  Menu {
    id: menu

    MenuItem {
      text: "Randomize";
      onTriggered: randomize();
    }

    MenuItem {
      text: "Close"
      onTriggered: menu.close();
    }
  }
}
