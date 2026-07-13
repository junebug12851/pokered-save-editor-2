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
  MapPicker.qml -- ONE control in the top bar that answers "which map, drawn out of what".

  Clicking it drops a small panel with the three choices the save actually keeps, and they are three
  because the SAVE keeps them as three:

    * the MAP        (`wCurMap`)         -- which map's block data is loaded
    * the TILESET    (`gfxPtr`)          -- where the tiles are drawn FROM, and Indoor/Cave/Outdoor
                                            (which is not a place -- it is which tiles MOVE)
    * the BLOCKSET   (`blockPtr`)        -- which tileset's BLOCKS the map is built out of

  Normally the last two name the same tileset. They are two separate pointers in the save, though,
  and a console draws exactly what they say -- so they get two separate controls, and a save that
  disagrees with itself is SHOWN doing so, never quietly tidied up.

  ⚠️ Picking a map writes ONE byte (`wCurMap`). The map's stored size lives in other bytes, and this
  control does not touch them -- it says so, and offers to fix them, which is the difference between
  an editor you can trust and one that "helpfully" rewrites your save.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: root

  implicitWidth: chip.implicitWidth
  implicitHeight: 24

  /// Drive the drop-down open/shut by name -- the DEBUG harness can only set properties on items,
  /// and the mandatory screenshot review has to be able to REACH the thing it is reviewing.
  /// (reference/dev-harness.md)
  property bool openState: false
  onOpenStateChanged: openState ? pop.open() : pop.close()

  // ── The chip ────────────────────────────────────────────────────────────────────────────────
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

      Text {
        text: brg.map.valid ? brg.map.mapName : qsTr("No map")
        font.pixelSize: 12
        font.bold: true
        color: brg.settings.textColorDark
      }

      Text {
        text: "·"
        font.pixelSize: 12
        color: brg.settings.textColorMid
      }

      Text {
        text: brg.map.tilesetName
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }

      // The blocks come from somewhere else than the tiles -- rare, legal, and worth saying out loud.
      Text {
        visible: !brg.map.blocksetIsTileset
        text: qsTr("blocks: %1").arg(brg.map.blocksetName)
        font.pixelSize: 11
        color: brg.settings.errorColor
      }

      Text {
        text: "⌄"
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }
    }
  }

  // ── The drop-down ───────────────────────────────────────────────────────────────────────────
  Popup {
    id: pop

    y: root.height + 4
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

      // ── Map ─────────────────────────────────────────────────────────────────────────────────
      Text {
        text: qsTr("Map")
        font.pixelSize: 11
        font.bold: true
        color: brg.settings.textColorMid
      }

      ComboBox {
        id: mapCombo
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        font.pixelSize: 12

        model: brg.map.mapList()
        textRole: "name"
        valueRole: "ind"

        currentIndex: {
          const list = model;
          for (let i = 0; i < list.length; i++)
            if (list[i].ind === brg.map.mapInd)
              return i;
          return -1;
        }

        onActivated: brg.map.mapInd = currentValue

        // Every one of the 248 ids, glitch and half-baked included -- GROUPED, like the music list
        // (Twilight, 2026-07-13). The group is the map's own tileset, which is real data out of
        // maps.json rather than a category we invented; the unfinished copies gather at the end.
        // 248 names in one flat list is a wall.
        delegate: ItemDelegate {
          required property var modelData
          required property int index

          width: mapCombo.width
          height: (modelData.group !== "" ? 20 : 0) + 26
          highlighted: mapCombo.highlightedIndex === index

          contentItem: ColumnLayout {
            spacing: 0

            // The heading rides on the first entry of its group (the model puts it there), so there
            // is one list and one model rather than two that can drift.
            Text {
              visible: modelData.group !== ""
              Layout.fillWidth: true
              text: modelData.group
              font.pixelSize: 10
              font.bold: true
              color: brg.settings.textColorMid
            }

            RowLayout {
              Layout.fillWidth: true
              spacing: 6

              Text {
                text: modelData.ind
                font.pixelSize: 10
                font.family: "monospace"
                color: brg.settings.textColorMid
                Layout.minimumWidth: 22
              }

              Text {
                Layout.fillWidth: true
                text: modelData.name
                font.pixelSize: 12
                color: brg.settings.textColorDark
                elide: Text.ElideRight
              }

              // A fact, not an alarm: this id has no map of its own, so the game draws the one it is
              // an unfinished copy of.
              Text {
                visible: modelData.isCopy
                text: qsTr("→ %1").arg(modelData.copyOf)
                font.pixelSize: 10
                font.italic: true
                color: brg.settings.textColorMid
              }
            }
          }
        }
      }

      // The size the save stores is a DIFFERENT set of bytes from the map id, and picking a map
      // leaves them stale. The doctrine says SHOW it and offer the fix -- never rewrite it quietly.
      //
      // But it is not an ERROR, so it is not red (Twilight, 2026-07-13: "you have red text everywhere,
      // even to indicate information, which is bad"). Red means *something is broken*. This is a
      // notice, so it reads as a notice: a muted amber line with a button that does the thing.
      RowLayout {
        Layout.fillWidth: true
        visible: !brg.map.headerMatches
        spacing: 6

        Text {
          Layout.fillWidth: true
          text: qsTr("The stored map size is from another map.")
          font.pixelSize: 10
          color: "#8a6d00"
          wrapMode: Text.WordWrap
        }

        Button {
          flat: true
          font.pixelSize: 10
          text: qsTr("Fix")
          onClicked: brg.map.fixMapHeader()
        }
      }

      Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: brg.settings.dividerColor }

      // ── Tileset (the graphics) + what animates ──────────────────────────────────────────────
      Text {
        text: qsTr("Tileset — the graphics")
        font.pixelSize: 11
        font.bold: true
        color: brg.settings.textColorMid
      }

      ComboBox {
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        font.pixelSize: 12

        model: brg.map.tilesetList()
        textRole: "name"
        valueRole: "ind"

        currentIndex: {
          const list = model;
          for (let i = 0; i < list.length; i++)
            if (list[i].ind === brg.map.tilesetInd)
              return i;
          return -1;
        }

        onActivated: brg.map.tilesetInd = currentValue
      }

      // Indoor / Cave / Outdoor. NOT a place -- it is which tiles MOVE, and it lives with the
      // tileset because it IS the tileset's byte (0x3522). Cave is not Indoor: cave water animates.
      RowLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
          model: [
            { v: 0, name: qsTr("Indoor"),  does: qsTr("Nothing animates.") },
            { v: 1, name: qsTr("Cave"),    does: qsTr("Water animates. Flowers don't.") },
            { v: 2, name: qsTr("Outdoor"), does: qsTr("Water and flowers animate.") }
          ]

          Rectangle {
            required property var modelData
            required property int index

            Layout.fillWidth: true
            implicitHeight: 26

            readonly property bool active: brg.map.tileAnim === modelData.v

            color: active ? brg.settings.accentColor
                 : segHover.hovered ? "#f0f0f0" : "transparent"

            border.width: 1
            border.color: brg.settings.dividerColor

            topLeftRadius: index === 0 ? 4 : 0
            bottomLeftRadius: index === 0 ? 4 : 0
            topRightRadius: index === 2 ? 4 : 0
            bottomRightRadius: index === 2 ? 4 : 0

            HoverHandler { id: segHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: brg.map.tileAnim = modelData.v }

            Text {
              anchors.centerIn: parent
              text: modelData.name
              font.pixelSize: 11
              font.bold: parent.active
              color: parent.active ? brg.settings.textColorLight : brg.settings.textColorDark
            }
          }
        }
      }

      // What the chosen one DOES, said underneath and changing as you pick (Twilight, 2026-07-13) --
      // rather than hidden in a tooltip you have to go hunting for. This is the whole reason the
      // control exists: "Indoor" is not a place, it is *nothing animates*.
      Text {
        Layout.fillWidth: true
        text: {
          switch (brg.map.tileAnim) {
            case 0: return qsTr("Nothing animates — no water, no flowers. (Surf needs the water tile, "
                                + "so this breaks it on a water map.)");
            case 1: return qsTr("The water animates. The flowers don't.");
            case 2: return qsTr("The water and the flowers both animate.");
          }
          // Every value the save can hold, including the ones no real game ships: the console tests
          // bit 0 and nothing else.
          return (brg.map.tileAnim % 2 === 1)
                 ? qsTr("%1 — the console reads bit 0, so this behaves as water only.")
                     .arg(brg.map.tileAnim)
                 : qsTr("%1 — the console reads bit 0, so this behaves as water and flowers.")
                     .arg(brg.map.tileAnim);
        }
        font.pixelSize: 10
        color: brg.settings.textColorMid
        wrapMode: Text.WordWrap
      }

      Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: brg.settings.dividerColor }

      // ── Blockset (the blocks) ───────────────────────────────────────────────────────────────
      Text {
        text: qsTr("Blockset — what the map is built from")
        font.pixelSize: 11
        font.bold: true
        color: brg.settings.textColorMid
      }

      ComboBox {
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        font.pixelSize: 12

        model: brg.map.tilesetList()
        textRole: "name"
        valueRole: "ind"

        currentIndex: {
          const list = model;
          for (let i = 0; i < list.length; i++)
            if (list[i].ind === brg.map.blocksetInd)
              return i;
          return -1;   // a blockPtr that is nobody's blockset. Shown, not "corrected".
        }

        onActivated: brg.map.blocksetInd = currentValue
      }

      // A fact about an unusual save, in the same muted voice as everything else here. It is not an
      // error -- a console draws it perfectly happily -- so it does not shout.
      Text {
        Layout.fillWidth: true
        visible: !brg.map.blocksetIsTileset
        text: brg.map.blocksetInd < 0
              ? qsTr("The blocks pointer is not any tileset's. The game would read whatever sits at "
                     + "that address.")
              : qsTr("The blocks come from %1 and the tiles from %2.")
                .arg(brg.map.blocksetName).arg(brg.map.tilesetName)
        font.pixelSize: 10
        color: brg.settings.textColorMid
        wrapMode: Text.WordWrap
      }
    }
  }
}
