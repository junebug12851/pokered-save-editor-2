import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "../fragments"
import "../common/Style.js" as Style

Rectangle {

  color: Style.primaryColorLight

  TextEdit {
    id: textBox
    width: 400
    text: "1234567890"
  }

  NameDisplay {
    anchors.left: textBox.left
    anchors.top: textBox.bottom
    anchors.topMargin: 10
    sizeMult: 4

    str: textBox.text
    hasBox: false
    chopLen: 7
  }
}
