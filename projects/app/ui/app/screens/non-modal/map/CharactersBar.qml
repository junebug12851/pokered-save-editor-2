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
 * CHARACTERS -- the cast you can put on the map. A panel in the left dock.
 *
 * ⚠️ It was briefly its own rail beside the dock, which broke the rule the whole screen is built on:
 * **panels do not stack out beside each other.** It is a dock panel like every other (2026-07-13).
 *
 * Two sections, and the order is the point:
 *
 *   1. **Safe here** — ungrouped, no ceremony: the characters *this map has already loaded*. Drag
 *      any of them out and the game draws them properly. This is the list you want 95% of the time,
 *      so it is the list you see first.
 *   2. **All** — the full 72, on five shelves (Story / Trainers / Townsfolk / Pokémon / Objects).
 *      The ones this map has *not* loaded carry a **!** — the console would draw them as garbage.
 *      They are still perfectly draggable: the byte allows it, so we allow it. We just say so.
 *
 * The **!** is the only thing with a tooltip, and the tooltip appears **on the ! itself** -- not
 * somewhere off in the corner, and not on every cell you happen to sweep past.
 *
 * Drag a character onto the map to place it. Drag a sprite off the map onto this panel to delete it.
 *
 * @see notes/plans/map-screen.md -> Phase 4c
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: bar

  /// The map canvas. Handed over by MapDock's Loader (its content cannot see the ids around it).
  property var canvas: null

  /// True while a sprite from the MAP is being dragged over us -- i.e. about to be deleted.
  property bool deleteHover: false

  readonly property var groups: ["Story", "Trainers", "Townsfolk", "Pokemon", "Objects"]
  property string filter: ""

  /// Everything, re-asked whenever the map changes (the "safe here" set moves with it).
  property int revision: 0

  Connections {
    target: brg.map
    function onChanged() { bar.revision++; }
  }

  readonly property var catalog: {
    bar.revision;
    return brg.map.spriteCatalog();
  }

  function matches(s) {
    return bar.filter === "" || s.name.toLowerCase().indexOf(bar.filter) >= 0;
  }

  readonly property var safeHere: (bar.catalog || []).filter(function(s) {
    return s.inSpriteSet && bar.matches(s);
  })

  // ── The delete target ──────────────────────────────────────────────────────────────────────
  //
  // Drag somebody off the map and onto the panel and they go back in the box. The WHOLE panel is
  // the target -- a small bin icon would be a thing to aim at, and aiming is work.
  DropArea {
    anchors.fill: parent
    keys: ["pse/map-sprite"]

    onEntered: bar.deleteHover = true
    onExited: bar.deleteHover = false

    onDropped: (drop) => {
      bar.deleteHover = false;

      const slot = drop.source && drop.source.spriteSlot !== undefined ? drop.source.spriteSlot : -1;

      if (slot <= 0) {
        drop.accept(Qt.IgnoreAction);   // the player cannot be deleted; he offers slot -1
        return;
      }

      brg.map.removeNpc(slot);

      if (bar.canvas) {
        bar.canvas.selectedNpc = -1;
        bar.canvas.status = qsTr("Removed. The sprites after it slid up a slot.");
      }

      // ⚠️ MoveAction, specifically. It is what tells MapSprite "the drop was a DELETE" so it does
      // not also commit a move on the sprite it has just destroyed.
      drop.accept(Qt.MoveAction);
    }
  }

  Rectangle {
    anchors.fill: parent
    visible: bar.deleteHover
    color: Qt.rgba(0.85, 0.2, 0.2, 0.14)
    border.width: 2
    border.color: "#d55e00"
    z: 50

    Label {
      anchors.centerIn: parent
      text: qsTr("Drop to remove")
      font.bold: true
      font.pixelSize: 12
      color: "#d55e00"
    }
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 8
    spacing: 6

    // Say the cap BEFORE they hit it, not after the drop is swallowed.
    Label {
      Layout.fillWidth: true
      text: brg.map.npcRoomLeft() > 0
              ? qsTr("Room for %1 more").arg(brg.map.npcRoomLeft())
              : qsTr("Full — 15 is the most a map can hold")
      font.pixelSize: 10
      opacity: 0.6
      color: brg.map.npcRoomLeft() > 0 ? palette.text : "#c04a00"
      wrapMode: Text.Wrap
    }

    // A SLIM filter. The stock TextField is 40+px tall with a big margin, which is a lot of nothing
    // at the top of a narrow panel (Twilight, 2026-07-13).
    TextField {
      id: filterField
      Layout.fillWidth: true
      Layout.preferredHeight: 26

      placeholderText: qsTr("Filter…")
      font.pixelSize: 11
      topPadding: 0
      bottomPadding: 0
      leftPadding: 6
      rightPadding: 6

      background: Rectangle {
        radius: 4
        color: Qt.rgba(1, 1, 1, 0.06)
        border.width: 1
        border.color: filterField.activeFocus ? "#56b4e9" : brg.settings.dividerColor
      }

      onTextChanged: bar.filter = text.trim().toLowerCase()
    }

    ScrollView {
      id: scroller
      Layout.fillWidth: true
      Layout.fillHeight: true
      clip: true
      contentWidth: availableWidth
      ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

      ColumnLayout {
        width: scroller.availableWidth
        spacing: 2

        // ── 1. SAFE HERE ─────────────────────────────────────────────────────────────────────
        //
        // No shelves, no ceremony. These are the characters this map has already loaded, so the
        // game draws them properly. It is the list you want almost every time, so it is first.
        Label {
          Layout.fillWidth: true
          Layout.bottomMargin: 2
          text: qsTr("Safe on this map")
          font.pixelSize: 11
          font.bold: true
          opacity: 0.55
          visible: bar.safeHere.length > 0
        }

        GridLayout {
          Layout.fillWidth: true
          columns: 2
          columnSpacing: 4
          rowSpacing: 4
          visible: bar.safeHere.length > 0

          Repeater {
            model: bar.safeHere
            delegate: CharacterCell {
              required property var modelData
              Layout.fillWidth: true
              character: modelData
              canvas: bar.canvas
            }
          }
        }

        Label {
          Layout.fillWidth: true
          Layout.topMargin: 4
          visible: bar.safeHere.length === 0 && bar.filter === ""
          text: qsTr("This map hasn't loaded any character pictures. Anything you place here, the game will draw as garbage.")
          font.pixelSize: 10
          opacity: 0.6
          wrapMode: Text.Wrap
        }

        Rectangle {
          Layout.fillWidth: true
          Layout.topMargin: 8
          Layout.bottomMargin: 2
          implicitHeight: 1
          color: brg.settings.dividerColor
        }

        // ── 2. ALL ───────────────────────────────────────────────────────────────────────────
        Label {
          Layout.fillWidth: true
          text: qsTr("All characters")
          font.pixelSize: 11
          font.bold: true
          opacity: 0.55
        }

        Repeater {
          model: bar.groups

          delegate: ColumnLayout {
            id: shelf
            required property string modelData

            Layout.fillWidth: true
            spacing: 2

            readonly property var members: (bar.catalog || []).filter(function(s) {
              return s.group === shelf.modelData && bar.matches(s);
            })

            // A shelf with nothing on it says nothing at all -- an empty header is furniture.
            visible: shelf.members.length > 0

            Label {
              Layout.fillWidth: true
              Layout.topMargin: 6
              text: shelf.modelData === "Pokemon" ? qsTr("Pokémon") : shelf.modelData
              font.pixelSize: 10
              opacity: 0.45
            }

            GridLayout {
              Layout.fillWidth: true
              columns: 2
              columnSpacing: 4
              rowSpacing: 4

              Repeater {
                model: shelf.members
                delegate: CharacterCell {
                  required property var modelData
                  Layout.fillWidth: true
                  character: modelData
                  canvas: bar.canvas
                }
              }
            }
          }
        }
      }
    }
  }
}
