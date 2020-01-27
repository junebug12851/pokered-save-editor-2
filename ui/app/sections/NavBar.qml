import QtQuick 2.14
import QtQuick.Shapes 1.14
import QtQuick.Layouts 1.14
import QtQuick.Controls.Material 2.14

import "../contents"
import "../common/Style.js" as Style

ColumnLayout {
  spacing: 0

  Rectangle {
    id: mainBar
    height: 60
    color: Style.accentColor
    Layout.fillWidth: true

    NavBarButtons {
      anchors.fill: parent
    }
  }

  Shape {
    id: shadow
    height: 40
    smooth: true
    antialiasing: true
    layer.enabled: true
    layer.samples: 4
    vendorExtensionsEnabled: false
    Layout.fillWidth: true

    property color color1: Style.accentColor
    property color color2: Style.accentColorDark

    property real ySplitPercent: .5
    property int y1: shadow.height * ySplitPercent;
    property int y2: shadow.height;
    property int xMax: shadow.width;

    ShapePath {
      id: shadow1
      fillColor: shadow.color1

      startX: -1
      startY: 0

      PathLine { x: -1; y: shadow.y1 }
      PathLine { x: shadow.xMax + 1; y: 0 }
    }

    ShapePath {
      id: shadow2
      fillColor: shadow.color2

      startX: -1
      startY: shadow.y1 - 1

      PathLine { x: -1; y: shadow.y2 }
      PathLine { x: shadow.xMax + 1; y: shadow.y1 }
      PathLine { x: shadow.xMax + 1; y: -1 }
    }
  }
}
