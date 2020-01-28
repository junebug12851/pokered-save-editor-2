import QtQuick 2.14
import QtQuick.Shapes 1.14
import QtQuick.Layouts 1.14
import QtQuick.Controls.Material 2.14

import "../contents"
import "../common/Style.js" as Style

/*
 * Footer with 1 Button
*/

ColumnLayout {
  spacing: 0

  property alias subFooterSrc: buttons.source
  property alias buttonsInner: buttons.item

  Shape {
    id: shadow
    height: 40
    smooth: true
    antialiasing: true
    layer.enabled: true
    layer.samples: 4
    vendorExtensionsEnabled: false
    Layout.fillWidth: true

    property color color1: "white"
    property color color2: Style.accentColorDark

    property real ySplitPercent: .5
    property int y1: shadow.height * ySplitPercent;
    property int y2: shadow.height;
    property int xMax: shadow.width;

    ShapePath {
      fillColor: shadow.color1

      startX: -1
      startY: shadow.y2 + 1

      PathLine { x: shadow.xMax + 1; y: shadow.y2 + 1 }
      PathLine { x: shadow.xMax + 1; y: shadow.y1 }
    }

    ShapePath {
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
    id: buttonsParent
    height: 60
    color: "white"
    Layout.fillWidth: true

    Loader {
      id: buttons
      anchors.fill: parent

      onLoaded: {
        shadow.color1 = Qt.binding(function() { return buttonsInner.color });
        buttonsParent.color = Qt.binding(function() { return buttonsInner.color });
      }
    }
  }
}
