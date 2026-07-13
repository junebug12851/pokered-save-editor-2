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
 * The DETAILS panel -- it edits whatever is selected, and it is never blank.
 *
 * With **nothing selected** it shows the MAP's own details. With a **sprite selected** it shows
 * every byte that sprite has, grouped and explained, each one editable across its **full range** --
 * hack and glitch values included, flagged in words, never refused and never quietly corrected.
 *
 * Right now sprites are the only selectable thing on the map (the ground is deliberately not
 * clickable); warps, signs and connections join them later, on the same machinery.
 *
 * ⚠️ It also tells the truth about something the cartridge does and nobody would guess: **the game
 * rebuilds the map's original cast from ROM the moment the player leaves the map and walks back
 * in.** An edited sprite is really there when the save loads — and it does not survive a round
 * trip through the door. Verified on the console. See notes/reference/sprites.md, Part 6.
 *
 * @see notes/plans/map-screen.md -> Phase 4d
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: details
  objectName: "mapDetails"   // the DEBUG harness reads the two diag properties below

  /// The map canvas, so we know what is selected. Handed over by MapDock (a Loader's content
  /// cannot see the ids around it), so it is briefly null before that happens.
  property var canvas: null

  /// Harness-visible: how many field rows the panel THINKS it has, and for whom. A panel that is
  /// empty when it shouldn't be is otherwise very hard to tell apart from one that is correct.
  readonly property int diagFieldCount: (details.fields || []).length
  readonly property int diagSlot: details.slot

  readonly property int slot: canvas ? canvas.selectedNpc : -1
  readonly property bool hasSprite: slot > 0

  // ⚠️ BINDINGS, not an imperative refresh().
  //
  // The first version called a refresh() from `onSlotChanged` + `Component.onCompleted`, and it
  // silently never ran: the panel is created by a Loader with `canvas` still null (so slot is -1),
  // and MapDock hands the canvas over in `onLoaded` -- AFTER completion. The panel came up
  // permanently blank while every C++ test passed, because the C++ was fine and the QML never
  // asked it anything. Bind it and the question gets asked whenever the answer could have changed.
  //
  // `revision` is what makes an EDIT re-ask: the model's own values change under us, and a binding
  // on npcAt(slot) alone would not know that.
  property int revision: 0

  Connections {
    target: brg.map
    function onChanged() { details.revision++; }
  }

  readonly property var sprite: {
    details.revision;   // a dependency, deliberately
    return details.hasSprite ? brg.map.npcAt(details.slot) : ({});
  }

  readonly property var fields: {
    details.revision;
    return details.hasSprite ? brg.map.npcFields(details.slot) : [];
  }

  readonly property var groupOrder: ["Who", "Where", "Movement", "What it is", "Animation scratch"]

  ScrollView {
    id: scroller
    anchors.fill: parent
    clip: true
    contentWidth: availableWidth          // never scroll sideways; the rows wrap instead
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ColumnLayout {
      width: scroller.availableWidth
      spacing: 8

      // ── Nothing selected: the MAP's own details ───────────────────────────────────────────
      //
      // The panel is never blank. Editing "what this map is" has one home, and this is it.
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 6
        visible: !details.hasSprite

        Label {
          text: brg.map.mapName
          font.bold: true
          font.pixelSize: 15
          Layout.fillWidth: true
          elide: Text.ElideRight
        }

        Label {
          Layout.fillWidth: true
          text: qsTr("Nothing selected — click somebody on the map to edit them.")
          wrapMode: Text.Wrap
          opacity: 0.6
          font.pixelSize: 11
        }

        MapDetailRow { label: qsTr("Map id");     value: String(brg.map.mapInd) }
        MapDetailRow { label: qsTr("Size");       value: qsTr("%1 × %2 blocks").arg(brg.map.blocksWide).arg(brg.map.blocksHigh) }
        MapDetailRow { label: qsTr("Tileset");    value: brg.map.tilesetName }
        MapDetailRow { label: qsTr("Palette");    value: brg.map.contrastName }
        MapDetailRow { label: qsTr("Player at");  value: qsTr("%1, %2").arg(brg.map.playerX).arg(brg.map.playerY) }
        MapDetailRow { label: qsTr("People");     value: qsTr("%1 of 15").arg(15 - brg.map.npcRoomLeft()) }

        // The view pointer the GAME itself computed and left in the save. If an edit has made it
        // stale we say so plainly and offer the one-click fix -- we never quietly rewrite it.
        Rectangle {
          Layout.fillWidth: true
          Layout.topMargin: 6
          visible: !brg.map.headerMatches
          radius: 6
          color: Qt.rgba(1, 0.84, 0.31, 0.15)
          border.width: 1
          border.color: "#ffd54f"
          implicitHeight: fixCol.implicitHeight + 16

          ColumnLayout {
            id: fixCol
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            Label {
              Layout.fillWidth: true
              text: qsTr("This map's header no longer matches what the game would compute.")
              wrapMode: Text.Wrap
              font.pixelSize: 11
            }

            Button {
              text: qsTr("Fix it")
              onClicked: brg.map.fixMapHeader()
            }
          }
        }
      }

      // ── A sprite selected ────────────────────────────────────────────────────────────────
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 8
        visible: details.hasSprite

        RowLayout {
          Layout.fillWidth: true
          spacing: 8

          Image {
            width: 32
            height: 32
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            source: details.sprite.source || ""
            smooth: false
            fillMode: Image.PreserveAspectFit
          }

          ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
              text: details.sprite.name || ""
              font.bold: true
              font.pixelSize: 14
              Layout.fillWidth: true
              elide: Text.ElideRight
            }

            Label {
              text: qsTr("Slot %1").arg(details.slot)
              font.pixelSize: 10
              opacity: 0.6
            }
          }

          ToolButton {
            text: "✕"
            ToolTip.visible: brg.settings.infoBtnPressed && (hovered)
            ToolTip.text: qsTr("Remove this sprite")
            onClicked: {
              brg.map.removeNpc(details.slot);
              if (details.canvas)
                details.canvas.selectedNpc = -1;
            }
          }
        }

        // The one thing about a sprite you cannot see by looking at it.
        Rectangle {
          Layout.fillWidth: true
          visible: details.sprite.inSpriteSet === false
          radius: 6
          color: Qt.rgba(1, 0.84, 0.31, 0.15)
          border.width: 1
          border.color: "#ffd54f"
          implicitHeight: setWarn.implicitHeight + 14

          Label {
            id: setWarn
            anchors.fill: parent
            anchors.margins: 7
            text: qsTr("This map hasn't loaded this character's picture, so the game will draw it as garbage. The save allows it — we're just telling you.")
            wrapMode: Text.Wrap
            font.pixelSize: 11
          }
        }

        // ⚠️ The honest note -- and it only appears once you have actually CHANGED something.
        //
        // A first version of this compared the save's cast against the ROM's and showed the warning
        // whenever they differed. That was wrong: a real save's cast ALREADY differs, because
        // walking NPCs wander. Pallet Town's girl stands at (3,8) in the cartridge and (3,6) in the
        // fixture save -- she had simply taken a few steps. The warning would have fired on
        // essentially every save ever loaded, which is noise, and noise is a bug.
        Rectangle {
          Layout.fillWidth: true
          visible: brg.map.npcsEdited()
          radius: 6
          color: Qt.rgba(0.34, 0.71, 0.91, 0.12)
          border.width: 1
          border.color: "#56b4e9"
          implicitHeight: romWarn.implicitHeight + 14

          Label {
            id: romWarn
            anchors.fill: parent
            anchors.margins: 7
            text: qsTr("This map's cast no longer matches the game's. Your changes are real and they load — but the game rebuilds the original cast from the cartridge the moment the player leaves this map and walks back in.")
            wrapMode: Text.Wrap
            font.pixelSize: 11
          }
        }

        // ── Every byte, grouped ──────────────────────────────────────────────────────────────
        Repeater {
          model: details.groupOrder

          delegate: ColumnLayout {
            id: fieldGroup
            required property string modelData

            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 3

            readonly property var members: (details.fields || []).filter(function(f) {
              return f.group === fieldGroup.modelData;
            })

            visible: fieldGroup.members.length > 0

            Label {
              text: fieldGroup.modelData
              font.pixelSize: 11
              font.bold: true
              opacity: 0.55
              Layout.fillWidth: true
              Layout.topMargin: 4
            }

            Repeater {
              model: fieldGroup.members

              delegate: SpriteField {
                required property var modelData
                Layout.fillWidth: true

                fieldData: modelData
                slot: details.slot
              }
            }
          }
        }
      }
    }
  }
}
