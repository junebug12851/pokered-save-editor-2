// InfoButtonReg.qml -- a round "i" help button that only shows when global
// tooltips are on.
//
// Same as InfoButton but its visibility is gated on brg.settings.infoBtnPressed
// (the header "?" toggle), so it appears only in "help mode".
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
  visible: brg.settings.infoBtnPressed

  property alias toolTipText: toolTip.text
  property alias toolTipEl: toolTip

  MainToolTip {
    id: toolTip
    followGlobalSetting: false
    x: parent.x + 25
    y: parent.y
  }
}
