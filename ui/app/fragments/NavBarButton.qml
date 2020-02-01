import QtQuick 2.14
import QtQuick.Controls 2.14

import "../fragments"
import "../common/Style.js" as Style

Button {

  property int iconSize: 20
  property alias toolTipText: toolTip.text
  property alias toolTipGlobalSetting: toolTip.followGlobalSetting
  property alias toolTipY: toolTip.y

  flat: true
  display: AbstractButton.IconOnly
  icon.width: iconSize
  icon.height: iconSize
  icon.color: Style.textColorAccent

  MainToolTip {
    id: toolTip
    y: 50
  }
}