// KeyCap.qml -- one key on the full keyboard's deck.
//
// A cap carries ONE game tile plus the physical key it's bound to, printed as a
// superscript legend in the corner (so you can either click the cap or just type
// that key). It is tinted by the tile's category colour -- a hairline border plus a
// faint wash of the same colour in the face -- so the deck reads as coloured regions
// without turning into a paint chart.
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

  // Set while the name field has focus: the deck isn't listening to the keyboard,
  // so the key legends are dimmed to SHOW that -- the mode is never hidden.
  property bool legendsDim: false

  property real tileScale: 3

  readonly property bool isEmpty: !info || info.empty === true
  readonly property color cat: top.catColor(info ? info.category : 0)

  // A single tile => draw the glyph. Anything else => draw the bare code.
  readonly property bool isGlyph: !isEmpty && info.render === 1

  // "<player>" -> "player". The brackets cost 2 of the ~6 characters that fit on a
  // cap, and every code has them, so they carry no information here.
  readonly property string capLabel: isEmpty
                                     ? ""
                                     : top.info.code.replace(/^</, "").replace(/>$/, "")

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

  // Fire the key: click, or the physical key being pressed (the deck calls this).
  function press() {
    if(top.isEmpty) return;
    pressAnim.restart();
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

    // An empty key still gets a cap -- a hole in the deck would read as broken.
    // It's just flat, borderless and inert.
    color: {
      if(top.isEmpty)
        return Qt.rgba(0, 0, 0, 0.03);
      if(mouse.pressed)
        return Qt.tint("#ffffff", Qt.rgba(top.cat.r, top.cat.g, top.cat.b, 0.26));
      if(mouse.containsMouse)
        return Qt.tint("#ffffff", Qt.rgba(top.cat.r, top.cat.g, top.cat.b, 0.14));

      return Qt.tint("#ffffff", Qt.rgba(top.cat.r, top.cat.g, top.cat.b, 0.06));
    }

    border.width: 1
    border.color: top.isEmpty
                  ? Qt.rgba(0, 0, 0, 0.06)
                  : (mouse.containsMouse
                     ? top.cat
                     : Qt.rgba(top.cat.r, top.cat.g, top.cat.b, 0.35))

    Behavior on color { ColorAnimation { duration: 90 } }
    Behavior on border.color { ColorAnimation { duration: 90 } }

    // The cap physically depresses on a press -- 1px, the way a real key does.
    y: (mouse.pressed ? 3 : 2)
    Behavior on y { NumberAnimation { duration: 60 } }

    // A soft lift while hovered. Not a Material elevation (that rounds/insets the
    // whole control); just a shadow-ish underlay so the cap reads as raised.
    Rectangle {
      z: -1
      anchors.fill: parent
      anchors.topMargin: 2
      radius: parent.radius
      color: Qt.rgba(0, 0, 0, 0.10)
      visible: mouse.containsMouse && !mouse.pressed && !top.isEmpty
    }

    // ---- The glyph ----

    TileGlyph {
      anchors.centerIn: parent
      visible: top.isGlyph
      ind: top.isGlyph ? top.info.ind : 0
      curFrame: top.curFrame
      sizeMult: top.tileScale
    }

    Text {
      anchors.fill: parent
      anchors.leftMargin: 2
      anchors.rightMargin: 2
      anchors.topMargin: 8      // clear of the key legend in the corner
      anchors.bottomMargin: 1

      visible: !top.isEmpty && !top.isGlyph
      text: top.capLabel
      color: top.cat

      font.pixelSize: Math.max(7, Math.round(top.height * 0.225))
      font.bold: true
      lineHeight: 0.92
      wrapMode: Text.WrapAnywhere
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
      maximumLineCount: 2
      elide: Text.ElideRight
    }

    // ---- The legend: which physical key this is ----

    Text {
      anchors.top: parent.top
      anchors.right: parent.right
      anchors.topMargin: 3
      anchors.rightMargin: 4

      text: top.info ? top.info.key : ""
      font.pixelSize: 9
      font.bold: true
      color: brg.settings.textColorMid
      opacity: top.legendsDim ? 0.25 : (top.isEmpty ? 0.35 : 0.75)

      Behavior on opacity { NumberAnimation { duration: 120 } }
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
