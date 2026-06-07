// ModalClose.qml -- the round "X" close button pinned to a modal's top-right.
//
// A transparent icon-only RoundButton; the caller wires onClicked (typically to
// brg.router.closeScreen()).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

RoundButton {
  display: AbstractButton.IconOnly
  icon.source: "qrc:/assets/icons/fontawesome/times-circle.svg"
  icon.width: 35
  icon.height: 35
  Material.background: "transparent"
  opacity: 0.75

  anchors.top: parent.top
  anchors.topMargin: 8

  anchors.right: parent.right
  anchors.rightMargin: 8
}
