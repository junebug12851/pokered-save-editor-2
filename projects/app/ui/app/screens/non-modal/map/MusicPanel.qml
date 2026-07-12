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

  // Sit visibly BESIDE the map rather than bleeding into it: a hair of grey and one hard edge.
  color: "#fafafa"

  Rectangle {
    anchors.left: parent.left
    width: 1
    height: parent.height
    color: "#22000000"
  }

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
    spacing: 8

    // ── Now playing ──────────────────────────────────────────────────────────────────────────
    Text {
      text: qsTr("Music")
      font.pixelSize: 13
      font.bold: true
      color: brg.settings.textColorDark
    }

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

    Text {
      text: qsTr("Tracks")
      font.pixelSize: 11
      font.bold: true
      color: brg.settings.textColorMid
      Layout.topMargin: 2
    }

    // ── The tracks ───────────────────────────────────────────────────────────────────────────
    ListView {
      id: list

      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.bottomMargin: 2
      clip: true
      model: brg.music.trackCount()
      currentIndex: -1
      boundsBehavior: Flickable.StopAtBounds

      ScrollBar.vertical: ScrollBar {}

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

        required property int index

        readonly property var entry: brg.music.track(index)
        readonly property bool isSelected: panel.hasAudio
                                           && entry.name !== undefined
                                           && entry.bank === panel.audio.musicBank
                                           && entry.id === panel.audio.musicID
        readonly property bool isHeard: brg.music.isPlaying
                                        && entry.name !== undefined
                                        && entry.bank === brg.music.playingBank
                                        && entry.id === brg.music.playingId

        width: list.width
        height: 24

        Rectangle {
          anchors.fill: parent
          color: row.isHeard ? "#2233aaff"
               : mouse.containsMouse ? "#11000000"
               : "transparent"
        }

        RowLayout {
          anchors.fill: parent
          anchors.leftMargin: 4
          anchors.rightMargin: 4
          spacing: 6

          Text {
            text: row.isSelected ? "✓" : ""
            color: brg.settings.textColorDark
            font.pixelSize: 11
            Layout.preferredWidth: 10
          }

          Text {
            Layout.fillWidth: true
            text: row.entry.name !== undefined ? row.entry.name : ""
            font.pixelSize: 11
            font.bold: row.isSelected
            color: brg.settings.textColorDark
            elide: Text.ElideRight
          }

          Text {
            text: row.entry.name !== undefined
                  ? qsTr("%1·%2").arg(row.entry.bank).arg(row.entry.id) : ""
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
            if (row.entry.name === undefined) return;
            settle.bank = row.entry.bank;
            settle.id = row.entry.id;
            settle.restart();
          }
          onExited: settle.stop()

          // Click COMMITS: this is now the map's music, in the save.
          onClicked: {
            if (row.entry.name === undefined) return;
            settle.stop();
            snapBack.stop();
            brg.music.select(row.entry.bank, row.entry.id);
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
