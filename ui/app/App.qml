import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Window 2.14
import QtQuick.Controls 2.14
import QtQuick.Controls.Material 2.14

import "./sections"
import "./common/Style.js" as Style

Rectangle {
  id: root
  color: Style.textColorAccent

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    NavBar{
      Layout.fillWidth: true
    }

    Rectangle {
      color: root.color
      Layout.fillHeight: true
      Layout.fillWidth: true
    }

    Footer1 {
      id: footer
      Layout.fillWidth: true
    }
  }
}
