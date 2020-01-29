import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

RoundButton {
  display: AbstractButton.IconOnly
  icon.source: "qrc:/assets/fontawesome-icons/info-circle.svg"
  icon.width: 20
  icon.height: 20
  icon.color: Style.textColorPrimary
  Material.background: "transparent"
  opacity: 0.75

  x: 15
  y: 15

  property alias toolTipText: toolTip.text

  MainToolTip {
    id: toolTip
    followGlobalSetting: false
    x: parent.x + 25
    y: parent.y
  }
}
