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
  MapCanvas.qml -- the map itself, floating in a dark well.

  Every number here comes from brg.map (MapModel) in BUFFER PIXELS -- one screen pixel per Game Boy
  pixel, 32 to a block, origin at the top-left of the 3-block border ring -- and QML's only job is to
  multiply by an integer `zoom`. If a rectangle looks wrong, the bug is in C++ (MapEngine), not here.
  Keep it that way.

  What is drawn, outermost first:
    * the WELL              -- a dark neutral surround. Every pixel editor does this, and for the
                               same reason: Game Boy art is four shades of grey, and a white page
                               behind it drowns the light end. The map floats on it with a shadow.
    * the rendered image    -- the map PLUS its border ring (the game keeps the map inside that ring
                               so connected maps can bleed in)
    * the overlay image     -- the meaning layers, rendered by MapEngine as ONE transparent image
    * the block grid        -- the 32px cells a map is really made of
    * the map bounds        -- where the real map ends and the ring begins
    * the draw area (6x5 blocks) -- what LoadCurrentMapView redraws (wSurroundingTiles)
    * the player            -- where the console's own OAM puts him (4px above his tile row)
    * the screen (20x18 tiles)   -- what the player actually SEES, sliding around inside the draw
                               area in half-block steps

  (In phase 2 those last five stop being hard-coded rectangles and become LAYERS you can switch off.
  Today they are exactly what they were, just re-homed. -- notes/plans/map-screen.md)

  ⚠️ The root id is `canvasRoot`, NOT `top`: this file has Repeaters, and a Repeater delegate cannot
  see the file's root id -- `top.zoom` inside one comes back `undefined`, x goes NaN, and every grid
  line silently collapses onto x = 0 with no warning and a green tst_qml_screens. Inside a delegate,
  reach values through a plain sibling id (`canvas`). See reference/qt-patterns.md.
*/
import QtQuick
import QtQuick.Controls

Item {
  id: canvasRoot

  /// The active tool, from the rail: "select" | "pan" | "zoom".
  property string tool: "select"

  /// The sprite SLOT currently selected (1-15), or -1 for nothing. Sprites are the only
  /// selectable object on the map right now -- the ground is deliberately not clickable.
  property int selectedNpc: -1

  // ── Zoom ────────────────────────────────────────────────────────────────────────────────────
  //
  // Integer only -- this is 8x8 pixel art and a fractional scale would smear it. Until the user
  // says otherwise (userZoom == 0) the map shows at the largest whole multiple that FITS, so
  // opening the screen shows you the map rather than a corner of it. Route 17 is 78 blocks tall and
  // will still land on 1x, which is correct -- it simply is bigger than the window.
  readonly property int minZoom: 1
  readonly property int maxZoom: 8

  property int userZoom: 0
  readonly property int fitZoom: (brg.map.imageWidth > 0 && view.width > 0)
    ? Math.max(minZoom, Math.min(maxZoom,
        Math.floor(Math.min(view.width / brg.map.imageWidth,
                            view.height / brg.map.imageHeight))))
    : minZoom
  readonly property int zoom: userZoom > 0 ? userZoom : fitZoom

  readonly property int scaledWidth: brg.map.imageWidth * zoom
  readonly property int scaledHeight: brg.map.imageHeight * zoom

  /// What is under the cursor (brg.map.describeAt()), or null when it is off the map. The status
  /// bar reads this; nothing else does.
  property var at: null

  /// Space held = pan with any tool (the universal convention, and it costs nothing).
  property bool spaceHeld: false

  readonly property bool panning: tool === "pan" || spaceHeld

  // The dark well. Not black: black would make the darkest Game Boy grey vanish into it.
  Rectangle {
    anchors.fill: parent
    color: "#2b2b2b"
  }

  Flickable {
    id: view

    anchors.fill: parent
    clip: true

    contentWidth: Math.max(width, canvasRoot.scaledWidth)
    contentHeight: Math.max(height, canvasRoot.scaledHeight)
    boundsBehavior: Flickable.StopAtBounds

    // Both axes, and a drag anywhere pans -- the map is the thing, not the scrollbars.
    flickableDirection: Flickable.HorizontalAndVerticalFlick
    interactive: true

    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

    // Genuinely nothing in the game to draw, and nothing this id is a copy of either.
    Text {
      anchors.centerIn: parent
      visible: !brg.map.valid
      text: qsTr("There is no map data in the game for this one.")
      font.pixelSize: 13
      color: "#bdbdbd"
    }

    Item {
      id: canvas

      visible: brg.map.valid

      // Centre the map while it is smaller than the view, then let it grow past it.
      x: Math.max(0, (view.contentWidth - width) / 2)
      y: Math.max(0, (view.contentHeight - height) / 2)
      width: canvasRoot.scaledWidth
      height: canvasRoot.scaledHeight

      readonly property int gridStep: brg.map.blockSize * canvasRoot.zoom

      // The map floats: a soft shadow under it, so the well reads as BEHIND rather than as a
      // border drawn around the art.
      Rectangle {
        anchors.fill: parent
        anchors.margins: -6
        color: "#40000000"
        radius: 3
        z: -1
      }

      // The whole overworld buffer: the map inside its border ring.
      Image {
        anchors.fill: parent
        source: brg.map.source
        smooth: false           // pixel art -- never interpolate it
        mipmap: false
        fillMode: Image.Stretch
        cache: true
      }

      // The semantic overlay -- walls, grass, warps... -- rendered by MapEngine as ONE transparent
      // image exactly the size of the map, so it can simply sit on top.
      //
      // An image, not a pile of QML rectangles, and that is not premature: Route 17 is 78 blocks
      // tall, which is over 20,000 tiles. As delegates it would crawl; as an image it scales with
      // the zoom for free and fades as one thing.
      Image {
        anchors.fill: parent
        source: brg.map.overlaySource

        // The map stays the point. The layers arrive, they don't slam on. How HARD they are painted
        // is the Layers panel's one dial -- stacked annotation over four shades of grey needs it.
        opacity: brg.map.layers !== 0 ? brg.mapLayers.overlayOpacity : 0
        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

        smooth: false
        mipmap: false
        fillMode: Image.Stretch
        cache: true
      }

      // ── The block grid ──────────────────────────────────────────────────────────────────────
      //
      // TINTED rather than grey on purpose: the map is four shades of grey, so a grey line
      // disappears into it (over the black trees or over the white paths, depending which grey you
      // pick). A low-alpha colour has nothing to hide against and reads everywhere without
      // shouting over the three boxes.
      // ── The ink ──────────────────────────────────────────────────────────────────────────────
      //
      // Okabe-Ito, the colour-blind-safe set, re-picked 2026-07-13 (Twilight: "lots of red outlines
      // and it looks confusing"). Three warm theme colours -- error red, primary pink, accent blue --
      // over a grey map read as one alarming mess and told you nothing. These stay distinct to every
      // kind of colour vision and sit quietly over four shades of grey.
      //
      // The SAME values are in MapLayersModel, which is what the Layers panel's swatches show -- so
      // the panel is the legend, and it cannot drift.
      readonly property color boundsColor: "#0072b2"   // blue   -- where the map ENDS
      readonly property color drawColor:   "#009e73"   // green  -- what the game REDRAWS
      readonly property color screenColor: "#e69f00"   // orange -- what the player SEES
      readonly property color selectColor: "#cc79a7"   // purple     -- what YOU picked
      readonly property color connectColor: "#d55e00"  // vermillion -- the NEIGHBOURS in the ring

      readonly property color gridColor: Qt.rgba(0.34, 0.71, 0.91, 0.42)
      readonly property color tileGridColor: Qt.rgba(0.34, 0.71, 0.91, 0.18)
      readonly property int tileStep: brg.map.blockSize / 4 * canvasRoot.zoom

      // The TILE grid (8px). Off by default -- it is four times as many lines, and at 1x they would
      // be a hairline every eight pixels. Under the block grid, so the coarse structure still reads.
      Repeater {
        model: brg.mapLayers.showTileGrid && canvas.tileStep >= 4
               ? Math.floor(canvas.width / canvas.tileStep) + 1 : 0
        Rectangle {
          required property int index
          x: index * canvas.tileStep
          y: 0
          width: 1
          height: canvas.height
          color: canvas.tileGridColor
        }
      }

      Repeater {
        model: brg.mapLayers.showTileGrid && canvas.tileStep >= 4
               ? Math.floor(canvas.height / canvas.tileStep) + 1 : 0
        Rectangle {
          required property int index
          x: 0
          y: index * canvas.tileStep
          width: canvas.width
          height: 1
          color: canvas.tileGridColor
        }
      }

      Repeater {
        model: brg.mapLayers.showBlockGrid ? Math.floor(canvas.width / canvas.gridStep) + 1 : 0
        Rectangle {
          required property int index
          x: index * canvas.gridStep
          y: 0
          width: 1
          height: canvas.height
          color: canvas.gridColor
        }
      }

      Repeater {
        model: brg.mapLayers.showBlockGrid ? Math.floor(canvas.height / canvas.gridStep) + 1 : 0
        Rectangle {
          required property int index
          x: 0
          y: index * canvas.gridStep
          width: canvas.width
          height: 1
          color: canvas.gridColor
        }
      }

      // ── The connections ──────────────────────────────────────────────────────────────────────
      //
      // The ring is NOT a wall of trees: the game bleeds each neighbouring map's edge into it, and
      // where each strip lands is the hardest arithmetic in the whole engine (a clamp that turns one
      // signed offset into two numbers, and two different loop shapes -- see
      // reference/map-connections.md). It is worth being able to look at it, so here it is: the
      // strip, its neighbour's name, and its size in blocks.
      //
      // Every number comes from brg.map.connectionList() in buffer pixels. QML multiplies by zoom.
      Repeater {
        model: brg.mapLayers.showConnections ? brg.map.connectionList() : []

        Rectangle {
          required property var modelData

          x: modelData.x * canvasRoot.zoom
          y: modelData.y * canvasRoot.zoom
          width: modelData.w * canvasRoot.zoom
          height: modelData.h * canvasRoot.zoom

          // No FILL (Twilight, 2026-07-13). An outline shows you the strip; a wash over it hides the
          // map you are trying to look at -- which is the one thing every layer here must not do.
          color: "transparent"
          border.width: 2
          border.color: canvas.connectColor

          // Which neighbour, and which way. The strip's SIZE is not on the label: it is on the
          // rectangle, which you can see, and a label that recites what the outline already shows is
          // just noise.
          Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 2

            width: label.implicitWidth + 8
            height: label.implicitHeight + 4
            radius: 3
            color: "#e6212121"

            visible: parent.width > width + 4 && parent.height > height + 4

            Text {
              id: label
              anchors.centerIn: parent
              text: modelData.dirName + " · " + modelData.name
              font.pixelSize: 10
              color: "#ffffff"
            }
          }
        }
      }

      // Where the real map ends and the 3-block border ring begins.
      Rectangle {
        visible: brg.mapLayers.showMapBounds
        x: brg.map.mapX * canvasRoot.zoom
        y: brg.map.mapY * canvasRoot.zoom
        width: brg.map.mapW * canvasRoot.zoom
        height: brg.map.mapH * canvasRoot.zoom
        color: "transparent"
        border.width: 2
        border.color: canvas.boundsColor
      }

      // The draw area: the 6x5 blocks LoadCurrentMapView redraws. Always block-aligned.
      Rectangle {
        visible: brg.mapLayers.showDrawArea
        x: brg.map.scratchX * canvasRoot.zoom
        y: brg.map.scratchY * canvasRoot.zoom
        width: brg.map.scratchW * canvasRoot.zoom
        height: brg.map.scratchH * canvasRoot.zoom
        color: "transparent"
        border.width: 2
        border.color: canvas.drawColor
      }

      // The player. He is drawn 4px ABOVE his tile row -- "which makes sprites appear to be in the
      // centre of a tile" (ram/wram.asm), confirmed against the console's own OAM. Facing right is
      // facing LEFT, mirrored: there is no right-facing art in the game.
      //
      // He is drawn through the OBJECT palette (rOBP0), which is the one the "harmless" glitch
      // palettes actually wreck -- contrast 1 and 2 leave the map looking perfectly normal and do
      // their damage here. (reference/sprites.md)
      Image {
        visible: brg.mapLayers.showPlayer
        x: brg.map.playerRectX * canvasRoot.zoom
        y: brg.map.playerRectY * canvasRoot.zoom
        width: brg.map.playerRectW * canvasRoot.zoom
        height: brg.map.playerRectH * canvasRoot.zoom

        source: brg.map.playerSource
        smooth: false
        mipmap: false
        fillMode: Image.Stretch
        cache: true
      }

      // ── Everybody else ──────────────────────────────────────────────────────────────────────
      //
      // The other fifteen sprite slots: every NPC, item ball and boulder the save has put on this
      // map. Same geometry as the player (they ARE the player's geometry -- he is slot 0), same
      // OBJECT palette, same "there is no right-facing art" rule.
      //
      // Click one to select it. The ground is NOT clickable any more (Twilight, 2026-07-13):
      // blocks and tiles are edited in their panels, and the canvas should not compete with them.
      Repeater {
        model: brg.mapLayers.showNpcs ? brg.map.npcList() : []

        delegate: Item {
          id: npc

          required property var modelData

          x: npc.modelData.rectX * canvasRoot.zoom
          y: npc.modelData.rectY * canvasRoot.zoom
          width: npc.modelData.rectW * canvasRoot.zoom
          height: npc.modelData.rectH * canvasRoot.zoom

          readonly property bool selected: canvasRoot.selectedNpc === npc.modelData.slot

          Image {
            anchors.fill: parent
            source: npc.modelData.source
            smooth: false
            mipmap: false
            fillMode: Image.Stretch
            cache: true
          }

          // The one thing you cannot see by looking at a sprite: whether this map has actually
          // LOADED its picture. If it hasn't, the console draws garbage there -- so we say so
          // rather than drawing it correctly and letting the user find out on the cartridge.
          Rectangle {
            visible: !npc.modelData.inSpriteSet && !npc.selected
            anchors.fill: parent
            color: "transparent"
            border.width: 1
            border.color: "#ffd54f"      // amber -- a warning, not an error
            opacity: 0.9
          }

          // A selection you can lose under a layer is not a selection: it draws above everything.
          Rectangle {
            visible: npc.selected
            z: 20
            anchors.fill: parent
            anchors.margins: -2
            color: "transparent"
            border.width: 2
            border.color: canvas.selectColor

            Rectangle {
              anchors.fill: parent
              anchors.margins: 2
              color: "transparent"
              border.width: 1
              border.color: "#ccffffff"
            }
          }

          TapHandler {
            enabled: !canvasRoot.panning && canvasRoot.tool !== "zoom"
            onTapped: canvasRoot.selectedNpc = npc.modelData.slot
          }

          HoverHandler {
            cursorShape: Qt.PointingHandCursor
          }
        }
      }

      // The visible screen: the 20x18 tiles actually on the Game Boy's screen.
      Rectangle {
        visible: brg.mapLayers.showScreenBox
        x: brg.map.screenX * canvasRoot.zoom
        y: brg.map.screenY * canvasRoot.zoom
        width: brg.map.screenW * canvasRoot.zoom
        height: brg.map.screenH * canvasRoot.zoom
        color: "transparent"
        border.width: 2
        border.color: canvas.screenColor
      }

      // ── Pointing at things ──────────────────────────────────────────────────────────────────
      //
      // ⚠️ The GROUND IS NOT SELECTABLE (Twilight, 2026-07-13). Clicking a block used to mark it;
      // it no longer does anything at all. Blocks and tiles are edited in their own panels, and a
      // clickable floor only fights the thing that should be clickable. **Sprites are, for now,
      // the only selectable object on the map** -- warps, signs and connections join them later,
      // on this same machinery.
      //
      // The hover readout stays: the status bar still tells you what you are pointing at, which
      // costs nothing and interrupts nobody.
      //
      // Below the Flickable's own drag handling, so a drag still pans the map and only a genuine
      // click acts.
      HoverHandler {
        id: hover
        cursorShape: canvasRoot.panning ? Qt.OpenHandCursor
                   : canvasRoot.tool === "zoom" ? Qt.CrossCursor
                   : Qt.ArrowCursor

        onPointChanged: {
          const px = Math.floor(point.position.x / canvasRoot.zoom);
          const py = Math.floor(point.position.y / canvasRoot.zoom);
          canvasRoot.at = brg.map.describeAt(px, py);
        }
      }

      TapHandler {
        onTapped: (eventPoint) => {
          const px = Math.floor(eventPoint.position.x / canvasRoot.zoom);
          const py = Math.floor(eventPoint.position.y / canvasRoot.zoom);

          if (canvasRoot.tool === "zoom") {
            view.zoomAround(canvasRoot.zoom + 1, eventPoint.position);
            return;
          }

          if (canvasRoot.panning)
            return;   // the hand does not select

          // Clicking the ground clears the sprite selection -- and does nothing else. The block
          // under the cursor is NOT selected any more; see the note above.
          canvasRoot.selectedNpc = -1;
        }
      }
    }

    // ── Navigation ───────────────────────────────────────────────────────────────────────────
    //
    // Zoom ANCHORS on the thing you are pointing at (the cursor, or the middle of the pinch) rather
    // than the top-left corner. Zooming into a corner you aren't looking at is the single most
    // annoying thing a map viewer can do.

    /// Keep the point under `centre` (in viewport coords) fixed across a zoom change.
    function zoomAround(newZoom, centre) {
      newZoom = Math.max(canvasRoot.minZoom, Math.min(canvasRoot.maxZoom, Math.round(newZoom)));
      if (newZoom === canvasRoot.zoom) {
        canvasRoot.userZoom = newZoom;
        return;
      }

      // Where that point sits on the MAP right now, in unscaled buffer pixels.
      const mapX = (view.contentX + centre.x - canvas.x) / canvasRoot.zoom;
      const mapY = (view.contentY + centre.y - canvas.y) / canvasRoot.zoom;

      canvasRoot.userZoom = newZoom;

      // canvas.x/y and contentWidth/Height are bindings; let them settle before we put that same
      // map point back under the cursor.
      Qt.callLater(function() {
        view.contentX = Math.max(0, Math.min(view.contentWidth - view.width,
                                             canvas.x + mapX * canvasRoot.zoom - centre.x));
        view.contentY = Math.max(0, Math.min(view.contentHeight - view.height,
                                             canvas.y + mapY * canvasRoot.zoom - centre.y));
      });
    }

    // Pinch: touchscreen, and a touchpad's two-finger pinch. Integer zoom only (pixel art), so the
    // scale is snapped -- but it tracks the gesture live.
    PinchHandler {
      target: null
      onScaleChanged: (delta) => {
        if (Math.abs(activeScale - 1) < 0.15)
          return;

        view.zoomAround(canvasRoot.zoom * activeScale, centroid.position);
      }
    }

    // Ctrl+wheel (and a touchpad's pinch, which most platforms report as Ctrl+wheel).
    WheelHandler {
      acceptedModifiers: Qt.ControlModifier
      onWheel: (event) => {
        view.zoomAround(canvasRoot.zoom + (event.angleDelta.y > 0 ? 1 : -1), point.position);
      }
    }

    // A plain wheel scrolls -- vertically, and HORIZONTALLY when the wheel/trackpad says so
    // (Shift+wheel, or a real two-finger sideways swipe). Flickable does the vertical axis on its
    // own; the horizontal one it ignores unless we hand it over.
    WheelHandler {
      acceptedModifiers: Qt.NoModifier | Qt.ShiftModifier
      onWheel: (event) => {
        const dx = (event.angleDelta.x !== 0)
                 ? event.angleDelta.x
                 : ((event.modifiers & Qt.ShiftModifier) ? event.angleDelta.y : 0);

        if (dx !== 0) {
          view.contentX = Math.max(0, Math.min(view.contentWidth - view.width,
                                               view.contentX - dx));
          event.accepted = true;
          return;
        }

        event.accepted = false;  // let the Flickable scroll vertically
      }
    }
  }

  /// "x,y" in BUFFER pixels -> select the block there, exactly as a click would. (DEBUG harness:
  /// it can only set properties on named items, and `brg.map` is a model, not an item.)
  property string selectAt: ""
  onSelectAtChanged: {
    const p = selectAt.split(",");
    if (p.length !== 2)
      return;

    brg.map.selectAtPixel(parseInt(p[0]), parseInt(p[1]));
  }
}
