// TileGlyph.qml -- one animated 8x8 game tile, clipped out of the shared tile sheet.
//
// WHY IT'S BUILT THIS WAY (do not "simplify" it back to one image://font request
// per tile): every image://font request runs a FULL tileset build
// (TilesetEngine::buildTileset) -- it is expensive. The deck shows 36 tiles at once
// and animates them across an 8-frame cycle, so per-tile requests would mean 36
// tileset builds per frame. That is exactly the cost that already froze the old
// hover tooltip (see TilePreview's note).
//
// So instead we ask for the WHOLE 16x16 tile sheet once per frame (the same
// image://tileset provider TilesetDisplay uses) and every key clips its own 8x8 cell
// out of it. Same URL for all 36 keys => QML's pixmap cache resolves it to ONE
// provider call and hands the same pixmap to everybody.
//
// The sheet is laid out 16 tiles per row, indexed by font code: row = ind / 16,
// col = ind % 16.
import QtQuick

Item {
  id: top

  // Font code (FontDBEntry.ind), 1-255.
  property int ind: 0

  // Pixel scale of an 8px tile.
  property real sizeMult: 4

  // The animation frame, driven by ONE timer on the deck -- not one per key. Every
  // key shares the frame, so they all hit the same cached sheet.
  property int curFrame: 0

  property string tileset: brg.settings.previewTileset
  property string isOutdoorStr: brg.settings.previewTilesetTypeStr

  // Paint a white square under the tile. The tileset's tiles are TRANSPARENT wherever
  // the game would show the background through them, so on a coloured keycap they came
  // out patchy and inconsistent -- some looked like they had a white card behind them and
  // some didn't. White backing gives every tile the same "screen" to sit on.
  //
  // Only the PICTURE tiles want this: the font glyphs (letters, punctuation) are meant to
  // sit straight on the cap and look wrong boxed in.
  property bool backing: false

  readonly property int cell: Math.trunc(8 * sizeMult)
  readonly property int sheetSize: cell * 16

  width: cell
  height: cell
  clip: true

  Rectangle {
    anchors.fill: parent
    visible: top.backing
    color: "#ffffff"
  }

  Image {
    id: sheet

    // The full sheet, positioned so this key's cell lands in the clipped window.
    x: -(top.ind % 16) * top.cell
    y: -Math.trunc(top.ind / 16) * top.cell

    width: top.sheetSize
    height: top.sheetSize

    // Nearest-neighbour: these are 8px Game Boy tiles. Smoothing them is a blur.
    smooth: false

    // cache:true is deliberate and is the whole point -- it's what makes the 36 keys
    // share a single provider call per frame.
    cache: true

    source: "image://tileset/" +
            top.tileset + "/" +
            top.isOutdoorStr + "/" +
            "font/" +
            top.curFrame + "/" +
            "whole/" +
            top.sheetSize + "/" +
            top.sheetSize
  }
}
