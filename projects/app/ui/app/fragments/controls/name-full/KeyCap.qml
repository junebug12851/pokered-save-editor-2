// KeyCap.qml -- one key on the full keyboard's deck.
//
// A cap carries ONE game tile plus the physical key it's bound to, printed as a
// superscript legend in the corner (so you can either click the cap or just type
// that key).
//
// THE CATEGORY COLOUR MUST READ AT A GLANCE. The first cut tinted the face 6% and
// bordered it at 35%, which on a light chassis meant you could only really see a key's
// category by hovering it -- i.e. the colour did nothing, which defeats the point of
// having a colour legend at all. The cap face is now a proper wash of its category
// colour with a solid border, sitting on a DARK chassis: the deck reads as coloured
// regions the moment you look at it.
//
// It draws a cap two ways, because the tile sheet cannot render everything (see
// FontKeyboard::Render):
//   Tile  -- a real 8x8 glyph, clipped from the deck's shared animated sheet. This is
//            the normal case, and it ANIMATES (water, flowers) with the tileset.
//   Label -- everything that ISN'T one tile: control codes (no glyph at all) and
//            multi-char/variable codes (`<player>`, `<trainer>` -- 7+ characters of
//            expanded text). Rendering those expanded on a ~30px cap was tried and is
//            unusable: they draw wider than the key and smear across their neighbours.
//            The cap shows the bare code instead; the DETAIL PANE renders the real
//            expanded glyphs at a size where you can actually read them.
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
  id: top

  // { key, ind, code, title, tip, category, render, empty } from brg.keyboard.
  property var info: null

  // The shared animation frame (one timer on the deck drives every cap).
  property int curFrame: 0

  property real tileScale: 3

  readonly property bool isEmpty: !info || info.empty === true
  readonly property color cat: top.catColor(info ? info.category : 0)

  // The Space tile renders as... nothing, because it's a space. Drawn as a glyph the
  // spacebar looked like a dead, disabled key. It gets its name written on it instead
  // -- it is ALWAYS Space, on every page, and should look it.
  readonly property bool isSpace: !isEmpty && top.info.code === " "

  // A single tile => draw the glyph. Anything else => draw its name.
  readonly property bool isGlyph: !isEmpty && info.render === 1 && !isSpace

  // The corner legend exists to teach you "this tile lives on THAT key". When the deck
  // types, on this key with these modifiers, exactly what a REAL keyboard would --
  // "a" on A, "!" on Shift+1, "?" on Shift+/ -- there is nothing to teach: the key is
  // already telling you, by being that key. So the legend is dropped.
  //
  // C++ decides this (`natural`), because it's the same rule that governs the map: a
  // tile goes where a real keyboard would put it whenever it can. It stays on every
  // Ctrl/Alt key, where a real keyboard types nothing and the legend is the only clue.
  readonly property bool legendRedundant: !isEmpty && top.info.natural === true

  // "<player>" -> "player". The brackets cost 2 of the ~6 characters that fit on a
  // cap, and every code has them, so they carry no information here.
  readonly property string capLabel: {
    if(top.isEmpty)  return "";
    if(top.isSpace)  return qsTr("Space");

    return top.info.code.replace(/^</, "").replace(/>$/, "");
  }

  signal activated(string code)
  signal entered()
  signal exited()

  // Category -> colour. Same six Settings colours (and the same precedence) the old
  // picker used, so anybody who learned them keeps them.
  function catColor(c) {
    switch(c) {
      case 1: return brg.settings.fontColorNormal;
      case 2: return brg.settings.fontColorSingle;
      case 3: return brg.settings.fontColorMulti;
      case 4: return brg.settings.fontColorVar;
      case 5: return brg.settings.fontColorPicture;
      case 6: return brg.settings.fontColorControl;
    }

    return brg.settings.dividerColor;
  }

  // The cap's press animation on its own. A PHYSICAL key press resolves its tile
  // against the model (the deck does that) and only flashes the cap, so typing and
  // clicking stay visibly the same act without two sources of truth for the code.
  function flash() {
    pressAnim.restart();
  }

  // A click on the cap itself.
  function press() {
    if(top.isEmpty) return;
    top.flash();
    top.activated(top.info.code);
  }

  // Switching page under a STATIONARY mouse swaps this cap's tile without any
  // enter/exit, so the detail pane would keep describing the tile that used to be
  // here. Re-announce whenever the tile under the cursor changes.
  onInfoChanged: {
    if(mouse.containsMouse)
      top.entered();
  }

  implicitWidth: 54
  implicitHeight: 54

  Rectangle {
    id: capFace
    anchors.fill: parent
    anchors.margins: 2
    radius: 6

    // A real wash of the category colour, not a hint of one -- this is what makes the
    // colour legible without hovering. An EMPTY key stays a dark, inert well in the
    // chassis: it reads as "no key here", not as a broken white one.
    color: {
      if(top.isEmpty)
        return Qt.darker(brg.settings.accentColor, 1.75);
      if(mouse.pressed)
        return Qt.lighter(top.cat, 1.35);
      if(mouse.containsMouse)
        return Qt.lighter(top.cat, 1.58);

      return Qt.lighter(top.cat, 1.82);
    }

    border.width: 1
    border.color: top.isEmpty
                  ? Qt.darker(brg.settings.accentColor, 1.9)
                  : (mouse.containsMouse
                     ? Qt.darker(top.cat, 1.25)
                     : Qt.lighter(top.cat, 1.25))

    Behavior on color { ColorAnimation { duration: 90 } }
    Behavior on border.color { ColorAnimation { duration: 90 } }

    // The cap physically depresses on a press -- the way a real key does.
    y: (mouse.pressed ? 3 : 1)
    Behavior on y { NumberAnimation { duration: 60 } }

    // The cap's shadow on the dark chassis, so it reads as a raised keycap rather than
    // a painted rectangle. Deeper while hovered.
    Rectangle {
      z: -1
      anchors.fill: parent
      anchors.topMargin: mouse.containsMouse && !mouse.pressed ? 3 : 2
      radius: parent.radius
      color: Qt.rgba(0, 0, 0, 0.22)
      visible: !top.isEmpty && !mouse.pressed
    }

    // ---- The glyph ----

    TileGlyph {
      anchors.centerIn: parent
      // Nudged off dead-centre so the corner legend isn't sitting on top of the glyph --
      // but only when there IS a legend. On a key that types itself, the glyph gets the
      // whole cap and sits dead centre, which is how a keycap should look.
      anchors.horizontalCenterOffset: top.legendRedundant ? 0 : -2
      anchors.verticalCenterOffset: top.legendRedundant ? 0 : 2

      visible: top.isGlyph
      ind: top.isGlyph ? top.info.ind : 0
      curFrame: top.curFrame
      sizeMult: top.tileScale
    }

    Text {
      anchors.fill: parent
      anchors.leftMargin: 2
      anchors.rightMargin: 2
      // The spacebar has no corner legend to dodge, so its name can sit dead centre.
      anchors.topMargin: top.isSpace ? 1 : 8
      anchors.bottomMargin: 1

      visible: !top.isEmpty && !top.isGlyph
      text: top.capLabel
      color: Qt.darker(top.cat, 1.35)

      font.pixelSize: top.isSpace
                      ? Math.max(9, Math.round(top.height * 0.28))
                      : Math.max(7, Math.round(top.height * 0.225))
      font.bold: true
      lineHeight: 0.92
      wrapMode: Text.WrapAnywhere
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
      maximumLineCount: 2
      elide: Text.ElideRight
    }

    // ---- The legend: which physical key this is ----
    // Not on the spacebar: its key IS the spacebar, and "Space" is already written
    // across the middle of it.
    //
    // This is the thing that makes "just type it" discoverable, and in the first cut you
    // could barely SEE it: 9px of mid-grey at 75% on a near-white cap. It now scales
    // with the key, sits at full opacity, and is drawn in a dark shade of the cap's own
    // category colour -- legible, and still clearly a legend rather than the glyph.

    Text {
      anchors.top: parent.top
      anchors.right: parent.right
      anchors.topMargin: 2
      anchors.rightMargin: 3

      visible: !top.isSpace && !top.legendRedundant
      text: top.info ? top.info.key : ""
      font.pixelSize: Math.max(9, Math.round(top.height * 0.30))
      font.bold: true
      color: top.isEmpty
             ? brg.settings.textColorLight
             : Qt.darker(top.cat, 1.85)
      opacity: top.isEmpty ? 0.35 : 1.0
    }

    MouseArea {
      id: mouse
      anchors.fill: parent
      hoverEnabled: true
      enabled: !top.isEmpty
      cursorShape: top.isEmpty ? Qt.ArrowCursor : Qt.PointingHandCursor

      onClicked: top.press();
      onEntered: top.entered();
      onExited: top.exited();
    }

    // The flash a physical key press gives the on-screen cap, so typing is visibly
    // connected to the deck.
    SequentialAnimation {
      id: pressAnim
      NumberAnimation { target: capFace; property: "scale"; to: 0.92; duration: 55 }
      NumberAnimation { target: capFace; property: "scale"; to: 1.0;  duration: 85 }
    }
  }
}
