import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../../fragments/navbar"
import "../../common/Style.js" as Style

RowLayout {
  spacing: 0

  NavBarButton {
    icon.source: "qrc:/assets/icons/fontawesome/th.svg"
    toolTipText: "Go back to the main menu"
    onClicked: root.changeScreen("home")
  }

  Text {
    text: root.title
    Layout.fillWidth: true
    color: Style.textColorAccent

    font.capitalization: Font.Capitalize
    font.pixelSize: 20
  }

  NavBarButton {
    icon.source: "qrc:/assets/icons/other/question.svg"
    toolTipText: "Toggle tooltips on or off for explanations and help"
    toolTipGlobalSetting: false // An exception
    opacity: (root.infoBtnPressed === true) ? 1.00 : 0.50
    onClicked: root.infoBtnPressed = !root.infoBtnPressed
  }

  NavBarButton {
    icon.source: "qrc:/assets/icons/fontawesome/users.svg"
    toolTipText: "Credits"
    onClicked: root.changeScreen("about")
  }

  NavBarButton {
    icon.source: "qrc:/assets/icons/other/edit-file-2.svg"
    toolTipText: "Actions for the current file like saving"
    onClicked: root.changeScreen("fileTools")
  }

  NavBarButton {
    icon.source: "qrc:/assets/icons/other/new-file.svg"
    toolTipText: "Switch to a new file"
    onClicked: root.changeScreen("newFile")
  }
}
