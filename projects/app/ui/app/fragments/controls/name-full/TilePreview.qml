import QtQuick

// TilePreview.qml --
// A tight, animated in-game render of a SINGLE font tile / character. Used by
// the character list's hover tooltip so you can see what a code actually looks
// like in-game (e.g. a Variable like <RIVAL> expands to the rival's name in
// bold tiles, just like it would on the real screen).
//
// It reuses the existing "image://font" provider — same source format as
// NameDisplay — but with chop sized tightly to the (expanded) content so there's
// no blank padding, and no box / editor / feedback baggage.
Image {
  id: img

  // The font code to render (FontDBEntry.name).
  property string tileName: ""

  // Pixel scale of an 8px tile.
  property real sizeMult: 6

  // Follow the app-wide simulated tileset / outdoor settings.
  property string tileset: brg.settings.previewTileset
  property bool isOutdoor: brg.settings.previewOutdoor
  property string isOutdoorStr: isOutdoor ? "outdoor" : "indoor"

  // Static single frame. Rendering a tile runs an expensive expand/convert pass;
  // animating it (re-rendering every frame) froze the UI when hovering an
  // expensive Variable like the rival/player name. One frame is plenty for a
  // hover preview.
  property int curFrame: 0

  // Width in tiles, sized to the expanded output so nothing gets chopped.
  // Clamp to the provider's 20-tile draw width.
  property int chop: {
    var n = brg.fonts.countSizeOfExpanded(tileName);
    if(n < 1) n = 1;
    if(n > 20) n = 20;
    return n;
  }

  width: Math.trunc(8 * chop * sizeMult)
  height: Math.trunc(8 * sizeMult)

  cache: false

  source: "image://font/" +
          tileset + "/" +
          isOutdoorStr + "/" +
          curFrame + "/" +
          width + "/" +
          height + "/" +
          "no-box/" +
          "1-line/" +
          chop + "/" +
          "transparent/" +
          "none/" +
          brg.util.encodeBeforeUrl("%%") + "/" +
          brg.util.encodeBeforeUrl(tileName)
}
