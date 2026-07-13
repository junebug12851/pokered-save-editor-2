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
  The Map screen's Music panel.

  Placement here is DELIBERATELY provisional -- Twilight is redoing the UI/UX later -- which is
  exactly why the whole thing is ONE file: a redesign moves one component, not a dozen bindings
  scattered through Map.qml.

  The rule the interaction is built on: HOVER AUDITIONS, CLICK COMMITS. Moving a mouse never touches
  the save, and audio never starts because a cursor drifted somewhere. See notes/plans/music.md §6.
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: panel

  // The save's audio node: musicID / musicBank / noAudioFadeout / preventMusicChange.
  //
  // The screen can exist with no save open (the QML smoke test loads every screen cold), so this
  // may be null -- and every binding below has to survive that. `hasAudio` is the single guard.
  readonly property var audio: (brg.file.data.dataExpanded
                                && brg.file.data.dataExpanded.area)
                               ? brg.file.data.dataExpanded.area.audio
                               : null
  readonly property bool hasAudio: audio !== null && audio !== undefined
                                   && audio.musicID !== undefined

  // The dock owns the panel's frame and its title bar (MapDock.qml). A panel is its CONTENT.
  color: "transparent"

  // ── Keep the player's idea of "selected" in step with the save ─────────────────────────────
  function syncFromSave() {
    if (!hasAudio) return;
    brg.music.selectedBank = audio.musicBank;
    brg.music.selectedId = audio.musicID;
  }

  Component.onCompleted: syncFromSave()
  onAudioChanged: syncFromSave()

  Connections {
    target: brg.music
    // select() is the ONLY thing that writes the save. Hovering cannot reach this.
    function onTrackSelected(bank, id) {
      if (!panel.hasAudio) return;
      panel.audio.musicBank = bank;
      panel.audio.musicID = id;
    }
  }

  // Nothing keeps humming behind another screen.
  Component.onDestruction: brg.music.stop()

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 10
    // The LIST is the point of this panel -- 151 pieces of music. Everything above it is chrome and
    // gets the minimum it needs. (Qt 6 Material controls are tall by default: left alone, the two
    // checkboxes alone ate enough height to leave three rows of tracks visible. See ui-patterns.md.)
    spacing: 4

    // ── Now playing ──────────────────────────────────────────────────────────────────────────
    Text {
      Layout.fillWidth: true
      text: brg.music.isPlaying
            ? brg.music.playingName
            : (panel.hasAudio
               ? brg.music.describe(panel.audio.musicBank, panel.audio.musicID)
               : qsTr("No save open"))
      font.pixelSize: 12
      color: brg.settings.textColorDark
      elide: Text.ElideRight
      wrapMode: Text.NoWrap
    }

    // The whole safety of hover-preview: it is never a mystery whether you are hearing the save.
    Text {
      Layout.fillWidth: true
      visible: brg.music.previewing && panel.hasAudio
      text: panel.hasAudio
            ? qsTr("previewing — the save still holds %1")
                .arg(brg.music.describe(panel.audio.musicBank, panel.audio.musicID))
            : ""
      font.pixelSize: 10
      font.italic: true
      color: brg.settings.textColorMid
      elide: Text.ElideRight
    }

    // ── Transport ────────────────────────────────────────────────────────────────────────────
    RowLayout {
      Layout.fillWidth: true
      Layout.preferredHeight: 28
      spacing: 8

      Button {
        text: brg.music.isPlaying ? "■" : "▶"
        enabled: brg.music.dataReady
                 && panel.hasAudio
                 && brg.music.isPlayableBank(panel.audio.musicBank)
        implicitWidth: 34
        implicitHeight: 26
        onClicked: brg.music.isPlaying ? brg.music.stop() : brg.music.play()
        ToolTip.visible: hovered
        ToolTip.text: brg.music.isPlaying ? qsTr("Stop") : qsTr("Play this map's music")
      }

      Slider {
        Layout.fillWidth: true
        from: 0
        to: 1
        value: brg.music.volume
        onMoved: brg.music.volume = value
      }
    }

    // ── The two save flags ───────────────────────────────────────────────────────────────────
    CheckBox {
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      topPadding: 0
      bottomPadding: 0
      leftPadding: 0
      font.pixelSize: 11
      text: qsTr("No Audio Fadeout")
      enabled: panel.hasAudio
      checked: panel.hasAudio ? panel.audio.noAudioFadeout : false
      onToggled: if (panel.hasAudio) panel.audio.noAudioFadeout = checked
      ToolTip.visible: hovered
      ToolTip.text: qsTr("The game normally forces the volume back to full every frame. With this " +
                         "set, it leaves the volume alone.")
    }

    CheckBox {
      Layout.fillWidth: true
      Layout.preferredHeight: 26
      topPadding: 0
      bottomPadding: 0
      leftPadding: 0
      font.pixelSize: 11
      text: qsTr("Prevent Music Change")
      enabled: panel.hasAudio
      checked: panel.hasAudio ? panel.audio.preventMusicChange : false
      onToggled: if (panel.hasAudio) panel.audio.preventMusicChange = checked
      ToolTip.visible: hovered
      ToolTip.text: qsTr("Entering a map won't start that map's music — whatever is playing keeps " +
                         "playing.")
    }

    // ── The save holds a bank the real game cannot play ───────────────────────────────────────
    Text {
      Layout.fillWidth: true
      visible: panel.hasAudio && !brg.music.isPlayableBank(panel.audio.musicBank)
      text: panel.hasAudio
            ? qsTr("⚠ This save's music bank is %1. The real game maps that bank and then runs it " +
                   "as code — it hangs the console. Left exactly as it is; pick a track below to " +
                   "fix it.").arg(panel.audio.musicBank)
            : ""
      font.pixelSize: 10
      color: brg.settings.errorColor
      wrapMode: Text.WordWrap
    }

    // ── Tracks header + the inner-voice switch ───────────────────────────────────────────────
    RowLayout {
      Layout.fillWidth: true
      Layout.topMargin: 2
      spacing: 6

      Text {
        text: qsTr("Tracks")
        font.pixelSize: 11
        font.bold: true
        color: brg.settings.textColorMid
      }

      Item { Layout.fillWidth: true }

      // The 105 "inner voices" are real music -- a song's single channel, played alone, exactly as
      // the console plays it. They are on by default because they are the point; the switch is here
      // because 151 rows is a lot when all you want is Pallet Town.
      Text {
        text: qsTr("inner voices")
        font.pixelSize: 10
        color: brg.settings.textColorMid
      }

      Switch {
        id: showInner
        checked: true
        implicitHeight: 22
        scale: 0.75
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Each song's individual channels, playable on their own — 105 of them. " +
                           "Not a trick: the game itself will play these if a save asks for them.")
      }
    }

    // ── The tracks ───────────────────────────────────────────────────────────────────────────
    ListView {
      id: list

      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.bottomMargin: 2
      clip: true
      // Every piece of music in the game: the 46 tracks and their 105 inner voices.
      model: showInner.checked
             ? brg.music.tracks
             : brg.music.tracks.filter(function(t) { return !t.inner; })
      currentIndex: -1
      boundsBehavior: Flickable.StopAtBounds

      ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

      // Hover auditions -- but only after you SETTLE on a row. A fast sweep down the list must not
      // machine-gun the engine.
      Timer {
        id: settle
        interval: 120
        property int bank: 0
        property int id: 0
        onTriggered: brg.music.preview(bank, id)
      }

      // Leaving the list snaps back to what is actually in the save -- after a grace, so that
      // crossing the gap between two rows doesn't trigger it.
      Timer {
        id: snapBack
        interval: 400
        onTriggered: brg.music.unpreview()
      }

      delegate: Item {
        id: row

        required property var modelData

        readonly property var entry: modelData
        readonly property bool isSelected: panel.hasAudio
                                           && entry.bank === panel.audio.musicBank
                                           && entry.id === panel.audio.musicID
        readonly property bool isHeard: brg.music.isPlaying
                                        && entry.bank === brg.music.playingBank
                                        && entry.id === brg.music.playingId
        readonly property bool hasHeader: entry.header !== undefined && entry.header !== ""

        width: list.width
        height: (hasHeader ? 20 : 0) + 24

        // ── Group heading (Towns & Cities / Routes / Places / Battle / Encounters / Special)
        Text {
          visible: row.hasHeader
          x: 2
          height: 20
          verticalAlignment: Text.AlignVCenter
          text: row.hasHeader ? row.entry.header : ""
          font.pixelSize: 10
          font.bold: true
          color: brg.settings.textColorMid
        }

        Item {
          id: rowBody
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.bottom: parent.bottom
          height: 24

        Rectangle {
          anchors.fill: parent
          color: row.isHeard ? "#2233aaff"
               : mouse.containsMouse ? "#11000000"
               : "transparent"
        }

        RowLayout {
          anchors.fill: parent
          // Inner voices sit UNDER the song they came out of, so it reads as what it is: this song,
          // one channel at a time.
          anchors.leftMargin: row.entry.inner ? 16 : 4
          // RESERVE THE SCROLLBAR LANE. The ScrollBar is an OVERLAY -- it sits on top of the rows,
          // so anything flush right (here: the ▶ and the bank·id) ends up underneath it and can't
          // be clicked. Recurring gotcha; see reference/ui-patterns.md.
          anchors.rightMargin: 18
          spacing: 6

          Text {
            text: row.isSelected ? "✓" : ""
            color: brg.settings.textColorDark
            font.pixelSize: 11
            Layout.preferredWidth: 10
          }

          Text {
            Layout.fillWidth: true
            // An inner voice already carries its parent's name; inside the list that is just noise.
            text: row.entry.inner
                  ? qsTr("channel %1 alone").arg(row.entry.channel)
                  : row.entry.name
            font.pixelSize: row.entry.inner ? 10 : 11
            font.italic: row.entry.inner
            font.bold: row.isSelected
            color: row.entry.inner ? brg.settings.textColorMid : brg.settings.textColorDark
            elide: Text.ElideRight
          }

          Text {
            text: qsTr("%1·%2").arg(row.entry.bank).arg(row.entry.id)
            font.pixelSize: 9
            color: brg.settings.textColorMid
            visible: mouse.containsMouse
          }

          Text {
            text: "▶"
            font.pixelSize: 10
            color: brg.settings.textColorMid
            visible: mouse.containsMouse

            MouseArea {
              anchors.fill: parent
              anchors.margins: -4
              cursorShape: Qt.PointingHandCursor
              onClicked: {
                // A pinned audition: play it, but do NOT commit it to the save.
                if (!brg.music.isPlaying)
                  brg.music.play();
                brg.music.preview(row.entry.bank, row.entry.id);
                snapBack.stop();
              }
            }
          }
        }

        MouseArea {
          id: mouse
          anchors.fill: parent
          hoverEnabled: true
          cursorShape: Qt.PointingHandCursor

          onEntered: {
            snapBack.stop();
            settle.bank = row.entry.bank;
            settle.id = row.entry.id;
            settle.restart();
          }
          onExited: settle.stop()

          // Click COMMITS: this is now the map's music, in the save.
          onClicked: {
            settle.stop();
            snapBack.stop();
            brg.music.select(row.entry.bank, row.entry.id);
          }
        }
        }
      }

      // The mouse left the list entirely: snap back to the truth, after a grace.
      HoverHandler {
        onHoveredChanged: {
          if (!hovered && brg.music.previewing)
            snapBack.restart();
          else
            snapBack.stop();
        }
      }
    }
  }
}
