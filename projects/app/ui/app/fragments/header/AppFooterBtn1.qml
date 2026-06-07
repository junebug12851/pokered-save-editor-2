import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Controls

// AppFooterBtn1.qml -- a screen footer holding a single FooterButton.
//
// Exposes icon1/text1/btn1 aliases and a btn1Clicked() signal. Draws the footer
// shadow on top, then one full-width button below.
ToolBar {
  property alias icon1: btn.icon
  property alias text1: btn.text
  property alias btn1: btn
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
