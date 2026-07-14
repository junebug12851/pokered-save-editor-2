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

  // ⚠️ THE TOOLS AND THE MAKERS ARE NOT HERE ANY MORE (2026-07-14).
  //
  // They moved to the LEFT RAIL (Twilight: *"move the tools onto the left toolbar above the panels,
  // and the maker buttons below that"*). This bar is back to what it should always have been: a
  // statement of WHAT IS LOADED. `tool` is owned by the screen now (`mapScreen.tool`), the rail sets
  // it, and the canvas reads it. See Map.qml → the left dock's `railHeader`.

  /// The drop-downs, drivable by name for the DEBUG harness / the screenshot review.
  property alias mapPickerOpen: mapPicker.openState
  property alias contrastPickerOpen: contrastPicker.openState
  property alias contrastShowGlitch: contrastPicker.showGlitch
  property alias outsideOpen: outsidePicker.openState

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
    anchors.leftMargin: 10
    anchors.rightMargin: 10
    spacing: 7

    // ── The map's NAME, in bold — a LABEL, not a button ───────────────────────────────────────
    //
    // Twilight, 2026-07-14: *"'Pallet Town' is littered all over the toolbar and we're cramped. Have
    // the map name in bold like it is now on the left, but not in a button — as a label."*
    //
    // It used to appear THREE times (the map chip, "Outside is Pallet Town", "♪ Pallet Town"). Now
    // it is said once, here, and everything else is a compact icon.
    RowLayout {
      spacing: 6

      Label {
        text: brg.map.valid ? brg.map.mapName : qsTr("No map")
        font.pixelSize: 14
        font.bold: true
        color: brg.settings.textColorDark
        elide: Text.ElideRight
        Layout.maximumWidth: 190
      }

      // A fact, not an alarm: this id has no map of its own, so the game draws the one it copies.
      Label {
        visible: brg.map.isCopy
        text: qsTr("· copy of %1").arg(brg.map.copyOfName)
        font.pixelSize: 10
        font.italic: true
        color: brg.settings.textColorMid
        elide: Text.ElideRight
        Layout.maximumWidth: 130
      }
    }

    Rectangle {
      implicitWidth: 1
      implicitHeight: 18
      color: brg.settings.dividerColor
      Layout.leftMargin: 2
      Layout.rightMargin: 2
    }

    // ── The four icon buttons: Map · Warp · Contrast · Music ──────────────────────────────────
    //
    // Compact icon tool-buttons, each with a ▾ that says "I drop a menu" (Twilight). The wordy chips
    // are gone; each button is an icon — and where a reactive icon is natural it is one: Contrast IS
    // its live four-shade swatch, Music's ♪ lights up while it plays, the Map icon carries an amber
    // dot when the map's blocks/size don't match. All four share MapBarButton, so they read as one
    // family. See each picker file for the icon it hands the button.

    // ── Map (⊞): which map, its tileset, its blockset ────────────────────────────────────────
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

    // ── Contrast (+ Colour) ──────────────────────────────────────────────────────────────────
    //
    // ⚠️ The COLOUR filter used to be its own chip beside this one. It is now folded INTO this
    // dropdown, below the glitch-contrast switch (Twilight, 2026-07-14). They are the palette pair —
    // contrast picks *which of the four shades* a pixel becomes, colour picks *what those four
    // shades are painted* — so a person setting one is usually thinking about the other, and two
    // chips for one idea was a chip too many.
    ContrastPicker { id: contrastPicker }

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
