import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

RoundButton {
  display: AbstractButton.IconOnly
  icon.source: "qrc:/assets/fontawesome-icons/times-circle.svg"
  icon.width: 35
  icon.height: 35
  icon.color: Style.textColorAccent
  Material.background: Qt.darker(Style.primaryColorDark, 1.50)

  x: parent.width - icon.width * 2
  y: icon.width * 0.60
}
