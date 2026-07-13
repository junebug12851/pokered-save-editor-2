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
  ContrastPicker.qml -- the map's palette, as a percentage you can slide.

  The chip reads as brightness (100% = a normal screen, 0% = black), because that is what a person
  means by contrast. Underneath it is `wMapPalOffset`, and it is NOT a brightness dial at all: the
  game SUBTRACTS the byte from a pointer into its fade-palette table. `0 / 3 / 6 / 9` land on real
  entries -- the four levels the game ships -- and everything else reads ACROSS THE SEAM between
  two of them: the six glitch palettes. (notes/reference/palettes.md)

  So the slider has FOUR segments by default. Flip the switch and it grows to TEN: the six
  in-between values appear as their own, differently-coloured segments, because they are not levels
  and pretending they are would be a lie. They are still fully editable -- every value the save can
  hold is the user's to set -- they are just labelled as what they are.

  ⚠️ Contrast 1 and 2 look NORMAL on the map. Their damage is in the sprite palette (rOBP0), so it
  only shows on the player. The picker says so, because a "glitch" that appears to do nothing is the
  most confusing kind.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: root

  implicitWidth: chip.implicitWidth
  implicitHeight: 24

  /// Show the six glitch values as their own segments.
  property bool showGlitch: brg.map.contrastIsGlitch   // a save already holding one has nothing to hide

  /// Drive the drop-down open/shut by name (the DEBUG harness). @see MapPicker.
  property bool openState: false
  onOpenStateChanged: openState ? pop.open() : pop.close()

  readonly property var levels: [0, 3, 6, 9]
  readonly property var allValues: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
  readonly property var values: showGlitch ? allValues : levels

  function nameOf(v) {
    switch (v) {
      case 0: return qsTr("Normal");
      case 3: return qsTr("Dark");
      case 6: return qsTr("Cave (needs Flash)");
      case 9: return qsTr("Black");
    }
    return qsTr("Glitch %1").arg(v);
  }

  Rectangle {
    id: chip

    anchors.fill: parent
    implicitWidth: chipRow.implicitWidth + 18
    radius: 12

    color: pop.opened ? "#e8e8e8" : (chipHover.hovered ? "#f0f0f0" : "#ffffff")
    border.width: 1
    border.color: brg.map.contrastIsGlitch ? brg.settings.errorColor : brg.settings.dividerColor

    Behavior on color { ColorAnimation { duration: 90 } }

    HoverHandler { id: chipHover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: pop.opened ? pop.close() : pop.open() }

    RowLayout {
      id: chipRow
      anchors.centerIn: parent
      spacing: 6

      // A little swatch of the four greys this palette actually produces. The control shows you the
      // answer, not just the number.
      Row {
        spacing: 0
        Repeater {
          model: 4
          Rectangle {
            required property int index
            width: 4
            height: 12
            color: {
              // The identity palette's four shades, dimmed by the level. Indicative -- the map
              // itself is drawn through the REAL rBGP byte in C++.
              const shades = ["#ffffff", "#a8a8a8", "#545454", "#000000"];
              const dim = brg.map.contrast / 9.0;
              return Qt.tint(shades[index], Qt.rgba(0, 0, 0, dim * 0.9));
            }
          }
        }
      }

      Text {
        text: qsTr("%1%").arg(brg.map.contrastPercent)
        font.pixelSize: 12
        font.bold: true
        color: brg.map.contrastIsGlitch ? brg.settings.errorColor : brg.settings.textColorDark
      }

      Text {
        text: "⌄"
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }
    }
  }

  // ── The drop-down: a segmented slider ───────────────────────────────────────────────────────
  Popup {
    id: pop

    y: root.height + 4
    x: -100
    width: 300
    padding: 10

    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    background: Rectangle {
      color: "#ffffff"
      radius: 6
      border.width: 1
      border.color: brg.settings.dividerColor
    }

    ColumnLayout {
      anchors.fill: parent
      spacing: 8

      RowLayout {
        Layout.fillWidth: true

        Text {
          Layout.fillWidth: true
          text: qsTr("Palette")
          font.pixelSize: 11
          font.bold: true
          color: brg.settings.textColorMid
        }

        Text {
          text: root.nameOf(brg.map.contrast)
          font.pixelSize: 11
          color: brg.map.contrastIsGlitch ? brg.settings.errorColor : brg.settings.textColorDark
        }
      }

      // The segments. Drag along them; each one is a value the save can hold. The four real levels
      // read in the accent; the six glitch values read in the error colour, because they are not
      // levels -- they are a read that lands between two of them.
      Item {
        id: track

        Layout.fillWidth: true
        implicitHeight: 30

        readonly property int count: root.values.length
        readonly property real seg: width / Math.max(1, count)

        function pick(x) {
          const i = Math.max(0, Math.min(track.count - 1, Math.floor(x / track.seg)));
          brg.map.contrast = root.values[i];
        }

        Row {
          anchors.fill: parent
          spacing: 0

          Repeater {
            model: root.values

            Rectangle {
              required property var modelData
              required property int index

              width: track.seg
              height: track.height

              readonly property bool glitch: (modelData % 3) !== 0
              readonly property bool active: brg.map.contrast === modelData

              color: active
                     ? (glitch ? brg.settings.errorColor : brg.settings.accentColor)
                     : (glitch ? Qt.rgba(brg.settings.errorColor.r, brg.settings.errorColor.g,
                                         brg.settings.errorColor.b, 0.12)
                               : "#f2f2f2")

              border.width: 1
              border.color: glitch
                            ? Qt.rgba(brg.settings.errorColor.r, brg.settings.errorColor.g,
                                      brg.settings.errorColor.b, 0.45)
                            : brg.settings.dividerColor

              topLeftRadius: index === 0 ? 4 : 0
              bottomLeftRadius: index === 0 ? 4 : 0
              topRightRadius: index === track.count - 1 ? 4 : 0
              bottomRightRadius: index === track.count - 1 ? 4 : 0

              Behavior on color { ColorAnimation { duration: 80 } }

              Text {
                anchors.centerIn: parent
                text: modelData
                font.pixelSize: 10
                font.family: "monospace"
                font.bold: parent.active
                color: parent.active ? brg.settings.textColorLight : brg.settings.textColorMid
              }
            }
          }
        }

        // One handler for the whole strip, so it is a SLIDER, not ten buttons: press anywhere and
        // drag along it and the palette follows your finger.
        MouseArea {
          anchors.fill: parent
          cursorShape: Qt.PointingHandCursor
          onPressed: (mouse) => track.pick(mouse.x)
          onPositionChanged: (mouse) => { if (pressed) track.pick(mouse.x); }
        }
      }

      // ── The switch that reveals the glitch values ────────────────────────────────────────────
      RowLayout {
        Layout.fillWidth: true
        spacing: 8

        MapSwitch {
          checked: root.showGlitch
          enabled: !brg.map.contrastIsGlitch    // a save already on one cannot hide it
          onToggled: root.showGlitch = !root.showGlitch
        }

        Text {
          Layout.fillWidth: true
          text: qsTr("Glitch palettes")
          font.pixelSize: 11
          color: brg.settings.textColorDark
        }
      }

      Text {
        Layout.fillWidth: true
        visible: root.showGlitch
        text: qsTr("The game keeps four palettes — 0, 3, 6 and 9. It reaches them by subtracting "
                   + "this byte from a pointer, so every other value reads across the seam between "
                   + "two of them. Those six exist nowhere in the game, and they are yours to set.")
        font.pixelSize: 10
        color: brg.settings.textColorMid
        wrapMode: Text.WordWrap
      }

      // The one that catches people out.
      Text {
        Layout.fillWidth: true
        visible: brg.map.contrast === 1 || brg.map.contrast === 2
        text: qsTr("⚠ This one leaves the map looking normal — it wrecks the SPRITE palette instead, "
                   + "so it shows on the player, not the world.")
        font.pixelSize: 10
        color: brg.settings.errorColor
        wrapMode: Text.WordWrap
      }
    }
  }
}
