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
  Map.qml -- the CHASSIS. Bars, rails, the well, the dock, and the wiring between them. Nothing else
  lives in this file, and that is the point of it.

  The design of record is notes/plans/map-screen.md. In one picture:

      ┌──────────────────────────────────────────────────────────────────┐
      │  IDENTITY BAR      what is loaded                                │ 34
      ├────┬──────────────────────────────────────────────────┬─────┬────┤
      │ T  │  CONTEXT BAR   the options for what you're holding│  D  │ I  │ 32
      │ O  ├──────────────────────────────────────────────────┤  O  │ C  │
      │ O  │                                                  │  C  │ O  │
      │ L  │            THE MAP, in a dark well               │  K  │ N  │
      │    │                                                  │  1  │    │
      │ 44 │                                                  │  ▾  │ 44 │
      ├────┴──────────────────────────────────────────────────┴─────┴────┤
      │  STATUS BAR    where you are · what's under you · zoom           │ 26
      └──────────────────────────────────────────────────────────────────┘

  Four thin bars, each with ONE job, and two rails. What replaced what, and why:

    * The old info row carried the map name AND a contrast stepper AND three panel toggles. The
      tool options went to the CONTEXT BAR (Aseprite / Photoshop: the bar changes with what you are
      holding); the panel toggles went to the DOCK RAIL. What is left says what is loaded.
    * The old panels opened as columns BESIDE each other and, when they stopped fitting, silently
      EVICTED the longest-open one. The dock opens ONE panel, collapses to an icon rail, and floats
      the panel over the canvas rather than squeezing the map when the window is narrow. Panels do
      not stack. Nothing is evicted behind your back.
    * The old footer was a colour legend plus zoom buttons. The STATUS BAR replaced it with what a
      map editor actually needs on every mouse-move -- the coordinates in both of the game's systems,
      the block under the cursor, and what that tile DOES in words. (The legend's job moves to the
      Layers panel in phase 2: a legend that lives on the layer it explains cannot drift out of sync
      with the renderer.)

  ⚠️ Root id is `mapScreen`, not `top` -- see MapCanvas.qml's note on the Repeater landmine.
*/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import "map"

Page {
  id: mapScreen
  objectName: "mapScreen"   // so the DEBUG harness can drive it (reference/dev-harness.md)

  /// The map's hard floor. It is the screen; it does not get to vanish so that furniture can fit.
  /// Below this the dock stops taking width and starts floating over the canvas instead.
  readonly property int mapMinWidth: 280

  // ── Handles for the DEBUG harness ───────────────────────────────────────────────────────────
  //
  // The harness can only SET PROPERTIES on NAMED QML ITEMS -- `brg.map` is a C++ model, not an item,
  // and its selection is a method. Without these there is no way for automation to switch a layer on
  // or select a block, which means the mandatory screenshot review cannot reach the thing it is
  // reviewing. A review that cannot turn the feature on is not a review. (reference/dev-harness.md)

  /// The shown overlay layers, as a bit set. Mirrors brg.map.layers.
  property int layerBits: brg.map.layers
  onLayerBitsChanged: if (brg.map.layers !== layerBits) brg.map.layers = layerBits

  /// "x,y" in BUFFER pixels -> select the block there, exactly as a click would.
  property alias selectAt: canvas.selectAt

  /// Which tool is in hand, and which dock panel is open -- both drivable by name.
  property alias tool: toolRail.tool
  property alias dockPanel: dock.open

  // Keyboard: the tools, and Space-to-pan. A tool the keyboard cannot reach is a tool half the
  // people who need it will never use.
  focus: true

  Keys.onPressed: (event) => {
    switch (event.key) {
    case Qt.Key_V:     toolRail.tool = "select"; event.accepted = true; break;
    case Qt.Key_H:     toolRail.tool = "pan";    event.accepted = true; break;
    case Qt.Key_Z:     toolRail.tool = "zoom";   event.accepted = true; break;
    case Qt.Key_Space: canvas.spaceHeld = true;  event.accepted = true; break;
    case Qt.Key_Escape:
      if (dock.open !== "") { dock.open = ""; event.accepted = true; }
      break;
    }
  }

  Keys.onReleased: (event) => {
    if (event.key === Qt.Key_Space && !event.isAutoRepeat) {
      canvas.spaceHeld = false;
      event.accepted = true;
    }
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    MapIdentityBar {
      Layout.fillWidth: true
    }

    RowLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      spacing: 0

      MapToolRail {
        id: toolRail
        Layout.fillHeight: true
      }

      // The canvas column: the context bar sits directly over the map it configures (Aseprite puts
      // it exactly there, and it is right -- the options belong to the canvas, not to the window).
      ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: mapScreen.mapMinWidth
        spacing: 0

        MapContextBar {
          Layout.fillWidth: true
          tool: toolRail.tool
          canvas: canvas
        }

        MapCanvas {
          id: canvas
          Layout.fillWidth: true
          Layout.fillHeight: true

          tool: toolRail.tool
          hoveredTile: dock.hoveredTile

          // A click on the map opens the panel that explains what it selected. A click that does
          // something invisible is a click that did nothing.
          onBlockSelected: dock.show("blocks")
        }
      }

      MapDock {
        id: dock
        Layout.fillHeight: true
        Layout.preferredWidth: inlineWidth

        // How much width the canvas could give up before it hits its floor. Below the panel's
        // minimum the dock floats instead -- the map never gets squeezed to nothing.
        roomForPanel: mapScreen.width - mapScreen.mapMinWidth - toolRail.width - railWidth
      }
    }

    MapStatusBar {
      Layout.fillWidth: true
      at: canvas.at
      canvas: canvas
    }
  }

  // Closing the Music panel stops playback: no invisible audio, and nothing starts making noise
  // because a screen opened. (notes/plans/music.md §6)
  //
  // The dock UNLOADS a closed panel, so this also covers "switched to another panel" -- but the
  // stop is explicit rather than relying on a destructor to be polite about it.
  Connections {
    target: dock
    function onOpenChanged() {
      if (dock.open !== "music")
        brg.music.stop();

      // A selection with nothing to say about it is a highlight in the way. Closing the panel that
      // explains it drops it.
      if (dock.open !== "blocks")
        brg.map.clearSelection();
    }
  }
}
