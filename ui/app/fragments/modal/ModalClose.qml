import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../common/Style.js" as Style

RoundButton {
  display: AbstractButton.IconOnly
  icon.source: "qrc:/assets/icons/fontawesome/times-circle.svg"
  icon.width: 35
  icon.height: 35
  icon.color: Style.textColorPrimary
  Material.background: "transparent"
  opacity: 0.75

  x: parent.width - icon.width * 2
  y: icon.height * 0.60
}
