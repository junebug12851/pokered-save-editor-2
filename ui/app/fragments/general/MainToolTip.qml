import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

ToolTip {
  id: toolTip

  Material.background: settings.textColorDark
  Material.foreground: settings.textColorLight

  property bool followGlobalSetting: true

  // If followGlobalSetting, then only when mouse is hovered and global
  // tooltips have been enabled. If not following global setting then only
  // when the mouse is hovered
  visible: (followGlobalSetting)
           ? parent.hovered && settings.infoBtnPressed
           : parent.hovered

  delay: 250
  font.pixelSize: 12
}
