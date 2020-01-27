import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

RowLayout {
  id: top
  spacing: 0

  property string title: "All"
  property int iconSize: 20

  Item {
    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
    width: icon.width + text.width - 10
    height: icon.height

    RowLayout {
      spacing: -12

      Button {
        id: icon
        hoverEnabled: false
        highlighted: false
        down: false
        flat: true
        display: AbstractButton.IconOnly
        icon.source: "qrc:/assets/fontawesome-icons/dice.svg"
        icon.width: iconSize + 5
        icon.height: iconSize + 5
        icon.color: Style.textColorAccent
        enabled: false
      }

      Text {
        id: text
        text: "Re-Roll " + top.title
        color: Style.textColorAccent
        font.capitalization: Font.Capitalize
        font.pixelSize: iconSize
      }
    }
  }
}
