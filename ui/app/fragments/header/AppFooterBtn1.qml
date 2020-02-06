import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls.Material 2.14
import QtQuick.Controls 2.14

// A Footer with 1 Button
ToolBar {
  property alias icon1: btn.icon
  property alias text1: btn.text
  signal btn1Clicked();

  height: brg.settings.headerHeight
  Material.foreground: brg.settings.textColorLight

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    // Shadow
    Rectangle {
      height: brg.settings.headerShadowHeight
      Layout.fillWidth: true
      color: brg.settings.primaryColorDark
    }

    // 1 Button
    FooterButton {
      id: btn
      Layout.fillWidth: true
      Layout.fillHeight: true

      onClicked: btn1Clicked();
    }
  }
}
