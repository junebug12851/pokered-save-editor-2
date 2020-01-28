import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../fragments"
import "../common/Style.js" as Style

RowLayout {
  spacing: 0

  property string title: "Home"
  property int iconSize: 20

  Button {
    flat: true
    display: AbstractButton.IconOnly
    icon.source: "qrc:/assets/fontawesome-icons/th.svg"
    icon.width: iconSize
    icon.height: iconSize
    icon.color: Style.textColorAccent
  }

  Text {
    text: title
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
  }

  Button {
    flat: true
    display: AbstractButton.IconOnly
    icon.source: "qrc:/assets/other-icons/edit-file-2.svg"
    icon.width: iconSize
    icon.height: iconSize
    icon.color: Style.textColorAccent
  }

  Button {
    flat: true
    display: AbstractButton.IconOnly
    icon.source: "qrc:/assets/other-icons/new-file.svg"
    icon.width: iconSize
    icon.height: iconSize
    icon.color: Style.textColorAccent

    onClicked: root.changeScreen("newFile")
  }
}
