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
 * ⚠️ **REBUILT 2026-07-13.** The first cut was a list of raw byte boxes under headings called Who,
 * Where and When, and Twilight took it apart -- rightly. What changed, and why, is written up in
 * MapModel::npcFields (the schema) and SpriteField.qml (the controls). The short version:
 *
 *   * every field declares a **kind**, and the kind picks the control -- a picture is a grid of
 *     ARTWORK, an X/Y pair is ONE control, a countdown is drawn as a countdown;
 *   * the raw number box appears **only when the combo cannot say the value**;
 *   * the "Talking to it" group **changes shape with the sprite**: the item picker exists for an
 *     item ball and the trainer roster for a trainer, because the save's own bits say which it is;
 *   * a byte the console recomputes on load wears a yellow **!**;
 *   * the panel's paragraph is a **?** in the title bar, and the ✕ is a **Delete** button that says
 *     Delete.
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

  /// The player is slot 0. He is selectable and draggable like anybody else (Twilight, 2026-07-13),
  /// but he has no NPC record -- his bytes live in the save's player block, not the sprite table --
  /// so he gets his own short list rather than an empty one.
  readonly property bool hasPlayer: slot === 0

  /// The panel's "?". MapDock puts it in the title bar; it is the one tooltip icon this panel gets.
  readonly property string panelInfo: qsTr(
    "Everything the save holds about whoever is selected — and it is genuinely everything, including "
    + "the values no real game would ever write. Nothing here is refused and nothing is quietly "
    + "corrected.\n\n"
    + "A yellow ! means the game works that value out again when it loads your save: real bytes, "
    + "yours to set, but they won't survive Continue.\n\n"
    + "Nothing selected? Then this is the map itself.")

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

  // ⚠️ These strings must match MapModel::npcFields' group names EXACTLY -- they are what the rows
  // are filtered by. (Was Who / Where / When, which Twilight called "really really dumb", and she is
  // right: they told you nothing about what was under them.)
  readonly property var groupOrder: ["Character", "Where", "Movement", "Talking to it",
                                     "Right now", "The drawing"]

  ScrollView {
    id: scroller
    anchors.fill: parent
    clip: true
    contentWidth: availableWidth          // never scroll sideways; the rows wrap instead
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    // ⚠️ RESERVE THE SCROLLBAR'S LANE -- 16px. The bar is an overlay, so full-width content ends up
    // underneath it. Here that put the yellow "!" icons (right-anchored on each field's label row)
    // behind the scrollbar. See ui-patterns.md; it is a recurring gotcha and this is the fix.
    ColumnLayout {
      width: scroller.availableWidth - 16
      spacing: 8

      // ── Nothing selected: the MAP's own details ───────────────────────────────────────────
      //
      // The panel is never blank. Editing "what this map is" has one home, and this is it.
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 6
        visible: !details.hasSprite && !details.hasPlayer

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

      // ── The PLAYER selected ──────────────────────────────────────────────────────────────
      //
      // He is slot 0, he is selectable and draggable like anybody else -- and he does not live in
      // the sprite table, so `npcFields` has nothing for him. His position is the one thing about
      // him that is genuinely a map fact, and the canvas already edits it by dragging; the rest of
      // him (name, badges, money, the lot) is the Trainer Card's job, not the map's.
      ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: 10
        spacing: 8
        visible: details.hasPlayer

        RowLayout {
          Layout.fillWidth: true
          spacing: 8

          Image {
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            source: "image://player/npc/1/0/" + brg.map.contrast
            smooth: false
            fillMode: Image.PreserveAspectFit
          }

          ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Label {
              Layout.fillWidth: true
              text: qsTr("The player")
              font.bold: true
              font.pixelSize: 14
              elide: Text.ElideRight
            }

            Label {
              text: qsTr("Slot 0 — the game requires him")
              font.pixelSize: 10
              opacity: 0.6
            }
          }
        }

        Label {
          Layout.fillWidth: true
          Layout.topMargin: 2
          text: qsTr("Where he is standing")
          font.pixelSize: 11
          color: brg.settings.textColorMid
        }

        RowLayout {
          Layout.fillWidth: true
          spacing: 6

          Label { text: qsTr("X"); font.pixelSize: 10; opacity: 0.6 }

          SpinBox {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredHeight: 28
            font.pixelSize: 11
            editable: true
            from: 0
            to: 255
            value: brg.map.playerX
            onValueModified: brg.map.movePlayer(value, brg.map.playerY)
          }

          Label { text: qsTr("Y"); font.pixelSize: 10; opacity: 0.6 }

          SpinBox {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredHeight: 28
            font.pixelSize: 11
            editable: true
            from: 0
            to: 255
            value: brg.map.playerY
            onValueModified: brg.map.movePlayer(brg.map.playerX, value)
          }
        }

        // Moving him invalidates the view pointer the GAME computed. We say so and offer the fix --
        // we never quietly rewrite it. (The derived-byte doctrine; map-screen.md.)
        Rectangle {
          Layout.fillWidth: true
          Layout.topMargin: 6
          visible: !brg.map.headerMatches
          radius: 6
          color: Qt.rgba(1, 0.84, 0.31, 0.15)
          border.width: 1
          border.color: "#ffd54f"
          implicitHeight: pFixCol.implicitHeight + 16

          ColumnLayout {
            id: pFixCol
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            Label {
              Layout.fillWidth: true
              text: qsTr("The map view pointer the game computed no longer matches where he is.")
              wrapMode: Text.Wrap
              font.pixelSize: 11
            }

            Button {
              text: qsTr("Fix it")
              onClicked: brg.map.fixMapHeader()
            }
          }
        }

        Label {
          Layout.fillWidth: true
          Layout.topMargin: 6
          text: qsTr("His name, his badges and everything else about him live on the Trainer Card.")
          wrapMode: Text.Wrap
          font.pixelSize: 10
          opacity: 0.55
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
        }

        // ⚠️ A DELETE BUTTON THAT SAYS DELETE.
        //
        // It was a "✕" ToolButton with a tooltip. Twilight: *"The x button deletes — it's not
        // self-explanatory, should be delete button."* A destructive action gets a word, not a glyph
        // you have to hover to identify.
        Button {
          Layout.fillWidth: true
          Layout.topMargin: 2
          Layout.preferredHeight: 28
          font.pixelSize: 11

          text: qsTr("Delete this character")

          contentItem: Label {
            text: parent.text
            font: parent.font
            color: parent.hovered ? "#ffffff" : "#d55e00"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
          }

          background: Rectangle {
            radius: 4
            color: parent.hovered ? "#d55e00" : "transparent"
            border.width: 1
            border.color: "#d55e00"

            Behavior on color { ColorAnimation { duration: 90 } }
          }

          onClicked: {
            brg.map.removeNpc(details.slot);
            if (details.canvas)
              details.canvas.selectedNpc = -1;
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

        // (A blue "this map's cast no longer matches the game's" notice sat here. REMOVED 2026-07-13
        // -- Twilight: "do not have notice on cast no longer matches". It fired on every edit, said
        // the same thing every time, and pushed the fields you were editing down the panel. The fact
        // it carried -- the game rebuilds a map's cast from ROM when you walk back in -- is still
        // true, and it is written down where a fact belongs: notes/reference/sprites.md, Part 6.)

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
                canvas: details.canvas   // so the picture picker can mute the ground while it is up
              }
            }
          }
        }
      }
    }
  }
}
