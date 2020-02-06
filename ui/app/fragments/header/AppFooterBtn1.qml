import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls.Material 2.14
import QtQuick.Controls 2.14

ToolBar {
  height: settings.headerHeight

  Material.foreground: settings.textColorLight

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    Rectangle {
      height: settings.headerShadowHeight
      Layout.fillWidth: true
      color: settings.primaryColorDark
    }

    Button {
      topInset: -20
      bottomInset: -20

      Layout.fillWidth: true
      Layout.fillHeight: true
      flat: true
      display: AbstractButton.TextBesideIcon
      icon.source: "qrc:/assets/icons/fontawesome/dice.svg"
      icon.width: 25
      icon.height: 25
      text: "Re-Roll"
      font.capitalization: Font.Capitalize
      font.pixelSize: 20
    }
  }
}
