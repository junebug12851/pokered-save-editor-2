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
  TilesetPanel.qml -- the TILES panel: the save bytes that decide what the map's tiles MEAN.

  Not a number in sight (principles.md -> "Every Byte, None of Them Raw"):

    * the edge of the world  -> the block that fills the ring around the map, as a block picker
    * the grass tile         -> a picker of the real, rendered tiles
    * counter tiles 1-3      -> the same, and it says what a "counter" IS
    * the two boulder bytes  -> a sprite slot and a tile, under "Last Strength push", labelled as
                                the leftovers they are -- but still editable, because every byte is
                                the user's property

  What is NOT here (2026-07-13): which tileset, which blockset, and Indoor/Cave/Outdoor -- they live
  in the TOP BAR's picker, where you pick things. And the raw pointers, which were a wall of hex
  addresses you could not type into.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: panel

  // ⚠️ The re-read dependency. `talkingOverTilesAt(i)` and friends are INVOKABLE reads -- QML is
  // never notified when their answer changes -- so without this the panel showed a counter slot
  // as "empty" right after you picked a tile into it, while the map (which re-renders on
  // changed()) showed it correctly (leadership, 2026-07-18). Every invokable read below carries
  // `panel.revision` as a dependency; every write bumps `editTick`.
  property int revision: 0
  property int editTick: 0
  Connections {
    target: brg.map
    function onChanged() { panel.revision++; }
  }

  // May be null -- the QML smoke test loads every screen with no save.
  readonly property var ts: (brg.file.data.dataExpanded
                             && brg.file.data.dataExpanded.area)
                            ? brg.file.data.dataExpanded.area.tileset
                            : null
  readonly property bool hasTs: ts !== null && ts !== undefined
                                && ts.grassTile !== undefined

  // The map's own block (the edge of the world lives here now).
  readonly property var areaMap: (brg.file.data.dataExpanded
                                  && brg.file.data.dataExpanded.area)
                                 ? brg.file.data.dataExpanded.area.map
                                 : null
  readonly property bool hasMap: areaMap !== null && areaMap !== undefined
                                 && areaMap.outOfBoundsBlock !== undefined

  // The dock owns the panel's frame and its title bar (MapDock.qml). A panel is its CONTENT --
  // it does not draw its own header, its own edge, or its own background chrome. (2026-07-12)
  color: "transparent"

  /// The panel's "?" — the explanation that used to be a paragraph under every group heading.
  readonly property string panelInfo: qsTr(
    "What the map's tiles MEAN. A wall and a floor are just two pictures until something says which "
    + "is which — and this is where the save says it.\n\n"
    + "Grass is the tile that means wild Pokémon. A counter is a tile you can talk ACROSS (a shop "
    + "desk — without one, the clerk behind it is out of reach). The edge of the world is the block "
    + "that fills the ring around the map, where no connected map bleeds in.")

  /// A titled group. A title, a MapWarnIcon if the game overwrites these bytes, and a SHORT hint.
  ///
  /// ⚠️ The multi-line `blurb` under every heading is gone (Twilight, 2026-07-13: the Strength group
  /// "has blocks of text again"). Prose goes in `panelInfo`, behind the "?" — not in the panel.
  component Group: Rectangle {
    id: grp
    default property alias content: inner.data
    property string title: ""

    /// ONE short line. Not a paragraph.
    property string hint: ""

    /// The game overwrites these bytes. Shows the yellow "!" beside the title. @see MapWarnIcon
    property string overwritten: ""

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

      RowLayout {
        Layout.fillWidth: true
        spacing: 5
        visible: grp.title !== ""

        Text {
          text: grp.title
          font.pixelSize: 12
          font.bold: true
          color: brg.settings.textColorDark
        }

        MapWarnIcon {
          visible: grp.overwritten !== ""
          text: grp.overwritten
        }

        Item { Layout.fillWidth: true }
      }

      Text {
        Layout.fillWidth: true
        visible: grp.hint !== ""
        text: grp.hint
        font.pixelSize: 10
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
      // 12px of margin each side, PLUS the 16px overlay-scrollbar lane. @see ui-patterns.md
      width: panel.width - 24 - 22
      spacing: 10

      // ── The edge of the world ───────────────────────────────────────────────
      //
      // `wMapBackgroundTile` (save 0x2659): the block that fills the 3-block ring around every map.
      // It moved here when the Blocks panel was deleted (Twilight, 2026-07-13) -- it is a property of
      // the map's tiles, and it was the one thing in that panel worth keeping.
      Group {
        title: qsTr("Edge of the world")

        BlockPick {
          Layout.fillWidth: true
          visible: panel.hasMap

          block: panel.hasMap ? panel.areaMap.outOfBoundsBlock : 0

          onPicked: (b) => {
            if (!panel.hasMap) return;
            panel.areaMap.outOfBoundsBlock = b;   // save 0x2659, and only that byte
          }
        }
      }

      // ── Grass ───────────────────────────────────────────────────────────────
      Group {
        title: qsTr("Grass")

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
        hint: qsTr("Three slots.")

        Repeater {
          model: 3

          TilePick {
            required property int index

            Layout.fillWidth: true
            visible: panel.hasTs

            // `revision`/`editTick` are the re-read dependencies -- an invokable read is never
            // notified, and without them a freshly-picked slot went on reading "empty" here
            // while the map already highlighted it. @see the property at the top.
            tile: {
              panel.revision; panel.editTick;
              return panel.hasTs ? panel.ts.talkingOverTilesAt(index) : 255;
            }
            noneLabel: qsTr("Slot %1 — empty").arg(index + 1)

            onPicked: (t) => {
              if (!panel.hasTs) return;
              panel.ts.talkingOverTilesSet(index, t);  // save 0x27DE + index
              panel.editTick++;
            }
          }
        }
      }

      // ── Strength ────────────────────────────────────────────────────────────
      //
      // ⚠️ BEHIND THE "SHOW CLEARED" SWITCH (Twilight, 2026-07-13: *"Last Strength push is still
      // there, should be behind reloaded values"*). These two bytes are scratch the game left behind
      // the last time somebody shoved a rock, and it overwrites them the next time. They are real,
      // they are hers, and she can edit them -- but they configure nothing, and having them sitting
      // under the grass and the counters implies they do.
      Group {
        visible: brg.map.showScratch

        title: qsTr("Last Strength push")
        overwritten: qsTr("Scratch the game left behind the last time a boulder was pushed. It "
                          + "overwrites these the next time you push one — but they're yours, so "
                          + "here they are.")

        // ⚠️ The label goes ABOVE the control, not beside it. Beside it, a 70px label left ~90px for
        // the combo in a 170px panel, and its text was unreadable (Twilight, 2026-07-13). In a narrow
        // panel a control gets the FULL width and the label gets its own line. Cheap, and it reads.
        ColumnLayout {
          Layout.fillWidth: true
          spacing: 2
          visible: panel.hasTs

          Text {
            text: qsTr("The boulder")
            font.pixelSize: 11
            color: brg.settings.textColorMid
          }

          // A sprite SLOT, so it's a slot picker -- "which of this map's 16 sprites".
          ComboBox {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
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

        ColumnLayout {
          Layout.fillWidth: true
          spacing: 2
          visible: panel.hasTs

          Text {
            text: qsTr("What it hit")
            font.pixelSize: 11
            color: brg.settings.textColorMid
          }

          // This byte does TWO jobs -- it holds the tile in front of the boulder, AND the
          // yes/no answer ($FF blocked, $00 clear). So the control says both, out loud,
          // rather than picking one and lying about the other.
          ComboBox {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            font.pixelSize: 11

            model: [
              qsTr("Blocked ($FF)"),
              qsTr("Clear ($00)"),
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

      Item { Layout.preferredHeight: 8 }
    }
  }

  // ("Where the tileset lives" -- the bank and the three raw pointers -- was removed on 2026-07-13
  // at Twilight's call. The tileset and blockset PICKERS in the top bar already reach every real
  // combination of them, and `MapModel::setTilesetInd` / `setBlocksetInd` write them properly. A
  // panel of hex addresses you cannot type into was a wall of numbers pretending to be a control.
  // MapModel::canonicalTileset() / restoreTilesetPointers() still exist for when the Inspector wants
  // them -- see notes/plans/map-screen.md, phase 5.)
}
