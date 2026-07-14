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
import QtQuick.Dialogs

Item {
  id: root
  objectName: "contrastPicker"   // the DEBUG harness opens the strip through this

  implicitWidth: trigger.implicitWidth
  implicitHeight: 26

  /// The glitch ink. YELLOW, not red (Twilight, 2026-07-13): a glitch palette is not an ERROR --
  /// the console renders it perfectly happily, and half the reason to have this control is to go
  /// looking for them. Yellow says "unusual, look here"; red says "broken", and it isn't.
  readonly property color glitchColor: "#c9a227"
  readonly property color glitchFill: "#f2d35c"

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

  // ⚠️ THE REACTIVE ICON Twilight named as the example: *"prefer a contrast icon showing the active
  // contrast over a generic contrast button."* The icon IS the four shades this palette actually
  // produces (the real rBGP byte, glitch reads included) — so the button shows the answer, live, and
  // the "%" text is gone with the wordiness. The frame goes yellow on a glitch value.
  MapBarButton {
    id: trigger
    anchors.fill: parent

    open: root.openState
    accent: brg.map.contrastIsGlitch ? root.glitchColor : brg.settings.dividerColor
    onToggle: root.openState = !root.openState

    tip: qsTr("Contrast & colour — %1 (%2%)").arg(root.nameOf(brg.map.contrast))
                                              .arg(brg.map.contrastPercent)

    // The live swatch. Four narrow bands, exactly the ink the map is drawn through.
    Row {
      anchors.verticalCenter: parent.verticalCenter
      spacing: 0

      readonly property var shades: brg.map.contrastShades(brg.map.contrast)

      Repeater {
        model: 4

        Rectangle {
          required property int index
          width: 5
          height: 13
          color: (parent.shades && parent.shades.length === 4) ? parent.shades[index] : "#f2f2f2"
          topLeftRadius: index === 0 ? 2 : 0
          bottomLeftRadius: index === 0 ? 2 : 0
          topRightRadius: index === 3 ? 2 : 0
          bottomRightRadius: index === 3 ? 2 : 0
        }
      }
    }
  }

  // ── The drop-down: a segmented slider ───────────────────────────────────────────────────────
  Popup {
    id: pop

    y: root.height + 4
    x: -20
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
          text: qsTr("Contrast")
          font.pixelSize: 11
          font.bold: true
          color: brg.settings.textColorMid
        }

        Text {
          text: root.nameOf(brg.map.contrast)
          font.pixelSize: 11
          color: brg.map.contrastIsGlitch ? root.glitchColor : brg.settings.textColorDark
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

            // ⚠️ EACH SEGMENT IS PAINTED IN THE PALETTE IT PRODUCES.
            //
            // They used to be accent-blue (real levels) and yellow (glitch ones) -- which told you
            // *that* a value was unusual and nothing whatever about what it would DO. Twilight:
            // *"coloured segments matching the current colours."*
            //
            // So a segment now shows the four real shades that value renders the map in (the genuine
            // rBGP byte, out of MapEngine, glitch reads across the fade table's seam included). Slide
            // along the strip and you watch the map go dark before the map does. The yellow survives
            // as a BORDER on the glitch values -- it still says "unusual, look here", it just no
            // longer stands in for the thing it is describing.
            Rectangle {
              id: seg
              required property var modelData
              required property int index

              width: track.seg
              height: track.height

              readonly property bool glitch: (modelData % 3) !== 0
              readonly property bool active: brg.map.contrast === modelData

              readonly property var shades: brg.map.contrastShades(seg.modelData)

              color: "transparent"

              // The palette itself, as four stacked bands. Real colour, real byte, no interpretation.
              Column {
                anchors.fill: parent
                spacing: 0

                Repeater {
                  model: 4

                  Rectangle {
                    required property int index
                    width: seg.width
                    height: seg.height / 4
                    color: (seg.shades && seg.shades.length === 4)
                             ? seg.shades[index]
                             : "#f2f2f2"
                  }
                }
              }

              // Dim the ones you are NOT on, so the chosen palette is the one that reads.
              Rectangle {
                anchors.fill: parent
                color: "#ffffff"
                opacity: seg.active ? 0.0 : 0.42
                Behavior on opacity { NumberAnimation { duration: 80 } }
              }

              // The border says which is selected, and (in yellow) which are glitch values.
              Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.width: seg.active ? 2 : 1
                border.color: seg.active
                                ? (seg.glitch ? root.glitchColor : brg.settings.accentColor)
                                : (seg.glitch
                                     ? Qt.rgba(root.glitchColor.r, root.glitchColor.g,
                                               root.glitchColor.b, 0.65)
                                     : brg.settings.dividerColor)
              }

              topLeftRadius: index === 0 ? 4 : 0
              bottomLeftRadius: index === 0 ? 4 : 0
              topRightRadius: index === track.count - 1 ? 4 : 0
              bottomRightRadius: index === track.count - 1 ? 4 : 0

              // The number, in a pill -- because it has to stay readable over BLACK and over WHITE,
              // and no single ink does that. This is the only honest way to label a swatch.
              Rectangle {
                anchors.centerIn: parent
                width: num.implicitWidth + 8
                height: 14
                radius: 7
                color: "#cc212121"

                Text {
                  id: num
                  anchors.centerIn: parent
                  text: seg.modelData
                  font.pixelSize: 10
                  font.family: "monospace"
                  font.bold: seg.active
                  color: "#ffffff"
                }
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

        // "Glitch contrast", not "glitch palettes" (Twilight, 2026-07-13) -- the control is called
        // Contrast, so its unusual values are glitch CONTRAST. One word for one thing.
        Text {
          Layout.fillWidth: true
          text: qsTr("Glitch contrast")
          font.pixelSize: 11
          color: brg.settings.textColorDark
        }
      }

      // ══ COLOUR ═══════════════════════════════════════════════════════════════════════════════
      //
      // ⚠️ Folded in here from its own chip (Twilight, 2026-07-14: *"mix the colour palette into the
      // contrast dropdown below glitch contrast"*).
      //
      // It is a DIFFERENT thing from contrast, and the two must not blur together: **contrast is a
      // save byte** — it decides which of the four shades each pixel becomes — and **colour is a VIEW
      // setting that changes not one byte of the save** — it decides what those four shades are
      // painted. They live in one dropdown because you usually think about them together; the heading
      // and the "view only" note keep them from being mistaken for each other.

      Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 2
        implicitHeight: 1
        color: brg.settings.dividerColor
      }

      RowLayout {
        Layout.fillWidth: true

        Text {
          Layout.fillWidth: true
          text: qsTr("Colour")
          font.pixelSize: 11
          font.bold: true
          color: brg.settings.textColorMid
        }

        // The one line that keeps it honest: this half of the dropdown touches no save data.
        Text {
          text: qsTr("view only — no save change")
          font.pixelSize: 9
          font.italic: true
          color: brg.settings.textColorMid
          opacity: 0.75
        }
      }

      // ── The presets ──────────────────────────────────────────────────────────────────────────
      //
      // Grey · Game Boy (the green screen) · Super Game Boy (each map in its OWN real colours, from
      // pret's data) · Custom.
      Repeater {
        model: brg.map.colourPresets()

        delegate: Rectangle {
          id: colRow
          required property var modelData

          Layout.fillWidth: true
          implicitHeight: 28
          radius: 5

          readonly property bool active: brg.map.colourMode === modelData.mode

          color: colRow.active     ? Qt.rgba(0.34, 0.71, 0.91, 0.16)
               : colRowHover.hovered ? Qt.rgba(0, 0, 0, 0.06)
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
                model: colRow.modelData.swatch
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
              text: colRow.modelData.name
              font.pixelSize: 12
              font.bold: colRow.active
              color: brg.settings.textColorDark
            }
          }

          HoverHandler { id: colRowHover; cursorShape: Qt.PointingHandCursor }
          TapHandler { onTapped: brg.map.colourMode = colRow.modelData.mode }
        }
      }

      // ── The custom swatches ────────────────────────────────────────────────────────────────
      //
      // Only when Custom is chosen. Four colours, lightest to darkest; clicking one opens the system
      // colour dialog on it.
      RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 2
        visible: brg.map.colourMode === 3   // Custom
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
