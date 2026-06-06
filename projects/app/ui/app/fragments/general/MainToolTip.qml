import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ToolTip {
  id: toolTip

  Material.background: brg.settings.textColorDark
  Material.foreground: brg.settings.textColorLight

  property bool followGlobalSetting: true

  // If followGlobalSetting, then only when mouse is hovered and global
  // tooltips have been enabled. If not following global setting then only
  // when the mouse is hovered
  visible: (followGlobalSetting)
           ? parent.hovered && brg.settings.infoBtnPressed
           : parent.hovered

  delay: 250
  font.pixelSize: 12
}
