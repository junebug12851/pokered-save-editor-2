// MainToolTip.qml -- the app's standard tooltip.
//
// A styled ToolTip whose visibility depends on followGlobalSetting: when true it
// shows only while hovered AND global tooltips are on (brg.settings.infoBtnPressed,
// the header "?" toggle); when false it shows on hover alone. Place one inside any
// hoverable control and set its text.
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
