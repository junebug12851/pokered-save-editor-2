import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls.Material 2.14
import QtQuick.Controls 2.14

// App-Wide Header
ToolBar {
  height: settings.headerHeight
  Material.foreground: settings.textColorLight

  ColumnLayout {
    anchors.fill: parent

    RowLayout {
      Layout.fillWidth: true

      HeaderButton {
        icon.source: "qrc:/assets/icons/fontawesome/th.svg"
        toolTipText: "Go back to the main menu"
        //onClicked: root.changeScreen("home")
      }

      Label {
        text: settings.title
        verticalAlignment: Qt.AlignVCenter
        Layout.fillWidth: true

        font.capitalization: Font.Capitalize
        font.pixelSize: 20
      }

      HeaderButton {
        icon.source: "qrc:/assets/icons/other/question.svg"
        toolTipText: "Toggle tooltips on or off for explanations and help"
        toolTipGlobalSetting: false // An exception
        opacity: (settings.infoBtnPressed === true) ? 1.00 : 0.50
        onClicked: settings.infoBtnPressed = !settings.infoBtnPressed
      }

      HeaderButton {
        icon.source: "qrc:/assets/icons/fontawesome/users.svg"
        toolTipText: "Credits"
        //onClicked: root.changeScreen("about")
      }

      HeaderButton {
        icon.source: "qrc:/assets/icons/other/edit-file-2.svg"
        toolTipText: "Actions for the current file like saving"
        //onClicked: root.changeScreen("fileTools")
      }

      HeaderButton {
        icon.source: "qrc:/assets/icons/other/new-file.svg"
        toolTipText: "Switch to a new file"
        //onClicked: root.changeScreen("newFile")
      }
    }
    Rectangle {
      height: settings.headerShadowHeight
      Layout.fillWidth: true
      color: settings.primaryColorDark
    }
  }
}
