/*
  * Copyright 2026 Twilight
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

/*
  LayerChips.qml -- "Show:" the semantic layers of the map.

  Walls, grass, water, warps, doors, ledges, counters. None of these can be seen by looking at
  the art -- a wall and a floor are just two pictures -- so this is the control that lets you
  see them.

  Three rules it is built on:

  * OFF BY DEFAULT. The map is the point. Nothing is drawn over it until you ask, and asking is
    one click. (Twilight's call.)

  * THE CHIP IS THE LEGEND. Each chip carries the exact colour its layer paints in, so there is
    no separate legend row to drift out of sync with the renderer. The colour comes from
    MapEngine, which is the same code that draws it.

  * A CHIP THAT HAS NOTHING TO SHOW SAYS SO. If this map has no grass, the Grass chip is
    disabled and says why -- rather than switching on an empty overlay and leaving you to
    wonder whether the feature is broken.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: root

  implicitHeight: flow.implicitHeight + 12

  // Rebuilt whenever the map, the tileset, or the save's grass/counter tiles change -- all of
  // which change which layers even APPLY here.
  property var layers: []

  function refresh() {
    root.layers = brg.map.valid ? brg.map.layerList() : [];
  }

  Component.onCompleted: refresh()

  Connections {
    target: brg.map
    function onChanged() { root.refresh(); }
    function onOverlayChanged() { root.refresh(); }
  }

  Flow {
    id: flow
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.verticalCenter: parent.verticalCenter
    anchors.leftMargin: 12
    anchors.rightMargin: 12

    spacing: 6

    Text {
      text: qsTr("Show")
      font.pixelSize: 11
      font.bold: true
      color: brg.settings.textColorMid
      height: 26
      verticalAlignment: Text.AlignVCenter
      rightPadding: 4
    }

    Repeater {
      model: root.layers

      // One chip. Its swatch IS the ink the overlay uses -- same value, same source.
      Rectangle {
        id: chip
        required property var modelData

        readonly property bool on: modelData.on
        readonly property bool usable: modelData.applies

        implicitWidth: chipRow.implicitWidth + 16
        implicitHeight: 26
        radius: 13

        // On: filled with its own colour, quietly. Off: a plain outline. Unusable: barely there.
        color: !usable
               ? "transparent"
               : (on ? Qt.rgba(modelData.color.r, modelData.color.g, modelData.color.b, 0.16)
                     : (mouse.containsMouse ? "#f0f0f0" : "transparent"))

        border.width: 1
        border.color: !usable
                      ? Qt.rgba(0, 0, 0, 0.08)
                      : (on ? modelData.color : brg.settings.dividerColor)

        opacity: usable ? 1.0 : 0.45

        Behavior on color { ColorAnimation { duration: 110 } }
        Behavior on opacity { NumberAnimation { duration: 110 } }

        RowLayout {
          id: chipRow
          anchors.centerIn: parent
          spacing: 6

          // The swatch. Hollow when off, solid when on -- so the chip reads at a glance even
          // in a screenshot, or to someone who can't separate the hues.
          Rectangle {
            implicitWidth: 10
            implicitHeight: 10
            radius: 2
            color: chip.on ? chip.modelData.color : "transparent"
            border.width: 1
            border.color: chip.modelData.color
          }

          Text {
            text: chip.modelData.name
            font.pixelSize: 11
            font.bold: chip.on
            color: chip.on ? brg.settings.textColorDark : brg.settings.textColorMid
          }
        }

        MouseArea {
          id: mouse

          // So the DEBUG harness can drive the chips (reference/dev-harness.md) -- which is
          // how the screenshot review captures each layer without a human clicking.
          objectName: "layerChip_" + chip.modelData.layer

          anchors.fill: parent
          hoverEnabled: true
          enabled: chip.usable
          cursorShape: chip.usable ? Qt.PointingHandCursor : Qt.ArrowCursor

          onClicked: brg.map.toggleLayer(chip.modelData.layer)
        }

        // The explanation lives ON the thing. Half of these are words nobody should be
        // expected to already know -- "counter" meaning a shop desk you can talk across -- so
        // the app is where that gets answered.
        ToolTip {
          id: tip
          visible: mouse.containsMouse
          delay: 350
          text: chip.usable
                ? chip.modelData.description
                : qsTr("%1 — none on this map.").arg(chip.modelData.name)

          // `tip.text`, NOT `parent.text`: a contentItem's parent is not the ToolTip, so
          // `parent.text` is undefined -- which QML reports only at RUNTIME, as ten identical
          // "Unable to assign [undefined] to QString" warnings. Caught by tst_qml_screens,
          // which is exactly the class of bug it exists for. (reference/qt-patterns.md)
          contentItem: Text {
            text: tip.text
            font.pixelSize: 11
            color: brg.settings.textColorLight
            wrapMode: Text.WordWrap
            width: 260
          }

          background: Rectangle {
            color: "#e0212121"
            radius: 4
          }
        }
      }
    }

    // Only worth offering once something is on.
    Button {
      visible: brg.map.layers !== 0
      flat: true
      height: 26
      implicitHeight: 26
      font.pixelSize: 11
      text: qsTr("Clear")
      onClicked: brg.map.layers = 0
    }
  }
}
