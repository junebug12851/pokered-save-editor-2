// KeyboardDeck.qml -- the full keyboard's deck: the chassis, the keys, the pages.
//
// A real ASDF/QWERTY silhouette. Its 36 alphanumeric caps each carry one game tile
// (the map lives in C++, `brg.keyboard` -- see FontKeyboard); the structural caps
// (Caps/Shift/Ctrl/Alt/Space/Backspace/Enter) do what they say.
//
// 255 tiles / 36 keys = 8 pages, and Shift/Ctrl/Alt give exactly 8 combinations --
// one page per chord, no leftovers.
//
// TWO WAYS IN, AND THEY BEHAVE DIFFERENTLY ON PURPOSE (Twilight's call):
//   * The PHYSICAL modifier keys are MOMENTARY. Hold Ctrl -> the deck flips to that
//     page; let go -> it drops straight back. Nothing latches, exactly like the shift
//     layer on the keyboard under your hands.
//   * CLICKING an on-screen modifier cap (or a page button) LATCHES it, because a
//     mouse can't hold a chord and click a key at the same time -- and because a
//     latched page is the only way in when the OS eats the chord (Shift+Alt and
//     Ctrl+Shift switch keyboard layout on multi-language Windows; Ctrl+Alt is AltGr).
//
// CAPS LOCK is a real Caps Lock, not a latched Shift: letters only, so the number row
// keeps typing digits ("PIKA2" without unlocking); ignored under Ctrl/Alt (Ctrl+B is
// still bold B); inverted by Shift. The rules live in C++ (FontKeyboard::pageForKey)
// where they're pinned by tst_font_keyboard, so the deck just asks.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
  id: deck

  // Held (momentary, from the real keys) OR latched (sticky, from a click).
  readonly property bool shiftOn: heldShift || latchShift
  readonly property bool ctrlOn:  heldCtrl  || latchCtrl
  readonly property bool altOn:   heldAlt   || latchAlt

  // The CHORD's page. Note this is not necessarily what every cap shows -- with Caps
  // Lock on, the letters read a different page than the number row, exactly as they
  // do on a real keyboard. This is what the page strip highlights.
  readonly property int page: brg.keyboard.pageFor(shiftOn, ctrlOn, altOn)

  property bool heldShift: false
  property bool heldCtrl: false
  property bool heldAlt: false

  property bool latchShift: false
  property bool latchCtrl: false
  property bool latchAlt: false

  property bool capsOn: false

  signal insert(string code) ///< A tile was picked (its code, e.g. "a" or "<player>").
  signal backspace()
  signal accept()
  signal dismiss()
  signal detail(var info)    ///< Hover feedback for the side pane; null when nothing is hovered.

  // Registry of live caps, so a physical key press can flash its on-screen cap --
  // typing and clicking are then visibly the same act.
  property var capIndex: ({})

  function setPage(p) {
    latchShift = (p & 1) !== 0;
    latchCtrl  = (p & 2) !== 0;
    latchAlt   = (p & 4) !== 0;
  }

  // Shift+digit does NOT arrive as Key_1..Key_0 -- the OS hands us the SYMBOL's key
  // code (Shift+1 == Key_Exclam). Map the US-layout number row back to its digit key
  // so the Shift layer's number row still types. Other layouts fall through (the caps
  // and the page strip still reach every tile).
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

  /// Resolve a key press against C++ (which owns the Caps Lock rules), insert its
  /// tile, and flash the on-screen cap. Resolving from the model rather than reading
  /// the cap's binding means a caps re-sync in the same event can't race the binding.
  function pressKey(label) {
    var d = brg.keyboard.keyDataFor(label, shiftOn, ctrlOn, altOn, capsOn);
    if(!d || d.empty)
      return false;

    var cap = deck.capIndex[label];
    if(cap)
      cap.flash();

    deck.insert(d.code);
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

    if(event.key === Qt.Key_CapsLock) {
      if(!event.isAutoRepeat)
        deck.capsOn = !deck.capsOn;
      event.accepted = true;
      return;
    }

    // Re-sync the held modifiers from the event: one pressed while we DIDN'T have
    // focus never gave us its press, but it's still down and the event knows it.
    deck.heldShift = (event.modifiers & Qt.ShiftModifier)   !== 0;
    deck.heldCtrl  = (event.modifiers & Qt.ControlModifier) !== 0;
    deck.heldAlt   = (event.modifiers & Qt.AltModifier)     !== 0;

    if(event.key === Qt.Key_Backspace) {
      backspaceKey.flash();
      deck.backspace();
      event.accepted = true;
      return;
    }

    if(event.key === Qt.Key_Space) {
      spaceCap.press();
      event.accepted = true;
      return;
    }

    if(event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
      enterKey.flash();
      deck.accept();
      event.accepted = true;
      return;
    }

    if(event.key === Qt.Key_Escape) {
      deck.dismiss();
      event.accepted = true;
      return;
    }

    var label = deck.keyLabelFor(event.key);
    if(label === "")
      return;

    // Trust the OS about the caps state. Qt gives us no portable way to READ the caps
    // light, so if it was already on before this screen opened we'd be out of step --
    // but `event.text` is the OS's own answer (it already folded in caps AND shift),
    // so a letter press tells us the truth and we correct ourselves from it.
    if(!deck.ctrlOn && !deck.altOn && event.text.length === 1) {
      var t = event.text;
      var isUpper = (t >= "A" && t <= "Z");
      var isLower = (t >= "a" && t <= "z");
      if(isUpper || isLower)
        deck.capsOn = (isUpper !== deck.heldShift);
    }

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
    running: deck.visible && deck.enabled
    repeat: true
    onTriggered: deck.curFrame = ((deck.curFrame + 1) >= 8) ? 0 : deck.curFrame + 1;
  }

  // ---- Sizing ----
  // The key unit shrinks to fit whatever room the screen gives us, so the deck SCALES
  // instead of spilling over its neighbours. Get these constants wrong and the chassis
  // quietly overflows the column and paints on top of the legend and the detail pane.
  //
  // Widest row is now the HOME row: Caps(1.75u) + 9 caps + Enter(2u) = 12.75u, plus 11
  // gaps at 0.07u = 13.52u, plus the chassis' 0.55u padding = 14.07u.
  // Tallest: 5 rows + 4 gaps + the same padding = 5.83u. Round both up a hair.
  readonly property real u: Math.max(18, Math.min(56,
                              Math.min(width  / 14.1,
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
            info: brg.keyboard.keyDataFor(modelData, deck.shiftOn, deck.ctrlOn,
                                          deck.altOn, deck.capsOn)

            onActivated: (code) => deck.insert(code);
            onEntered: deck.detail(info);
            onExited: deck.detail(null);

            Component.onCompleted: deck.capIndex[modelData] = this;
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
            info: brg.keyboard.keyDataFor(modelData, deck.shiftOn, deck.ctrlOn,
                                          deck.altOn, deck.capsOn)

            onActivated: (code) => deck.insert(code);
            onEntered: deck.detail(info);
            onExited: deck.detail(null);

            Component.onCompleted: deck.capIndex[modelData] = this;
          }
        }
      }

      // ---- Caps + home row + Enter ----
      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: deck.gap

        StructKey {
          label: "Caps"
          unit: deck.u
          units: 1.75
          active: deck.capsOn
          onFired: deck.capsOn = !deck.capsOn;
        }

        Repeater {
          model: ["A","S","D","F","G","H","J","K","L"]

          KeyCap {
            required property string modelData

            width: deck.u
            height: deck.u
            tileScale: deck.tileScale
            curFrame: deck.curFrame
            info: brg.keyboard.keyDataFor(modelData, deck.shiftOn, deck.ctrlOn,
                                          deck.altOn, deck.capsOn)

            onActivated: (code) => deck.insert(code);
            onEntered: deck.detail(info);
            onExited: deck.detail(null);

            Component.onCompleted: deck.capIndex[modelData] = this;
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
            info: brg.keyboard.keyDataFor(modelData, deck.shiftOn, deck.ctrlOn,
                                          deck.altOn, deck.capsOn)

            onActivated: (code) => deck.insert(code);
            onEntered: deck.detail(info);
            onExited: deck.detail(null);

            Component.onCompleted: deck.capIndex[modelData] = this;
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
