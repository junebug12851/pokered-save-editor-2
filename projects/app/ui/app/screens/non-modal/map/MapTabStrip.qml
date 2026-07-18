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
 * THE tab strip — ONE, at canvas level, following the hovered block.
 *
 * ⚠️ Why it is not a child of MapBlockHotspot any more (leadership, 2026-07-18): *"sprites should
 * always be shown … the sprite has to go somewhere still and its in the same spot and has to be
 * shown on top. Player and people objects are at the top so they should always be rendered … at
 * top."* In Qt Quick a child can never out-stack its PARENT's siblings — so as long as the strip
 * lived inside the hotspot block, raising it above the sprites meant raising the whole block,
 * boxes and all, over the artwork. Splitting them is the only shape that satisfies both rules at
 * once:
 *
 *   - the storage BOXES stay at z 0, under every object, always — they annotate, never cover;
 *   - the objects (people, doors, signs, the player) render above them, always;
 *   - THIS strip sits above the objects (it is the way IN to a crowded cell, and a handle you
 *     cannot reach is no handle) — and since only one block is ever hovered, one strip is all
 *     there ever was.
 *
 * Everything else about the tabs is unchanged and documented in MapBlockHotspot.qml's standard:
 * one square per spot, non-tile family left / tile traits right with a gap, hover lights the
 * spot's own outline, click selects-and-opens, and a MOVABLE spot's tab can be DRAGGED to move
 * the thing itself (the canvas proxy-drag).
 */
import QtQuick
import QtQuick.Controls

Item {
  id: strip

  /// The canvas -- the hover state, the zoom, the proxy-drag, the storage list.
  required property var canvas

  /// A spot was reached (click). The canvas selects the thing and opens its editor.
  signal spotClicked(string kind, string section, int ind)

  /// ⚠️ FROZEN while a tab is being dragged. The strip follows the hovered cell -- but a tab drag
  /// moves the cursor OFF that cell immediately, and if the strip re-targeted (or hid) mid-drag it
  /// would move or destroy the very MouseArea holding the grab. Lock the entry on drag start;
  /// release on drop/cancel.
  property var lockedEntry: null

  /// The hovered block's entry of MapCanvas.storageBlocks, or null. Computed HERE so the strip
  /// costs one lookup per hover move, not one item per block.
  readonly property var entry: {
    if (strip.lockedEntry !== null)
      return strip.lockedEntry;
    if (strip.canvas.hoverBlockX < 0)
      return null;
    const list = strip.canvas.storageBlocks;
    for (let i = 0; i < list.length; i++)
      if (list[i].blockX === strip.canvas.hoverBlockX
          && list[i].blockY === strip.canvas.hoverBlockY)
        return list[i];
    return null;
  }

  /// Only what the active layers admit -- the same filter the boxes use.
  readonly property var spots: strip.entry === null ? []
      : strip.entry.spots.filter(s => strip.canvas.activeStorageKinds.indexOf(s.kind) >= 0)

  readonly property var tileSpots: strip.spots.filter(s => s.unit === "tile")
  readonly property var gridSpots: strip.spots.filter(s => s.unit !== "tile")

  /// The withdraw rule, unchanged: pointing at a movable OBJECT in this cell gets you the object
  /// (its own outline, ready to drag); pointing at the cell around it gets the tabs. Unless the
  /// object fills the cell, in which case the tabs are the only way in and stay.
  readonly property bool shown: strip.lockedEntry !== null
      || (strip.entry !== null && strip.spots.length > 0
          && !(strip.canvas.hoverMovable === (strip.entry.blockX + "," + strip.entry.blockY)
               && !strip.canvas.hoverMovableFullCell))

  visible: strip.shown
  onShownChanged: if (!strip.shown) strip.canvas.litSpot = null

  x: strip.entry !== null ? strip.entry.rectX * strip.canvas.zoom + 1 : 0
  y: strip.entry !== null ? strip.entry.rectY * strip.canvas.zoom + 1 : 0

  // Above the objects, always -- the whole reason this file exists. (The boxes stay at 0.)
  z: 6

  // Fade rather than blink: the strip follows the cursor, and a hard cut reads as a glitch.
  opacity: strip.shown ? 1 : 0
  Behavior on opacity { NumberAnimation { duration: 60 } }

  function inkOf(spot) {
    return spot.ink !== undefined ? spot.ink : brg.map.ink(spot.kind);
  }

  function isMovable(kind) {
    return kind === "warp" || kind === "sign" || kind === "sprite" || kind === "player";
  }

  /// Is this spot's object currently switched OFF by the save? (WorldMissables: set = HIDDEN.)
  function isHidden(spot) {
    if (spot.kind !== "filterFlag")
      return false;
    strip.canvas.revision;
    return strip.canvas.wMissables ? strip.canvas.wMissables.missablesAt(spot.ind) : false;
  }

  /// ONE short line -- a tooltip on a dot is a label, not a page (the standard, rule 5).
  function labelOf(spot) {
    switch (spot.kind) {
      case "filterFlag":
        return strip.isHidden(spot) ? qsTr("%1 — hidden").arg(spot.name) : spot.name;
      case "eventFlag":
        return spot.action === "reset" ? qsTr("%1 — turned off here").arg(spot.name) : spot.name;
      case "hiddenItem":
      case "hiddenCoin":
        return qsTr("%1 — hidden pickup").arg(spot.name);
      case "script":
        return spot.shape === "scriptRow" ? qsTr("Script — this whole row")
             : spot.shape === "scriptCol" ? qsTr("Script — this whole column")
             : qsTr("Script trigger");
      case "cardKeyDoor":
        return qsTr("Card Key door");
      case "tileTrait":
        return (spot.section === "wild") ? qsTr("%1 — wild Pokémon").arg(spot.name) : spot.name;
    }
    return spot.name;
  }

  Row {
    spacing: 1

    // NON-TILE first (left): the walk-grid family. Then the gap, then the tile traits (right).
    Repeater { model: strip.gridSpots; delegate: tabDelegate }

    Item {
      visible: strip.tileSpots.length > 0 && strip.gridSpots.length > 0
      height: 1
      width: Math.max(4, Math.round(4 * strip.canvas.zoom))
    }

    Repeater { model: strip.tileSpots; delegate: tabDelegate }
  }

  Component {
    id: tabDelegate

    Rectangle {
      id: tab
      required property var modelData

      readonly property bool hovered: tabHit.containsMouse

      width: Math.max(7, Math.round(6 * strip.canvas.zoom))
      height: width
      radius: 1
      color: strip.inkOf(modelData)
      border.width: 1
      border.color: tab.hovered ? "#ffffff" : Qt.darker(color, 1.6)
      scale: tab.hovered ? 1.35 : 1.0
      Behavior on scale { NumberAnimation { duration: 60 } }
      opacity: strip.isHidden(modelData) ? 0.45 : 1.0

      // A MouseArea, never a PointerHandler -- a TapHandler fires THROUGH a floating panel.
      // See notes/reference/qt-patterns.md (top of file).
      MouseArea {
        id: tabHit
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: tabHit.tabDragging ? Qt.ClosedHandCursor : Qt.PointingHandCursor
        preventStealing: true

        onEntered: {
          strip.canvas.litSpot = tab.modelData;   // light ITS box on the map
          strip.canvas.overTab = true;            // the ground stands down
        }
        onExited: {
          if (strip.canvas.litSpot === tab.modelData)
            strip.canvas.litSpot = null;
          strip.canvas.overTab = false;
        }

        // ── A MOVABLE spot's tab is a DRAG HANDLE (leadership, 2026-07-18) ──────────────────
        //
        // Pull it and the OBJECT moves, previewing live through the canvas proxy-drag and
        // committing through the same byte-exact move its own drag uses. The press SELECTS the
        // thing first -- so a filter-hidden sprite (Prof. Oak) stays visible as the selected
        // ghost after the drop instead of vanishing back into hiding the moment it lands.
        property point press
        property bool tabDragging: false
        property bool didDrag: false

        onPressed: (m) => {
          tabHit.press = Qt.point(m.x, m.y);
          if (strip.isMovable(tab.modelData.kind))
            strip.canvas.selectSpot(tab.modelData.kind, tab.modelData.ind);
        }

        onPositionChanged: (m) => {
          if (!tabHit.pressed || !strip.isMovable(tab.modelData.kind))
            return;
          if (!tabHit.tabDragging
              && Math.abs(m.x - tabHit.press.x) < 4 && Math.abs(m.y - tabHit.press.y) < 4)
            return;
          if (!tabHit.tabDragging)
            strip.lockedEntry = strip.entry;   // freeze the strip under the pressed mouse
          tabHit.tabDragging = true;
          const g = tabHit.mapToGlobal(m.x, m.y);
          const p = strip.canvas.tileAtGlobal(g.x, g.y);
          strip.canvas.proxyUpdate(tab.modelData.kind, tab.modelData.ind, p.x, p.y);
        }

        onReleased: {
          if (tabHit.tabDragging) {
            tabHit.tabDragging = false;
            tabHit.didDrag = true;
            strip.canvas.proxyCommit();
          }
          strip.lockedEntry = null;
        }
        onCanceled: {
          tabHit.tabDragging = false;
          tabHit.didDrag = false;
          strip.canvas.proxyCancel();
          strip.canvas.overTab = false;
          strip.lockedEntry = null;
        }

        // ⚠️ NEVER disabled -- every kind has a real destination, so every tab is live.
        onClicked: {
          if (tabHit.didDrag) {   // a drag ending, not a click
            tabHit.didDrag = false;
            return;
          }
          strip.spotClicked(modelData.kind, modelData.section, modelData.ind);
        }

      }

      // ⚠️ MapToolTip, NEVER the stock ToolTip (the map screen's absolute rule -- the stock one is
      // dark-on-translucent, ignores the "?" gate and floats wherever it likes, which is exactly
      // leadership's "tooltips often look really bad ... rendered very far away from the cursor
      // and very dark"). `followGlobalSetting: false`: a tab is a coloured dot, and its one-line
      // label is not a hint you opt into -- it is the only thing that says WHICH dot this is.
      MapToolTip {
        shown: tabHit.containsMouse && !tabHit.tabDragging
        followGlobalSetting: false
        delay: 200
        text: strip.labelOf(tab.modelData)
      }
    }
  }
}
