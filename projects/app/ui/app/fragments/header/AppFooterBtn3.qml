import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls.Material 2.14
import QtQuick.Controls 2.14

// A Footer with 3 Buttons
ToolBar {
  property alias icon1: btn1.icon
  property alias text1: btn1.text
  property alias btn1: btn1
  signal btn1Clicked();

  property alias icon2: btn2.icon
  property alias text2: btn2.text
  property alias btn2: btn2
  signal btn2Clicked();

  property alias icon3: btn3.icon
  property alias text3: btn3.text
  property alias btn3: btn3
  signal btn3Clicked();

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

      FooterButton {
        id: btn3
        Layout.fillWidth: true
        Layout.fillHeight: true

        onClicked: btn3Clicked();
      }
    }
  }
}
