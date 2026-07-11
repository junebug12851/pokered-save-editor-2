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
// CAPS LOCK locks the Shift PAGE (Shift inverts it; Ctrl/Alt ignore it), so the deck
// is always showing exactly one page and the strip can always name it. The rules live
// in C++ (FontKeyboard::effectivePage), pinned by tst_font_keyboard.
//
// The look: a DARK chassis with light caps, like a real keyboard. It isn't decoration
// -- light-grey caps on a light-grey chassis on a light-grey pane had no figure/ground
// at all, and the category colours washed out to nothing (you could only see them by
// hovering). Dark body, coloured caps: the colours now read at a glance, which is the
// entire point of colouring them.
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

  // The page on show. Caps Lock LOCKS THE SHIFT PAGE (Shift inverts it, Ctrl/Alt
  // ignore it), so this is always exactly one of the 8 pages -- every key on the deck
  // reads it, and the page strip can always say where you are. That is precisely why
  // caps isn't the real-keyboard letters-only rule: that produced a hybrid layer the
  // strip could not name.
  readonly property int page: brg.keyboard.effectivePage(shiftOn, ctrlOn, altOn, capsOn)

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

  // A key does NOT arrive as "the key you pressed" -- the OS hands us the key code of
  // the CHARACTER it produced. Shift+1 is Key_Exclam, not Key_1; Shift+/ is Key_Question;
  // Shift+; is Key_Colon. So both the plain and the shifted code of every non-letter key
  // have to map back to the same physical key, or half the deck stops responding to the
  // keyboard. (US layout; other layouts fall through to the caps and the page strip,
  // which reach every tile regardless.)
  readonly property var physicalKeys: ({
    // number row, plain then shifted
    "49":"1", "33":"1",   // 1  !
    "50":"2", "64":"2",   // 2  @
    "51":"3", "35":"3",   // 3  #
    "52":"4", "36":"4",   // 4  $
    "53":"5", "37":"5",   // 5  %
    "54":"6", "94":"6",   // 6  ^
    "55":"7", "38":"7",   // 7  &
    "56":"8", "42":"8",   // 8  *
    "57":"9", "40":"9",   // 9  (
    "48":"0", "41":"0",   // 0  )

    // punctuation, plain then shifted
    "96":"`",  "126":"`",   // `  ~
    "45":"-",  "95":"-",    // -  _
    "61":"=",  "43":"=",    // =  +
    "91":"[",  "123":"[",   // [  {
    "93":"]",  "125":"]",   // ]  }
    "92":"\\", "124":"\\",  // \  |
    "59":";",  "58":";",    // ;  :
    "39":"'",  "34":"'",    // '  "
    "44":",",  "60":",",    // ,  <
    "46":".",  "62":".",    // .  >
    "47":"/",  "63":"/"     // /  ?
  })

  /// Physical key -> this deck's key label ("A", "7", "."), or "" if it isn't ours.
  function keyLabelFor(k) {
    if(k >= Qt.Key_A && k <= Qt.Key_Z) return String.fromCharCode(k);

    var mapped = deck.physicalKeys[k.toString()];
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
  // Every row is a standard ANSI 15u wide (see the rows below), plus up to 13 gaps at
  // 0.06u = 15.78u, plus the chassis' 0.4u padding = 16.2u.
  // Tallest: 5 rows + 4 gaps + the same padding = 5.65u. Round both up a hair.
  readonly property real u: Math.max(16, Math.min(52,
                              Math.min(width  / 16.3,
                                       height / 5.9)))
  readonly property real gap: Math.max(2, Math.round(u * 0.06))
  readonly property real tileScale: Math.max(2, (u - 20) / 8)

  // Clicking the bare chassis takes focus back from the name field.
  MouseArea {
    anchors.fill: parent
    onClicked: deck.forceActiveFocus();
  }

  Rectangle {
    id: chassis
    anchors.centerIn: parent

    width: deckRows.width + deck.u * 0.4
    height: deckRows.height + deck.u * 0.4
    radius: 12

    // The keyboard's BODY: a dark slate derived from the app accent, so the light,
    // category-tinted caps sit on it the way real keycaps sit in a real keyboard.
    color: Qt.darker(brg.settings.accentColor, 1.55)
    border.width: 1
    border.color: Qt.darker(brg.settings.accentColor, 1.9)

    Column {
      id: deckRows
      anchors.centerIn: parent
      spacing: deck.gap

      // ================= THE ROWS ==================================================
      // A standard ANSI silhouette, 15u per row.
      //
      // The PUNCTUATION keys (` - = [ ] \ ; ' , . /) are real, typing keys: the tiles
      // that belong on them are on them ("." types ".", Shift+"/" types "?"). They were
      // dead filler in the first cut, which meant "." and "," were exiled onto a number
      // row they have no business being on.
      //
      // Only Tab and the Win/Menu keys stay DEAD -- drawn, muted, inert, unclickable.
      // They're pure silhouette, and they earn it: without them the deck was a floating
      // block of caps, roomier and somehow worse, because the shape you recognise as "a
      // keyboard" comes from the ragged edges as much as from the letters.
      // =============================================================================

      // ---- Number row: ` 1..0 - = ⌫ ----
      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: deck.gap

        Repeater {
          model: ["`","1","2","3","4","5","6","7","8","9","0","-","="]

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

      // ---- QWERTY row: Tab Q..P [ ] \ ----
      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: deck.gap

        StructKey { label: "Tab"; unit: deck.u; units: 1.5; dead: true }

        Repeater {
          model: ["Q","W","E","R","T","Y","U","I","O","P","[","]","\\"]

          KeyCap {
            required property string modelData

            // The backslash key is the one 1.5u cap in the alphanumeric block.
            width: deck.u * (modelData === "\\" ? 1.5 : 1)
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

      // ---- Caps + home row + ; ' + Enter ----
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
          model: ["A","S","D","F","G","H","J","K","L",";","'"]

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
          units: 2.25
          onFired: deck.accept();
        }
      }

      // ---- Bottom row: Shift Z..M , . / Shift ----
      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: deck.gap

        StructKey {
          label: "Shift"
          unit: deck.u
          units: 2.25
          active: deck.shiftOn
          onFired: deck.latchShift = !deck.latchShift;
        }

        Repeater {
          model: ["Z","X","C","V","B","N","M",",",".","/"]

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
          units: 2.75
          active: deck.shiftOn
          onFired: deck.latchShift = !deck.latchShift;
        }
      }

      // ---- Modifier row + the spacebar ----
      // The spacebar carries the Space TILE. It is the one tile not on a letter or a
      // number, because this is where every human already expects it -- and it says
      // "Space" across it, because the Space tile renders as (correctly) nothing, and a
      // blank cap read as a dead key.
      Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: deck.gap

        StructKey {
          label: "Ctrl"
          unit: deck.u
          units: 1.25
          active: deck.ctrlOn
          onFired: deck.latchCtrl = !deck.latchCtrl;
        }

        StructKey { label: ""; unit: deck.u; units: 1.25; dead: true }   // Win

        StructKey {
          label: "Alt"
          unit: deck.u
          units: 1.25
          active: deck.altOn
          onFired: deck.latchAlt = !deck.latchAlt;
        }

        KeyCap {
          id: spaceCap

          width: deck.u * 6.25
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
          units: 1.25
          active: deck.altOn
          onFired: deck.latchAlt = !deck.latchAlt;
        }

        StructKey { label: ""; unit: deck.u; units: 1.25; dead: true }   // Win
        StructKey { label: ""; unit: deck.u; units: 1.25; dead: true }   // Menu

        StructKey {
          label: "Ctrl"
          unit: deck.u
          units: 1.25
          active: deck.ctrlOn
          onFired: deck.latchCtrl = !deck.latchCtrl;
        }
      }
    }
  }
}
