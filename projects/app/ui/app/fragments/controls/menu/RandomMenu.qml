// RandomMenu.qml -- a small hidden "..." overflow button exposing a Randomize
// action.
//
// A normally-invisible IconButtonSquare (vertical ellipsis) anchored to the right
// of its parent; opening its Menu offers Randomize (emits randomize()) and Close.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

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
