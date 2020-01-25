import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Rectangle {
  id: root

  // Because Dark Theme has quite an ugly Brighter Shade which ruins
  // The look and feel and can make things more difficult to read/see
  Material.accent: Material.color(Material.Pink, Material.Shade500)
  color: Material.background

  Text {
    color: Material.foreground
    id: helloText
    text: file.getRecentFile(0)
    anchors.verticalCenter: root.verticalCenter
    anchors.horizontalCenter: root.horizontalCenter
    font.pointSize: 24;
    font.bold: true
  }
}
