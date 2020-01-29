import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../fragments"
import "../common/Style.js" as Style

RowLayout {
  spacing: 0

  property int iconSize: 20

  Button {
    flat: true
    display: AbstractButton.IconOnly
    icon.source: "qrc:/assets/fontawesome-icons/th.svg"
    icon.width: iconSize
    icon.height: iconSize
    icon.color: Style.textColorAccent

    MainToolTip {
      text: "Go back to the main menu"
      y: 50
    }

    onClicked: root.changeScreen("home")
  }

  Text {
    text: root.title
    Layout.fillWidth: true
    color: Style.textColorAccent

    font.capitalization: Font.Capitalize
    font.pixelSize: iconSize
  }

  Button {
    flat: true
    display: AbstractButton.IconOnly
    icon.source: "qrc:/assets/other-icons/question.svg"
    icon.width: iconSize
    icon.height: iconSize
    icon.color: Style.textColorAccent
    opacity: (root.infoBtnPressed === true) ? 1.00 : 0.50

    MainToolTip {
      text: "Toggle tooltips on or off for explanations and help"
      y: 50
      followGlobalSetting: false // An exception
    }

    onClicked: root.infoBtnPressed = !root.infoBtnPressed
  }

  Button {
    flat: true
    display: AbstractButton.IconOnly
    icon.source: "qrc:/assets/fontawesome-icons/users.svg"
    icon.width: iconSize
    icon.height: iconSize
    icon.color: Style.textColorAccent

    MainToolTip {
      text: "Contributors and Credits"
      y: 50
    }

    onClicked: root.changeScreen("about")
  }

  Button {
    flat: true
    display: AbstractButton.IconOnly
    icon.source: "qrc:/assets/other-icons/edit-file-2.svg"
    icon.width: iconSize
    icon.height: iconSize
    icon.color: Style.textColorAccent

    MainToolTip {
      text: "Actions for the current file like saving"
      y: 50
    }

    onClicked: root.changeScreen("fileTools")
  }

  Button {
    flat: true
    display: AbstractButton.IconOnly
    icon.source: "qrc:/assets/other-icons/new-file.svg"
    icon.width: iconSize
    icon.height: iconSize
    icon.color: Style.textColorAccent

    MainToolTip {
      text: "Switch to a new file"
      y: 50
    }

    onClicked: root.changeScreen("newFile")
  }
}
