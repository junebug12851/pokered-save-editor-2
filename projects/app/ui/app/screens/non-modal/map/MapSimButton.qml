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
  MapSimButton.qml -- one control in the top bar's SIMULATION group: a play/pause tool button with a
  subject icon, and an optional ▾ that drops a menu.

  Twilight, 2026-07-14: *"the music button, tile-animation button and walk button need to all be icon
  buttons with an optional dropdown menu — a play/pause button next to a symbol, with a little dropdown
  arrow to the right. Flower for the tile animation, someone walking for the walk. Those three are all
  simulation and go in the group to the right of the config buttons."*

  So the three read as one family:

      [ ▶/⏸  ♪  ▾ ]     music        -- the dropdown is the track / volume / flags
      [ ▶/⏸  ✿  ▾ ]     tile anim    -- the dropdown is speed / step
      [ ▶/⏸  👣 ]        walk         -- (footprints, not a person: see footprints.svg)

  The button is play/pause; the ▾ (when there is one) opens a menu the CALLER owns and positions. This
  file is just the trigger.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
  id: sim
  spacing: 2

  /// The subject icon, as a glyph (♪, ✿) — leave empty to use @ref iconSource instead.
  property string glyph: ""

  /// The subject icon, as an SVG (the walk footprints) — the DARK one, for the button at rest.
  ///
  /// ⚠️ Two pre-coloured SVGs, swapped by state — NOT one SVG recoloured with a shader. `MultiEffect`
  /// (and every GPU recolour) draws NOTHING on Qt's software backend, which is what the headless
  /// screenshooter, the screenshot review and CI all run on — so the footprints came up blank there
  /// and could not be reviewed. Plain `Image` renders on every backend. (Same lesson as PixelImage's
  /// shader falling back to nearest — see ui-patterns.md.)
  property string iconSource: ""

  /// The WHITE version of the SVG, for the playing (filled-orange) state. Falls back to @ref
  /// iconSource if empty.
  property string iconSourcePlaying: ""

  /// Is it running? Drives ▶ vs ⏸ and the filled "on" look.
  property bool playing: false

  /// Can it run at all? A disabled sim button greys and says why in its tooltip.
  property bool playEnabled: true

  /// The play/pause button's tooltip.
  property string playTip: ""

  /// Does this button carry a ▾ dropdown? (Walk, for now, does not.)
  property bool hasMenu: false

  /// Is the dropdown open? The caller binds this to its own popup's open flag, so the ▾ lights up.
  property bool menuOpen: false

  /// The ▾'s tooltip.
  property string menuTip: ""

  /// Fired when the play/pause button is clicked.
  signal toggled()

  /// Fired when the ▾ is clicked.
  signal menuToggled()

  // ── Play / pause + the subject icon (one flat button) ──────────────────────────────────────
  Rectangle {
    id: playBtn
    Layout.preferredHeight: 26
    Layout.alignment: Qt.AlignVCenter
    implicitWidth: playRow.implicitWidth + 14
    radius: 6

    // Running = filled accent (the look the old Walk chip had). At rest = a plain outlined button,
    // like the config buttons. Spelled out, not inherited from the palette.
    color: !sim.playEnabled ? "transparent"
         : sim.playing      ? "#d55e00"
         : playHover.hovered ? "#f0f0f0"
         : "#ffffff"

    border.width: 1
    border.color: sim.playing ? "#d55e00" : brg.settings.dividerColor
    opacity: sim.playEnabled ? 1.0 : 0.4

    Behavior on color { ColorAnimation { duration: 90 } }

    readonly property color ink: sim.playing ? "#ffffff" : brg.settings.textColorDark

    Row {
      id: playRow
      anchors.centerIn: parent
      spacing: 3

      Text {
        anchors.verticalCenter: parent.verticalCenter
        text: sim.playing ? "⏸" : "▶"
        font.pixelSize: 10
        color: playBtn.ink
      }

      // The subject: a glyph…
      Text {
        anchors.verticalCenter: parent.verticalCenter
        visible: sim.glyph !== ""
        text: sim.glyph
        font.pixelSize: 13
        color: playBtn.ink
      }

      // …or an SVG. Two pre-coloured files, swapped by state — a plain Image, so it renders on the
      // software backend the screenshot review runs on (a shader recolour would draw nothing there).
      Image {
        anchors.verticalCenter: parent.verticalCenter
        visible: sim.iconSource !== ""
        width: 15
        height: 15
        source: (sim.playing && sim.iconSourcePlaying !== "") ? sim.iconSourcePlaying : sim.iconSource
        sourceSize: Qt.size(30, 30)
        fillMode: Image.PreserveAspectFit
        smooth: true
      }
    }

    HoverHandler { id: playHover; enabled: sim.playEnabled; cursorShape: Qt.PointingHandCursor }
    TapHandler { enabled: sim.playEnabled; onTapped: sim.toggled() }

    MapToolTip {
      shown: playHover.hovered && sim.playTip !== ""
      text: sim.playTip
    }
  }

  // ── The ▾ dropdown, when there is a menu ───────────────────────────────────────────────────
  MapRailButton {
    visible: sim.hasMenu
    size: 22
    glyph: "▾"
    active: sim.menuOpen
    tip: sim.menuTip
    onClicked: sim.menuToggled()
  }
}
