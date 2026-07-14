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

/**
 * MUSIC, in the top toolbar -- a chip that drops a picker, in exactly the same language as the
 * map/tileset/blockset one next to it (Twilight, 2026-07-13). The Music DOCK PANEL is gone.
 *
 *   [ ▶ ] [ ♪ Pallet Town ⌄ ]
 *
 * Inside the drop-down, top to bottom:
 *
 *   * a **real grouped ComboBox** of all 151 pieces -- the 46 real tracks and the 105 inner voices,
 *     which are real music too;
 *   * a **volume slider**, directly under it;
 *   * the **sub-tracks** toggle;
 *   * the two **save flags**.
 *
 * ⚠️ **REBUILT 2026-07-13.** The first cut was a hand-rolled ListView of 151 rows in a popup, which
 * Twilight had already told me not to build: *"The map dropdown has actual group real combo boxes,
 * the music dropdown does not — it's way too big and it's a manual list when I stated combo box
 * earlier. It's also missing the volume slider, which should be below the combo box."* It is now the
 * same `ComboBox` + grouped `ItemDelegate` as MapPicker's 248-map list, and for the same reason: one
 * control, one language, no wall.
 *
 * ⚠️ **HOVER NO LONGER AUDITIONS.** It did, and it was wrong -- sweeping the list to read it meant
 * the music kept changing under you. Twilight: *"Stop music change on hover change. Just only when
 * it's selected."* Selecting a track plays it (if the player is running) **and** writes the save.
 * There is one gesture and it does the one thing.
 *
 * ⚠️ Nothing plays on its own. The ▶ is a thing you press.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
  id: music
  objectName: "musicPicker"   // the DEBUG harness opens the drop-down through this
  spacing: 4

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

  // ── ▶ ─────────────────────────────────────────────────────────────────────────────────────
  MapRailButton {
    objectName: "musicPlay"
    size: 26
    glyph: brg.music.isPlaying ? "■" : "▶"
    enabledBtn: music.playable
    tip: !music.playable
           ? qsTr("This save's music bank isn't one a console can play (2, 8 or 31)")
           : brg.music.isPlaying ? qsTr("Stop") : qsTr("Play this map's music")

    onClicked: brg.music.isPlaying ? brg.music.stop() : brg.music.play()
  }

  // ── The chip ──────────────────────────────────────────────────────────────────────────────
  Rectangle {
    id: chip
    objectName: "musicChip"

    implicitWidth: Math.min(chipRow.implicitWidth + 18, 190)
    implicitHeight: 26
    radius: 13

    color: music.openState ? Qt.rgba(0, 0, 0, 0.10)
         : chipHover.hovered ? Qt.rgba(0, 0, 0, 0.06)
         : Qt.rgba(0, 0, 0, 0.03)

    border.width: 1
    border.color: brg.settings.dividerColor

    RowLayout {
      id: chipRow
      anchors.fill: parent
      anchors.leftMargin: 9
      anchors.rightMargin: 7
      spacing: 5

      Label {
        text: "♪"
        font.pixelSize: 12
        opacity: 0.75
      }

      // The chip always says what is IN THE SAVE. (It used to go italic while a hover-audition was
      // playing something else -- there are no auditions any more, so there is nothing to disclaim:
      // what you hear IS what is saved.)
      Label {
        Layout.fillWidth: true
        text: !music.hasAudio
                ? qsTr("—")
                : brg.music.describe(music.audio.musicBank, music.audio.musicID)
        font.pixelSize: 11
        font.bold: true
        elide: Text.ElideRight
      }

      Label {
        text: "⌄"
        font.pixelSize: 10
        opacity: 0.6
      }
    }

    HoverHandler { id: chipHover; cursorShape: Qt.PointingHandCursor }
    TapHandler { onTapped: music.openState = !music.openState }

    Popup {
      id: panel

      visible: music.openState
      onClosed: music.openState = false

      y: chip.height + 5
      x: -60

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

        // ── The track: a REAL grouped ComboBox ─────────────────────────────────────────────
        //
        // The same control, and the same grouped delegate, as MapPicker's 248-map list. The group
        // heading rides on the FIRST entry of its group (MusicPlayer::tracks puts it there), so
        // there is one model and one list rather than two that can drift apart.
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

          // A save can hold a bank/id the game never wrote. Say what it is rather than snapping the
          // combo to the nearest thing it likes.
          displayText: currentIndex >= 0
                         ? currentText
                         : (music.hasAudio
                              ? brg.music.describe(music.audio.musicBank, music.audio.musicID)
                              : "—")

          // ⚠️ SELECTING is the ONLY thing that changes the music, and it is also the only thing
          // that writes the save. Hovering a row used to audition it -- so reading the list changed
          // what you were hearing, which Twilight (rightly) asked me to stop.
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

          // ⚠️ NO `popup.height` BINDING. Sizing the popup from its own contentItem is a binding loop
          // -- Qt breaks it, and the rows come out too short to read AND unclickable. @see SpriteField.
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

            // ⚠️ `header`, NOT `group`. The model puts the group's name on the FIRST row of that
            // group and leaves it empty on the rest -- that is what makes it a heading. Reading
            // `group` (which every row carries) drew a heading above every single track, 151 times.
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
                  // An inner voice is one channel of a song, alone. Real music -- and worth saying
                  // it is a part rather than a whole.
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

        // ── Volume, DIRECTLY under the combo ───────────────────────────────────────────────
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

            // MapToolTip, not the stock one -- dark text on translucent is unreadable and it is the
            // recurring complaint on this screen. `followGlobalSetting: false`: a % readout while you
            // are dragging a slider is not a HINT, it is the control's own value.
            MapToolTip {
              parent: vol.handle
              shown: vol.pressed || vol.hovered
              followGlobalSetting: false
              delay: 0
              text: Math.round(vol.value * 100) + "%"
            }
          }
        }

        // ── Sub-tracks ─────────────────────────────────────────────────────────────────────
        //
        // The 105 "inner voices" are real music: one channel of a song, played alone, exactly as the
        // console plays it. But **OFF by default** (Twilight, 2026-07-13) -- 151 rows is a wall when
        // all you wanted was Pallet Town, and 46 is a list. They are here when you want them.
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

        // ── The two save flags, below the list ──────────────────────────────────────────────
        //
        // ⚠️ A stock CheckBox is ~40px tall by default, so two of them stacked leave a canyon between
        // the labels. Twilight: *"checkboxes are too far apart."* They are a pair of related flags,
        // so they read as a pair: tight, in their own column, with the spacing set here and not
        // inherited from whatever the style felt like.
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
}
