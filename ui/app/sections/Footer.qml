import QtQuick 2.14
import QtQuick.Shapes 1.14
import QtQuick.Layouts 1.14
import QtQuick.Controls.Material 2.14

import "../contents"
import "../common/Style.js" as Style

ColumnLayout {
  spacing: 0

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
      startY: shadow.y2 + 1

      PathLine { x: shadow.xMax + 1; y: shadow.y2 + 1 }
      PathLine { x: shadow.xMax + 1; y: shadow.y1 }
    }

    ShapePath {
      id: shadow2
      fillColor: shadow.color2

      // So I have no idea why this is needed, but after more than 2 hours of
      // fiddling around with numbers, positions, and coordinates I simply
      // could not get this to work no matter how hard I tried. So this is a
      // hack so cheat and get it to work so I can move on before I completely
      // lose my mind over this
      strokeColor: shadow.color2
      strokeWidth: 2

      startX: -1
      startY: shadow.y1

      PathLine { x: -1; y: shadow.y2 }
      PathLine { x: shadow.xMax + 1; y: shadow.y1 + 1 }
      PathLine { x: shadow.xMax + 1; y: 0 }
    }
  }

  Rectangle {
    id: buttonBar
    height: 60
    color: Style.accentColor
    Layout.fillWidth: true

    MouseArea {
      id: hoverArea
      anchors.fill: parent
      hoverEnabled: true

      onEntered: {
        buttonBar.color = Qt.darker(Style.accentColor, 1.10)
        shadow.color1 = Qt.darker(Style.accentColor, 1.10)
      }

      onExited: {
        buttonBar.color = Style.accentColor
        shadow.color1 = Style.accentColor
      }

      onPressed: {
        buttonBar.color = Qt.darker(Style.accentColor, 1.40)
        shadow.color1 = Qt.darker(Style.accentColor, 1.40)
      }

      onReleased: {
        buttonBar.color = (hoverArea.containsMouse) ? Qt.darker(Style.accentColor, 1.10) : Style.accentColor
        shadow.color1 = (hoverArea.containsMouse) ? Qt.darker(Style.accentColor, 1.10) : Style.accentColor
      }
    }

    FooterButtons {
      anchors.fill: parent
    }
  }
}
