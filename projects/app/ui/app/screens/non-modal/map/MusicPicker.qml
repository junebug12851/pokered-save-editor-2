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
 * Inside the drop-down: a **sub-tracks** toggle, the **track list** (all 151 -- the 46 real tracks
 * and the 105 inner voices, which are real music too), and the two **save flags** underneath.
 *
 * **Hover auditions; click commits.** Running the mouse down the list changes what you hear and
 * touches nothing. A line says plainly when what you are hearing is not what is stored.
 *
 * ⚠️ Nothing plays on its own. The ▶ is a thing you press.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
  id: music
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

      Label {
        Layout.fillWidth: true
        text: !music.hasAudio ? qsTr("—")
            : brg.music.previewing ? brg.music.playingName
            : brg.music.describe(music.audio.musicBank, music.audio.musicID)
        font.pixelSize: 11
        font.bold: !brg.music.previewing
        // ⚠️ Italic while you are AUDITIONING -- what you are hearing is not what is saved, and the
        // chip must not quietly imply that it is.
        font.italic: brg.music.previewing
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
      onClosed: {
        music.openState = false;
        brg.music.unpreview();     // stop auditioning; snap back to the truth
      }

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
        spacing: 6

        // ── Sub-tracks, above the list ─────────────────────────────────────────────────────
        //
        // The 105 "inner voices" are real music: one channel of a song, played alone, exactly as
        // the console plays it. On by default, because they are the point -- but 151 rows is a lot
        // when all you want is Pallet Town.
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
            checked: true
            implicitHeight: 22
            scale: 0.8
          }
        }

        Label {
          Layout.fillWidth: true
          visible: brg.music.previewing
          text: qsTr("You're hearing a preview. The save still has %1.")
                  .arg(music.hasAudio
                         ? brg.music.describe(music.audio.musicBank, music.audio.musicID) : "—")
          font.pixelSize: 10
          color: "#c77800"
          wrapMode: Text.Wrap
        }

        // ── The tracks ─────────────────────────────────────────────────────────────────────
        ListView {
          id: list

          Layout.fillWidth: true
          Layout.preferredHeight: 210
          clip: true

          model: showInner.checked
                 ? brg.music.tracks
                 : brg.music.tracks.filter(function(t) { return !t.inner; })

          boundsBehavior: Flickable.StopAtBounds
          ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

          // Hover auditions -- but only once you SETTLE. A fast sweep must not machine-gun the
          // engine with 151 track changes.
          Timer {
            id: settle
            interval: 120
            property int bank: 0
            property int id: 0
            onTriggered: brg.music.preview(bank, id)
          }

          delegate: Rectangle {
            id: row
            required property var modelData

            width: ListView.view.width
            implicitHeight: 22
            radius: 4

            readonly property bool isSaved: music.hasAudio
                                         && music.audio.musicBank === row.modelData.bank
                                         && music.audio.musicID === row.modelData.id

            color: row.isSaved ? Qt.rgba(0.34, 0.71, 0.91, 0.18)
                 : rowHover.hovered ? Qt.rgba(0, 0, 0, 0.06)
                 : "transparent"

            RowLayout {
              anchors.fill: parent
              anchors.leftMargin: 6
              anchors.rightMargin: 6
              spacing: 6

              Label {
                Layout.fillWidth: true
                text: row.modelData.name
                font.pixelSize: 11
                font.italic: row.modelData.inner
                color: "#212121"
                opacity: row.modelData.inner ? 0.75 : 1.0
                elide: Text.ElideRight
              }

              Label {
                visible: row.isSaved
                text: qsTr("in the save")
                font.pixelSize: 9
                color: "#0072b2"
              }
            }

            HoverHandler {
              id: rowHover
              cursorShape: Qt.PointingHandCursor

              // HOVER AUDITIONS. It never touches the save.
              onHoveredChanged: {
                if (!hovered || !brg.music.isPlaying)
                  return;

                settle.bank = row.modelData.bank;
                settle.id = row.modelData.id;
                settle.restart();
              }
            }

            // CLICK COMMITS. This, and only this, writes the save.
            TapHandler {
              onTapped: {
                if (!music.hasAudio)
                  return;

                music.audio.musicBank = row.modelData.bank;
                music.audio.musicID = row.modelData.id;

                brg.music.select(row.modelData.bank, row.modelData.id);
              }
            }
          }
        }

        Rectangle {
          Layout.fillWidth: true
          implicitHeight: 1
          color: brg.settings.dividerColor
        }

        // ── The two save flags, below the list ──────────────────────────────────────────────
        CheckBox {
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
            wrapMode: Text.Wrap
          }
        }
      }
    }
  }
}
