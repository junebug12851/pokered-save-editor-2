import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls.Material 2.14
import QtQuick.Controls 2.14

// A Footer with 2 Buttons
ToolBar {
  property alias icon1: btn1.icon
  property alias text1: btn1.text
  signal btn1Clicked();

  property alias icon2: btn2.icon
  property alias text2: btn2.text
  signal btn2Clicked();

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

    RowLayout {
      Layout.fillWidth: true
      spacing: 0

      FooterButton {
        id: btn1
        Layout.fillWidth: true
        Layout.fillHeight: true

        onClicked: btn1Clicked();
      }

      FooterButton {
        id: btn2
        Layout.fillWidth: true
        Layout.fillHeight: true

        onClicked: btn2Clicked();
      }
    }
  }
}
