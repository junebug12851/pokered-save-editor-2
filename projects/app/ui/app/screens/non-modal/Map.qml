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
  ///
  /// ⚠️ The docks no longer take any width from it AT ALL -- their panels always float over the
  /// canvas (Twilight: "map does not need to resize on panel changes"). This is now just the floor
  /// for the WINDOW getting small.
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
  ///
  /// ⚠️ `tool` is OWNED HERE now (2026-07-14). It used to live on the top bar, because that is where
  /// the tools were. They moved to the LEFT RAIL (Twilight), so the top bar no longer owns them and
  /// the screen does -- both the rail and the canvas read `mapScreen.tool`.
  property string tool: "select"
  property alias dockPanel: rightDock.open
  property alias layersPanel: leftDock.open

  /// The top bar's two drop-downs (the map/tileset/blockset picker, and the palette).
  property alias mapPickerOpen: identityBar.mapPickerOpen
  property alias contrastPickerOpen: identityBar.contrastPickerOpen
  property alias contrastShowGlitch: identityBar.contrastShowGlitch

  /// "Outside is…" — the wLastMap chip. Drivable, so the screenshot review can actually open it.
  property alias outsideOpen: identityBar.outsideOpen

  /// The "Useless edits" toggle (the toolbar "!"), mirrored. `brg.map` is a C++ model, so automation cannot set it
  /// directly — and without this the review could never SEE the four fields that do nothing, which
  /// is precisely the thing that needs reviewing.
  property bool showScratchNow: brg.map.showScratch
  onShowScratchNowChanged: if (brg.map.showScratch !== showScratchNow) brg.map.showScratch = showScratchNow

  /// The door currently selected, mirrored out of the canvas for the harness.
  property alias selectedWarp: canvas.selectedWarp

  /// The sign currently selected, mirrored out of the canvas for the harness.
  property alias selectedSign: canvas.selectedSign

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
    case Qt.Key_V:     mapScreen.tool = "select"; event.accepted = true; break;
    case Qt.Key_H:     mapScreen.tool = "pan";    event.accepted = true; break;
    case Qt.Key_Z:     mapScreen.tool = "zoom";   event.accepted = true; break;

    // The makers. "A tool is not done until it has a cursor, a context bar, an empty state and a
    // keyboard path" (map-screen.md §9) -- this is the keyboard path.
    case Qt.Key_W:     mapScreen.tool = "placeWarp";   event.accepted = true; break;
    case Qt.Key_N:     mapScreen.tool = "placeSprite"; event.accepted = true; break;
    case Qt.Key_S:     mapScreen.tool = "placeSign";   event.accepted = true; break;

    case Qt.Key_Space: canvas.spaceHeld = true;  event.accepted = true; break;
    case Qt.Key_Escape:
      // Esc walks OUT, one step at a time, in the order a person expects: put a maker tool down,
      // then drop the selection, then close the panels. Slamming all three shut at once is the kind
      // of "helpful" that loses you your place.
      if (canvas.placing) {
        mapScreen.tool = "select";
        event.accepted = true;
      } else if (canvas.selectedNpc >= 0 || canvas.selectedWarp >= 0 || canvas.selectedSign >= 0) {
        canvas.selectedNpc = -1;
        canvas.selectedWarp = -1;
        canvas.selectedSign = -1;
        event.accepted = true;
      } else if (rightDock.open !== "" || leftDock.open !== "") {
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

    // The top bar says WHAT IS LOADED -- the map picker, the palette, the music, the animation. The
    // TOOLS moved to the left rail (Twilight, 2026-07-14: *"move the tools onto the left toolbar at
    // the top above the panels, and the maker buttons below that"*), which is the natural home for
    // "what you DO" -- your off hand lives there, and it leaves this bar to say "what you're looking
    // AT". (It bounced back and forth: a left rail, then up here 2026-07-13, and now a left rail
    // again -- but this time it carries the makers too, so it earns the width it did not before.)
    MapIdentityBar {
      id: identityBar
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

        // Details FIRST (leadership, 2026-07-18: "Details needs to be moved above layers") — it is
        // the panel a click opens, so it owns the top slot.
        panels: [
          { id: "details", glyph: "✎", title: qsTr("Details"),
            tip: qsTr("Details — edit whatever is selected. Nothing selected? The map itself.") },
          { id: "layers", glyph: "◈", title: qsTr("Layers"),
            tip: qsTr("Layers — everything drawn over the map, in groups") },
          { id: "characters", glyph: "☻", title: qsTr("Characters"),
            tip: qsTr("Characters — the people and objects you can put on the map. Drag one out.") },
          { id: "wild", glyph: "✿", title: qsTr("Wild Pokémon"),
            tip: qsTr("Wild Pokémon — the grass and water encounters on this map") }
        ]
        sources: ({ "layers": "LayersPanel.qml",
                    "characters": "CharactersBar.qml",
                    "details": "DetailsPanel.qml",
                    "wild": "WildPokemonPanel.qml" })

        panelContext: canvas

        // ── The tools + the makers, above the panel icons — each group COLLAPSED to one button ──
        //
        // Twilight, 2026-07-14: *"collapse the left toolbar button groups into a single button each,
        // reduce it down to three buttons."* So the rail is three flyout groups now: Tools, Makers,
        // and (below, in the dock) Panels. Each face shows its ACTIVE member and flies the rest out.
        //
        // Declared HERE (in the screen), so the buttons can see `mapScreen`, `canvas` and the keyboard
        // — a Component resolves ids from where it is written, not from the Loader that instantiates
        // it. See MapDock.railHeader.
        railHeader: Component {
          Column {
            spacing: 4

            // ── Tools: Select · Pan · Zoom — how you LOOK at the map ──────────────────────────
            MapRailGroup {
              objectName: "toolsGroup"
              anchors.horizontalCenter: parent.horizontalCenter

              members: [
                { id: "select", glyph: "↖", shortcut: "V",
                  tip: qsTr("Select & Move — click to select, drag to move") },
                { id: "pan", glyph: "✥", shortcut: "H",
                  tip: qsTr("Pan — drag the map (or hold Space with any tool)") },
                { id: "zoom", glyph: "⌕", shortcut: "Z",
                  tip: qsTr("Zoom — click to zoom in, Alt-click to zoom out") }
              ]

              activeId: (mapScreen.tool === "select" || mapScreen.tool === "pan"
                         || mapScreen.tool === "zoom") ? mapScreen.tool : ""
              tip: qsTr("Tools — select, pan, zoom")

              onChosen: (id) => mapScreen.tool = id

              // The zoom ▾ (slider + "Go to…") rides inside the flyout, right after the zoom tool, so
              // ZOOM STILL LIVES IN EXACTLY ONE PLACE.
              //
              // ⚠️ `canvas: leftDock.panelContext`, NOT `canvas: canvas`. This header is built inside
              // the dock, BEFORE the MapCanvas exists, so a raw `canvas` id resolves to undefined and
              // never updates (an id is not notifiable). `panelContext` is a real property already
              // bound to the canvas. `tst_qml_screens` caught the raw-id version.
              ZoomMenu { canvas: leftDock.panelContext }
            }

            Rectangle {
              anchors.horizontalCenter: parent.horizontalCenter
              width: 22; height: 1
              color: brg.settings.dividerColor
            }

            // ── Makers: ⇄ place a door · ☻ place a person — how you CHANGE the map ────────────
            MapRailGroup {
              objectName: "makersGroup"
              anchors.horizontalCenter: parent.horizontalCenter

              members: [
                { id: "placeWarp", glyph: "⇄", shortcut: "W",
                  tip: qsTr("Place a warp — click a tile. It starts as a way back outside, which is what a warp usually is. (Up to 32.)") },
                { id: "placeSprite", glyph: "☻", shortcut: "N",
                  tip: qsTr("Place a character — click a tile. A random one, but only ever a picture THIS map has loaded, so it can never be one the console would draw as garbage. (Up to 15.)") },
                { id: "placeSign", glyph: "▤", shortcut: "S",
                  tip: qsTr("Place a sign — click a tile. It starts reading this map's first sign text; choose what it says in the panel. (Up to 16.)") }
              ]

              activeId: (mapScreen.tool === "placeWarp"
                         || mapScreen.tool === "placeSprite"
                         || mapScreen.tool === "placeSign") ? mapScreen.tool : ""
              tip: qsTr("Make — place a warp, a character or a sign")

              onChosen: (id) => mapScreen.tool = id
            }
          }
        }

        Layout.fillHeight: true
        Layout.preferredWidth: inlineWidth   // the RAIL, and only the rail. The panel floats.
      }

      MapCanvas {
        id: canvas
        objectName: "mapCanvas"   // the DEBUG harness drives selection through this

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: mapScreen.mapMinWidth

        tool: mapScreen.tool

        // Drag a sprite off the map and onto the open Characters panel and it goes back in the box.
        // The canvas does the containment test itself -- there is no DropArea anywhere on this screen.
        deleteZone: leftDock

        // ⚠️ The panels FLOAT over the canvas, so a click on one of them is geometrically also a
        // click on the map -- and Qt's pointer handlers fire for BOTH. Without this the map's ground
        // tap cleared the sprite selection every time you touched a control in the Details panel.
        // @see MapCanvas.overPanel
        overlays: [leftDock, rightDock]

        // A plain CLICK on any object (sprite, warp, sign, a stack member's tab) opens the Details
        // panel ON it -- no hunting for the panel. (A warp or sign passes -1: the panel reads
        // `selectedWarp`/`selectedSign`, not the slot.)
        onEditRequested: (slot) => { leftDock.open = "details"; }

        // A storage spot was reached -- from a tab, or from a single-spot block. The canvas is an
        // index into Map Storage: open the panel and land ON that row -- scrolled to and lit up --
        // rather than dropping her at the top of a page that can run to 227 rows.
        //
        // ONE signal for every storage kind, keyed by the section the spot says it lives in. Adding
        // the next kind of storage means adding a spot type in the model, not another signal here.
        // The dock's Loader is synchronous, so the panel exists by the time the next line runs; the
        // guard is for a panel that hasn't grown a reveal().
        onStorageRequested: (section, ind) => {
          // ⭐ ONE gesture, routed by the spot's own declared destination -- so clicking anything on
          // the canvas lands you where that thing actually lives, and the map answers the same way
          // everywhere. Twilight: *"clicking water doesnt even bring up wild mons it should at least
          // have that ... have proper standardization"*.
          //
          // The spot says where it belongs; this just obeys. Adding the next storage kind means
          // giving its spot a section, not another branch here.
          if (section === "wild") {
            leftDock.open = "wild";       // grass/water -> the encounter tables they really are
            return;
          }
          if (section === "tiles") {
            rightDock.open = "tiles";     // a tile trait is a TILESET fact -- open the tileset
            return;
          }
          if (section === "details") {
            leftDock.open = "details";    // a person, a door, a sign -> its own editor
            return;
          }
          rightDock.open = "storage";
          const p = rightDock.panelItem;
          if (p && p.reveal)
            p.reveal(section, ind);
        }

        // ⭐ A genuine click on the bare GROUND closes the open panels — *"if a click opens a panel
        // clicking off should close it"* (leadership, 2026-07-18). The refinement she asked for is
        // structural, not special-cased: this signal only fires when the tap was NOT on a panel,
        // NOT on the rail, NOT on a tab and NOT a popup's dismiss-press (the canvas returns early
        // for every one of those) — so switching panels, re-clicking the tab that opened one, or
        // working the rail can never accidentally close anything. Only the map itself does.
        onGroundClicked: {
          leftDock.open = "";
          rightDock.open = "";
        }

        // A maker tool put something down. The status bar says what and where -- never a modal, and
        // never nothing at all.
        onPlaced: (kind, index) => {
          canvas.status = kind === "warp"
            ? qsTr("Warp %1 placed. It leads back outside — pick where it really goes in the panel.")
                .arg(index)
            : kind === "sign"
            ? qsTr("Sign %1 placed. Choose what it says in the panel.").arg(index)
            : qsTr("Character placed in slot %1.").arg(index);

          leftDock.open = "details";   // you made it; here is what it is
        }
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
          { id: "sprites", glyph: "▧", title: qsTr("Sprite set"),
            tip: qsTr("Sprite set — the eleven sprite pictures the game had loaded for this map") },
          // ⇄ The twelve bytes AROUND the doors -- fly, hole, Dig, scripted. They belong to the MAP,
          // which is why they are here with the other things you edit about it, and not in the
          // Details panel (which is for whatever is SELECTED). Twilight asked for exactly this:
          // "I will place them in the right panel as warp state" (2026-07-14).
          { id: "warps", glyph: "⇄", title: qsTr("Warp state"),
            tip: qsTr("Warp state — where FLY goes, where falling drops you, where DIG puts you") },
          // ☺ The nine map-global character flags (v1's "NPC" page): whether NPCs face you, whether a
          // scripted cutscene has the controls, whether a trainer battle is queued. They belong to the
          // MAP, so they live here with the other things you edit about it -- not in the Details panel
          // (that is for whatever is SELECTED). Briefed + researched 2026-07-15; reference/npc-character-state.md.
          { id: "charstate", glyph: "⚙", title: qsTr("Character state"),
            tip: qsTr("Character state — how the characters on this map behave: facing, scripted control, trainer battle") },
          // ▣ MAP STORAGE — global save bytes that each belong to one map (Vermilion Gym trash-can
          // switches, Cinnabar Gym quiz opponent, Safari Zone run counters). `primary: true` gives its
          // rail icon the filled, accent-coloured "this holds persistent storage" look Twilight asked
          // for (2026-07-15). Briefed + researched this session; reference/gym-safari-state.md.
          // ⭐ "WORLD" is its name (leadership, 2026-07-18: "Map Storage should still be called
          // World") — the world's persistent storage, viewed one map at a time.
          { id: "storage", glyph: "▣", primary: true, title: qsTr("World"),
            tip: qsTr("World — the save's persistent storage, one map at a time: story flags, who's on the map, minigame state") }
        ]
        // ⚠️ MUSIC IS NOT HERE ANY MORE. It is a chip in the toolbar (MusicPicker.qml) -- a whole
        // dock panel for one combo, two checkboxes and a ▶ was a panel too many (Twilight).
        sources: ({ "tiles": "TilesetPanel.qml",
                    "sprites": "SpriteSetPanel.qml",
                    "warps": "WarpStatePanel.qml",
                    "charstate": "CharacterStatePanel.qml",
                    "storage": "MapStoragePanel.qml" })

        Layout.fillHeight: true
        Layout.preferredWidth: inlineWidth   // the RAIL, and only the rail. The panel floats.
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

  // Leaving the Map screen stops playback: no invisible audio, and nothing starts making noise
  // because a screen opened. (notes/plans/music.md §6)
  //
  // MusicPicker stops it on destruction too; this is the belt to that pair of braces.
  Component.onDestruction: brg.music.stop()
}
