import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

ToolTip {
  id: toolTip
  text: "Home"
  delay: 250
  visible: parent.hovered
  font.pixelSize: 15

  contentItem: Text {
    text: toolTip.text
    font: toolTip.font
    color: Style.textColorAccent
  }

  background: Rectangle {
    color: Qt.darker(Style.primaryColorDark, 3)
    radius: 10
  }
}
