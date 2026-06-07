import QtQuick
import QtQuick.Controls

import "../general"

// HeaderButton.qml -- an icon ToolButton for the app-wide header (AppHeader).
//
// Icon-only button with a built-in MainToolTip. Exposes aliases for the tooltip
// text, whether it follows the global tooltip setting, and its Y; iconSize sets
// both icon dimensions.
ToolButton {
  property int iconSize: 20
  property alias toolTipText: toolTip.text
  property alias toolTipGlobalSetting: toolTip.followGlobalSetting
  property alias toolTipY: toolTip.y

  //flat: true
  display: AbstractButton.IconOnly
  icon.width: iconSize
  icon.height: iconSize

  MainToolTip {
    id: toolTip
    y: 50
  }
}
