import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/general"
import "../../common/Style.js" as Style

/*
 * 1 Randomize Button for footers that display 1 button
*/

Rectangle {
  id: top

  property string title: "All"
  property int iconSize: 20
  color: manualMouse.curColor

  RowLayout {
    anchors.centerIn: parent
    width: icon.width + text.width - 10
    height: icon.height
    spacing: -12

    Button {
      id: icon
      hoverEnabled: false
      highlighted: false
      down: false
      flat: true
      display: AbstractButton.IconOnly
      icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
      icon.width: iconSize + 5
      icon.height: iconSize + 5
      icon.color: Style.textColorAccent
      enabled: false
    }

    Text {
      id: text
      text: "Re-Roll"
      color: Style.textColorAccent
      font.capitalization: Font.Capitalize
      font.pixelSize: iconSize
    }
  }

  ManualButton {
    id: manualMouse
    refColor: Style.accentColor
    anchors.fill: parent

    onClicked: file.data.randomizeExpansion()
  }
}
