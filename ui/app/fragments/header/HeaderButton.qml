import QtQuick 2.14
import QtQuick.Controls 2.14

import "../general"

// Button that goes in the app-wide header
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
