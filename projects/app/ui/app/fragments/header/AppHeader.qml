import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Controls

// AppHeader.qml -- the app-wide top toolbar (shown above every non-modal screen).
//
// Left side: a home/back button that swaps based on brg.router.homeBtnShown
// (grid icon -> "home"; back arrow -> closeScreen), then the current screen title
// (brg.router.title). Right side: the tooltips on/off toggle (this one button
// intentionally does NOT follow the global tooltip setting -- see the inline
// note), Credits, File Tools, and New File -- each just calls
// brg.router.changeScreen(). A thin Rectangle at the bottom draws the header
// shadow.
ToolBar {
  height: brg.settings.headerHeight
  Material.foreground: brg.settings.textColorLight

  ColumnLayout {
    anchors.fill: parent

    RowLayout {
      Layout.fillWidth: true

      HeaderButton {
        visible: brg.router.homeBtnShown
        icon.source: "qrc:/assets/icons/fontawesome/th.svg"
        toolTipText: "Go back to the main menu"
        onClicked: brg.router.changeScreen("home")
      }

      HeaderButton {
        visible: !brg.router.homeBtnShown
        icon.source: "qrc:/assets/icons/fontawesome/angle-left.svg"
        toolTipText: "Go back to the previous screen"
        onClicked: brg.router.closeScreen();

        icon.width: 10
        icon.height: 20
      }

      Label {
        text: brg.router.title
        verticalAlignment: Qt.AlignVCenter
        Layout.fillWidth: true

        font.capitalization: Font.Capitalize
        font.pixelSize: 20
      }

      HeaderButton {
        icon.source: "qrc:/assets/icons/other/question.svg"
        toolTipText: "Toggle tooltips on or off for explanations and help"
        toolTipGlobalSetting: false // An exception
        opacity: (brg.settings.infoBtnPressed === true) ? 1.00 : 0.50
        onClicked: brg.settings.infoBtnPressed = !brg.settings.infoBtnPressed
      }

      HeaderButton {
        icon.source: "qrc:/assets/icons/fontawesome/users.svg"
        toolTipText: "Credits"
        onClicked: brg.router.changeScreen("about")
      }

      HeaderButton {
        icon.source: "qrc:/assets/icons/other/edit-file-2.svg"
        toolTipText: "Actions for the current file like saving"
        onClicked: brg.router.changeScreen("fileTools")
      }

      HeaderButton {
        icon.source: "qrc:/assets/icons/other/new-file.svg"
        toolTipText: "Switch to a new file"
        onClicked: brg.router.changeScreen("newFile")
      }
    }
    Rectangle {
      height: brg.settings.headerShadowHeight
      Layout.fillWidth: true
      color: brg.settings.primaryColorDark
    }
  }
}
