import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../common/Style.js" as Style

RowLayout {
  spacing: 0

  property string title: "All"
  property int iconSize: 20

  Button {
    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
    flat: true
    display: AbstractButton.TextBesideIcon
    icon.source: "qrc:/assets/fontawesome-icons/dice.svg"
    icon.width: iconSize + 5
    icon.height: iconSize + 5
    icon.color: Style.textColorAccent
    text: "Re-Roll " + title
    Material.foreground: Style.textColorAccent
    font.capitalization: Font.Capitalize
    font.pixelSize: iconSize
  }
}
