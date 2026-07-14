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

/**
 * The OUTPUT palette — a Game Boy Color / Super Game Boy / custom colour filter for the map.
 *
 * ⚠️ **A VIEW SETTING. It changes not one byte of the save**, and it is a different thing from the
 * Contrast chip beside it: contrast decides *which of the four shades* each pixel becomes (it is a
 * real save byte); this decides *what colour each of those four shades is painted*. (Twilight asked
 * for "Game Boy Color filters for the colour output and custom colors", 2026-07-13.)
 *
 * The chip shows the current four-colour swatch. The drop-down offers the presets — **Super Game
 * Boy** is the standout: it paints each map in the game's OWN SGB palette (Pallet green, Vermilion
 * orange…), straight from `pret`'s data. **Custom** reveals four editable swatches.
 *
 * Same chip-and-popup chassis as the Contrast and Music pickers.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
  id: root
  objectName: "colourPicker"   // the DEBUG harness drives the drop-down through this

  implicitWidth: chip.implicitWidth
  implicitHeight: 24

  property bool openState: false
  onOpenStateChanged: openState ? pop.open() : pop.close()

  /// The four colours the CURRENT mode paints — the chip's swatch. Re-asked when the mode changes
  /// (it is a plain method call, so it needs the notifying property as a dependency).
  readonly property var swatch: {
    brg.map.colourMode;   // the dependency
    for (const p of brg.map.colourPresets())
      if (p.mode === brg.map.colourMode)
        return p.swatch;
    return [];
  }

  Rectangle {
    id: chip
    anchors.fill: parent
    implicitWidth: chipRow.implicitWidth + 18
    radius: 12

    color: pop.opened ? "#e8e8e8" : (chipHover.hovered ? "#f0f0f0" : "#ffffff")
    border.width: 1
    border.color: brg.settings.dividerColor

    Behavior on color { ColorAnimation { duration: 90 } }

    HoverHandler { id: chipHover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: pop.opened ? pop.close() : pop.open() }

    RowLayout {
      id: chipRow
      anchors.centerIn: parent
      spacing: 6

      // The four shades this palette produces, as a little strip — the control shows the answer.
      Row {
        spacing: 0
        Repeater {
          model: root.swatch
          Rectangle {
            required property var modelData
            width: 5
            height: 12
            color: modelData
          }
        }
      }

      Text {
        text: "⌄"
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }
    }
  }

  Popup {
    id: pop

    y: root.height + 4
    x: -70
    width: 220
    padding: 10

    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    onClosed: root.openState = false

    background: Rectangle {
      color: "#ffffff"
      radius: 6
      border.width: 1
      border.color: brg.settings.dividerColor
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: 6

      Text {
        text: qsTr("Colour")
        font.pixelSize: 11
        font.bold: true
        color: brg.settings.textColorMid
      }

      // ── The presets ──────────────────────────────────────────────────────────────────────
      Repeater {
        model: brg.map.colourPresets()

        delegate: Rectangle {
          id: row
          required property var modelData

          Layout.fillWidth: true
          implicitHeight: 30
          radius: 5

          readonly property bool active: brg.map.colourMode === modelData.mode

          color: row.active      ? Qt.rgba(0.34, 0.71, 0.91, 0.16)
               : rowHover.hovered ? Qt.rgba(0, 0, 0, 0.06)
               : "transparent"

          RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            spacing: 8

            // The preset's own four-colour strip.
            Row {
              spacing: 0
              Repeater {
                model: row.modelData.swatch
                Rectangle {
                  required property var modelData
                  width: 8
                  height: 16
                  color: modelData
                }
              }
            }

            Text {
              Layout.fillWidth: true
              text: row.modelData.name
              font.pixelSize: 12
              font.bold: row.active
              color: brg.settings.textColorDark
            }
          }

          HoverHandler { id: rowHover; cursorShape: Qt.PointingHandCursor }
          TapHandler { onTapped: brg.map.colourMode = row.modelData.mode }
        }
      }

      Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 2
        visible: brg.map.colourMode === 3   // Custom
        implicitHeight: 1
        color: brg.settings.dividerColor
      }

      // ── The custom swatches ──────────────────────────────────────────────────────────────
      //
      // Only when Custom is chosen. Four colours, lightest to darkest. Clicking one opens the system
      // colour dialog on it.
      RowLayout {
        Layout.fillWidth: true
        visible: brg.map.colourMode === 3
        spacing: 4

        Text {
          text: qsTr("Shades")
          font.pixelSize: 10
          color: brg.settings.textColorMid
        }

        Item { Layout.fillWidth: true }

        Repeater {
          model: 4

          delegate: Rectangle {
            required property int index

            width: 26
            height: 22
            radius: 4

            color: brg.map.customColours()[index]
            border.width: 1
            border.color: swHover.hovered ? "#56b4e9" : brg.settings.dividerColor

            HoverHandler { id: swHover; cursorShape: Qt.PointingHandCursor }
            TapHandler {
              onTapped: {
                colourDialog.shade = index;
                colourDialog.selectedColor = brg.map.customColours()[index];
                colourDialog.open();
              }
            }
          }
        }
      }
    }
  }

  // Qt's own colour dialog, for editing one custom shade. `shade` remembers which one.
  ColorDialog {
    id: colourDialog
    property int shade: 0
    onAccepted: brg.map.setCustomColour(colourDialog.shade, colourDialog.selectedColor)
  }
}
