/*
  * Copyright 2026 Fairy Fox
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

/**
 * MUSIC, in the top bar's SIMULATION group -- a play/pause button with a ♪ and a ▾ that drops the
 * track / volume / flags (project leadership, 2026-07-14). It sits with the tile-animation and walk buttons,
 * because all three are things you PLAY.
 *
 * ⚠️ **HOVER NEVER AUDITIONS.** Selecting a track plays it (if the player is running) *and* writes the
 * save; hovering does nothing. One gesture, one effect.
 *
 * ⚠️ Nothing plays on its own. ▶ is a thing you press.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: music
  objectName: "musicPicker"

  implicitWidth: musicSim.implicitWidth
  implicitHeight: 26

  property bool openState: false

  /// The save's audio node: musicID / musicBank / noAudioFadeout / preventMusicChange.
  readonly property var audio: (brg.file && brg.file.data && brg.file.data.dataExpanded
                                && brg.file.data.dataExpanded.area)
                                 ? brg.file.data.dataExpanded.area.audio : null
  readonly property bool hasAudio: music.audio !== null

  readonly property bool playable: music.hasAudio
                                && brg.music.dataReady
                                && brg.music.isPlayableBank(music.audio.musicBank)

  // Keep the player's SELECTED track pointing at whatever the save says, so ▶ plays the map's music.
  function syncSelection() {
    if (!music.hasAudio)
      return;

    brg.music.selectedBank = music.audio.musicBank;
    brg.music.selectedId = music.audio.musicID;
  }

  Component.onCompleted: music.syncSelection()

  Connections {
    target: brg.map
    function onChanged() { music.syncSelection(); }
  }

  // ⚠️ Leaving the Map screen must not leave a tune humming behind it.
  Component.onDestruction: brg.music.stop()

  // ── The button: ▶/⏸ + ♪ + ▾ ────────────────────────────────────────────────────────────────
  MapSimButton {
    id: musicSim
    objectName: "musicChip"   // the DEBUG harness opens the drop-down through this (its ▾)

    glyph: "♪"

    playing: brg.music.isPlaying
    playEnabled: music.playable
    playTip: !music.playable
               ? qsTr("This save's music bank isn't one a console can play (2, 8 or 31)")
               : brg.music.isPlaying ? qsTr("Stop") : qsTr("Play this map's music")
    onToggled: brg.music.isPlaying ? brg.music.stop() : brg.music.play()

    hasMenu: true
    menuOpen: music.openState
    menuTip: qsTr("Music — %1").arg(music.hasAudio
               ? brg.music.describe(music.audio.musicBank, music.audio.musicID)
               : "—")
    onMenuToggled: music.openState = !music.openState
  }

  // ── The drop-down ──────────────────────────────────────────────────────────────────────────
  Popup {
    id: panel

    visible: music.openState
    onClosed: music.openState = false

    y: musicSim.height + 5
    x: -40

    width: 250
    padding: 8

    background: Rectangle {
      color: "#ffffff"
      radius: 8
      border.width: 1
      border.color: brg.settings.dividerColor
    }

    ColumnLayout {
      width: parent.width
      spacing: 8

      // ── The track: a REAL grouped ComboBox ─────────────────────────────────────────────────
      ComboBox {
        id: trackCombo

        Layout.fillWidth: true
        Layout.preferredHeight: 32
        font.pixelSize: 12

        model: showInner.checked
               ? brg.music.tracks
               : brg.music.tracks.filter(function(t) { return !t.inner; })

        textRole: "name"

        currentIndex: {
          if (!music.hasAudio)
            return -1;

          const list = trackCombo.model;
          for (let i = 0; i < list.length; i++)
            if (list[i].bank === music.audio.musicBank && list[i].id === music.audio.musicID)
              return i;

          return -1;   // a bank/id pair no track has -- shown as it is, never corrected
        }

        displayText: currentIndex >= 0
                       ? currentText
                       : (music.hasAudio
                            ? brg.music.describe(music.audio.musicBank, music.audio.musicID)
                            : "—")

        // ⚠️ SELECTING is the ONLY thing that changes the music, and the only thing that writes the
        // save. Hovering a row does not audition.
        onActivated: {
          if (!music.hasAudio)
            return;

          const t = trackCombo.model[trackCombo.currentIndex];
          if (!t)
            return;

          music.audio.musicBank = t.bank;
          music.audio.musicID = t.id;

          brg.music.select(t.bank, t.id);
        }

        delegate: ItemDelegate {
          id: row
          required property var modelData
          required property int index

          width: trackCombo.width
          height: (row.heading !== "" ? 20 : 0) + 28
          highlighted: trackCombo.highlightedIndex === row.index

          readonly property bool isSaved: music.hasAudio
                                       && music.audio.musicBank === row.modelData.bank
                                       && music.audio.musicID === row.modelData.id

          readonly property string heading: row.modelData.header || ""

          contentItem: ColumnLayout {
            spacing: 0

            Text {
              visible: row.heading !== ""
              Layout.fillWidth: true
              text: row.heading
              font.pixelSize: 10
              font.bold: true
              color: brg.settings.textColorMid
            }

            RowLayout {
              Layout.fillWidth: true
              spacing: 6

              Text {
                Layout.fillWidth: true
                text: row.modelData.name
                font.pixelSize: 12
                font.italic: row.modelData.inner
                color: brg.settings.textColorDark
                opacity: row.modelData.inner ? 0.8 : 1.0
                elide: Text.ElideRight
              }

              Text {
                visible: row.isSaved
                text: qsTr("in the save")
                font.pixelSize: 9
                color: "#0072b2"
              }
            }
          }
        }
      }

      // ── Volume, DIRECTLY under the combo ─────────────────────────────────────────────────────
      RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Label {
          text: "🔉"
          font.pixelSize: 12
          opacity: 0.6
        }

        Slider {
          id: vol
          Layout.fillWidth: true
          Layout.preferredHeight: 22

          from: 0.0
          to: 1.0
          value: brg.music.volume
          onMoved: brg.music.volume = value

          MapToolTip {
            parent: vol.handle
            shown: vol.pressed || vol.hovered
            followGlobalSetting: false
            delay: 0
            text: Math.round(vol.value * 100) + "%"
          }
        }
      }

      // ── Sub-tracks ───────────────────────────────────────────────────────────────────────────
      RowLayout {
        Layout.fillWidth: true
        spacing: 6

        Label {
          Layout.fillWidth: true
          text: qsTr("Sub-tracks")
          font.pixelSize: 11
          color: "#424242"
        }

        Switch {
          id: showInner
          checked: false
          implicitHeight: 22
          scale: 0.8
        }
      }

      Rectangle {
        Layout.fillWidth: true
        implicitHeight: 1
        color: brg.settings.dividerColor
      }

      // ── The two save flags ───────────────────────────────────────────────────────────────────
      ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        CheckBox {
          Layout.fillWidth: true
          implicitHeight: 26
          topPadding: 0
          bottomPadding: 0

          text: qsTr("No audio fade-out")
          checked: music.hasAudio ? music.audio.noAudioFadeout : false
          enabled: music.hasAudio
          onToggled: if (music.hasAudio) music.audio.noAudioFadeout = checked

          contentItem: Label {
            text: parent.text
            font.pixelSize: 11
            color: "#424242"
            leftPadding: parent.indicator.width + 5
            verticalAlignment: Text.AlignVCenter
          }
        }

        CheckBox {
          Layout.fillWidth: true
          implicitHeight: 26
          topPadding: 0
          bottomPadding: 0

          text: qsTr("Don't change music on map entry")
          checked: music.hasAudio ? music.audio.preventMusicChange : false
          enabled: music.hasAudio
          onToggled: if (music.hasAudio) music.audio.preventMusicChange = checked

          contentItem: Label {
            text: parent.text
            font.pixelSize: 11
            color: "#424242"
            leftPadding: parent.indicator.width + 5
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
          }
        }
      }
    }
  }
}
