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
 * The CHARACTERS BAR -- the cast you can put on the map.
 *
 * A permanent left-hand rail (NOT the dock, and NOT the sprite-SET panel, which is a different
 * thing entirely: that one is the eleven pictures the Game Boy has in memory). Two columns of the
 * game's own artwork, on five shelves -- Story, Trainers, Townsfolk, Pokémon, Objects -- with a
 * filter box, because 72 characters is a long scroll otherwise.
 *
 *   * DRAG one onto the map     -> a new sprite, tile-snapped, with the game's own sane defaults.
 *   * DRAG a sprite off the map onto the BAR -> delete it.
 *
 * A character this map has not LOADED (its picture is not in the map's sprite set) is shown with a
 * quiet amber corner. It is still perfectly draggable -- the byte allows it, so we allow it -- but
 * the console would draw it as garbage, and you should hear that from us first.
 *
 * @see notes/plans/map-screen.md -> Phase 4c, notes/reference/sprites.md
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: bar

  /// The map canvas we place onto / delete from.
  required property var canvas

  /// Collapsed to a thin strip. Clutter is a bug: at the default window the rail, the Layers dock
  /// and the map cannot all have what they want, and the CONTENT is not the thing that gives way.
  property bool collapsed: false

  implicitWidth: collapsed ? 26 : 168
  Behavior on implicitWidth { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

  color: Qt.rgba(1, 1, 1, 0.04)
  clip: true

  /// True while a sprite from the MAP is being dragged over us -- i.e. about to be deleted.
  property bool deleteHover: false

  readonly property var groups: ["Story", "Trainers", "Townsfolk", "Pokemon", "Objects"]
  property string filter: ""

  /// Everything, filtered. Recomputed when the map changes (the sprite-set flags move with it).
  property var catalog: []

  function refresh() {
    bar.catalog = brg.map.spriteCatalog();
  }

  Component.onCompleted: bar.refresh()

  Connections {
    target: brg.map
    function onChanged() { bar.refresh(); }
  }

  // ── The delete target ──────────────────────────────────────────────────────────────────────
  //
  // Drag somebody off the map and onto the bar and they go back in the box. The whole rail is the
  // target -- a small bin icon would be a thing to aim at, and aiming is work.
  DropArea {
    anchors.fill: parent
    keys: ["pse/map-sprite"]

    onEntered: bar.deleteHover = true
    onExited: bar.deleteHover = false

    onDropped: (drop) => {
      bar.deleteHover = false;
      const slot = drop.source && drop.source.spriteSlot !== undefined ? drop.source.spriteSlot : -1;
      if (slot > 0) {
        brg.map.removeNpc(slot);
        bar.canvas.selectedNpc = -1;
        bar.canvas.status = qsTr("Removed. The sprites after it slid up a slot.");
      }
      drop.accept();
    }
  }

  Rectangle {
    anchors.fill: parent
    visible: bar.deleteHover
    color: Qt.rgba(0.85, 0.2, 0.2, 0.18)
    border.width: 2
    border.color: "#d55e00"
    z: 50

    Label {
      anchors.centerIn: parent
      text: qsTr("Drop to remove")
      font.bold: true
      color: "#d55e00"
    }
  }

  // ── Collapsed: a strip you can click to get it back ────────────────────────────────────────
  Item {
    anchors.fill: parent
    visible: bar.collapsed

    Label {
      anchors.centerIn: parent
      text: "☻"
      font.pixelSize: 15
      opacity: stripHover.hovered ? 1.0 : 0.7
    }

    HoverHandler { id: stripHover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: bar.collapsed = false }

    ToolTip.visible: stripHover.hovered
    ToolTip.text: qsTr("Characters — the people and objects you can put on the map")
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 8
    spacing: 6
    visible: !bar.collapsed

    RowLayout {
      Layout.fillWidth: true
      spacing: 4

      Label {
        text: qsTr("Characters")
        font.bold: true
        opacity: 0.8
        Layout.fillWidth: true
        elide: Text.ElideRight
      }

      Label {
        text: "⟨"
        opacity: collapseHover.hovered ? 1.0 : 0.5

        HoverHandler { id: collapseHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: bar.collapsed = true }

        ToolTip.visible: collapseHover.hovered
        ToolTip.text: qsTr("Collapse")
      }
    }

    // Say the cap BEFORE they hit it, not after the drop is swallowed.
    Label {
      Layout.fillWidth: true
      text: brg.map.npcRoomLeft() > 0
              ? qsTr("Room for %1 more").arg(brg.map.npcRoomLeft())
              : qsTr("Full — 15 is the most a map can hold")
      font.pixelSize: 11
      opacity: 0.65
      color: brg.map.npcRoomLeft() > 0 ? palette.text : "#d55e00"
      wrapMode: Text.Wrap
    }

    TextField {
      id: filterField
      Layout.fillWidth: true
      placeholderText: qsTr("Filter…")
      onTextChanged: bar.filter = text.trim().toLowerCase()
    }

    ScrollView {
      Layout.fillWidth: true
      Layout.fillHeight: true
      clip: true
      ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

      ColumnLayout {
        width: bar.width - 22
        spacing: 4

        Repeater {
          model: bar.groups

          delegate: ColumnLayout {
            id: shelf
            required property string modelData

            Layout.fillWidth: true
            spacing: 2

            readonly property var members: (bar.catalog || []).filter(function(s) {
              return s.group === shelf.modelData
                  && (bar.filter === "" || s.name.toLowerCase().indexOf(bar.filter) >= 0);
            })

            // A shelf with nothing on it says nothing at all -- an empty header is furniture.
            visible: shelf.members.length > 0

            Label {
              Layout.fillWidth: true
              Layout.topMargin: 6
              text: shelf.modelData === "Pokemon" ? qsTr("Pokémon") : shelf.modelData
              font.pixelSize: 11
              font.bold: true
              opacity: 0.55
            }

            GridLayout {
              Layout.fillWidth: true
              columns: 2
              columnSpacing: 4
              rowSpacing: 4

              Repeater {
                model: shelf.members

                delegate: Item {
                  id: cell
                  required property var modelData

                  Layout.fillWidth: true
                  implicitHeight: 62

                  Rectangle {
                    id: cellBg
                    anchors.fill: parent
                    radius: 6
                    color: cellHover.hovered ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(1, 1, 1, 0.04)

                    Column {
                      anchors.centerIn: parent
                      spacing: 2

                      // ⚠️ 32, not 24. The art is 16x16 -- an INTEGER multiple or the pixels come
                      // out uneven, and uneven pixels on pixel art is just a blurry mess.
                      Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 32
                        height: 32
                        source: cell.modelData.source
                        smooth: false
                        mipmap: false
                        fillMode: Image.PreserveAspectFit
                        cache: true
                      }

                      Label {
                        width: cellBg.width - 8
                        horizontalAlignment: Text.AlignHCenter
                        text: cell.modelData.name
                        font.pixelSize: 9
                        elide: Text.ElideRight
                        opacity: 0.85
                      }
                    }

                    // Not in this map's sprite set -- the console would draw it as garbage.
                    Rectangle {
                      visible: !cell.modelData.inSpriteSet
                      anchors.top: parent.top
                      anchors.right: parent.right
                      anchors.margins: 3
                      width: 5
                      height: 5
                      radius: 2.5
                      color: "#ffd54f"
                    }

                    HoverHandler { id: cellHover }

                    ToolTip.visible: cellHover.hovered
                    ToolTip.delay: 600
                    ToolTip.text: cell.modelData.inSpriteSet
                        ? qsTr("%1 — drag onto the map").arg(cell.modelData.name)
                        : qsTr("%1 — this map hasn't loaded this picture, so the game would draw it as garbage. You can still place it.")
                            .arg(cell.modelData.name)
                  }

                  // ── Drag onto the map ──────────────────────────────────────────────────────
                  //
                  // The drag carries the PICTURE id; the canvas turns the drop point into a tile.
                  Item {
                    id: dragProxy
                    anchors.fill: parent

                    property int spritePicture: cell.modelData.ind

                    Drag.active: dragHandler.active
                    Drag.source: dragProxy
                    Drag.keys: ["pse/catalog-sprite"]
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2

                    DragHandler {
                      id: dragHandler
                      enabled: brg.map.npcRoomLeft() > 0
                      onActiveChanged: {
                        if (!active)
                          dragProxy.Drag.drop();
                      }
                    }
                  }

                  // The thing that actually follows the cursor. Drawing the cell itself would tear
                  // a hole in the rail.
                  Image {
                    parent: cell.Window.window ? cell.Window.window.contentItem : cell
                    visible: dragHandler.active
                    z: 999
                    width: 32
                    height: 32
                    source: cell.modelData.source
                    smooth: false
                    opacity: 0.85
                    x: dragHandler.centroid.scenePosition.x - width / 2
                    y: dragHandler.centroid.scenePosition.y - height / 2
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
