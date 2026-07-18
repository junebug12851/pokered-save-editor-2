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

    // ── The CONFIG buttons: Map · Warp · Contrast ────────────────────────────────────────────
    //
    // Compact icon tool-buttons, each with a ▾ that says "I drop a menu" (Twilight). Where a reactive
    // icon is natural it is one: Contrast IS its live four-shade swatch, the Map icon carries an amber
    // dot when the map's blocks/size don't match. They share MapBarButton, so they read as a family.
    // (Music used to be here too; it moved to the SIMULATION group below, with the other things you
    // play.)

    // ── Map (⊞): which map, its tileset, its blockset ────────────────────────────────────────
    MapPicker { id: mapPicker; objectName: "mapPickerControl" }

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

    // ══ THE SIMULATION GROUP — the three things you PLAY ══════════════════════════════════════
    //
    // Twilight, 2026-07-14: *"the music button, the tile-animation button and the walk button need to
    // all be icon buttons with an optional dropdown — a play/pause button next to a symbol, with a
    // little dropdown arrow. Those three are all SIMULATION and go in the group to the RIGHT of the
    // config buttons."* So they sit together, past a divider, each a MapSimButton (▶/⏸ + a symbol +
    // an optional ▾).
    Rectangle {
      implicitWidth: 1
      implicitHeight: 18
      color: brg.settings.dividerColor
      Layout.leftMargin: 4
      Layout.rightMargin: 4
    }

    // ⚠️ The three sim buttons sit in their OWN tight RowLayout, so they read as a GROUP -- close
    // together, and clearly apart from the config buttons across the divider (Twilight, 2026-07-14:
    // *"make the simulation buttons look like a proper grouping with proper spacing"*). With the ▾
    // now INSIDE each button, they are compact enough to cluster.
    RowLayout {
      spacing: 3

    // Music — ▶/⏸ ♪ ▾ (the ▾ drops the track / volume / flags).
    MusicPicker { id: musicPicker }

    // ── SIMULATION — one icon-text button, one panel ────────────────────────────────────────
    //
    // Twilight, 2026-07-14: *"move the walking button into the animation button, use the walking
    // symbol not the flower, make it an icon-text button, name the panel Simulation and put the tile
    // animation and the walking options together nicely in one panel."*
    //
    // So the two separate play buttons (tile animation ✿, walk 👣) become ONE `[👣 Simulate ⌄]`
    // button, filled orange while EITHER is running. Everything — play/pause, speed, step, the walk —
    // lives in the "Simulation" panel it drops.
    Item {
      id: simWrap
      objectName: "simGroup"   // the screenshot review drives the panel through this
      implicitWidth: simBtn.implicitWidth
      implicitHeight: 26

      property bool menuOpen: false
      readonly property bool anyPlaying: brg.mapClock.playing || brg.mapSim.playing

      // A little play/pause toggle, reused for both the tile animation and the walk, inside the panel.
      component PlayToggle: Rectangle {
        id: tog
        property bool on: false
        property bool en: true
        signal toggled()

        implicitWidth: 32
        implicitHeight: 24
        radius: 5

        color: !tog.en ? "transparent"
             : tog.on ? "#d55e00"
             : togHover.hovered ? "#f0f0f0" : "#ffffff"
        border.width: 1
        border.color: tog.on ? "#d55e00" : brg.settings.dividerColor
        opacity: tog.en ? 1.0 : 0.4

        Behavior on color { ColorAnimation { duration: 90 } }

        Text {
          anchors.centerIn: parent
          text: tog.on ? "⏸" : "▶"
          font.pixelSize: 11
          color: tog.on ? "#ffffff" : brg.settings.textColorDark
        }

        HoverHandler { id: togHover; enabled: tog.en; cursorShape: Qt.PointingHandCursor }
        TapHandler { enabled: tog.en; onTapped: tog.toggled() }
      }

      // The button: footprints + "Simulate" + ⌄. Orange while anything is running.
      Rectangle {
        id: simBtn
        anchors.fill: parent
        radius: 6
        implicitWidth: sbRow.implicitWidth + 16

        color: simWrap.anyPlaying ? "#d55e00"
             : (sbHover.hovered || simWrap.menuOpen) ? "#f0f0f0"
             : "#ffffff"
        border.width: 1
        border.color: simWrap.anyPlaying ? "#d55e00" : brg.settings.dividerColor

        Behavior on color { ColorAnimation { duration: 90 } }

        readonly property color ink: simWrap.anyPlaying ? "#ffffff" : brg.settings.textColorDark

        Row {
          id: sbRow
          anchors.centerIn: parent
          spacing: 5

          Image {
            anchors.verticalCenter: parent.verticalCenter
            width: 15; height: 15
            source: simWrap.anyPlaying ? "qrc:/assets/icons/footprints-light.svg"
                                       : "qrc:/assets/icons/footprints.svg"
            sourceSize: Qt.size(30, 30)
            fillMode: Image.PreserveAspectFit
            smooth: true
          }

          // Icon only — no text (Twilight). Just the ▾ to say it drops a menu.
          Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "⌄"
            font.pixelSize: 10
            color: simBtn.ink
            opacity: 0.8
          }
        }

        HoverHandler { id: sbHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: simWrap.menuOpen = !simWrap.menuOpen }

        MapToolTip {
          shown: sbHover.hovered && !simWrap.menuOpen
          text: qsTr("Simulation — animate the map and let the people walk")
        }
      }

      // ── The Simulation panel ─────────────────────────────────────────────────────────────
      Popup {
        visible: simWrap.menuOpen
        onClosed: simWrap.menuOpen = false

        y: simWrap.height + 5
        x: -40
        width: 230
        padding: 12

        background: Rectangle {
          color: "#ffffff"; radius: 8
          border.width: 1; border.color: brg.settings.dividerColor
        }

        ColumnLayout {
          width: parent.width
          spacing: 8

          Label {
            text: qsTr("Simulation")
            font.pixelSize: 12; font.bold: true
            color: brg.settings.textColorMid
          }

          Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: brg.settings.dividerColor }

          // ── Tile animation ──────────────────────────────────────────────────────────────
          RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
              Layout.fillWidth: true
              text: qsTr("Tile animation")
              font.pixelSize: 12; font.bold: true
              color: brg.settings.textColorDark
            }

            PlayToggle {
              on: brg.mapClock.playing
              en: brg.mapClock.animates
              onToggled: brg.mapClock.playing = !brg.mapClock.playing
            }
          }

          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: brg.mapClock.animates
                    ? qsTr("The water and the flowers, moving at the console's own pace.")
                    : qsTr("This map has nothing that animates.")
            font.pixelSize: 10
            opacity: 0.6
          }

          RowLayout {
            Layout.fillWidth: true
            visible: brg.mapClock.animates
            spacing: 4

            Label { text: qsTr("Speed"); font.pixelSize: 11; color: brg.settings.textColorMid }

            Repeater {
              model: [ { s: 0.5, label: "½×" }, { s: 1.0, label: "1×" }, { s: 2.0, label: "2×" } ]

              delegate: Rectangle {
                required property var modelData
                Layout.fillWidth: true
                implicitHeight: 24
                radius: 5

                readonly property bool on: Math.abs(brg.mapClock.speed - modelData.s) < 0.01

                color: on ? brg.settings.accentColor
                     : spdHover.hovered ? "#f0f0f0" : "#ffffff"
                border.width: 1; border.color: brg.settings.dividerColor

                Label {
                  anchors.centerIn: parent
                  text: modelData.label
                  font.pixelSize: 11; font.bold: parent.on
                  color: parent.on ? brg.settings.textColorLight : brg.settings.textColorDark
                }

                HoverHandler { id: spdHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: brg.mapClock.speed = modelData.s }
              }
            }

            Rectangle {
              implicitWidth: stepRow.implicitWidth + 10
              implicitHeight: 24
              radius: 5
              color: stepHover.hovered ? "#f0f0f0" : "#ffffff"
              border.width: 1; border.color: brg.settings.dividerColor

              RowLayout {
                id: stepRow
                anchors.centerIn: parent
                spacing: 2
                Label { text: "⏭"; font.pixelSize: 11 }
              }

              HoverHandler { id: stepHover; cursorShape: Qt.PointingHandCursor }
              TapHandler { onTapped: brg.mapClock.step() }

              MapToolTip { shown: stepHover.hovered; text: qsTr("Step one frame") }
            }
          }

          Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: brg.settings.dividerColor }

          // ── People walking ──────────────────────────────────────────────────────────────
          RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
              Layout.fillWidth: true
              text: qsTr("People walking")
              font.pixelSize: 12; font.bold: true
              color: brg.settings.textColorDark
            }

            PlayToggle {
              on: brg.mapSim.playing
              en: brg.mapSim.canSimulate
              onToggled: {
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
          }

          Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: !brg.mapSim.canSimulate
                    ? qsTr("Nobody on this map can walk — they are all set to Stay.")
                    : qsTr("Lets the people wander. ⚠️ This MOVES the real sprite data.")
            font.pixelSize: 10
            opacity: 0.6
          }
        }
      }
    }

    }   // end of the simulation group's RowLayout

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
