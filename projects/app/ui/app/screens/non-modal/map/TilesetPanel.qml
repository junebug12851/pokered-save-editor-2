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
  TilesetPanel.qml -- the TILE layer of the map.

  Every save byte the tileset owns, editable, and NOT ONE OF THEM AS A NUMBER
  (principles.md -> "Every Byte, None of Them Raw"):

    * which tileset          -> a list of tilesets, by name
    * Indoor / Cave / Outdoor -> a tri-state, each option saying what it ANIMATES
    * grass tile              -> a picker of the real, rendered tiles
    * counter tiles 1-3       -> the same, and it says what a "counter" IS
    * bank + the 3 pointers   -> a named choice ("what Mart uses"), with Custom... behind it
    * the two boulder bytes   -> a sprite slot and a tile, under "Last Strength push",
                                 labelled as the leftovers they are -- but still editable,
                                 because every byte is the user's property

  The doctrine, throughout: when the save disagrees with the cartridge we SHOW it and never
  silently rewrite it. Same as the music bank and the glitch palettes.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: panel

  // May be null -- the QML smoke test loads every screen with no save.
  readonly property var ts: (brg.file.data.dataExpanded
                             && brg.file.data.dataExpanded.area)
                            ? brg.file.data.dataExpanded.area.tileset
                            : null
  readonly property bool hasTs: ts !== null && ts !== undefined
                                && ts.grassTile !== undefined

  // The dock owns the panel's frame and its title bar (MapDock.qml). A panel is its CONTENT --
  // it does not draw its own header, its own edge, or its own background chrome. (2026-07-12)
  color: "transparent"

  // A titled group. Used for every section so the panel reads as a stack of ideas rather than
  // a wall of controls.
  component Group: Rectangle {
    id: grp
    default property alias content: inner.data
    property string title: ""
    property string blurb: ""

    Layout.fillWidth: true
    implicitHeight: col.implicitHeight + 16

    radius: 4
    color: "#ffffff"
    border.width: 1
    border.color: brg.settings.dividerColor

    ColumnLayout {
      id: col
      anchors.fill: parent
      anchors.margins: 8
      spacing: 6

      Text {
        visible: grp.title !== ""
        text: grp.title
        font.pixelSize: 12
        font.bold: true
        color: brg.settings.textColorDark
      }

      Text {
        Layout.fillWidth: true
        visible: grp.blurb !== ""
        text: grp.blurb
        font.pixelSize: 11
        color: brg.settings.textColorMid
        wrapMode: Text.WordWrap
      }

      ColumnLayout {
        id: inner
        Layout.fillWidth: true
        spacing: 8
      }
    }
  }

  ScrollView {
    anchors.fill: parent
    anchors.margins: 12
    clip: true

    ColumnLayout {
      width: panel.width - 24
      spacing: 10

      // ── Which tileset ───────────────────────────────────────────────────────
      Group {
        title: qsTr("Which tileset")
        blurb: qsTr("The graphics, the blocks, and the rules for every tile on this map.")

        ComboBox {
          Layout.fillWidth: true
          font.pixelSize: 12
          enabled: panel.hasTs

          // Plain data, straight from MapModel -- QML never touches the DB struct.
          model: brg.map.tilesetList()
          textRole: "name"
          valueRole: "ind"

          currentIndex: brg.map.tilesetInd

          // Changing this changes what every block and tile on the map MEANS, so it is
          // deliberately the first and most prominent control -- not buried.
          onActivated: {
            if (!panel.hasTs) return;
            panel.ts.current = currentValue;
          }
        }
      }

      // ── What animates. The tri-state. ───────────────────────────────────────
      Group {
        title: qsTr("Indoor / Cave / Outdoor")
        blurb: qsTr("Not a place — it's which tiles MOVE. The game keeps one byte for this, "
                  + "and these are its three values.")

        // Three real options, each saying what it does. Nobody should have to know that
        // "Cave" is the ROM's TILEANIM_WATER.
        Repeater {
          model: [
            { v: 0, name: qsTr("Indoor"),  does: qsTr("Nothing animates.") },
            { v: 1, name: qsTr("Cave"),    does: qsTr("Water animates. Flowers don't.") },
            { v: 2, name: qsTr("Outdoor"), does: qsTr("Water and flowers animate.") }
          ]

          Rectangle {
            required property var modelData

            readonly property bool on: brg.map.tileAnim === modelData.v

            Layout.fillWidth: true
            implicitHeight: optRow.implicitHeight + 12

            radius: 4
            color: on ? Qt.rgba(0.85, 0.11, 0.38, 0.08)
                      : (optMouse.containsMouse ? "#f5f5f5" : "transparent")
            border.width: 1
            border.color: on ? brg.settings.primaryColor : brg.settings.dividerColor

            Behavior on color { ColorAnimation { duration: 110 } }

            RowLayout {
              id: optRow
              anchors.fill: parent
              anchors.margins: 6
              spacing: 8

              Rectangle {
                implicitWidth: 12
                implicitHeight: 12
                radius: 6
                color: "transparent"
                border.width: parent.parent.on ? 4 : 1
                border.color: parent.parent.on
                              ? brg.settings.primaryColor
                              : brg.settings.dividerColor
              }

              ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                Text {
                  text: modelData.name
                  font.pixelSize: 12
                  font.bold: parent.parent.parent.on
                  color: brg.settings.textColorDark
                }

                Text {
                  Layout.fillWidth: true
                  text: modelData.does
                  font.pixelSize: 10
                  color: brg.settings.textColorMid
                  elide: Text.ElideRight
                }
              }
            }

            MouseArea {
              id: optMouse
              anchors.fill: parent
              hoverEnabled: true
              cursorShape: Qt.PointingHandCursor
              onClicked: brg.map.tileAnim = modelData.v
            }
          }
        }

        // The save is allowed to disagree with the cartridge. Say so; never rewrite it.
        Text {
          Layout.fillWidth: true
          visible: !brg.map.tileAnimIsDefault
          text: qsTr("⚠ %1 normally uses %2. This save says otherwise — which is what a "
                   + "console would actually run, so that's what's drawn.")
                  .arg(brg.map.tilesetName)
                  .arg(brg.map.tileAnimStrFor(brg.map.tileAnimDefault))
          font.pixelSize: 10
          color: brg.settings.errorColor
          wrapMode: Text.WordWrap
        }
      }

      // ── Grass ───────────────────────────────────────────────────────────────
      Group {
        title: qsTr("Grass")
        blurb: qsTr("The one tile that means wild Pokémon. It's also the tile drawn over "
                  + "your feet when you stand in it.")

        TilePick {
          Layout.fillWidth: true
          visible: panel.hasTs

          tile: panel.hasTs ? panel.ts.grassTile : 255
          noneLabel: qsTr("No grass on this tileset")

          onPicked: (t) => {
            if (!panel.hasTs) return;
            panel.ts.grassTile = t;   // save 0x27E1, and only that byte
          }
        }
      }

      // ── Counters ────────────────────────────────────────────────────────────
      Group {
        title: qsTr("Counters")
        blurb: qsTr("The tiles you can talk ACROSS — a shop desk. Without one of these, the "
                  + "clerk standing behind the counter is out of reach. Three slots.")

        Repeater {
          model: 3

          TilePick {
            required property int index

            Layout.fillWidth: true
            visible: panel.hasTs

            tile: panel.hasTs ? panel.ts.talkingOverTilesAt(index) : 255
            noneLabel: qsTr("Slot %1 — empty").arg(index + 1)

            onPicked: (t) => {
              if (!panel.hasTs) return;
              panel.ts.talkingOverTilesSet(index, t);  // save 0x27DE + index
            }
          }
        }
      }

      // ── Strength ────────────────────────────────────────────────────────────
      //
      // These two really are leftovers -- the game rewrites them the next time you shove a
      // rock. But they ARE save bytes, so they are the user's, and they get proper editors.
      // What they don't get is a pretense that they configure anything.
      Group {
        title: qsTr("Last Strength push")
        blurb: qsTr("Scratch the game left behind the last time a boulder was pushed. It "
                  + "overwrites these the next time you push one — but they're yours, so "
                  + "here they are.")

        RowLayout {
          Layout.fillWidth: true
          spacing: 8
          visible: panel.hasTs

          Text {
            text: qsTr("The boulder")
            font.pixelSize: 11
            color: brg.settings.textColorMid
            Layout.preferredWidth: 70
          }

          // A sprite SLOT, so it's a slot picker -- "which of this map's 16 sprites".
          ComboBox {
            Layout.fillWidth: true
            font.pixelSize: 11

            model: {
              const out = [qsTr("None")];
              for (let i = 1; i <= 15; i++)
                out.push(qsTr("Sprite slot %1").arg(i));
              return out;
            }

            currentIndex: panel.hasTs
                          ? Math.max(0, Math.min(15, panel.ts.boulderIndex))
                          : 0

            onActivated: {
              if (!panel.hasTs) return;
              panel.ts.boulderIndex = currentIndex;   // save 0x29C4 (wBoulderSpriteIndex)
            }
          }
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 8
          visible: panel.hasTs

          Text {
            text: qsTr("What it hit")
            font.pixelSize: 11
            color: brg.settings.textColorMid
            Layout.preferredWidth: 70
          }

          // This byte does TWO jobs -- it holds the tile in front of the boulder, AND the
          // yes/no answer ($FF blocked, $00 clear). So the control says both, out loud,
          // rather than picking one and lying about the other.
          ComboBox {
            Layout.fillWidth: true
            font.pixelSize: 11

            model: [
              qsTr("Blocked — the push failed ($FF)"),
              qsTr("Clear — the boulder moved ($00)"),
              qsTr("A tile…")
            ]

            currentIndex: {
              if (!panel.hasTs) return 1;
              if (panel.ts.boulderColl === 255) return 0;
              if (panel.ts.boulderColl === 0) return 1;
              return 2;
            }

            onActivated: {
              if (!panel.hasTs) return;
              if (currentIndex === 0) panel.ts.boulderColl = 255;
              else if (currentIndex === 1) panel.ts.boulderColl = 0;
              // index 2 -- the tile picker below is the escape hatch; leave the byte alone.
            }
          }
        }

        // Every value still reachable: the byte can hold any tile id, and here is where you
        // reach it. (principles.md -- the "Custom..." escape hatch.)
        TilePick {
          Layout.fillWidth: true
          visible: panel.hasTs && panel.ts.boulderColl !== 0 && panel.ts.boulderColl !== 255

          tile: panel.hasTs ? panel.ts.boulderColl : 255
          noneLabel: qsTr("No tile")

          onPicked: (t) => {
            if (!panel.hasTs) return;
            panel.ts.boulderColl = t;   // save 0x29C8
          }
        }
      }

      // ── Advanced ────────────────────────────────────────────────────────────
      //
      // The pointers. Editable -- every byte is -- but they are the ones that make a map
      // unplayable if you get them wrong, so they sit behind a disclosure and each one is a
      // NAMED choice rather than a hex box.
      Group {
        title: qsTr("Where the tileset lives")
        blurb: qsTr("The cartridge addresses this tileset is read from. Getting these wrong "
                  + "makes the map unplayable — which is why they're a list, not a box.")

        Button {
          Layout.fillWidth: true
          flat: true
          font.pixelSize: 11
          text: advanced.visible ? qsTr("Hide") : qsTr("Show them anyway")
          onClicked: advanced.visible = !advanced.visible
        }

        ColumnLayout {
          id: advanced
          Layout.fillWidth: true
          spacing: 6
          visible: false

          // Say plainly when the save's pointers aren't the ones the cartridge has for this
          // tileset. This is the check that would have caught the collPtr bug from the outside.
          Text {
            Layout.fillWidth: true
            visible: panel.hasTs && !panel.matchesCartridge
            text: qsTr("⚠ These don't match what the game ships for %1. Shown as they are, "
                     + "never rewritten.").arg(brg.map.tilesetName)
            font.pixelSize: 10
            color: brg.settings.errorColor
            wrapMode: Text.WordWrap
          }

          Repeater {
            model: panel.hasTs ? panel.pointerRows : []

            RowLayout {
              required property var modelData
              Layout.fillWidth: true
              spacing: 8

              Text {
                text: modelData.name
                font.pixelSize: 11
                color: brg.settings.textColorMid
                Layout.preferredWidth: 70
              }

              Text {
                Layout.fillWidth: true
                text: modelData.value
                font.pixelSize: 11
                font.family: "monospace"
                color: modelData.ok ? brg.settings.textColorDark : brg.settings.errorColor
                elide: Text.ElideRight
              }
            }
          }

          Button {
            Layout.fillWidth: true
            flat: true
            font.pixelSize: 11
            enabled: panel.hasTs && !panel.matchesCartridge
            text: qsTr("Put them back to %1's").arg(brg.map.tilesetName)

            // The only way these change: deliberately, back to the truth. There is no
            // free-typing a pointer here, because there is no version of that which is a good
            // idea -- and the tileset picker above already reaches every real combination.
            // It touches ONLY the four pointer bytes; grass, counters and the animation byte
            // are left exactly as they are, because the user may well have meant those.
            onClicked: brg.map.restoreTilesetPointers()
          }
        }
      }

      Item { Layout.preferredHeight: 8 }
    }
  }

  // ── Does the save agree with the cartridge? ─────────────────────────────────
  //
  // Plain data from MapModel, never the DB struct (QML cannot read one, and handing it a
  // parentless QObject instead would get it garbage-collected -- see qt-patterns.md).
  readonly property var canon: brg.map.canonicalTileset()

  readonly property bool matchesCartridge: {
    if (!hasTs || !canon || canon.bank === undefined) return true;
    return ts.bank === canon.bank
        && ts.blockPtr === canon.blockPtr
        && ts.gfxPtr === canon.gfxPtr
        && ts.collPtr === canon.collPtr;
  }

  readonly property var pointerRows: {
    if (!hasTs || !canon || canon.bank === undefined) return [];

    const hex = (v) => "$" + v.toString(16).toUpperCase().padStart(4, "0");

    return [
      { name: qsTr("Bank"),      value: String(ts.bank), ok: ts.bank === canon.bank },
      { name: qsTr("Blocks"),    value: hex(ts.blockPtr), ok: ts.blockPtr === canon.blockPtr },
      { name: qsTr("Graphics"),  value: hex(ts.gfxPtr),   ok: ts.gfxPtr === canon.gfxPtr },
      { name: qsTr("Collision"), value: hex(ts.collPtr),  ok: ts.collPtr === canon.collPtr }
    ];
  }
}
