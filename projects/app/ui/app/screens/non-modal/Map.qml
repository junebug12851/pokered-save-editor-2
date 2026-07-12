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

Page {
  id: mapScreen

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

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    // ── What is loaded ────────────────────────────────────────────────────────
    Rectangle {
      Layout.fillWidth: true
      implicitHeight: 34
      color: "transparent"

      RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 14

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
      }
    }

    // ── The map ───────────────────────────────────────────────────────────────
    Flickable {
      id: view

      Layout.fillWidth: true
      Layout.fillHeight: true

      clip: true
      contentWidth: Math.max(width, mapScreen.scaledWidth)
      contentHeight: Math.max(height, mapScreen.scaledHeight)
      boundsBehavior: Flickable.StopAtBounds

      ScrollBar.vertical: ScrollBar {}
      ScrollBar.horizontal: ScrollBar {}

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

      // Ctrl+wheel zooms; a plain wheel still scrolls, as it should.
      WheelHandler {
        acceptedModifiers: Qt.ControlModifier
        onWheel: (event) => {
          mapScreen.userZoom = Math.max(mapScreen.minZoom,
                                  Math.min(mapScreen.maxZoom, mapScreen.zoom + (event.angleDelta.y > 0 ? 1 : -1)))
        }
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
