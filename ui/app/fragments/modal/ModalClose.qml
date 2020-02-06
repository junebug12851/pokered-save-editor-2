import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

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
