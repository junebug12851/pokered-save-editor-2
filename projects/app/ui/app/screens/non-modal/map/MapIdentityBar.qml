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
  MapIdentityBar.qml -- the top bar: THE TOOLS, then WHAT IS LOADED, then THE PALETTE.

  Twilight's call (2026-07-13): the tools moved up here from a left-hand rail, and the whole left
  edge went back to the map. What used to be three read-only chips (map name, tileset, size) is now
  ONE control you can actually drive:

    [ ↖ ✥ ⌕ ] │ [ Pallet Town · Overworld ⌄ ] [ 100% ⌄ ] [ 10x9 ] [ ⚠ unfinished copy ]
      tools        the map picker              the palette   size    what's wrong, if anything

  * The MAP PICKER (MapPicker.qml) drops the three things the save keeps separately -- the map id,
    the tileset the graphics come from (with Indoor/Cave/Outdoor, which is not a place but which
    tiles MOVE), and the blockset the map is BUILT from. Two pointers, two controls, because the
    save has two pointers and a console obeys both.
  * The PALETTE (ContrastPicker.qml) reads as a percentage and drops a segmented slider. The six
    glitch values are behind a switch -- shown as their own, differently coloured segments, because
    they are not levels; they are a read that lands between two.

  Nothing here is a menu bar and nothing here is a toolbar with separators. It is chips.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: bar

  /// The active tool. The rail moved into this bar, so the bar owns it now.
  property string tool: "select"

  /// The canvas -- the zoom menu drives it, and zoom has exactly one home.
  required property var canvas

  /// The drop-downs, drivable by name for the DEBUG harness / the screenshot review.
  property alias mapPickerOpen: mapPicker.openState
  property alias contrastPickerOpen: contrastPicker.openState
  property alias contrastShowGlitch: contrastPicker.showGlitch
  property alias outsideOpen: outsidePicker.openState

  readonly property var tools: [
    { id: "select", glyph: "↖", key: "V", tip: qsTr("Select & Move — click to select, drag to move") },
    { id: "pan",    glyph: "✥", key: "H", tip: qsTr("Pan — drag the map (or hold Space with any tool)") },
    { id: "zoom",   glyph: "⌕", key: "Z", tip: qsTr("Zoom — click to zoom in, Alt-click to zoom out") }
  ]

  // ── THE MAKERS ─────────────────────────────────────────────────────────────────────────────
  //
  // Twilight, 2026-07-14: *"we would need the top toolbar to ironically contain actual tools and this
  // is one of them, a create random sprite here tool, and a create warp here tool. I guess they can
  // be next to the cursor and stuff."*
  //
  // ⚠️ EXACTLY THE TWO SHE NAMED, and no others. There is no sign tool, because signs have not been
  // briefed -- and "it's the same shape as a warp" is not a brief. This group is designed to GROW one
  // slot at a time, as each object type gets its own conversation.
  // (notes/plans/map-screen.md §12b.)
  readonly property var makers: [
    { id: "placeWarp", glyph: "⇄", key: "W",
      tip: qsTr("Place a door — click a tile. It starts as a way back outside, which is what a door usually is.") },
    { id: "placeSprite", glyph: "☻", key: "N",
      tip: qsTr("Place a character — click a tile. A random one, but only ever a picture THIS map has loaded, so it can never be one the console would draw as garbage.") }
  ]

  implicitHeight: 36
  color: "#f7f7f7"

  Rectangle {
    anchors.bottom: parent.bottom
    width: parent.width
    height: 1
    color: brg.settings.dividerColor
  }

  RowLayout {
    anchors.fill: parent
    anchors.leftMargin: 8
    anchors.rightMargin: 10
    spacing: 8

    // ── The tools ─────────────────────────────────────────────────────────────────────────────
    RowLayout {
      spacing: 2

      Repeater {
        model: bar.tools

        MapRailButton {
          required property var modelData

          objectName: "toolBtn_" + modelData.id   // the DEBUG harness picks tools through these
          size: 26
          glyph: modelData.glyph
          tip: modelData.tip
          shortcut: modelData.key
          active: bar.tool === modelData.id

          onClicked: bar.tool = modelData.id
        }
      }

      // The ▾ on the zoom tool: the slider, and somewhere to go. ZOOM LIVES IN EXACTLY ONE PLACE
      // and this is it -- the canvas has no zoom buttons of its own any more.
      ZoomMenu {
        id: zoomMenu
        canvas: bar.canvas
        Layout.alignment: Qt.AlignVCenter
      }
    }

    Rectangle {
      implicitWidth: 1
      implicitHeight: 18
      color: brg.settings.dividerColor
      Layout.leftMargin: 2
      Layout.rightMargin: 2
    }

    // ── The MAKERS: the tools that put something on the map ───────────────────────────────────
    //
    // Separated from the three above by a divider, because they are a different kind of thing: those
    // three change how you LOOK at the map, these two CHANGE it.
    RowLayout {
      spacing: 2

      Repeater {
        model: bar.makers

        MapRailButton {
          id: makerBtn
          required property var modelData

          objectName: "toolBtn_" + modelData.id
          size: 26
          glyph: modelData.glyph
          shortcut: modelData.key
          active: bar.tool === modelData.id

          // ⚠️ THE CAP IS STATED BEFORE YOU HIT IT, NOT AFTER. A dead tool looks dead -- and the
          // tooltip says why, so it is never a mystery. (map-screen.md §9: "the caps are stated
          // before you hit them, not after".)
          readonly property int roomLeft: modelData.id === "placeWarp" ? brg.map.warpRoomLeft()
                                                                       : brg.map.npcRoomLeft()
          readonly property int cap: modelData.id === "placeWarp" ? 32 : 15

          enabled: makerBtn.roomLeft > 0
          opacity: enabled ? 1.0 : 0.35

          tip: makerBtn.roomLeft > 0
               ? modelData.tip + "\n\n" + qsTr("%1 of %2 used.").arg(makerBtn.cap - makerBtn.roomLeft)
                                                                .arg(makerBtn.cap)
               : (modelData.id === "placeWarp"
                  ? qsTr("This map already has all 32 doors the game can hold.")
                  : qsTr("This map already has all 15 characters the game can hold."))

          onClicked: bar.tool = modelData.id
        }
      }
    }

    Rectangle {
      implicitWidth: 1
      implicitHeight: 18
      color: brg.settings.dividerColor
      Layout.leftMargin: 2
      Layout.rightMargin: 2
    }

    // ── What is loaded, and what it is drawn out of ───────────────────────────────────────────
    MapPicker { id: mapPicker }

    // ── ⭐ "Outside is…" — the map a `back outside` door returns you to (wLastMap) ─────────────
    //
    // It sits in the TOOLBAR, with what is loaded, and not in a panel — and that is a deliberate
    // call, not a convenience.
    //
    // Every building's exit warp is `$FF`, which does NOT name a map: it means "put me back on
    // whatever map I last stood on outdoors", and THIS byte is that map. So changing it re-labels
    // every "back outside" door on the canvas AT ONCE — and watching that happen is the entire point.
    // A control that changes what a dozen other things MEAN does not belong three clicks deep.
    //
    // ⚠️ Twilight reached for this as "From map". The byte actually called that
    // (`wWarpedFromWhichMap`) is DEAD — the game writes it on every warp and nothing anywhere reads
    // it. This is the one she meant. See notes/reference/warps.md §4.
    OutsideIsPicker { id: outsidePicker }

    // ── Contrast ─────────────────────────────────────────────────────────────────────────────
    ContrastPicker { id: contrastPicker }

    // ── Colour (a Game Boy Color / SGB / custom filter) ───────────────────────────────────────
    //
    // Next to Contrast, because they are the palette pair: contrast picks which shade, this picks
    // what colour that shade is. A VIEW setting -- it never touches the save. (Twilight, 2026-07-13.)
    ColourPicker { id: colourPicker }

    // ── Music ────────────────────────────────────────────────────────────────────────────────
    //
    // In the toolbar, in the same language as the map picker beside it -- a chip that drops a
    // picker (Twilight, 2026-07-13). The Music DOCK PANEL is gone: a whole panel for one combo box,
    // two checkboxes and a ▶ was a panel too many.
    MusicPicker { id: musicPicker }

    Rectangle {
      visible: brg.mapClock.animates
      implicitWidth: 1
      implicitHeight: 18
      color: brg.settings.dividerColor
      Layout.leftMargin: 2
    }

    // ── The animation: ONE button that is either ▶ or ⏸ ──────────────────────────────────────
    //
    // The console rewrites the water tile every 20 frames (21 with flowers) -- about three times a
    // second -- so the map MOVES. This is the switch for it, and it is a thing you DO, which is why
    // it is up here and the frame counter is down in the status bar with the other facts.
    MapRailButton {
      objectName: "animToggle"
      visible: brg.mapClock.animates
      size: 26
      glyph: brg.mapClock.playing ? "⏸" : "▶"
      tip: brg.mapClock.playing
           ? qsTr("Pause the animation")
           : qsTr("Animate the map — the water and the flowers, at the console's own pace")
      onClicked: brg.mapClock.playing = !brg.mapClock.playing
    }

    // ── The town comes to life ───────────────────────────────────────────────────────────────
    //
    // The SECOND ▶. The one above animates the water; this one makes the PEOPLE walk.
    //
    // ⚠️ It is DESTRUCTIVE and it says so, once, with a "don't show me this again" that starts
    // UNTICKED -- because a warning you have to opt back into is not a warning.
    // ⚠️ A LABELLED chip, not a second ▶.
    //
    // The first version was a bare glyph sitting next to the animation's ▶, and two play buttons side
    // by side is a puzzle, not a toolbar (caught on the screenshot review). The word costs 30px and
    // buys "I know exactly what this does" -- which is the trade this whole screen is supposed to
    // make. The app's language is chips, and this is one.
    Rectangle {
      objectName: "simToggle"

      implicitWidth: simRow.implicitWidth + 14
      implicitHeight: 26
      radius: 13

      enabled: brg.mapSim.canSimulate
      opacity: enabled ? 1.0 : 0.4

      // ⚠️ CONTRAST. At rest this was a 4%-black chip -- so pale it was barely a chip at all -- with
      // its text on `palette.text`, which the theme resolves LIGHT. Pale text on a near-white chip:
      // Twilight, twice, and the screenshot review walked straight past it both times.
      //
      // Both states are now spelled out, not inherited. At rest: a real outline, and text dark enough
      // to read. Running: solid orange, white on it. Nothing here depends on what the palette feels
      // like doing.
      color: brg.mapSim.playing ? "#d55e00"
           : simHover.hovered   ? Qt.rgba(0, 0, 0, 0.10)
           : Qt.rgba(0, 0, 0, 0.05)

      border.width: 1
      border.color: brg.mapSim.playing ? "#d55e00" : brg.settings.dividerColor

      Behavior on color { ColorAnimation { duration: 90 } }

      readonly property color ink: brg.mapSim.playing ? "#ffffff" : brg.settings.textColorDark

      RowLayout {
        id: simRow
        anchors.centerIn: parent
        spacing: 4

        Label {
          text: brg.mapSim.playing ? "⏸" : "▶"
          font.pixelSize: 10
          color: parent.parent.ink
        }

        Label {
          text: brg.mapSim.playing ? qsTr("Walking") : qsTr("Walk")
          font.pixelSize: 11
          font.bold: true
          color: parent.parent.ink
        }
      }

      HoverHandler {
        id: simHover
        enabled: brg.mapSim.canSimulate
        cursorShape: Qt.PointingHandCursor
      }

      TapHandler {
        enabled: brg.mapSim.canSimulate
        onTapped: {
          if (brg.mapSim.playing) {
            brg.mapSim.playing = false;
            return;
          }

          if (brg.settings.mapSimWarned)
            brg.mapSim.playing = true;
          else
            simWarning.open();
        }
      }

      // ⚠️ MapToolTip, NOT the stock `ToolTip.text` attached property. The stock one is DARK TEXT on
      // a translucent background, and over this pale toolbar it is genuinely hard to read -- Twilight
      // has said so more times than I want to count. MapToolTip is white-on-opaque-dark and it is the
      // ONLY tooltip anything on this screen may use. If you are typing `ToolTip.text`, stop.
      MapToolTip {
        shown: simHover.hovered || (!brg.mapSim.canSimulate && simDeadHover.hovered)
        text: !brg.mapSim.canSimulate
                ? qsTr("Nobody on this map can walk — they are all set to Stay")
                : brg.mapSim.playing
                  ? qsTr("Stop them")
                  : qsTr("Let the people wander — ⚠️ this MOVES the real sprite data")
      }

      // A disabled chip still has to be able to SAY why it is disabled.
      HoverHandler { id: simDeadHover }
    }

    SimWarningDialog {
      id: simWarning
      onAccepted: brg.mapSim.playing = true
    }

    Item { Layout.fillWidth: true }

    // ── The clutter switch, hard right ────────────────────────────────────────────────────────
    //
    // ⚠️ Twilight, 2026-07-13, and it is a good idea: *"Have a switch right-aligned on the top
    // toolbar that toggles on values that will be overwritten as options to change. Have it OFF by
    // default, give it a good label. When it's off, the fields that relate to things there's no
    // point in changing will not be present and add clutter."*
    //
    // Roughly a THIRD of a sprite's bytes are scratch the console works out again the moment it
    // loads the save -- the walk state, the on-screen pixels, the VRAM slot. They are real, they are
    // hers, and she can edit every one of them. But they are not what anybody came for, and having
    // them stacked under the fields that DO matter is the difference between a panel and a hex dump.
    //
    // Off: they simply are not there. On: they are, each wearing its yellow "!".
    //
    // ⚠️ AN ICON, NOT A LABELLED SWITCH. It was "Reloaded values" + a Switch, and Twilight:
    // *"it's way too long -- don't put a long label to the left of it, do something else, maybe an
    // icon of sorts."* She is right: a sentence of chrome sitting permanently in the toolbar to
    // describe a thing you touch once. It is a chip with a mark on it now, and the words are in its
    // tooltip -- which is the whole bargain this toolbar makes everywhere else.
    Rectangle {
      objectName: "showScratchToggle"   // the DEBUG harness drives the panel through this

      implicitWidth: 30
      implicitHeight: 26
      radius: 13

      readonly property bool on: brg.map.showScratch

      color: on ? "#ffd54f"
           : scratchHover.hovered ? Qt.rgba(0, 0, 0, 0.10)
           : Qt.rgba(0, 0, 0, 0.05)

      border.width: 1
      border.color: on ? "#8a6d00" : brg.settings.dividerColor

      Behavior on color { ColorAnimation { duration: 90 } }

      // The same "!" that marks every one of the fields it reveals. One mark, one meaning.
      Text {
        anchors.fill: parent
        text: "!"
        font.pixelSize: 13
        font.bold: true
        color: parent.on ? "#3a2e00" : brg.settings.textColorMid
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: parent.on ? 1.0 : 0.55
      }

      HoverHandler { id: scratchHover; cursorShape: Qt.PointingHandCursor }

      // ⚠️ ReleaseWithinBounds -- it takes an EXCLUSIVE GRAB, so the press stops here. The default
      // (DragThreshold) does not grab, and Qt then goes on delivering the point to every other
      // pointer handler underneath. That is the bug that made the map's ground tap fire through the
      // panels. @see MapCanvas.overPanel
      TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: brg.map.showScratch = !brg.map.showScratch
      }

      MapToolTip {
        shown: scratchHover.hovered
        text: qsTr("Show cleared values — the ones the game works out again every time it loads your "
                   + "save (the walk state, the on-screen pixels, the sprite cache).\n\nThey're real "
                   + "bytes and you can edit every one of them. They just won't survive Continue.")
      }
    }
  }
}
