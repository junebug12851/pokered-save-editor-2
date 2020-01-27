import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import "../common/Style.js" as Style

RowLayout {
  spacing: 0

  property string title: ""

  Image {
    Layout.fillHeight: true
    width: 50
    source: "/fontawesome-icons/th.svg"
  }
}
