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

  /// Which tool is in hand, and which panel each dock has open -- all drivable by name.
  property alias tool: identityBar.tool
  property alias dockPanel: rightDock.open
  property alias layersPanel: leftDock.open

  /// The top bar's two drop-downs (the map/tileset/blockset picker, and the palette).
  property alias mapPickerOpen: identityBar.mapPickerOpen
  property alias contrastPickerOpen: identityBar.contrastPickerOpen
  property alias contrastShowGlitch: identityBar.contrastShowGlitch

  /// The animation, mirrored for the harness: does this map animate, is it running, which step.
  /// (`brg.mapClock` is a C++ object, not a QML item, so automation cannot reach it directly.)
  readonly property bool animates: brg.mapClock.animates
  property bool animPlaying: brg.mapClock.playing
  onAnimPlayingChanged: if (brg.mapClock.playing !== animPlaying) brg.mapClock.playing = animPlaying
  property int animFrame: brg.mapClock.frame

  // Keyboard: the tools, and Space-to-pan. A tool the keyboard cannot reach is a tool half the
  // people who need it will never use.
  focus: true

  Keys.onPressed: (event) => {
    switch (event.key) {
    case Qt.Key_V:     identityBar.tool = "select"; event.accepted = true; break;
    case Qt.Key_H:     identityBar.tool = "pan";    event.accepted = true; break;
    case Qt.Key_Z:     identityBar.tool = "zoom";   event.accepted = true; break;
    case Qt.Key_Space: canvas.spaceHeld = true;  event.accepted = true; break;
    case Qt.Key_Escape:
      if (rightDock.open !== "" || leftDock.open !== "") {
        rightDock.open = "";
        leftDock.open = "";
        event.accepted = true;
      }
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

    // The tools live HERE now, in the top bar, next to what they act on -- and the left edge went
    // back to the map (Twilight, 2026-07-13). A rail of three buttons was 44px of chrome down the
    // whole height of the screen to hold three glyphs.
    MapIdentityBar {
      id: identityBar
      canvas: canvas          // zoom lives here, and only here
      Layout.fillWidth: true
    }

    RowLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      spacing: 0

      // ── LEFT: ONE dock, three panels, ONE open at a time ─────────────────────────────────────
      //
      // ⚠️ The Characters bar was briefly its OWN rail beside this dock. That was a mistake and it
      // broke the rule this whole screen was rebuilt around: **panels do not stack out beside each
      // other.** It is a panel in this dock like everything else (Twilight, 2026-07-13).
      //
      // The map's legend, the cast you can place, and the editor for whatever is selected. All on
      // the left, where your other hand is; the panels that configure the MAP ITSELF are on the
      // right.
      MapDock {
        id: leftDock
        objectName: "mapLeftDock"

        side: "left"

        // Nothing open (Twilight). You open the Map screen and you see THE MAP.
        open: ""

        panels: [
          { id: "layers", glyph: "◈", title: qsTr("Layers"),
            tip: qsTr("Layers — everything drawn over the map, in groups") },
          { id: "characters", glyph: "☻", title: qsTr("Characters"),
            tip: qsTr("Characters — the people and objects you can put on the map. Drag one out.") },
          { id: "details", glyph: "✎", title: qsTr("Details"),
            tip: qsTr("Details — edit whatever is selected. Nothing selected? The map itself.") }
        ]
        sources: ({ "layers": "LayersPanel.qml",
                    "characters": "CharactersBar.qml",
                    "details": "DetailsPanel.qml" })

        panelContext: canvas

        Layout.fillHeight: true
        Layout.preferredWidth: inlineWidth

        roomForPanel: mapScreen.width - mapScreen.mapMinWidth - railWidth - rightDock.inlineWidth
      }

      MapCanvas {
        id: canvas
        objectName: "mapCanvas"   // the DEBUG harness drives selection through this

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: mapScreen.mapMinWidth

        tool: identityBar.tool

        // The ✎ button on a sprite opens the Details panel ON it -- no hunting for the panel.
        onEditRequested: (slot) => { leftDock.open = "details"; }
      }

      // ── RIGHT: the things you edit ───────────────────────────────────────────────────────────
      MapDock {
        id: rightDock

        side: "right"
        open: ""

        // "Blocks & Tiles", not "Tiles" (Twilight, 2026-07-13: "Tiles is not very good or accurate --
        // it has to do with the block and tile config on the map"). It is exactly that: the BLOCK
        // that fills the edge of the world, and the TILES that mean grass, counter, and boulder.
        panels: [
          { id: "tiles", glyph: "▦", title: qsTr("Blocks & Tiles"),
            tip: qsTr("Blocks & Tiles — the edge of the world, the grass, the counters, the boulder") },
          { id: "sprites", glyph: "☻", title: qsTr("Sprite set"),
            tip: qsTr("Sprite set — the eleven sprite pictures the game had loaded for this map") },
          { id: "music", glyph: "♪", title: qsTr("Music"),
            tip: qsTr("Music — the map's track, and play it") }
        ]
        sources: ({ "tiles": "TilesetPanel.qml",
                    "sprites": "SpriteSetPanel.qml",
                    "music": "MusicPanel.qml" })

        Layout.fillHeight: true
        Layout.preferredWidth: inlineWidth

        // How much width the canvas could give up before it hits its floor. Below the panel's
        // minimum the dock floats instead -- the map never gets squeezed to nothing.
        roomForPanel: mapScreen.width - mapScreen.mapMinWidth - railWidth - leftDock.inlineWidth
      }
    }

    // Everything ABOUT what you're looking at lives down here, not in the toolbar (Twilight,
    // 2026-07-13): the cursor's coordinates, the block under it, the map's size, whether this id is
    // an unfinished copy, the animation, the zoom.
    MapStatusBar {
      Layout.fillWidth: true
      at: canvas.at
      canvas: canvas
    }
  }

  // Closing the Music panel stops playback: no invisible audio, and nothing starts making noise
  // because a screen opened. (notes/plans/music.md §6)
  //
  // The dock UNLOADS a closed panel, so this also covers "switched to another panel" -- but the stop
  // is explicit rather than relying on a destructor to be polite about it.
  Connections {
    target: rightDock
    function onOpenChanged() {
      if (rightDock.open !== "music")
        brg.music.stop();
    }
  }
}
