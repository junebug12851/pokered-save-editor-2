// KeyboardDeck.qml -- the full keyboard's deck: the chassis, the keys, the pages.
//
// A real ASDF/QWERTY silhouette. Its 36 alphanumeric caps each carry one game tile
// (the map lives in C++, `brg.keyboard` -- see FontKeyboard); the structural caps
// (Shift/Ctrl/Alt/Space/Backspace/Enter) do what they say.
//
// 255 tiles / 36 keys = 8 pages, and Shift/Ctrl/Alt give exactly 8 combinations --
// one per page, no leftovers. You reach a page three ways, and all three agree:
//   * HOLD the modifiers (like a real keyboard's shift layer),
//   * CLICK the on-screen modifier caps -- they LATCH, so mouse/touch users never
//     have to hold a chord, and a page stays reachable even when the OS eats the
//     combo (Shift+Alt / Ctrl+Shift are Windows' switch-layout shortcuts on
//     multi-language setups, and Ctrl+Alt is AltGr),
//   * CLICK a button in the page strip above.
// setPage() drives the latches, so the strip and the caps can never disagree.
//
// The deck holds keyboard focus whenever the name field doesn't. When the field IS
// focused (the user clicked in to paste or select), the deck stops listening and
// DIMS every key legend -- the mode is visible, never hidden.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
  id: deck

  // The page on show = whatever the modifiers say, held or latched.
  readonly property bool shiftOn: heldShift || latchShift
  readonly property bool ctrlOn:  heldCtrl  || latchCtrl
  readonly property bool altOn:   heldAlt   || latchAlt
  readonly property int page: brg.keyboard.pageFor(shiftOn, ctrlOn, altOn)

  property bool heldShift: false
  property bool heldCtrl: false
  property bool heldAlt: false

  property bool latchShift: false
  property bool latchCtrl: false
  property bool latchAlt: false

  signal insert(string code) ///< A tile was picked (its code, e.g. "A" or "<player>").
  signal backspace()
  signal accept()
  signal dismiss()
  signal detail(var info)    ///< Hover feedback for the side pane; null when nothing is hovered.

  // Registry of live caps, so a physical key press can find and flash its on-screen
  // cap (typing and clicking are then literally the same act).
  property var caps: ({})

  function setPage(p) {
    latchShift = (p & 1) !== 0;
    latchCtrl  = (p & 2) !== 0;
    latchAlt   = (p & 4) !== 0;
  }

  // Shift+digit does NOT arrive as Key_1..Key_0 -- the OS hands us the SYMBOL's key
  // code (Shift+1 == Key_Exclam). Map the US-layout number row back to its digit key
  // so the Shift page's number row still types. Other layouts simply fall through
  // (the caps and the page strip still reach every tile).
  readonly property var shiftedDigits: ({
    "33": "1",  // !
    "64": "2",  // @
    "35": "3",  // #
    "36": "4",  // $
    "37": "5",  // %
    "94": "6",  // ^
    "38": "7",  // &
    "42": "8",  // *
    "40": "9",  // (
    "41": "0"   // )
  })

  /// Physical key -> this deck's key label ("A", "7"), or "" if it isn't one of ours.
  function keyLabelFor(k) {
    if(k >= Qt.Key_A && k <= Qt.Key_Z) return String.fromCharCode(k);
    if(k >= Qt.Key_0 && k <= Qt.Key_9) return String.fromCharCode(k);

    var mapped = deck.shiftedDigits[k.toString()];
    return (mapped !== undefined) ? mapped : "";
  }

  function pressKey(label) {
    var cap = deck.caps[label];
    if(!cap || cap.isEmpty)
      return false;

    cap.press();
    return true;
  }

  // A modifier held while the window loses focus would otherwise stay "held" forever
  // and strand the deck on the wrong page.
  onActiveFocusChanged: {
    if(activeFocus)
      return;

    heldShift = false;
    heldCtrl = false;
    heldAlt = false;
  }

  focus: true

  Keys.onPressed: (event) => {
    if(event.key === Qt.Key_Shift)   { deck.heldShift = true; event.accepted = true; return; }
    if(event.key === Qt.Key_Control) { deck.heldCtrl  = true; event.accepted = true; return; }
    if(event.key === Qt.Key_Alt ||
       event.key === Qt.Key_AltGr)   { deck.heldAlt   = true; event.accepted = true; return; }

    // Re-sync from the event: a modifier pressed while we DIDN'T have focus never
    // gave us its press, but it's still down and the event knows it.
    deck.heldShift = (event.modifiers & Qt.ShiftModifier)   !== 0;
    deck.heldCtrl  = (event.modifiers & Qt.ControlModifier) !== 0;
    deck.heldAlt   = (event.modifiers & Qt.AltModifier)     !== 0;

    if(event.key === Qt.Key_Backspace) {
      backspaceKey.press();
      event.accepted = true;
      return;
    }

    if(event.key === Qt.Key_Space) {
      spaceCap.press();
      event.accepted = true;
      return;
    }

    if(event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
      enterKey.press();
      event.accepted = true;
      return;
    }

    if(event.key === Qt.Key_Escape) {
      deck.dismiss();
      event.accepted = true;
      return;
    }

    var label = deck.keyLabelFor(event.key);
    if(label !== "")
      event.accepted = deck.pressKey(label);
  }

  Keys.onReleased: (event) => {
    if(event.key === Qt.Key_Shift)   { deck.heldShift = false; event.accepted = true; }
    if(event.key === Qt.Key_Control) { deck.heldCtrl  = false; event.accepted = true; }
    if(event.key === Qt.Key_Alt ||
       event.key === Qt.Key_AltGr)   { deck.heldAlt   = false; event.accepted = true; }
  }

  // ---- The ONE animation clock for the whole deck ----
  // Every tile shares this frame. That's what lets all 36 keys hit a single cached
  // tile sheet per frame instead of each rebuilding the tileset. See TileGlyph.
  property int curFrame: 0

  Timer {
    interval: 1000 / 3
    running: deck.visible
    repeat: true
    onTriggered: deck.curFrame = ((deck.curFrame + 1) >= 8) ? 0 : deck.curFrame + 1;
  }

  // ---- Sizing ----
  // The deck is ~12.5 units wide and 5 rows tall; the unit shrinks to fit whatever
  // room the screen gives it, so the deck scales instead of spilling over its
  // neighbours. Get these constants wrong and the chassis quietly overflows the column
  // and paints on top of the legend and the detail pane -- which is exactly what the
  // first cut did.
  //
  // Widest row (the number row) = 10 caps + a 2u Backspace + 11 gaps, gap = 0.07u:
  //   12u + 11*0.07u = 12.77u, plus the chassis' 0.55u padding = 13.32u.
  // Tallest: 5 rows + 4 gaps + the same padding = 5.83u. Round both up a hair.
  readonly property real u: Math.max(18, Math.min(56,
                              Math.min(width  / 13.5,
                                       height / 6.0)))
  readonly property real gap: Math.max(2, Math.round(u * 0.07))
  readonly property real tileScale: Math.max(2, (u - 20) / 8)

  // Clicking the bare chassis takes focus back from the name field.
  MouseArea {
    anchors.fill: parent
    onClicked: deck.forceActiveFocus();
  }

  Rectangle {
    id: chassis
    anchors.centerIn: parent

    width: deckRows.width + deck.u * 0.55
    height: deckRows.height + deck.u * 0.55
    radius: 12

    color: Qt.lighter(brg.settings.dividerColor, 1.44)
    border.width: 1
    border.color: Qt.rgba(0, 0, 0, 0.10)

    Column {
      id: deckRows
      anchors.centerIn: parent
      spacing: deck.gap

      // ---- Number row + Backspace ----
      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: deck.gap

        Repeater {
          model: ["1","2","3","4","5","6","7","8","9","0"]

          KeyCap {
            required property string modelData

            width: deck.u
            height: deck.u
            tileScale: deck.tileScale
            curFrame: deck.curFrame
            legendsDim: !deck.activeFocus
            info: brg.keyboard.keyData(deck.page, modelData)

            onActivated: (code) => deck.insert(code);
            onEntered: deck.detail(info);
            onExited: deck.detail(null);

            Component.onCompleted: deck.caps[modelData] = this;
          }
        }

        StructKey {
          id: backspaceKey
          label: "⌫"
          unit: deck.u
          units: 2
          onFired: deck.backspace();
        }
      }

      // ---- QWERTY row ----
      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: deck.gap

        Repeater {
          model: ["Q","W","E","R","T","Y","U","I","O","P"]

          KeyCap {
            required property string modelData

            width: deck.u
            height: deck.u
            tileScale: deck.tileScale
            curFrame: deck.curFrame
            legendsDim: !deck.activeFocus
            info: brg.keyboard.keyData(deck.page, modelData)

            onActivated: (code) => deck.insert(code);
            onEntered: deck.detail(info);
            onExited: deck.detail(null);

            Component.onCompleted: deck.caps[modelData] = this;
          }
        }
      }

      // ---- Home row + Enter ----
      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: deck.gap

        Repeater {
          model: ["A","S","D","F","G","H","J","K","L"]

          KeyCap {
            required property string modelData

            width: deck.u
            height: deck.u
            tileScale: deck.tileScale
            curFrame: deck.curFrame
            legendsDim: !deck.activeFocus
            info: brg.keyboard.keyData(deck.page, modelData)

            onActivated: (code) => deck.insert(code);
            onEntered: deck.detail(info);
            onExited: deck.detail(null);

            Component.onCompleted: deck.caps[modelData] = this;
          }
        }

        StructKey {
          id: enterKey
          label: "Enter"
          unit: deck.u
          units: 2
          onFired: deck.accept();
        }
      }

      // ---- Bottom row, wrapped in the two Shift caps ----
      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: deck.gap

        StructKey {
          label: "Shift"
          unit: deck.u
          units: 1.9
          active: deck.shiftOn
          onFired: deck.latchShift = !deck.latchShift;
        }

        Repeater {
          model: ["Z","X","C","V","B","N","M"]

          KeyCap {
            required property string modelData

            width: deck.u
            height: deck.u
            tileScale: deck.tileScale
            curFrame: deck.curFrame
            legendsDim: !deck.activeFocus
            info: brg.keyboard.keyData(deck.page, modelData)

            onActivated: (code) => deck.insert(code);
            onEntered: deck.detail(info);
            onExited: deck.detail(null);

            Component.onCompleted: deck.caps[modelData] = this;
          }
        }

        StructKey {
          label: "Shift"
          unit: deck.u
          units: 1.9
          active: deck.shiftOn
          onFired: deck.latchShift = !deck.latchShift;
        }
      }

      // ---- Modifiers + the spacebar ----
      // The spacebar carries the Space TILE. It is the one tile not on a letter or a
      // number, because this is where every human already expects it.
      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: deck.gap

        StructKey {
          label: "Ctrl"
          unit: deck.u
          units: 1.7
          active: deck.ctrlOn
          onFired: deck.latchCtrl = !deck.latchCtrl;
        }

        StructKey {
          label: "Alt"
          unit: deck.u
          units: 1.7
          active: deck.altOn
          onFired: deck.latchAlt = !deck.latchAlt;
        }

        KeyCap {
          id: spaceCap

          width: deck.u * 4.9
          height: deck.u
          tileScale: deck.tileScale
          curFrame: deck.curFrame
          legendsDim: !deck.activeFocus
          info: brg.keyboard.spaceData()

          onActivated: (code) => deck.insert(code);
          onEntered: deck.detail(info);
          onExited: deck.detail(null);
        }

        StructKey {
          label: "Alt"
          unit: deck.u
          units: 1.7
          active: deck.altOn
          onFired: deck.latchAlt = !deck.latchAlt;
        }

        StructKey {
          label: "Ctrl"
          unit: deck.u
          units: 1.7
          active: deck.ctrlOn
          onFired: deck.latchCtrl = !deck.latchCtrl;
        }
      }
    }
  }
}
