import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

RoundButton {
  display: AbstractButton.IconOnly
  icon.source: "qrc:/assets/icons/fontawesome/info-circle.svg"
  icon.width: 20
  icon.height: 20
  Material.background: "transparent"
  opacity: 0.75

  x: 15
  y: 15

  property alias toolTipText: toolTip.text
  property alias toolTipEl: toolTip
  property bool followGlobalSetting: false

  MainToolTip {
    id: toolTip
    followGlobalSetting: parent.followGlobalSetting
    x: parent.x + 25
    y: parent.y
  }
}
