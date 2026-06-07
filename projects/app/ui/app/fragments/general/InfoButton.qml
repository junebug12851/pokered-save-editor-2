// InfoButton.qml -- a small round "i" help button with an attached MainToolTip.
//
// Pinned near the top-left of its parent; set toolTipText for the help text. This
// variant is always visible (followGlobalSetting defaults false) -- compare
// InfoButtonReg, which only appears while global tooltips are enabled.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

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
