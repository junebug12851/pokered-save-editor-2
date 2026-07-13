// Map.qml -- the map screen: the loaded map, drawn the way the Game Boy builds it.
//
// Everything here comes from brg.map (MapModel), in buffer pixels -- one screen pixel
// per Game Boy pixel, 32 to a block -- and is simply multiplied by `zoom`. Nothing is
// measured or guessed in QML; the geometry is the game's, computed in C++ (MapEngine).
//
// What is drawn, outermost first:
//   * the rendered image      -- the map PLUS its 3-block border ring (the game keeps
//                                the map inside that ring so connected maps can bleed in)
//   * the block grid          -- every 32 px, the blocks a map is actually made of
//   * the map bounds          -- where the real map ends and the border ring begins
//   * the scratch area (6x5 blocks) -- what LoadCurrentMapView redraws (wSurroundingTiles)
//   * the visible screen (20x18 tiles) -- what the player actually sees, sliding around
//                                inside the scratch area in half-block steps
//
// The player is deliberately NOT drawn yet.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import "../../fragments/general"
import "../../fragments/header"
import "map"

Page {
  id: mapScreen
  objectName: "mapScreen"   // so the DEBUG harness can drive it (reference/dev-harness.md)

  // Integer zoom only -- this is 8x8 pixel art and a fractional scale would smear it.
  //
  // Until the user says otherwise (userZoom == 0) the map shows at the largest whole
  // multiple that fits, so opening the screen shows you the map rather than a corner
  // of it. Route 17 is 78 blocks tall and will still land on 1x, which is correct --
  // it simply is bigger than the window.
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

  // ── The side panels ─────────────────────────────────────────────────────────
  //
  // Each has its own toggle and its own column (Twilight's call), and several can be open at
  // once. But "the map shrinks" must never become "the map disappears" -- with three 260px
  // panels open in the default 750px window there is simply no room left, and the whole
  // RowLayout overflows: the map is squeezed to nothing, the chip bar stops wrapping, and the
  // panels spill over the footer. (All four of those were real, and all four were this.)
  //
  // So the map keeps a hard floor, and only as many panels as actually FIT can be open. Open
  // one more than fits and the longest-open one steps aside -- predictable, never traps you,
  // and it keeps every toggle independent the way she asked.
  // Sized against the app's REAL default window, which is ~760 LOGICAL px (it renders at 1140
  // physical on a 1.5x display -- measure the logical width, not the screenshot). At 760 these
  // let two panels sit beside the map; on a resized or maximised window all three do.
  readonly property int mapMinWidth: 280
  readonly property int panelMinWidth: 220

  readonly property int maxPanels: Math.max(1, Math.floor((width - mapMinWidth) / panelMinWidth))

  /// The open panels, oldest first. The order IS the eviction order.
  property var openPanels: []

  readonly property bool tilesetOpen: openPanels.indexOf("tileset") >= 0
  readonly property bool blocksOpen: openPanels.indexOf("blocks") >= 0
  readonly property bool musicOpen: openPanels.indexOf("music") >= 0

  function togglePanel(name) {
    const next = openPanels.slice();
    const at = next.indexOf(name);

    if (at >= 0) {
      next.splice(at, 1);
    } else {
      next.push(name);
      while (next.length > maxPanels)
        next.shift();          // the longest-open one gives up its column
    }

    openPanels = next;
  }

  // Shrink the window and the panels have to give way too, not overflow it.
  onMaxPanelsChanged: {
    if (openPanels.length <= maxPanels)
      return;

    const next = openPanels.slice();
    while (next.length > maxPanels)
      next.shift();

    openPanels = next;
  }

  // What each open panel gets. They share what's left after the map's floor, and never take
  // more than they need.
  readonly property int panelWidth: Math.max(panelMinWidth,
    Math.min(268, (width - mapMinWidth) / Math.max(1, openPanels.length)))

  // Closing the music panel stops playback: no invisible audio, and nothing starts making
  // noise because a screen opened. (notes/plans/music.md §6)
  onMusicOpenChanged: if (!musicOpen) brg.music.stop()

  // Clicking a block opens the panel that explains it -- the click would otherwise do something
  // invisible. Closing that panel drops the selection, so there is never a highlight on the map
  // with nothing to say about it.
  onBlocksOpenChanged: if (!blocksOpen) brg.map.clearSelection()

  // Which tile of the selected block the Block panel is hovering -- mirrored onto the map, so
  // the two views are one thing. -1 = none.
  readonly property int hoveredTile: blockPanel.hoveredTile

  // ── Handles for the DEBUG harness ───────────────────────────────────────────
  //
  // The harness can only SET PROPERTIES on NAMED QML ITEMS. `brg.map` is a C++ model, not an
  // item, and its selection is a method -- so without these two there is no way for automation
  // to switch a layer on or select a block.
  //
  // That matters more than it sounds: the mandatory screenshot review has to be able to reach
  // the thing it is reviewing. A review that cannot turn the feature on is not a review. Both
  // are plain mirrors of state the UI already owns; neither adds behaviour.
  // (reference/dev-harness.md)

  /// The shown overlay layers, as a bit set. Mirrors brg.map.layers.
  property int layerBits: brg.map.layers
  onLayerBitsChanged: if (brg.map.layers !== layerBits) brg.map.layers = layerBits

  /// "x,y" in BUFFER pixels -> select the block there, exactly as a click would.
  property string selectAt: ""
  onSelectAtChanged: {
    const p = selectAt.split(",");
    if (p.length !== 2)
      return;

    brg.map.selectAtPixel(parseInt(p[0]), parseInt(p[1]));
    mapScreen.showBlockPanel();
  }

  /// Open the Blocks panel if it isn't already. (togglePanel would CLOSE it if it were.)
  function showBlockPanel() {
    if (brg.map.hasSelection && !blocksOpen)
      togglePanel("blocks");
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    // ── What is loaded ────────────────────────────────────────────────────────
    Rectangle {
      Layout.fillWidth: true
      implicitHeight: 38
      color: "transparent"

      RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12

        Text {
          text: brg.map.mapName
          font.pixelSize: 14
          font.bold: true
          color: brg.settings.textColorDark
          elide: Text.ElideRight
          Layout.maximumWidth: 200
        }

        Text {
          text: qsTr("Tileset: %1").arg(brg.map.tilesetName)
          font.pixelSize: 12
          color: brg.settings.textColorMid
        }

        Text {
          text: qsTr("%1 x %2 blocks").arg(brg.map.blocksWide).arg(brg.map.blocksHigh)
          font.pixelSize: 12
          color: brg.settings.textColorMid
        }

        Text {
          text: qsTr("Player at %1, %2").arg(brg.map.playerX).arg(brg.map.playerY)
          font.pixelSize: 12
          color: brg.settings.textColorMid
        }

        // A glitch / half-baked map id draws another map's data -- which is exactly what
        // the game does with it. Say so, rather than let it pass as a map of its own.
        Text {
          visible: brg.map.isCopy
          text: qsTr("⚠ unfinished copy — the game draws %1's data here").arg(brg.map.copyOfName)
          font.pixelSize: 12
          color: brg.settings.errorColor
          elide: Text.ElideRight
          // Room to say it in full; it only elides when the window really is too narrow.
          Layout.maximumWidth: implicitWidth
          Layout.minimumWidth: 0
        }

        Item { Layout.fillWidth: true }

        // ── Contrast (wMapPalOffset) ─────────────────────────────────────────
        //
        // Not a brightness slider. The game SUBTRACTS this byte from a pointer into its
        // fade-palette table, so 0/3/6/9 land on real entries (the four levels) and
        // everything else reads across the seam between two of them -- the six glitch
        // palettes. The map is drawn through whichever byte that produces, so a glitch
        // palette renders as the genuine article rather than an imitation of one.
        //
        // Hand-rolled rather than a SpinBox on purpose: Material controls fight small
        // heights (see ui-patterns.md), and a SpinBox here shoved the zoom controls clean
        // off the footer.
        Text {
          text: qsTr("Contrast")
          font.pixelSize: 11
          color: brg.settings.textColorDark
        }

        ContrastStep {
          text: "−"
          enabled: brg.map.contrast > 0
          onClicked: brg.map.contrast = brg.map.contrast - 1
        }

        Text {
          text: brg.map.contrast
          font.pixelSize: 12
          font.bold: true
          color: brg.map.contrastIsGlitch ? brg.settings.errorColor : brg.settings.textColorDark
          horizontalAlignment: Text.AlignHCenter
          Layout.minimumWidth: 14
        }

        ContrastStep {
          text: "+"
          enabled: brg.map.contrast < brg.map.contrastMax
          onClicked: brg.map.contrast = brg.map.contrast + 1
        }

        Text {
          text: brg.map.contrastName
          font.pixelSize: 11
          color: brg.map.contrastIsGlitch ? brg.settings.errorColor : brg.settings.textColorMid
          elide: Text.ElideRight
          Layout.maximumWidth: 210
        }

        // ── The panels ───────────────────────────────────────────────────────
        //
        // One toggle each, and each opens its own column beside the map. The map is a
        // Flickable, so it just gets narrower -- and it pans and zooms, so that costs
        // nothing you can't get back.

        // Tiles: the tileset, what animates, the grass, the counters. (map/TilesetPanel.qml)
        ContrastStep {
          objectName: "panelBtn_tileset"   // so the DEBUG harness can drive it
          text: "▦"
          Layout.leftMargin: 10   // don't crowd the contrast name
          highlighted: mapScreen.tilesetOpen
          onClicked: mapScreen.togglePanel("tileset")

          MainToolTip { text: qsTr("Tileset — what the tiles are and what they do") }
        }

        // Blocks: what you clicked, and the block the border ring is made of.
        ContrastStep {
          objectName: "panelBtn_blocks"
          text: "▣"
          highlighted: mapScreen.blocksOpen
          onClicked: mapScreen.togglePanel("blocks")

          MainToolTip { text: qsTr("Blocks — click the map to inspect one") }
        }

        // The map's music, its two save flags, and -- the point of all of it -- actually
        // playing the thing. Closed by default: no invisible audio.
        // See screens/non-modal/map/MusicPanel.qml and notes/plans/music.md.
        ContrastStep {
          objectName: "panelBtn_music"
          text: "♪"
          highlighted: mapScreen.musicOpen
          onClicked: mapScreen.togglePanel("music")

          MainToolTip { text: qsTr("Music") }
        }
      }
    }

    // ── Show: the semantic layers ─────────────────────────────────────────────
    //
    // Walls, grass, warps, doors, ledges, counters -- none of which you can see by looking at
    // the art. Off by default; the map is the point. Each chip carries the colour its layer
    // paints in, so the chips ARE the legend. (map/LayerChips.qml)
    LayerChips {
      Layout.fillWidth: true
    }

    Rectangle {
      Layout.fillWidth: true
      implicitHeight: 1
      color: brg.settings.dividerColor
    }

    // ── The map, and (when opened) the music panel beside it ──────────────────
    RowLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      spacing: 0

    Flickable {
      id: view

      Layout.fillWidth: true
      Layout.fillHeight: true

      // The map's floor. Without this the panels squeeze it to nothing and the whole row
      // overflows the window -- which is what made the chip bar stop wrapping and the panels
      // spill over the footer. The map is the screen; it does not get to vanish.
      Layout.minimumWidth: mapScreen.mapMinWidth

      clip: true
      contentWidth: Math.max(width, mapScreen.scaledWidth)
      contentHeight: Math.max(height, mapScreen.scaledHeight)
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
        color: brg.settings.textColorMid
      }

      Item {
        id: canvas

        visible: brg.map.valid

        // Centre the map while it is smaller than the view, then let it grow past it.
        x: Math.max(0, (view.contentWidth - width) / 2)
        y: Math.max(0, (view.contentHeight - height) / 2)
        width: mapScreen.scaledWidth
        height: mapScreen.scaledHeight

        // The whole overworld buffer: the map inside its border ring.
        Image {
          anchors.fill: parent
          source: brg.map.source
          smooth: false           // pixel art -- never interpolate it
          mipmap: false
          fillMode: Image.Stretch
          cache: true
        }

        // The semantic overlay -- walls, grass, warps... -- rendered by MapEngine as ONE
        // transparent image exactly the size of the map, so it can simply sit on top.
        //
        // An image, not a pile of QML rectangles, and that is not premature: Route 17 is 78
        // blocks tall, which is over 20,000 tiles. As delegates it would crawl; as an image it
        // scales with the zoom for free and fades as one thing.
        Image {
          anchors.fill: parent
          source: brg.map.overlaySource

          // The map stays the point. The layers arrive, they don't slam on.
          opacity: brg.map.layers !== 0 ? 1 : 0
          Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

          smooth: false
          mipmap: false
          fillMode: Image.Stretch
          cache: true
        }

        // ── Click a block ───────────────────────────────────────────────────
        //
        // Below the Flickable's own drag handling, so a drag still pans the map and only a
        // genuine click selects. Selecting opens the panel that explains the selection --
        // a click that does something invisible is a click that did nothing.
        TapHandler {
          onTapped: (eventPoint) => {
            const px = Math.floor(eventPoint.position.x / mapScreen.zoom);
            const py = Math.floor(eventPoint.position.y / mapScreen.zoom);

            brg.map.selectAtPixel(px, py);

            // Selecting opens the panel that explains the selection -- otherwise the click
            // did something invisible, which is the same as doing nothing.
            mapScreen.showBlockPanel();
          }
        }

        // The selected block. Held ABOVE everything -- the grid, the three boxes, the player,
        // and any overlay -- because a selection you can lose under a layer is not a selection.
        Rectangle {
          visible: brg.map.hasSelection
          z: 10

          x: brg.map.selectedBlockX * canvas.gridStep
          y: brg.map.selectedBlockY * canvas.gridStep
          width: canvas.gridStep
          height: canvas.gridStep

          color: "transparent"
          border.width: 2
          border.color: brg.settings.primaryColor

          // A second, inner line in white: the border alone can vanish against dark trees or
          // a glitch palette, and a selection you can't find is not a selection.
          Rectangle {
            anchors.fill: parent
            anchors.margins: 2
            color: "transparent"
            border.width: 1
            border.color: "#ccffffff"
          }
        }

        // The tile the Block panel is hovering, lit up on the map. Hovering a tile in the
        // panel and seeing it on the map is what makes the two one thing rather than two.
        Rectangle {
          visible: brg.map.hasSelection && mapScreen.hoveredTile >= 0
          z: 11

          readonly property int tileStep: canvas.gridStep / 4

          x: brg.map.selectedBlockX * canvas.gridStep
             + (mapScreen.hoveredTile % 4) * tileStep
          y: brg.map.selectedBlockY * canvas.gridStep
             + Math.floor(mapScreen.hoveredTile / 4) * tileStep
          width: tileStep
          height: tileStep

          color: Qt.rgba(1, 1, 1, 0.30)
          border.width: 1
          border.color: brg.settings.primaryColor
        }

        // The block grid -- the 32 px cells a map is really made of.
        //
        // The grid line is TINTED rather than grey on purpose: the map is four shades
        // of grey, so a grey line disappears into it (over the black trees or over the
        // white paths, depending which grey you pick). A low-alpha colour has nothing
        // to hide against and reads everywhere without shouting over the three boxes.
        readonly property color gridColor: Qt.rgba(0.16, 0.44, 0.75, 0.40)
        readonly property int gridStep: brg.map.blockSize * mapScreen.zoom

        // The delegates below multiply by `canvas.gridStep`, NOT by `mapScreen.zoom`,
        // and that is not a style choice. A Repeater delegate cannot see the file's
        // root id: `mapScreen.zoom` inside one of these comes back `undefined`, x goes
        // NaN, and all 17 lines collapse onto x = 0 -- silently, with no QML warning
        // and a green tst_qml_screens. Reading the value through a plain sibling id
        // (`canvas`) resolves it properly. See reference/qt-patterns.md -- this is the
        // same landmine that bit the keyboard deck, and it is why this file's root is
        // `mapScreen` rather than the project's usual `top`.
        Repeater {
          model: Math.floor(canvas.width / canvas.gridStep) + 1
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
          model: Math.floor(canvas.height / canvas.gridStep) + 1
          Rectangle {
            required property int index
            x: 0
            y: index * canvas.gridStep
            width: canvas.width
            height: 1
            color: canvas.gridColor
          }
        }

        // Where the real map ends and the 3-block border ring begins.
        Rectangle {
          x: brg.map.mapX * mapScreen.zoom
          y: brg.map.mapY * mapScreen.zoom
          width: brg.map.mapW * mapScreen.zoom
          height: brg.map.mapH * mapScreen.zoom
          color: "transparent"
          border.width: 2
          border.color: brg.settings.primaryColor
        }

        // The scratch area: the 6x5 blocks LoadCurrentMapView draws. Always block-aligned.
        Rectangle {
          x: brg.map.scratchX * mapScreen.zoom
          y: brg.map.scratchY * mapScreen.zoom
          width: brg.map.scratchW * mapScreen.zoom
          height: brg.map.scratchH * mapScreen.zoom
          color: "transparent"
          border.width: 2
          border.color: brg.settings.accentColor
        }

        // The player. He is drawn 4 px ABOVE his tile row -- "which makes sprites appear to
        // be in the centre of a tile" (ram/wram.asm), and confirmed against the console's
        // own OAM. Facing right is facing LEFT, mirrored: there is no right-facing art.
        //
        // He is drawn through the OBJECT palette (rOBP0), which is the one the "harmless"
        // glitch palettes actually wreck -- contrast 1 and 2 leave the map looking perfectly
        // normal and do their damage here. See reference/sprites.md.
        Image {
          x: brg.map.playerRectX * mapScreen.zoom
          y: brg.map.playerRectY * mapScreen.zoom
          width: brg.map.playerRectW * mapScreen.zoom
          height: brg.map.playerRectH * mapScreen.zoom

          source: brg.map.playerSource
          smooth: false
          mipmap: false
          fillMode: Image.Stretch
          cache: true
        }

        // The visible screen: the 20x18 tiles actually on the Game Boy's screen.
        Rectangle {
          x: brg.map.screenX * mapScreen.zoom
          y: brg.map.screenY * mapScreen.zoom
          width: brg.map.screenW * mapScreen.zoom
          height: brg.map.screenH * mapScreen.zoom
          color: "transparent"
          border.width: 2
          border.color: brg.settings.errorColor
        }
      }

      // ── Navigation ────────────────────────────────────────────────────────
      //
      // The map only gets bigger from here -- Route 17 is 78 blocks tall, and once whole
      // connected regions are drawn together it is a lot of surface area at a lot of
      // detail. So panning and zooming have to be properly good, not a scrollbar and a
      // pair of buttons.
      //
      // Zoom ANCHORS on the thing you are pointing at (the cursor, or the middle of the
      // pinch) rather than the top-left corner. Zooming into a corner you aren't looking
      // at is the single most annoying thing a map viewer can do.

      // Keep the point under `centre` (in viewport coords) fixed across a zoom change.
      function zoomAround(newZoom, centre) {
        newZoom = Math.max(mapScreen.minZoom, Math.min(mapScreen.maxZoom, Math.round(newZoom)));
        if (newZoom === mapScreen.zoom) {
          mapScreen.userZoom = newZoom;
          return;
        }

        // Where that point sits on the MAP right now, in unscaled buffer pixels.
        const mapX = (view.contentX + centre.x - canvas.x) / mapScreen.zoom;
        const mapY = (view.contentY + centre.y - canvas.y) / mapScreen.zoom;

        mapScreen.userZoom = newZoom;

        // canvas.x/y and contentWidth/Height are bindings; let them settle before we put
        // that same map point back under the cursor.
        Qt.callLater(function() {
          view.contentX = Math.max(0, Math.min(view.contentWidth - view.width,
                                               canvas.x + mapX * mapScreen.zoom - centre.x));
          view.contentY = Math.max(0, Math.min(view.contentHeight - view.height,
                                               canvas.y + mapY * mapScreen.zoom - centre.y));
        });
      }

      // Pinch: touchscreen, and a touchpad's two-finger pinch. Integer zoom only (pixel
      // art), so the scale is snapped -- but it tracks the gesture live.
      PinchHandler {
        target: null
        onScaleChanged: (delta) => {
          if (Math.abs(activeScale - 1) < 0.15)
            return;

          view.zoomAround(mapScreen.zoom * activeScale, centroid.position);
        }
      }

      // Ctrl+wheel (and a touchpad's pinch, which most platforms report as Ctrl+wheel).
      WheelHandler {
        acceptedModifiers: Qt.ControlModifier
        onWheel: (event) => {
          view.zoomAround(mapScreen.zoom + (event.angleDelta.y > 0 ? 1 : -1), point.position);
        }
      }

      // A plain wheel scrolls -- vertically, and HORIZONTALLY when the wheel/trackpad says
      // so (Shift+wheel, or a real two-finger sideways swipe). Flickable does the vertical
      // axis on its own; the horizontal one it ignores unless we hand it over.
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

      // The panels. One file each, so a redesign moves one thing.
      //
      // Separate toggles and separate columns (Twilight's call): several can be open at once.
      // The map is a Flickable and it pans and zooms, so the width it gives up is width you
      // can always get back.

      // A hidden panel must take NO width, or three of them still overflow the row while
      // showing nothing. `visible: false` alone does not do that in a Layout.
      TilesetPanel {
        visible: mapScreen.tilesetOpen
        Layout.preferredWidth: visible ? mapScreen.panelWidth : 0
        Layout.maximumWidth: Layout.preferredWidth
        Layout.fillHeight: true
      }

      BlockPanel {
        id: blockPanel
        visible: mapScreen.blocksOpen
        Layout.preferredWidth: visible ? mapScreen.panelWidth : 0
        Layout.maximumWidth: Layout.preferredWidth
        Layout.fillHeight: true
      }

      MusicPanel {
        visible: mapScreen.musicOpen
        Layout.preferredWidth: visible ? mapScreen.panelWidth : 0
        Layout.maximumWidth: Layout.preferredWidth
        Layout.fillHeight: true
      }
    }
  }

  // ── Legend + zoom ───────────────────────────────────────────────────────────
  footer: Rectangle {
    implicitHeight: 42
    color: "transparent"

    RowLayout {
      anchors.fill: parent
      anchors.leftMargin: 12
      anchors.rightMargin: 12
      spacing: 16

      Repeater {
        model: [
          { name: qsTr("Screen (20x18 tiles)"), color: brg.settings.errorColor },
          { name: qsTr("Draw area (6x5 blocks)"), color: brg.settings.accentColor },
          { name: qsTr("Map bounds"), color: brg.settings.primaryColor }
        ]

        RowLayout {
          required property var modelData
          spacing: 6

          Rectangle {
            implicitWidth: 14
            implicitHeight: 14
            radius: 3
            color: "transparent"
            border.width: 2
            border.color: parent.modelData.color
          }

          Text {
            text: parent.modelData.name
            font.pixelSize: 11
            color: brg.settings.textColorDark
          }
        }
      }

      Item { Layout.fillWidth: true }

      Text {
        text: qsTr("Zoom %1x").arg(mapScreen.zoom)
        font.pixelSize: 11
        color: brg.settings.textColorMid
      }

      FooterButton {
        text: "−"
        enabled: mapScreen.zoom > mapScreen.minZoom
        onClicked: mapScreen.userZoom = mapScreen.zoom - 1
      }

      FooterButton {
        text: "+"
        enabled: mapScreen.zoom < mapScreen.maxZoom
        onClicked: mapScreen.userZoom = mapScreen.zoom + 1
      }

      FooterButton {
        text: qsTr("Fit")
        enabled: mapScreen.userZoom > 0
        onClicked: mapScreen.userZoom = 0
      }
    }
  }
}
